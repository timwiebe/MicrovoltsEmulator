#ifndef CDB_REWARD_INFO_H
#define CDB_REWARD_INFO_H

#include <cstdint>
#include "visit_struct/visit_struct.hpp"
#include "Macros.h"

namespace Common
{
	namespace ConstantDatabase
	{
PACK_PUSH(1)
		struct CdbRewardInfo
		{
			std::uint32_t ri_mod = static_cast<std::uint32_t>(-1);
			std::uint32_t ri_exp_base;
			std::uint32_t ri_exp_max;
			std::uint32_t ri_exp_kill;
			std::uint32_t ri_exp_mod_kill;
			std::uint32_t ri_exp_death; 
			std::uint32_t ri_exp_assist;
			std::uint32_t ri_exp_mission;
			std::uint32_t ri_exp_mission_win;
			std::uint32_t ri_exp_pcbang;
			std::uint32_t ri_exp_silver;
			std::uint32_t ri_exp_gold;
			std::uint32_t ri_exp_vip;
			std::uint32_t ri_poi_base;
			std::uint32_t ri_poi_max;
			std::uint32_t ri_poi_kill;
			std::uint32_t ri_poi_mod_kill;
			std::uint32_t ri_poi_death;
			std::uint32_t ri_poi_assist;
			std::uint32_t ri_poi_mission;
			std::uint32_t ri_poi_mission_win;
			std::uint32_t ri_poi_pcbang;
			std::uint32_t ri_poi_silver;
			std::uint32_t ri_poi_gold;
			std::uint32_t ri_poi_vip;
			std::uint32_t ri_mod_limited_time;
			std::uint32_t ri_player_limited_time;
			std::uint32_t ri_penalty_point;
			std::uint32_t ri_event_exp;
			std::uint32_t ri_event_point;
			std::uint32_t ri_clan_base_exp;
			std::uint32_t ri_clan_bonus_exp;

			constexpr std::uint32_t getId() const noexcept { return ri_mod; }
			constexpr bool isValid() const noexcept { return ri_mod != static_cast<std::uint32_t>(-1); }
		};
PACK_POP()
	}
}

VISITABLE_STRUCT(Common::ConstantDatabase::CdbRewardInfo,
	ri_mod,
	ri_exp_base,
	ri_exp_max,
	ri_exp_kill,
	ri_exp_mod_kill,
	ri_exp_death,
	ri_exp_assist,
	ri_exp_mission,
	ri_exp_mission_win,
	ri_exp_pcbang,
	ri_exp_silver,
	ri_exp_gold,
	ri_exp_vip,
	ri_poi_base,
	ri_poi_max,
	ri_poi_kill,
	ri_poi_mod_kill,
	ri_poi_death,
	ri_poi_assist,
	ri_poi_mission,
	ri_poi_mission_win,
	ri_poi_pcbang,
	ri_poi_silver,
	ri_poi_gold,
	ri_poi_vip,
	ri_mod_limited_time,
	ri_player_limited_time,
	ri_penalty_point,
	ri_event_exp,
	ri_event_point,
	ri_clan_base_exp,
	ri_clan_bonus_exp
);

#endif