#ifndef PACKET_FLOODING_CHECK
#define PACKET_FLOODING_CHECK

#include "../Interfaces.h"
#include <unordered_map>
#include <deque>
#include <format>
#include "../../Utils/Utils.h"

namespace Ac
{
    class PacketFloodChecker : public IACChecker<PacketFloodingEvent>
    {
    private:
        struct PacketRecord
        {
            std::uint64_t serverTime;
            PacketFloodingEvent event;
        };

        // [SEID] [ [PacketId][PreviousPackets] ]
        std::unordered_map<uint32_t, std::unordered_map<uint32_t, std::deque<PacketRecord>>> playerData;

        std::string floatToString(float value, int precision = 2)
        {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(precision) << value;
            return oss.str();
        }

    public:
        std::optional<ACFlag> processEvent(const PacketFloodingEvent& event) override
        {
            auto& packetRecords = playerData[event.session->getId()][event.packetId];
            packetRecords.push_back({ event.eventTime, event });

            while (!packetRecords.empty() &&
                (event.eventTime - packetRecords.front().serverTime) > event.analysisWindowMs)
            {
                packetRecords.pop_front();
            }

            if (packetRecords.size() > event.maxPacketsPerSecond)
            {
                const ACFlag flag{
                    event.session->getAccountId(),
                    event.floodingType,
                    "Packet flood (ID " + std::to_string(event.packetId) + "): " +
                    std::to_string(packetRecords.size()) + " packets in " +
                    floatToString((packetRecords.back().serverTime - packetRecords.front().serverTime) / 1000.0f) +
                    "s (max " + std::to_string(event.maxPacketsPerSecond) + ")"
                };
                packetRecords.clear();
                //event.session->closeSocket();

                return flag;
            }

            return std::nullopt;
        }

    };
}

#endif