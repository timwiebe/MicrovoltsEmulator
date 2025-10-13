#ifndef ROOM_CREATION_HANDLER_H
#define ROOM_CREATION_HANDLER_H

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
		enum RoomCreationExtra
		{
			CREATION_FAIL = 0,
			CREATION_SUCCESS = 1,
			CREATION_EMPTY = 6,
			CREATION_BOTBATTLE = 61,
		};

		inline void handleRoomCreation(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session, Main::Classes::RoomsManager& roomsManager,
			bool isRoomCreationEnabled)
		{
			if (session->getPlayer().getRoomNumber()) // cannot create a room while already in one
			{
				session->sendMessage("You are already in a room - If this is an error, report it");
				return;
			}
			else if (!isRoomCreationEnabled && session->getAccountInfo().playerGrade < Common::Enums::GRADE_ES)
			{
				session->sendMessage("Public room creation is currently disabled by the team.");
				return;
			}
			else if (!session->getPlayer().isRoomCreationEnabled())
			{
				session->sendMessage("You currently cannot create a new room.");
				return;
			}

			START_BENCHMARK

			if (request.getFullSize() == Common::Constants::headerSize) return;

			const auto& selfAccountInfo = session->getAccountInfo();
			Main::Structures::RoomPlayerInfo roomCreator{};
			roomCreator.character = selfAccountInfo.latestSelectedCharacter;
			roomCreator.level = selfAccountInfo.playerLevel;
			std::memcpy(roomCreator.playerName, selfAccountInfo.nickname, 16);
			roomCreator.state = Common::Enums::STATE_WAITING;
			roomCreator.uniqueId = selfAccountInfo.uniqueId;
			roomCreator.ping = session->getPlayer().getPing();
			roomCreator.team = Common::Enums::TEAM_ALL;

			const Main::Structures::CompleteRoomInfo roomInfo = Main::Details::parseData<Main::Structures::CompleteRoomInfo>(request);

			// request.getOption() ==> server/channel ID
			Main::Classes::Room room{ roomInfo.title, roomInfo.roomSettings, roomCreator, session };
			if (room.isModeTeamBased())
			{
				room.updatePlayersTeamToTeamBased();
			}
			if (request.getDataSize() >= sizeof(Main::Structures::CompleteRoomInfo))
			{
				room.setPassword(roomInfo.password);
			}
			// request.getExtra() == specific setting. E.g eli => num of rounds, TDM => num of total kills / 10
			room.setSpecificSetting(request.getExtra()); 

			Common::Network::Packet response;
			response.setTcpHeader(request.getSession(), Common::Enums::NO_ENCRYPTION);
			response.setOrder(request.getOrder());

			if (roomsManager.getTotalRooms() >= Common::Constants::maxRooms)
			{
				response.setExtra(RoomCreationExtra::CREATION_FAIL);
				session->asyncWrite(response);
			}
			else
			{
				Main::Ipc::M2C_sendRoomNumber(session->getId(), room.getRoomNumber());
				room.setStateFor(session->getAccountInfo().uniqueId, Common::Enums::STATE_WAITING);
				session->setRoomNumber(room.getRoomNumber());
				const std::pair<std::uint16_t, std::uint16_t> roomInfo{ room.getRoomNumber() - 1, 2 }; // {roomNum, unk}
				response.setExtra(RoomCreationExtra::CREATION_SUCCESS);
				response.setData(reinterpret_cast<const std::uint8_t*>(&roomInfo), sizeof(roomInfo));
				session->asyncWrite(response);

				// Disable team balance for now, since it causes issues such as team bugs
				if (room.isModeTeamBased() && room.getRoomSettings().mode != Common::Enums::AiBattle)
				{
					response.setCommand(125, 0, 0, room.getRoomSettings().mode);
					auto settings = room.getRoomSettingsUpdate();
					response.setData(reinterpret_cast<std::uint8_t*>(&settings), sizeof(settings));
					session->asyncWrite(response);
				}
				roomsManager.addRoom(std::move(room));
			}

			END_BENCHMARK(handleRoomCreation, session)
		}
	}
}

#endif
