#ifndef ROOM_PLAYER_CLAN_H
#define ROOM_PLAYER_CLAN_H

#include <cstdint>
#include "Macros.h"

namespace Main
{
	namespace Structures
	{
PACK_PUSH(1)
		struct PlayerClan
		{
			char clanName[16]{};
			std::uint64_t playerRoomIdx : 4 = 0; // 0 = clan displayed,  1 or 14 = no clan displayed (test other values)
			std::uint64_t clanLogoFrontId : 16 = 0;
			std::uint64_t clanLogoBackId : 14 = 0;
			std::uint64_t unknown2 : 27 = 0; // maybe clan id?
			std::uint64_t unused : 3 = 0;
		};
PACK_POP()
	}
}
#endif