#ifndef CGD_SET_ITEM_INFO_H
#define CGD_SET_ITEM_INFO_H

#include "visit_struct/visit_struct.hpp"
#include "Macros.h"
#include <cstdint>

namespace Common
{
	namespace ConstantDatabase
	{
PACK_PUSH(1)
		struct SetItemInfo
		{
			std::uint32_t si_id = static_cast<std::uint32_t>(-1);
			std::uint32_t si_hair;
			std::uint32_t si_face;
			std::uint32_t si_top;
			std::uint32_t si_under;
			std::uint32_t si_pants;
			std::uint32_t si_arms;
			std::uint32_t si_boots;
			std::uint32_t si_acce_A;
			std::uint32_t si_acce_B;
			std::uint32_t si_acce_C;

			constexpr std::uint32_t getId() const noexcept { return si_id; }
			constexpr bool isValid() const noexcept { return si_id != static_cast<std::uint32_t>(-1); }
		};
PACK_POP()
	}
}

VISITABLE_STRUCT(Common::ConstantDatabase::SetItemInfo, si_id, si_hair, si_face, si_top, si_under, si_pants, si_arms, si_boots, si_acce_A, si_acce_B, si_acce_C);


#endif
