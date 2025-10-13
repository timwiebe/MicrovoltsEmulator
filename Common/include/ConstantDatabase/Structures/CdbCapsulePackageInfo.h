#ifndef CGD_CAPSULE_PACKAGE_INFO_H
#define CGD_CAPSULE_PACKAGE_INFO_H

#include "visit_struct/visit_struct.hpp"
#include "Macros.h"

namespace Common
{
	namespace ConstantDatabase
	{
PACK_PUSH(1)
		struct CdbCapsulePackageInfo
		{
			int gi_id{};
			std::uint32_t gi_infoid = static_cast<std::uint32_t>(-1); // common with CdbCapsuleInfo.gi_infoid
			int gi_type{};   // type 1 => rare unlimited, type 0 => not unlimited
			int gi_luckytype{};
			int gi_group{};
			int gi_prob{};   // probability to win item
			int gi_itemid{};
			int gi_noticeid{};

			constexpr std::uint32_t getId() const noexcept { return gi_infoid; }
			constexpr bool isValid() const noexcept { return gi_infoid != static_cast<std::uint32_t>(-1); }
		};
PACK_POP()
	}
}

VISITABLE_STRUCT(Common::ConstantDatabase::CdbCapsulePackageInfo, gi_id, gi_infoid, gi_type, gi_luckytype, gi_group, gi_prob, gi_itemid, gi_noticeid);

#endif