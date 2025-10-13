#ifndef SPAWNED_ITEM_H
#define SPAWNED_ITEM_H

#include <chrono>
#include <cstdint>
#include "MainItemSerialInfo.h"
#include "ConstantDatabase/Cdb.h"
#include "../../MainEnums.h"
#include "ConstantDatabase/CdbSingleton.h"
#include "ConstantDatabase/Structures/CdbItemInfo.h"
#include "ConstantDatabase/Structures/CdbWeaponsInfo.h"
#include "../../Detail/CdbUtils.h"
#include "ItemId.h"
#include "Macros.h"

// Note: Creation date cannot be 0 (otherwise the client doesn't know how to handle equipping/unequipping) !!!
namespace Main
{
	namespace Structures
	{
PACK_PUSH(1)
		struct SpawnedItem
		{
			Main::Structures::ItemId itemId;
			__time32_t expirationDate{};
			Main::Structures::ItemSerialInfo serialInfo{};
			Main::Enums::ItemFrom itemFrom = Main::Enums::ItemFrom::SHOP;

			explicit SpawnedItem(std::uint32_t id)
				: itemId{id}
			{
				const std::uint32_t duration = Main::CdbUtils::getItemDuration(id);
				expirationDate = duration <= 3 ? duration : serialInfo.itemCreationDate + duration;
				serialInfo.itemOrigin = Main::Enums::ItemFrom::SHOP;
			}

			explicit SpawnedItem(std::uint32_t id, std::uint32_t stock)
				: itemId{ id }
			{
				itemId.stock = stock;
				const std::uint32_t duration = Main::CdbUtils::getItemDuration(id);
				expirationDate = duration <= 3 ? duration : serialInfo.itemCreationDate + duration;
				serialInfo.itemOrigin = Main::Enums::ItemFrom::SHOP;
			}
		};
PACK_POP()

PACK_PUSH(1)
		struct BoxItem
		{
			Main::Structures::ItemId itemId;
			__time32_t expirationDate{};
			Main::Structures::ItemSerialInfo serialInfo{};

			explicit BoxItem(std::uint32_t id)
				: itemId{ id }
			{
				const std::uint32_t duration = Main::CdbUtils::getItemDuration(id);
				expirationDate = duration <= 3 ? duration : serialInfo.itemCreationDate + duration;
				serialInfo.itemOrigin = Main::Enums::ItemFrom::SHOP;
			}

			explicit BoxItem(std::uint32_t id, __time32_t exp, const Main::Structures::ItemSerialInfo& serial)
				: itemId{ id }, expirationDate{ exp }, serialInfo{ serial }
			{
			}
		};
PACK_POP()

PACK_PUSH(1)
		struct GiftItem
		{
			std::uint64_t clientData{}; // includes uint32_t unknown, uint32_t timestamp => client uses this to delete the gift from the list of gifts once opened
			Main::Structures::ItemId itemId;
			Main::Structures::ItemSerialInfo serialInfo{};
			std::uint32_t unknown{};

			explicit GiftItem(std::uint32_t itemId)
				: itemId{ itemId }
			{
				const std::uint32_t duration = Main::CdbUtils::getItemDuration(itemId);
				unknown = duration <= 3 ? duration : serialInfo.itemCreationDate + duration;
				serialInfo.itemOrigin = Main::Enums::ItemFrom::SHOP;
			}
		};
PACK_POP()

PACK_PUSH(1)
		struct WeeklyReward
		{
			std::uint64_t unknown : 16 = 0;
			std::uint64_t day : 8 = 0;
			std::uint64_t unknown1 : 40 = 0;
			std::array<std::uint32_t, 7> items{};

			explicit WeeklyReward(const std::array<std::uint32_t, 7>& items)
				: items{ items }
			{
				day = std::chrono::weekday{ std::chrono::floor<std::chrono::days>(std::chrono::current_zone()->to_local(std::chrono::system_clock::now())) }.iso_encoding();
			}
		};
PACK_POP()

PACK_PUSH(1)
		struct MonthlyReward
		{
			std::uint64_t month : 16 = 0;
			std::uint64_t day : 8 = 0;
			std::uint64_t unknown : 40 = 0;
			std::array<std::uint32_t, 32> items{};

			explicit MonthlyReward(const std::array<std::uint32_t, 32>& items)
				: items{ items }
			{
				auto now = std::chrono::floor<std::chrono::days>(std::chrono::current_zone()->to_local(std::chrono::system_clock::now()));
				std::chrono::year_month_day ymd{ now };
				month = static_cast<std::uint64_t>(unsigned{ ymd.month() }); 
				day = static_cast<std::uint64_t>(unsigned{ ymd.day() });   
			}
		};
PACK_POP()
	}
}

#endif

