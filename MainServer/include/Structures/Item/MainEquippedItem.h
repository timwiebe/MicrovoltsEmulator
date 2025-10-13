#ifndef MAIN_EQUIPPED_ITEM_INFO_H
#define MAIN_EQUIPPED_ITEM_INFO_H

#include <cstdint>
#include <array>
#include "MainItemSerialInfo.h"
#include "Macros.h"

namespace Main
{
	namespace Structures
	{
		class Item;

#ifdef _WIN32
		using ExpirationTimeType = __time32_t;
#else
		using ExpirationTimeType = std::int32_t;
#endif

PACK_PUSH(1)
		struct EquippedItem
		{
			std::uint32_t type : 9 = 0; 
			std::uint32_t id : 23 = 0;    
			ExpirationTimeType expirationDate{}; 
			ItemSerialInfo serialInfo{};
			std::uint16_t durability{}; 
			std::uint16_t energy{};
			std::uint32_t isSealed{};
			std::uint32_t sealLevel{};
			std::uint32_t experienceEnhancement{};
			std::uint32_t mpEnhancement{};

			EquippedItem() = default;
			/* explicit removed on purpose */ EquippedItem(const Item& item, std::uint64_t aid = 0);
			//EquippedItem& operator=(const Item& other);
			//EquippedItem& operator=(const EquippedItem& other);
		};
PACK_POP()

PACK_PUSH(1)
		struct DetailedEquippedItem : EquippedItem
		{
			std::uint16_t characterId{};

			DetailedEquippedItem() = default;

			explicit DetailedEquippedItem(const Item& item, std::uint16_t charId)
				: EquippedItem{ item }, characterId{ charId }
			{
			}

			explicit DetailedEquippedItem(const EquippedItem& equippedItem, std::uint16_t charId)
				: EquippedItem{ equippedItem }, characterId{ charId }
			{
			}
		};
PACK_POP()

PACK_PUSH(1)
		struct BasicEquippedItem
		{
		private:
			struct Data
			{
				std::uint32_t type : 9 = 254; // on purpose, otherwise "0" overrides the type HAIR
				std::uint32_t equippedItemId : 23 = 0;
				std::uint32_t unknown = 0;

				Data(const Main::Structures::EquippedItem& equippedItem)
					: equippedItemId{ equippedItem.id }, type{ equippedItem.type }
				{
				}

				Data() = default;
			};

		public:
			std::array<Data, 17> items{};
		};
PACK_POP()

PACK_PUSH(1)
		struct BasicEquippedItemLobby
		{
		private:
			struct Data
			{
				std::uint32_t type : 9 = 254; // on purpose, otherwise "0" overrides the type HAIR
				std::uint32_t equippedItemId : 23 = 0;

				Data(const Main::Structures::EquippedItem& equippedItem)
					: equippedItemId{ equippedItem.id }, type{ equippedItem.type }
				{
				}

				Data() = default;
			};

		public:
			std::array<Data, 17> items{};
		};
PACK_POP()
	}
}

#endif

