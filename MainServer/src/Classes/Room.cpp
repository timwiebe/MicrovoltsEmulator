
#include <cstdint>
#include <vector>
#include <string>
#include "../../include/Structures/Room/ClientRoomCreationInfo.h"
#include "../../include/Structures/Room/RoomPlayerInfo.h"
#include "../../include/Structures/Room/RoomsList.h"
#include "../../include/Structures/Room/RoomJoin.h"
#include "../../include/Structures/Room/RoomPlayerItems.h"
#include "../../include/Structures/Room/RoomPlayerClan.h"
#include "../../include/Structures/Room/RoomSettingsUpdate.h"
#include "../../include/Classes/Player.h"
#include "../../include/Network/MainSession.h"
#include "../../include/Structures/Room/RoomPlayerInfo.h"
#include "../../include/Classes/Room.h"
#include "../../include/Handlers/Room/RoomLeaveHandler.h"
#include <Enums/PlayerEnums.h>
#include <Utils/Logger.h>
#include <range/v3/all.hpp>
#include "../../include/Detail/Utilities.h"
#include "../../include/Classes/RoomNumberManager.h"

#include <algorithm>
#include <random>
#include <cstring> 

namespace Main
{
	namespace Classes
	{
		// Refactored
		Room::Room(const std::string& title, const Main::Structures::RoomSettings& settings,
			const Main::Structures::RoomPlayerInfo& player,
			std::shared_ptr<Main::Network::Session> session, bool isClanRoom)
			: m_title{ title }
			, m_settings{ settings }
			, m_isClanRoom{ isClanRoom }
		{
			if (isClanRoom)
				m_number = Main::Classes::RoomNumberGenerator<Main::Enums::RoomType::Clan>::getInstance().generate().value_or(0);
			else
				m_number = Main::Classes::RoomNumberGenerator<Main::Enums::RoomType::Room>::getInstance().generate().value_or(0);

			m_players.reserve(Common::Constants::maxRoomPlayersNoObs);
			m_observerPlayers.reserve(Common::Constants::maxObserverPlayers);
			m_players.emplace_back(std::make_pair(player, session));
			m_packet.setTcpHeader(0, Common::Enums::NO_ENCRYPTION);
		}

		// Refactored
		Main::Structures::RoomPlayerInfo Room::createRoomPlayerInfo(std::shared_ptr<Main::Network::Session> session, std::uint32_t team) const
		{
			const auto& accountInfo = session->getAccountInfo();
			return Main::Structures::RoomPlayerInfo{ accountInfo.uniqueId, static_cast<std::uint32_t>(accountInfo.latestSelectedCharacter),
				team, static_cast<std::uint32_t>(accountInfo.playerLevel), accountInfo.nickname, Common::Enums::STATE_WAITING, session->getPlayer().getPing() };
		}

		void Room::addPlayer(std::shared_ptr<Main::Network::Session> session, std::uint32_t team)
		{
			if (!session) return;

			const auto sessionId = session->getAccountInfo().uniqueId.session;

			auto isDuplicate = [&](const auto& list) {
				return std::any_of(list.begin(), list.end(), [&](const auto& player) {
					auto locked = player.second.lock();
					return locked && locked->getAccountInfo().uniqueId.session == sessionId;
					});
				};

			if (isDuplicate(m_players) || isDuplicate(m_observerPlayers))
			{
				::Utils::Logger::log("[Room::addPlayer] Duplicate player detected - disconnected target session", ::Utils::LogType::Warning);
				return;
			}

			m_players.emplace_back(createRoomPlayerInfo(session, team), std::weak_ptr<Main::Network::Session>{ session });
			session->setRoomNumber(m_number);
		}

		void Room::addObserverPlayer(std::shared_ptr<Main::Network::Session> session)
		{
			if (!session) return;
			const auto sessionId = session->getAccountInfo().uniqueId.session;

			auto isDuplicate = [&](const auto& list) {
				return std::any_of(list.begin(), list.end(), [&](const auto& player) {
					auto locked = player.second.lock();
					return locked && locked->getAccountInfo().uniqueId.session == sessionId;
					});
				};

			if (isDuplicate(m_players) || isDuplicate(m_observerPlayers))
			{
				::Utils::Logger::log("[Room::addObserverPlayer] Duplicate player detected - disconnected target session", ::Utils::LogType::Warning);
				return;
			}

			m_observerPlayers.emplace_back(createRoomPlayerInfo(session, Common::Enums::TEAM_OBSERVER), std::weak_ptr<Main::Network::Session>{ session });
			session->setRoomNumber(m_number);
		}

		// Refactored
		void Room::setPassword(const std::string& password)
		{
			m_password = password;
		}

		const std::string& Room::getPassword() const
		{
			return m_password;
		}

		std::uint16_t Room::getRoomNumber() const
		{
			return m_number;
		}

		// Refactored
		void Room::updatePlayerInfo(std::shared_ptr<Main::Network::Session> session)
		{
			if (!session) return;

			if (auto player = findPlayer(session->getId()); player != m_players.end())
			{
				const auto& accountInfo = session->getAccountInfo();
				player->first.character = accountInfo.latestSelectedCharacter;
				player->first.level = accountInfo.playerLevel;
			}
			else if (auto obsPlayer = findObserverPlayer(session->getId()); obsPlayer != m_observerPlayers.end())
			{
				const auto& accountInfo = session->getAccountInfo();
				obsPlayer->first.character = accountInfo.latestSelectedCharacter;
				obsPlayer->first.level = accountInfo.playerLevel;
			}
			else
			{
				SEND_DEBUG_MESSAGE("[Room::updatePlayerInfo] error: session not found", *session);
			}
		}

		std::size_t Room::getPlayersCount() const
		{
			return m_players.size();
		}

		void Room::addKickedPlayer(std::uint32_t accountId, const std::string& nickname)
		{
			m_kickedPlayerAccountIds.push_back(std::pair{ accountId, nickname });
		}

		void Room::muteRoom()
		{
			m_isMuted = true;
		}

		void Room::unmuteRoom()
		{
			m_isMuted = false;
		}

		bool Room::isMuted() const
		{
			return m_isMuted;
		}

