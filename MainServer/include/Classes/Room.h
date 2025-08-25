#ifndef CLASS_ROOM_HEADER
#define CLASS_ROOM_HEADER

#include <cstdint>
#include <vector>
#include <string>
#include "../Structures/Room/ClientRoomCreationInfo.h"
#include "../Structures/Room/RoomPlayerInfo.h"
#include "../Structures/Room/RoomsList.h"
#include "../Structures/Room/RoomJoin.h"
#include "../Structures/Room/RoomPlayerItems.h"
#include "../Structures/Room/RoomPlayerClan.h"
#include "../Structures/Room/RoomSettingsUpdate.h"
#include "Player.h"
#include "../Network/MainSession.h"
#include "../Structures/Room/RoomPlayerInfo.h"
#include "../../include/Structures/EndScoreboard.h"

namespace Main
{
	namespace Classes
	{
		class Room
		{
		private:
			inline static std::uint16_t idCounter{};
			inline static std::uint16_t idCounterClans{ 150 }; // default value on purpose

			std::uint16_t m_number{};
			std::string m_title{};   
			std::string m_password{}; 
			bool m_hasMatchStarted{};
			bool m_isTeamBalanceOn{}; // Could not find this in any structure client<=>server, for now keep it like a separate variable...
			std::uint8_t m_specificSetting{}; // E.g eli = > num of rounds, TDM = > num of total kills / 10
			Main::Structures::RoomSettings m_settings{};
			std::uint32_t m_actualMap;

			std::vector<std::pair<Main::Structures::RoomPlayerInfo, std::weak_ptr<Main::Network::Session>>> m_players{};
			std::vector<std::pair<Main::Structures::RoomPlayerInfo, std::weak_ptr<Main::Network::Session>>> m_observerPlayers{};

			std::vector<std::pair<std::uint32_t, std::string>> m_kickedPlayerAccountIds{};
			std::vector<std::uint32_t> m_votekickStarters{}; // each player has only 1 votekick per match

			// Points
			std::uint32_t m_bluePoints{};
			std::uint32_t m_redPoints{};

			// Other
			bool m_isMuted = false;
			bool m_isClanRoom = false;
			std::uint64_t m_matchStartTime = 0;

			// Network 
			Common::Network::Packet m_packet;

			// Votekick system
			std::uint32_t m_votekickYesses{};
			std::uint32_t m_votekickReason{};
			Main::Structures::UniqueId m_votekickTargetUid{};
			std::string m_votekickTargetNickname{};

			// Missions
			static inline std::unordered_map<std::uint32_t, std::uint32_t> weaponToMissionIndex
			{
				{0, 1}, // melee
				{1, 2}, // rifle
				{3, 3}, // sniper
				{5, 4}, // bazooka
				{6, 5} // grenade
			};

			// Special modes
			bool m_isAssassinMode{};
			bool m_isCsdMode{};

		private:
			Main::Structures::RoomPlayerInfo createRoomPlayerInfo(std::shared_ptr<Main::Network::Session> session, std::uint32_t team) const;
			void sendHostChangePacket(std::uint32_t playerIdx, Common::Enums::RoomChangeHostExtra result);
			void removePlayerFromRoomAndMatch(std::uint32_t playerIdx, Main::Structures::UniqueId& originalHostUniqueId, std::uint32_t extra = 1);
			void setStateFor(std::pair<Main::Structures::RoomPlayerInfo, std::weak_ptr<Main::Network::Session>>& player, Common::Enums::PlayerState state);
			void toMatchExceptSelfHelper(Common::Network::Packet& packet, std::shared_ptr<Main::Network::Session> session);

			void toMatchDeadExceptSelfHelper(Common::Network::Packet& packet, std::shared_ptr<Main::Network::Session> givenSession, auto& players)
			{
				if (!givenSession) return;
				for (auto& [roomInfo, weakSession] : players)
				{
					if (auto session = weakSession.lock())
					{
						if (roomInfo.uniqueId != givenSession->getAccountInfo().uniqueId &&
							session->getPlayer().getPlayerState() == Common::Enums::STATE_DYING)
						{
							session->asyncWrite(packet);
						}
					}
				}
			}

			bool movePlayerHelper(auto& from, auto& to, auto it, std::uint32_t team)
			{
				auto playerInfo = std::move(*it);
				playerInfo.first.team = team;
				to.push_back(std::move(playerInfo));
				from.erase(it);
				return true;
			}

