#ifndef BOUGHT_ITEM_H
#define BOUGHT_ITEM_H

#include <cstdint>
#include "../AccountInfo/MainAccountUniqueId.h"
#include "MainItemSerialInfo.h"
#include "ConstantDatabase/Cdb.h"
#include "../../MainEnums.h"
#include "ConstantDatabase/CdbSingleton.h"
#include "ConstantDatabase/Structures/CdbItemInfo.h"
#include "ConstantDatabase/Structures/CdbWeaponsInfo.h"
#include "ItemId.h"
#include "Macros.h"


namespace Main
{
	namespace Structures
	{
PACK_PUSH(1)
		struct BoughtItem
		{
			Main::Structures::ItemId itemId;
			std::uint32_t unknown{};  // 2==bomb, 3==expired, 0==unlimited, etc.
			ItemSerialInfo serialInfo{};

		public:
			explicit BoughtItem(std::uint32_t id, bool isCouponItem = false)
				: itemId{ id }
			{
				if (!isCouponItem)
				{
					itemId.itemId -= 8388608; 
				}
				const std::uint32_t duration = Main::CdbUtils::getItemDuration(itemId.itemId);
				unknown = duration <= 3 ? duration : serialInfo.itemCreationDate + duration;
				serialInfo.itemOrigin = Main::Enums::ItemFrom::SHOP;
			}


			BoughtItem() = default;
		};
PACK_POP()

PACK_PUSH(1)
		struct BoughtItemToProlong
		{
			ItemSerialInfo serialInfo{};
			std::uint32_t unknown = 1;

		public:
			explicit BoughtItemToProlong(const ItemSerialInfo& itemSerialInfo)
				: serialInfo{ itemSerialInfo }
			{
			}
		};
PACK_POP()
	}
}

#endif