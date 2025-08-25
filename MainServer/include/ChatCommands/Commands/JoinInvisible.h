#ifndef JOINROOM_INVISIBLE_HEADER
#define JOINROOM_INVISIBLE_HEADER

#include "../ChatCommands.h"
#include "../ICommand.h"
#include "Utils/Utils.h"
#include "../../MainServer.h"
#include "../../Handlers/Room/RoomJoinHandler.h"
#include "../../Handlers/Room/RoomStartHandler.h"

namespace Main
{
	namespace Command
	{
		class Join final : public ICommand
		{
		private:
			std::uint32_t m_roomNumber{};

			bool parseCommand(const std::string& providedCommand) override
			{
				std::smatch match;
				if (std::regex_match(providedCommand, match, m_pattern))
				{
					const std::string& matched_str = match[1].str();
					const std::from_chars_result result = std::from_chars(matched_str.data(), matched_str.data() + matched_str.size(), m_roomNumber);
					if (result.ec == std::errc())
					{
						return true;
					}
				}
				return false;
			}

		public:
			explicit Join(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/join <roomNumber>",  R"(^\S+\s(\d+))" }
			{
			}

			void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager&, MC::RoomsManager& roomsManager, 
				MP::MainScheduler&, std::uint32_t,
				Main::MainServer&) override
			{
				if (!parseCommand(command))
				{
					session->sendMessage("parsing error");
					return;
				}

				Common::Network::Packet response;

				Main::ClientData::RoomInfo joinInfo{ m_roomNumber - 1 };
				response.setTcpHeader(session->getId(), Common::Enums::NO_ENCRYPTION);
				response.setCommand(140, 0, 0, 0);
				response.setData(reinterpret_cast<std::uint8_t*>(&joinInfo), sizeof(joinInfo));

				if (session->getPlayer().getPlayerState() != Common::Enums::STATE_LOBBY || !session->getPlayer().isInLobby())
				{
					session->sendMessage("error: You must be in the lobby when using this command");
				}
				else if (Main::Classes::Room* room = roomsManager.getRoomByNumber(m_roomNumber))
				{
					session->sendMessage("success");
					Main::Handlers::handleRoomJoin(response, session, roomsManager, false, joinInfo, true);
					session->setIsInvisible(true);
				}
				else
				{
					session->sendMessage("error: Room not found.");
				}
			}
		};

		REGISTER_CMD(Join, Common::Enums::PlayerGrade::GRADE_MOD)



		struct EnterMatch final : public ICommand
		{
			explicit EnterMatch(const Common::Enums::PlayerGrade requiredGrade)
				: ICommand{ requiredGrade, "/entermatch" }
			{
			}

			void execute(const std::string&, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager, MC::RoomsManager& roomsManager, 
				MP::MainScheduler&, std::uint32_t roomNumber,
				Main::MainServer& mainSv) override
			{
				Common::Network::Packet response;
				response.setTcpHeader(session->getId(), Common::Enums::NO_ENCRYPTION);
				response.setCommand(107, 0, 38, 0);
				response.setData(nullptr, 0);
				if (Main::Handlers::handleRoomStartInvisible(response, session, roomsManager))
				{
					session->sendMessage("success");
				}
			}
		};

		REGISTER_CMD(EnterMatch, Common::Enums::PlayerGrade::GRADE_MOD)
	}
}


#endif