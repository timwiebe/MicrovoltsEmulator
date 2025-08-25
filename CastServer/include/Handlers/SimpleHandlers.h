#ifndef CAST_SIMPLE_HANDLERS_H
#define CAST_SIMPLE_HANDLERS_H

#include "Network/Session.h"
#include "../Network/CastSession.h"
#include "../../../MainServer/include/Structures/AccountInfo/MainAccountUniqueId.h"
#include "../Classes/RoomsManager.h"
#include <Utils/SetupParser.h>
#include "../Utils/Utilities.h"
#include <vector>
#include "../Structures/PlayerPositionFromClient.h"
#include "../Network/SessionsManager.h"
#include "../Structures/Rest.h"
#include <asio/post.hpp>
#include "AntiCheat/AntiCheat.h"
#include "AntiCheat/Event.h"

namespace Cast
{
    namespace Handlers
    {
        inline bool sendPlayerStateUpdate(std::uint32_t accountID, bool isDead)
        {
            Common::Network::UnecryptedPacket packet;

            try
            {
                asio::io_context ioContext;
                asio::ip::tcp::socket socket(ioContext);
                asio::ip::tcp::resolver resolver(ioContext);

                auto selfMainServerInfo = Common::Utils::SetupParser::getInstance().getSelfMainServerInfo();

                asio::error_code ec;
                auto endpoints = resolver.resolve(selfMainServerInfo.ip, std::to_string(selfMainServerInfo.ipcPort), ec);
                if (ec)
                {
                    ::Utils::Logger::log("Failed to resolve " + selfMainServerInfo.ip, ::Utils::LogType::Warning, "sendPlayerStateUpdate");
                    return false;
                }

                asio::connect(socket, endpoints, ec);
                if (ec)
                {
                    ::Utils::Logger::log("Failed to connect to " + selfMainServerInfo.ip, ::Utils::LogType::Warning, "sendPlayerStateUpdate");
                    return false;
                }

                socket.set_option(asio::ip::tcp::no_delay(true));
                packet.setTcpHeader(0);
                packet.setCommand(Common::Constants::C2M_updatePlayerState, 0, 0, isDead ? Common::Enums::STATE_DYING : Common::Enums::STATE_NORMAL);
                packet.setData(reinterpret_cast<const std::uint8_t*>(&accountID), sizeof(accountID));
                asio::write(socket, asio::buffer(packet.generateOutgoingPacket()), ec);
                if (ec)
                {
                    ::Utils::Logger::log("Failed to send player state update to " + selfMainServerInfo.ip, ::Utils::LogType::Warning, "sendPlayerStateUpdate");
                    return false;
                }
                return true;
            }
            catch (const std::exception& e)
            {
                ::Utils::Logger::log("Exception: " + std::string(e.what()), ::Utils::LogType::Error, "sendPlayerStateUpdate");
            }

            return false;
        }


