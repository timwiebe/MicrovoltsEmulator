#ifndef ROOM_SETTINGS_UPDATE_H
#define ROOM_SETTINGS_UPDATE_H

#include <cstdint>
#include "Macros.h"
#include <cstring> 

namespace Main
{
	namespace Structures
	{
PACK_PUSH(1)
		struct RoomSettingsUpdateBase
		{
			std::uint32_t maxPlayers : 5 = 0; // ok
			std::uint32_t time : 5 = 0; // ok
			std::uint32_t weaponRestriction : 4 = 0; // ok
			std::uint32_t isItemOn : 1 = 0; // ok
			std::uint32_t isOpen : 1 = 0; // ok
			std::uint32_t map : 6 = 0; // ok
			std::uint32_t unknown1 : 1 = 0;
			std::uint32_t unknown2 : 1 = 0;
			std::uint32_t specificSetting : 5 = 0; // ok
			std::uint32_t isTeamBalanceOn : 1 = 0; // ok
			std::uint32_t unknown3 : 1 = 0;
			std::uint32_t unknown4 : 1 = 0;
		};
PACK_POP()

PACK_PUSH(1)
		struct RoomSettingsUpdateTitle
		{
			RoomSettingsUpdateBase roomSettingsUpdateBase;
			char title[30]{};
			char padding[2]{};
		};
PACK_POP()

PACK_PUSH(1)
		struct RoomSettingsUpdatePassword
		{
			RoomSettingsUpdateBase roomSettingsUpdateBase;
			char password[9]{};
			char padding[7]{};
		};
PACK_POP()

PACK_PUSH(1)
		struct RoomSettingsUpdateTitlePassword
		{
			RoomSettingsUpdateBase roomSettingsUpdateBase;
			char title[30]{};   
			char padding[2]{};
			char password[9]{};
			char padding2[7]{};

			RoomSettingsUpdateTitlePassword() = default;
			explicit RoomSettingsUpdateTitlePassword(const Main::Structures::RoomSettings& settings, const std::string& password, const std::string& title, 
				std::uint32_t specificSetting)
			{
				roomSettingsUpdateBase.isItemOn = settings.isItemOn;
				roomSettingsUpdateBase.isOpen = settings.isOpen;
				roomSettingsUpdateBase.isTeamBalanceOn = false;
				roomSettingsUpdateBase.map = settings.map;
				roomSettingsUpdateBase.maxPlayers = settings.playersPerTeam == 1 ? 1 : settings.playersPerTeam * 2;
				std::memcpy(this->password, password.c_str(), password.size() > 9 ? 9 : password.size());
				roomSettingsUpdateBase.specificSetting = specificSetting;
				roomSettingsUpdateBase.time = settings.time;
				std::memcpy(this->title, title.c_str(), title.size() > 30 ? 30 : title.size());
				roomSettingsUpdateBase.weaponRestriction = settings.weaponRestriction;
			}
		};
PACK_POP()
	}
}

#endif