			auto findPlayer(std::uint32_t sessionId)
			{
				return std::find_if(m_players.begin(), m_players.end(),
					[=](const auto& player) {
						auto session = player.second.lock();
						return session && session->getId() == sessionId;
					});
			}

			const auto findPlayer(std::uint32_t sessionId) const
			{
				return std::find_if(m_players.cbegin(), m_players.cend(),
					[=](const auto& player) {
						auto session = player.second.lock();
						return session && session->getId() == sessionId;
					});
			}

			auto findObserverPlayer(std::uint32_t sessionId)
			{
				return std::find_if(m_observerPlayers.begin(), m_observerPlayers.end(),
					[=](const auto& obs) {
						auto session = obs.second.lock();
						return session && session->getId() == sessionId;
					});
			}

			const auto findObserverPlayer(std::uint32_t sessionId) const
			{
				return std::find_if(m_observerPlayers.cbegin(), m_observerPlayers.cend(),
					[=](const auto& obs) {
						auto session = obs.second.lock();
						return session && session->getId() == sessionId;
					});
			}

			std::uint32_t getPlayersPerTeam() const
			{
				if (m_settings.playersPerTeam == 1) // 1v1 or 2 players in square mode
					return 2;
				if (m_settings.mode == Common::Enums::SquareMode && m_settings.playersPerTeam == 0) // 1 player in square mode
					return 1;
				return m_settings.playersPerTeam * 2;
			}

			template <typename PlayerContainer>
			std::vector<Main::Structures::PlayerClan> getPlayersClansFrom(const PlayerContainer& players) const
			{
				std::vector<Main::Structures::PlayerClan> ret;
				for (std::size_t playerIdx = 0; const auto & [roomInfo, weakSession] : players)
				{
					auto session = weakSession.lock();
					if (!session) { ++playerIdx; continue; }

					const auto& accountInfo = session->getAccountInfo();
					if (accountInfo.clanId > 8)
					{
						Main::Structures::PlayerClan playerClan;
						playerClan.clanLogoBackId = accountInfo.clanLogoBackId;
						playerClan.clanLogoFrontId = accountInfo.clanLogoFrontId;
						std::memcpy(playerClan.clanName, accountInfo.clanName, 16);
						playerClan.unknown2 = accountInfo.clanId;
						playerClan.playerRoomIdx = playerIdx;
						ret.push_back(playerClan);
					}
					++playerIdx;
				}
				return ret;
			}

			template <typename Predicate>
			bool checkWeapons(Predicate&& pred, const std::string& failMessage) const
			{
				if (m_players.empty()) return false;

				auto& pair = m_players[0];
				auto hostSession = pair.second.lock();
				if (!hostSession) return false;

				bool ret = true;
				for (auto& [roomInfo, session] : m_players)
				{
					if (auto actSession = session.lock();
						actSession && roomInfo.state == Common::Enums::STATE_READY && !pred(actSession))
					{
						hostSession->sendMessage(
							"(error) Player " + std::string{ actSession->getAccountInfo().nickname } + " " + failMessage
						);
						ret = false;
					}
				}
				return ret;
			}


		public:
			Room() = default;

			explicit Room(const std::string& title, const Main::Structures::RoomSettings& settings, const Main::Structures::RoomPlayerInfo& player,
				std::shared_ptr<Main::Network::Session> session, bool isClanRoom = false);

			std::string getRoomInfoAsString() const
			{
				const std::size_t totalPlayersInMatch = std::count_if(m_players.begin(), m_players.end(), [](const auto& currentPlayer) {
					if (auto session = currentPlayer.second.lock(); session)
						return session->getPlayer().isInMatch();
					return false;
					});

				return "[RoomNumber:" + std::to_string(m_number) + "] [HasMatchStarted:" + std::to_string(m_hasMatchStarted)
					+ "] [Players:" + std::to_string(m_players.size()) + "] [ObserverPlayers:" + std::to_string(m_observerPlayers.size()) + "]"
					+ " [InMatch:" + std::to_string(totalPlayersInMatch) + "]";
			}

			std::size_t getPlayersSize() /* obs + non obs */ const { return m_players.size(); }

