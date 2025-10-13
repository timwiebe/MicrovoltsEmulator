#ifndef SINGLE_LOBBY_CLAN_LIST_H
#define SINGLE_LOBBY_CLAN_LIST_H

#include "../../../include/Structures/AccountInfo/MainAccountUniqueId.h"
#include "Macros.h"

namespace Main
{
    namespace Structures
    {
PACK_PUSH(1)
        struct SingleLobbyClanList
        {
            char name[16]{};
            UniqueId uniqueId{};
            std::uint16_t level : 7;
            std::uint16_t unkown = 0;
        };
PACK_POP()
    }
}


#endif