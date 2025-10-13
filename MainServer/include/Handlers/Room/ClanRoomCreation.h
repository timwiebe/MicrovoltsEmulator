#ifndef ROOM_CREATION_HANDLER_CLAN_H
#define ROOM_CREATION_HANDLER_CLAN_H

#include "../../Network/MainSession.h"
#include "Network/Packet.h"
#include "../../MainEnums.h"
#include "../../Structures/Room/ClientRoomCreationInfo.h"
#include "../../Classes/RoomsManager.h"
#include "../../Classes/Room.h"
#include "../../Detail/IpcUtils.h"
#include <cstring> 

namespace Main
{
	namespace Handlers
	{
		inline bool handleRoomCreationClan(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
			Main::Classes::RoomsManager& roomsManager)
		{
			const auto& selfAccountInfo = session->getAccountInfo();
			Main::Structures::RoomPlayerInfo roomCreator{};
			roomCreator.character = selfAccountInfo.latestSelectedCharacter;
			roomCreator.level = selfAccountInfo.playerLevel;
			std::memcpy(roomCreator.playerName, selfAccountInfo.nickname, 16);
			roomCreator.state = Common::Enums::STATE_WAITING;
			roomCreator.uniqueId = selfAccountInfo.uniqueId;
			roomCreator.ping = session->getPlayer().getPing();
			roomCreator.team = Common::Enums::TEAM_BLUE;

			const Main::Structures::CompleteRoomInfo roomInfo = Main::Details::parseData<Main::Structures::CompleteRoomInfo>(request);

			// request.getOption() ==> server/channel ID
			Main::Classes::Room room{ roomInfo.title, roomInfo.roomSettings, roomCreator, session, true };
			room.updatePlayersTeamToTeamBased();
			if (request.getDataSize() >= sizeof(Main::Structures::CompleteRoomInfo))
			{
				room.setPassword(roomInfo.password);
			}
			// request.getExtra() == specific setting. E.g eli => num of rounds, TDM => num of total kills / 10
			room.setSpecificSetting(request.getExtra());

			// Send room number to cast server
			Main::Ipc::M2C_sendRoomNumber(session->getId(), room.getRoomNumber());
			Common::Network::Packet response;
			response.setTcpHeader(request.getSession(), Common::Enums::NO_ENCRYPTION);
			response.setOrder(request.getOrder());
			room.setStateFor(session->getAccountInfo().uniqueId, Common::Enums::STATE_WAITING);
			session->setRoomNumber(room.getRoomNumber());
			const std::pair<std::uint16_t, std::uint16_t> roomNum{ room.getRoomNumber() - 1, 1 }; // {roomNum, unk}
			response.setExtra(1);
			response.setData(reinterpret_cast<const std::uint8_t*>(&roomNum), sizeof(roomNum));
			session->asyncWrite(response);

			roomsManager.addRoom(std::move(room));
			return true;
		}
	}
}

#endif
