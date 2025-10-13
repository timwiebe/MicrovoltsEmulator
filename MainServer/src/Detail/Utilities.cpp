
#include "../../include/Detail/Utilities.h"
#include "../../include/Classes/Room.h"
#include "../../include/Classes/RoomsManager.h"


namespace Main
{
	namespace Details
	{
		void broadcastPlayerItems(Main::Classes::RoomsManager& roomsManager, std::shared_ptr<Main::Network::Session> session, const Common::Network::Packet& request)
		{
			const auto& player = session->getPlayer();
			if (Main::Classes::Room* room = roomsManager.getRoomByNumber(player.getRoomNumber()))
			{
				auto& setItemsInstance = Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::SetItemInfo>::getInstance();
				room->updatePlayerInfo(session);
				std::pair<Main::Structures::UniqueId, Main::Structures::BasicEquippedItem> dataToSend{};
				const auto& accountInfo = player.getAccountInfo();
				dataToSend.first = accountInfo.uniqueId;

				// reset state -- needed for when the target user unequips an item!
				for (std::uint32_t i = 0; i < Common::Constants::maxEquippedItems; ++i)
				{
					dataToSend.second.items[i].equippedItemId = 0;
					dataToSend.second.items[i].type = i;
				}

				const std::size_t offset = accountInfo.latestSelectedCharacter * Common::Enums::MAX_ITEMTYPE;
				const auto& targetEquippedItems = player.getEquippedItems();
				for (std::size_t i = 0; i < Common::Enums::MAX_ITEMTYPE; ++i)
				{
					if (offset + i >= targetEquippedItems.size()) return;
					auto& itemByCharacter = targetEquippedItems[offset + i];
					if (!itemByCharacter.serialInfo.itemNumber) continue;

					if (itemByCharacter.type >= Common::Enums::SET)
					{
						if (auto entry = setItemsInstance.getEntry(itemByCharacter.id); entry)
						{
							for (auto currentTypeNotNull : Common::Utils::getPartTypesWhereSetItemInfoTypeNotNull(*entry, accountInfo.latestSelectedCharacter))
							{
								if (currentTypeNotNull >= dataToSend.second.items.size()) continue;
								dataToSend.second.items[currentTypeNotNull].equippedItemId = itemByCharacter.id;
								dataToSend.second.items[currentTypeNotNull].type = currentTypeNotNull;
							}
						}
					}
					else if (itemByCharacter.type >= Common::Enums::ItemType::HAIR && itemByCharacter.type <= Common::Enums::ItemType::GRENADE)
					{
						dataToSend.second.items[itemByCharacter.type] = itemByCharacter;
					}
				}

				Common::Network::Packet response;
				response.setTcpHeader(request.getSession(), Common::Enums::NO_ENCRYPTION);
				response.setCommand(Details::Orders::PLAYER_ITEMS_BROADCAST, 0, accountInfo.latestSelectedCharacter, Common::Constants::maxEquippedItems);
				response.setData(reinterpret_cast<std::uint8_t*>(&dataToSend), sizeof(dataToSend));
				room->broadcastToRoomExceptSelf(response, accountInfo.uniqueId);
			}
		}
	}
}
