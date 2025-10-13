#ifndef CLASS_CLAN_HEADER
#define CLASS_CLAN_HEADER

#include <cstdint>
#include <vector>
#include <string>
#include <expected>

#include "../Network/MainSession.h"
#include "../Structures/Clan/ClanStructures.h"
#include "../Structures/Room/RoomPlayerInfo.h"
#include "../Structures/Room/ClientRoomCreationInfo.h"
#include <source_location>
#include <expected>
#include <cstring> 

namespace Main
{
	namespace Classes
	{
		class ClanRoom
		{
		private:
			Main::Structures::ClanRoomSettings m_settings;
			Main::Structures::PartyInfo m_partyInfo;
			bool m_isRegistered{};
			std::vector<std::pair<Main::Structures::PartyPlayerInfo, std::weak_ptr<Main::Network::Session>>> m_waitingPlayers{}; // leader = 0
			Common::Enums::Team m_team{};

			enum LeaderChangeResult
			{
				NO_PLAYERS_LEFT,
				ERR,
				SUCCESS
			};

		public:
			explicit ClanRoom(std::shared_ptr<Main::Network::Session> session, std::uint16_t clanRoomNumber, const Main::ClientData::ClanRoomSettings& settings)
				: m_settings{ settings }
			{
				const auto& selfInfo = session->getAccountInfo();

				std::memcpy(m_partyInfo.leaderName, selfInfo.nickname, Common::Constants::maxNicknameSize);
				m_partyInfo.leaderLevel = selfInfo.playerLevel;
				m_partyInfo.clanRoomId = selfInfo.clanId;
				m_partyInfo.clanRoomNumber = clanRoomNumber;

				Main::Structures::PartyPlayerInfo waitingPlayerInfo;
				waitingPlayerInfo.uid = selfInfo.uniqueId;
				waitingPlayerInfo.level = selfInfo.playerLevel;
				std::memcpy(waitingPlayerInfo.nickname, selfInfo.nickname, Common::Constants::maxNicknameSize);
				waitingPlayerInfo.totalClanDraws = selfInfo.clanDraws;
				waitingPlayerInfo.totalClanLosses = selfInfo.clanLosses;
				waitingPlayerInfo.totalClanWins = selfInfo.clanWins;
				waitingPlayerInfo.clanContribution = selfInfo.clanContribution;

				m_waitingPlayers.emplace_back(waitingPlayerInfo, session);

				session->setClanRoomNumber(m_partyInfo.clanRoomNumber);
			}

			void addPlayer(std::shared_ptr<Main::Network::Session> session)
			{
				const auto& selfInfo = session->getAccountInfo();

				Main::Structures::PartyPlayerInfo waitingPlayerInfo;
				waitingPlayerInfo.uid = selfInfo.uniqueId;
				waitingPlayerInfo.level = selfInfo.playerLevel;
				std::memcpy(waitingPlayerInfo.nickname, selfInfo.nickname, Common::Constants::maxNicknameSize);
				waitingPlayerInfo.totalClanDraws = selfInfo.clanDraws;
				waitingPlayerInfo.totalClanLosses = selfInfo.clanLosses;
				waitingPlayerInfo.totalClanWins = selfInfo.clanWins;
				waitingPlayerInfo.clanContribution = selfInfo.clanContribution;

				m_waitingPlayers.emplace_back(waitingPlayerInfo, session); 

				++m_partyInfo.numPlayers;

				session->setClanRoomNumber(m_partyInfo.clanRoomNumber);
			}

			void updatePartyStatus(bool hasStarted)
			{
				broadcastMessageToWaitingPlayers("Match Started: " + std::to_string(hasStarted));
				if (hasStarted)
				{
					m_isRegistered = false;
				}
				m_partyInfo.hasMatchStarted = hasStarted;
			}

			bool hasMatchStarted() const noexcept
			{
				return m_partyInfo.hasMatchStarted;
			}

