#ifndef ELIMINATION_NEXT_ROUND_HANDLER_H
#define ELIMINATION_NEXT_ROUND_HANDLER_H

#include <algorithm>
#include "../../Network/MainSession.h"
#include "../../../include/Structures/AccountInfo/MainAccountInfo.h"
#include "Network/Packet.h"
#include "../../Classes/RoomsManager.h"
#include "../../Structures/EndScoreboard.h"
#include "RoomStartHandler.h"
#include "../../Classes/ClansManager.h"

namespace Main
{
	namespace Handlers
	{
		inline void unknown(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session, Main::Classes::RoomsManager& roomsManager)
		{
			if (auto* room = roomsManager.getRoomByNumber(session->getPlayer().getRoomNumber()))
			{
				room->broadcastToRoom(const_cast<Common::Network::Packet&>(request));
			}
		}

		inline void handleEliminationNextRound(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
			Main::Classes::RoomsManager& roomsManager)
		{
			if (auto* room = roomsManager.getRoomByNumber(session->getPlayer().getRoomNumber()))
			{
				if (!room->isHost(session->getAccountInfo().uniqueId)) return;
				room->broadcastToRoomExceptSelf(const_cast<Common::Network::Packet&>(request), session->getAccountInfo().uniqueId);

				if (room->isAssassinMode())
				{
					auto blueAssassinOpt = room->getRandomAssassinFrom(Common::Enums::TEAM_BLUE, false);
					auto redAssassinOpt = room->getRandomAssassinFrom(Common::Enums::TEAM_RED, false);
					if (blueAssassinOpt && redAssassinOpt)
					{
						if (!Main::Ipc::M2C_sendAssassinModeInfo(true, session->getAccountInfo().uniqueId.session, blueAssassinOpt->first, 
							blueAssassinOpt->second, redAssassinOpt->first, redAssassinOpt->second))
						{
							room->broadcastMessage("[Main::handleEliminationNextRound] Failed to send IPC data for Assassin mode! Please report this issue.");
						}
					}
					else
					{
						room->broadcastMessage(
							"[Main::handleEliminationNextRound] Failed to get random blue or red assassin for Assassin Mode. Please report this issue");
					}
				}
			}
		}

		inline void handleEliminationNextRound2(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
			Main::Classes::RoomsManager& roomsManager)
		{
			if (auto* room = roomsManager.getRoomByNumber(session->getPlayer().getRoomNumber()))
			{
				if (!room->isHost(session->getAccountInfo().uniqueId)) return;
				Common::Network::Packet response = request;
				response.setExtra(1);
				room->broadcastToRoomExceptSelf(response, session->getAccountInfo().uniqueId);
			}
		}

