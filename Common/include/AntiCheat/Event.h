#ifndef EVENT_AC_H
#define EVENT_AC_H

#include <string>
#include <memory>
#include "../Utils/Utils.h"
#include "../Network/Session.h"

namespace Ac
{
    struct ACEvent
    {
        virtual ~ACEvent() = default;

        enum class Type 
        { 
            PlayerPosition, 
            PlayerKill, 
            PacketFlooding,
            PacketReplication,
            ObserverCheck,
        } type;
    };

    struct PacketFloodingEvent : ACEvent
    {
        std::shared_ptr<Common::Network::Session> session;
        std::size_t maxPacketsPerSecond;
        std::uint64_t analysisWindowMs;
        std::string floodingType;
        std::uint32_t packetId;
        std::uint64_t eventTime;

        PacketFloodingEvent(std::shared_ptr<Common::Network::Session> session_, std::size_t maxPacketsPerS_,
            std::uint64_t analysisWindowMs_, const std::string& type_, std::uint32_t packetId_)
            : session(session_), maxPacketsPerSecond(maxPacketsPerS_), analysisWindowMs(analysisWindowMs_),
            floodingType(type_), packetId(packetId_),
            eventTime(Common::Utils::getCurrentTimestampMs()) 
        {
            type = Type::PacketFlooding;
        }

        static constexpr Type typeValue = Type::PacketFlooding;
    };

    template<typename SessionType>
    struct OutsideMatchActionEvent : ACEvent 
    {
        std::shared_ptr<SessionType> session;
        std::string message;

        OutsideMatchActionEvent(std::shared_ptr<SessionType> session_, const std::string& mss)
            : session(session_), message(mss)
        {
            type = Type::ObserverCheck;
        }

        static constexpr Type typeValue = Type::ObserverCheck;
    };


    struct PacketReplicationEvent : ACEvent
    {
        std::shared_ptr<Common::Network::Session> session;
        std::uint16_t packetId;
        std::vector<std::uint8_t> data;
        std::uint64_t eventTime;

        PacketReplicationEvent(std::shared_ptr<Common::Network::Session> session_, std::uint16_t packetId_,
            const std::vector<std::uint8_t>& data_)
            : session(session_), packetId(packetId_), data(data_),
            eventTime(Common::Utils::getCurrentTimestampMs()) 
        {
            type = Type::PacketReplication;
        }

        static constexpr Type typeValue = Type::PacketReplication;
    };

    struct ACFlag
    {
        std::uint32_t sessionId;
        std::string cheatType;
        std::string details;
    };
}

#endif
