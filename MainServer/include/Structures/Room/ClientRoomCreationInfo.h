#ifndef CLIENT_ROOM_CREATION_INFO_H
#define CLIENT_ROOM_CREATION_INFO_H

#include <cstdint>
#include <iostream> 
#include "../../Common/include/Enums/RoomEnums.h"
#include "../../Detail/Utilities.h"
#include "Macros.h"
#include <cstring> 

// These structures are sent from client => server when a new room is created
// Team balance is apparently NOT sent ==> on room creation, set it up based on the chosen mode

namespace Main
{
	namespace Structures
	{
PACK_PUSH(1)
        struct RoomSettings
        {
            std::uint32_t time : 5 = 0;
            std::uint32_t weaponRestriction : 4 = 0;
            std::uint32_t isItemOn : 1 = 0;
            std::uint32_t mode : 5 = 0;
            std::uint32_t isOpen : 1 = 0;
            std::uint32_t hasPassword : 1 = 0;
            std::uint32_t unknown1 : 1 = 0;
            std::uint32_t playersPerTeam : 4 = 0;
            std::uint32_t map : 7 = 0;
            std::uint32_t unknown3 : 2 = 0;
            std::uint32_t isObserverModeOn : 1 = 0;

            explicit RoomSettings(std::uint32_t time, std::uint32_t weaponRestriction,
                bool isItemOn, std::uint32_t mode, bool isOpen,
                bool hasPassword, std::uint32_t playersPerTeam,
                std::uint32_t map, bool isObserverModeOn)
                : time{ time },
                weaponRestriction{ weaponRestriction },
                isItemOn{ isItemOn },
                mode{ mode },
                isOpen{ isOpen },
                hasPassword{ hasPassword },
                playersPerTeam{ playersPerTeam },
                map{ map },
                isObserverModeOn{ isObserverModeOn }
            {
            }

            RoomSettings() = default;
        };
PACK_POP()

PACK_PUSH(1)
		struct CompleteRoomInfo
		{
			RoomSettings roomSettings{};
			char title[30]{};
			std::uint16_t padding = 0;
			char password[9]{};
            char paddin2[7]{};

            explicit CompleteRoomInfo(const RoomSettings& roomSettings, const std::string& title, const char* password = "")
                : roomSettings{roomSettings}
            {
                std::memcpy(this->title, title.c_str(), sizeof(this->title));
                std::memcpy(this->password, password, sizeof(this->password));
            }

            CompleteRoomInfo() = default;
		};
PACK_POP()
	}
}

#endif