		// Note: At this point, the player identified by uniqueId must already been removed from m_players
		// this function sends a "notification" to all remaining players in the room about the player who left (so they don't see them anymore)
		void Room::playerRoomLeaveNotification(const Main::Structures::UniqueId& uniqueId, std::size_t targetPlayerIdx, decltype(m_players)& container,
			std::uint32_t extra)
		{
			if (targetPlayerIdx >= container.size())
			{
				for (const auto& [roomInfo, sessionWeak] : ranges::views::concat(m_players, m_observerPlayers))
				{
					if (auto session = sessionWeak.lock())
					{
						session->sendMessage("[Room::playerRoomLeaveNotification] some weird logic error happened, the room is closed");
					}
				}
				removeAllPlayers();
				return;
			}

			m_packet.setCommand(422, 0, 0, targetPlayerIdx); // sessionId set inside .broadcastToRoom()
			m_packet.setData(reinterpret_cast<const std::uint8_t*>(&uniqueId), sizeof(uniqueId));
			broadcastToRoomExceptSelf(m_packet, uniqueId);

			if (auto session = container[targetPlayerIdx].second.lock())
			{
				Common::Network::Packet playerLeavePacket;
				playerLeavePacket.setTcpHeader(session->getId(), Common::Enums::NO_ENCRYPTION);
				playerLeavePacket.setOrder(141);
				playerLeavePacket.setExtra(extra);

				session->leaveRoom();
				session->asyncWrite(playerLeavePacket); // this must go after sending 422, otherwise messages such as "kicked" aren't shown
			}

			if (&container == &m_players)
			{
				// Resort the vector as the client does (the host remains at index 0)
				// => just swap the position of the player that must be removed with the highest indexed player
				std::swap(container.back(), container[targetPlayerIdx]);
				container.pop_back();
			}
			else
			{
				container.erase(container.begin() + targetPlayerIdx);
			}
		}

		// Refactored
		void Room::removeAllPlayers(std::uint32_t extra)
		{
			auto removePlayers = [&](auto& players) {
				for (auto& player : players)
				{
					if (auto session = player.second.lock())
					{
						m_packet.setCommand(141, 0, extra, 0);
						session->asyncWrite(m_packet);
						session->leaveRoom();
					}
				}
				players.clear();
				};

			removePlayers(m_players);
			removePlayers(m_observerPlayers);
		}

		void Room::removeAllObserverPlayers(std::uint32_t extra)
		{
			auto removePlayers = [&](auto& players) {
				std::size_t targetPlayerIdx = 0;
				for (auto& player : players)
				{
					if (auto session = player.second.lock())
					{
						m_packet.setCommand(422, 0, 0, targetPlayerIdx++);
						auto uniqueId = session->getAccountInfo().uniqueId;
						m_packet.setData(reinterpret_cast<std::uint8_t*>(&uniqueId), sizeof(uniqueId));
						session->asyncWrite(m_packet);

						m_packet.setCommand(141, 0, extra, 0);
						session->asyncWrite(m_packet);
						session->leaveRoom();
					}
				}
				players.clear();
				};

			removePlayers(m_observerPlayers);
		}

		// Refactored
		std::uint32_t Room::getBestMsIndexExceptSelf(std::uint64_t selfId, bool checkInMatch)
		{
			if (m_players.size() <= 1) return static_cast<std::uint32_t>(-1);

			std::uint32_t bestMs = std::numeric_limits<std::uint32_t>::max();
			std::uint32_t bestMsPlayerIdx = std::numeric_limits<std::uint32_t>::max();
			bool foundBestMs = false;

			for (std::uint32_t currentIdx = 0; const auto & [roomInfo, weakSession] : m_players)
			{
				auto session = weakSession.lock();
				if (!session) continue;

				const auto& player = session->getPlayer();
				const bool isInMatch = player.isInMatch();

				if (selfId != session->getId() && (checkInMatch ? isInMatch : !isInMatch) && player.getPing() < bestMs)
				{
					bestMs = player.getPing();
					bestMsPlayerIdx = currentIdx;
					foundBestMs = true;
				}
				++currentIdx;
			}
			return foundBestMs ? bestMsPlayerIdx : static_cast<std::uint32_t>(-1);
		}

		void Room::sendHostChangePacket(std::uint32_t playerIdx, Common::Enums::RoomChangeHostExtra result)
		{
			if (playerIdx < m_players.size())
			{
				auto session = m_players[playerIdx].second.lock();
				if (session)
				{
					Common::Network::Packet hostChange;
					const std::uint64_t sessionId = session->getId();
					hostChange.setTcpHeader(sessionId, Common::Enums::NO_ENCRYPTION);
					hostChange.setOrder(128);
					hostChange.setOption(playerIdx);
					hostChange.setExtra(result);
					broadcastToRoom(hostChange);
				}
			}
		}

		void Room::removePlayerFromRoomAndMatch(std::uint32_t playerIdx, Main::Structures::UniqueId& originalHostUniqueId, std::uint32_t extra)
		{
			if (playerIdx < m_players.size())
			{
				auto session = m_players[playerIdx].second.lock();
				if (session)
				{
					Common::Network::Packet removePlayerFromMatchServerRequest;
					removePlayerFromMatchServerRequest.setTcpHeader(session->getId(), Common::Enums::NO_ENCRYPTION);
					removePlayerFromMatchServerRequest.setOrder(256);
					removePlayerFromMatchServerRequest.setData(reinterpret_cast<std::uint8_t*>(&originalHostUniqueId), sizeof(originalHostUniqueId));

					broadcastToRoomExceptSelf(removePlayerFromMatchServerRequest, originalHostUniqueId);
				}
			}
			playerRoomLeaveNotification(originalHostUniqueId, playerIdx, m_players, extra);
		}

		// Refactored
		bool Room::removeHostFromMatch(std::uint32_t leaveTypeExtra)
		{
			if (m_players.empty()) return false;

			auto hostSession = m_players[0].second.lock();
			if (!hostSession) return false;

			Main::Structures::UniqueId originalHostUniqueId = hostSession->getAccountInfo().uniqueId;
			const std::uint64_t originalHostSessionId = hostSession->getId();
			hostSession->setIsInMatch(false); 

			const std::size_t totalPlayersInMatch = std::count_if(
				m_players.begin(), m_players.end(), [](const auto& currentPlayer) {
					auto session = currentPlayer.second.lock();
					return session && session->getPlayer().isInMatch();
				});
			const std::uint32_t totalPlayersInRoom = static_cast<std::uint32_t>(m_players.size());

			if (totalPlayersInMatch >= 1)
			{ // At least one player is still inside the match, they get the host
				const std::uint32_t bestMsPlayerIdxInMatch = getBestMsIndexExceptSelf(originalHostSessionId, true);
				if (changeHost(bestMsPlayerIdxInMatch))
				{ // note: changeHost uses std::swap, the player to be removed now has index "bestMsPlayerIdxInMatch" (= previous host)
					sendHostChangePacket(bestMsPlayerIdxInMatch, Common::Enums::CHANGE_HOST_SUCCESS);
					removePlayerFromRoomAndMatch(bestMsPlayerIdxInMatch, originalHostUniqueId, leaveTypeExtra);
					return false;
				}
				else
				{
					sendHostChangePacket(0, Common::Enums::CHANGE_HOST_FAIL); 
					return true; // Close the room as we cannot proceed further
				}
			}
			else if (totalPlayersInRoom > 1)
			{ // Otherwise, check if there are players inside the room that can be changed to host
				const std::uint32_t bestMsIndexPlayerOutsideMatch = getBestMsIndexExceptSelf(originalHostSessionId, false);
				if (changeHost(bestMsIndexPlayerOutsideMatch))
				{ // note: changeHost uses std::swap, the player to be removed now has index "bestMsPlayerIdxInMatch" (= previous host)
					sendHostChangePacket(bestMsIndexPlayerOutsideMatch, Common::Enums::CHANGE_HOST_SUCCESS);
					// The previous host leaves the match, host->setIsInMatch(false) already set at this point
					removePlayerFromRoomAndMatch(bestMsIndexPlayerOutsideMatch, originalHostUniqueId, leaveTypeExtra);
					m_hasMatchStarted = false; // The host was the only one inside the match, so we end it
					return false; // Another player got the host in the room, no need to close it
				}
				else
				{
					sendHostChangePacket(0, Common::Enums::CHANGE_HOST_FAIL); 
					return true;
				}
			}
			return true; // If no valid host replacement found, close the room
		}

