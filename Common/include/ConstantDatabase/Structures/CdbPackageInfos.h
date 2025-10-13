#ifndef CDB_PACKAGE_INFOS_H
#define CDB_PACKAGE_INFOS_H

#include "visit_struct/visit_struct.hpp"
#include "Macros.h"

namespace Common
{
	namespace ConstantDatabase
	{
PACK_PUSH(1)
		struct CdbItemPackageInfo
		{
			std::uint32_t ip_id;
			std::uint32_t ip_infoid = static_cast<std::uint32_t>(-1);
			std::uint32_t ip_type{};
			std::uint32_t ip_group{};
			std::uint32_t ip_prob{};
			std::uint32_t ip_itemid{};
			std::uint32_t ip_noticeid{};

			constexpr std::uint32_t getId() const noexcept { return ip_infoid; } // this is what the client sends when buying a weapon/item package
			constexpr bool isValid() const noexcept { return ip_infoid != static_cast<std::uint32_t>(-1); }
		};
PACK_POP()

PACK_PUSH(1)
		struct CdbWeaponPackageInfo
		{
			std::uint32_t pi_id;
			std::uint32_t pi_type;
			std::uint32_t pi_itemid = static_cast<std::uint32_t>(-1);
			std::uint32_t pi_group{};
			std::uint32_t pi_prob{};
			std::uint32_t pi_valueA{};
			std::uint32_t pi_valueB{};
			std::uint32_t pi_valueC{};

			constexpr std::uint32_t getId() const noexcept { return pi_itemid; } // this is what the client sends when buying a weapon/item package
			constexpr bool isValid() const noexcept { return pi_itemid != static_cast<std::uint32_t>(-1); }
		};
PACK_POP()
	}
}

VISITABLE_STRUCT(Common::ConstantDatabase::CdbItemPackageInfo, ip_id, ip_infoid, ip_type, ip_group, ip_prob, ip_itemid, ip_noticeid);
VISITABLE_STRUCT(Common::ConstantDatabase::CdbWeaponPackageInfo, pi_id, pi_type, pi_itemid, pi_group, pi_prob, pi_valueA, pi_valueB, pi_valueC);

#endif