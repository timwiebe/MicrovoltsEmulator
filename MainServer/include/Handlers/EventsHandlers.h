#ifndef MAIN_EVENTS_HANDLERS_H
#define MAIN_EVENTS_HANDLERS_H

#include "Network/Packet.h"
#include "../../include/Structures/MainEventsList.h"
#include "../Persistence/MainDatabaseManager.h"
#include "../Classes/Player.h"
#include "../../include/Network/MainSession.h"
#include <cstring> 

namespace Main
{
    namespace Handlers
    {
        inline void handleModeEvents(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session, Main::Persistence::MainScheduler& scheduler)
        {
            auto eventsList = scheduler.immediatePersist(std::source_location::current(), &Main::Persistence::PersistentDatabase::getEventsModeList);
            std::uint32_t totalEvents = eventsList.size();

            const std::size_t messageSize = totalEvents * sizeof(Main::Structures::SingleModeEvent) + sizeof(std::uint32_t);

            Common::Network::Packet response;
            response.setTcpHeader(request.getSession(), Common::Enums::NO_ENCRYPTION);
            response.setOrder(202);
            std::vector<std::uint8_t> message(messageSize);
            std::memcpy(message.data(), &totalEvents, sizeof(std::uint32_t));
            std::memcpy(message.data() + sizeof(std::uint32_t), eventsList.data(), totalEvents * sizeof(Main::Structures::SingleModeEvent));
            response.setData(message.data(), message.size());

            session->asyncWrite(response);
        }

        inline void handleMapEvents(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session, Main::Persistence::MainScheduler& scheduler)
        {
            auto eventsList = scheduler.immediatePersist(std::source_location::current(), &Main::Persistence::PersistentDatabase::getEventsMapList);

            std::uint32_t totalEvents = eventsList.size();
            const std::size_t messageSize = totalEvents * sizeof(Main::Structures::SingleMapEvent) + sizeof(std::uint32_t);

            Common::Network::Packet response;
            response.setTcpHeader(request.getSession(), Common::Enums::NO_ENCRYPTION);
            response.setOrder(203);
            std::vector<std::uint8_t> message(messageSize);
            std::memcpy(message.data(), &totalEvents, sizeof(std::uint32_t));
            std::memcpy(message.data() + sizeof(std::uint32_t), eventsList.data(), totalEvents * sizeof(Main::Structures::SingleMapEvent));
            response.setData(message.data(), message.size());

            session->asyncWrite(response);
        }
    }
}

#endif