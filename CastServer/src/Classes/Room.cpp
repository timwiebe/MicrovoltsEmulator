
#include <cstdint>
#include <unordered_map>
#include <utility>
#include "../../include/Network/CastSession.h"
#include "../../include/Classes/Room.h"
#include <Utils/Logger.h>
#include <Enums/RoomEnums.h>
#include <Utils/Utils.h>
#include <Utils/SetupParser.h>
#include <Structures/PlayerPositionFromServer.h>
#include <Utils/Utilities.h>

namespace Cast
{
	namespace Classes
	{
		Room::Room(std::uint64_t hostSessionId, std::shared_ptr<Cast::Network::Session> hostSession)
		{
			m_serverId = Common::Utils::SetupParser::getInstance().getSelfCastServerInfo().serverNumber;
			m_playersVec.push_back(hostSession);
		}

		void Room::setRoomNumber(std::uint32_t roomNumber)
		{
			for (auto& weakSession : m_playersVec)
			{
				if (auto session = weakSession.lock()) 
				{
					session->setRoomNumber(roomNumber);  
				}
			}
			m_roomNumber = roomNumber;
		}

		std::uint32_t Room::getRoomNumber() const
		{
			return m_roomNumber;
		}

		void Room::endMatch()
		{
			m_hasMatchStarted = false;

			for (auto& weakSession : m_playersVec)
			{
				if (auto session = weakSession.lock()) 
				{
					session->setIsInMatch(false);  
				}
			}
		}

		std::size_t Room::getTotalPlayers() const
		{
			return m_playersVec.size();
		}

		void Room::removeAllPlayers()
		{
			for (auto& weakSession : m_playersVec)
			{
				if (auto session = weakSession.lock())  
				{
					session->setIsInMatch(false);
					session->setRoomNumber(-1);
				}
			}
			m_playersVec.clear();
		}

		// Returns true if the room must be closed because there are no players left, false otherwise
		bool Room::removePlayer(std::uint64_t playerSessionId)
		{
			auto it = std::find_if(m_playersVec.begin(), m_playersVec.end(),
				[=](const auto& weakSession) {
					if (auto session = weakSession.lock())  
					{
						return session->getId() == playerSessionId;
					}
					return false;
				});

			if (it == m_playersVec.end())
			{
				return m_playersVec.empty(); 
			}

			if (auto session = it->lock()) 
			{
				session->setIsInMatch(false);
				session->setRoomNumber(-1);
			}

			m_playersVec.erase(it);
			return m_playersVec.empty(); 
		}

		void Room::addPlayer(std::shared_ptr<Cast::Network::Session> playerSession)
		{
			if (!playerSession) return;

			auto it = std::remove_if(m_playersVec.begin(), m_playersVec.end(),
				[&](const std::weak_ptr<Cast::Network::Session>& weakSession) {
					if (auto session = weakSession.lock()) 
					{
						return session->getId() == playerSession->getId();
					}
					return false; 
				});

			if (it != m_playersVec.end())
			{
				::Utils::Logger::log("Duplicate sessionID found and replaced in room join", ::Utils::LogType::Warning, "Room::addPlayer");
				m_playersVec.erase(it, m_playersVec.end());
			}

			m_playersVec.push_back(playerSession); 
			playerSession->setRoomNumber(m_roomNumber); 
		}

		void Room::broadcastToRoomExceptSelf(std::uint64_t selfSessionId, Common::Network::UnecryptedPacket& packet)
		{
			for (auto& currentPlayer : m_playersVec)
			{
				if (auto player = currentPlayer.lock())
				{
					if (player->getId() == selfSessionId) continue;
					packet.setTcpHeader(selfSessionId);
					player->asyncWrite(packet);
				}
			}
		}

		void Room::broadcastToRoomExceptSelfAndHost(std::uint64_t selfSessionId, std::uint64_t hostSessionId, Common::Network::UnecryptedPacket& packet)
		{
			for (auto& currentPlayer : m_playersVec)
			{
				if (auto player = currentPlayer.lock())
				{
					auto currentId = player->getId();
					if (currentId == selfSessionId || currentId == hostSessionId) continue;
					packet.setTcpHeader(selfSessionId);
					player->asyncWrite(packet);
				}
			}
		}

		void Room::broadcastToRoom(Common::Network::UnecryptedPacket& packet)
		{
			for (auto& currentPlayer : m_playersVec)
			{
				if (auto player = currentPlayer.lock())
				{
					packet.setTcpHeader(player->getId());
					player->asyncWrite(packet);
				}
			}
		}

