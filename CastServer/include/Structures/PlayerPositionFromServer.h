
#ifndef PLAYER_POSITION_FROM_SERVER_H
#define PLAYER_POSITION_FROM_SERVER_H

#include <cstdint>
#include "PlayerPositionFromClient.h"
#include "Macros.h"

namespace Cast
{
    namespace Structures
    {

PACK_PUSH(1)
        struct SpecificInfo
        {
            std::uint32_t sessionId : 14 = 0;
            std::uint32_t enableMovement : 1 = true;
            std::uint32_t enableBullet : 1 = false;
            std::uint32_t animation1 : 7 = 0;
            std::uint32_t enableRotation : 1 = true;
            std::uint32_t animation2 : 6 = 0;
            std::uint32_t unknown : 1 = true;
            std::uint32_t enableJump : 1 = false;
        };
PACK_POP()

PACK_PUSH(1)
        struct PlayerInfoBasicResponse
        {
           // std::uint32_t tick{};
            SpecificInfo specificInfo{}; 
            Cast::Structures::PositionStruct position;   
            Cast::Structures::DirectionStruct direction; 
            std::uint32_t rotation1 : 8 = 0; 
            std::uint32_t rotation2 : 8 = 0; 
            std::uint32_t rotation3 : 9 = 0;
            std::uint32_t currentWeapon : 4 = 0; 
        };
PACK_POP()

PACK_PUSH(1)
        struct PlayerInfoResponseWithJump
        {
            PlayerInfoBasicResponse playerInfoBasicResponse;
            Cast::Structures::JumpStruct jump{};
        };
PACK_POP()

PACK_PUSH(1)
        struct PlayerInfoResponseWithBullets
        {
           // std::uint32_t tick{}; 
            SpecificInfo specificInfo{}; 
            Cast::Structures::PositionStruct position; 
            Cast::Structures::DirectionStruct direction; 
            Cast::Structures::BulletsStruct bullets{}; 
            std::uint32_t rotation1 : 8 = 0;  
            std::uint32_t rotation2 : 8 = 0; 
            std::uint32_t rotation3 : 9 = 0; 
            std::uint32_t currentWeapon : 4 = 0; 
        };
PACK_POP()

PACK_PUSH(1)
        struct PlayerInfoResponseComplete
        {
            PlayerInfoResponseWithBullets playerInfoBasicResponse;
            Cast::Structures::JumpStruct jump{};
        };
PACK_POP()

    }
}

#endif