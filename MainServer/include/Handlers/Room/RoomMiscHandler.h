#ifndef ROOM_MISC_HANDLER_HEADER
#define ROOM_MISC_HANDLER_HEADER

#include "../../Network/MainSession.h"
#include "../../../include/Structures/AccountInfo/MainAccountInfo.h"
#include "../../../include/Structures/Room/RoomSettingsUpdate.h"
#include "Network/Packet.h"
#include "../Room/RoomJoinHandler.h"
#include "../Room/RoomLeaveHandler.h"
#include <cstring> 

namespace Main
{
	namespace Handlers
	{
		enum VotekickExtras
		{
			VOTEKICK_ERROR_LIMIT_EXCEEDED = 8,
			VOTEKICK_ERROR_TARGET_LEFT = 13,
		};

		// Takes care of settings that are inside the "Room Settings" button + switching team
		inline void handleRoomMiscellaneous(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session, 
			Main::Classes::RoomsManager& roomsManager,
			std::uint64_t m_latestServerRestart)
		{
			if (Main::Classes::Room* room = roomsManager.getRoomByNumber(session->getPlayer().getRoomNumber()))
			{
				if (room->getRoomSettings().mode == Common::Enums::AiBattle)
				{
					session->sendMessage("Cannot change settings in AI battle, please create a new room");
					return;
				}
				auto response = request;

				if (request.getOrder() == 125 && request.getExtra() == 28) 
				{ // Votekick start
					struct VotekickResponse
					{
						Main::Structures::UniqueId targetUniqueId{};
						Main::Structures::UniqueId selfUniqueId{};
						std::uint32_t kickReasonId = 0;
						std::uint32_t totalVoteTime = 0; // here the tick is in ms / 10 since 4 bytes might not be enough for tick in pure milliseconds!
					} votekickResponse;

					// targetUniqueId and kickReason is provided by the client
					votekickResponse.targetUniqueId = Main::Details::parseData<Main::Structures::UniqueId>(request);
					votekickResponse.kickReasonId = Main::Details::parseData<std::uint32_t>(request, sizeof(Main::Structures::UniqueId));

					if (auto targetSession = room->getPlayer(votekickResponse.targetUniqueId))
					{
						if (targetSession->getAccountInfo().playerGrade >= Common::Enums::GRADE_ES)
						{
							session->sendMessage("Cannot votekick a staff member!");
						}
						else if (!session->getPlayer().isVotekickEnabled())
						{
							session->sendMessage("Your votekicking permissions are currently disabled.");
							return;
						}
						else if (session->getAccountInfo().microPoints < 100)
						{
							session->sendMessage("Not enough micro points for a votekick!");
						}
						else
						{
							const auto& accountInfo = session->getAccountInfo();
							if (!room->startVotekick(votekickResponse.targetUniqueId, targetSession->getAccountInfo().nickname, votekickResponse.kickReasonId,
								session->getAccountInfo().accountID))
							{ // the votekick starter has already used their votekick for this match
								response.setExtra(VotekickExtras::VOTEKICK_ERROR_LIMIT_EXCEEDED);
								response.setData(nullptr, 0);
								session->asyncWrite(response);
							}
							else
							{
								const std::uint32_t totalVoteTime = 30'000; // in ms
								votekickResponse.selfUniqueId = accountInfo.uniqueId;
								votekickResponse.totalVoteTime = (static_cast<std::uint32_t>(accountInfo.getUtcTimeMs() - m_latestServerRestart + totalVoteTime) / 10);

								response.setCommand(request.getOrder(), 0, 28, 1);
								response.setData(reinterpret_cast<std::uint8_t*>(&votekickResponse), sizeof(votekickResponse));
								room->broadcastToRoom(response);
								session->setAccountMicroPoints(accountInfo.microPoints - 100);
							}
						}
					}
					else
					{
						response.setExtra(VotekickExtras::VOTEKICK_ERROR_TARGET_LEFT);
						response.setData(nullptr, 0);
						session->asyncWrite(response);
					}
				}
				else if (request.getOrder() == 125 && request.getExtra() == 29)
				{ // Votekick "Y" vote (+1)
					room->votekickVoteYes();
				}
				else if (request.getOrder() == 125 && request.getExtra() == 42)
				{ // Votekick result
					const auto targetUniqueId = room->getTargetVotekickUid();
					const auto votekickResultPair = room->getVotekickResultDetails();
					const std::size_t totalPlayersInMatch = room->getTotalPlayersInMatch();
					const std::uint32_t totalNo = (votekickResultPair.first <= totalPlayersInMatch) ? (totalPlayersInMatch - votekickResultPair.first) : 0;

					if (votekickResultPair.first > totalNo)
					{
						room->votekickPlayer(targetUniqueId);
						room->broadcastMessage(votekickResultPair.second + " has been votekicked."
							" [Yes: " + std::to_string(votekickResultPair.first) + ", No: " + std::to_string(totalNo) + "]");
					}
					else
					{
						room->broadcastMessage(votekickResultPair.second + " 's votekick has not passed."
							" [Yes: " + std::to_string(votekickResultPair.first) + ", No: " + std::to_string(totalNo) + "]");
					}
				}
				else if (request.getMission() == 2 && request.getExtra() == 18) 
				{ // add password without clicking "room settings" first
					if (!room->isHost(session->getAccountInfo().uniqueId)) return;
					char newPassword[Common::Constants::maxPassword]{};
					std::memcpy(newPassword, request.getData(), sizeof(newPassword));
					room->updatePassword(newPassword);
					room->broadcastToRoom(response);
				}
				else if (request.getMission() == 3) 
				{ // password removed without clicking "room settings" first
					if (!room->isHost(session->getAccountInfo().uniqueId)) return;
					room->updatePassword("");
					room->broadcastToRoom(response);
				}
				else if (request.getOrder() == 159 && room->changePlayerTeam(session->getAccountInfo().uniqueId, request.getOption()))
				{ // team switch
					response.setOrder(313); 
					response.setExtra(1);  // success - add  sanity checks? (the client already prevents this)
					auto uniqueId = session->getAccountInfo().uniqueId;
					response.setData(reinterpret_cast<std::uint8_t*>(&uniqueId), sizeof(uniqueId));
					room->broadcastToRoom(response);
				}
				else
				{
					const std::uint32_t previousGameMode = room->getRoomSettings().mode;
					const bool wasPreviousGameModeTeamBased = room->isModeTeamBased();

					if (request.getOrder() == 129 && request.getMission() == 0
						|| request.getOrder() == 126 && request.getMission() == 0)
					{
						room->updatePassword("");
					}
					if (request.getDataSize() == sizeof(Main::Structures::RoomSettingsUpdateBase))
					{ // Both title & password changed
						if (!room->isHost(session->getAccountInfo().uniqueId)) return;
						auto updatedRoomSettings = Main::Details::parseData<Main::Structures::RoomSettingsUpdateBase>(request);
						room->updateRoomSettings(updatedRoomSettings, request.getOption());

						// Disable team balance
						updatedRoomSettings.isTeamBalanceOn = 0;
						response.setData(reinterpret_cast<std::uint8_t*>(&updatedRoomSettings), request.getDataSize());
						room->broadcastToRoom(response);
					}
					else if (request.getDataSize() == sizeof(Main::Structures::RoomSettingsUpdateTitle))
					{
						if (!room->isHost(session->getAccountInfo().uniqueId)) return;
						Main::Structures::RoomSettingsUpdateTitle updatedRoomSettings = Main::Details::parseData<Main::Structures::RoomSettingsUpdateTitle>(request);

						room->updateRoomSettings(updatedRoomSettings.roomSettingsUpdateBase, request.getOption());
						room->updateTitle(updatedRoomSettings.title);

						// Disable team balance
						updatedRoomSettings.roomSettingsUpdateBase.isTeamBalanceOn = 0;
						response.setData(reinterpret_cast<std::uint8_t*>(&updatedRoomSettings), request.getDataSize());
						room->broadcastToRoom(response);
					}
					else if (request.getDataSize() == sizeof(Main::Structures::RoomSettingsUpdatePassword))
					{
						if (!room->isHost(session->getAccountInfo().uniqueId)) return;
						Main::Structures::RoomSettingsUpdatePassword updatedRoomSettings = Main::Details::parseData<Main::Structures::RoomSettingsUpdatePassword>(request);

						room->updateRoomSettings(updatedRoomSettings.roomSettingsUpdateBase, request.getOption());
						room->updatePassword(updatedRoomSettings.password);

						// Disable team balance
						updatedRoomSettings.roomSettingsUpdateBase.isTeamBalanceOn = 0;
						response.setData(reinterpret_cast<std::uint8_t*>(&updatedRoomSettings), request.getDataSize());
						room->broadcastToRoom(response);
					}
					else if (request.getDataSize() == sizeof(Main::Structures::RoomSettingsUpdateTitlePassword))
					{
						if (!room->isHost(session->getAccountInfo().uniqueId)) return;
						Main::Structures::RoomSettingsUpdateTitlePassword updatedRoomSettings =
							Main::Details::parseData<Main::Structures::RoomSettingsUpdateTitlePassword>(request);

						room->updateRoomSettings(updatedRoomSettings.roomSettingsUpdateBase, request.getOption());
						room->updatePassword(updatedRoomSettings.password);
						room->updateTitle(updatedRoomSettings.title);

						// Disable team balance
						updatedRoomSettings.roomSettingsUpdateBase.isTeamBalanceOn = 0;
						response.setData(reinterpret_cast<std::uint8_t*>(&updatedRoomSettings), request.getDataSize());
						room->broadcastToRoom(response);
					}

					const std::uint32_t newGameMode = room->getRoomSettings().mode;
					const bool isNewGameModeTeamBased = room->isModeTeamBased();
					if (previousGameMode != newGameMode)
					{
						if (!wasPreviousGameModeTeamBased && isNewGameModeTeamBased) 
						{ // Mode changed from "Non Team Based" to "Team Based"
							// Disable Team balance for now: it causes issues such as team bugs.
							response.setCommand(125, 0, 0, room->getRoomSettings().mode);
							const auto settings = room->getRoomSettingsUpdate();
							response.setData(reinterpret_cast<const std::uint8_t*>(&settings), sizeof(settings));
							session->asyncWrite(response);

							room->updatePlayersTeamToTeamBased();
						}
						else if (wasPreviousGameModeTeamBased && !isNewGameModeTeamBased) 
						{ // Otherwise, check whether we went from TeamBased mode => NonTeamBased mode
							room->updatePlayersTeamToNonTeamBased();
						}
					}
				}
			}
		}
	}
}

#endif