        inline std::optional<std::uint32_t> getSessionId(std::uint32_t aid)
        {
            Common::Network::UnecryptedPacket packet;
            Common::Network::UnecryptedPacket responsePacket;

            try
            {
                asio::io_context ioContext;
                asio::ip::tcp::socket socket(ioContext);
                asio::ip::tcp::resolver resolver(ioContext);

                auto selfMainServerInfo = Common::Utils::SetupParser::getInstance().getSelfMainServerInfo();

                asio::error_code ec;
                auto endpoints = resolver.resolve(selfMainServerInfo.ip, std::to_string(selfMainServerInfo.ipcPort), ec);
                if (ec)
                {
                    ::Utils::Logger::log("Failed to resolve " + selfMainServerInfo.ip, ::Utils::LogType::Warning, "Cast::getSessionId");
                    return std::nullopt;
                }

                asio::connect(socket, endpoints, ec);
                if (ec)
                {
                    ::Utils::Logger::log("Failed to connect to " + selfMainServerInfo.ip, ::Utils::LogType::Warning, "Cast::getSessionId");
                    return std::nullopt;
                }

                socket.set_option(asio::ip::tcp::no_delay(true));
                packet.setTcpHeader(0);
                packet.setCommand(Common::Constants::M2C_sessionId, 0, 0, 0);
                packet.setData(reinterpret_cast<std::uint8_t*>(&aid), sizeof(aid));

                asio::write(socket, asio::buffer(packet.generateOutgoingPacket()), ec);
                if (ec)
                {
                    ::Utils::Logger::log("Failed to send request to " + selfMainServerInfo.ip, ::Utils::LogType::Warning, "Cast::getSessionId");
                    return std::nullopt;
                }

                std::vector<std::uint8_t> responseBuffer(Common::Constants::maxPacketBytes);
                asio::steady_timer timer(ioContext);
                bool timeoutOccurred = false;
                std::size_t bytesRead = 0;

                timer.expires_after(std::chrono::seconds(15));
                timer.async_wait([&](const asio::error_code& error) {
                    if (!error)
                    {
                        timeoutOccurred = true;
                        socket.cancel();
                    }
                    });

                socket.async_read_some(asio::buffer(responseBuffer), [&](const asio::error_code& error, std::size_t length) {
                    if (!error)
                        bytesRead = length;
                    timer.cancel();
                    });

                ioContext.run();

                if (timeoutOccurred || bytesRead == 0)
                {
                    ::Utils::Logger::log("Timeout / No response from " + selfMainServerInfo.ip, ::Utils::LogType::Warning, "Cast::getSessionId");
                    return std::nullopt;
                }

                if (bytesRead > responseBuffer.size())
                {
                    ::Utils::Logger::log("Buffer overflow detected, bytesRead: " + std::to_string(bytesRead), ::Utils::LogType::Error, "Cast::getSessionId");
                    return std::nullopt;
                }

                responsePacket.processIncomingPacket(responseBuffer.data(), bytesRead);

                if (responsePacket.getOrder() != Common::Constants::M2C_sessionId || responsePacket.getExtra() == 1)
                {
                    ::Utils::Logger::log("Invalid session response", ::Utils::LogType::Warning, "Cast::getSessionId");
                    return std::nullopt;
                }

                if (responsePacket.getDataSize() == sizeof(std::uint32_t))
                {
                    const std::uint8_t* dataPtr = responsePacket.getData();
                    if (!dataPtr)
                    {
                        ::Utils::Logger::log("DataPtr was null", ::Utils::LogType::Error, "Cast::getSessionId");
                        return std::nullopt;
                    }

                    std::uint32_t sessionId;
                    std::memcpy(&sessionId, dataPtr, sizeof(sessionId));
                    return sessionId;
                }
                else
                {
                    ::Utils::Logger::log("Unexpected data size: " + std::to_string(responsePacket.getDataSize()), ::Utils::LogType::Error, "Cast::getSessionId");
                }
            }
            catch (const std::exception& e)
            {
                ::Utils::Logger::log("Exception: " + std::string(e.what()), ::Utils::LogType::Error, "Cast::getSessionId");
            }

            return std::nullopt;
        }
        

        inline void connectionHandler(const Common::Network::UnecryptedPacket& request,
            std::shared_ptr<Cast::Network::Session> session,
            Cast::Network::SessionsManager& sessionsManager,
            asio::io_context& ioContext)
        {
            const std::uint32_t aid = Cast::Details::parseData<std::uint32_t>(request, 4);
            std::jthread([aid, session, &sessionsManager, &ioContext]() {
                std::optional<std::uint32_t> retrievedSessionId = getSessionId(aid);

                asio::post(ioContext.get_executor(), [retrievedSessionId, session, &sessionsManager, aid]() {
                    if (!retrievedSessionId)
                    {
                        session->closeSocket(); 
                        return;
                    }

                    session->setAccountId(aid);
                    session->setSessionId(*retrievedSessionId);
                    sessionsManager.addSession(session, *retrievedSessionId);

                    Common::Network::UnecryptedPacket response;
                    response.setTcpHeader(session->getId());
                    response.setCommand(501, 0, 32, 0);
                    session->asyncWrite(response);
                    });
                });
        }
            

