#ifndef CLAN_MATCH_LEAVE_HANDLER_H
#define CLAN_MATCH_LEAVE_HANDLER_H

#include "../../Network/MainSession.h"
#include "Network/Packet.h"
#include "../../Classes/ClansManager.h"
#include "../../Structures/Clan/ClanStructures.h"
#include "../../Classes/Room.h"
#include "PartyJoinHandler.h"

#include <memory>

namespace Main
{
    namespace Handlers
    {
        enum ClanRoomLeaveExtra
        {
            CLANROOM_LEAVE_SUCCESS = 1,
            CLANROOM_LEAVE_UNK1 = 51,
            CLANROOM_LEAVE_NOT_LEADER = 16,
            CLANROOM_LEAVE_CLOSE = 27, // used to get back to the party waiting room
            CLANROOM_LEAVE_GENERAL_ERROR = 28
        };

        inline void handleClanRoomError(Main::Classes::ClansManager& clansManager, Main::Classes::RoomsManager& roomsManager,
            std::shared_ptr<Main::Network::Session> session, std::uint16_t roomNumber, std::uint16_t clanRoomNumber, std::uint32_t clanId, const std::string& errorMessage)
        {
            if (auto* selfClanRoom = clansManager.getExactRoomFor(clanId, clanRoomNumber))
            {
                selfClanRoom->broadcastChatMessage(errorMessage);
                clansManager.removeExactRoom(clanId, clanRoomNumber);
            }

            if (auto* room = roomsManager.getRoomByNumber(roomNumber))
            {
                room->broadcastMessage(errorMessage);

                auto sessions = room->getAllPlayersWithSessions();
                auto it = std::find_if(sessions.begin(), sessions.end(), [&](const auto& pair) {
                    return pair.second->getAccountInfo().clanId != session->getAccountInfo().clanId;
                    });

                if (it != sessions.end() && it->second)
                {
                    const std::uint16_t otherClanRoomNumber = it->second->getPlayer().getClanRoomNumber();
                    const std::uint32_t otherClanId = it->second->getAccountInfo().clanId;

                    if (auto otherPartyRoom = clansManager.getExactRoomFor(otherClanId, otherClanRoomNumber))
                    {
                        clansManager.removeExactRoom(otherClanId, otherClanRoomNumber);
                    }
                }

                roomsManager.removeRoom(roomNumber);
            }
        }

        inline void handlePartyRoomLeave(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session, Main::Classes::ClansManager& clansManager,
            Main::Classes::RoomsManager& roomsManager, bool isLeaderLeaving = false)
        {
            Common::Network::Packet response = request;
            response.setTcpHeader(request.getSession(), Common::Enums::NO_ENCRYPTION);
            response.setExtra(Main::Enums::PartyLeaveExtra::PARTY_LEAVE_SUCCESS);

            const auto& ainfo = session->getAccountInfo();
            std::uint16_t clanRoomNumber = session->getPlayer().getClanRoomNumber();
            const std::uint16_t roomNumber = session->getPlayer().getRoomNumber();

            if (auto* selfClanRoom = clansManager.getExactRoomFor(ainfo.clanId, clanRoomNumber))
            {
                auto* room = roomsManager.getRoomByNumber(roomNumber);
                auto targetPlayerIndex = selfClanRoom->getPlayerIndex(ainfo.uniqueId.session);
                if (std::optional<bool> res = selfClanRoom->removePlayer(ainfo.uniqueId.session); res)
                {
                    session->asyncWrite(response);

                    if (*res)
                    { // the party must be closed since the only player that was in it left
                        clansManager.removeExactRoom(ainfo.clanId, clanRoomNumber);
                        return;
                    }

                    if (selfClanRoom->isRegistered())
                    { // if in a clan vs clan room => it won't be marked as registered - no issues there
                        response.setCommand(120, 0, 45, 0);
                        selfClanRoom->broadcastToWaitingPlayers(response);
                        selfClanRoom->switchRegistered();
                    }

                    if (room)
                    { // clan vs clan room, notify all players about leaving
                        room->removePlayer(session, 27);
                    }
                    else if (!isLeaderLeaving)
                    {
                        // party room, notify other party members
                        response.setCommand(419, 0, 0, targetPlayerIndex.value_or(0));
                        response.setData(reinterpret_cast<const std::uint8_t*>(&ainfo.uniqueId), sizeof(ainfo.uniqueId));
                        selfClanRoom->broadcastToWaitingPlayersExceptSelf(response, ainfo.uniqueId.session);
                    }
                }
                else
                { // remove everyone to avoid further bugs
                    handleClanRoomError(clansManager, roomsManager, session, roomNumber, clanRoomNumber, ainfo.clanId,
                        "[handlePartyRoomLeave] An unexpected error occurred and the party was removed.");
                }
            }
            else
            {
                response.setExtra(Main::Enums::PartyLeaveExtra::PARTY_LEAVE_GENERAL_ERROR);
                response.setData(nullptr, 0);
                session->asyncWrite(response);
            }
        }