			// Setters
			void addPlayer(std::shared_ptr<Main::Network::Session> session, std::uint32_t team);
			void addObserverPlayer(std::shared_ptr<Main::Network::Session> session);
			void removeAllPlayers(std::uint32_t extra = 1);
			void removeAllObserverPlayers(std::uint32_t extra = 1);
			void updatePlayerInfo(std::shared_ptr<Main::Network::Session> session);
			void addKickedPlayer(std::uint32_t accountId, const std::string& nickname);
			bool removeHostFromMatch(std::uint32_t leaveTypeExtra = 1);
			bool removePlayer(std::shared_ptr<Main::Network::Session> session, std::uint32_t extra);
			void breakroom();
			bool changeHost(std::size_t newHostIdx);
			bool changeHostByNickname(const std::string& nickname);
			void updatePlayersTeamToTeamBased();
			void updatePlayersTeamToNonTeamBased();
			void setStateFor(const Main::Structures::UniqueId& uniqueId, const Common::Enums::PlayerState& playerState);
			void setPlayersPerTeam(std::uint16_t playersPerTeam);
			bool kickPlayer(const std::string& name);
			bool votekickPlayer(const Main::Structures::UniqueId& uniqueId);
			void startMatch();
			void endMatch();
			void updateMap(std::uint16_t newMap);
			void updateRoomSettings(const Main::Structures::RoomSettingsUpdateBase& newRoomSettings, std::uint16_t newMode);
			void updateTitle(const std::string& newTitle);
			void updatePassword(const std::string& newPassword);
			void setPassword(const std::string& password);
			void addPoint(std::uint32_t team);
			void sendTo(const Main::Structures::UniqueId& uniqueId, const Common::Network::Packet& packet);
			void storeEndMatchStatsFor(const Main::Structures::UniqueId& uniqueId, const Main::Structures::ScoreboardResponse& stats,
				std::uint32_t blueScore, std::uint32_t redScore, bool hasLeveledUp, const Main::Structures::EventMissionInfo& eventMissionInfo);
			void setSpecificSetting(std::uint8_t setting);
			void setTime(std::uint16_t time);
			bool changePlayerTeam(const Main::Structures::UniqueId& uniqueId, std::uint32_t newTeam);
			std::optional<std::uint32_t> getTeamForSession(std::uint32_t sessionId) const;
			bool isEveryoneCsd() const;
			bool isEveryoneBasic() const;

			void muteRoom();
			void unmuteRoom();	
			bool isMuted() const;

			void playerRoomLeaveNotification(const Main::Structures::UniqueId& uniqueId, std::size_t playerIdx, decltype(m_players)& container, std::uint32_t extra = 1);

			std::shared_ptr<Main::Network::Session> getPlayer(const Main::Structures::UniqueId& uniqueId);
			std::uint32_t getBestMsIndexExceptSelf(std::uint64_t selfId, bool checkInMatch);
			const std::string& getPassword() const;
			bool playerExists(std::uint32_t sessionId) const;
			std::uint16_t getRoomNumber() const;
			std::vector<Main::Structures::RoomPlayerInfo> getAllPlayers() const;
			std::vector<std::pair<Main::Structures::RoomPlayerInfo, std::shared_ptr<Main::Network::Session>>> getAllPlayersWithSessions() const;
			Main::Structures::SingleRoom getRoomInfo() const;
			const Main::Structures::RoomSettings& getRoomSettings() const;
			Main::Structures::RoomJoin getRoomJoinInfo() const;
			std::vector<Main::Structures::RoomPlayerItems> getPlayersItems() const;
			std::vector<Main::Structures::PlayerClan> getObserverPlayersClans() const;
			std::vector<Main::Structures::PlayerClan> getPlayersClans() const;
			bool isHost(const Main::Structures::UniqueId& uniqueId) const;
			Common::Enums::Team calculateNewPlayerTeam() const;
			bool isModeTeamBased() const;
			std::uint8_t getSpecificSetting() const;
			const std::string& getRoomTitle() const;
			bool isRoomFullObserverExcluded() const;
			bool hasMatchStarted() const;
			bool isObserverFull() const;
			std::size_t getPlayersCount() const;
			std::uint32_t getHostLevel() const;
			bool wasPreviouslyKicked(std::uint32_t accountId) const;
			bool removeKickedPlayerByNickname(const std::string& nickname);
			std::vector<std::string> getKickedPlayerNicknames() const;
			Main::Structures::RoomSettingsUpdateTitlePassword getRoomSettingsUpdate() const;
			Main::Network::Session::AccountInfo getAccountInfoFor(const Main::Structures::UniqueId& uniqueId) const;

