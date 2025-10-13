#ifndef DELETE_ITEM_HANDLER_H
#define DELETE_ITEM_HANDLER_H

#include "../../Network/MainSession.h"
#include "../../../include/Structures/AccountInfo/MainAccountInfo.h"
#include "Network/Packet.h"
#include <cstring> 

namespace Main
{
	namespace Handlers
	{
		inline void handleItemDelete(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session)
		{
			START_BENCHMARK

			Main::Structures::ItemSerialInfo itemSerialInfoToDelete{};
			const std::uint32_t totalItemsToDelete = Main::Details::parseData<std::uint32_t>(request); 

            for (std::uint32_t i = 0; i < totalItemsToDelete; ++i)
            {
                std::uint32_t currentOffset = sizeof(std::uint32_t) + sizeof(itemSerialInfoToDelete) * i;

                const std::uint32_t dataSize = request.getDataSize();
                if (currentOffset + sizeof(itemSerialInfoToDelete) > dataSize)
                {
                    break;
                }

                std::memcpy(&itemSerialInfoToDelete, request.getData() + currentOffset, sizeof(itemSerialInfoToDelete));
                session->deleteItem(itemSerialInfoToDelete, "The item was deleted (either manually or it was expired)");
            }


			END_BENCHMARK(handleItemToDelete, session)
		}
	}
}

#endif