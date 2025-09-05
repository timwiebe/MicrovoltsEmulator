#ifndef ROOM_START_HANDLER_HEADER
#define ROOM_START_HANDLER_HEADER

#include "../../Network/MainSession.h"
#include "../../../include/Structures/AccountInfo/MainAccountInfo.h"
#include "Network/Packet.h"
#include "../../Classes/RoomsManager.h"
#include "../../Classes/ClanRoom.h"
#include "../../Classes/ClansManager.h"

namespace Main
{
	namespace Handlers
	{
		inline std::pair<Main::Classes::ClanRoom*, Main::Classes::ClanRoom*> getPartyRooms(Main::Classes::Room* room, Main::Classes::ClansManager& clansManager)
		{
			std::pair<Main::Classes::ClanRoom*, Main::Classes::ClanRoom*> ret{nullptr, nullptr};
			if (!room) return ret;

			auto sessions = room->getAllPlayersWithSessions();
			if (sessions.empty()) return ret;

			auto firstClanIt = std::ranges::find_if(sessions, [&](const auto& pair) {
				return clansManager.getExactRoomFor(pair.second->getAccountInfo().clanId, pair.second->getPlayer().getClanRoomNumber()) != nullptr;
				});

			if (firstClanIt == sessions.end()) return ret;

			ret.first = clansManager.getExactRoomFor(firstClanIt->second->getAccountInfo().clanId, firstClanIt->second->getPlayer().getClanRoomNumber());
			
			auto secondClanIt = std::ranges::find_if(sessions, [&](const auto& pair) {
				return pair.second->getAccountInfo().clanId != firstClanIt->second->getAccountInfo().clanId &&
					clansManager.getExactRoomFor(pair.second->getAccountInfo().clanId, pair.second->getPlayer().getClanRoomNumber()) != nullptr;
				});

			if (secondClanIt == sessions.end()) return ret;

			ret.second = clansManager.getExactRoomFor(secondClanIt->second->getAccountInfo().clanId, secondClanIt->second->getPlayer().getClanRoomNumber());
			return ret;
		}