		void Room::broadcastToMatch(Common::Network::UnecryptedPacket& packet)
		{
			for (auto& currentPlayer : m_playersVec)
			{
				if (auto player = currentPlayer.lock())
				{
					if (!player->m_isInMatch) continue;
					packet.setTcpHeader(player->getId());
					player->asyncWrite(packet);
				}
			}
		}

		void Room::broadcastToMatchExceptSelf(Common::Network::UnecryptedPacket& packet, std::uint32_t selfId)
		{
			for (auto& currentPlayer : m_playersVec)
			{
				if (auto player = currentPlayer.lock())
				{
					if (!player->m_isInMatch) continue;
					if (player->getId() == selfId) continue;

					packet.setTcpHeader(player->getId());
					player->asyncWrite(packet);
				}
			}
		}


		bool Room::isInMatch(std::uint64_t playerSessionId) const
		{
			auto it = std::find_if(m_playersVec.begin(), m_playersVec.end(),
				[playerSessionId](const auto& player) {
					auto lockedPlayer = player.lock();  
					return lockedPlayer && lockedPlayer->getId() == playerSessionId;
				});

			if (it != m_playersVec.end())
			{
				auto lockedPlayer = it->lock();
				return lockedPlayer && lockedPlayer->isInMatch();
			}

			return false;
		}

		void Room::playerForwardToHost(std::uint64_t hostSessionId, std::uint64_t senderSessionId, Common::Network::UnecryptedPacket& packet)
		{
			auto it = std::find_if(m_playersVec.begin(), m_playersVec.end(),
				[hostSessionId](const auto& player) {
					auto lockedPlayer = player.lock(); 
					return lockedPlayer && lockedPlayer->getId() == hostSessionId;
				});

			if (it != m_playersVec.end())
			{
				if (auto lockedHost = it->lock())  
				{
					packet.setTcpHeader(senderSessionId);
					lockedHost->asyncWrite(packet);
				}
			}
		}

		void Room::hostForwardToPlayer(std::uint64_t hostSessionId, std::uint64_t playerId, Common::Network::UnecryptedPacket& packet, bool useHostIdInTcpHeader)
		{
			auto it = std::find_if(m_playersVec.begin(), m_playersVec.end(),
				[playerId](const auto& player) {
					auto lockedPlayer = player.lock();  
					return lockedPlayer && lockedPlayer->getId() == playerId;
				});

			if (it != m_playersVec.end())
			{
				if (auto lockedPlayer = it->lock())
				{
					packet.setTcpHeader(useHostIdInTcpHeader ? hostSessionId : playerId);
					lockedPlayer->asyncWrite(packet);
				}
			}
		}

		void Room::setMap(std::uint32_t mapId)
		{
			m_map = mapId;
		}

		void Room::setMode(std::uint32_t modeId)
		{
			m_mode = modeId;
		}

		std::uint32_t Room::getMode() const
		{
			return m_mode;
		}

		std::uint32_t Room::getMap() const
		{
			return m_map;
		}

		void Room::setPlayerTeams(const std::vector<Main::ClientData::PlayerTeamInfo>& players)
		{
			for (auto& weakSession : m_playersVec)
			{
				if (auto session = weakSession.lock())
				{
					auto it = std::find_if(players.begin(), players.end(),
						[&](const Main::ClientData::PlayerTeamInfo& info) {
							return info.uid.session == session->getId();
						});

					if (it != players.end())
					{
						session->m_team = static_cast<Common::Enums::Team>(it->team);
						session->m_nickname = it->nickname;
					}
				}
			}
		}

		void Room::broadcastMessage(const std::string& message)
		{
			for (const auto& currentPlayer : m_playersVec)
			{
				if (auto player = currentPlayer.lock())
				{
					player->sendMessage(message);
				}
			}
		}

		void Room::killTeam(Common::Enums::Team team)
		{
			Common::Network::UnecryptedPacket packet;
			packet.setOrder(264);
			Cast::Structures::SuicideStructure suicideStruct;
			suicideStruct.posX = suicideStruct.posY = suicideStruct.posZ = 0;

			for (auto& playerWeak : m_playersVec)
			{
				if (auto player = playerWeak.lock(); player && player->m_team == team)
				{
					suicideStruct.uniqueId = Main::Structures::UniqueId{ static_cast<std::uint32_t>(player->getId()), m_serverId, 0 };
					packet.setData(reinterpret_cast<std::uint8_t*>(&suicideStruct), sizeof(suicideStruct));
					broadcastToRoom(packet);
				}
			}
		}

