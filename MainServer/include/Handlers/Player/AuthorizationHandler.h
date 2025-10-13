#ifndef AUTHORIZATION_HANDLER_H
#define AUTHORIZATION_HANDLER_H

#include "../../Network/MainSession.h"
#include "../../../include/Structures/AccountInfo/MainAccountInfo.h"
#include "Network/Packet.h"
#include "Utils/Constants.h"
#include <optional>
#include <source_location>
#include <string>

#ifdef _WIN32
    #include <winsock2.h>
    #include <WS2tcpip.h>
#else
    #include <arpa/inet.h>
#endif

#include "../../Detail/Utilities.h"
#include <Enums/PlayerEnums.h>

namespace Main
{
    namespace Handlers
    {
        inline std::string ipToString(std::uint32_t ip)
        {
            in_addr addr;
#ifdef _WIN32
            addr.S_un.S_addr = ip;
#else
            addr.s_addr = ip;
#endif
            char str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &addr, str, INET_ADDRSTRLEN);
            return std::string(str);
        }

        inline std::optional<Main::Structures::AccountInfo> handleAuthorization(
            const Common::Network::Packet& request,
            std::shared_ptr<Main::Network::Session> session,
            std::size_t totalOnlinePlayers,
            bool isServerOffline,
            Main::Persistence::MainScheduler& scheduler,
            bool isPublic)
        {
            START_BENCHMARK

            Common::Network::Packet response;
            response.setTcpHeader(request.getSession(), Common::Enums::NO_ENCRYPTION);
            response.setOrder(request.getOrder());
            response.setExtra(static_cast<std::uint8_t>(Main::Enums::AuthorizationExtra::SUCCESS));

            if (request.getDataSize() < sizeof(Main::ClientData::ClientAuthorization))
            {
                response.setExtra(static_cast<std::uint8_t>(Main::Enums::AuthorizationExtra::AUTHORIZATION_FAILED));
                session->asyncWrite(response);
                return std::nullopt;
            }

            const auto clientInfo = Main::Details::parseData<Main::ClientData::ClientAuthorization>(request);
            const auto& clientVersionRequired = Common::Utils::SetupParser::getInstance().getClientSetup();

            if (auto accountInfoOpt =
                    scheduler.immediatePersist(std::source_location::current(), &Main::Persistence::PersistentDatabase::getPlayerInfo, clientInfo.accountID); 
                accountInfoOpt)
            {
                const bool clientVersionMatches = clientInfo.clientVersion.matches(
                    clientVersionRequired.version1,
                    clientVersionRequired.version2,
                    clientVersionRequired.version3
                );
                const bool serverUnavailable = totalOnlinePlayers >= Common::Constants::maxServerCapacity || isServerOffline;

                if ((serverUnavailable || !clientVersionMatches || !isPublic) && accountInfoOpt->playerGrade < Common::Enums::PlayerGrade::GRADE_MOD)
                {
                    response.setExtra(static_cast<std::uint8_t>(Main::Enums::AuthorizationExtra::WRONG_CLIENT_VER_OR_SERVER_FULL_OR_OFFLINE));
                    session->asyncWrite(response);
                    END_BENCHMARK(handleAuthorization, session)
                    return std::nullopt;
                }

                else if (accountInfoOpt->accountID != clientInfo.accountID || clientInfo.accountHash != accountInfoOpt->accountKey)
                {
                    response.setExtra(static_cast<std::uint8_t>(Main::Enums::AuthorizationExtra::AUTHORIZATION_FAILED));
                    session->asyncWrite(response);
                    return std::nullopt;
                }

                auto hasBeenMatchBannedOpt = scheduler.immediatePersist(
                    std::source_location::current(),
                    &Main::Persistence::PersistentDatabase::hasBeenMatchBanned,
                    accountInfoOpt->accountID
                );

                if (hasBeenMatchBannedOpt == std::nullopt)
                {
                    session->closeSocket();
                    return std::nullopt;
                }

                session->setHasBeenMatchBanned(*hasBeenMatchBannedOpt);
                session->asyncWrite(response);
                session->sendMessage("Welcome! To see all commands, type /commands", Main::Enums::ChatExtra::INFO);
                session->sendMessage(
                    "Client Version: " +
                    std::to_string(clientInfo.clientVersion.ver2) + "." +
                    std::to_string(clientInfo.clientVersion.ver3) + "." +
                    std::to_string(clientInfo.clientVersion.ver4)
                );

                END_BENCHMARK(handleAuthorization, session)
                return *accountInfoOpt;
            }

            END_BENCHMARK(handleAuthorization, session)
            return std::nullopt;
        }
    }
}

#endif

