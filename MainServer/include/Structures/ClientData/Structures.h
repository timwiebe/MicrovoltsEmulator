#ifndef CLIENT_REQUEST_STRUCTURES_H
#define CLIENT_REQUEST_STRUCTURES_H

#include <cstdint>
#include "../Item/MainItemSerialInfo.h"
#include "Macros.h"

// NOTE:
// To correctly "read" client data:
// - given the client bytes, reverse their order => then to binary => read from left to right
// - if there are bitfields, the N last bits of the bitfields are the first N bits of the found binary ^
// (basically: read the binary [=>], and bitfield [^]

namespace Main
{
	namespace ClientData
	{
PACK_PUSH(1)
		struct Ping
		{
			std::uint32_t unknown : 10 = 0; 
			std::uint32_t ping : 10 = 0;
			std::uint32_t rest : 12 = 0;
		};
PACK_POP()

PACK_PUSH(1)
		struct ItemRefund
		{
			Main::Structures::ItemSerialInfo serialInfo{};
			std::uint32_t mpToAdd{};
		};
PACK_POP()

PACK_PUSH(1)
		struct ItemAddEnergy
		{
			Main::Structures::ItemSerialInfo serialInfo{};
			std::uint32_t usedEnergy{};
		};
PACK_POP()

PACK_PUSH(1)
		struct MailboxMessage
		{
			char nickname[16]{};
			char message[256]{};
		};
PACK_POP()

PACK_PUSH(1)
		struct RoomInfo
		{
			std::uint16_t roomNumber{};
			std::uint16_t unknown{ 2 }; // seemingly always 2 for some reason
			char password[9]{};
		};
PACK_POP()

PACK_PUSH(1)
		struct ClanRoomInfo
		{
			std::uint16_t clanId{};
			std::uint16_t roomNumber{};
			char password[9]{};
		};
PACK_POP()

PACK_PUSH(1)
		struct BoxOpen
		{
			Main::Structures::ItemSerialInfo serialInfo;
			Main::Structures::ItemSerialInfo serialInfo2; // e.g. capsule
		};
PACK_POP()

PACK_PUSH(1)
		struct UpgradeReset
		{
			Main::Structures::ItemSerialInfo weaponToResetSerialInfo;
			Main::Structures::ItemSerialInfo upgradeResetItemSerialInfo;
		};
PACK_POP()

PACK_PUSH(1)
		struct CapsuleSpin
		{
			std::uint32_t capsuleId{};
			std::uint32_t currencySpent{};
			std::uint64_t unknown{};
		};
PACK_POP()

PACK_PUSH(1)
		struct ClientEndingMatchHeader
		{
			std::uint32_t redScore : 8 = 0;
			std::uint32_t blueScore : 8 = 0;
			std::uint32_t unknown1 : 8 = 0; // Seems related to num of players
			std::uint32_t unknown2 : 8 = 0; // Seems related to num of players
		};
PACK_POP()

PACK_PUSH(1)
		struct SinglewaveEndRequest
		{
			std::uint32_t type;
			std::uint32_t score;
			std::uint32_t stage;
		};
PACK_POP()

PACK_PUSH(1)
		struct ClanRoomSettings
		{
			std::uint16_t unknown1 : 4 = 0;
			std::uint16_t mode : 5 = 0;
			std::uint16_t map : 7 = 0;
			std::uint16_t unknown = 0;
		};
PACK_POP()

PACK_PUSH(1)
		struct ClientVersion
		{
			std::uint32_t ver4 : 8 = 0;
			std::uint32_t ver3 : 8 = 0;
			std::uint32_t ver2 : 8 = 0;
			std::uint32_t ver1 : 8 = 0; // not used

			bool matches(std::uint32_t v1, std::uint32_t v2, std::uint32_t v3) const
			{
				return ver2 == v1 && ver3 == v2 && ver4 == v3;
			}
		};
PACK_POP()

PACK_PUSH(1)
		struct ClientAuthorization
		{
			std::uint32_t accountID;
			std::uint32_t accountHash;
			std::uint32_t localIp;
			ClientVersion clientVersion;
		};
PACK_POP()

		struct EventMissionPoint
		{
			std::uint32_t eventIndex : 8 = 0;
			std::uint32_t padding : 4 = 0;
			std::uint32_t unk2 : 20 = 0;
		};

		struct CouponItemUseRet
		{
			Common::Enums::CouponItemAction action;
			std::uint32_t newStock;
			Main::Structures::ItemSerialInfo serialInfo;
		};

		struct CouponItemAddRet
		{
			Common::Enums::AddCouponAction action;
			std::uint32_t newStock;
			Main::Structures::ItemSerialInfo serialInfo;
		};

PACK_PUSH(1)
		struct PlayerTeamInfo
		{
			Main::Structures::UniqueId uid;
			std::uint32_t team;	
			char nickname[16]{};
		};
PACK_POP()

PACK_PUSH(1)
		struct ItemRepair
		{
			std::uint32_t newTotalRT{};
			std::uint32_t newTotalMP{};
			std::vector<Main::Structures::ItemSerialInfo> serialInfo;
		};
PACK_POP()

PACK_PUSH(1)
		struct SingleWeaponDurabilityDamage
		{
			Main::Structures::ItemSerialInfo serialInfo{};
			std::uint32_t durabilityToRemove;
		};
PACK_POP()
	}
}
#endif