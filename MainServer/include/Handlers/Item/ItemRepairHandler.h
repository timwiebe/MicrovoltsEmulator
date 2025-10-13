#ifndef REPAIR_ITEM_HANDLER_H
#define REPAIR_ITEM_HANDLER_H

#include "../../Network/MainSession.h"
#include "../../../include/Structures/AccountInfo/MainAccountInfo.h"
#include "Network/Packet.h"
#include "../../Detail/Utilities.h"
#include <cstring> 

namespace Main
{
    namespace Handlers
    {
        inline std::vector<std::uint8_t> serializeItemRepair(const Main::ClientData::ItemRepair& itemRepair)
        {
            const std::size_t size = sizeof(itemRepair.newTotalRT) + sizeof(itemRepair.newTotalMP) + itemRepair.serialInfo.size() * sizeof(Main::Structures::ItemSerialInfo);
            std::vector<std::uint8_t> buffer(size);

            std::uint8_t* ptr = buffer.data();
            std::memcpy(ptr, &itemRepair.newTotalRT, sizeof(itemRepair.newTotalRT));
            ptr += sizeof(itemRepair.newTotalRT);
            std::memcpy(ptr, &itemRepair.newTotalMP, sizeof(itemRepair.newTotalMP));
            ptr += sizeof(itemRepair.newTotalMP);

            if (!itemRepair.serialInfo.empty())
            {
                std::memcpy(ptr, itemRepair.serialInfo.data(), itemRepair.serialInfo.size() * sizeof(Main::Structures::ItemSerialInfo));
            }

            return buffer;
        }

        inline void handleItemRepair(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session)
        {
            Common::Network::Packet response;
            response.setTcpHeader(session->getId(), Common::Enums::USER_ENCRYPTION);
            response.setCommand(request.getOrder(), request.getMission(), 1, 1);

            auto ainfo = session->getAccountInfo();

            Main::ClientData::ItemRepair itemRepair;
            itemRepair.newTotalRT = ainfo.rockTotens;
            itemRepair.newTotalMP = ainfo.microPoints;
            itemRepair.serialInfo.reserve(request.getOption() * sizeof(Main::Structures::ItemSerialInfo));

            // check whether enough MP
            std::uint32_t totalMpNeeded = 0;
            std::vector<Main::Structures::ItemSerialInfo> serialInfos;
            serialInfos.reserve(request.getOption() * sizeof(Main::Structures::ItemSerialInfo));

            for (std::size_t i = 0; i < request.getOption(); ++i)
            {
                auto serialInfo = Main::Details::parseData<Main::Structures::ItemSerialInfo>(request, i * sizeof(Main::Structures::ItemSerialInfo));
                serialInfos.push_back(serialInfo);

                if (auto idAndDurability = session->getPlayer().findItemIdAndDurabilityBySerialInfo(serialInfo))
                {
                    const auto baseItemDurability = Main::CdbUtils::getItemDurability(idAndDurability->first);
                    if (!baseItemDurability)
                    {
                        session->sendMessage("[Handlers::handleItemRepair] baseItemDurability was nullopt");
                        continue;
                    }

                    totalMpNeeded += *baseItemDurability - idAndDurability->second;
                }
                else
                {
                    session->sendMessage("[Handlers::handleItemRepair] idAndDurability was nullopt");
                }
            }

            if (ainfo.microPoints < totalMpNeeded)
            {
                itemRepair.serialInfo.clear();
                itemRepair.serialInfo.push_back(Main::Structures::ItemSerialInfo{});
                auto buffer = serializeItemRepair(itemRepair);
                response.setData(buffer.data(), static_cast<uint16_t>(buffer.size()));
                session->asyncWrite(response);
                return;
            }

            // enough MP => repair everything
            itemRepair.newTotalMP -= totalMpNeeded;
            itemRepair.serialInfo = std::move(serialInfos);

            for (const auto& serialInfo : itemRepair.serialInfo)
            {
                if (auto idAndDurability = session->getPlayer().findItemIdAndDurabilityBySerialInfo(serialInfo))
                {
                    const auto baseItemDurability = Main::CdbUtils::getItemDurability(idAndDurability->first);
                    if (baseItemDurability)
                    {
                        session->updateItemDurability(serialInfo.itemNumber, *baseItemDurability);
                    }
                }
            }

            session->setAccountMicroPoints(itemRepair.newTotalMP);
            auto buffer = serializeItemRepair(itemRepair);
            response.setData(buffer.data(), static_cast<uint16_t>(buffer.size()));
            response.setOption(itemRepair.serialInfo.size());
            session->asyncWrite(response);
        }
    }
}

#endif