#ifndef CGD_COLLECTION_INFO_H
#define CGD_COLLECTION_INFO_H

#include "visit_struct/visit_struct.hpp"
#include "Macros.h"

namespace Common
{
	namespace ConstantDatabase
	{
PACK_PUSH(1)
		struct CdbCollectionInfo
		{
			std::uint32_t ci_id = static_cast<std::uint32_t>(-1);
			std::uint32_t ci_set_index;
			char ci_name[50];
			std::uint32_t ci_reward_point;
			std::uint32_t ci_reward_exp;
			std::uint32_t ci_reward_coin;
			std::uint32_t ci_reward_item;
			std::uint32_t ci_goal_max;
			std::uint32_t ci_notice;
			std::uint32_t ci_icon_uncompleted;
			std::uint32_t ci_icon_complete;
			std::uint32_t ci_icon_size;
			std::uint32_t ci_icon_x;
			std::uint32_t ci_icon_y;
			char ci_desc_skill[255];
			char ci_desc_cond[255];
			char ci_desc_reward[255];
			bool ci_title_check;
			char ci_title_desc[255];
			std::uint32_t ci_mission_type;
			bool ci_hidden_check;
			std::uint32_t ci_hidden_valueA;
			std::uint32_t ci_hidden_valueB;
			std::uint32_t ci_hidden_valueC;

			constexpr std::uint32_t getId() const noexcept { return ci_id; }
			constexpr bool isValid() const noexcept { return ci_id != static_cast<std::uint32_t>(-1); }
		};
PACK_POP()
	}
}

VISITABLE_STRUCT(Common::ConstantDatabase::CdbCollectionInfo, ci_id, ci_set_index, ci_name, ci_reward_point, ci_reward_exp, ci_reward_coin, ci_reward_item, ci_goal_max, ci_notice,
	ci_icon_uncompleted, ci_icon_complete, ci_icon_size, ci_icon_x, ci_icon_y, ci_desc_skill, ci_desc_cond, ci_desc_reward, ci_title_check, ci_title_desc, ci_mission_type, ci_hidden_check,
	ci_hidden_valueA, ci_hidden_valueB, ci_hidden_valueC);

#endif