#ifndef MAIN_EQUIPPED_ITEM_CPP
#define MAIN_EQUIPPED_ITEM_CPP

#include <cstdint>
#include <ctime> 
#include "../../../include/Structures/Item/MainItem.h"
#include "../../../include/Detail/CdbUtils.h"
#include "../../../include/Structures/Item/MainEquippedItem.h"
#include "Enums/GameEnums.h"
#include <ConstantDatabase/Structures/SetItemInfo.h>

namespace Main
{
	namespace Structures
	{
		EquippedItem::EquippedItem(const Main::Structures::Item& item, std::uint64_t aid)
			: id{ item.itemId.itemId }
#ifdef _WIN32
			, expirationDate{ static_cast<__time32_t>(item.expirationDate) }
#else
			, expirationDate{ static_cast<std::int32_t>(item.expirationDate) }
#endif
			, serialInfo{ item.serialInfo }
			, durability{ item.durability }
			, energy{ item.energy }
			, isSealed{ item.isSealed }
			, sealLevel{ item.sealLevel }
			, experienceEnhancement{ item.experienceEnhancement }
			, mpEnhancement{ item.mpEnhancement }
		{
			type = CdbUtils::getItemType(item.itemId.itemId).value_or(static_cast<std::uint32_t>(-1));
			auto& setItemsInstance = Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::SetItemInfo>::getInstance();

			if (type != static_cast<std::uint32_t>(-1) && type >= 17)
			{
				if (!setItemsInstance.getEntry(item.itemId.itemId))
				{
					type = static_cast<std::uint32_t>(-1);
				}
				else
				{
					type = Common::Enums::ItemType::SET;
				}
			}
		}
	}
}

#endif