		inline void handleMatchEnding(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
			Main::Classes::RoomsManager& roomsManager,
			Main::Classes::ClansManager& clansManager, Main::Persistence::MainScheduler& scheduler, const Main::Structures::ExpMpBonusInfo& expMpBonusInfo,
			const Main::Structures::EventMissionInfo& eventMissionInfo)
		{
			namespace MS = Main::Structures;
			namespace MC = Main::ClientData;
			namespace CD = Common::ConstantDatabase;
			Common::Network::Packet response = request;

			if (request.getExtra() == 6)
			{
				auto now = Main::Details::getUtcTimeMs();
				const Main::ClientData::SinglewaveEndRequest req = Main::Details::parseData<Main::ClientData::SinglewaveEndRequest>(request);
				if (req.type == 1 || req.type == 2) //  single wave
				{
					response.setCommand(66, 0, 17, 2); // single wave ack

					if (session->getPlayer().hasEnoughInventorySpace(1))
					{
						constexpr std::uint64_t fiveMinsMs = 5 * 60 * 1000;
						constexpr std::uint64_t tenMinsMs = 10 * 60 * 1000;

						if (req.type == 1 && req.stage == 10 && (now - session->m_matchStartTime >= fiveMinsMs))
						{ 
							Main::Structures::BoughtItem reward{ Common::Constants::singlewaveEasyBox }; 
							reward.serialInfo.itemNumber = session->getPlayer().getLatestItemNumber() + 1;
							session->setLatestItemNumber(reward.serialInfo.itemNumber);
							response.setData(reinterpret_cast<std::uint8_t*>(&reward), sizeof(reward));
							session->addItem(Main::Structures::Item{ reward });
							session->m_matchStartTime = now;
						}
						else if (req.type == 2)
						{
							if (req.stage == 20 && (now - session->m_matchStartTime >= tenMinsMs))
							{
								Main::Structures::BoughtItem reward{ Common::Constants::singlewaveHardBox };
								reward.serialInfo.itemNumber = session->getPlayer().getLatestItemNumber() + 1;
								session->setLatestItemNumber(reward.serialInfo.itemNumber);
								response.setData(reinterpret_cast<std::uint8_t*>(&reward), sizeof(reward));
								session->addItem(Main::Structures::Item{ reward });
								session->m_matchStartTime = now;
							}
							session->updateSingleWaveScore(req.score, req.stage);
						}
					}
					session->asyncWrite(response);
				}
				else 
				{
					session->completeTutorial();
				}
			}
			else if (Main::Classes::Room* room = roomsManager.getRoomByNumber(session->getPlayer().getRoomNumber()))
			{
				if (!room->isHost(session->getAccountInfo().uniqueId)) return; // only the host should send this packet, prevent lvl up exploits
				MC::ClientEndingMatchHeader endMatchHeader = Main::Details::parseData<MC::ClientEndingMatchHeader>(request);

				if (room->getRoomNumber() >= Common::Constants::clanRoomNumberStart)
				{
					auto clanRooms = getPartyRooms(room, clansManager);
					bool updatedAtleastOne = false;
					if (clanRooms.first)
					{
						clanRooms.first->updatePartyStatus(false);
						clanRooms.first->storeStats(scheduler, endMatchHeader);
						updatedAtleastOne = true;
					}
					if (clanRooms.second)
					{
						clanRooms.second->updatePartyStatus(false);
						clanRooms.first->storeStats(scheduler, endMatchHeader);
						updatedAtleastOne = true;
					}
					if (!updatedAtleastOne)
					{
						session->sendMessage("[Main::Handlers::handleMatchEnding] error while retrieving either clan room!");
					}
				}
				room->endMatch();


				bool isFarm = false;
				const std::uint64_t roomStartTime = room->getMatchStartTime();
				const std::uint64_t timeNow = Main::Details::getUtcTimeMs();
				if ((timeNow > roomStartTime && ((timeNow - roomStartTime) < 120 * 1000)) || room->getRoomSettings().mode == Common::Enums::SquareMode
					|| room->getRoomSettings().mode == Common::Enums::AiBattle)
				{ // prevent farming
					isFarm = true;
				}

				// client sends all info of all players, we resend it back to everyone (else the other clients outside the match will see the target still inside the match)
				room->broadcastToRoomExceptSelf(response, session->getAccountInfo().uniqueId);

				for (std::size_t i = 0; i < request.getOption(); ++i)
				{
					auto clientScore = Main::Details::parseData<MS::ClientEndingMatch>(request, sizeof(MS::ClientEndingMatch) * i + sizeof(endMatchHeader));
					Main::Structures::ScoreboardResponse scoreboardResponse(clientScore);

					auto targetSession = room->getPlayer(clientScore.uniqueId);
					if (!targetSession) continue;

					const Main::Structures::AccountInfo ainfo = targetSession->getAccountInfo();
					if (room->getRoomNumber() >= Common::Constants::clanRoomNumberStart)
					{
						const auto totalNewContribution = Common::Constants::clanBaseContribution + scoreboardResponse.totalKills * 6;
						scoreboardResponse.newTotalClanContribution = ainfo.clanContribution +
							(totalNewContribution <= Common::Constants::maxExpAndMpPerMatch ? totalNewContribution : Common::Constants::maxExpAndMpPerMatch);
					}

					std::uint32_t totalExpBonus = 0;
					std::uint32_t totalMpBonus = 0;
					const auto& equippedItems = targetSession->getPlayer().getEquippedItemsFor(targetSession->getAccountInfo().latestSelectedCharacter);
					for (const auto& currentItem : equippedItems)
					{
						if (currentItem.serialInfo.itemNumber == 0) continue;
						const auto [expBonus, mpBonus] = Main::CdbUtils::getExpAndMpEnhancementFor(currentItem.id);
						totalExpBonus += expBonus;
						totalMpBonus += mpBonus;
					}

					const auto gainedMp = isFarm ? 0 : ((scoreboardResponse.totalKills * 60 + scoreboardResponse.deaths * 25 + Common::Constants::matchBaseMp) * 3);
					const auto gainedExp = isFarm ? 0 : ((scoreboardResponse.totalKills * 50 + scoreboardResponse.deaths * 25 + Common::Constants::matchBaseExp) * 3);

					const auto finalGainedExp = gainedExp + (gainedExp * totalExpBonus / 100);
					const auto finalGainedMp = gainedMp + (gainedMp * totalMpBonus / 100);

					std::uint32_t now = static_cast<std::uint32_t>(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
					std::uint32_t finalGainedExpWithEvent = finalGainedExp;
					std::uint32_t finalGainedMpWithEvent = finalGainedMp;

					if (now <= expMpBonusInfo.endDate)
					{
						finalGainedExpWithEvent += (finalGainedExp * expMpBonusInfo.expBonusPercent / 100);
						finalGainedMpWithEvent += (finalGainedMp * expMpBonusInfo.mpBonusPercent / 100);
					}

					const auto clampedExp = (finalGainedExpWithEvent <= Common::Constants::maxExpAndMpPerMatch) ? finalGainedExpWithEvent 
						: Common::Constants::maxExpAndMpPerMatch;
					const auto clampedMp = (finalGainedMpWithEvent <= Common::Constants::maxExpAndMpPerMatch) ? finalGainedMpWithEvent 
						: Common::Constants::maxExpAndMpPerMatch;
					if (!isFarm && (clampedExp + ainfo.experience) < ainfo.experience)
					{
						targetSession->sendMessage("[Handlers::handleEliminationNextRound] error: negative experience detected");
						scoreboardResponse.newTotalEXP = ainfo.experience + 200;
					}
					else
					{
						scoreboardResponse.newTotalEXP = ainfo.experience + clampedExp;
					}
					if (!isFarm && (clampedMp + ainfo.microPoints) < ainfo.microPoints)
					{
						targetSession->sendMessage("[Handlers::handleEliminationNextRound] error: negative MP detected");
						scoreboardResponse.newTotalMP = ainfo.microPoints + 200;
					}
					else
					{
						scoreboardResponse.newTotalMP = ainfo.microPoints + clampedMp;
					}

					if (auto* gradeInfo = CD::CdbSingleton<CD::CdbGradeInfo>::getInstance().getEntry(ainfo.playerLevel + 1);
						!isFarm && gradeInfo && scoreboardResponse.newTotalEXP >= gradeInfo->gi_exp)
					{ // the player leveled up
						targetSession->sendMessage("You obtained a reward box!", Main::Enums::TIP);

						const std::uint32_t actualPlayerLevel = ainfo.playerLevel;
						const std::uint64_t newPlayerLevel = actualPlayerLevel + 1;

						response.setCommand(311, 0, 1, newPlayerLevel);
						response.setData(reinterpret_cast<std::uint8_t*>(&clientScore.uniqueId), sizeof(clientScore.uniqueId));
						room->broadcastToRoomExceptSelf(response, clientScore.uniqueId);

						targetSession->spawnItemCommand((actualPlayerLevel % 10 == 0) ? Common::Constants::goldLevelBox
							: (actualPlayerLevel % 5 == 0) ? Common::Constants::silverLevelBox : Common::Constants::bronzeLevelBox, 
							"Item spawned automatically (Level-up reward item)");

						scoreboardResponse.newTotalMP += gradeInfo->gi_reward_point;
						room->storeEndMatchStatsFor(clientScore.uniqueId, scoreboardResponse, endMatchHeader.blueScore, endMatchHeader.redScore, true,
							eventMissionInfo);

						if (actualPlayerLevel >= 5 && actualPlayerLevel % 5 == 0)
						{ // RT & coupon reward
							const std::uint32_t rtToAdd = 5000 * (actualPlayerLevel / 5);
							targetSession->sendRt(rtToAdd);
							targetSession->spawnCouponImmediate(10);
							targetSession->sendMessage("You obtained " + std::to_string(rtToAdd) + " RockTokens and 10 coupons!");
						}
					}
					else
					{
						if (!isFarm)
						{
							room->storeEndMatchStatsFor(clientScore.uniqueId, scoreboardResponse, endMatchHeader.blueScore, endMatchHeader.redScore, false,
								eventMissionInfo);
						}
					}
					response.setCommand(request.getOrder(), 3, isFarm ? 6 : 1, 0);
					response.setData(reinterpret_cast<std::uint8_t*>(&scoreboardResponse), sizeof(scoreboardResponse));
					targetSession->asyncWrite(response);
					if (!isFarm)
					{
						targetSession->sendRt(static_cast<std::uint32_t>(static_cast<double>(clampedMp)/3));
						if (room->getRoomSettings().mode != Common::Enums::SquareMode && room->getRoomSettings().mode != Common::Enums::AiBattle)
						{
							targetSession->reduceEquippedItemsDurability();
						}
					}
				}
			}
		}
	}
}

#endif