		void Room::generateMapIfRandom()
		{
			if (m_settings.map == Common::Enums::Random)
			{
				m_actualMap = Main::CdbUtils::getRandomMapForMode(m_settings.mode);
			}
		}

		std::uint32_t Room::getActualMap()
		{
			if (m_settings.map == Common::Enums::Random)
			{
				return m_actualMap;
			}
			return m_settings.map;
		}

		// Refactored
		bool Room::removePlayer(std::shared_ptr<Main::Network::Session> session, std::uint32_t extra)
		{
			if (m_players.empty()) return false;

			if (auto toRemovePlayerIter = findPlayer(session->getId()); toRemovePlayerIter != m_players.end())
			{
				if (auto playerSession = toRemovePlayerIter->second.lock(); playerSession)
				{
					Common::Network::Packet hostChange;
					hostChange.setTcpHeader(playerSession->getId(), Common::Enums::NO_ENCRYPTION);

					if (isHost(playerSession->getAccountInfo().uniqueId))
					{
						const std::uint32_t bestMsPlayerIndexOutsideMatch = getBestMsIndexExceptSelf(playerSession->getId(), false);

						if (changeHost(bestMsPlayerIndexOutsideMatch))
						{
							hostChange.setCommand(128, 0, Common::Enums::CHANGE_HOST_SUCCESS, bestMsPlayerIndexOutsideMatch);
							broadcastToRoom(hostChange);

							// revalidate iterator
							toRemovePlayerIter = findPlayer(session->getId());
							if (toRemovePlayerIter == m_players.end()) return m_players.empty();
						}
						else if (m_players.size() > 1)
						{
							hostChange.setCommand(128, 0, Common::Enums::CHANGE_HOST_FAIL, 0);
							broadcastToRoom(hostChange);
							return true; // Room must be closed
						}
					}

					if (auto finalSession = toRemovePlayerIter->second.lock(); finalSession)
					{
						playerRoomLeaveNotification(finalSession->getAccountInfo().uniqueId, std::distance(m_players.begin(), toRemovePlayerIter),
							m_players, extra);
					}
				}
				return m_players.empty();
			}
			else if (auto observerIter = findObserverPlayer(session->getId()); observerIter != m_observerPlayers.end())
			{
				if (auto obsSession = observerIter->second.lock(); obsSession)
				{
					playerRoomLeaveNotification(obsSession->getAccountInfo().uniqueId, std::distance(m_observerPlayers.begin(), observerIter),
						m_observerPlayers, extra);
				}
			}
			return false;
		}

		bool Room::kickPlayerImpl(std::shared_ptr<Main::Network::Session> session, std::uint32_t extra)
		{
			if (session->getPlayer().isInMatch() && isHost(session->getAccountInfo().uniqueId))
			{
				return removeHostFromMatch(extra);
			}
			else
			{
				return removePlayer(session, extra);
			}
		}

		// Refactored
		std::vector<Main::Structures::RoomPlayerInfo> Room::getAllPlayers() const
		{
			std::vector<Main::Structures::RoomPlayerInfo> allPlayers;
			allPlayers.reserve(m_players.size() + m_observerPlayers.size());

			for (const auto& [info, weakSession] : m_players)
			{
				if (weakSession.expired()) continue;
				allPlayers.push_back(info);
			}
			for (const auto& [info, weakSession] : m_observerPlayers)
			{
				if (weakSession.expired()) continue;
				allPlayers.push_back(info);
			}

			return allPlayers;
		}


		// Refactored
		std::vector<std::pair<Main::Structures::RoomPlayerInfo, std::shared_ptr<Main::Network::Session>>> Room::getAllPlayersWithSessions() const
		{
			std::vector<std::pair<Main::Structures::RoomPlayerInfo, std::shared_ptr<Main::Network::Session>>> allPlayersWithSessions;
			allPlayersWithSessions.reserve(m_players.size() + m_observerPlayers.size());

			for (const auto& [info, weakSession] : m_players)
			{
				if (auto session = weakSession.lock())
					allPlayersWithSessions.emplace_back(info, session);
			}
			for (const auto& [info, weakSession] : m_observerPlayers)
			{
				if (auto session = weakSession.lock())
					allPlayersWithSessions.emplace_back(info, session);
			}

			return allPlayersWithSessions;
		}

		// Refactored
		Main::Structures::SingleRoom Room::getRoomInfo() const
		{
			auto session = m_players[0].second.lock();
			if (!session) return Main::Structures::SingleRoom{};

			return Main::Structures::SingleRoom{m_title.c_str(), static_cast<std::uint16_t>(m_number - 1), static_cast<std::uint16_t>(m_settings.map),
				m_settings.mode,getPlayersPerTeam(), static_cast<std::uint16_t>(m_players.size()), m_hasMatchStarted, !m_password.empty(),
				m_settings.weaponRestriction, m_settings.isObserverModeOn, session->getPlayer().getPing()
			};
		}


		// Refactored
		const Main::Structures::RoomSettings& Room::getRoomSettings() const
		{
			return m_settings;
		}

		// Refactored
		Main::Structures::RoomJoin Room::getRoomJoinInfo() const
		{
			return Main::Structures::RoomJoin{ m_settings.map, m_settings.mode,
				getPlayersPerTeam(), m_hasMatchStarted,!m_password.empty(), m_settings.isOpen, m_settings.weaponRestriction, m_isTeamBalanceOn, 
				m_settings.isObserverModeOn, false, m_password };
		}

