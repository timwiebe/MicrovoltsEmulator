#ifndef MAIN_ACOUNT_UNIQUE_ID_H
#define MAIN_ACOUNT_UNIQUE_ID_H

#include <cstdint>
#include "Macros.h"

namespace Main
{
    namespace Structures
    {
PACK_PUSH(1)
        struct UniqueId
        {
            std::uint32_t session : 16 = 0; 
            std::uint32_t server : 15 = 0; 
            std::uint32_t unknown : 1 = 0; 

            bool operator==(const UniqueId& other) const = default;
        };
PACK_POP()
    }
}

#endif