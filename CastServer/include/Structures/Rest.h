
#ifndef REST_STRUCTS_H
#define REST_STRUCTS_H

#include <cstdint>
#include "../../../MainServer/include/Structures/AccountInfo/MainAccountUniqueId.h"
#include "Macros.h"

namespace Cast
{
    namespace Structures
    {
PACK_PUSH(1)
        struct SpecialItem // Used to send special items for assassin mode
        {
            std::uint32_t number{};
            std::uint32_t itemId{};
            Main::Structures::UniqueId uid{};
        };
PACK_POP()

PACK_PUSH(1)
        struct SpecialItemUse
        {
            std::uint32_t itemId{};
            Main::Structures::UniqueId uid{};
        };
PACK_POP()

PACK_PUSH(1)
        struct RespawnCoord 
        {
            std::int32_t x;
            std::int32_t y;
            std::int32_t z;
            std::int32_t w = 0;
        };
PACK_POP()
    }
}

#endif