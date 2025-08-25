#ifndef MAIN_TO_CAST_IPC_CALLBACKS_H
#define MAIN_TO_CAST_IPC_CALLBACKS_H

#include "Network/Packet.h"
#include "Network/Session.h"
#include "../Classes/RoomsManager.h"
#include "../../../MainServer/include/Structures/ClientData/Structures.h"

namespace Cast
{
    namespace Handlers
    {
        inline void handleMapId(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Common::Network::Session> session, 
            Cast::Classes::RoomsManager& roomsManager)
        {
            roomsManager.setMapFor(request.getSession(), request.getExtra());
            roomsManager.setModeFor(request.getSession(), request.getOption());
            session->asyncWrite(request);
        }

        enum InvisibilityType { SELF_NOT_INVISIBLE = 0, SELF_INVISIBLE, ALL_INVISIBLE, NONE_INVISIBLE };
        inline void handleInvisibleCmd(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Common::Network::Session> session,
            Cast::Classes::RoomsManager& roomsManager, Cast::Network::SessionsManager& sm)
        {
            auto roomOpt = roomsManager.getRoom(request.getSession());
            if (!roomOpt) return;
            auto& room = *roomOpt;

            if (auto s = sm.getSession(request.getSession()))
            {
                if (request.getExtra() == 0)  s->m_isInvisible = false;
                else if (request.getExtra() == 1)  s->m_isInvisible = true;
            }
            if (request.getExtra() == 2)  room->m_isInvisible = true;
            else if (request.getExtra() == 3) room->m_isInvisible = false;
        }

        inline void handleAssassinMode(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Common::Network::Session> session,
            Cast::Classes::RoomsManager& roomsManager)
        {
            struct AssassinData
            {
                bool isAssassinMode{};
                Main::Structures::UniqueId uidRed{};
                char nameRed[16]{};
                Main::Structures::UniqueId uidBlue{};
                char nameBlue[16]{};
            } ainfo;
            ainfo = *reinterpret_cast<const AssassinData*>(request.getData());
            roomsManager.setAssassinModeInfoFor(request.getSession(), ainfo.uidBlue, ainfo.nameBlue, ainfo.uidRed, ainfo.nameRed, ainfo.isAssassinMode);
            session->asyncWrite(request);
        }

        inline void handleRoomNumber(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Common::Network::Session> session,
            Cast::Classes::RoomsManager& roomsManager)
        {
            roomsManager.setRoomNumberFor(request.getSession(), request.getExtra());
            session->asyncWrite(request);
        }

        inline void handleIpReq(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Common::Network::Session> session)
        {
            auto response = request;
            const std::string ip = reinterpret_cast<const char*>(request.getData());

            if (session->s_loggedIps.find(ip) == session->s_loggedIps.end())
            {
                if (session->s_loggedIps.size() >= (Common::Constants::maxSessionsPerServer * 4))
                {
                    const std::string& oldestIp = session->ipQueue.front();
                    session->s_loggedIps.erase(oldestIp);
                    session->ipQueue.pop_front();
                }

                session->s_loggedIps.insert(ip);
                session->ipQueue.push_back(ip);
            }
            session->asyncWrite(request);
        }

        inline void handlePlayerTeamInfoBatch(const Common::Network::UnecryptedPacket& request,  std::shared_ptr<Common::Network::Session> session,
            Cast::Classes::RoomsManager& roomsManager)
        {
            const std::uint8_t* data = request.getData(); 
            std::size_t dataSize = request.getDataSize();

            if (dataSize % sizeof(Main::ClientData::PlayerTeamInfo) != 0)
            {
                ::Utils::Logger::log("Invalid player team info batch size", ::Utils::LogType::Error, "Cast::handlePlayerTeamInfoBatch");
                return;
            }

            std::size_t numPlayers = dataSize / sizeof(Main::ClientData::PlayerTeamInfo);

            std::vector<Main::ClientData::PlayerTeamInfo> players(numPlayers);
            std::memcpy(players.data(), data, dataSize);

          
            roomsManager.setPlayerTeamsFor(request.getSession(), players);
            session->asyncWrite(request);
        }

    }
}

#endif