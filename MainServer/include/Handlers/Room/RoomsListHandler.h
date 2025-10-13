#ifndef ROOMS_LIST_HANDLER_H
#define ROOMS_LIST_HANDLER_H

#include "../../Network/MainSession.h"
#include "Network/Packet.h"
#include "../../MainEnums.h"
#include "../../Classes/RoomsManager.h"
#include <memory>
#include <cstring> 

namespace Main
{
	namespace Handlers
	{
		inline void handleRoomsList(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session, Main::Classes::RoomsManager& roomsManager)
		{
			const auto roomsList = roomsManager.getRoomsList();
			auto size = roomsList.size();
			std::vector<std::uint8_t> message(roomsList.size() * sizeof(Main::Structures::SingleRoom) + sizeof(std::uint32_t));
			std::memcpy(message.data(), &size, sizeof(std::uint16_t));
			std::memcpy(message.data() + sizeof(std::uint16_t), &size, sizeof(std::uint16_t));
			std::memcpy(message.data() + sizeof(std::uint32_t), roomsList.data(), roomsList.size() * sizeof(Main::Structures::SingleRoom));

			Common::Network::Packet response;
			response.setTcpHeader(request.getSession(), Common::Enums::NO_ENCRYPTION);
			response.setCommand(request.getOrder(), 0, 37, 0);
			response.setData(message.data(), message.size());
			session->asyncWrite(response);
		}
	}	
}

#endif