			void broadcastToRoom(Common::Network::Packet& packet);
			void broadcastToRoomExceptSelf(Common::Network::Packet& packet, const Main::Structures::UniqueId& uniqueId);

			void randomizeTeams();

			// In-room chat messages
			void broadcastToTeamExceptSelf(Common::Network::Packet& packet, std::shared_ptr<Main::Network::Session> givenSession);
			void broadcastToDeadTeamExceptSelf(Common::Network::Packet& packet, std::shared_ptr<Main::Network::Session> givenSession);
			void broadcastToMatchExceptSelf(Common::Network::Packet& packet, std::shared_ptr<Main::Network::Session> session, std::uint32_t extra);
			void broadcastOutsideMatchExceptSelf(Common::Network::Packet& packet, std::shared_ptr<Main::Network::Session> givenSession, std::uint32_t extra);
			void broadcastMessage(const std::string& message);
			void broadcastToDeadExceptSelf(Common::Network::Packet& packet, std::shared_ptr<Main::Network::Session> session, std::uint32_t extra);
			void broadcastToTeam(Common::Network::Packet& packet, Common::Enums::Team team);

			// Other
			void generateMapIfRandom();
			std::uint32_t getActualMap();
			void votekickVoteYes();
			std::pair<std::uint32_t, std::string> getVotekickResultDetails();
			void resetVotekick();
			bool startVotekick(const Main::Structures::UniqueId& uniqueId, const std::string& targetNickname, std::uint32_t reason, std::uint32_t starterAid);
			const Main::Structures::UniqueId& getTargetVotekickUid() const noexcept { return m_votekickTargetUid; }
			std::uint32_t getVotekickReason() const noexcept { return m_votekickReason; }
			const std::string& getTargetVotekickNickname() const noexcept { return m_votekickTargetNickname; }
			std::uint32_t getTotalPlayersInMatch() const;
			std::uint32_t getObserversSize() const noexcept{ return m_observerPlayers.size(); }
			std::size_t getPlayersSizeNoObserver() const noexcept { return m_players.size(); }

		private:
			bool kickPlayerImpl(std::shared_ptr<Main::Network::Session> session, std::uint32_t extra);

		public:
			template<Main::Enums::RoomSimpleSetting T> requires (T == Main::Enums::SETTING_ITEM)
			void switchSimpleSetting()
			{
				m_settings.isItemOn = ~m_settings.isItemOn;
			}

			template<Main::Enums::RoomSimpleSetting T> requires (T == Main::Enums::SETTING_OPEN)
			void switchSimpleSetting()
			{
				m_settings.isOpen = ~m_settings.isOpen;
			}

			template<Main::Enums::RoomSimpleSetting T> requires (T == Main::Enums::SETTING_TEAMBALANCE)
			void switchSimpleSetting()
			{
				m_isTeamBalanceOn = !m_isTeamBalanceOn;
			}

			template<Main::Enums::RoomSimpleSetting T> requires (T == Main::Enums::SETTING_OBSERVER)
			void switchSimpleSetting()
			{
				m_settings.isObserverModeOn = ~m_settings.isObserverModeOn;
				if (!m_settings.isObserverModeOn)
				{
					removeAllObserverPlayers();
				}
			}

			std::uint64_t getMatchStartTime() const
			{
				return m_matchStartTime;
			}

			bool isClanRoom() const noexcept
			{
				return m_isClanRoom;
			}

			bool setAssassinMode()
			{
				m_isAssassinMode = !m_isAssassinMode;
				return m_isAssassinMode;
			}

			bool isAssassinMode() const
			{
				return m_isAssassinMode && m_settings.mode == Common::Enums::Elimination;
			}

			bool setCsdMode()
			{
				m_isCsdMode = !m_isCsdMode;
				return m_isCsdMode;
			}

			bool isCsdMode() const noexcept { return m_isCsdMode; }

			bool assassinModeEnoughPlayers() const noexcept;

			std::optional<std::pair<Main::Structures::UniqueId, std::string>> getRandomAssassinFrom(Common::Enums::Team team, bool fromReadyPlayers);

			std::string getPlayerNameByIndex(std::uint32_t index) const
			{
				if (index >= m_players.size()) return "error";
				if (auto session = m_players[index].second.lock(); session)
					return session->getPlayer().getPlayerName();
				return "disconnected";
			}
		};
	}
}

#endif

