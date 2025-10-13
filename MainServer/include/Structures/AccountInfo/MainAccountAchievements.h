#ifndef MAIN_ACCOUNT_ACHIEVEMENTS_H
#define MAIN_ACCOUNT_ACHIEVEMENTS_H

#include "Enums/AchievementEnums.h"
#include "Utils/Logger.h"
#include <cstdint>
#include "Macros.h"

namespace Main
{
	namespace Structures
	{
PACK_PUSH(1)
        struct AccountAchievements
        {
            // MVS had probably only achievementsTier1
            std::uint64_t achievementsTier1{};
            std::uint64_t achievementsTier2{};
            std::uint64_t achievementsTier3{};
            std::uint64_t achievementsTier4{};

            void setAchievementTier1(std::uint32_t achievementIdx)
            {
                achievementsTier1 |= (static_cast<std::uint64_t>(1) << achievementIdx);
            }
        };
PACK_POP()
    }
}

#endif