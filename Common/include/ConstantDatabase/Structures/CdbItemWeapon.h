#ifndef CGD_ITEM_WEAPON_INFO_H
#define CGD_ITEM_WEAPON_INFO_H

#include "visit_struct/visit_struct.hpp"
#include "CdbWeaponsInfo.h"
#include "CdbItemInfo.h"
#include "Macros.h"

namespace Common
{
	namespace ConstantDatabase
	{
		// Common stuff between CdbItem and CdbWeapons to reduce map lookups in Cdb.h and CdbUtils.h
PACK_PUSH(1)
		struct CdbItemWeapon
		{
			std::uint32_t ii_id = static_cast<std::uint32_t>(-1);
			int ii_buy_coupon{};
			int ii_buy_cash{};
			int ii_buy_point{};
			int ii_durable_value{};
			bool ii_immediately_set{};
			int ii_limited_time{};
			int ii_type{};
			int ii_stocks{};
			int ii_effect_1{};
			int ii_effect_2{};
			int ii_effect_3{};
			int ii_tradeable{};
			int ii_type_inven{};
			std::array<char, 50> ii_name{};
			std::array<char, 100> ii_option{};

			constexpr CdbItemWeapon() = default;

			constexpr CdbItemWeapon(const CdbWeaponInfo& cdbWeapon)
				: ii_id{ cdbWeapon.ii_id }, ii_buy_coupon{ cdbWeapon.ii_buy_coupon }, ii_buy_cash{ cdbWeapon.ii_buy_cash }
				, ii_buy_point{ cdbWeapon.ii_buy_point }, ii_durable_value{ cdbWeapon.ii_durable_value }
				, ii_immediately_set{ cdbWeapon.ii_immediately_set }, ii_limited_time{ cdbWeapon.ii_limited_time }, ii_type{ cdbWeapon.ii_type }
				, ii_name{cdbWeapon.ii_name}, ii_stocks{cdbWeapon.ii_stocks}, ii_effect_1{cdbWeapon.ef_effect_1}, ii_effect_2{cdbWeapon.ef_effect_2}
				, ii_effect_3{cdbWeapon.ef_effect_3}, ii_tradeable{cdbWeapon.ii_is_trade}, ii_type_inven{cdbWeapon.ii_type_inven}
			{
				std::copy(cdbWeapon.ii_name_option.begin(),cdbWeapon.ii_name_option.end(), ii_option.begin());
			}

			constexpr CdbItemWeapon(const CdbItemInfo& cdbItem)
				: ii_id{ cdbItem.ii_id }, ii_buy_coupon{ cdbItem.ii_buy_coupon }, ii_buy_cash{ cdbItem.ii_buy_cash }
				, ii_buy_point{ cdbItem.ii_buy_point }, ii_durable_value{ cdbItem.ii_durable_value }
				, ii_immediately_set{ cdbItem.ii_immediately_set }, ii_limited_time{ cdbItem.ii_limited_time }, ii_type{ cdbItem.ii_type }
				, ii_name{ cdbItem.ii_name }, ii_stocks{cdbItem.ii_stocks}, ii_effect_1{ cdbItem.ef_effect_1 }, ii_effect_2{ cdbItem.ef_effect_2 }
				, ii_effect_3{ cdbItem.ef_effect_3 }, ii_tradeable{cdbItem.ii_is_trade}, ii_type_inven{cdbItem.ii_type_inven}
				, ii_option{cdbItem.ii_name_option}
			{
			}

			constexpr std::uint32_t getId() const noexcept { return ii_id; }
		};
PACK_POP()
	}
}


#endif