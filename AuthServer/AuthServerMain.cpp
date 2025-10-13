#include "Utils/Logger.h"
#include <chrono>
#include <format>
#include <asio/execution_context.hpp>
#include "../include/AuthServer.h"

#include <iostream>
#include <Utils/SetupParser.h>
#include "Utils/Utils.h"

int main()
{
	Common::Utils::setConsoleTitle(L"Microvolts Auth Server");

	auto const time = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());
	auto const time_s = std::format("{:%Y-%m-%d %X}", time);
	Utils::Logger::log("Auth server initialized on " + time_s, Utils::LogType::Info, "AuthServer");
	auto parsedServerInfo = Common::Utils::SetupParser::getInstance().getAuthSetup();

	Utils::Logger::log(std::format("Server Information: IP: {},  Port: {}",
		parsedServerInfo.ip, parsedServerInfo.port), Utils::LogType::Normal);

	const std::string banner = R"(

   _____   __          __  ____             _         ______                 _       _             
  / ____|  \ \        / / |  _ \           (_)       |  ____|               | |     | |            
 | (___   __\ \  /\  / /__| |_) | ___  __ _ _ _ __   | |__   _ __ ___  _   _| | __ _| |_ ___  _ __ 
  \___ \ / _ \ \/  \/ / _ \  _ < / _ \/ _` | | '_ \  |  __| | '_ ` _ \| | | | |/ _` | __/ _ \| '__|
  ____) | (_) \  /\  /  __/ |_) |  __/ (_| | | | | | | |____| | | | | | |_| | | (_| | || (_) | |   
 |_____/ \___/ \/  \/ \___|____/ \___|\__, |_|_| |_| |______|_| |_| |_|\__,_|_|\__,_|\__\___/|_|   
                                       __/ |                                                       
                                      |___/                                                       

    GitHub: https://github.com/SoWeBegin/MicrovoltsEmulator

)";

	Utils::Logger::log(banner, Utils::LogType::Info);



	asio::io_context io_context;
	Auth::AuthServer srv(io_context, parsedServerInfo.ip, parsedServerInfo.port);
	srv.asyncAccept();
	io_context.run();
}
