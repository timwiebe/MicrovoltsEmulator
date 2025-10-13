#ifndef CGD_EFFECTINFO_H
#define CGD_EFFECTINFO_H

#include "visit_struct/visit_struct.hpp"
#include <array>
#include "Macros.h"

namespace Common
{
	namespace ConstantDatabase
	{

PACK_PUSH(1)
		struct CdbEffectInfo
		{
			std::uint32_t ei_id;
			std::array<char, 20> ei_name;
			std::uint32_t ei_key;
			std::uint32_t ei_effect_type;
			std::uint32_t ei_sign;
			std::uint32_t ei_durable;
			std::uint32_t ei_buff;
			std::uint32_t ei_weapon_type;
			std::uint32_t ei_duration;
			std::uint32_t ei_interval;
			std::uint32_t ei_valueA;
			std::uint32_t ei_valueB;
			std::uint32_t ei_valueC;

			constexpr std::uint32_t getId() const noexcept { return ei_id; }
			constexpr bool isValid() const noexcept { return ei_id != static_cast<std::uint32_t>(-1); }
		};
PACK_POP()
	}
}

VISITABLE_STRUCT(Common::ConstantDatabase::CdbEffectInfo, ei_id, ei_name, ei_key, ei_effect_type, ei_sign, ei_durable, ei_buff, ei_weapon_type, ei_duration,
	ei_interval, ei_valueA, ei_valueB, ei_valueC );

#endif