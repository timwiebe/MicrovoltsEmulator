#ifndef MAIN_AUTH_SESSION_H
#define MAIN_AUTH_SESSION_H

#include <asio.hpp>
#include <iostream> // debug, remove later
#include "MainSessionManager.h"

// Used for Main<=>Auth IPC communication
// (AuthServer acts as a client here, retrieves certain info, and MainServer responds to its requests)
namespace Main 
{
    namespace Network 
    {
        class AuthSession : public std::enable_shared_from_this<AuthSession> 
        {
        private:
            asio::ip::tcp::socket m_socket;
            Main::Network::SessionsManager& m_sessionsManager;

        public:
            AuthSession(asio::ip::tcp::socket socket, Main::Network::SessionsManager& sessionsManager)
                : m_socket(std::move(socket)) 
                , m_sessionsManager{ sessionsManager }
            {
            }

            void start() 
            {
                auto self(shared_from_this());
                auto requestBuffer = std::make_shared<asio::streambuf>();
                asio::async_read_until(m_socket, *requestBuffer, "\n",
                    [this, self, requestBuffer](asio::error_code ec, std::size_t length) 
                    {
                        if (!ec) 
                        {
                            std::istream requestStream(requestBuffer.get());
                            std::string request;
                            std::getline(requestStream, request);

                            if (request == "get_player_count")
                            {
                                sendPlayerCount();
                            }
                            else if (request.find("is_player_online") == 0)
                            {
                                disconnectPlayerIfOnline(request.substr(17));
                            }
                            else if (request.find("get_session_id") == 0)
                            {
                                std::string arg = request.substr(15);
                                std::uint32_t accountId = 0;
                                auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), accountId, 10);

                                if (ec == std::errc{} && ptr == arg.data() + arg.size())
                                {
                                    handleGetSessionId(accountId);
                                }
                                else
                                {
                                    std::cerr << "Invalid account ID format for get_session_id: " << arg << "\n";
                                    asio::async_write(m_socket, asio::buffer(std::string("fail\n")), [](auto, auto) {});
                                }
                            }
                        }
                    });
            }

        private:
            void sendPlayerCount() 
            {
                auto self(shared_from_this());
                const std::string totalPlayersOnline = std::to_string(m_sessionsManager.getTotalSessions()) + "\n";
                asio::async_write(m_socket, asio::buffer(totalPlayersOnline),
                    [this, self](asio::error_code ec, std::size_t length) 
                    {
                    });
            }

            void disconnectPlayerIfOnline(const std::string& accountIDStr)
            {
                auto self(shared_from_this());
                auto response = std::make_shared<std::string>("not_removed");

                std::uint32_t accountID;
                auto [ptr, ec] = std::from_chars(accountIDStr.data(),
                    accountIDStr.data() + accountIDStr.size(),
                    accountID, 10);

                if (ec != std::errc{} || ptr != accountIDStr.data() + accountIDStr.size())
                {
                    auto errorMsg = std::make_shared<std::string>("Invalid account ID\n");
                    asio::async_write(m_socket, asio::buffer(*errorMsg),
                        [this, self, errorMsg](asio::error_code ec, std::size_t length)
                        {
                            if (ec) { /* handle error */ }
                        });
                    return;
                }

                if (auto targetSession = m_sessionsManager.getSessionByAccountId(accountID))
                {
                    Common::Network::Packet pkt;
                    pkt.setTcpHeader(targetSession->getId(), Common::Enums::USER_LARGE_ENCRYPTION);
                    pkt.setOrder(73);
                    pkt.setExtra(5);

                    targetSession->asyncWrite(pkt); // Send disconnect first
                    m_sessionsManager.removeSession(targetSession->getId()); // Then remove
                    *response = "removed";
                }

                asio::async_write(m_socket, asio::buffer(*response),
                    [this, self, response](asio::error_code ec, std::size_t length)
                    {
                        if (ec) { /* handle error */ }
                    });
            }
            void handleGetSessionId(std::uint32_t accountId)
            {
                auto self(shared_from_this());

                std::cerr << "AuthServer requested SessionID for AID: " << accountId << std::endl;

                auto response = std::make_shared<std::string>();

                if (auto targetSession = m_sessionsManager.getSessionByAccountId(accountId); targetSession)
                {
                    std::uint32_t sessionId = targetSession->getId();
                    response->append("success ");
                    response->append(std::to_string(sessionId));
                    response->append("\n");
                }
                else
                {
                    response->append("fail\n");
                }

                asio::async_write(m_socket, asio::buffer(*response),
                    [this, self, response](asio::error_code ec, std::size_t length)
                    {
                        if (ec)
                        {
                            std::cerr << "Error sending session ID response: " << ec.message() << "\n";
                        }
                    });
            }
        };
    }
}

#endif
