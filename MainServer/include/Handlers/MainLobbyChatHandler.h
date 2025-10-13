#ifndef MAIN_CHAT_HANDLER_H
#define MAIN_CHAT_HANDLER_H

#include "Network/Session.h"
#include "Network/Packet.h"
#include "../MainEnums.h"
#include "../Classes/RoomsManager.h"
#include "../ChatCommands/ChatCommands.h"
#include "../Classes/Room.h"
#include <cstring> 
#include <vector>

namespace Main
{
	namespace Handlers
	{
		inline Main::Enums::ChatGrade getChatGrade(Common::Enums::PlayerGrade playerGrade)
		{
 		   if (playerGrade == Main::Enums::PlayerGrade::GRADE_NORMAL)
		    {
		        return static_cast<Main::Enums::ChatGrade>(playerGrade - 1);
		    }
		    else if (playerGrade == Main::Enums::PlayerGrade::GRADE_MOD || playerGrade == Main::Enums::PlayerGrade::GRADE_ES)
		    {
		        return static_cast<Main::Enums::ChatGrade>(playerGrade - 2);
		    }
		    else if (playerGrade == Main::Enums::PlayerGrade::GRADE_TESTER)
		    {
		        return Main::Enums::ChatGrade::CHAT_TESTER;
		    }
		    else if (playerGrade == Main::Enums::PlayerGrade::GRADE_GM)
 		   {
		        return Main::Enums::ChatGrade::CHAT_GM;
		    }
		    return Main::Enums::ChatGrade::CHAT_NORMAL;
		}

		inline void executeCommand(std::shared_ptr<Main::Network::Session> session, const Common::Network::Packet& request, Common::Network::Packet& response,
			Main::Classes::RoomsManager& roomsManager, Main::Command::ChatCommands& chatCommands, Main::Network::SessionsManager& sessionsManager,
			Main::Persistence::MainScheduler& scheduler, const Main::Network::Session::AccountInfo& accountInfo, Main::MainServer& mainSv)
		{
			const std::string command{ reinterpret_cast<const char*>(request.getData() + 1), static_cast<std::size_t>(request.getOption() - 1) };
			if (command == "commands" || command == "?")
			{
				Main::Command::ChatCommands::showUsages(session, response, static_cast<Common::Enums::PlayerGrade>(accountInfo.playerGrade));
				return;
			}
			const std::string commandName = command.substr(0, command.find(' '));
			Main::Command::ChatCommands::executeCommand(commandName, command, session, sessionsManager, roomsManager, scheduler,
				session->getPlayer().getRoomNumber(), mainSv);
		}

