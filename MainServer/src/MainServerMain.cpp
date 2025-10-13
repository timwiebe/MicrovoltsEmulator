
#include <iostream>
#include <chrono>
#include <format>
#include <asio/execution_context.hpp>
#include "../include/MainServer.h"
#include "../include/Structures/AccountInfo/MainAccountInfo.h"
#include "../include/ConstantDatabase/CdbSingleton.h"
#include <thread>

#include <ConstantDatabase/Structures/CdbWeaponsInfo.h>
#include <ConstantDatabase/Structures/CdbItemInfo.h>
#include <ConstantDatabase/Structures/CdbUpgradeInfo.h>
#include <ConstantDatabase/Structures/CdbCapsuleInfo.h>
#include <ConstantDatabase/Structures/CdbCapsulePackageInfo.h>
#include <ConstantDatabase/Structures/SetItemInfo.h>
#include <ConstantDatabase/Structures/CdbRewardInfo.h>
#include <ConstantDatabase/Structures/CdbPackageInfos.h>
#include <ConstantDatabase/Structures/CdbVendor.h>
#include <ConstantDatabase/Structures/CdbItemWeapon.h>
#include <ConstantDatabase/Structures/CdbEffectInfo.h>
#include <ConstantDatabase/Structures/CdbCollectionInfo.h>
#include <ConstantDatabase/Structures/CdbMissionEventInfo.h>

#include "../include/Detail/Utilities.h"
#include "Utils/Logger.h"

void printInitialInformation()
{
	auto const time = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());
	auto const time_s = std::format("{:%Y-%m-%d %X}", time);
	Utils::Logger::log("Main server initialized on " + time_s);
}

void initializeCdbFiles()
{
	const std::string cdbItemInfoPath = "../ExternalLibraries/cgd_original/ENG";
	const std::string cdbItemInfoName = "iteminfo.cdb";
	const std::string cdbWeaponItemInfoName = "itemweaponsinfo.cdb";

	Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbItemInfo>::initialize(cdbItemInfoPath, cdbItemInfoName);
	Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbItemInfo>::initializeItemTypes(cdbItemInfoPath, cdbWeaponItemInfoName, cdbItemInfoName);
	Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::SetItemInfo>::initialize(cdbItemInfoPath, "setiteminfo.cdb");
	Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbWeaponInfo>::initialize(cdbItemInfoPath, cdbWeaponItemInfoName);
	Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbUpgradeInfo>::initialize(cdbItemInfoPath, "upgradeinfo.cdb");
	Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbCapsuleInfo>::initialize(cdbItemInfoPath, "gachaponinfo.cdb");
	Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbCapsulePackageInfo>::initialize(cdbItemInfoPath, "gachaponpackageinfo.cdb");
	Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbRewardInfo>::initialize(cdbItemInfoPath, "rewardinfo.cdb");
	Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbGradeInfo>::initialize(cdbItemInfoPath, "gradeinfo.cdb");
	Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbItemPackageInfo>::initialize(cdbItemInfoPath, "itempackageinfo.cdb");
	Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbVendorInfo>::initialize(cdbItemInfoPath, "vendorinfo.cdb");
	Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbMapInfo>::initialize(cdbItemInfoPath, "mapinfo.cdb");
	Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbCollectionInfo>::initialize(cdbItemInfoPath, "collectioninfo.cdb");
    Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbEffectInfo>::initialize(cdbItemInfoPath, "effectinfo.cdb");
    Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbEventMissionInfo>::initialize(cdbItemInfoPath, "eventmissioninfo.cdb");
	Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbItemWeapon>::initialize(
		Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbItemInfo>::getInstance(),
		Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbWeaponInfo>::getInstance());

	const auto rareCapsuleItems = Main::CdbUtils::getAllRareCapsuleItems();
	Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbItemInfo>::filterGambleItemsByRareCapsules(rareCapsuleItems);
	Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbWeaponInfo>::filterGambleItemsByRareCapsules(rareCapsuleItems);

	Utils::Logger::newline();
	Utils::Logger::log("Constant database cache successfully initialized", Utils::LogType::Info);

	Main::Details::generateRewards<7>();
	Main::Details::generateRewards<32>();
}

int main()
{
	Common::Utils::setConsoleTitle(L"Microvolts Main Server");

	printInitialInformation();
	initializeCdbFiles();

	asio::io_context io_context;
	boost::asio::io_context io_context_boost;

	auto parsedServerInfo = Common::Utils::SetupParser::getInstance().getSelfMainServerInfo();
	Utils::Logger::log(std::format("Server Information: IP: {},  Port: {},  IPC Port: {},  Server Number: {}",
		parsedServerInfo.ip, parsedServerInfo.port,
		parsedServerInfo.ipcPort, parsedServerInfo.serverNumber),
		Utils::LogType::Normal);

	auto websiteInfo = Common::Utils::SetupParser::getInstance().getWebsiteSetup();
	Utils::Logger::log(std::format("Website Information: IP: {},  Port: {}",
		websiteInfo.ip, websiteInfo.port), Utils::LogType::Normal);

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

	Main::MainServer srv(io_context, io_context_boost, parsedServerInfo, websiteInfo.port);

	srv.asyncAccept();
	srv.asyncAcceptIpcServer();
	srv.asyncAcceptHttp(websiteInfo.ip);

	std::thread t1([&io_context_boost]() { io_context_boost.run(); });

	std::thread t2([&io_context]() { io_context.run();  });

	t1.join();
	t2.join();
}