		void Room::tryFindNewAssassin(std::uint32_t leavingPlayerSid)
		{
			if (!m_isAssassinMode) return;

			using Team = Common::Enums::Team;

			auto assignNewAssassin = [&](Team team, Main::Structures::UniqueId& assassinUid, std::string& assassinName) {
				for (auto& playerWeak : m_playersVec)
				{
					if (auto player = playerWeak.lock();
						player && player->m_team == team && player->getId() != leavingPlayerSid)
					{
						assassinUid.session = player->getId();
						assassinName = player->m_nickname;
						broadcastMessage("New " + (team == Team::TEAM_BLUE ? std::string("BLUE") : std::string("RED")) + " assassin: " + player->m_nickname);
						break;
					}
				}
				};

			if (m_assassinBlueUid.session == leavingPlayerSid)
				assignNewAssassin(Team::TEAM_BLUE, m_assassinBlueUid, m_assassinBlueName);
			else if (m_assassinRedUid.session == leavingPlayerSid)
				assignNewAssassin(Team::TEAM_RED, m_assassinRedUid, m_assassinRedName);
		}


		std::uint32_t Room::getTotalAlivePlayers() const
		{
			std::uint32_t total = 0;
			for (auto& playerWeak : m_playersVec)
			{
				if (auto player = playerWeak.lock(); player && player->m_isInMatch && player->m_team == Common::Enums::TEAM_ALL && !player->isDead)
				{
					++total;
				}
			}

			return total;
		}

		void Room::shuffleCoordinates()
		{
			static std::random_device rd;
			static std::mt19937 gen(rd());

			std::ranges::shuffle(respawnPoints, gen);
		}

		void Room::respawnEveryoneArena()
		{
			m_arenaRoundFinished = false;

			Common::Network::UnecryptedPacket response;
			response.setOrder(276);
			Cast::Structures::PlayerRespawnPosition position;

			static std::size_t coordIndex = 0;
			const std::size_t totalCoords = respawnPoints.size();

			for (auto& playerWeak : m_playersVec)
			{
				if (auto player = playerWeak.lock(); player && player->m_isInMatch && player->m_team == Common::Enums::TEAM_ALL && player->isDead)
				{
					const auto& coord = respawnPoints[coordIndex];

					position.targetUniqueId.session = player->getId();
					position.targetUniqueId.server = m_serverId;
					position.x = coord.x;
					position.y = coord.y;
					position.z = coord.z;
					coordIndex = (coordIndex + 1) % totalCoords;
					player->isDead = false;

					response.setData(reinterpret_cast<std::uint8_t*>(&position), sizeof(position));
					broadcastToMatch(response);
				}
			}
		}

		void Room::enqueuePosition(Common::Network::UnecryptedPacket&& pkt) 
		{
			m_pendingPositions.push_back(std::move(pkt));
		}

		void Room::flushPendingPositions()
		{
			if (m_pendingPositions.empty()) return;

			static std::array<std::uint8_t, 2048> batchBuffer;
			std::size_t totalSize = 0;
			std::size_t count = 0;

			for (auto& pkt : m_pendingPositions)
			{
				const auto size = pkt.getDataSize();
				if (totalSize + size > 2036) break;  // 8 bytes header + 4 bytes roomtick
				std::memcpy(batchBuffer.data() + totalSize, pkt.getData(), size);
				totalSize += size;
				++count;
			}

			//const std::uint32_t serverTick = m_roomTick - (((Common::Utils::getCurrentTimestampMs() - timeSinceLastRestart) / 10) - m_roomTick);
			std::memmove(batchBuffer.data() + sizeof(m_roomTick), batchBuffer.data(), totalSize);
			std::memcpy(batchBuffer.data(), &m_roomTick, sizeof(m_roomTick));
			totalSize += sizeof(m_roomTick);
			static Common::Network::UnecryptedPacket batch(2048, 322, static_cast<uint32_t>(count));
			batch.setOption(static_cast<uint32_t>(count));
			batch.setData(batchBuffer.data(), static_cast<uint16_t>(totalSize));

			broadcastToRoom(batch);
			m_pendingPositions.clear();
		}

	};
}