		inline void handleWhisperMessage(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
			Main::Network::SessionsManager& sessionsManager, Common::Network::Packet& response, Main::Persistence::MainScheduler& scheduler)
		{
			// Rationale: the client sends a nickname if the receiver is outside a room, otherwise it sends its uniqueId
			char receiverNickname[16];
			std::memcpy(receiverNickname, request.getData(), Common::Constants::maxNicknameSize);

			bool inRoom = false;
			auto targetSession = sessionsManager.findSessionByName(receiverNickname);
			if (!targetSession)
			{
				const Main::Structures::UniqueId uniqueId = Main::Details::parseData<Main::Structures::UniqueId>(request);
				targetSession = sessionsManager.getSessionBySessionId(uniqueId.session);
				inRoom = true;
			}

			const auto& accountInfo = session->getAccountInfo();
			const char* senderNickname = accountInfo.nickname;
			const uint8_t* uid = reinterpret_cast<const uint8_t*>(&(accountInfo.uniqueId));
			const char* message = reinterpret_cast<const char*>(request.getData() + (inRoom ? sizeof(Main::Structures::UniqueId) : Common::Constants::maxNicknameSize));

			std::size_t dataSize = request.getOption();
			std::string prefix = "[Whisper To: " + std::string(receiverNickname) + "]: ";
			std::string logMessage = prefix + std::string(message, message + dataSize);
			scheduler.addRepetitiveCallback(std::source_location::current(), accountInfo.accountID,
				&Main::Persistence::PersistentDatabase::logMessage, accountInfo.accountID, logMessage);

			std::vector<std::uint8_t> responseData(Common::Constants::maxNicknameSize + request.getOption() + sizeof(accountInfo.uniqueId));
			std::copy(uid, uid + sizeof(accountInfo.uniqueId), responseData.begin());
			std::copy(senderNickname, senderNickname + Common::Constants::maxNicknameSize, responseData.begin() + sizeof(accountInfo.uniqueId));
			std::copy(message, message + request.getOption(), responseData.begin() + Common::Constants::maxNicknameSize + sizeof(accountInfo.uniqueId));
			response.setData(responseData.data(), responseData.size());

			if (targetSession)
			{
				if (session->getPlayer().hasBlocked(targetSession->getAccountInfo().accountID))
				{
					response.setExtra(Enums::WhisperExtra::WHISPER_SENDER_BLOCKED_RECEIVER);
				}
				else if (targetSession->getPlayer().hasBlocked(accountInfo.accountID))
				{
					response.setExtra(Enums::WhisperExtra::WHISPER_RECEIVER_BLOCKED_SENDER);
				}
				else
				{
					sessionsManager.sendTo(targetSession->getId(), response);
					response.setExtra(Enums::WhisperExtra::WHISPER_SENT);
				}
			}
			else
			{
				response.setExtra(Enums::WhisperExtra::RECEIVER_OFFLINE);
			}

			// Auto-sends the message, sort of a confirmation
			response.setOrder(315); 
			response.setMission(1);  // 1 = WhisperConfirmation
			response.setData(nullptr, 0);
			session->asyncWrite(response);
		}

		inline bool executeCommon(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
			Main::Network::SessionsManager& sessionsManager,
			Main::Command::ChatCommands& chatCommands, Main::Classes::RoomsManager& roomsManager, Main::Persistence::MainScheduler& scheduler,
			const Main::Network::Session::AccountInfo& accountInfo, Common::Network::Packet& response, Main::MainServer& mainSv)
		{
			if (session->getPlayer().isMuted())
			{
				session->sendMessage("you have been muted by a moderator.");
				return true;
			}
			else if (request.getExtra() == Enums::ChatExtra::COMMAND)
			{
				executeCommand(session, request, response, roomsManager, chatCommands, sessionsManager, scheduler, accountInfo, mainSv);
				return true;
			}
			else if (request.getExtra() == Enums::ChatExtra::WHISPER)
			{
				handleWhisperMessage(request, session, sessionsManager, response, scheduler);
				return true;
			}
			return false;
		}


		inline void handleLobbyChatMessage(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session, 
			Main::Network::SessionsManager& sessionsManager,
			Main::Command::ChatCommands& chatCommands, Main::Classes::RoomsManager& roomsManager, Main::Persistence::MainScheduler& scheduler, 
			Main::MainServer& mainSv)
		{
			START_BENCHMARK

			const bool hasBeenMatchBanned = session->hasBeenMatchBanned();
			const auto& accountInfo = session->getAccountInfo();
			Common::Network::Packet response;
			response.setTcpHeader(request.getSession(), Common::Enums::NO_ENCRYPTION);
			response.setCommand(316, hasBeenMatchBanned ? Main::Enums::ChatGrade::CHAT_TESTER : getChatGrade(static_cast<Common::Enums::PlayerGrade>(accountInfo.playerGrade)), 
				request.getExtra(), request.getOption());

			const char* originalMessage = reinterpret_cast<const char*>(request.getData());
			const uint8_t* originalData = request.getData();
			std::size_t dataSize = request.getOption();
			std::string logMessage(originalData, originalData + dataSize);
			scheduler.addRepetitiveCallback(std::source_location::current(), accountInfo.accountID,
				&Main::Persistence::PersistentDatabase::logMessage, accountInfo.accountID, logMessage);


			if (!hasBeenMatchBanned && executeCommon(request, session, sessionsManager, chatCommands, roomsManager, scheduler, accountInfo, response, mainSv))
			{
				return;
			}

			const char* cheaterMessage = "I suck at this game and that's why I'm a dirty cheater";

			const char* messageToSend = hasBeenMatchBanned ? cheaterMessage : originalMessage;
			std::size_t messageLength = hasBeenMatchBanned ? std::strlen(cheaterMessage) : request.getOption();

			std::vector<std::uint8_t> responseData(Common::Constants::maxNicknameSize + messageLength);
			std::copy(accountInfo.nickname, accountInfo.nickname + Common::Constants::maxNicknameSize, responseData.begin());
			std::copy(messageToSend, messageToSend + messageLength, responseData.begin() + Common::Constants::maxNicknameSize);
			response.setData(responseData.data(), responseData.size());

			if (request.getExtra() == Enums::ChatExtra::NORMAL)
			{
				sessionsManager.broadcastToLobbyExceptSelf(session->getId(), response);
			}
			else
			{
				sessionsManager.broadcastToClan(session->getId(), response);
			}

			END_BENCHMARK(handleLobbyChatMessage, session)
		}


