

#include <unordered_map>
#include <functional>
#include "../../include/Network/MainSessionManager.h"
#include "../../include/Handlers/Room/RoomLeaveHandler.h"
#include "../../include/Handlers/Clan/ClanRoomLeaveHandler.h"
#include "../../include/Classes/Room.h"
#include "../../include/Handlers/Room/MatchLeaveHandler.h"

namespace Main
{
	namespace Network
	{
		void SessionsManager::addSession(std::shared_ptr<Main::Network::Session> session)
		{
			m_sessionsBySessionId.emplace(session->getId(), session);
			m_sessionsVector.push_back(std::move(session));
		}

		void SessionsManager::removeSession(std::size_t sessionId)
		{
			Common::Network::Packet packet;

			auto it = m_sessionsBySessionId.find(sessionId);
			if (it == m_sessionsBySessionId.end() || !it->second)
			{
				std::cerr << "[removeSession] Session not found or is null: " << sessionId << "\n";
				return;
			}

			auto session = it->second;

			std::vector<std::weak_ptr<Session>> validFriendSessions;
			for (const auto& [_, friendSession] : session->getFriendSessions())
			{
				if (friendSession.lock())
					validFriendSessions.push_back(friendSession);
			}

			for (const auto& weakFriendSession : validFriendSessions)
			{
				if (auto friendSession = weakFriendSession.lock())
				{
					friendSession->logFriend(Main::Enums::FriendLogType::LOGOUT, session->getAccountInfo().accountID);
					friendSession->updateFriendSession(session, true);
				}
			}

			// 1. remove the player from a normal room/clan room, remove them from the match too
			if (session->getPlayer().getRoomNumber())
			{
				if (session->getPlayer().isInMatch())
				{
					packet.setTcpHeader(session->getId(), Common::Enums::NO_ENCRYPTION);
					packet.setCommand(256, 0, 0, 0);
					Main::Handlers::handleMatchLeave(packet, session, *this, *roomsManager, *m_clansManager);
				}

				packet.setCommand(141, 0, 0, 0);
				Main::Handlers::handleRoomLeave(packet, session, *this, *roomsManager, *m_clansManager, session->getAccountInfo().uniqueId);
			}

			// 2. remove them from the party
			//m_clansManager->tryRemovePlayerFromParty(session->getAccountInfo().clanId, sessionId);

			if (session->getPlayer().getClanRoomNumber())
			{
				Common::Network::Packet leavePartyReq;
				leavePartyReq.setCommand(111, 0, 0, 0);
				Main::Handlers::handlePartyRoomLeave(leavePartyReq, session, *m_clansManager, *roomsManager);
			}

			it->second->persistNow();
			m_sessionsBySessionId.erase(it);

			m_sessionsVector.erase(
				std::remove_if(m_sessionsVector.begin(), m_sessionsVector.end(),
					[&](const std::weak_ptr<Session>& weakSession) {
						auto session = weakSession.lock();
						return !session || session->getSessionId() == sessionId;
					}),
				m_sessionsVector.end()
			);
		}

		void SessionsManager::setRoomsManager(Main::Classes::RoomsManager* rmManager)
		{
			roomsManager = rmManager;
		}

		void SessionsManager::setClansManager(Main::Classes::ClansManager* clansManager)
		{
			m_clansManager = clansManager;
		}

		const std::unordered_map<std::uint64_t, std::shared_ptr<Main::Network::Session>>& SessionsManager::getAllSessions() const
		{
			return m_sessionsBySessionId;
		}

		std::unordered_map<std::uint64_t, std::shared_ptr<Main::Network::Session>>& SessionsManager::getAllSessions()
		{
			return m_sessionsBySessionId;
		}

		std::vector<std::shared_ptr<Main::Network::Session>> SessionsManager::getAllSessionsVec() const
		{
			return m_sessionsVector;
		}

		void SessionsManager::broadcast(const Common::Network::Packet& message) const
		{
			for (auto& currentSession : m_sessionsVector)
			{
				currentSession->asyncWrite(message);
			}
		}

