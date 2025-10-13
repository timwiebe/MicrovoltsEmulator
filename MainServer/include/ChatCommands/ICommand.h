#ifndef INTERFACE_COMMAND_HEADER
#define INTERFACE_COMMAND_HEADER

#include <string>
#include "Network/Packet.h"
#include "../Network/MainSession.h"
#include "../Network/MainSessionManager.h"
#include "Enums/PlayerEnums.h"
#include "Utils/Utils.h"

#include <regex>

namespace Main
{
	class MainServer;

	namespace Command
	{
		namespace MN = Main::Network;
		namespace MC = Main::Classes;
		namespace MP = Main::Persistence;
		
		/*
			All in-game commands should inherit from this class (see examples inside the Commands folder)
			Once a command is implemented, make sure to register it to the list of commands at the end of the class by using REGISTER_CMD(ClassName, GradeRequiredToUse)
			There's a python script that will run before compilation of the MainServer:
			  - It will auto-generate a .cpp file that includes each command header. This makes sure that all commands are compiled automatically and added to the list of commands
			  - It will also check, for each class/struct, that there's a corresponding REGISTER_CMD call. If there isn't, compilation will fail
			    - If a class should not be registered in the commands (e.g. a common base class, see "Commands/Announce.h" for an example),
				  then use //NO_CHECK one line before the class declaration, e.g.:
				  
				  //NO_CHECK
				  class CustomCommand : public ICommand { ... }
		*/
		class ICommand
		{
		protected:
			std::regex m_pattern;
			Common::Enums::PlayerGrade m_requiredGrade{};
			std::string m_commandDescription{};

			explicit ICommand(Common::Enums::PlayerGrade requiredGrade, const std::string& commandDescription, const std::string& regexPattern = "")
				: m_requiredGrade{ requiredGrade }
				, m_commandDescription{ commandDescription }
				, m_pattern{ regexPattern }
			{
			}
			

		private:
			virtual bool parseCommand(const std::string& providedCommand) { return true; };

		public:
			virtual Common::Enums::PlayerGrade getRequiredGrade(Main::Persistence::MainScheduler& scheduler) const
			{
				return m_requiredGrade; 
			}

			void sendCommandUsage(std::shared_ptr<Main::Network::Session> session) const
			{
				session->sendMessage(m_commandDescription);
			}

			virtual void execute(const std::string& providedCommand, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager, 
				MC::RoomsManager& roomsManager,
				MP::MainScheduler& scheduler, std::uint32_t roomNumber, Main::MainServer& mainServer) = 0;
			virtual ~ICommand() = default;
		};
	}
}
#endif
