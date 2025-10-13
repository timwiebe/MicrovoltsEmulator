#ifndef FRIEND_STRUCTURE_H
#define FRIEND_STRUCTURE_H

#include <cstdint>
#include <string>
#include "../AccountInfo/MainAccountUniqueId.h"
#include <functional>
#include "Macros.h"

namespace Main
{
    namespace Structures
    {
PACK_PUSH(1)
        struct Friend
        {
            UniqueId targetUniqueId{};
            std::uint32_t targetAccountId{};
            char targetNickname[16];

            bool operator==(const Friend& other) const
            {
                return targetAccountId == other.targetAccountId;
            }
        };
PACK_POP()
    }
}

template<>
struct std::hash<Main::Structures::Friend>
{
    std::size_t operator()(const Main::Structures::Friend& f) const noexcept
    {
        return std::hash<std::uint32_t>{}(f.targetAccountId);
    }
};

#endif