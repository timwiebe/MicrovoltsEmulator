#ifndef BLOCKED_PLAYER_STRUCTURE_H
#define BLOCKED_PLAYER_STRUCTURE_H

#include <cstdint>
#include "Macros.h"

namespace Main
{
    namespace Structures
    {
PACK_PUSH(1)
        struct BlockedPlayer
        {
            std::uint32_t targetAccountId{ static_cast<std::uint32_t>(-1) }; // sentinel
            char targetNickname[16];
        };
PACK_POP()
    }
}

#endif