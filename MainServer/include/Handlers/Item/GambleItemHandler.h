#ifndef GAMBLE_ITEM_HANDLER_H
#define GAMBLE_ITEM_HANDLER_H

#include "Network/Packet.h"
#include "../../../include/Network/MainSession.h"
#include "DeleteItemHandler.h"
#include <random>
#include "../../Detail/Utilities.h"

namespace Main
{
	namespace Handlers
	{
		inline void handleGambleItem(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session)
		{
			Common::Network::Packet response;
			response.setTcpHeader(request.getSession(), Common::Enums::USER_LARGE_ENCRYPTION);
			const auto& accountInfo = session->getAccountInfo();

			constexpr const std::uint32_t gambleCost = 50'000;
			if (accountInfo.microPoints < gambleCost)
			{   // send "not enough MP" message
				response.setOrder(193);
				response.setExtra(44);
				session->asyncWrite(response);
				return;
			}

			if (session->getPlayer().getPlayerState() == Common::Enums::STATE_INVENTORY)
			{
				Main::Structures::ItemSerialInfo itemSerialInfo = Main::Details::parseData<Main::Structures::ItemSerialInfo>(request);

				using cdbItems = Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbItemInfo>;
				using cdbWeapons = Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbWeaponInfo>;

				const auto& itemGambleItems = cdbItems::getGambleItems();
				const auto& weaponGambleItems = cdbWeapons::getGambleItems();

				const auto foundId = session->getPlayer().findItemIdBySerialInfo(itemSerialInfo);
				if (foundId == std::nullopt)
				{
					session->sendMessage("[Handlers::handleGambleItem] error: foundId == nullopt, please report this issue");
					return;
				}
				auto itemType = Main::CdbUtils::getItemType(foundId.value());
				if (itemType == std::nullopt)
				{
					session->sendMessage("[Handlers::handleGambleItem] error: itemType == nullopt, please report this issue");
					return;
				}
				if (Main::CdbUtils::getItemDuration(foundId.value()) != 0)
				{
					session->sendMessage("The gamble system can only be used with unlimited items");
					return;
				}

				std::uint32_t newRandomId{};
				std::mt19937 gen(std::random_device{}());

				if (itemGambleItems.contains(itemType.value()) && !itemGambleItems.at(itemType.value()).empty())
				{
					const auto& items = itemGambleItems.at(itemType.value());
					std::uniform_int_distribution<std::size_t> dist(0, items.size() - 1);
					newRandomId = items[dist(gen)];
				}
				else if (weaponGambleItems.contains(itemType.value()) && !weaponGambleItems.at(itemType.value()).empty())
				{
					const auto& items = weaponGambleItems.at(itemType.value());
					std::uniform_int_distribution<std::size_t> dist(0, items.size() - 1);
					newRandomId = items[dist(gen)];
				}
				else
				{
					session->sendMessage("[Handlers::handleGambleItem] Error: this item cannot be gambled!");
					return;
				}

				if (session->replaceItem(itemSerialInfo, newRandomId, "Item replaced after using the gamble system (" +
					std::to_string(foundId.value()) + " => " + std::to_string(newRandomId) + ")"))
				{
					session->setAccountMicroPoints(accountInfo.microPoints - gambleCost);
					session->sendCurrency();
				}
				else
				{
					session->sendMessage("[Handlers::handleGambleItem] error while replacing the item with a new one - report this issue");
				}
			}
		}
	}
}

#endif