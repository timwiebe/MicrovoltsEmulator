#ifndef CGD_MAP_INFO_H
#define CGD_MAP_INFO_H

#include "visit_struct/visit_struct.hpp"
#include <array>
#include "Macros.h"

namespace Common
{
	namespace ConstantDatabase
	{
PACK_PUSH(1)
			
		struct CdbMapInfo
		{
			std::uint32_t mi_id = static_cast<std::uint32_t>(-1);
			std::uint32_t mi_rid;
			std::array<char, 30> mi_name;
			std::uint32_t mi_max_user;
			std::uint32_t mi_recommend_max_use;
			std::uint32_t mi_death_height;
			std::uint32_t mi_exp;
			std::uint32_t mi_point;
			std::uint32_t mi_event_exp;
			std::uint32_t mi_event_point;
			bool mi_mod_tdm;
			bool mi_mod_ffa;
			bool mi_mod_itm;
			bool mi_mod_ctf;
			bool mi_mod_ctm;
			bool mi_mod_sab;
			bool mi_mod_cim;
			bool mi_mod_zsm;
			bool mi_mod_grm;
			bool mi_mod_mock;
			bool mi_mod_bmb;
			bool mi_mod_sni;
			bool mi_mod_nod;
			bool mi_mod_pve;
			bool mi_mod_bot;
			bool mi_mod_tut;
			bool mi_mod_cl_ctf;
			bool mi_mod_cl_sab;
			bool mi_mod_cl_tdm;
			bool mi_mod_cl_bmb;
			std::uint32_t mi_size;
			std::array<char, 100> mi_desc;
			std::uint32_t mi_HDR_GM;
			std::uint32_t mi_HDR_GD;
			std::uint32_t mi_HDR_MG;
			std::uint32_t mi_HDR_BT;
			std::uint32_t mi_HDR_MaxL;
			std::uint32_t mi_HDR_MinL;
			std::array<char, 50> mi_effect_filename;
			std::uint32_t mi_effect_repeat_time;
			std::array<char, 50> mi_sound_filename;
			std::uint32_t mi_sound_repeat_time;
			std::uint32_t mi_type;

			constexpr std::uint32_t getId() const noexcept { return mi_id; }
			constexpr bool isValid() const noexcept { return mi_id != static_cast<std::uint32_t>(-1); }
		};
PACK_POP()
	}
}

VISITABLE_STRUCT(Common::ConstantDatabase::CdbMapInfo, mi_id, mi_rid, mi_name, mi_max_user, mi_recommend_max_use, mi_death_height, mi_exp, mi_point, mi_event_exp, mi_event_point, mi_mod_tdm, mi_mod_ffa,
	mi_mod_itm, mi_mod_ctf, mi_mod_ctm, mi_mod_sab, mi_mod_cim, mi_mod_zsm, mi_mod_grm, mi_mod_mock, mi_mod_bmb, mi_mod_sni, mi_mod_nod, mi_mod_pve, mi_mod_bot, mi_mod_tut, mi_mod_cl_ctf, mi_mod_cl_sab, mi_mod_cl_tdm, mi_mod_cl_bmb, mi_size, mi_desc,
	mi_HDR_GM, mi_HDR_GD, mi_HDR_MG, mi_HDR_BT, mi_HDR_MaxL, mi_HDR_MinL, mi_effect_filename, mi_effect_repeat_time, mi_sound_filename, mi_sound_repeat_time, mi_type);

#endif