		// Refactored
		std::vector<Main::Structures::RoomPlayerItems> Room::getPlayersItems() const
		{
			std::vector<Main::Structures::RoomPlayerItems> ret;
			for (const auto& [roomInfo, weakSession] : ranges::views::concat(m_players, m_observerPlayers))
			{
				auto session = weakSession.lock();
				if (!session) continue;

				Main::Structures::RoomPlayerItems roomPlayerItems;
				const auto separatedItems = session->getPlayer().getEquippedItemsSeparated();
				roomPlayerItems.equippedItems = separatedItems.first;
				roomPlayerItems.equippedWeapons = separatedItems.second;
				roomPlayerItems.uniqueId = roomInfo.uniqueId;
				ret.push_back(roomPlayerItems);
			}
			return ret;
		}

		// Refactored
		std::vector<Main::Structures::PlayerClan> Room::getPlayersClans() const
		{
			return getPlayersClansFrom(m_players);
		}

		std::vector<Main::Structures::PlayerClan> Room::getObserverPlayersClans() const
		{
			return getPlayersClansFrom(m_observerPlayers);
		}

		// Refactored
		void Room::breakroom()
		{
			for (auto& [unused, weakSession] : ranges::views::concat(m_players, m_observerPlayers))
			{
				auto session = weakSession.lock();
				if (!session) continue;
				session->leaveRoom();
			}
			m_players.clear();
			m_observerPlayers.clear();
		}

		// Refactored
		void Room::broadcastToRoom(Common::Network::Packet& packet)
		{
			for (auto& [u, weakSession] : m_players)
			{
				auto session = weakSession.lock();
				if (!session) continue;
				session->asyncWrite(packet);
			}
			for (auto& [u, weakSession] : m_observerPlayers)
			{
				auto session = weakSession.lock();
				if (!session) continue;
				session->asyncWrite(packet);
			}
		}

		void Room::broadcastMessage(const std::string& message)
		{
			for (auto& [info, weakSession] : ranges::views::concat(m_players, m_observerPlayers))
			{
				auto session = weakSession.lock();
				if (!session) continue;
				session->sendMessage(message);
			}
		}

		bool Room::playerExists(std::uint32_t sessionId) const
		{
			return findPlayer(sessionId) != m_players.end() || findObserverPlayer(sessionId) != m_observerPlayers.end();
		}

		// Refactored
		void Room::broadcastToRoomExceptSelf(Common::Network::Packet& packet, const Main::Structures::UniqueId& uniqueId)
		{
			for (auto& [player, weakSession] : m_players)
			{
				if (player.uniqueId.session == uniqueId.session) continue;
				auto session = weakSession.lock();
				if (!session) continue;
				session->asyncWrite(packet);
			}
			for (auto& [player, weakSession] : m_observerPlayers)
			{
				if (player.uniqueId.session == uniqueId.session) continue;
				auto session = weakSession.lock();
				if (!session) continue;
				session->asyncWrite(packet);
			}
		}

		void Room::randomizeTeams()
		{
			using namespace Main::Structures;
			using namespace Main::Network;

			std::vector<std::pair<RoomPlayerInfo*, std::shared_ptr<Session>>> blueTeam;
			std::vector<std::pair<RoomPlayerInfo*, std::shared_ptr<Session>>> redTeam;

			for (auto& [player, weakSession] : m_players)
			{
				auto session = weakSession.lock();
				if (!session) continue;

				if (player.team == Common::Enums::TEAM_BLUE)
					blueTeam.emplace_back(&player, session);
				else if (player.team == Common::Enums::TEAM_RED)
					redTeam.emplace_back(&player, session);
			}

			const std::size_t totalSwaps = std::min(blueTeam.size(), redTeam.size());

			static std::mt19937 rng(std::random_device{}()); 
			std::shuffle(blueTeam.begin(), blueTeam.end(), rng);
			std::shuffle(redTeam.begin(), redTeam.end(), rng);

			Common::Network::Packet response;
			response.setTcpHeader(0, Common::Enums::NO_ENCRYPTION);
			response.setOrder(313);
			response.setExtra(1);

			for (std::size_t i = 0; i < totalSwaps; ++i)
			{
				auto& [bluePlayer, session1] = blueTeam[i];
				auto& [redPlayer, session2] = redTeam[i];

				if (session1 && session2)
				{
					auto uniqueId = session1->getAccountInfo().uniqueId;
					response.setData(reinterpret_cast<std::uint8_t*>(&uniqueId), sizeof(uniqueId));
					broadcastToRoom(response);

					uniqueId = session2->getAccountInfo().uniqueId;
					response.setData(reinterpret_cast<std::uint8_t*>(&uniqueId), sizeof(uniqueId));
					broadcastToRoom(response);

					auto temp = bluePlayer->team;
					bluePlayer->team = redPlayer->team;
					redPlayer->team = temp;
				}
			}
		}

		// Refactored
		void Room::broadcastToTeamExceptSelf(Common::Network::Packet& packet, std::shared_ptr<Main::Network::Session> givenSession)
		{
			if (auto it = findPlayer(givenSession->getAccountInfo().uniqueId.session); it != m_players.end())
			{
				auto selfSession = it->second.lock();
				if (!selfSession) return;

				const auto targetTeam = static_cast<Common::Enums::Team>(it->first.team);
				const bool isInMatch = selfSession->getPlayer().isInMatch();

				for (auto& [roomInfo, weakSession] : m_players)
				{
					auto session = weakSession.lock();
					if (!session) continue;

					if (roomInfo.uniqueId.session != givenSession->getAccountInfo().uniqueId.session &&
						roomInfo.team == targetTeam &&
						session->getPlayer().isInMatch() == isInMatch)
					{
						session->asyncWrite(packet);
					}
				}
			}
		}

		void Room::broadcastToTeam(Common::Network::Packet& packet, Common::Enums::Team team)
		{
			for (auto& [roomInfo, weakSession] : m_players)
			{
				auto session = weakSession.lock();
				if (!session) continue;

				if (roomInfo.team == team)
				{
					session->asyncWrite(packet);
				}
			}
		}

		void Room::broadcastToDeadTeamExceptSelf(Common::Network::Packet& packet, std::shared_ptr<Main::Network::Session> givenSession)
		{
			if (auto it = findPlayer(givenSession->getAccountInfo().uniqueId.session); it != m_players.end())
			{
				auto sessionSelf = it->second.lock();
				if (!sessionSelf) return;

				const auto targetTeam = static_cast<Common::Enums::Team>(it->first.team);
				const bool isInMatch = sessionSelf->getPlayer().isInMatch();

				for (auto& [roomInfo, weakSession] : m_players)
				{
					auto session = weakSession.lock();
					if (!session) continue;

					if (roomInfo.uniqueId.session != givenSession->getAccountInfo().uniqueId.session &&
						roomInfo.team == targetTeam &&
						session->getPlayer().isInMatch() == isInMatch &&
						session->getPlayer().getPlayerState() == Common::Enums::STATE_DYING)
					{
						session->asyncWrite(packet);
					}
				}
			}
		}

