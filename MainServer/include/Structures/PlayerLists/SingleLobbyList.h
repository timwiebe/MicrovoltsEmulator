#ifndef SINGLE_LOBBY_LIST_H
#define SINGLE_LOBBY_LIST_H

#include "../../../include/Structures/AccountInfo/MainAccountUniqueId.h"
#include "Macros.h"

namespace Main
{
	namespace Structures
	{
PACK_PUSH(1)
        struct SinglePlayerInfoList
        {
            char name[16]{};
            UniqueId uniqueId{};
            std::uint64_t clanLogoFrontId : 16;
            std::uint64_t clanLogoBackId : 14;
            std::uint64_t level : 7;
        };
PACK_POP()
	}
}


#endif