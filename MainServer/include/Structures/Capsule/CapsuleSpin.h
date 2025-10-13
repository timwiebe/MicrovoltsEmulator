#ifndef CAPSULE_SPIN_STRUCT_H
#define CAPSULE_SPIN_STRUCT_H

#include <cstdint>
#include "../Item/MainItemSerialInfo.h"
#include "../../MainEnums.h"
#include "../../Detail/CdbUtils.h"
#include "Macros.h"


namespace Main
{
	namespace Structures
	{
PACK_PUSH(1)
		struct CapsuleSpin
		{
			Main::Structures::ItemId itemId;
			std::uint32_t expirationDate{};
			Main::Structures::ItemSerialInfo itemSerialInfo{};

			CapsuleSpin(std::uint32_t id, const Main::Structures::ItemSerialInfo& serialInfo)
				: itemId{ id }, itemSerialInfo { serialInfo}
			{
				const std::uint32_t duration = Main::CdbUtils::getItemDuration(id);
				expirationDate = duration <= 3 ? duration : serialInfo.itemCreationDate + duration;
				itemSerialInfo.itemOrigin = Main::Enums::ItemFrom::SHOP;
			}
		};
PACK_POP()
	}
}

#endif