		void Room::toMatchExceptSelfHelper(Common::Network::Packet& packet, std::shared_ptr<Main::Network::Session> givenSession)
		{
			const auto uniqueId = givenSession->getAccountInfo().uniqueId;

			bool isSenderInPlayers = std::any_of(m_players.begin(), m_players.end(),
				[&uniqueId](const auto& pair) { return pair.first.uniqueId == uniqueId; });
			bool isSenderInObservers = std::any_of(m_observerPlayers.begin(), m_observerPlayers.end(),
				[&uniqueId](const auto& pair) { return pair.first.uniqueId == uniqueId; });
		

			auto toMatchExceptSelf = [&packet, &uniqueId](auto& playerGroup) {
				for (auto& [roomInfo, weakSession] : playerGroup) {
					auto session = weakSession.lock();
					if (!session) continue;
					if (roomInfo.uniqueId != uniqueId && session->getPlayer().isInMatch())
						session->asyncWrite(packet);
				}
				};

			if (isSenderInPlayers || (isSenderInObservers && givenSession->getAccountInfo().playerGrade >= Common::Enums::GRADE_ES))
			{
				toMatchExceptSelf(m_players);
				toMatchExceptSelf(m_observerPlayers);
			}
			else if (isSenderInObservers)
			{
				toMatchExceptSelf(m_observerPlayers);
			}
		}

		// Refactored
		void Room::broadcastToMatchExceptSelf(Common::Network::Packet& packet, std::shared_ptr<Main::Network::Session> session, std::uint32_t extra)
		{
			if (extra == Main::Enums::ChatExtra::TEAM)
			{
				broadcastToTeamExceptSelf(packet, session);
			}
			else
			{
				toMatchExceptSelfHelper(packet, session);
			}
		}

		void Room::broadcastToDeadExceptSelf(Common::Network::Packet& packet, std::shared_ptr<Main::Network::Session> session, std::uint32_t extra)
		{
			if (extra == Main::Enums::ChatExtra::TEAM)
			{
				if (session->getAccountInfo().playerGrade >= Common::Enums::GRADE_ES)
				{
					broadcastToTeamExceptSelf(packet, session);
				}
				else
				{
					broadcastToDeadTeamExceptSelf(packet, session);
				}
			}
			else
			{
				if (session->getAccountInfo().playerGrade >= Common::Enums::GRADE_ES)
				{
					broadcastToMatchExceptSelf(packet, session, Main::Enums::ChatExtra::NORMAL);
				}
				else
				{
					toMatchDeadExceptSelfHelper(packet, session, m_players);
				}
			}
		}

		// Refactored
		void Room::broadcastOutsideMatchExceptSelf(Common::Network::Packet& packet, std::shared_ptr<Main::Network::Session> givenSession, std::uint32_t extra)
		{
			if (extra == Main::Enums::ChatExtra::TEAM)
			{
				broadcastToTeamExceptSelf(packet, givenSession);
			}
			else
			{
				for (auto& [roomInfo, weakSession] : m_players)
				{
					auto session = weakSession.lock();
					if (roomInfo.uniqueId == givenSession->getAccountInfo().uniqueId || !session || session->getPlayer().isInMatch()) continue;
					session->asyncWrite(packet);
				}
				for (auto& [roomInfo, weakSession] : m_observerPlayers)
				{
					auto session = weakSession.lock();
					if (roomInfo.uniqueId == givenSession->getAccountInfo().uniqueId || !session || session->getPlayer().isInMatch()) continue;
					session->asyncWrite(packet);
				}
			}
		}

		bool Room::hasMatchStarted() const
		{
			return m_hasMatchStarted;
		}

		void Room::endMatch()
		{
			for (auto& pair : ranges::views::concat(m_players, m_observerPlayers))
			{
				auto session = pair.second.lock();
				if (!session) continue;

				auto playerState = session->getPlayer().getPlayerState();

				if (playerState == Common::Enums::STATE_INVENTORY
					|| playerState == Common::Enums::STATE_LOBBY
					|| playerState == Common::Enums::STATE_SHOP
					|| playerState == Common::Enums::STATE_CAPSULE
					|| playerState == Common::Enums::STATE_WAITING)
				{
					continue;
				}

				setStateFor(pair, Common::Enums::PlayerState::STATE_WAITING);
			}

			m_hasMatchStarted = false;
			m_votekickYesses = 0;
			m_votekickTargetUid = Main::Structures::UniqueId{};
			m_votekickReason = 0;
			m_votekickTargetNickname = "";
		}

		std::uint32_t Room::getTotalPlayersInMatch() const
		{
			std::uint32_t ret = 0;
			for (const auto& currentPlayer : m_players)
			{
				if (currentPlayer.first.state == Common::Enums::STATE_NORMAL)
				{
					++ret;
				}
			}
			return ret;
		}


		// this is used by both (!) manual AND automatic host-changes
		bool Room::changeHost(std::size_t newHostIdx)
		{
			if (newHostIdx >= m_players.size() || m_players.empty()) return false;
			std::swap(m_players[0], m_players[newHostIdx]); 
			return true;
		}

		bool Room::changeHostByNickname(const std::string& nickname)
		{
			std::uint32_t playerIndex = 0;
			for (const auto& currentPlayer : m_players)
			{
				auto session = currentPlayer.second.lock();
				if (!session)
				{
					++playerIndex;
					continue;
				}
				if (session->getAccountInfo().nickname == nickname)
				{
					break;
				}
				++playerIndex;
			}
			if (playerIndex != 0 && changeHost(playerIndex))
			{
				sendHostChangePacket(playerIndex, Common::Enums::CHANGE_HOST_SUCCESS);
				return true;
			}
			else
			{
				return false;
			}
		}

		std::shared_ptr<Main::Network::Session> Room::getPlayer(const Main::Structures::UniqueId& uniqueId)
		{
			auto playerIt = findPlayer(uniqueId.session);
			if (playerIt == m_players.end())
				return nullptr;

			return playerIt->second.lock(); 
		}

		bool Room::isHost(const Main::Structures::UniqueId& uniqueId) const
		{
			return m_players.empty() ? false : m_players[0].first.uniqueId.session == uniqueId.session;
		}

