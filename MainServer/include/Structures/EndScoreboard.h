#ifndef END_SCOREBOARD_STRUCT_H
#define END_SCOREBOARD_STRUCT_H

#include <cstdint>
#include "AccountInfo/MainAccountUniqueId.h"

namespace Main
{
	// Todo: Remove assists, recheck this!
	namespace Structures
	{
#pragma pack(push, 1)
		struct ClientEndingMatch
		{
			std::uint32_t meleeKills : 8 = 0; // total infections for zombie mode
			std::uint32_t rifleKills : 8 = 0; // ZM: always 0
			std::uint32_t shotgunKills : 8 = 0;// ZM: always 0
			std::uint32_t sniperKills : 8 = 0;// ZM: always 0
			std::uint32_t mgKills : 8 = 0;// ZM: always 0
			std::uint32_t bazookaKills : 8 = 0;// ZM: always 0
			std::uint32_t grenadeKills : 8 = 0;// ZM: always 0
			std::uint32_t killstreak : 8 = 0; // also counted in zombie mode
			std::uint32_t totalKills : 8 = 0; // For zombie, client does totalKills * 3 (so 1 zombie kill in client is 3 total kills) => re-check this!
			std::uint32_t totalDeaths : 8 = 0; // also counted in zombie mode
			std::uint32_t headshots : 8 = 0; // also counted in zombie mode
			std::uint32_t assists : 8 = 0; // ZM: always 0

			// Zombie related
			std::uint32_t u1 : 8 = 0; 
			std::uint32_t totalSurvivedZombieMode : 8 = 0; 
			std::uint32_t u2 : 8 = 0;
			std::uint32_t u3 : 8 = 0;
			Main::Structures::UniqueId uniqueId{};
		};
#pragma pack(pop)


#pragma pack(push, 1)
		struct ScoreboardResponse
		{
			std::uint32_t meleeKills : 8;
			std::uint32_t rifleKills : 8;
			std::uint32_t shotgunKills : 8;
			std::uint32_t sniperKills : 8;
			std::uint32_t mgKills : 8;
			std::uint32_t bazookaKills : 8;
			std::uint32_t grenadeKills : 8;
			std::uint32_t probablyKillStreak : 8;
			std::uint32_t totalKills : 8;
			std::uint32_t deaths : 8;
			std::uint32_t headshots : 8;
			std::uint32_t assists : 8;
			std::uint32_t u0 = 1; // seems related to AFK system or time passed since match start
			// if 0, then no MP or EXP is given, if more then MP and EXP is given
			std::uint32_t newTotalMP = 0;
			std::uint32_t newTotalEXP = 0; 
			std::uint32_t newTotalClanContribution = 0; // probably wrong but who cares

			ScoreboardResponse(const Main::Structures::ClientEndingMatch& finalScoreGivenByClient)
				: meleeKills{ finalScoreGivenByClient.meleeKills }, rifleKills{ finalScoreGivenByClient.rifleKills }, shotgunKills{ finalScoreGivenByClient.shotgunKills },
				sniperKills{ finalScoreGivenByClient.sniperKills }, mgKills{ finalScoreGivenByClient.mgKills }, bazookaKills{ finalScoreGivenByClient.bazookaKills },
				grenadeKills{ finalScoreGivenByClient.grenadeKills }, probablyKillStreak{ finalScoreGivenByClient.killstreak }, totalKills{ finalScoreGivenByClient.totalKills },
				deaths{ finalScoreGivenByClient.totalDeaths }, headshots{ finalScoreGivenByClient.headshots }, assists{ finalScoreGivenByClient.assists }
			{
			}

			ScoreboardResponse() = default;

			std::array<std::uint32_t, 7> weaponKills() const
			{
				return 
				{
					meleeKills,
					rifleKills,
					shotgunKills,
					sniperKills,
					mgKills,
					bazookaKills,
					grenadeKills
				};
			}
		};
#pragma pack(pop)

	}
}

#endif
