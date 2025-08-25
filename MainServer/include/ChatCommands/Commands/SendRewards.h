#ifndef SENDREWARDS_COMPLEXCOMMAND_H
#define SENDREWARDS_COMPLEXCOMMAND_H

#include "../ICommand.h"
#include "../ChatCommands.h"
#include "Utils/Utils.h"
#include "Utils/Constants.h"
#include "../../MainServer.h"
#include <source_location>

namespace Main
{
	namespace Command
	{
		class SendRewards final : public ICommand
		{
		private:
			std::uint32_t m_itemId{};
			std::string m_giftDescription{};

			bool parseCommand(const std::string& providedCommand) override
			{
				std::smatch match;
				if (std::regex_match(providedCommand, match, this->m_pattern))
				{
					const std::string& itemid = match[2].str();
					auto [ptr, ec] = std::from_chars(itemid.data(), itemid.data() + itemid.size(), m_itemId);
					if (ec != std::errc{} || ptr != itemid.data() + itemid.size())
					{
						return false;
					}

					m_giftDescription = match[3].str();
					return true;
				}
				return false;
			}

		public:
			SendRewards(const Common::Enums::PlayerGrade requiredGrade) 
				: ICommand{ requiredGrade, "/sendrewards <itemid> <description>", R"(^(\S+)\s*(\d+)\s*(.*)$)" }
			{
			}

			void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager, MC::RoomsManager& roomsManager,
				MP::MainScheduler& scheduler, std::uint32_t roomNum, Main::MainServer&) override
			{
				if (!parseCommand(command))
				{
					session->sendMessage("error: parsing error, see /? for command usage");
					return;
				}
				if (!Main::CdbUtils::itemExists(m_itemId))
				{
					session->sendMessage("error: ItemID not found");
					return;
				}
				if (m_giftDescription.size() >= Common::Constants::maxMailboxMessage)
				{
					session->sendMessage("error: description should be less than 255 characters");
					return;
				}

				if (Main::Classes::Room* room = roomsManager.getRoomByNumber(roomNum))
				{
					for (auto& [u, targetSession] : room->getAllPlayersWithSessions())
					{
						if (session->getAccountId() == targetSession->getAccountId()) continue;

						if (targetSession->receiveGift(m_itemId, m_giftDescription))
						{
							session->sendMessage(" - " + std::string{ targetSession->getAccountInfo().nickname } + ": success", Main::Enums::TIP);
						}
						else
						{
							session->sendMessage(" - " + std::string{ targetSession->getAccountInfo().nickname } + ": ERROR (not enough space)");
						}
					}
				}
				else
				{
					session->sendMessage("error: you must be in a room");
				}
			}
		};

		REGISTER_CMD(SendRewards, Common::Enums::PlayerGrade::GRADE_TESTER)
	}
}
#endif