		void SessionsManager::broadcastExceptSelf(std::size_t selfSessionId, const Common::Network::Packet& message) const
		{
			for (auto& currentSession : m_sessionsVector)
			{
				if (currentSession->getId() == selfSessionId) continue;
				currentSession->asyncWrite(message);
			}
		}

		void SessionsManager::broadcastToLobbyExceptSelf(std::size_t selfSessionId, const Common::Network::Packet& message) const
		{
			for (auto& currentSession : m_sessionsVector)
			{
				if (selfSessionId == currentSession->getId() || !currentSession->getPlayer().isInLobby()) continue; 
				currentSession->asyncWrite(message);
			}
		}

		void SessionsManager::broadcastToClan(std::uint64_t selfSessionId, const Common::Network::Packet& message) const
		{
			const auto& selfAccountInfo = m_sessionsBySessionId.at(selfSessionId)->getAccountInfo();
			for (auto& currentSession : m_sessionsVector)
			{
				if (selfSessionId == currentSession->getId()) continue;
				if (selfAccountInfo.clanId >= 8 && selfAccountInfo.clanId == currentSession->getAccountInfo().clanId)
				{
					currentSession->asyncWrite(message);
				}
			}
		}

		std::shared_ptr<Main::Network::Session> SessionsManager::findSessionByName(const char* nickname)
		{
			auto it = std::ranges::find_if(m_sessionsVector, [&](const auto& currentSession) {
				return std::strcmp(currentSession->getPlayer().getPlayerName(), nickname) == 0;
				});
			return it != m_sessionsVector.end() ? *it : nullptr;
		}

		std::shared_ptr<Main::Network::Session> SessionsManager::getSessionByAccountId(std::uint32_t aid)
		{
			for (const auto& currentSession : m_sessionsVector)
			{
				if (currentSession->getAccountInfo().accountID == aid)
				{
					return currentSession;
				}
			}
			return nullptr;
		}

		std::shared_ptr<Main::Network::Session> SessionsManager::getSessionBySessionId(std::size_t sessionId)
		{
			auto it = m_sessionsBySessionId.find(sessionId);
			if (it != m_sessionsBySessionId.end())
			{
				return it->second;
			}
			return nullptr;
		}

		std::uint32_t SessionsManager::getTotalSessions() const
		{
			return m_sessionsBySessionId.size();
		}

		bool SessionsManager::sendTo(std::size_t sessionId, const Common::Network::Packet& packet)
		{
			if (m_sessionsBySessionId.contains(sessionId))
			{
				m_sessionsBySessionId[sessionId]->asyncWrite(packet);
				return true;
			}
			return false;
		}

		Common::Network::Packet SessionsManager::prepareMessage(const std::string& message) const
		{
			Common::Network::Packet response;
			response.setOrder(316);
			response.setExtra(1);
			std::string m_confirmationMessage{ std::string(16, '0') };
			m_confirmationMessage += message;
			response.setData(reinterpret_cast<std::uint8_t*>(m_confirmationMessage.data()), m_confirmationMessage.size());
			return response;
		}

		void SessionsManager::broadcastMessage(const std::string& message) const
		{
			auto response = prepareMessage(message);
			for (auto& currentSession : m_sessionsVector)
			{
				currentSession->asyncWrite(response);
			}
		}

		void SessionsManager::broadcastMessageExceptSelf(std::size_t selfSessionId, const std::string& message) const
		{
			auto response = prepareMessage(message);
			for (auto& currentSession : m_sessionsVector)
			{
				if (currentSession->getId() == selfSessionId) continue;
				currentSession->asyncWrite(response);
			}
		}

		void SessionsManager::broadcastMessageToLobby(const std::string& message) const
		{
			auto response = prepareMessage(message);
			for (auto& currentSession : m_sessionsVector)
			{
				if (!currentSession->getPlayer().getRoomNumber())
				{
					currentSession->sendMessage(message);
				}
			}
		}

	};
}
