#ifndef TRADE_ADDED_ITEM_H
#define TRADE_ADDED_ITEM_H

#include <cstdint>
#include "../Item/MainItemSerialInfo.h"
#include "Macros.h"

// Whenever one player adds an item to the trade system this structure is used
namespace Main
{
	namespace Structures
	{
PACK_PUSH(1)
		struct TradeAddedItemDetailed
		{
			std::uint32_t unused{};
			std::uint32_t originalItemOwnerAccountId{};
			ItemSerialInfo itemSerialInfo{};
			std::uint32_t itemId{};
			std::uint32_t unused2{};
			std::uint32_t unknown1{};
			std::uint32_t unknown2{};

			explicit TradeAddedItemDetailed(std::uint32_t accountId, const ItemSerialInfo& serialInfo, std::uint32_t itemId)
				: originalItemOwnerAccountId{ accountId }, itemSerialInfo{ serialInfo }, itemId{ itemId }
			{
			}
		};
PACK_POP()

PACK_PUSH(1)
		struct TradeBasicItem
		{
			Main::Structures::ItemId itemId;
			std::uint32_t unused{1};
			ItemSerialInfo itemSerialInfo{};

			TradeBasicItem(std::uint32_t itemId, ItemSerialInfo itemSerialInfo)
				: itemId{ itemId }
			{
				this->itemSerialInfo = itemSerialInfo;
			}
		};
PACK_POP()
	}
}

#endif