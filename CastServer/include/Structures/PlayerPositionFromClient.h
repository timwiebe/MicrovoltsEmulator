#ifndef PLAYER_POSITION_STRUCTURE_H
#define PLAYER_POSITION_STRUCTURE_H

#include <cstdint>
#include <directxmath/DirectXPackedVector.h>
#include "AntiCheat/Event.h"
#include "Macros.h"

namespace Cast
{
	namespace Structures
	{
        inline bool isBadPosition(auto position)
        {
            std::uint32_t rawVal = reinterpret_cast<std::uint32_t&>(position);
            if (rawVal == -2147483648 || rawVal == 2139095040)
                return true;

            uint16_t rawBits = *reinterpret_cast<uint16_t*>(&position);  
            uint16_t exponentMask = 0x7C00; 
            uint16_t fractionMask = 0x03FF; 

            return (rawBits & exponentMask) == exponentMask && (rawBits & fractionMask) != 0 ||
                (rawBits & exponentMask) == exponentMask && (rawBits & fractionMask) == 0;  
        }

        inline bool isNaNOrInfinity(float value)
        {
            return std::isnan(value) || value == -2147483648 || value == 2139095040;
        }

        inline std::int32_t halfToFloat(std::uint16_t half)
        {
            std::int32_t exponent = (half >> 10) & 0x1F;
            std::int32_t sign = half >> 15;
            std::int32_t mantissa = half & 0x3FF;
            if (!exponent)
            {
                if ((half & 0x3FF) == 0)
                    return sign << 31;
                do
                {
                    mantissa *= 2;
                    --exponent;
                } while ((mantissa & 0x400) == 0);
                ++exponent;
                mantissa &= ~0x400u;
                return ((exponent + 112) << 23) | ((mantissa | (sign << 18)) << 13);
            }
            if (exponent != 31)
                return ((exponent + 112) << 23) | ((mantissa | (sign << 18)) << 13);
            if ((half & 0x3FF) != 0)
                return (mantissa | (sign << 18) | 0x3FC00) << 13; // NaN case
            else
                return (sign << 31) | 0x7F800000; // Infinity case
        }

PACK_PUSH(1)
        struct PositionStruct
        {
            DirectX::PackedVector::HALF positionX{};
            DirectX::PackedVector::HALF positionY{};
            DirectX::PackedVector::HALF positionZ{};

            bool isBadPos()
            {
                if (isNaNOrInfinity(halfToFloat(positionX)) || isNaNOrInfinity(halfToFloat(positionY)) || isNaNOrInfinity(halfToFloat(positionZ))) return true;
                return isBadPosition(positionX) || isBadPosition(positionY) || isBadPosition(positionZ);
            }
        };
PACK_POP()

PACK_PUSH(1)
        struct DirectionStruct
        {
            DirectX::PackedVector::HALF directionX{};
            DirectX::PackedVector::HALF directionY{};
            DirectX::PackedVector::HALF directionZ{};

            bool isBadDir()
            {
                if (isNaNOrInfinity(halfToFloat(directionX)) || isNaNOrInfinity(halfToFloat(directionY)) || isNaNOrInfinity(halfToFloat(directionZ))) return true;
                return isBadPosition(directionX) || isBadPosition(directionY) || isBadPosition(directionZ);
            }
        };
PACK_POP()

PACK_PUSH(1)
        struct BulletsStruct
        {
            DirectX::PackedVector::HALF bullet1{};
            DirectX::PackedVector::HALF bullet2{};
            DirectX::PackedVector::HALF bullet3{};
            DirectX::PackedVector::HALF bullet4{};

            bool isBadBullets()
            {
                if (isNaNOrInfinity(halfToFloat(bullet1)) || isNaNOrInfinity(halfToFloat(bullet2)) || isNaNOrInfinity(halfToFloat(bullet3))
                    || isNaNOrInfinity(halfToFloat(bullet4))) return true;
                return isBadPosition(bullet1) || isBadPosition(bullet2) || isBadPosition(bullet3) || isBadPosition(bullet4);
            }
        };
PACK_POP()


PACK_PUSH(1)
        struct JumpStruct
        {
            DirectX::PackedVector::HALF jump1;
            DirectX::PackedVector::HALF jump2;

            bool isBadJump()
            {
                if (isNaNOrInfinity(halfToFloat(jump1)) || isNaNOrInfinity(halfToFloat(jump2))) return true;
                return isBadPosition(jump1) || isBadPosition(jump2);
            }
        };
PACK_POP()
        
PACK_PUSH(1)
        struct ClientPlayerInfoBasic
        {
            PositionStruct position;
            DirectionStruct direction;
            std::uint32_t matchTick{};
            std::uint32_t animation1 : 7 = 0;
            std::uint32_t animation2 : 6 = 0;
            std::uint32_t weapon : 4 = 0;
            std::uint32_t rotation : 9 = 0;
            std::uint32_t unknown : 6 = 0;

            bool isBad()
            {
                return position.isBadPos() || direction.isBadDir();
            }

        };
PACK_POP()

PACK_PUSH(1)
        struct ClientPlayerInfoJump
        {
            ClientPlayerInfoBasic playerPositionBasic;
            JumpStruct jumpStruct{};

            bool isBad()
            {
                return playerPositionBasic.isBad() || jumpStruct.isBadJump();
            }
        };
PACK_POP()


PACK_PUSH(1)
        struct ClientPlayerInfoBullet
        {
            ClientPlayerInfoBasic playerPositionBasic;
            BulletsStruct bulletStruct{};

            bool isBad()
            {
                return playerPositionBasic.isBad() || bulletStruct.isBadBullets();
            }
        };
PACK_POP()


PACK_PUSH(1)
        struct ClientPlayerInfoComplete
        {
            ClientPlayerInfoBasic playerPositionBasic;
            BulletsStruct bulletStruct{};
            JumpStruct jumpStruct{};

            bool isBad()
            {
                return playerPositionBasic.isBad() || bulletStruct.isBadBullets() || jumpStruct.isBadJump();
            }
        };
PACK_POP()

PACK_PUSH(1)
        struct SinglePlayerJoinInfo
        {
            char u0[16]{};
            Main::Structures::UniqueId uid;
            std::uint32_t unknown{};
        };  
PACK_POP()

PACK_PUSH(1)
        struct SinglePlayerJoinInfoResponse
        {
            Main::Structures::UniqueId uid;
            std::uint32_t u0 : 20 = 0; // This may be zombie mode weapon ID or HP given by client, no idea about it -- could be related to special melee weapon in zombie mode
            std::uint32_t mode : 5 = 0; // client checks whether the mode is zombie for some reason
            std::uint32_t playerState : 4 = 0;
        };
PACK_POP()

PACK_PUSH(1)
        struct PlayerRespawnPosition
        {
            std::uint16_t x{};
            std::uint16_t y{};
            std::uint16_t z{};
            std::uint16_t w{};
            Main::Structures::UniqueId targetUniqueId{};

            PlayerRespawnPosition& operator=(const Cast::Structures::PositionStruct& pos)
            {
                x = pos.positionX;
                z = pos.positionZ;
                y = pos.positionY;

                return *this;
            }
        };
PACK_POP()
    }
}
#endif
