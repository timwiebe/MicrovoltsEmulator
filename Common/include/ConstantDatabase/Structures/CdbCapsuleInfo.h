#ifndef CGD_CAPSULE_INFO_H
#define CGD_CAPSULE_INFO_H

#include "visit_struct/visit_struct.hpp"
#include "Macros.h"

namespace Common
{
	namespace ConstantDatabase
	{
PACK_PUSH(1)
		struct CdbCapsuleInfo
		{
			std::uint32_t gi_id = static_cast<std::uint32_t>(-1);
			char gi_name[64]{};
			std::uint32_t gi_type{}; // capsule currency: coin(0), RT(1), MP(2)
			std::uint32_t gi_statetype{};
			std::uint32_t gi_infoid{};
			std::uint32_t gi_limited_grade{};
			std::uint32_t gi_price{};
			std::uint32_t gi_luckypoint{};
			std::uint32_t gi_listicon{};
			std::uint32_t gi_titleicon{};
			char gi_desc[255]{};

			constexpr std::uint32_t getId() const noexcept { return gi_id; }
			constexpr bool isValid() const noexcept { return gi_id != static_cast<std::uint32_t>(-1); }
		};
PACK_POP()
	}
}

VISITABLE_STRUCT(Common::ConstantDatabase::CdbCapsuleInfo, gi_id, gi_name, gi_type, gi_statetype, gi_infoid, gi_limited_grade, gi_price,
	gi_luckypoint, gi_listicon, gi_titleicon, gi_desc);

#endif