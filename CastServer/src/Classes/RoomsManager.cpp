#include "../../include/Classes/Room.h"
#include "../../include/Classes/RoomsManager.h"

#include <Utils/Logger.h>
#include "../../../MainServer/include/Structures/ClientData/Structures.h"

namespace Cast
{
	namespace Classes
	{
		void RoomsManager::addRoom(std::shared_ptr<Room> room, std::uint64_t playerId)
		{
			if (playerId < m_playerSessionIdToRoom.size())
			{
				m_playerSessionIdToRoom[playerId] = room;
				m_rooms.push_back(std::move(room));
			}
		}

		void RoomsManager::switchRoomJoinOrExit(std::shared_ptr<Cast::Network::Session> session, std::uint64_t hostSessionId)
		{
			std::size_t playerId = session->getId();

			if (playerId >= m_playerSessionIdToRoom.size())
				return;

			auto& playerRoom = m_playerSessionIdToRoom[playerId];

			if (playerRoom)
			{
				removePlayerFromRoom(playerId);
				playerRoom.reset();
			}
			else if (hostSessionId < m_playerSessionIdToRoom.size())
			{
				auto& hostRoom = m_playerSessionIdToRoom[hostSessionId];

				if (hostRoom)
				{
					hostRoom->addPlayer(session);
					playerRoom = hostRoom;
				}
			}
		}

		void RoomsManager::broadcastToRoomExceptSelf(std::uint64_t sessionId, Common::Network::UnecryptedPacket& packet)
		{
			if (sessionId >= m_playerSessionIdToRoom.size())
				return;

			auto& playerRoom = m_playerSessionIdToRoom[sessionId];

			if (!playerRoom)
				return;

			playerRoom->broadcastToRoomExceptSelf(sessionId, packet);
		}


		void RoomsManager::broadcastToRoom(std::uint64_t sessionId, Common::Network::UnecryptedPacket& packet)
		{
			if (sessionId >= m_playerSessionIdToRoom.size())
				return;

			auto& playerRoom = m_playerSessionIdToRoom[sessionId];

			if (!playerRoom)
				return;

			playerRoom->broadcastToRoom(packet);
		}

		void RoomsManager::broadcastToMatch(std::uint64_t sessionId, Common::Network::UnecryptedPacket& packet)
		{
			if (sessionId >= m_playerSessionIdToRoom.size())
				return;

			auto& playerRoom = m_playerSessionIdToRoom[sessionId];

			if (!playerRoom)
				return;

			playerRoom->broadcastToMatch(packet);
		}

		void RoomsManager::broadcastToMatchExceptSelf(std::uint64_t sessionId, Common::Network::UnecryptedPacket& packet)
		{
			if (sessionId >= m_playerSessionIdToRoom.size())
				return;

			auto& playerRoom = m_playerSessionIdToRoom[sessionId];

			if (!playerRoom)
				return;

			playerRoom->broadcastToMatchExceptSelf(packet, sessionId);
		}

		void RoomsManager::broadcastToRoomExceptSelfAndHost(std::uint64_t sessionId, std::uint64_t hostSessionId, Common::Network::UnecryptedPacket& packet)
		{
			if (sessionId >= m_playerSessionIdToRoom.size())
				return;

			auto& playerRoom = m_playerSessionIdToRoom[sessionId];

			if (!playerRoom)
				return;

			playerRoom->broadcastToRoomExceptSelfAndHost(sessionId, hostSessionId, packet);
		}

		void RoomsManager::playerForwardToHost(std::uint64_t hostSessionId, std::uint64_t senderSessionId, Common::Network::UnecryptedPacket& packet)
		{
			if (senderSessionId >= m_playerSessionIdToRoom.size())
				return;

			auto& playerRoom = m_playerSessionIdToRoom[senderSessionId];

			if (!playerRoom)
				return;

			playerRoom->playerForwardToHost(hostSessionId, senderSessionId, packet);
		}

		void RoomsManager::hostForwardToPlayer(std::uint64_t hostSessionId, std::uint64_t receiverSessionId, Common::Network::UnecryptedPacket& packet, 
			bool useHostSessionIdInTcpHeader)
		{
			if (hostSessionId >= m_playerSessionIdToRoom.size())
				return;

			auto& playerRoom = m_playerSessionIdToRoom[hostSessionId];

			if (!playerRoom)
				return;

			playerRoom->hostForwardToPlayer(hostSessionId, receiverSessionId, packet, useHostSessionIdInTcpHeader);
		}

		void RoomsManager::setAssassinModeInfoFor(std::uint64_t playerId, const Main::Structures::UniqueId& assassinBlueUid, const std::string& assassinBlueName,
			const Main::Structures::UniqueId& assassinRedUid, const std::string& assassinRedName, bool isAssassinMode)
		{
			if (playerId >= m_playerSessionIdToRoom.size())
				return;

			auto& room = m_playerSessionIdToRoom[playerId];

			if (!room)
				return;

			if (!isAssassinMode)
			{
				room->m_isAssassinMode = false;
				return;
			}
			room->m_assassinBlueName = assassinBlueName;
			room->m_assassinBlueUid = assassinBlueUid;
			room->m_assassinRedName = assassinRedName;
			room->m_assassinRedUid = assassinRedUid;
			room->m_isAssassinMode = true;
		}

