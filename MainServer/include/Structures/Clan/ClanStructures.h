#ifndef CLAN_STRUCTURES_HEADER
#define CLAN_STRUCTURES_HEADER

#include <cstdint>
#include "../AccountInfo/MainAccountUniqueId.h"
#include "../ClientData/Structures.h"
#include "Macros.h"
#include <cstring> 

namespace Main
{
	namespace Structures
	{
PACK_PUSH(1)
		// This structure is used when one clicks on "Clan Match" (= info about clan rooms of the target player's clan)
		struct PartyInfo
		{
			std::uint16_t clanRoomId = 0;
			std::uint16_t clanRoomNumber = 0;
			std::uint32_t numPlayers : 4 = 1; // default
			std::uint32_t maxPlayers : 4 = 8; // default
			std::uint32_t unknown0 : 2 = 0;
			std::uint32_t hasMatchStarted : 1 = 0;
			std::uint32_t unknown1 : 2 = 0;
			std::uint32_t leaderLevel : 7 = 0;
			std::uint32_t unknown2 : 12 = 0;
			char leaderName[16]{};
		};
PACK_POP()

		// Single info of a player who's waiting in a clan room
PACK_PUSH(1)
		struct PartyPlayerInfo
		{
			Main::Structures::UniqueId uid{};
			std::uint32_t level : 7 = 0;
			std::uint32_t u0 : 3 = 0;
			std::uint32_t clanContribution : 22 = 0;
			std::uint64_t totalClanWins : 23 = 0;
			std::uint64_t totalClanLosses : 23 = 0;
			std::uint64_t totalClanDraws : 14 = 0;
			std::uint64_t padding : 4 = 0;
			char nickname[16]{};
		};
PACK_POP()

PACK_PUSH(1)
		struct ClanRoomSettings
		{
			std::uint32_t mode : 5 = 0; // 19 = bomb battle, 18 = tdm, 17 = eli, 16 = CTB
			std::uint32_t playersPerTeam : 3 = 0; // 4, 5, 6, 7 (0 = 8v8)
			std::uint32_t u0 : 1 = 0;
			std::uint32_t map : 7 = 18; 
			std::uint32_t u1 : 3 = 0;
			std::uint32_t hasPassword : 1 = 0;
			std::uint32_t rest : 12 = 0;
			char password[9]{};
			char u2[7]{};

			explicit ClanRoomSettings(const Main::ClientData::ClanRoomSettings settings)
				: mode{ settings.mode }, map{ settings.map }
			{
			}
		};
PACK_POP()

		// Used to send the info of the player who joined the clan match to the other players waiting
PACK_PUSH(1)
		struct JoinPartyInfo
		{
			char nickname[16]{};
			Main::Structures::UniqueId uniqueId{};
			std::uint32_t level : 7 = 0;
			std::uint32_t unknown : 3 = 0;
			std::uint32_t clanContribution : 22 = 0;
			std::uint64_t totalClanWins : 23 = 0;
			std::uint64_t totalClanLosses : 23 = 0;
			std::uint64_t totalClanDraws : 14 = 0;
			std::uint64_t padding : 4 = 0;

			explicit JoinPartyInfo(const char* nickname, const Main::Structures::UniqueId& uniqueId,
				std::uint32_t level, std::uint32_t clanContrib, std::uint32_t clanWins, std::uint32_t clanLosses,
				std::uint32_t clanDraws)
				: uniqueId{ uniqueId }, level{ level }, clanContribution{ clanContrib }
				, totalClanWins{ clanWins }, totalClanLosses{ clanLosses }, totalClanDraws{ clanDraws }
			{
				std::memcpy(this->nickname, nickname, Common::Constants::maxNicknameSize);
			}
		};
PACK_POP()

PACK_PUSH(1)
		struct RegisteredClanInfo
		{
			std::uint64_t clanRoomId : 16 = 0;
			std::uint64_t clanRoomNumber : 16 = 0;
			std::uint64_t u0 : 4 = 0; // contains "hasPassword" bit and probably something else
			std::uint64_t mode : 5 = 0;
			std::uint64_t u1 : 3 = 0;
			std::uint64_t maxPlayers : 5 = 30;
			std::uint64_t map : 7 = 2;
			std::uint64_t level : 7 = 67;
			std::uint64_t clanLogoFrontIdLastBit : 1 = 0; 
			std::uint32_t clanLogoFrontId : 15 = 0; 
			std::uint32_t clanLogoBackId : 16 = 0;

			char clanName[Common::Constants::maxNicknameSize]{};
			char leaderName[Common::Constants::maxNicknameSize]{};

			explicit RegisteredClanInfo(std::uint64_t initMode,
				std::uint64_t initMaxPlayers,
				std::uint64_t initMap,
				std::uint64_t initLevel,
				std::uint16_t initClanLogoFrontId,
				std::uint16_t initClanLogoBackId,
				const char* initClanName,
				const char* initLeaderName,
				std::uint64_t roomId,
				std::uint64_t roomNum)
				: mode(initMode), maxPlayers(initMaxPlayers), map(initMap), level(initLevel),
				clanLogoFrontId((initClanLogoFrontId >> 1) & 0x7FFF), 
				clanLogoFrontIdLastBit(initClanLogoFrontId & 0x1),           
				clanLogoBackId(initClanLogoBackId), clanRoomId(roomId), clanRoomNumber(roomNum) 
			{
				std::memcpy(clanName, initClanName, Common::Constants::maxNicknameSize);
				std::memcpy(leaderName, initLeaderName, Common::Constants::maxNicknameSize);
			}
		};
PACK_POP()
	}
}

#endif