		void Room::setSpecificSetting(std::uint8_t setting)
		{
			m_specificSetting = setting;
		}

		void Room::votekickVoteYes()
		{
			++m_votekickYesses;
		}

		std::pair<std::uint32_t, std::string> Room::getVotekickResultDetails()
		{
			const std::pair<std::uint32_t, std::string> ret = { m_votekickYesses, m_votekickTargetNickname };
			m_votekickReason = 0;
			m_votekickTargetUid = Main::Structures::UniqueId{};
			m_votekickYesses = 0;
			m_votekickTargetNickname = "";
			return ret;
		}

		void Room::resetVotekick()
		{
			m_hasMatchStarted = false;
			m_votekickYesses = 0;
			m_votekickTargetUid = Main::Structures::UniqueId{};
			m_votekickReason = 0;
			m_votekickTargetNickname = "";
		}

		bool Room::startVotekick(const Main::Structures::UniqueId& uniqueId, const std::string& targetNickname, std::uint32_t reason, std::uint32_t starterAid)
		{
			if (std::find(m_votekickStarters.begin(), m_votekickStarters.end(), starterAid) != m_votekickStarters.end())
			{
				return false; 
			}
			m_votekickReason = reason;
			m_votekickTargetUid = uniqueId;
			++m_votekickYesses;
			m_votekickTargetNickname = targetNickname;
			
			m_votekickStarters.push_back(starterAid);
			return true;
		}

		void Room::setTime(std::uint16_t time)
		{
			m_settings.time = time;
		}

		// Refactored
		bool Room::changePlayerTeam(const Main::Structures::UniqueId& uniqueId, std::uint32_t newTeam)
		{
			if (m_settings.mode == Common::Enums::ZombieMode && (newTeam == 1 || newTeam == 2))
			{
				if (auto playerIt = findPlayer(uniqueId.session); playerIt != m_players.end())
				{
					if (auto session = playerIt->second.lock())
					{
						playerIt->first.team = newTeam;
						return true;
					}
				}
				return false;
			}
			else if (auto playerIt = findPlayer(uniqueId.session); playerIt != m_players.end())
			{
				auto session = playerIt->second.lock();
				if (!session || session->getPlayer().isInMatch()) return false;

				if (newTeam == Common::Enums::TEAM_OBSERVER)
				{
					if (m_observerPlayers.size() >= Common::Constants::maxObserverPlayers) return false;

					std::swap(m_players.back(), *playerIt);

					if (playerIt = findPlayer(uniqueId.session); playerIt != m_players.end()
						&& movePlayerHelper(m_players, m_observerPlayers, playerIt, newTeam))
					{
						return true;
					}
					return false;
				}
				else
				{
					playerIt->first.team = playerIt->first.team == Common::Enums::TEAM_BLUE ? Common::Enums::TEAM_RED : Common::Enums::TEAM_BLUE;
					return true;
				}
			}
			else if (auto observerIt = findObserverPlayer(uniqueId.session); observerIt != m_observerPlayers.end()
				&& (newTeam == Common::Enums::TEAM_ALL || newTeam == Common::Enums::TEAM_RED || newTeam == Common::Enums::TEAM_BLUE))
			{
				auto session = observerIt->second.lock();
				if (!session || session->getPlayer().isInMatch() || m_players.size() >= getPlayersPerTeam()) return false;

				observerIt->first.team = isModeTeamBased() ? calculateNewPlayerTeam() : Common::Enums::TEAM_ALL;
				return movePlayerHelper(m_observerPlayers, m_players, observerIt, observerIt->first.team);
			}
			return false;
		}
		
		// Refactored
		bool Room::isModeTeamBased() const
		{
			return m_settings.mode == Common::Enums::TeamDeathMatch || m_settings.mode == Common::Enums::Elimination
				|| m_settings.mode == Common::Enums::CaptureTheBattery || m_settings.mode == Common::Enums::ItemMatch
				|| m_settings.mode == Common::Enums::BombBattle || m_settings.mode == Common::Enums::SniperMode
				|| m_settings.mode == Common::Enums::CloseCombat || m_settings.mode == Common::Enums::Scrimmage
				|| m_settings.mode == Common::Enums::SuperItemMatch
				|| m_settings.mode == Common::Enums::Clan_BombBattle || m_settings.mode == Common::Enums::Clan_CaptureTheBattery
				|| m_settings.mode == Common::Enums::Clan_Elimination || m_settings.mode == Common::Enums::Clan_TeamDeathMatch;
		}

		// Refactored
		void Room::updatePlayersTeamToTeamBased()
		{
			for (std::size_t i = 0; auto& [roomPlayerInfo, s] : m_players)
			{
				roomPlayerInfo.team = (i % 2 == 0) ? Common::Enums::TEAM_BLUE : Common::Enums::TEAM_RED;
				++i;
			}
		}

		// Refactored
		void Room::updatePlayersTeamToNonTeamBased()
		{
			std::for_each(m_players.begin(), m_players.end(), [](auto& player) { player.first.team = Common::Enums::TEAM_ALL;});
		}
		
		// Refactored
		Common::Enums::Team Room::calculateNewPlayerTeam() const
		{
			const auto totalBlueTeam = std::ranges::count_if(m_players, [](const auto& player) {
				return player.first.team == Common::Enums::Team::TEAM_BLUE;
				});
			const auto totalRedTeam = std::ranges::count_if(m_players, [](const auto& player) {
				return player.first.team == Common::Enums::Team::TEAM_RED;
				});
			return (totalRedTeam >= totalBlueTeam) ? Common::Enums::TEAM_BLUE : Common::Enums::TEAM_RED;
		}


		void Room::setPlayersPerTeam(std::uint16_t playersPerTeam)
		{
			m_settings.playersPerTeam = playersPerTeam;
		}

		std::uint8_t Room::getSpecificSetting() const
		{
			return m_specificSetting;
		}

		const std::string& Room::getRoomTitle() const
		{
			return m_title;
		}

		bool Room::isRoomFullObserverExcluded() const
		{
			return static_cast<std::uint32_t>(m_players.size()) >= getPlayersPerTeam();
		}

		// Refactored
		void Room::setStateFor(const Main::Structures::UniqueId& uniqueId, const Common::Enums::PlayerState& playerState)
		{
			if (auto playerIt = findPlayer(uniqueId.session); playerIt != m_players.end())
			{
				if (auto session = playerIt->second.lock())
				{
					playerIt->first.state = playerState;
					session->setPlayerState(playerState);
				}
			}
			else if (auto observerIt = findObserverPlayer(uniqueId.session); observerIt != m_observerPlayers.end())
			{
				if (auto session = observerIt->second.lock())
				{
					observerIt->first.state = playerState;
					session->setPlayerState(playerState);
				}
			}
		}