		bool RoomsManager::isAssassinMode(std::uint64_t playerId) const
		{
			if (playerId >= m_playerSessionIdToRoom.size())
				return false;

			auto& room = m_playerSessionIdToRoom[playerId];

			if (!room)
				return false;

			return room->m_isAssassinMode;
		}

		void RoomsManager::setMapFor(std::uint64_t playerId, std::uint32_t map)
		{
			if (playerId >= m_playerSessionIdToRoom.size())
				return;

			auto& room = m_playerSessionIdToRoom[playerId];

			if (!room)
				return;

			room->setMap(map);
		}

		void RoomsManager::setPlayerTeamsFor(std::uint64_t playerId, const std::vector<Main::ClientData::PlayerTeamInfo>& players)
		{
			if (playerId >= m_playerSessionIdToRoom.size())
				return;

			auto& room = m_playerSessionIdToRoom[playerId];

			if (!room)
				return;

			room->setPlayerTeams(players);
		}

		bool RoomsManager::setAssassinPosition(std::uint64_t playerId, const Cast::Structures::PositionStruct& position)
		{
			if (playerId >= m_playerSessionIdToRoom.size()) return false;
			auto& room = m_playerSessionIdToRoom[playerId];
			if (!room) return false;
			if (!room->m_isAssassinMode) return false;
			if (room->m_assassinBlueUid.session == playerId)
			{
				room->m_blueAssassinPos = position;
				return true;
			}
			else if (room->m_assassinRedUid.session == playerId)
			{
				room->m_redAssassinPos = position;
				return true;
			}
			return false;
		}

		std::optional<Cast::Structures::PositionStruct> RoomsManager::getPositionFor(std::uint64_t playerId, const Common::Enums::Team team)
		{
			if (playerId >= m_playerSessionIdToRoom.size()) return std::nullopt;
			auto& room = m_playerSessionIdToRoom[playerId];
			if (!room) return std::nullopt;
			if (!room->m_isAssassinMode) return std::nullopt;
			if (team == Common::Enums::TEAM_BLUE)
			{
				return room->m_blueAssassinPos;
			}
			if (team == Common::Enums::TEAM_RED)
			{
				return room->m_redAssassinPos;
			}
			return std::nullopt;
		}

		void RoomsManager::setModeFor(std::uint64_t playerId, std::uint32_t mode)
		{
			if (playerId >= m_playerSessionIdToRoom.size())
				return;

			auto& room = m_playerSessionIdToRoom[playerId];

			if (!room)
				return;

			room->setMode(mode);
		}


		bool RoomsManager::setRoomNumberFor(std::uint64_t playerId, std::uint32_t roomNum)
		{
			if (playerId >= m_playerSessionIdToRoom.size())
				return false;

			auto& room = m_playerSessionIdToRoom[playerId];

			if (!room)
				return false;

			room->setRoomNumber(roomNum);
			return true;
		}

		bool RoomsManager::exists(std::uint64_t playerId)
		{
			return playerId < m_playerSessionIdToRoom.size() && m_playerSessionIdToRoom[playerId] != nullptr;
		}

		std::uint32_t RoomsManager::getMapOf(std::uint64_t playerId)
		{
			if (playerId >= m_playerSessionIdToRoom.size())
				return 0;

			auto& room = m_playerSessionIdToRoom[playerId];

			if (!room)
				return 0;

			return room->getMap();
		}

		std::uint32_t RoomsManager::getModeOf(std::uint64_t playerId)
		{
			if (playerId >= m_playerSessionIdToRoom.size())
				return Common::Enums::MODES_MAX;

			auto& room = m_playerSessionIdToRoom[playerId];

			if (!room)
				return Common::Enums::MODES_MAX;

			return room->getMode();
		}


		void RoomsManager::removePlayerFromRoom(std::uint64_t sessionIdToRemoveFromRoom)
		{
			if (sessionIdToRemoveFromRoom >= m_playerSessionIdToRoom.size())
				return;

			auto roomToRemove = m_playerSessionIdToRoom[sessionIdToRemoveFromRoom];

			if (!roomToRemove)
				return;

			const bool mustRoomBeRemoved = roomToRemove->removePlayer(sessionIdToRemoveFromRoom);
			m_playerSessionIdToRoom[sessionIdToRemoveFromRoom] = nullptr;

			if (mustRoomBeRemoved)
			{
				m_rooms.erase(std::remove(m_rooms.begin(), m_rooms.end(), roomToRemove), m_rooms.end());

				for (auto& roomSlot : m_playerSessionIdToRoom)
				{
					if (roomSlot == roomToRemove)
						roomSlot = nullptr;
				}
			}
		}

		std::optional<std::shared_ptr<Room>> RoomsManager::getRoom(std::uint64_t sessionId)
		{
			if (sessionId >= m_playerSessionIdToRoom.size()) return std::nullopt;
			auto room = m_playerSessionIdToRoom[sessionId];
			return !room ? std::nullopt : std::optional(room);
		}

		void RoomsManager::endMatch(std::uint64_t sessionId)
		{
			if (sessionId >= m_playerSessionIdToRoom.size())
				return;

			auto room = m_playerSessionIdToRoom[sessionId];

			if (!room)
				return;

			room->endMatch();
		}
	};
}