			void removeAllPlayers()
			{
				Common::Network::Packet removePlayerPacket;
				removePlayerPacket.setTcpHeader(0, Common::Enums::NO_ENCRYPTION);
				removePlayerPacket.setCommand(111, 0, 1, 0);

				for (auto& [partyInfo, weakSession] : m_waitingPlayers)
				{
					if (auto session = weakSession.lock())
					{
						session->asyncWrite(removePlayerPacket);
						session->setClanRoomNumber(0);
					}
				}

				m_partyInfo.numPlayers = 0;
				m_waitingPlayers.clear();
			}

			void broadcastChatMessage(const std::string& message)
			{
				for (auto& [partyInfo, weakSession] : m_waitingPlayers)
				{
					if (auto session = weakSession.lock())
						session->sendMessage(message);
				}
			}

			std::optional<std::uint32_t> getPlayerIndex(std::uint32_t sessionId)
			{
				for (std::uint32_t idx = 0; const auto & [_, weakSession] : m_waitingPlayers)
				{
					if (auto session = weakSession.lock(); session && session->getAccountInfo().uniqueId.session == sessionId)
					{
						return idx;
					}
					++idx;
				}
				return std::nullopt;
			}

			// Returns true if the room must also be closed
			std::optional<bool> removePlayer(std::uint16_t sessionId)
			{
				auto it = std::find_if(m_waitingPlayers.begin(), m_waitingPlayers.end(), [sessionId](const auto& player) {
					if (auto session = player.second.lock())
						return session->getAccountInfo().uniqueId.session == sessionId;
					return false;
					});

				if (it == m_waitingPlayers.end())
				{
					broadcastMessageToWaitingPlayers("Player to be removed was NOT found in the room [1]");
					return std::nullopt;
				}

				auto lockedSession = it->second.lock();
				if (!lockedSession)
				{
					broadcastMessageToWaitingPlayers("Player session expired before removal [1]");
					return std::nullopt;
				}

				const auto sessionToRemove = lockedSession->getAccountInfo().uniqueId.session;

				if (isLeader(sessionId))
				{
					auto newLeaderIdxOpt = changeLeaderToFirstAvailable();

					if (!newLeaderIdxOpt)
					{
						broadcastMessageToWaitingPlayers("newLeaderIdxOpt nullopt -- New leader NOT chosen");
						return std::nullopt;
					}

					if (*newLeaderIdxOpt != 0)
					{
						Common::Network::Packet response;
						response.setTcpHeader(0, Common::Enums::NO_ENCRYPTION);
						response.setCommand(114, 0, 0, *newLeaderIdxOpt);
						broadcastToWaitingPlayers(response);
					}
				}

				it = std::find_if(m_waitingPlayers.begin(), m_waitingPlayers.end(), [sessionToRemove](const auto& player) {
					if (auto session = player.second.lock())
						return session->getAccountInfo().uniqueId.session == sessionToRemove;
					return false;
					});

				if (it == m_waitingPlayers.end())
				{
					broadcastMessageToWaitingPlayers("Player to be removed was NOT found in the room [2]");
					return std::nullopt;
				}

				if (auto session = it->second.lock())
				{
					session->setClanRoomNumber(0);
				}
				else
				{
					broadcastMessageToWaitingPlayers("Player session expired before removal [2]");
				}

				m_waitingPlayers.erase(it);

				if (m_partyInfo.numPlayers)
				{
					--m_partyInfo.numPlayers;
				}
				else
				{
					broadcastMessageToWaitingPlayers("Failed to decrement m_partyInfo.numPlayers");
				}

				if (m_waitingPlayers.empty())
				{
					m_partyInfo.numPlayers = 0;
					return true;
				}

				return false;
			}


			std::uint32_t getRoomNumber() const noexcept
			{
				return m_partyInfo.clanRoomNumber;
			}

			std::uint32_t getClanId() const	noexcept
			{
				return m_partyInfo.clanRoomId;
			}

			void updateMap(std::uint16_t map)
			{
				if (map >= Common::Enums::MAPS_MAX) return;
				m_settings.map = map;
			}

			std::shared_ptr<Main::Network::Session> getLeaderSession() const noexcept
			{
				if (m_waitingPlayers.empty())
					return nullptr;

				return m_waitingPlayers[0].second.lock();
			}

