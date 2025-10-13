#include <iostream>
#include <chrono>
#include <string>
#include <format>
#include <asio/execution_context.hpp>
#include "../include/CastServer.h"
#include "../include/ConstantDatabase/CdbSingleton.h"
#include "../include/ConstantDatabase/Structures/CdbMapInfo.h"
#include <Utils/SetupParser.h>
#include <AntiCheat/AntiCheat.h>


void printInitialInformation()
{
	auto const time = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());
	auto const time_s = std::format("{:%Y-%m-%d %X}", time);
	std::cout << "[Info] Cast server initialized on " << time_s << "\n\n";
	std::cout << "[Info] Initializing constant database maps...\n";
	const std::string cdbItemInfoPath = "../ExternalLibraries/cgd_original/ENG";
	const std::string cdbMapInfoName = "mapinfo.cdb";
	using mapInfo = Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbMapInfo>;
	mapInfo::initialize(cdbItemInfoPath, cdbMapInfoName);
	std::cout << "[Info] Constant database successfully initialized.\n";
}

int main()
{
	Common::Utils::setConsoleTitle(L"Microvolts Cast Server");
	printInitialInformation();

	asio::io_context io_context;

	auto parsedServerInfo = Common::Utils::SetupParser::getInstance().getSelfCastServerInfo();
	Utils::Logger::log(std::format("Server Information: IP: {},  Port: {},  IPC Port: {},  Server Number: {}",
		parsedServerInfo.ip, parsedServerInfo.port,
		parsedServerInfo.ipcPort, parsedServerInfo.serverNumber),
		Utils::LogType::Normal);

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

	Cast::CastServer srv(io_context, parsedServerInfo.ip, parsedServerInfo.port, parsedServerInfo.ipcPort, parsedServerInfo.serverNumber);

	srv.asyncAccept();
	srv.asyncAcceptMainServer();
	io_context.run();
}
