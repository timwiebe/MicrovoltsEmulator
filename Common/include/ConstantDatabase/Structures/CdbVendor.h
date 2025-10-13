#ifndef CGD_VENDORINFO_H
#define CGD_VENDORINFO_H

#include "visit_struct/visit_struct.hpp"
#include <array>
#include "Macros.h"

namespace Common
{
	namespace ConstantDatabase
	{
PACK_PUSH(1)
        struct CdbVendorInfo
        {
            std::uint32_t vi_id = static_cast<std::uint32_t>(-1);
            std::uint32_t vi_category;
            std::uint32_t vi_type;
            std::uint32_t vi_array_none;
            std::uint32_t vi_array_new;
            std::uint32_t vi_array_hit;
            std::uint32_t vi_list_type;
            std::uint32_t vi_list_01;
            std::uint32_t vi_list_01_a;
            std::uint32_t vi_list_01_b;
            std::uint32_t vi_list_01_c;
            std::uint32_t vi_list_01_d;
            std::uint32_t vi_list_02;
            std::uint32_t vi_list_02_a;
            std::uint32_t vi_list_02_b;
            std::uint32_t vi_list_02_c;
            std::uint32_t vi_list_02_d;
            std::uint32_t vi_list_03;
            std::uint32_t vi_list_03_a;
            std::uint32_t vi_list_03_b;
            std::uint32_t vi_list_03_c;
            std::uint32_t vi_list_03_d;
            std::uint32_t vi_list_04;
            std::uint32_t vi_list_04_a;
            std::uint32_t vi_list_04_b;
            std::uint32_t vi_list_04_c;
            std::uint32_t vi_list_04_d;
            std::array<char, 200> vi_desc;
            bool vi_isgift;

			constexpr std::uint32_t getId() const noexcept { return vi_id; }
            constexpr bool isValid() const noexcept { return vi_id != static_cast<std::uint32_t>(-1); }
		};
PACK_POP()
    }
}

VISITABLE_STRUCT(Common::ConstantDatabase::CdbVendorInfo,
    vi_id, vi_category, vi_type, vi_array_none, vi_array_new, vi_array_hit, vi_list_type,
    vi_list_01, vi_list_01_a, vi_list_01_b, vi_list_01_c, vi_list_01_d,
    vi_list_02, vi_list_02_a, vi_list_02_b, vi_list_02_c, vi_list_02_d,
    vi_list_03, vi_list_03_a, vi_list_03_b, vi_list_03_c, vi_list_03_d,
    vi_list_04, vi_list_04_a, vi_list_04_b, vi_list_04_c, vi_list_04_d,
    vi_desc, vi_isgift
);


#endif