			Main::Structures::RoomSettings getRoomSettings() const
			{
				const std::uint32_t time = m_settings.mode == Common::Enums::Clan_TeamDeathMatch ? 10 : 2;
				return Main::Structures::RoomSettings{ time, Common::Enums::WeaponRestriction::All, 0 /*itemOn*/, m_settings.mode, 0 /*isOpen*/,
					0 /*hasPassword*/, m_settings.playersPerTeam, m_settings.map,  0/*obs*/
				};
			}

			std::uint32_t getSpecificSetting() const noexcept
			{ // num rounds or total kills for TDM
				return m_settings.mode == Common::Enums::Clan_TeamDeathMatch ? 80 : 5;
			}

			void updateMode(std::uint16_t mode)
			{
				if (mode >= Common::Enums::CLANMODES_MAX) return;
				m_settings.mode = mode;
			}

			bool isLeader(std::uint16_t sessionId) const noexcept
			{
				if (m_waitingPlayers.empty())
					return false;

				if (auto leaderSession = m_waitingPlayers[0].second.lock())
					return leaderSession->getAccountInfo().uniqueId.session == sessionId;

				return false;
			}

			bool canRegister() const noexcept
			{
				return m_waitingPlayers.size() >= m_partyInfo.maxPlayers;
			}

			bool changeLeaderTo(std::uint16_t idx)
			{
				if (idx == 0 || idx >= m_waitingPlayers.size())
					return false;

				auto sessionPtr = m_waitingPlayers[idx].second.lock();
				if (!sessionPtr)
				{
					broadcastMessageToWaitingPlayers("Can't change leader: selected session is no longer valid - please report this issue");
					return false;
				}

				broadcastMessageToWaitingPlayers("Leader changed to: " + std::string(sessionPtr->getAccountInfo().nickname));

				std::swap(m_waitingPlayers[0], m_waitingPlayers[idx]);
				m_partyInfo.leaderLevel = m_waitingPlayers[0].first.level;
				std::memcpy(m_partyInfo.leaderName, m_waitingPlayers[0].first.nickname, Common::Constants::maxNicknameSize);
				return true;
			}


			void changeLeaderIfLeader(std::uint16_t sessionId)
			{
				if (isLeader(sessionId))
				{
					auto newLeaderIdxOpt = changeLeaderToFirstAvailable();

					if (!newLeaderIdxOpt)
					{
						broadcastMessageToWaitingPlayers("newLeaderIdxOpt nullopt -- New leader NOT chosen");
					}
					if (*newLeaderIdxOpt != 0)
					{
						Common::Network::Packet response;
						response.setTcpHeader(0, Common::Enums::NO_ENCRYPTION);
						response.setCommand(114, 0, 0, *newLeaderIdxOpt);
						broadcastToWaitingPlayers(response);
					}
				}
			}

			std::expected<std::size_t, std::string> changeLeaderToFirstAvailable()
			{
				if (m_waitingPlayers.size() <= 1)
					return 0;

				for (std::size_t idx = 1; idx < m_waitingPlayers.size(); ++idx)
				{
					if (auto session = m_waitingPlayers[idx].second.lock())
					{
						if (!changeLeaderTo(static_cast<std::uint16_t>(idx)))
							return std::unexpected("Failed to change leader to index " + std::to_string(idx));

						return idx;
					}
				}
				return std::unexpected("No valid leader found");
			}

			void updatePlayersPerTeam(std::uint16_t playersPerTeam)
			{
				m_settings.playersPerTeam = playersPerTeam;
				m_partyInfo.maxPlayers = playersPerTeam;
			}

			std::pair<std::uint16_t, std::uint16_t> getRoomId() const
			{
				return std::pair{ m_partyInfo.clanRoomId, m_partyInfo.clanRoomNumber };
			}

			const Main::Structures::ClanRoomSettings& getSettings() const noexcept
			{
				return m_settings;
			}

			bool isFull() const noexcept
			{
				return m_partyInfo.numPlayers >= m_partyInfo.maxPlayers;
			}

			std::vector<Main::Structures::PartyPlayerInfo> getWaitingPlayers() const noexcept
			{
				std::vector<Main::Structures::PartyPlayerInfo> waitingPlayers;
				for (const auto& [playerInfo, session] : m_waitingPlayers)
				{
					waitingPlayers.push_back(playerInfo);
				}
				return waitingPlayers;
			}