		inline void handleRoomStart(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session, Main::Classes::RoomsManager& roomsManager,
			Main::Classes::ClansManager& clansManager, std::uint64_t timeSinceLastServerRestart)
		{
			if (request.getExtra() == 6)
			{ // Single wave
				session->setMatchStartTime();
				return;
			}
			else if (Main::Classes::Room* room = roomsManager.getRoomByNumber(session->getPlayer().getRoomNumber()))
			{
				if (session->hasBeenMatchBanned())
				{ 
					session->closeSocket();
					return;
				}

				Common::Network::Packet response;
				response.setTcpHeader(request.getSession(), Common::Enums::NO_ENCRYPTION);
				const Main::Structures::UniqueId selfUniqueId = session->getAccountInfo().uniqueId;

				if (request.getExtra() == 38) 
				{ // host or non-host clicks on "start" button 
					if (room->isCsdMode() && !session->hasCsdItems())
					{
						session->sendMessage("Error: This room requires CSD equipment.");
						return;
					}
					if (room->isHost(selfUniqueId))
					{
						if (room->isCsdMode() && !room->isEveryoneCsd()) return;
						room->generateMapIfRandom();

						if (!Main::Ipc::M2C_sendMapId(selfUniqueId.session, room->getActualMap(), room->getRoomSettings().mode))
						{
							session->sendMessage("[Main::HandleRoomStart] internal server error (IPC)!");
							return;
						}
						if (room->isAssassinMode())
						{
							auto blueAssassinOpt = room->getRandomAssassinFrom(Common::Enums::TEAM_BLUE, true);
							auto redAssassinOpt = room->getRandomAssassinFrom(Common::Enums::TEAM_RED, true);
							if (blueAssassinOpt && redAssassinOpt)
							{
								if (!Main::Ipc::M2C_sendAssassinModeInfo(true, selfUniqueId.session, blueAssassinOpt->first,
									blueAssassinOpt->second, redAssassinOpt->first, redAssassinOpt->second))
								{
									session->sendMessage("[Main::HandleRoomStart] Failed to send IPC data for Assassin mode! Please report this issue.");
									return;
								}
							}
							else
							{
								session->sendMessage("[Main::HandleRoomStart] Failed to get random blue or red assassin for Assassin Mode. Please report this issue");
								return;
							}
						}
						else
						{
							Main::Ipc::M2C_sendAssassinModeInfo(false, selfUniqueId.session);
						}
						if (room->getRoomNumber() >= Common::Constants::clanRoomNumberStart)
						{
							auto clanRooms = getPartyRooms(room, clansManager);

							if (!clanRooms.first && !clanRooms.second)
							{
								session->sendMessage("[Main::HandleRoomStart] error while retrieving either clan room!");
								return;
							}
							if (clanRooms.first)
							{
								clanRooms.first->updatePartyStatus(true);
							}
							if (clanRooms.second)
							{
								clanRooms.second->updatePartyStatus(true);
							}
						}
						if (room->getRoomSettings().mode == Common::Enums::BossBattle && !session->removeBossBattleTicket())
						{
							session->sendMessage("[Handlers::handleRoomStart] Could not find boss battle ticket - if this is an error, report it");
							return;
						}
						room->startMatch();
					}
					else if (room->hasMatchStarted())
					{
						session->m_totalBossBattleRespawnsLeft = 3;
						room->setStateFor(selfUniqueId, Common::Enums::PlayerState::STATE_NORMAL);

						Main::ClientData::PlayerTeamInfo info;
						info.uid = selfUniqueId;
						std::memcpy(info.nickname, session->getAccountInfo().nickname, 16);
						if (auto foundTeam = room->getTeamForSession(selfUniqueId.session))
						{
							info.team = *foundTeam;
						}
						else
						{
							Utils::Logger::log("[Handlers::StartMatch] Failed to retrieve player team for IPC", Utils::LogType::Error);
							return;
						}
						if (!Main::Ipc::M2C_sendPlayerTeamInfoBatch(session->getId(), { info }))
						{
							Utils::Logger::log("[Handlers::StartMatch] Failed to send single player team info to Cast Server", Utils::LogType::Error);
						}
					}

					session->setMatchStartTime();
					response.setCommand(request.getOrder(), 0, 38, room->getActualMap());
					response.setData(reinterpret_cast<const std::uint8_t*>(&selfUniqueId), sizeof(selfUniqueId));
					room->broadcastToRoom(response);
				}
				else if (request.getExtra() == 41)
				{
					if (room->isHost(selfUniqueId)) 
					{ // broadcast the tick to the room
						const std::uint64_t roomTick = Details::getUtcTimeMs() - timeSinceLastServerRestart;
						response.setCommand(258, 0, 1, 0);  // What's extra 5 here?
						response.setData(reinterpret_cast<const std::uint8_t*>(&roomTick), sizeof(roomTick));
						room->broadcastToRoom(response);
					}
					else 
					{ // Tell the other players in the match that we joined
						response.setCommand(415, 0, 1, 0);
						response.setData(reinterpret_cast<const std::uint8_t*>(&selfUniqueId), sizeof(selfUniqueId));
						if (!session->isInvisible())
						{
							room->broadcastToRoom(response);
						}
						else
						{
							session->asyncWrite(response);
						}
					}
					
				}
			}
		}

		inline bool handleRoomStartInvisible(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
			Main::Classes::RoomsManager& roomsManager)
		{
			if (Main::Classes::Room* room = roomsManager.getRoomByNumber(session->getPlayer().getRoomNumber()))
			{
				Common::Network::Packet response;
				response.setTcpHeader(request.getSession(), Common::Enums::NO_ENCRYPTION);
				const Main::Structures::UniqueId selfUniqueId = session->getAccountInfo().uniqueId;

				if (request.getExtra() == 38)
				{ // host or non-host clicks on "start" button (n.b: SingleWave's extra is 6)
					if (room->isHost(selfUniqueId))
					{
						session->sendMessage("Error: use this command when you are not the host");
						return false;
					}
					else if (room->hasMatchStarted())
					{
						room->setStateFor(selfUniqueId, Common::Enums::PlayerState::STATE_NORMAL);
					}

					session->setMatchStartTime();
					response.setCommand(request.getOrder(), 0, 38, room->getActualMap());
					response.setData(reinterpret_cast<const std::uint8_t*>(&selfUniqueId), sizeof(selfUniqueId));
					room->broadcastToRoom(response);
				}
				return true;
			}
			session->sendMessage("Error: not in a room");
			return false;
		}
	}
}

#endif
