#ifndef COMMON_GAME_ENUMS_H
#define COMMON_GAME_ENUMS_H

#include <cstdint>
#include <string>

namespace Common
{
	namespace Enums
	{
		enum Characters
		{
			Naomi = 0,
			Kai = 1,
			Pandora = 2,
			CHIP = 3,
			Knox = 4,
			Simon = 5,
			Amelia = 6,
			Sharkill = 7,
			Sophitia = 8,
			MAX_CHARACTERS = 9
		};

		enum Team
		{
			TEAM_ALL = 0,
			TEAM_RED = 1,
			TEAM_BLUE = 2,
			TEAM_OBSERVER = 4,
			TEAM_ZOMBIE = 5
		};

		enum ItemType
		{
			HAIR = 0,
			FACE = 1,
			DRESS = 2,
			LEGS = 3,
			SKIRT = 4,
			GLOVES = 5,
			BOOTS = 6,
			ACC_UPPER = 7,
			ACC_WAIST = 8,
			ACC_BACK = 9,
			MELEE = 10,
			RIFLE = 11,
			SHOTGUN = 12,
			SNIPER = 13,
			MG = 14,
			BAZOOKA = 15,
			GRENADE = 16,
			SET = 17,
			MAX_ITEMTYPE = 18
		};

		inline bool isWeapon(ItemType itemType)
		{
			return itemType >= ItemType::MELEE && itemType <= ItemType::GRENADE;
		}

		enum PlayerState : std::uint32_t
		{
			STATE_EXIT = 0,
			STATE_SINGLEPLAYER = 3,
			STATE_LOBBY = 4,
			STATE_SHOP = 5,
			STATE_INVENTORY = 6,
			STATE_CAPSULE = 7,
			STATE_WAITING = 8,
			STATE_READY = 9,
			STATE_LOADING = 10,
			STATE_LOADED = 11,
			STATE_NORMAL = 12,
			STATE_DYING = 13, // ???
			STATE_TRADE = 14, // N.b. this state is only for the server, doesn't exist in the client
		};


		inline std::string playerStateToString(std::uint32_t state) 
		{
			switch (state) {
			case 0:  return "Exit";
			case 3:  return "Singleplayer";
			case 4:  return "Lobby";
			case 5:  return "Shop";
			case 6:  return "Inventory";
			case 7:  return "Capsule";
			case 8:  return "Waiting";
			case 9:  return "Ready";
			case 10: return "Loading";
			case 11: return "Loaded";
			case 12: return "Normal";
			case 13: return "Dying";
			case 14: return "Trade (Server Only)";
			default: return "Unknown State";
			}
		}


		enum PlayerType
		{
			HOST,
			NON_HOST,
		};

		enum MatchItemAction
		{
			MATCHITEM_DO_NOTHING,
			MATCHITEM_DELETE,
			MATCHITEM_STOCKS_REDUCED_SUCCESS,
			MATCHITEM_STOCK_ZERO,
		};

		enum CouponItemAction
		{
			COUPON_ITEM_DO_NOTHING,
			COUPON_ITEM_DELETE,
			COUPON_ITEM_STOCKS_REDUCED_SUCCESS,
			COUPON_ITEM_STOCK_IS_ZERO
		};

		enum AddCouponAction
		{
			EXISTING_COUPON_STOCK_UPDATED,
			MUST_CREATE_NEW_COUPON,
			UNKNOWN_COUPON_ADD_ERROR
		};
	}
}

#endif