        // This is sent by the leader's client when they leave a clan room
        inline void handleClanRoomLeave(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
            Main::Classes::ClansManager& clansManager, Main::Classes::RoomsManager& roomsManager)
        {
            START_BENCHMARK

            Common::Network::Packet response = request;
            response.setTcpHeader(request.getSession(), Common::Enums::NO_ENCRYPTION);
            response.setExtra(ClanRoomLeaveExtra::CLANROOM_LEAVE_SUCCESS);
            response.setData(nullptr, 0);

            const std::uint16_t selfClanRoomNumber = session->getPlayer().getClanRoomNumber();
            const std::uint16_t selfRoomNumber = session->getPlayer().getRoomNumber();

            if (auto* clanRoom = clansManager.getExactRoomFor(session->getAccountInfo().clanId, selfClanRoomNumber))
            {
                if (!clanRoom->isLeader(session->getAccountInfo().uniqueId.session))
                { // only the host should've gotten this packet
                    session->sendMessage("Server error: the server received a host-only packet from a non-host client. Please report this issue");
                    response.setExtra(ClanRoomLeaveExtra::CLANROOM_LEAVE_NOT_LEADER);
                    session->asyncWrite(response);
                    return;
                }
            }
            else
            {
                session->sendMessage("Server error: failed to retrieve clan room info in HandleClanRoomLeave. Please report this issue");
                response.setExtra(ClanRoomLeaveExtra::CLANROOM_LEAVE_GENERAL_ERROR);
                session->asyncWrite(response);
                return;
            }

            if (auto* room = roomsManager.getRoomByNumber(selfRoomNumber))
            {
                std::unordered_map<uint32_t, std::uint16_t> clanRoomNumbers;
                std::unordered_map<uint32_t, std::vector<std::shared_ptr<Main::Network::Session>>> clanSessions;

                for (const auto& pair : room->getAllPlayersWithSessions())
                {
                    auto clanId = pair.second->getAccountInfo().clanId;
                    clanRoomNumbers[clanId] = pair.second->getPlayer().getClanRoomNumber();
                    clanSessions[clanId].push_back(pair.second);
                }

                roomsManager.removeRoom(selfRoomNumber, ClanRoomLeaveExtra::CLANROOM_LEAVE_CLOSE);

                for (const auto& [clanId, sessions] : clanSessions)
                {
                    if (auto* clanRoom = clansManager.getExactRoomFor(clanId, clanRoomNumbers[clanId]))
                    {
                        clanRoom->updatePartyStatus(false);
                        clanRoom->broadcastToWaitingPlayers(response);

                        for (auto& partySession : clanRoom->getWaitingPlayerSessions())
                        {
                            if (!clanRoom->isLeader(partySession->getSessionId()))
                            {
                                Common::Network::Packet req;
                                req.setCommand(111, 0, 0, 0);
                                handlePartyRoomLeave(req, partySession, clansManager, roomsManager, true);

                                Main::ClientData::ClanRoomInfo requestStructure;
                                requestStructure.clanId = clanId;
                                requestStructure.roomNumber = clanRoomNumbers[clanId];
                                req.setCommand(110, 0, 0, 0);
                                req.setData(reinterpret_cast<std::uint8_t*>(&requestStructure), sizeof(requestStructure));
                                handlePartyJoin(req, partySession, clansManager, roomsManager);
                            }
                        }
                    }
                }
            }
            else
            { // room not found
                session->sendMessage("Server error: failed to retrieve clan-match room information. Please report this issue");
                response.setExtra(ClanRoomLeaveExtra::CLANROOM_LEAVE_GENERAL_ERROR);
                session->asyncWrite(response);
            }

            END_BENCHMARK(handleClanRoomLeave, session)
        }
    }
}


#endif

