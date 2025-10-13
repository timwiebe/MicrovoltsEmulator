#ifndef MAIN_ITEM_SERIALINFO_H
#define MAIN_ITEM_SERIALINFO_H

#include <cstdint>
#include <ctime>
#include <compare>
#include "Macros.h"

#ifdef WIN32
#include <corecrt.h>
#else
#define __time32_t uint32_t
#endif

namespace Main
{
	namespace Structures
	{
PACK_PUSH(1)
		struct ItemSerialInfo
		{
			std::uint64_t itemNumber : 20 = 0; // Note: "0" is treated as a sentinel value for EquippedItem (no item exists with itemNumber = 0 in the database!)
			std::uint64_t m_serverId : 4 = 0;
			std::uint64_t unknown : 4 = 0;
			std::uint64_t itemOrigin : 4 = 0; 
			std::uint64_t itemCreationDate : 32 = 0;
			
			auto operator<=>(const ItemSerialInfo&) const = default;

			ItemSerialInfo()
			{
				// this is necessary, itemCreationDate can't be 0 otherwise the client doesn't know how to handle equipping/unequipping of items
				itemCreationDate = static_cast<__time32_t>(std::time(0)); 
			}
		};
PACK_POP()
	}
}

#endif