		inline void handleRoomChatMessage(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session, 
			Main::Network::SessionsManager& sessionsManager,
			Main::Command::ChatCommands& chatCommands, Main::Classes::RoomsManager& roomsManager,
			Main::Persistence::MainScheduler& scheduler, Main::MainServer& mainSv)
		{
			const bool hasBeenMatchBanned = session->hasBeenMatchBanned();
			const auto& accountInfo = session->getAccountInfo();
			Common::Network::Packet response;
			response.setCommand(316, hasBeenMatchBanned ? Main::Enums::ChatGrade::CHAT_TESTER
				: getChatGrade(static_cast<Common::Enums::PlayerGrade>(accountInfo.playerGrade)), 
				request.getExtra(), request.getOption());
			response.setTcpHeader(request.getSession(), Common::Enums::NO_ENCRYPTION);

			const char* originalMessage = reinterpret_cast<const char*>(request.getData());

			const uint8_t* originalData = request.getData(); 
			std::size_t dataSize = request.getOption(); 
			std::string logMessage(originalData, originalData + dataSize);
			scheduler.addRepetitiveCallback(std::source_location::current(), accountInfo.accountID,
				&Main::Persistence::PersistentDatabase::logMessage, accountInfo.accountID, logMessage);

			if (!hasBeenMatchBanned && executeCommon(request, session, sessionsManager, chatCommands, roomsManager, scheduler, accountInfo, response, mainSv))
			{
				return;
			}

			const char* senderNickname = accountInfo.nickname;
			const char* cheatMessage = "I suck at this game and that's why I'm a dirty cheater";

			const char* messageToSend = hasBeenMatchBanned ? cheatMessage : originalMessage;
			std::size_t messageLength = hasBeenMatchBanned ? std::strlen(cheatMessage) : request.getOption();

			std::vector<std::uint8_t> responseData(Common::Constants::maxNicknameSize + messageLength);
			std::copy(senderNickname, senderNickname + Common::Constants::maxNicknameSize, responseData.begin());
			std::copy(messageToSend, messageToSend + messageLength, responseData.begin() + Common::Constants::maxNicknameSize);
			response.setData(responseData.data(), responseData.size());

			if (request.getExtra() == Enums::ChatExtra::CLAN)
			{
				sessionsManager.broadcastToClan(session->getId(), response);
				return;
			}
			else if (auto* room = roomsManager.getRoomByNumber(session->getPlayer().getRoomNumber()))
			{
				if (room->isMuted() && session->getAccountInfo().playerGrade < Common::Enums::GRADE_ES)
				{
					session->sendMessage("the room is currently muted");
				}
				else if (session->getPlayer().getPlayerState() == Common::Enums::STATE_DYING)
				{
					room->broadcastToDeadExceptSelf(response, session, request.getExtra());
				}
				else if (session->getPlayer().isInMatch())
				{
					room->broadcastToMatchExceptSelf(response, session, request.getExtra());
				}
				else
				{
					room->broadcastOutsideMatchExceptSelf(response, session, request.getExtra());
				}
			}
		}
	}
}

#endif
