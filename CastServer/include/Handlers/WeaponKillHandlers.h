#ifndef WEAPON_KILL_HANDLERS
#define WEAPON_KILL_HANDLERS

#include <Network/Packet.h>
#include <memory>
#include "../Utils/Utilities.h"
#include "../../../MainServer/include/Structures/AccountInfo/MainAccountUniqueId.h"
#include "../Structures/PlayerPositionFromClient.h"
#include <thread>
#include "../Network/CastSession.h"
#include "../Network/SessionsManager.h"
#include "../Structures/PlayerPositionFromServer.h"
#include "../Structures/SuicideStruct.h"
#include "SimpleHandlers.h"
#include <Utils/Utils.h>
#include <AntiCheat/AntiCheat.h>

namespace Cast
{
	namespace Handlers
	{
		inline bool handleAssassinMode(Cast::Classes::RoomsManager& roomsManager, std::shared_ptr<Cast::Classes::Room> room, const Main::Structures::UniqueId& attackerUid, 
			const Main::Structures::UniqueId& targetUid,
			std::shared_ptr<Cast::Network::Session> session, std::shared_ptr<Cast::Network::Session> targetSession)
		{
			if (room->m_assassinBlueUid == targetUid || room->m_assassinRedUid == targetUid)
			{
				room->killTeam(room->m_assassinBlueUid == targetUid ? Common::Enums::TEAM_BLUE : Common::Enums::TEAM_RED);
				room->broadcastMessage("The assassin of team " + 
					(room->m_assassinBlueUid == targetUid ? std::string("BLUE") : std::string("RED")) + " was killed by " + session->m_nickname + "!");
				return true;
			}
			else
			{
				if (attackerUid == room->m_assassinBlueUid || attackerUid == room->m_assassinRedUid)
				{ // if target killed by opposite assassin => return early
					targetSession->sendMessage("You were killed by the assassin! Wait until next round.");
					return true;
				}
				else
				{ // if target killed by normal opponent => respawn immediately
					Common::Network::UnecryptedPacket response;
					response.setTcpHeader(session->getId());
					response.setOrder(276);
					Cast::Structures::PlayerRespawnPosition position;
					position.targetUniqueId = targetUid;
					if (targetSession->m_team == Common::Enums::TEAM_RED || targetSession->m_team == Common::Enums::TEAM_BLUE)
					{
						if (auto posOpt = roomsManager.getPositionFor(session->getId(), targetSession->m_team))
						{
							position = *posOpt;
						}
						else return true;
					}
					else return true;

					response.setData(reinterpret_cast<std::uint8_t*>(&position), sizeof(position));
					roomsManager.broadcastToMatch(session->getId(), response); 
					return false;
				}
			}
		}

		inline void handleArenaMode(Cast::Classes::RoomsManager& roomsManager, std::shared_ptr<Cast::Classes::Room> room)
		{
			room->m_hasMatchStarted = true;

			auto totalAlive = room->getTotalAlivePlayers();
			if (totalAlive <= 1 && !room->m_arenaRoundFinished)
			{
				room->m_arenaRoundFinished = true;
				room->broadcastMessage("[ROOM: " + std::to_string(room->getRoomNumber()) + "] Arena end. Wait 10 seconds...");
				std::thread([room]() {
					room->shuffleCoordinates();
					std::this_thread::sleep_for(std::chrono::seconds(10));
					room->respawnEveryoneArena();
					}).detach();
			}
		}

		inline void handleNormalWeaponDamage(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session, 
			Cast::Classes::RoomsManager& roomsManager, Cast::Network::SessionsManager& sessionsManager, Ac::AntiCheatManager& acManager)
		{
			auto roomOpt = roomsManager.getRoom(session->getId());
			if (!roomOpt) return;
			auto& room = *roomOpt;

			const auto attackerUid = Cast::Details::parseData<Main::Structures::UniqueId>(request, 16);
			const auto targetUid = Cast::Details::parseData<Main::Structures::UniqueId>(request, 20);
			const std::uint16_t targetHp = Cast::Details::parseData<std::uint16_t>(request, 24);

			if (room->getMode() == Common::Enums::AiBattle || room->getMode() == Common::Enums::BossBattle)
			{
				roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
				return;
			}

			auto attackerSession = sessionsManager.getSession(attackerUid.session);
			if (attackerSession &&
				(attackerSession->m_team == Common::Enums::TEAM_OBSERVER || !attackerSession->m_isInMatch))
			{
				return;
			}

			if (auto targetSession = sessionsManager.getSession(targetUid.session))
			{
				if (targetHp)
				{
					if (!targetSession->isDead)
						roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
				}
				else
				{
					if (room->m_isAssassinMode)
					{
						if (!handleAssassinMode(roomsManager, room, attackerUid, targetUid, session, targetSession)) return;
						else roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
					}
					else
					{
						if (!targetSession->isDead) roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
						targetSession->isDead = true;

						if (room->isArenaMode())
						{
							handleArenaMode(roomsManager, room);
							return;
						}
						if (Cast::Details::mustBroadcastDeath(roomsManager.getModeOf(session->getId())))
						{
							Cast::Handlers::sendPlayerStateUpdate(targetSession->getAccountId(), true);
						}
						if (attackerSession)
						{
							acManager.submitEvent(std::make_unique<Ac::PacketFloodingEvent>(attackerSession, 4, 1000, "Room Rape (flooding)", 265));
						}
					}
				}
			}
		}

