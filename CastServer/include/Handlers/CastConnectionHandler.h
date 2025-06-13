#ifndef CAST_CONNECTION_HANDLER_H
#define CAST_CONNECTION_HANDLER_H

#include "Network/Session.h"
#include "../Network/CastSession.h"
#include <charconv>
#include <iomanip>

namespace Cast
{
    namespace Handlers
    {
        inline void connectionHandler(const Common::Network::Packet& request, Cast::Network::Session& session,
            Cast::Network::SessionsManager& sessionsManager)
        {
            if (request.getDataSize() == 12) {
                // Packet data extraction
                std::cerr << "Received Packet Data: ";
                const uint8_t* data = request.getData();
                for (size_t i = 0; i < request.getDataSize(); ++i) {
                    std::cerr << std::setw(2) << std::setfill('0') << std::hex << (int)data[i] << " ";
                }
                std::cerr << std::dec << std::endl;

                auto aid = *reinterpret_cast<const std::uint32_t*>(data + 4);
                std::cerr << "Extracted AID: " << aid << std::endl;

                // Set the account ID
                session.setAccountId(aid);

                // IPC Server communication
                asio::io_context ioContext;
                asio::ip::tcp::socket socket(ioContext);
                asio::ip::tcp::resolver resolver(ioContext);

                try {
                    // Connect to IPC server
                    asio::connect(socket, resolver.resolve("127.0.0.1", "13004"));
                    // Send full command as a string
                    std::string command = "get_session_id " + std::to_string(aid) + "\n";
                    asio::write(socket, asio::buffer(command));


                    // Read response with EOF handling
                    asio::streambuf responseBuffer;
                    asio::error_code read_ec;
                    size_t bytes_read = asio::read_until(socket, responseBuffer, "\n", read_ec);

                    std::string response;
                    {
                        std::istream responseStream(&responseBuffer);
                        std::getline(responseStream, response);
                    }

                    // Try to parse response even if we got EOF
                    bool parse_success = false;
                    if (!response.empty()) {
                        if (response.rfind("success ", 0) == 0) {
                            std::uint32_t targetSessionId = 0;
                            auto result = std::from_chars(
                                response.data() + 8,  // Skip "success "
                                response.data() + response.size(),
                                targetSessionId
                            );

                            if (result.ec == std::errc()) {
                                session.setSessionId(targetSessionId);
                                sessionsManager.addSession(&session, targetSessionId);
                                std::cerr << "Set target session ID: " << targetSessionId << std::endl;
                                parse_success = true;
                            }
                        }
                    }

                    // Handle read errors (only if we didn't get a successful parse)
                    if (read_ec && !parse_success) {
                        if (read_ec == asio::error::eof) {
                            std::cerr << "Warning: IPC server closed connection after sending response" << std::endl;
                            if (!parse_success) {
                                throw std::runtime_error("Incomplete response from IPC server");
                            }
                        }
                        else {
                            throw std::runtime_error("Read error: " + read_ec.message());
                        }
                    }

                    if (!parse_success) {
                        throw std::runtime_error("Invalid IPC server response: " + response);
                    }

                    // Clean shutdown
                    asio::error_code ec;
                    socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
                    socket.close(ec);
                }
                catch (const std::exception& e) {
                    std::cerr << "IPC Server communication failed: " << e.what() << std::endl;
                    asio::error_code ec;
                    socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
                    socket.close(ec);
                    session.closeSocket();
                    return;
                }

                // Send response to client
                Common::Network::Packet response;
                response.setTcpHeader(session.getId(), Common::Enums::NO_ENCRYPTION);
                response.setOrder(501);
                response.setExtra(32);
                session.asyncWrite(response);
            }
            else {
                std::cerr << "Received packet with incorrect size" << std::endl;
                session.closeSocket();
            }
        }
    }
}

#endif