		void Room::setStateFor(std::pair<Main::Structures::RoomPlayerInfo, std::weak_ptr<Main::Network::Session>>& player, Common::Enums::PlayerState state)
		{
			auto session = player.second.lock();
			if (!session)
			{
				return;
			}

			player.first.state = state;
			session->setPlayerState(state);
		}

		bool Room::isEveryoneCsd() const
		{
			return checkWeapons(
				[](const std::shared_ptr<Main::Network::Session>& s) { return s->hasCsdItems(); }, "is not CSD");
		}

		bool Room::isEveryoneBasic() const
		{
			return checkWeapons(
				[](const std::shared_ptr<Main::Network::Session>& s) { return s->hasBasicItems(); }, "doesn't have basic weapons");
		}


		// Refactored
		void Room::startMatch()
		{
			auto& pair = m_players[0];
			auto hostSession = pair.second.lock();
			if (!hostSession) return;

			// Reset votekick stuff
			resetVotekick();
			m_votekickStarters.clear();

			if (m_players.empty()) return;
			// Check that all players are CSD if the mode is enabled.

			m_hasMatchStarted = true;
			m_matchStartTime = Main::Details::getUtcTimeMs();
			setStateFor(pair, Common::Enums::PlayerState::STATE_NORMAL);

			std::vector<Main::ClientData::PlayerTeamInfo> playerTeamBatch;
			for (auto& pair : ranges::views::concat(m_players, m_observerPlayers))
			{
				if (pair.first.state == Common::Enums::STATE_READY || hostSession->getId() == pair.first.uniqueId.session)
				{
					if (auto session = pair.second.lock())
					{
						session->m_totalBossBattleRespawnsLeft = 3;
						setStateFor(pair, Common::Enums::PlayerState::STATE_NORMAL);

						Main::ClientData::PlayerTeamInfo info;
						info.uid = pair.first.uniqueId;
						info.team = pair.first.team;
						std::memcpy(info.nickname, session->getAccountInfo().nickname, 16);
						playerTeamBatch.push_back(info);
					}
				}
			}

			if (!playerTeamBatch.empty())
			{
				if (!Main::Ipc::M2C_sendPlayerTeamInfoBatch(hostSession->getId(), playerTeamBatch))
				{
					Utils::Logger::log("Failed to send player team batch to Cast Server", Utils::LogType::Error);
				}
			}
		}
		
		bool Room::isObserverFull() const
		{
			return m_observerPlayers.size() >= Common::Constants::maxObserverPlayers;
		}

		// Refactored
		bool Room::kickPlayer(const std::string& name)
		{
			for (auto& [roomInfo, weakSession] : ranges::views::concat(m_players, m_observerPlayers))
			{
				auto session = weakSession.lock();
				if (!session) continue;

				if (std::string(session->getAccountInfo().nickname) == name)
				{
					// Order is important here, kick must happen before "removePlayer" call as it swaps sessions if the host is the one being kicked!
					if (session->getAccountInfo().playerGrade >= Common::Enums::PlayerGrade::GRADE_ES) return false;
					m_kickedPlayerAccountIds.emplace_back(session->getAccountInfo().accountID, session->getAccountInfo().nickname);
					kickPlayerImpl(std::move(session), 0x23);
					return true;
				}
			}
			return false;
		}

		bool Room::votekickPlayer(const Main::Structures::UniqueId& uniqueId)
		{
			for (auto& [roomInfo, weakSession] : m_players)
			{
				auto session = weakSession.lock();
				if (!session) continue;

				if (session->getAccountInfo().uniqueId == uniqueId)
				{
					// Order is important here, kick must happen before "removePlayer" call as it swaps sessions if the host is the one being kicked!
					if (session->getAccountInfo().playerGrade >= Common::Enums::PlayerGrade::GRADE_ES) return false;
					m_kickedPlayerAccountIds.emplace_back(session->getAccountInfo().accountID, session->getAccountInfo().nickname);
					kickPlayerImpl(std::move(session), 39);
					return true;
				}
			}
			return false;
		}

		// Refactored
		bool Room::wasPreviouslyKicked(std::uint32_t accountId) const
		{
			return std::ranges::find_if(m_kickedPlayerAccountIds,
				[accountId](const auto& pair) { return pair.first == accountId; })
				!= m_kickedPlayerAccountIds.end();
		}

		bool Room::removeKickedPlayerByNickname(const std::string& nickname)
		{
			auto initialSize = m_kickedPlayerAccountIds.size();
			std::erase_if(m_kickedPlayerAccountIds,
				[&nickname](const auto& pair) { return pair.second == nickname; });

			return m_kickedPlayerAccountIds.size() < initialSize;
		}

		std::vector<std::string> Room::getKickedPlayerNicknames() const
		{
			std::vector<std::string> nicknames;
			nicknames.reserve(m_kickedPlayerAccountIds.size());

			std::ranges::transform(m_kickedPlayerAccountIds, std::back_inserter(nicknames),
				[](const auto& pair) { return pair.second; });

			return nicknames;
		}

		void Room::updateMap(std::uint16_t newMap)
		{
			m_settings.map = newMap;
		}

		void Room::updateRoomSettings(const Main::Structures::RoomSettingsUpdateBase& newRoomSettings, std::uint16_t newMode)
		{
			m_settings.isItemOn = newRoomSettings.isItemOn;
			m_settings.isOpen = newRoomSettings.isOpen;
			m_settings.map = newRoomSettings.map;
			setPlayersPerTeam(newRoomSettings.maxPlayers / 2);
			m_settings.time = newRoomSettings.time;
			m_settings.weaponRestriction = newRoomSettings.weaponRestriction;
			m_specificSetting = newRoomSettings.specificSetting;
			m_isTeamBalanceOn = false; // Team balance currently disabled as it's not implemented & causes team-bugs
			m_settings.mode = newMode;
		}

		// Refactored
		Main::Structures::RoomSettingsUpdateTitlePassword Room::getRoomSettingsUpdate() const
		{
			return Main::Structures::RoomSettingsUpdateTitlePassword{ m_settings, m_password, m_title, m_specificSetting };
		}

		void Room::updateTitle(const std::string& newTitle)
		{
			m_title = newTitle;
		}

		// Refactored
		void Room::updatePassword(const std::string& newPassword)
		{
			m_settings.hasPassword = !newPassword.empty();
			m_password = newPassword;
		}

		void Room::addPoint(std::uint32_t team)
		{
			if (team == Common::Enums::TEAM_RED) ++m_redPoints;
			else ++m_bluePoints;
		}