		// mg & shotgun
		// issue: in boss battle, mg/shotgun work for NPCs, but they don't disappear when killed with mg/shotgun
		inline void handleSpecialWeaponDamage(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session, 
			Cast::Classes::RoomsManager& roomsManager,
			Cast::Network::SessionsManager& sessionsManager,
			Ac::AntiCheatManager& acManager)
		{
			auto roomOpt = roomsManager.getRoom(session->getId());
			if (!roomOpt) return;
			auto& room = *roomOpt;

			std::uint16_t targetHp = Cast::Details::parseDataFromEnd<std::uint16_t>(request, 6);
			auto targetUid = Cast::Details::parseDataFromEnd<Main::Structures::UniqueId>(request, 8);
			auto attackerUid = Cast::Details::parseData<Main::Structures::UniqueId>(request, 16);

			if (room->getMode() == Common::Enums::AiBattle || room->getMode() == Common::Enums::BossBattle)
			{
				roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
				return;
			}

			auto attackerSession = sessionsManager.getSession(attackerUid.session);
			if (attackerSession &&
				(attackerSession->m_team == Common::Enums::TEAM_OBSERVER || !attackerSession->m_isInMatch))
			{
				return;
			}

			if (auto targetSession = sessionsManager.getSession(targetUid.session))
			{
				if (targetHp)
				{
					if (!targetSession->isDead)
						roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
				}
				else
				{
					if (room->m_isAssassinMode)
					{
						if (!handleAssassinMode(roomsManager, room, attackerUid, targetUid, session, targetSession)) return;
						else roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
					}
					else
					{
						if (!targetSession->isDead) roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
						targetSession->isDead = true;
						if (room->isArenaMode())
						{
							handleArenaMode(roomsManager, room);
							return;
						}
						if (Cast::Details::mustBroadcastDeath(roomsManager.getModeOf(session->getId())))
						{
							Cast::Handlers::sendPlayerStateUpdate(targetSession->getAccountId(), true);
						}
						if (attackerSession)
						{
							acManager.submitEvent(std::make_unique<Ac::PacketFloodingEvent>(attackerSession, 4, 1000, "Room Rape (flooding)", 265));
						}
					}
				}
			}
		}

		inline void handleExplosiveDamage(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session, 
			Cast::Classes::RoomsManager& roomsManager,
			Cast::Network::SessionsManager& sessionsManager)
		{
			auto roomOpt = roomsManager.getRoom(session->getId());
			if (!roomOpt) return;
			auto& room = *roomOpt;

		
			if (request.getOption() == 0 || room->getMode() == Common::Enums::AiBattle || room->getMode() == Common::Enums::BossBattle)
			{
				roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
			}

			for (std::uint32_t i = 0; i < request.getOption(); ++i)
			{
				const auto targetUid = Cast::Details::parseData<Main::Structures::UniqueId>(request, 12 * (i + 1));
				const auto targetNewHp = Cast::Details::parseData<std::uint16_t>(request, 12 * (i + 1) + sizeof(Main::Structures::UniqueId));
				if (auto targetSession = sessionsManager.getSession(targetUid.session))
				{
					if (targetNewHp)
					{
						if (!targetSession->isDead)
							roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
					}
					else
					{
						if (room->m_isAssassinMode)
						{
							if (!handleAssassinMode(roomsManager, room, Main::Structures::UniqueId{ 0, 0, 1 } /*on purpose*/, targetUid, session, targetSession))
								return;
							else roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
						}
						else
						{
							if (!targetSession->isDead) roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
							targetSession->isDead = true;

							if (room->isArenaMode())
							{
								handleArenaMode(roomsManager, room);
								return;
							}
							if (Cast::Details::mustBroadcastDeath(roomsManager.getModeOf(session->getId())))
							{
								Cast::Handlers::sendPlayerStateUpdate(targetSession->getAccountId(), true);
							}
						}
					}
				}
			}
		}
	}
}

#endif

