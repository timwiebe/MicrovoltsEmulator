#ifndef ROOMS_LIST_STRUCTURE_H
#define ROOMS_LIST_STRUCTURE_H

#include <cstdint>
#include <vector>
#include "Utils/Constants.h"
#include "Macros.h"
#include <cstring> 

namespace Main
{
	namespace Structures
	{
PACK_PUSH(1)
		struct SingleRoom
		{
			char title[30]{};
			std::uint16_t padding{};
			std::uint16_t roomNumber{};
			std::uint16_t roomId : 9 = 2; // if this is 0 you can't join the room...?! 
			std::uint16_t map : 7 = 0;
			std::uint32_t mode : 5 = 0; // 15 = room not displayed, if > 15 == clan related, else if < 15 == mode, else if == 14 then more infos for aibattle
			std::uint32_t maxPlayers : 5 = 0;
			std::uint32_t numPlayers : 5 = 0;
			std::uint32_t matchStarted : 1 = 0;
			std::uint32_t hasPassword : 1 = 0;
			std::uint32_t unknown : 1 = 0;
			std::uint32_t weaponRestriction : 4 = 0;
			std::uint32_t isObserverOn : 1 = 1;
			std::uint32_t ping : 9 = 0;

			explicit SingleRoom(const char* roomTitle, std::uint16_t initRoomNumber, std::uint16_t initMap, std::uint32_t initMode, 
				std::uint32_t initMaxPlayers, std::uint32_t initNumPlayers, std::uint32_t initMatchStarted, std::uint32_t initHasPassword,
				std::uint32_t initWeaponRestriction, std::uint32_t initIsObserverOn, std::uint32_t initPing)
				: roomNumber(initRoomNumber), map(initMap), mode(initMode), maxPlayers(initMaxPlayers), numPlayers(initNumPlayers)
				, matchStarted(initMatchStarted), hasPassword(initHasPassword), weaponRestriction(initWeaponRestriction),
				isObserverOn(initIsObserverOn), ping(initPing)
			{
				std::memcpy(this->title, roomTitle, sizeof(this->title) - 1);
				title[sizeof(this->title) - 1] = '\0'; 
			}

			SingleRoom() = default;
		};
PACK_POP()

PACK_PUSH(1)
		struct RoomsList
		{
			std::uint16_t totalRooms1{};
			std::uint16_t totalRooms2{}; // apparently must be the same as totalRooms1x		
			std::vector<SingleRoom> rooms{};
		};
PACK_POP()
	}
}
#endif