        inline void pongHandler(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session, 
            Cast::Classes::RoomsManager& roomsManager,
            Cast::Network::SessionsManager& sessionsManager, std::uint32_t m_serverId)
        {
            Common::Network::UnecryptedPacket response;
            response.setTcpHeader(session->getId());
            response.setCommand(72, 1, 0, request.getOption());
            session->asyncWrite(response);

            sessionsManager.addSession(session, session->getId());

            auto roomOpt = roomsManager.getRoom(session->getId());
            if (!roomOpt) return;
            auto& room = *roomOpt;

            if (room->m_isAssassinMode)
            {
                static Cast::Structures::SpecialItem speedItem{ 327680, 4011000 };
                static Cast::Structures::SpecialItem powerItem{ 1376256, 4010000 };
                static Cast::Structures::SpecialItemUse speedItemUse{ 4011000 };
                static Cast::Structures::SpecialItemUse powerItemUse{ 4010000 };

                if (session->getId() == room->m_assassinBlueUid.session || session->getId() == room->m_assassinRedUid.session)
                {
                    speedItem.uid.session = powerItem.uid.session = speedItemUse.uid.session = powerItemUse.uid.session = session->getId();
                    speedItem.uid.server = powerItem.uid.server = speedItemUse.uid.server = powerItemUse.uid.server = m_serverId;

                    response.setCommand(262, 0, 0, 0);
                    response.setData(reinterpret_cast<std::uint8_t*>(&speedItem), sizeof(speedItem)); // get speed item
                    room->broadcastToMatch(response);
                    response.setCommand(263, 0, 1, 0);
                    response.setData(reinterpret_cast<std::uint8_t*>(&speedItemUse), sizeof(speedItemUse));
                    room->broadcastToMatch(response); // use speed item

                    response.setCommand(262, 0, 0, 0);
                    response.setData(reinterpret_cast<std::uint8_t*>(&powerItem), sizeof(powerItem)); // get power item
                    room->broadcastToMatch(response);
                    response.setCommand(263, 0, 1, 0);
                    response.setData(reinterpret_cast<std::uint8_t*>(&powerItemUse), sizeof(powerItemUse)); // use power item
                    room->broadcastToMatch(response);
                }
            }
        }

        inline void handleCrash(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session, 
            Cast::Classes::RoomsManager& roomsManager,
            std::uint32_t serverId)
        {
            session->setIsInMatch(false);
            Main::Structures::UniqueId uniqueId{ static_cast<std::uint16_t>(session->getId()), serverId, 0 };
            Common::Network::UnecryptedPacket response = request;
            response.setData(reinterpret_cast<std::uint8_t*>(&uniqueId), sizeof(uniqueId));
            roomsManager.playerForwardToHost(request.getSession(), session->getId(), response);

            // This sets is in match to false, roomNum to -1, removes player from room, removes player from roomsManager's unordered_map, and closes room if necessary
            roomsManager.removePlayerFromRoom(session->getId());
        }

        inline void handleEliminationNextRound(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session, 
            Cast::Classes::RoomsManager& roomsManager)
        {
            // Just ignore this on the cast server, otherwise it all stops working for some reason...
            //roomsManager.printRoomInfo(session.getRoomId(), "After EliminationNextRoundHandler");
        }

