#ifndef MAILBOX_HANDLERS_H
#define MAILBOX_HANDLERS_H

#include "Network/Packet.h"
#include "../../../Network/MainSession.h"
#include "../../../../include/MainEnums.h"
#include "../../../Network/MainSessionManager.h"
#include "Utils/Constants.h"
#include <cstring> 

namespace Main
{
	namespace Handlers
	{
		inline void handleReadMailbox(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
			Main::Persistence::MainScheduler& scheduler,
			std::uint32_t timestamp)
		{
			START_BENCHMARK
			if (request.getExtra() == Main::Enums::MailboxExtra::MAILBOX_HAS_BEEN_READ)
			{
				scheduler.addRepetitiveCallback(std::source_location::current(), 
					session->getAccountInfo().accountID, &Main::Persistence::PersistentDatabase::updateReadMailbox, session->getAccountInfo().accountID, timestamp);
				session->asyncWrite(request);
			}
			END_BENCHMARK(handleReadMailbox, session)
		}

		inline void handleMailboxCommunication(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
			Main::Network::SessionsManager& sessionsManager,
			const Main::ClientData::MailboxMessage& mailboxData)
		{
			START_BENCHMARK
			if (request.getOption() == 2) // option seems to always be 2
			{
				auto targetSession = sessionsManager.findSessionByName(mailboxData.nickname);
				if (!targetSession)
				{
					Main::Structures::Mailbox mailbox{ 0, static_cast<__time32_t>(std::time(0)) };
					std::memcpy(mailbox.nickname, mailboxData.nickname, Common::Constants::maxNicknameSize);
					std::memcpy(mailbox.message, mailboxData.message, request.getDataSize() - Common::Constants::maxNicknameSize);
					session->sendOfflineMailbox(mailbox);
				}
				else
				{
					Main::Structures::Mailbox mailbox{ targetSession->getAccountInfo().accountID, static_cast<__time32_t>(std::time(0)) };
					std::memcpy(mailbox.nickname, session->getAccountInfo().nickname, Common::Constants::maxNicknameSize);
					std::memcpy(mailbox.message, mailboxData.message, request.getDataSize() - Common::Constants::maxNicknameSize);
					session->sendOnlineMailbox(targetSession, mailbox);
				}
			}
			END_BENCHMARK(handleMailboxCommunication, session)
		}
	}
}

#endif