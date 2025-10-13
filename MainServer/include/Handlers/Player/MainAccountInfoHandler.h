#ifndef ACCOUNT_INFO_HANDLER_MAIN_H
#define ACCOUNT_INFO_HANDLER_MAIN_H

#include "Network/Session.h"
#include "Network/Packet.h"
#include "../../MainEnums.h"
#include "../../Structures/AccountInfo/MainAccountInfo.h"
#include "../../../include/Network/MainSession.h"
#include "../../../include/Network/MainSessionManager.h"
#include "../../../include/Structures/AccountInfo/MuteInfo.h"
#include <source_location>

namespace Main
{
	namespace Handlers
	{
		inline void handleAccountInformation(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
			Main::Network::SessionsManager& sessionsManager, Main::Persistence::MainScheduler& scheduler, Main::Structures::AccountInfo& accountInfo, 
			std::uint64_t timeSinceLastServerRestart, std::uint32_t serverId, std::uint32_t extra = 1)
		{
			START_BENCHMARK

			if (extra == 59)
			{
				session->sendAccountInfoConfirmation();
				return;
			}

			if (auto foundSession = sessionsManager.getSessionByAccountId(accountInfo.accountID))
			{ // the session is already online, disonnect it 
				foundSession->closeSocket();
			}

			accountInfo.uniqueId.session = session->getId();
			accountInfo.uniqueId.server = serverId;
			accountInfo.serverTime = accountInfo.getUtcTimeMs() - timeSinceLastServerRestart;
			session->setAccountInfo(accountInfo);
			sessionsManager.addSession(session);

			const auto friends = scheduler.immediatePersist(std::source_location::current(),
				&Main::Persistence::PersistentDatabase::loadFriends, accountInfo.accountID);
			session->setFriendList(friends);
			for (const auto& currentFriend : friends)
			{
				if (auto targetSession = sessionsManager.getSessionByAccountId(currentFriend.targetAccountId))
				{ // This friend is online. Notify them that we're online
					session->updateFriendSession(targetSession);
					targetSession->updateFriendSession(sessionsManager.getSessionBySessionId(session->getId()));
					targetSession->logFriend(Main::Enums::FriendLogType::LOGIN, accountInfo.accountID);
				}
			}

			
			session->setBlockedPlayers(scheduler.immediatePersist(std::source_location::current(),
				&Main::Persistence::PersistentDatabase::loadBlockedPlayers, accountInfo.accountID));
			session->setMute(scheduler.immediatePersist(std::source_location::current(), &Main::Persistence::PersistentDatabase::isMuted, accountInfo.accountID));
			if (scheduler.immediatePersist(std::source_location::current(), &Main::Persistence::PersistentDatabase::isRoomCreationDisabled, accountInfo.accountID))
			{
				session->setRoomCreationDisabled();
			}
			if (scheduler.immediatePersist(std::source_location::current(), &Main::Persistence::PersistentDatabase::isVotekickDisabled, accountInfo.accountID))
			{
				session->setVotekickDisabled();
			}
			auto [sentMailboxes, receivedMailboxes] = scheduler.immediatePersist(std::source_location::current(), 
				&Main::Persistence::PersistentDatabase::loadMailboxes, accountInfo.accountID);
			session->setMailbox(sentMailboxes, true);
			session->setMailbox(receivedMailboxes, false);
			session->setReceivedGiftboxes(scheduler.immediatePersist(std::source_location::current(), 
				&Main::Persistence::PersistentDatabase::loadReceivedGiftboxes, accountInfo.accountID));
			session->setLatestWeeklyRewardDate(scheduler.immediatePersist(std::source_location::current(),
				&Main::Persistence::PersistentDatabase::getLatestRewardDayFor,
				"LatestWeeklyRewardDay", accountInfo.accountID));
			session->setLatestMonthlyRewardDate(scheduler.immediatePersist(std::source_location::current(), 
				&Main::Persistence::PersistentDatabase::getLatestRewardDayFor,
				"LatestMonthlyRewardDay", accountInfo.accountID));
			session->sendUnreadMailboxes();
			
			END_BENCHMARK(handleAccountInformation, session)
		}
	}
}

#endif
