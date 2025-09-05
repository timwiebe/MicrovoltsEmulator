#ifndef MATCH_LEAVE_HANDLER_H
#define MATCH_LEAVE_HANDLER_H

#include "../../Network/MainSession.h"
#include "../../Classes/RoomsManager.h"
#include "Network/Packet.h"
#include "../../Network/MainSessionManager.h"
#include "../Clan/ClanRoomLeaveHandler.h"

namespace Main
{
	namespace Handlers
	{	
        inline void handleMatchLeave(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session, 
            Main::Network::SessionsManager& sessionsManager,
            Main::Classes::RoomsManager& roomsManager, Main::Classes::ClansManager& clansManager)
        {
            if (Main::Classes::Room* room = roomsManager.getRoomByNumber(session->getPlayer().getRoomNumber()))
            {
                const std::uint16_t clanRoomNum = session->getPlayer().getClanRoomNumber();
                auto clanRoom = clansManager.getExactRoomFor(session->getAccountInfo().clanId, clanRoomNum);

                const std::uint32_t selfRoomNumber = room->getRoomNumber();
                const auto ainfo = session->getAccountInfo();

                auto removePlayerFromParty = [&](Main::Classes::Room* room, auto& clanRoom, const std::uint16_t clanRoomNum, 
                    const std::uint32_t selfRoomNumber, bool isHost) {
                    if (clanRoom)
                    {
                        if (isHost)
                        { // only the host leaves both match AND room, in which case we must check if they were without other clan members (alone)
                            if (auto mustBeClosed = clanRoom->removePlayer(session->getAccountInfo().uniqueId.session))
                            {
                                if (*mustBeClosed) 
                                    clansManager.removeExactRoom(session->getAccountInfo().clanId, clanRoomNum);
                            }
                            else
                            {
                                handleClanRoomError(clansManager, roomsManager, session, selfRoomNumber, clanRoomNum, ainfo.clanId,
                                    "[handleMatchLeave] An unexpected error occurred and the party was removed.");
                                return false;
                            }
                        }
                        else
                        {
                            clanRoom->changeLeaderIfLeader(session->getAccountInfo().uniqueId.session);
                        }
                    }
                    return true;
                    };

                
                if (room->getTargetVotekickUid() == ainfo.uniqueId)
                { // The target votekicked player is leaving while a votekick is going on
                    const std::string& targetNickname = room->getAccountInfoFor(ainfo.uniqueId).nickname;
                    room->votekickPlayer(ainfo.uniqueId);
                    room->resetVotekick();
                    room->broadcastMessage("[" + targetNickname + "] was kicked after attempting to leave the room during a votekick.");
                }
                else
                {
                    if (request.getExtra() == 28)
                    { // no penalty item used
                        const Main::Structures::ItemSerialInfo itemSerialInfo = 
                            Main::Details::parseData<Main::Structures::ItemSerialInfo>(request, request.getDataSize() - sizeof(Main::Structures::ItemSerialInfo));
                       session->useNoPenalty(itemSerialInfo, request);
                    }
                    else if (room->getTeamForSession(session->getId()).value_or(0) != Common::Enums::TEAM_OBSERVER)
                    { // remove penalty mp
                        const auto currentMp = session->getPlayer().getAccountInfo().microPoints;
                        session->setAccountMicroPoints(currentMp <= 120 ? 0 : currentMp - 120);
                        session->sendCurrency();
                    }
                    if (room->isHost(ainfo.uniqueId))
                    {
                        if (!removePlayerFromParty(room, clanRoom, clanRoomNum, selfRoomNumber, true))
                            return;

                        if (room->removeHostFromMatch())
                        {
                            roomsManager.removeRoom(selfRoomNumber);
                        }
                    }
                    else
                    {
                        if (!removePlayerFromParty(room, clanRoom, clanRoomNum, selfRoomNumber, false))
                            return;

                        const auto uniqueId = ainfo.uniqueId;
                        Common::Network::Packet response;
                        response.setTcpHeader(request.getSession(), Common::Enums::NO_ENCRYPTION);
                        response.setCommand(request.getOrder(), 0, 0, 0);
                        response.setData(reinterpret_cast<const std::uint8_t*>(&uniqueId), sizeof(uniqueId));
                        room->broadcastToRoom(response);
                        room->setStateFor(uniqueId, Common::Enums::STATE_WAITING);
                    }
                }
            }
		}
	}
}

#endif
