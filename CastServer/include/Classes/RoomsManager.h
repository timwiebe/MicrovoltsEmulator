#ifndef ROOMS_MANAGER_H
#define ROOMS_MANAGER_H

#include <array>
#include "Room.h"
#include "../include/Enums/RoomEnums.h"
#include <Utils/Constants.h>

namespace Cast
{
	namespace Classes
	{
		class RoomsManager
		{
		private:
			std::array<std::shared_ptr<Cast::Classes::Room>, Common::Constants::maxSessionsPerServer + 1> m_playerSessionIdToRoom{};

		public:
			void addRoom(std::shared_ptr<Room> room, std::uint64_t playerId);

			void switchRoomJoinOrExit(std::shared_ptr<Cast::Network::Session> session, std::uint64_t hostSessionId = -1);

			void broadcastToRoomExceptSelf(std::uint64_t selfSessionId, Common::Network::UnecryptedPacket& packet);

			void broadcastToRoomExceptSelfAndHost(std::uint64_t sessionId, std::uint64_t hostSessionId, Common::Network::UnecryptedPacket& packet);

			void broadcastToRoom(std::uint64_t hostSessionId, Common::Network::UnecryptedPacket& packet);

			void broadcastToMatch(std::uint64_t sessionId, Common::Network::UnecryptedPacket& packet);

			void broadcastToMatchExceptSelf(std::uint64_t sessionId, Common::Network::UnecryptedPacket& packet);

			void playerForwardToHost(std::uint64_t hostSessionId, std::uint64_t senderSessionId, Common::Network::UnecryptedPacket& packet);

			void hostForwardToPlayer(std::uint64_t hostSessionId, std::uint64_t receiverSessionId, Common::Network::UnecryptedPacket& packet, bool useHostSessionIdInTcpHeader = true);

			void setMapFor(std::uint64_t playerId, std::uint32_t map);

			void setModeFor(std::uint64_t playerId, std::uint32_t mode);

			void setRoomNumberFor(std::uint64_t playerId, std::uint32_t roomNum);

			void endMatch(std::uint64_t hostId);

			std::uint32_t getMapOf(std::uint64_t hostId);

			std::uint32_t getModeOf(std::uint64_t playerId);

			std::optional<std::shared_ptr<Room>> getRoom(std::uint64_t sessionId);

			bool exists(std::uint64_t playerId);

			void removePlayerFromRoom(std::uint64_t sessionIdToRemoveFromRoom);

			void setAssassinModeInfoFor(std::uint64_t playerId, const Main::Structures::UniqueId& assassinBlueUid, const std::string& assassinBlueName,
				const Main::Structures::UniqueId& assassinRedUid, const std::string& assassinRedName, bool isAssassinMode);

			bool setAssassinPosition(std::uint64_t playerId, const Cast::Structures::PositionStruct& position);

			std::optional<Cast::Structures::PositionStruct> getPositionFor(std::uint64_t playerId, const Common::Enums::Team team);

			bool isAssassinMode(std::uint64_t playerId) const;

			void setPlayerTeamsFor(std::uint64_t playerId, const std::vector<Main::ClientData::PlayerTeamInfo>& players);
		};
	}
}
#endif
