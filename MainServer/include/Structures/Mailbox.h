#ifndef MAILBOX_STRUCTURE_H
#define MAILBOX_STRUCTURE_H

#include "AccountInfo/MainAccountUniqueId.h"
#include "Macros.h"

namespace Main
{
	namespace Structures
	{
PACK_PUSH(1)
        struct Mailbox
        {
            std::uint32_t accountId{};
            std::uint32_t timestamp{};
            std::uint32_t hasBeenRead{};
            char nickname[16]{};
            char message[256]{};
        };
PACK_POP()

PACK_PUSH(1)
        struct Giftbox
        {
            std::uint32_t accountId{};
            std::uint32_t timestamp{}; 
            std::uint32_t id{};
            std::uint32_t id1{};
            std::uint32_t id2{};
            char nickname[16]{};
            char message[256]{};
        };
PACK_POP()

PACK_PUSH(1)
        struct Giftbox2
        {
            std::uint32_t accountId{};
            std::uint32_t timestamp{}; 
            Main::Structures::ItemId itemId;
            std::uint32_t expiration{}; // correct
            Main::Structures::ItemSerialInfo serialInfo{};

            Giftbox2(const Giftbox& giftbox)
                : itemId{ giftbox.id }, accountId{ giftbox.accountId }, timestamp{ giftbox.timestamp }
            {
                serialInfo.itemCreationDate = static_cast<__time32_t>(std::time(0));
                serialInfo.itemOrigin = 8; // Main::Enums::ItemFrom::GIFT;
            }
        };
PACK_POP()
    }
}
	

#endif