        inline void handleMatchInitialLoading(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session, 
            Cast::Classes::RoomsManager& roomsManager,
            std::uint32_t serverId)
        {
            auto roomOpt = roomsManager.getRoom(session->getId());
            if (!roomOpt) return;
            auto& room = *roomOpt;

            Common::Network::UnecryptedPacket response;
            response.setTcpHeader(session->getId());
            response.setCommand(request.getOrder(), 0, 0, request.getOption());
            Main::Structures::UniqueId uniqueId{ static_cast<std::uint16_t>(session->getId()), serverId, 0 };
            response.setData(reinterpret_cast<std::uint8_t*>(&uniqueId), sizeof(uniqueId));
            room->broadcastToMatch(response);

            if (request.getOption() == 9)
            {
                if (Cast::Details::mustBroadcastDeath(room->getMode()))
                {
                    sendPlayerStateUpdate(session->getAccountId(), true);
                }
                session->isDead = true;
                session->m_isInMatch = true;


                if (room->m_isAssassinMode && request.getSession() == session->getId())
                {
                    session->sendMessage("Assassin for RED team: " + room->m_assassinRedName);
                    session->sendMessage("Assassin for BLUE team: " + room->m_assassinBlueName);
                }
            }
        }

        inline void handlePlayerRespawn(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session, 
            Cast::Classes::RoomsManager& roomsManager,
            Cast::Network::SessionsManager& sessionsManager,
            Ac::AntiCheatManager& acManager)
        {
            auto roomOpt = roomsManager.getRoom(session->getId());
            if (!roomOpt) return;
            auto& room = *roomOpt;

            Common::Network::UnecryptedPacket response;
            response.setTcpHeader(session->getId());
            response.setCommand(request.getOrder(), 0, 0, 0);

            struct PlayerRespawnPosition
            {
                std::uint16_t x = 0;
                std::uint16_t y = 0;
                std::uint16_t z = 0;
                std::uint16_t w = 0;
                Main::Structures::UniqueId targetUniqueId{};

                bool isNaNOrInfinity(std::uint16_t half) const
                {
                    std::uint16_t exponent = (half >> 10) & 0x1F;
                    return (exponent == 0x1F); 
                }

                bool isBad() const
                {
                    if (isNaNOrInfinity(x) || isNaNOrInfinity(y) || isNaNOrInfinity(z) || isNaNOrInfinity(w))
                        return true;
                    return false;
                }
            };

            PlayerRespawnPosition playerRespawnPosition = Cast::Details::parseData<PlayerRespawnPosition>(request);
            if (room->isArenaMode() && room->m_hasMatchStarted)
            { // academy ground has special respawns, in other maps => dead players are just invisible
                playerRespawnPosition.x = 60590;
                playerRespawnPosition.y = 58810;
                playerRespawnPosition.z = 26000;
            }
            if (playerRespawnPosition.isBad())
            {
                ::Utils::Logger::log("Bad Player Respawn Position", ::Utils::LogType::Warning, "Cast::handlePlayerRespawn");
                session->asyncWrite(response);
            }
            else
            {
                response.setData(reinterpret_cast<std::uint8_t*>(&playerRespawnPosition), sizeof(playerRespawnPosition));
                room->broadcastToMatch(response);
            }
            
            if (auto targetSession = sessionsManager.getSession(playerRespawnPosition.targetUniqueId.session); targetSession)
            {                
                if (targetSession->m_team == Common::Enums::TEAM_OBSERVER) return;

                targetSession->isDead = (room->isArenaMode() && room->m_hasMatchStarted) ? true : false;
                targetSession->m_isInMatch = true;
                session->m_isInMatch = true;
                if (Cast::Details::mustBroadcastDeath(room->getMode()))
                {
                    sendPlayerStateUpdate(targetSession->getAccountId(), false);
                }
            }
            else
            {
                session->sendMessage("Server-side error: no target player to respawn with sessionID: " + std::to_string(playerRespawnPosition.targetUniqueId.session));
            }
        }

        // This is a request from the client to get the Room Info (e.g. how many wins in which team, etc)
        inline void roomInfoHandler(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session,
            Cast::Classes::RoomsManager& roomsManager)
        {
            const auto roomHostSessionId = request.getSession();
            const auto selfSessionId = session->getId();
            // roomsManager.playerForwardToHost(session.getRoomId(), selfSessionId, const_cast<Common::Network::Packet&>(request));
            // This command does NOT exist in the client! [order = 255]
        }

