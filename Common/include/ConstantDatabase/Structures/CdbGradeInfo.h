#include <cstdint>
#ifndef CDB_GRADEINFO_H
#define CDB_GRADEINFO_H

#include "visit_struct/visit_struct.hpp"
#include <array>
#include "Macros.h"

namespace Common
{
	namespace ConstantDatabase
	{
PACK_PUSH(1)
		struct CdbGradeInfo
		{
			std::uint32_t gi_grade = static_cast<std::uint32_t>(-1);
			std::array<char, 16> gi_name;
			std::uint32_t gi_exp;
			std::uint32_t gi_reward_point;
			std::uint32_t gi_reward_item;
			std::uint32_t gi_icon;
			std::uint32_t gi_pve_ticket;

			constexpr std::uint32_t getId() const noexcept { return gi_grade; }
			constexpr bool isValid() const noexcept { return gi_grade != static_cast<std::uint32_t>(-1); }
		};
PACK_POP()
	}
}

VISITABLE_STRUCT(Common::ConstantDatabase::CdbGradeInfo, gi_grade, gi_name, gi_exp, gi_reward_point, gi_reward_item, gi_icon, gi_pve_ticket);

#endif