		// Refactored
		void Room::sendTo(const Main::Structures::UniqueId& uniqueId, const Common::Network::Packet& packet)
		{
			if (auto playerIt = findPlayer(uniqueId.session); playerIt != m_players.end())
			{
				if (auto player = playerIt->second.lock())
				{
					player->asyncWrite(packet);
				}
			}
			else if (auto observerIt = findObserverPlayer(uniqueId.session); observerIt != m_observerPlayers.end())
			{
				if (auto observer = observerIt->second.lock())
				{
					observer->asyncWrite(packet);
				}
			}
		}

		std::optional<std::uint32_t> Room::getTeamForSession(std::uint32_t sessionId) const
		{
			for (const auto& [info, weakSession] : ranges::views::concat(m_players, m_observerPlayers))
			{
				if (info.uniqueId.session == sessionId)
				{
					return info.team;
				}
			}
			return std::nullopt; // Not found
		}


		// Refactored
		void Room::storeEndMatchStatsFor(const Main::Structures::UniqueId& uniqueId, const Main::Structures::ScoreboardResponse& stats,
			std::uint32_t blueScore, std::uint32_t redScore, bool hasLeveledUp, const Main::Structures::EventMissionInfo& eventMissionInfo)
		{
			for (auto& [roomInfo, weakSession] : m_players)
			{
				auto session = weakSession.lock();
				if (!session) continue;
				if (roomInfo.uniqueId == uniqueId)
				{
					Main::Enums::MatchEnd matchEnd = (redScore == blueScore) ? Main::Enums::MATCH_DRAW
						: ((blueScore > redScore && roomInfo.team == Common::Enums::TEAM_BLUE) ||
							(redScore > blueScore && roomInfo.team == Common::Enums::TEAM_RED))
						? Main::Enums::MATCH_WON
						: Main::Enums::MATCH_LOST;

					if (m_settings.mode == Common::Enums::ZombieMode || m_settings.mode == Common::Enums::FreeForAll
						|| m_settings.mode == Common::Enums::BossBattle || m_settings.mode == Common::Enums::ArmsRace
						|| m_settings.mode == Common::Enums::SquareMode || m_settings.mode == Common::Enums::AiBattle)
					{
						matchEnd = Main::Enums::MATCH_DO_NOTHING;
					}

					const auto matchStartTime = session->getMatchStartTime();
					const auto currentTime = Main::Details::getUtcTimeMs();
					std::uint32_t matchDurationSeconds = 0;
					if (matchStartTime != 0) 
					{
						const auto durationMs = currentTime - matchStartTime;
						if (durationMs > 0) 
						{
							matchDurationSeconds = static_cast<uint32_t>(durationMs / 1000);
							if (matchDurationSeconds > 7200) matchDurationSeconds = 0;
						}
					}

					session->storeEndMatchStats(matchDurationSeconds, stats, matchEnd, hasLeveledUp, m_settings.mode == Common::Enums::ZombieMode,
						session->getPlayer().getRoomNumber() >= Common::Constants::clanRoomNumberStart);

					const std::uint32_t now = static_cast<std::uint32_t>(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
					if (now >= eventMissionInfo.startDate && now <= eventMissionInfo.endDate)
					{
						if (m_settings.mode == Common::Enums::ZombieMode && (stats.totalKills / 3) >= 1) // >= 1 zombie kills per match = 1 pt
						{
							session->sendEventMission(ClientData::EventMissionPoint{ 1 });
						}
						if (m_settings.mode == Common::Enums::ZombieMode && stats.meleeKills >= 2) // >= 2 infections per match = 1 pt
						{
							session->sendEventMission(ClientData::EventMissionPoint{ 2 });
						}
						if (stats.headshots >= 2) // >= 2 headshots per match = 1 pt
						{
							session->sendEventMission(ClientData::EventMissionPoint{ 3 });
						}
						if (stats.totalKills >= 15) // >= total kills >= 15 per match = 1 pt
						{
							session->sendEventMission(ClientData::EventMissionPoint{ 4 });
						}
						if (stats.mgKills >= 5) // >= mgKills >= 5 per match = 1 pt
						{ 
							session->sendEventMission(ClientData::EventMissionPoint{ 5 });
						}
					}
					return;
				}
			}
		}

		Main::Network::Session::AccountInfo Room::getAccountInfoFor(const Main::Structures::UniqueId& uniqueId) const
		{
			if (auto playerIt = findPlayer(uniqueId.session); playerIt != m_players.end())
			{
				if (auto player = playerIt->second.lock())
				{
					return player->getAccountInfo();
				}
			}
			else if (auto observerIt = findObserverPlayer(uniqueId.session); observerIt != m_observerPlayers.end())
			{
				if (auto observer = observerIt->second.lock())
				{
					return observer->getAccountInfo();
				}
			}

			return Main::Network::Session::AccountInfo{};
		}

		bool Room::assassinModeEnoughPlayers() const noexcept
		{
			using Common::Enums::Team;
			using Common::Enums::PlayerState;

			std::uint32_t redReady = 0;
			std::uint32_t blueReady = 0;

			for (std::size_t i = 0; const auto & [info, weakSession] : m_players)
			{
				if (info.team != Team::TEAM_RED && info.team != Team::TEAM_BLUE)
					continue;

				if (i == 0) // host
				{
					++i;
					if (info.team == Team::TEAM_RED) ++redReady;
					else if (info.team == Team::TEAM_BLUE) ++blueReady;
					continue;
				}

				if (auto session = weakSession.lock())
				{
					if (session->getPlayer().getPlayerState() == PlayerState::STATE_READY)
					{
						if (info.team == Team::TEAM_RED) ++redReady;
						else if (info.team == Team::TEAM_BLUE) blueReady;
					}
				}
			}

			return redReady >= 4 && blueReady >= 4;
		}

		std::optional<std::pair<Main::Structures::UniqueId, std::string>> Room::getRandomAssassinFrom(Common::Enums::Team team, bool fromReadyPlayers)
		{
			std::vector<std::pair<Main::Structures::UniqueId, std::string>> candidates;

			std::size_t idx = 0;
			for (const auto& [info, weakSession] : m_players)
			{
				if (info.team != team)
				{
					++idx;
					continue;
				}

				if (auto session = weakSession.lock())
				{
					bool isEligible = false;
					if (fromReadyPlayers)
					{
						isEligible = session->getPlayer().getPlayerState() == Common::Enums::STATE_READY || idx == 0;
					}
					else
					{
						isEligible = session->getPlayer().isInMatch();
					}
					if (isEligible)
					{
						candidates.emplace_back(info.uniqueId, session->getPlayer().getPlayerName());
					}
				}

				++idx;
			}

			if (candidates.empty())
				return std::nullopt;

			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);

			return candidates[dist(gen)];
		}
	}
}