        inline void handleMatchStart(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session, 
            Cast::Classes::RoomsManager& roomsManager)
        {
            const auto receiverSessionId = request.getSession();
            const auto hostSessionId = session->getId();

            // For some reason, whenever a new round starts in elimination, the client resends this packet to the host
            // And if we resend this packet to the host client, it resets the score to 0... so we just avoid doing that.
            if (hostSessionId != receiverSessionId)
            {
                roomsManager.hostForwardToPlayer(hostSessionId, receiverSessionId, const_cast<Common::Network::UnecryptedPacket&>(request));
            }
        }

        inline void unknownHandler3(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session, 
            Cast::Classes::RoomsManager& roomsManager)
        {
            roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
        }

        inline void roomInfoJoinHandler(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session, 
            Cast::Classes::RoomsManager& roomsManager,
            Cast::Network::SessionsManager& sessionsManager)
        {
            const std::uint32_t mode = roomsManager.getModeOf(session->getId());
            std::vector<Cast::Structures::SinglePlayerJoinInfoResponse> singleInfoResp(request.getOption());
            const std::uint8_t* dataPtr = request.getData();
            for (std::size_t i = 0; i < request.getOption(); ++i)
            {
                Cast::Structures::SinglePlayerJoinInfo sp = Cast::Details::parseData<Cast::Structures::SinglePlayerJoinInfo>(request, 
                    sizeof(Cast::Structures::SinglePlayerJoinInfo) * i);
                
                if (auto targetSession = sessionsManager.getSession(sp.uid.session); targetSession)
                {
                    singleInfoResp[i].uid = sp.uid;
                    singleInfoResp[i].playerState = targetSession->isDead ? Common::Enums::STATE_DYING : Common::Enums::STATE_NORMAL;
                    singleInfoResp[i].mode = mode == Common::Enums::MODES_MAX ? 0 : mode;
                }
            }
            auto response = request;
            response.setMission(2);
            response.setData(reinterpret_cast<std::uint8_t*>(singleInfoResp.data()), singleInfoResp.size() * sizeof(Cast::Structures::SinglePlayerJoinInfoResponse));

            roomsManager.hostForwardToPlayer(session->getId(), request.getSession(), const_cast<Common::Network::UnecryptedPacket&>(request), false);
            roomsManager.hostForwardToPlayer(session->getId(), request.getSession(), response, false);
        }

        template<Common::Enums::PlayerType PlayerType>
        inline void handleItemPickup(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session, 
            Cast::Classes::RoomsManager& roomsManager)
        {
            if (session->m_team == Common::Enums::TEAM_OBSERVER || !session->m_isInMatch) return;

            if constexpr (PlayerType == Common::Enums::HOST)
            {
                roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
            }
            else if constexpr (PlayerType == Common::Enums::NON_HOST)
            {
                const auto hostSessionId = request.getSession();
                const auto senderId = session->getId();
                roomsManager.playerForwardToHost(hostSessionId, senderId, const_cast<Common::Network::UnecryptedPacket&>(request));
            }
        }

        template<Common::Enums::PlayerType PlayerType>
        inline void handleZombieAbility(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session,
            Cast::Classes::RoomsManager& roomsManager)
        {
            if (session->m_team == Common::Enums::TEAM_OBSERVER || !session->m_isInMatch) return;

            if constexpr (PlayerType == Common::Enums::HOST)
            {
                roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
            }
            else if constexpr (PlayerType == Common::Enums::NON_HOST)
            {
                const auto hostSessionId = request.getSession();
                const auto senderId = session->getId();
                roomsManager.playerForwardToHost(hostSessionId, senderId, const_cast<Common::Network::UnecryptedPacket&>(request));
            }
        }
    }
}

#endif