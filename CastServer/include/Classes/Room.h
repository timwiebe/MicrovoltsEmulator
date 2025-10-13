#ifndef CAST_ROOM_H
#define CAST_ROOM_H

#include <cstdint>
#include <unordered_map>
#include <utility>
#include "../Network/CastSession.h"
#include <mutex>
#include "../../../MainServer/include/Structures/ClientData/Structures.h"
#include "../Structures/SuicideStruct.h"
#include "Enums/GameEnums.h"
#include "Enums/RoomEnums.h"
#include "../Structures/Rest.h"
#include <random>

namespace Cast
{
	namespace Classes
	{
		class Room
		{
		private:
			std::vector<std::weak_ptr<Cast::Network::Session>> m_playersVec{};
			std::uint32_t m_map{};
			std::uint32_t m_mode{};
			std::uint32_t m_tick{};
			std::uint32_t m_roomNumber = -1;
			std::uint32_t m_serverId{};

			std::vector<Common::Network::UnecryptedPacket> m_pendingPositions;

		public:
			bool m_hasMatchStarted{};
			bool m_isInvisible{};
			std::uint32_t m_roomTick{};


			// Arena Mode
			bool m_isArenaMode{};
			bool m_arenaRoundFinished{};
			std::vector<Cast::Structures::RespawnCoord> respawnPoints =
			{
				{24583, 60683, 19115},
				{58722, 60694, 19115},
				{25497, 60099, 19415},
				{25475, 58678, 19415},
				{25521, 23589, 19415},
				{25922, 26812, 19415},
				{55990, 26504, 56296},
				{59297, 26439, 56296},
				{60178, 26367, 56296},
				{60225, 23721, 56296},
				{60510, 58834, 56296},
				{59849, 59616, 56296},
				{58965, 59961, 56296},
				{58252, 59675, 56296},
				{57185, 58780, 56296},
				{21758, 25560, 56296}
			};

			// Assassin Mode
			std::uint32_t m_isAssassinMode{};
			Main::Structures::UniqueId m_assassinBlueUid{};
			Main::Structures::UniqueId m_assassinRedUid{};
			Cast::Structures::PositionStruct m_blueAssassinPos{};

			std::string m_assassinBlueName{};
			std::string m_assassinRedName{};
			Cast::Structures::PositionStruct m_redAssassinPos{};

			Room(std::uint64_t hostSessionId, std::shared_ptr<Cast::Network::Session> hostSession);

			void setRoomNumber(std::uint32_t roomNumber);

			std::uint32_t getRoomNumber() const;

			void endMatch();

			std::size_t getTotalPlayersInMatch() const;

			bool isInMatch(std::uint64_t playerSessionId) const;

			void removeAllPlayers();

			bool removePlayer(std::uint64_t playerSessionId);

			std::size_t getTotalPlayers() const;

			void addPlayer(std::shared_ptr<Cast::Network::Session> playerSession);

			void broadcastToRoomExceptSelf(std::uint64_t selfSessionId, Common::Network::UnecryptedPacket& packet);

			void broadcastToRoomExceptSelfAndHost(std::uint64_t selfSessionId, std::uint64_t hostSessionId, Common::Network::UnecryptedPacket& packet);

			void broadcastToRoom(Common::Network::UnecryptedPacket& packet);

			void broadcastToMatch(Common::Network::UnecryptedPacket& packet);

			void broadcastToMatchExceptSelf(Common::Network::UnecryptedPacket& packet, std::uint32_t selfId);

			void playerForwardToHost(std::uint64_t hostSessionId, std::uint64_t senderSessionId, Common::Network::UnecryptedPacket& packet);

			void hostForwardToPlayer(std::uint64_t, std::uint64_t playerId, Common::Network::UnecryptedPacket& packet, bool useHostIdInTcpHeader = true);
			
			void setMap(std::uint32_t mapId);

			std::uint32_t getMap() const;

			void setMode(std::uint32_t modeId);

			std::uint32_t getMode() const;

			void setPlayerTeams(const std::vector<Main::ClientData::PlayerTeamInfo>& players);

			void broadcastMessage(const std::string& message);

			void killTeam(Common::Enums::Team team);

			void tryFindNewAssassin(std::uint32_t leavingPlayerSid);

			bool isArenaMode() const 
			{
				 return m_map == Common::Enums::AcademyTrainingGround && m_mode == Common::Enums::FreeForAll;
			}

			std::uint32_t getTotalAlivePlayers() const;

			void shuffleCoordinates();

			void respawnEveryoneArena();

			void enqueuePosition(Common::Network::UnecryptedPacket&& pkt);

			void flushPendingPositions();
		};
	}
}

#endif
