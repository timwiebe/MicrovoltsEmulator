
#include <unordered_map>
#include <functional>
#include "../../include/Network/CastSession.h"
#include "../../include/Network/SessionsManager.h"
#include <vector>
#include "../../../MainServer/include/Structures/AccountInfo/MainAccountInfo.h"
#include <Utils/Logger.h>

namespace Cast
{
	namespace Network
	{
		void SessionsManager::setRoomsManager(Cast::Classes::RoomsManager* roomsManager)
		{
			m_roomsManager = roomsManager;
		}

		void SessionsManager::addSession(std::shared_ptr<Cast::Network::Session> session, std::size_t sessionId)
		{
			if (!m_sessionsBySessionId.contains(sessionId))
			{
				m_sessionsBySessionId.emplace(sessionId, session);
				session->setSessionId(sessionId);
			}
		}

		void SessionsManager::removeSession(std::size_t sessionId)
		{
			// The player is inside a room
			if (m_sessionsBySessionId.contains(sessionId))
			{
				m_sessionsBySessionId[sessionId]->setIsInMatch(false);
				m_sessionsBySessionId.erase(sessionId);
			}
		}

		std::shared_ptr<Cast::Network::Session> SessionsManager::getSession(std::size_t sessionId)
		{
			if (m_sessionsBySessionId.contains(sessionId))
			{
				return m_sessionsBySessionId[sessionId];
			}
			return nullptr;
		}
	};
}