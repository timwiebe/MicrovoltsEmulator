#ifndef GAME_CONSTANTS_HEADER
#define GAME_CONSTANTS_HEADER

#include <cstdint>
#include <string>

namespace Common
{
	namespace Constants
	{
		// IPC handler ids
		constexpr inline std::uint32_t A2M_disconnectOnlinePlayer = 0;
		constexpr inline std::uint32_t A2M_getPlayersPerServer = 1;
		constexpr inline std::uint32_t M2C_mapId = 2;
		constexpr inline std::uint32_t M2C_roomNumber = 3;
		constexpr inline std::uint32_t M2C_sessionId = 4;
		constexpr inline std::uint32_t C2M_updatePlayerState = 5;
		constexpr inline std::uint32_t A2M_passIp = 6;
		constexpr inline std::uint32_t M2C_assassinModeInfo = 7;
		constexpr inline std::uint32_t M2C_playerTeamInfoBatch = 8;
		constexpr inline std::uint32_t M2C_Invisibility = 9;

		// Rest
		constexpr inline std::uint16_t maxSessionsPerServer = 500;
		constexpr inline std::uint16_t maxServerCapacity = 300;
		constexpr inline std::uint16_t maxNicknameSize = 16;
		constexpr inline std::uint16_t maxEquippedItems = 17;
		constexpr inline std::uint16_t maxMailbox = 100;
		constexpr inline std::uint16_t maxPacketBytes = 1440;
		constexpr inline std::uint16_t headerSize = 8;
		constexpr inline std::uint16_t maxRoomPlayers = 26; // 16 + 10 observers
		constexpr inline std::uint16_t maxObserverPlayers = 10;
		constexpr inline std::uint16_t maxRoomPlayersNoObs = 16;
		constexpr inline std::uint16_t maxFriends = 59;
		constexpr inline std::uint16_t maxMailboxMessage = 256;
		constexpr inline std::uint16_t upgradeFailRate = 15;
		constexpr inline std::uint32_t maxItemProbability = 1'000'000;
		constexpr inline double capsulePriceFactor = 0.2;
		constexpr inline std::uint16_t maxLuckySpin = 1000;
		constexpr inline std::uint16_t maxItemType = 30;
		constexpr inline std::uint16_t maxRooms = 30;
		constexpr inline std::uint16_t maxPassword = 9;
		constexpr inline std::uint16_t maxClanRooms = 30;
		constexpr inline std::uint16_t maxPartiesPerClan = 4;
		constexpr inline std::uint32_t clanRoomNumberStart = 151;
		constexpr inline std::uint16_t matchBaseExp = 50;
		constexpr inline std::uint16_t matchBaseMp = 150;
		constexpr inline std::uint16_t maxExpAndMpPerMatch = 150000;
		constexpr inline std::uint16_t clanBaseContribution = 100;
		constexpr inline std::uint32_t goldLevelBox = 5336571;
		constexpr inline std::uint32_t silverLevelBox = 5336570;
		constexpr inline std::uint32_t bronzeLevelBox = 5336569;
		constexpr inline std::uint32_t tutorialBox = 4500000;
		constexpr inline std::uint32_t singlewaveEasyBox = 5337005;
		constexpr inline std::uint32_t singlewaveHardBox = 5337006;
		constexpr inline std::uint16_t eventMissionTotal = 51;
		constexpr inline std::uint16_t totalEventMissions = 5;
		inline std::string teamString = "Team";
	}
}
#endif