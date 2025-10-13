#ifndef CHANGEROOM_TITLE_COMMAND_H
#define CHANGEROOM_TITLE_COMMAND_H

#include "../ICommand.h"
#include "../ChatCommands.h"
#include "Utils/Utils.h"
#include "../../MainServer.h"
#include <cstring> 

namespace Main
{
    namespace Command
    {
        class ChangeRoomTitle : public ICommand
        {
        private:
            std::string m_title;

        public:
            explicit ChangeRoomTitle(const Common::Enums::PlayerGrade requiredGrade)
                : ICommand{ requiredGrade, "/changeroomtitle <newTitle> (max 30 chars)", R"(^\S+\s+(.*)$)"}
            {
            }

            bool parseCommand(const std::string& providedCommand) override
            {
                std::smatch match;
                if (std::regex_match(providedCommand, match, this->m_pattern))
                {
                    m_title = match[1].str();
                    return true;
                }
                return false;
            }

            void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager,
                MC::RoomsManager& roomsManager, MP::MainScheduler&, std::uint32_t,
                Main::MainServer&) override
            {
                if (!parseCommand(command))
                {
                    session->sendMessage("parse error");
                    return;
                }
                if (m_title.size() >= 30)
                {
                    session->sendMessage("error: the message must be smaller than 30 characters");
                    return;
                }
                if (Main::Classes::Room* room = roomsManager.getRoomByNumber(session->getPlayer().getRoomNumber()))
                {
                    if (room->hasMatchStarted())
                    {
                        session->sendMessage("Error: you can change the title only when the match isn't ongoing");
                        return;
                    }

                    Main::Structures::RoomSettingsUpdateTitle updatedSettings;
                    updatedSettings.roomSettingsUpdateBase = room->getRoomSettingsUpdate().roomSettingsUpdateBase;
                    std::memset(updatedSettings.title, 0, sizeof(updatedSettings.title));
                    std::memcpy(updatedSettings.title, m_title.c_str(), std::min(m_title.size(), sizeof(updatedSettings.title) - 1));
                    room->updateTitle(m_title);

                    Common::Network::Packet response;
                    response.setCommand(130, 0, 0, room->getRoomSettings().mode);
                    response.setData(reinterpret_cast<std::uint8_t*>(&updatedSettings), sizeof(updatedSettings));
                    room->broadcastToRoom(response);
                }
                else
                {
                    session->sendMessage("Error: not in a room");
                }
            }
        };

        REGISTER_CMD(ChangeRoomTitle, Common::Enums::PlayerGrade::GRADE_MOD)

    }
}

#endif