			std::vector<std::shared_ptr<Main::Network::Session>> getWaitingPlayerSessions() noexcept
			{
				std::vector<std::shared_ptr<Main::Network::Session>> waitingPlayers;
				for (const auto& [playerInfo, sessionWeak] : m_waitingPlayers)
				{
					if (auto session = sessionWeak.lock())
					{
						waitingPlayers.push_back(session);
					}
				}
				return waitingPlayers;
			}

			const Main::Structures::PartyInfo& getClanMatchInfo() const noexcept
			{
				return m_partyInfo;
			}

			void broadcastToWaitingPlayers(const Common::Network::Packet& packet)
			{
				for (const auto& [u, sessionWeak] : m_waitingPlayers)
				{
					if (auto session = sessionWeak.lock())
					{
						session->asyncWrite(packet);
					}
				}
			}

			void broadcastMessageToWaitingPlayers(const std::string& message)
			{
				for (const auto& [u, sessionWeak] : m_waitingPlayers)
				{
					if (auto session = sessionWeak.lock())
					{
						session->sendMessage(message, Main::Enums::TIP);
					}
				}
			}

			bool isRegistered() const noexcept
			{
				return m_isRegistered;
			}

			void switchRegistered()
			{
				m_isRegistered = !m_isRegistered;
			}

			void broadcastToWaitingPlayersExceptSelf(const Common::Network::Packet& packet, std::uint16_t sessionId)
			{
				for (const auto& [u, sessionWeak] : m_waitingPlayers)
				{
					if (u.uid.session == sessionId) continue;
					if (auto session = sessionWeak.lock())
					{
						session->asyncWrite(packet);
					}
				}
			}

			const char* getClanName() noexcept
			{
				if (m_waitingPlayers.empty()) return "";

				if (auto session = m_waitingPlayers[0].second.lock())
					return session->getAccountInfo().clanName;

				return "";
			}

			std::optional<Main::Structures::RegisteredClanInfo> getInfo() const
			{
				if (m_waitingPlayers.empty())
				{
					return std::nullopt;
				}

				auto session = m_waitingPlayers[0].second.lock();
				if (!session)
				{
					return std::nullopt;
				}

				const auto& leaderInfo = session->getAccountInfo();
				return Main::Structures::RegisteredClanInfo{
					static_cast<std::uint64_t>(m_settings.mode),
					static_cast<std::uint64_t>(m_settings.playersPerTeam * 2),
					static_cast<std::uint64_t>(m_settings.map),
					static_cast<std::uint64_t>(leaderInfo.playerLevel),
					static_cast<std::uint16_t>(leaderInfo.clanLogoFrontId),
					static_cast<std::uint16_t>(leaderInfo.clanLogoBackId),
					leaderInfo.clanName,
					leaderInfo.nickname,
					m_partyInfo.clanRoomId,
					m_partyInfo.clanRoomNumber
				};
			}

			void setTeam(Common::Enums::Team team)
			{
				m_team = team;
			}

			Common::Enums::Team getTeam() const noexcept
			{
				return m_team;
			}

			void storeStats(Main::Persistence::MainScheduler& scheduler, const Main::ClientData::ClientEndingMatchHeader& stats)
			{
				Main::Enums::MatchEnd type = Main::Enums::MatchEnd::MATCH_DRAW;

				if ((stats.blueScore > stats.redScore && m_team == Common::Enums::TEAM_BLUE)
					|| (stats.redScore > stats.blueScore && m_team == Common::Enums::TEAM_RED))
				{
					type = Main::Enums::MATCH_WON;
				}
				else if ((stats.blueScore > stats.redScore && m_team == Common::Enums::TEAM_RED)
					|| (stats.redScore > stats.blueScore && m_team == Common::Enums::TEAM_BLUE))
				{
					type = Main::Enums::MATCH_LOST;
				}

				scheduler.immediatePersist(std::source_location::current(),  
					&Main::Persistence::PersistentDatabase::updateClanStats, m_partyInfo.clanRoomId, type);
			}
		};
	}
}

#endif

