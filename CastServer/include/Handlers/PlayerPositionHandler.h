#ifndef PLAYER_POSITION_HANDLER_H
#define PLAYER_POSITION_HANDLER_H

#include "../Network/CastSession.h"
#include "../Network/SessionsManager.h"
#include "../Structures/PlayerPositionFromServer.h"
#include "../Structures/SuicideStruct.h"

#include "Utils/Logger.h"
#include "AntiCheat/AntiCheat.h"
#include "AntiCheat/Event.h"
#include "../Utils/Utilities.h"
#include <Utils/Utils.h>

namespace Cast
{
    namespace Handlers
    {
        inline void handlePlayerPosition(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session,
            Cast::Classes::RoomsManager& roomsManager, std::uint32_t serverId, Cast::Network::SessionsManager& sm,
            Ac::AntiCheatManager& acManager)
        {
            using namespace Cast::Structures;

            auto roomOpt = roomsManager.getRoom(session->getId());
            if (!roomOpt) return;
            auto& room = *roomOpt;

            Cast::Structures::ClientPlayerInfoBasic playerPositionFromClient = Cast::Details::parseData<Cast::Structures::ClientPlayerInfoBasic>(request);
            if (playerPositionFromClient.isBad()) return;

            /*
            if (room->m_isInvisible || session->m_isInvisible)
            {
                playerPositionFromClient.position.positionZ = 0;
            }
            */

            if (room->m_isAssassinMode)
            {
                const auto seid = session->getId();
                if (room->m_assassinBlueUid.session == seid)
                {
                    room->m_blueAssassinPos = playerPositionFromClient.position;
                }
                else if (room->m_assassinRedUid.session == seid)
                {
                    room->m_redAssassinPos = playerPositionFromClient.position;;
                }
            }
            else
            {
                acManager.submitEvent(std::make_unique<Ac::PacketFloodingEvent>(session, 20, 1000, "Speed hack (Cheat Engine)", 281));
            }

            static Common::Network::UnecryptedPacket response{ 1440, 322, 1 };
            response.setCommand(322, 0, 0, 1);
            const auto fullSize = request.getFullSize();
            

            if (fullSize == 36)
            {
                Cast::Structures::ClientPlayerInfoBullet playerPositionBullet{};
                std::memcpy(&playerPositionBullet, request.getData(), sizeof(playerPositionBullet));
                if (playerPositionBullet.isBad())
                {
                    ::Utils::Logger::log("Bad Player Respawn Position", ::Utils::LogType::Warning, "Cast::handlePlayerPosition");
                    return;
                }

                PlayerInfoResponseWithBullets playerInfoResponseWithBullets;
                playerInfoResponseWithBullets.specificInfo.enableBullet = true;

                playerInfoResponseWithBullets.tick = playerPositionFromClient.matchTick;
                playerInfoResponseWithBullets.position = playerPositionFromClient.position;
                playerInfoResponseWithBullets.direction = playerPositionFromClient.direction;
                playerInfoResponseWithBullets.specificInfo.animation1 = playerPositionFromClient.animation1;
                playerInfoResponseWithBullets.specificInfo.animation2 = playerPositionFromClient.animation2;
                playerInfoResponseWithBullets.rotation1 = request.getExtra();
                playerInfoResponseWithBullets.rotation2 = request.getOption();
                playerInfoResponseWithBullets.rotation3 = playerPositionFromClient.rotation;
                playerInfoResponseWithBullets.specificInfo.sessionId = static_cast<std::uint32_t>(session->getId());
                playerInfoResponseWithBullets.bullets = playerPositionBullet.bulletStruct;
                playerInfoResponseWithBullets.currentWeapon = playerPositionBullet.bulletStruct.bullet4;

                response.setData(reinterpret_cast<std::uint8_t*>(&playerInfoResponseWithBullets), sizeof(playerInfoResponseWithBullets));

            }
            else if (fullSize == 40)
            {
                Cast::Structures::ClientPlayerInfoComplete playerPositionComplete{};
                std::memcpy(&playerPositionComplete, request.getData(), request.getDataSize());
                if (playerPositionComplete.isBad()) return;

                Cast::Structures::ClientPlayerInfoBullet playerPositionBullet{};
                std::memcpy(&playerPositionBullet, request.getData(), sizeof(playerPositionBullet));
                if (playerPositionBullet.isBad()) return;

                PlayerInfoResponseWithBullets playerInfoResponseWithBullets;
                playerInfoResponseWithBullets.specificInfo.enableBullet = true;

                playerInfoResponseWithBullets.tick = playerPositionFromClient.matchTick;
                playerInfoResponseWithBullets.position = playerPositionFromClient.position;
                playerInfoResponseWithBullets.direction = playerPositionFromClient.direction;
                playerInfoResponseWithBullets.specificInfo.animation1 = playerPositionFromClient.animation1;
                playerInfoResponseWithBullets.specificInfo.animation2 = playerPositionFromClient.animation2;
                playerInfoResponseWithBullets.rotation1 = request.getExtra();
                playerInfoResponseWithBullets.rotation2 = request.getOption();
                playerInfoResponseWithBullets.rotation3 = playerPositionFromClient.rotation;
                playerInfoResponseWithBullets.specificInfo.sessionId = static_cast<std::uint32_t>(session->getId());
                playerInfoResponseWithBullets.bullets = playerPositionBullet.bulletStruct;
                playerInfoResponseWithBullets.currentWeapon = playerPositionBullet.bulletStruct.bullet4;

                PlayerInfoResponseComplete playerInfoResponseComplete{ playerInfoResponseWithBullets };
                playerInfoResponseComplete.playerInfoBasicResponse.specificInfo.enableJump = true;

                playerInfoResponseComplete.jump = playerPositionComplete.jumpStruct;
                response.setData(reinterpret_cast<std::uint8_t*>(&playerInfoResponseComplete), sizeof(playerInfoResponseComplete));
            }
            else
            {
                PlayerInfoBasicResponse playerInfoBasicResponse;
                playerInfoBasicResponse.tick = playerPositionFromClient.matchTick;
                playerInfoBasicResponse.position = playerPositionFromClient.position;
                playerInfoBasicResponse.direction = playerPositionFromClient.direction;
                playerInfoBasicResponse.currentWeapon = playerPositionFromClient.weapon;
                playerInfoBasicResponse.specificInfo.animation1 = playerPositionFromClient.animation1;
                playerInfoBasicResponse.specificInfo.animation2 = playerPositionFromClient.animation2;
                playerInfoBasicResponse.rotation1 = request.getExtra();
                playerInfoBasicResponse.rotation2 = request.getOption();
                playerInfoBasicResponse.rotation3 = playerPositionFromClient.rotation;
                playerInfoBasicResponse.specificInfo.sessionId = static_cast<std::uint32_t>(session->getId());

                
                if (fullSize == 28)
                {
                    response.setData(reinterpret_cast<std::uint8_t*>(&playerInfoBasicResponse), sizeof(playerInfoBasicResponse));
                }
                else if (fullSize == 32)
                {
                    playerInfoBasicResponse.specificInfo.enableJump = true;
                    PlayerInfoResponseWithJump playerInfoResponseWithJump{ playerInfoBasicResponse };
                 

                    Cast::Structures::ClientPlayerInfoJump playerPositionJump{};
                    std::memcpy(&playerPositionJump, request.getData(), request.getDataSize());
                    if (playerPositionJump.isBad()) return;

                    playerInfoResponseWithJump.jump = playerPositionJump.jumpStruct;
                    response.setData(reinterpret_cast<std::uint8_t*>(&playerInfoResponseWithJump), sizeof(playerInfoResponseWithJump));
                }
                else
                {
                    ::Utils::Logger::log("Unknown player position case!", ::Utils::LogType::Error, "Cast::handlePlayerPosition");
                    return;
                }
            }
            
            // reminder: this is correct, dont use exceptSelf as that causes log errors in SysLog (host receives [322] command not found) 
            room->broadcastToRoom(response);
        }
    }
}

#endif
