#ifndef HTTP_SESSION_H
#define HTTP_SESSION_H

#include <asio.hpp>
#include "MainSessionManager.h"
#include "boost/beast.hpp"
#include "boost/json.hpp"
#include "boost/beast/ssl/ssl_stream.hpp"
#include <format>       
#include "jwt-cpp/jwt.h"
#include <cstring> 

namespace Main
{
    namespace Network
    {
        namespace http = boost::beast::http;

        // Note: This serves HTTP only, if you need HTTPS you can use NGINX
        class HttpSession : public std::enable_shared_from_this<HttpSession>
        {
            using tcp = boost::asio::ip::tcp;     

        public:
            explicit HttpSession(tcp::socket&& socket, Main::Network::SessionsManager& sm, Main::Classes::RoomsManager& rm,
                Main::Persistence::MainScheduler& ms) 
                : m_socket(std::move(socket))
                , m_sessionsManager{ sm }
                , m_roomsManager{ rm }
                , m_scheduler{ ms }
            {
            }

            void start() { readRequest(); }

        private:
            boost::beast::tcp_stream m_socket;
            boost::beast::flat_buffer m_buffer;
            http::request<http::string_body> m_request;
            Main::Network::SessionsManager& m_sessionsManager;
            Main::Classes::RoomsManager& m_roomsManager;
            Main::Persistence::MainScheduler& m_scheduler;


            bool verifyTokenAndGrade(const std::string& token, const std::string& secret, std::uint32_t requiredGrade)
            {
                try
                {
                    auto decoded_token = jwt::decode(token);
                    auto verifier = jwt::verify().allow_algorithm(jwt::algorithm::hs256{ 
                        "YOUR_SECRET_TOKEN_GOES_HERE"});
                    verifier.verify(decoded_token);
                    auto grade_claim = decoded_token.get_payload_claim("role");
                    int grade = grade_claim.as_integer();
                    if (grade < requiredGrade)
                    {
                        Utils::Logger::log("Attempt to use command with low user grade!", Utils::LogType::Warning, "HttpSession::verifyTokenAndGrade");
                        return false;
                    }
                    return true;
                }
                catch (const std::exception& e)
                {
                    Utils::Logger::log("Error while verifying JWT token: " + std::string{ e.what() }, Utils::LogType::Error, "HttpSession::verifyTokenAndGrade");
                    return false;
                }
            }

            std::optional<std::string> extractToken(const http::request<http::string_body>& request)
            {
                auto authHeader = request[http::field::authorization];
                if (authHeader.empty())
                {
                    return std::nullopt;
                }
                return authHeader.substr(7);  // ignore "bearer"
            }

            bool validateRequest(const http::request<http::string_body>& m_request,
                std::string& responseBody,
                http::status& statusCode,
                boost::json::object& obj,
                std::uint32_t grade)
            {
                try
                {
                    auto tokenOpt = extractToken(m_request);
                    if (!tokenOpt)
                    {
                        responseBody = "Invalid request (invalid JWT token provided)";
                        statusCode = http::status::unauthorized;
                        return false;
                    }

                    const std::string secret = "secret_temp";
                    if (!verifyTokenAndGrade(*tokenOpt, secret, 3))
                    {
                        responseBody = "Invalid request (invalid JWT token provided or too low grade)";
                        statusCode = http::status::unauthorized;
                        return false;
                    }

                    if (m_request.body().empty())
                    {
                        obj = boost::json::object(); 
                        return true;
                    }
                    try
                    {
                        auto body = boost::json::parse(m_request.body());
                        if (!body.is_object())
                        {
                            responseBody = "Invalid request (request body is not JSON object)";
                            statusCode = http::status::bad_request;
                            return false;
                        }

                        obj = body.as_object();
                        return true;
                    }
                    catch (const boost::system::system_error& e)
                    {
                        responseBody = "Invalid request (JSON parsing error: " + std::string(e.what()) + ")";
                        statusCode = http::status::bad_request;
                        return false;
                    }
                }
                catch (const std::exception& e)
                {
                    Utils::Logger::log("Exception in validateRequest: " + std::string(e.what()),
                        Utils::LogType::Error, "HttpSession::validateRequest");

                    responseBody = "Internal server error";
                    statusCode = http::status::internal_server_error;
                    return false;
                }
            }


            void readRequest()
            {
                boost::beast::http::read(m_socket, m_buffer, m_request);
                handleRequest();
                m_socket.close();
            }

            void handleRequest()
            {
                std::string responseBody;
                http::status statusCode = http::status::ok;

                if (m_request.method() == http::verb::post)
                {
                    if (m_request.target() == "/ban")
                    {
                        handleBanCommand(responseBody, statusCode, m_request, false);
                    }
                    else if (m_request.target() == "/cheatban")
                    {
                        handleBanCommand(responseBody, statusCode, m_request, true);
                    }
                    else if (m_request.target() == "/announce")
                    {
                        handleAnnounceCommand(responseBody, statusCode, m_request, false);
                    }
                    else if (m_request.target() == "/tip")
                    {
                        handleAnnounceCommand(responseBody, statusCode, m_request, true);
                    }
                    else if (m_request.target() == "/mute")
                    {
                        handleMute(responseBody, statusCode, m_request);
                    }
                    else if (m_request.target() == "/disconnect")
                    {
                        handleDisconnect(responseBody, statusCode, m_request);
                    }
                    else if (m_request.target() == "/breakroom")
                    {
                        handleBreakroom(responseBody, statusCode, m_request);
                    }
                    else if (m_request.target() == "/changeroomtitle")
                    {
                        handleRoomTitleChange(responseBody, statusCode, m_request);
                    }
                    else if (m_request.target() == "/kick")
                    {
                        handleKick(responseBody, statusCode, m_request);
                    }
                    else if (m_request.target() == "/changehost")
                    {
                        handleHostChange(responseBody, statusCode, m_request);
                    }
                    else if (m_request.target() == "/getroominfo")
                    {
                        handleGetRoomInfo(responseBody, statusCode, m_request);
                    }
                    else if (m_request.target() == "/getplayerinfo")
                    {
                        handleGetPlayerInfo(responseBody, statusCode, m_request);
                    }
                    else if (m_request.target() == "/sendreward")
                    {
                        handleReward(responseBody, statusCode, m_request);
                    }
                    else if (m_request.target() == "/updatecapsuleevent")
                    {
                        handleCapsuleEvent(responseBody, statusCode, m_request);
                    }
                    else if (m_request.target() == "/updateeventmission")
                    {
                        handleEventMission(responseBody, statusCode, m_request);
                    }
                    else if (m_request.target() == "/updatetradesystem")
                    {
                        handleTradeEvent(responseBody, statusCode, m_request);
                    }
                    else if (m_request.target() == "/updateexpmpevent")
                    {
                        handleExpMpBonus(responseBody, statusCode, m_request);
                    }
                    else
                    {
                        responseBody = "Invalid API endpoint";
                        statusCode = http::status::not_found;
                    }
                }
                else if (m_request.method() == http::verb::get)
                {
                    if (m_request.target() == "/online")
                    {
                        getOnlinePlayers(responseBody, statusCode, m_request);
                    }
                    else if (m_request.target() == "/getrooms")
                    {
                        handleGetRooms(responseBody, statusCode, m_request);
                    }
                    else
                    {
                        responseBody = "Invalid API endpoint";
                        statusCode = http::status::not_found;
                    }
                }
                else 
                {
                    responseBody = "Method not allowed";
                    statusCode = http::status::method_not_allowed;
                }

                sendResponse(responseBody, statusCode);
            }

            void sendResponse(const std::string& body, http::status statusCode)
            {
                http::response<http::string_body> response{ statusCode, m_request.version() };

                response.set(http::field::content_type, "text/plain");
                response.body() = body;
                response.prepare_payload();

                std::string origin = m_request[http::field::origin];

                if (m_request.method() == http::verb::options)
                {
                    response.set(http::field::access_control_allow_origin, origin);
                    response.set(http::field::access_control_allow_credentials, "true");
                    response.set(http::field::access_control_allow_methods, "GET, POST, PUT, DELETE, OPTIONS");
                    response.set(http::field::access_control_allow_headers, "Origin, X-Requested-With, Content-Type, Accept, Authorization, Access-Control-Allow-Origin");
                    response.set(http::field::access_control_max_age, "86400");
                }
                else
                {
                    response.set(http::field::access_control_allow_origin, origin);
                    response.set(http::field::access_control_allow_credentials, "true");
                    response.set(http::field::access_control_allow_methods, "GET, POST, PUT, DELETE, OPTIONS");
                    response.set(http::field::access_control_allow_headers, "Origin, X-Requested-With, Content-Type, Accept, Authorization");
                }

                http::write(m_socket, response);
            }


            bool isOriginOk(const std::string& origin)
            {
                const std::vector<std::string> allowedOrigins = {
                    "YOUR_ALLOWED_ORIGINS_GO_HERE",
                };

                std::string normalizedOrigin = origin;
                if (normalizedOrigin.back() == '/') {
                    normalizedOrigin.pop_back();
                }

                return std::find(allowedOrigins.begin(), allowedOrigins.end(), normalizedOrigin) != allowedOrigins.end();
            }


            /*
               Request: POST /ban   OR   /cheatban
               Header:
                - Authorization: Bearer <JWT_TOKEN>
               Body:
                - TargetNickname: (string) The nickname of the player to be banned, OR
                - TargetAccountId

               Response status:
                - 200 OK: success
                - 400 Bad Request: missing required fields or request body is not valid JSON object
                - 401 Unauthorized: invalid JWT token or user grade too low
                - 500: exception thrown in the server
               Response body: contains message (success or error)
           */
            void handleBanCommand(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request, bool isCheatBan)
            {
                try
                {
                    auto tokenOpt = extractToken(m_request);
                    if (!tokenOpt)
                    {
                        responseBody = "Invalid request (invalid JWT token provided)";
                        statusCode = http::status::unauthorized;
                        return;
                    }

                    const std::string secret = "secret_temp";
                    if (!verifyTokenAndGrade(*tokenOpt, secret, 3))
                    {
                        responseBody = "Invalid request (invalid JWT token provided or too low grade)";
                        statusCode = http::status::unauthorized;
                        return;
                    }

                    auto body = boost::json::parse(m_request.body());
                    if (!body.is_object())
                    {
                        responseBody = "Invalid request (request body is not JSON object)";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    auto& obj = body.as_object();

                    if (!(obj.contains("TargetNickname") || obj.contains("TargetAccountId")))
                    {
                        responseBody = "Invalid request (missing required fields)";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    const bool banByAccountId = obj.contains("TargetAccountId");
                    std::string targetIdentifier = banByAccountId ? obj["TargetAccountId"].as_string().c_str() : obj["TargetNickname"].as_string().c_str();

                    uint32_t targetAccountId = 0;
                    if (banByAccountId)
                    {
                        auto [ptr, ec] = std::from_chars(targetIdentifier.data(), targetIdentifier.data() + targetIdentifier.size(), targetAccountId);
                        if (ec != std::errc())
                        {
                            responseBody = "Invalid account ID format";
                            statusCode = http::status::bad_request;
                            return;
                        }
                    }

                    auto targetSession = banByAccountId ?
                        m_sessionsManager.getSessionByAccountId(targetAccountId) :
                        m_sessionsManager.findSessionByName(targetIdentifier.c_str());

                    if (targetSession)
                    {
                        if (isCheatBan)
                        {
                            targetSession->banAccount(9999, "AUTOMATIC_CHEAT_BAN", true);
                            targetSession->closeSocket();
                        }
                        else
                        {
                            m_sessionsManager.removeSession(targetSession->getId());
                            Common::Network::Packet packet;
                            packet.setCommand(73, 0, 1, 0);
                            packet.setData(nullptr, 0);
                            targetSession->asyncWrite(packet);
                        }
                        statusCode = http::status::ok;
                        responseBody = std::format("{} '{}' found online and banned", banByAccountId ? "Account ID" : "Player", targetIdentifier);
                    }
                    else
                    {
                        statusCode = http::status::ok;
                        responseBody = std::format("{} '{}' not online", banByAccountId ? "Account ID" : "Player", targetIdentifier);
                    }
                }
                catch (const std::exception& e)
                {
                    Utils::Logger::log("Error: " + std::string{ e.what() }, Utils::LogType::Error, "HttpSession::handleBanCommand");
                    responseBody = "Server error while processing the request";
                    statusCode = http::status::internal_server_error;
                }
            }

            /*
               Request: POST /mute
               Header:
                - Authorization: Bearer <JWT_TOKEN>
               Body:
                - TargetNickname: (string) The nickname of the player to be muted, OR
                - TargetAccountId

               Response status:
                - 200 OK: success
                - 400 Bad Request: missing required fields or request body is not valid JSON object
                - 401 Unauthorized: invalid JWT token or user grade too low
                - 500: exception thrown in the server
               Response body: contains message (success or error)
           */
            void handleMute(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request)
            {
                try
                {
                    auto tokenOpt = extractToken(m_request);
                    if (!tokenOpt)
                    {
                        responseBody = "Invalid request (invalid JWT token provided)";
                        statusCode = http::status::unauthorized;
                        return;
                    }

                    const std::string secret = "secret_temp";
                    if (!verifyTokenAndGrade(*tokenOpt, secret, 3))
                    {
                        responseBody = "Invalid request (invalid JWT token provided or too low grade)";
                        statusCode = http::status::unauthorized;
                        return;
                    }

                    auto body = boost::json::parse(m_request.body());
                    if (!body.is_object())
                    {
                        responseBody = "Invalid request (request body is not JSON object)";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    auto& obj = body.as_object();

                    if (!(obj.contains("TargetNickname") || obj.contains("TargetAccountId")))
                    {
                        responseBody = "Invalid request (missing required fields)";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    const bool muteByAccountId = obj.contains("TargetAccountId");
                    std::string targetIdentifier = muteByAccountId ? obj["TargetAccountId"].as_string().c_str() : obj["TargetNickname"].as_string().c_str();

                    uint32_t targetAccountId = 0;
                    if (muteByAccountId)
                    {
                        auto [ptr, ec] = std::from_chars(targetIdentifier.data(), targetIdentifier.data() + targetIdentifier.size(), targetAccountId);
                        if (ec != std::errc())
                        {
                            responseBody = "Invalid account ID format";
                            statusCode = http::status::bad_request;
                            return;
                        }
                    }

                    auto targetSession = muteByAccountId ?
                        m_sessionsManager.getSessionByAccountId(targetAccountId) :
                        m_sessionsManager.findSessionByName(targetIdentifier.c_str());

                    if (targetSession)
                    {
                        targetSession->setMute(Main::Structures::MuteInfo{ true, "", "", "" });
                        statusCode = http::status::ok;
                        responseBody = std::format("{} '{}' found online and muted", muteByAccountId ? "Account ID" : "Player", targetIdentifier);
                    }
                    else
                    {
                        statusCode = http::status::ok;
                        responseBody = std::format("{} '{}' not online", muteByAccountId ? "Account ID" : "Player", targetIdentifier);
                    }
                }
                catch (const std::exception& e)
                {
                    Utils::Logger::log("Error: " + std::string{ e.what() }, Utils::LogType::Error, "HttpSession::handleMuteCommand");
                    responseBody = "Server error while processing the request";
                    statusCode = http::status::internal_server_error;
                }
            }


            /*
              Request: POST /announce or POST /tip
              Header:  === IGNORE FOR NOW ===
              - Content-Type: application/json
              - Authorization: Bearer <JWT TOKEN>

              Body:
              - Text: (str) message to announce (max 512 chars)

              Response:
              - 200 OK: Response body will contain message "Announcement: '<text>'"
              - 400 bad req: missing "Text" field or invalid JSON format or text exceeds 512 chars
              - 401 unauthorized: invalid JWT token or too low grade
              - 500: server exception

              Response body: success or error message
            */
            void handleAnnounceCommand(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request, bool isTip)
            {
                try
                {
                    auto tokenOpt = extractToken(m_request);
                    if (!tokenOpt)
                    {
                        responseBody = "Invalid request (invalid JWT token provided)";
                        statusCode = http::status::unauthorized;
                        return;
                    }

                    const std::string secret = "secret_temp";
                    if (!verifyTokenAndGrade(*tokenOpt, secret, 4)) 
                    {
                        responseBody = "Invalid request (invalid JWT token provided or too low grade)";
                        statusCode = http::status::unauthorized;
                        return;
                    }

                    auto body = boost::json::parse(m_request.body());
                    if (!body.is_object())
                    {
                        responseBody = "Invalid request (request body is not a JSON object)";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    auto& obj = body.as_object();

                    if (!obj.contains("Text"))
                    {
                        responseBody = "Invalid request (missing 'Text' field)";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    std::string text = obj["Text"].as_string().c_str();

                    if (text.size() > 512)
                    {
                        responseBody = "Invalid request (Text exceeds the 512 character limit)";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    responseBody = std::format("Announcement: '{}'", text);
                    statusCode = http::status::ok;

                    Common::Network::Packet packet;
                    packet.setTcpHeader(0, Common::Enums::NO_ENCRYPTION);
                    packet.setCommand(402, 0, isTip ? 0xC : 0xA, 0);
                    packet.setData(reinterpret_cast<std::uint8_t*>(const_cast<char*>(text.c_str())), text.size());
                    m_sessionsManager.broadcast(packet);
                }
                catch (const std::exception& e)
                {
                    Utils::Logger::log("Error: " + std::string{ e.what() }, Utils::LogType::Error, "HttpSession::handleAnnounceCommand");
                    responseBody = "Server error while processing the request";
                    statusCode = http::status::internal_server_error;
                }
            }

           
            /*
                 Request: GET /getOnlinePlayers
                 Header:
                 - Content-Type: application/json
                 - Authorization: Bearer <JWT TOKEN>

                 Body: not needed

                 Response:
                 - 200 OK: response body contains JSON array with online players + their session IDs and account IDs, example:
                   {
                       "online_players": [
                           {
                               "nickname": "player1",
                               "session_id": 1234,
                               "account_id": 5678
                           },
                           {
                               "nickname": "player2",
                               "session_id": 9101,
                               "account_id": 1121
                           }
                       ]
                   }

                 - 400 Bad Request: bad JWT token or too low grade
                 - 500: server exception
            */
            void getOnlinePlayers(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request)
            {
                try
                {
                    auto tokenOpt = extractToken(m_request);
                    if (!tokenOpt)
                    {
                        std::cout << "!tokenOpt  \n";
                        responseBody = "Invalid request (invalid JWT token provided)";
                        statusCode = http::status::unauthorized;
                        return;
                    }

                    const std::string secret = "secret_temp";
                    if (!verifyTokenAndGrade(*tokenOpt, secret, 3))
                    {
                        std::cout << "!verifyTokenAndGrade\n";
                        responseBody = "Invalid request (invalid JWT token provided or too low grade)";
                        statusCode = http::status::unauthorized;
                        return;
                    }

                    std::vector<std::tuple<std::string, uint32_t, uint32_t>> onlinePlayers; 
                    for (auto& [sessionID, session] : m_sessionsManager.getAllSessions())
                    {
                        if (session)
                        {
                            std::string playerName = session->getAccountInfo().nickname;
                            uint32_t sessionId = session->getId(); 
                            uint32_t accountId = session->getAccountInfo().accountID;

                            onlinePlayers.push_back({ playerName, sessionId, accountId });
                        }
                    }

                    if (onlinePlayers.empty())
                    {
                        std::cout << "No players online\n";
                        responseBody = "No players currently online";
                        statusCode = http::status::ok;
                    }
                    else
                    {
                        boost::json::object responseJson;
                        responseJson["online_players"] = boost::json::array();

                        for (const auto& player : onlinePlayers)
                        {
                            boost::json::object playerObj;
                            playerObj["nickname"] = std::get<0>(player);
                            playerObj["session_id"] = std::get<1>(player);
                            playerObj["account_id"] = std::get<2>(player); 

                            responseJson["online_players"].as_array().push_back(playerObj);
                        }

                        responseBody = boost::json::serialize(responseJson);
                        statusCode = http::status::ok;
                    }
                }
                catch (const std::exception& e)
                {
                    Utils::Logger::log("Error: " + std::string{ e.what() }, Utils::LogType::Error, "HttpSession::handleGetOnlinePlayers");
                    responseBody = "Server error while processing the request";
                    statusCode = http::status::internal_server_error;
                }
            }

            /*
                Request: POST /disconnect
                Header:
                - Content-Type: application/json
                - Authorization: Bearer <JWT TOKEN>

                Body:
                - TargetNickname: (str), OR
                - TargetAccountId: 

                Response:
                - 200 OK: whether player is disconnected or not
                - 400 Bad Request: missing required fields or (for account ID) invalid format
                - 500: server exception
            */
            void handleDisconnect(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request)
            {
                try
                {
                    boost::json::object obj;
                    if (!validateRequest(m_request, responseBody, statusCode, obj, 3))
                        return;

                    if (!(obj.contains("TargetNickname") || obj.contains("TargetAccountId")))
                    {
                        responseBody = "Invalid request (missing required fields)";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    const bool banByAccountId = obj.contains("TargetAccountId");
                    std::string targetIdentifier = banByAccountId ? obj["TargetAccountId"].as_string().c_str() : obj["TargetNickname"].as_string().c_str();

                    uint32_t targetAccountId = 0;
                    if (banByAccountId)
                    {
                        auto [ptr, ec] = std::from_chars(targetIdentifier.data(), targetIdentifier.data() + targetIdentifier.size(), targetAccountId);
                        if (ec != std::errc())
                        {
                            responseBody = "Invalid account ID format";
                            statusCode = http::status::bad_request;
                            return;
                        }
                    }

                    auto targetSession = banByAccountId ?
                        m_sessionsManager.getSessionByAccountId(targetAccountId) :
                        m_sessionsManager.findSessionByName(targetIdentifier.c_str());

                    if (targetSession)
                    {
                        m_sessionsManager.removeSession(targetSession->getId());
                        Common::Network::Packet disconnectionReq;
                        disconnectionReq.setTcpHeader(targetSession->getId(), Common::Enums::USER_LARGE_ENCRYPTION);
                        disconnectionReq.setCommand(73, 0, 0x23, 0);
                        targetSession->asyncWrite(disconnectionReq);
                        statusCode = http::status::ok;
                        responseBody = std::format("{} '{}' found online and disconnected", banByAccountId ? "Account ID" : "Player", targetIdentifier);
                    }
                    else
                    {
                        statusCode = http::status::ok;
                        responseBody = std::format("{} '{}' not online", banByAccountId ? "Account ID" : "Player", targetIdentifier);
                    }
                }
                catch (const std::exception& e)
                {
                    Utils::Logger::log("Error: " + std::string{ e.what() }, Utils::LogType::Error, "HttpSession::handleDisconnect");
                    responseBody = "Server error while processing the request";
                    statusCode = http::status::internal_server_error;
                }
            }

            /*
                Request: GET /getrooms
                Header:
                - Content-Type: application/json
                - Authorization: Bearer <JWT TOKEN>

                Body: -

                Response:
                - 200 OK: JSON array of all rooms + their info, example follows
                  {
                      "rooms": [
                          {
                              "RoomNumber": 1,
                              "RoomTitle": "Room One",
                              "TotalPlayers": 5,
                              "TotalMaxPlayers": 10,
                              "Password": "password123"
                          },
                          {
                              "RoomNumber": 2,
                              "RoomTitle": "Room Two",
                              "TotalPlayers": 2,
                              "TotalMaxPlayers": 10,
                              "Password": "password456"
                          }
                      ]
                  }
                - If no room available: the message is "No rooms available"
                - 400 Bad Request: bad JSON or bad grade
                - 500: server exception
            */
            void handleGetRooms(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request)
            {
                try
                {
                    boost::json::object obj;
                    if (!validateRequest(m_request, responseBody, statusCode, obj, 3))
                        return;

                    auto rooms = m_roomsManager.getRoomsList();
                    boost::json::object responseJson;
                    boost::json::array roomsArray;

                    if (rooms.empty())
                    {
                        responseBody = "No rooms available";
                        statusCode = http::status::ok;
                    }
                    else
                    {
                        for (const auto& room : rooms)
                        {
                            boost::json::object roomObj;
                            roomObj["RoomNumber"] = room.roomNumber;
                            roomObj["RoomTitle"] = std::string(room.title);
                            roomObj["TotalPlayers"] = room.numPlayers;
                            roomObj["TotalMaxPlayers"] = room.maxPlayers;

                            auto detailedRoom = m_roomsManager.getRoomByNumber(room.roomNumber + 1);
                            std::string password = (detailedRoom && !detailedRoom->getPassword().empty())
                                ? detailedRoom->getPassword()
                                : "";

                            roomObj["Password"] = password;
                            roomsArray.push_back(roomObj);
                        }

                        responseJson["rooms"] = roomsArray;

                        Utils::Logger::log("Final JSON before serialization: " + boost::json::serialize(responseJson),
                            Utils::LogType::Info, "HttpSession::handleGetRooms");

                        responseBody = boost::json::serialize(responseJson);
                        statusCode = http::status::ok;
                    }
                }
                catch (const std::exception& e)
                {
                    Utils::Logger::log("Error during handleGetRooms: " + std::string(e.what()),
                        Utils::LogType::Error, "HttpSession::handleGetRooms");

                    responseBody = "Server error while processing the request";
                    statusCode = http::status::internal_server_error;
                }
            }



            /*
                Request: POST /breakroom
                Header:
                - Content-Type: application/json
                - Authorization: Bearer <JWT TOKEN>

                Body:
                - RoomNumber: (STR) The room number to break (required)

                Response:
                - 200 OK: success message
                - 400 Bad Request: missing fields or bad format
                - 404 Not Found: room not found
                - 500 Internal Server Error: server exception
            */
            void handleBreakroom(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request)
            {
                try
                {
                    boost::json::object obj;
                    if (!validateRequest(m_request, responseBody, statusCode, obj, 3))
                        return;

                    if (!obj.contains("RoomNumber"))
                    {
                        responseBody = "Invalid request (missing required field: RoomNumber)";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    auto data = obj["RoomNumber"].as_string();
                    std::uint32_t roomNumber;
                    auto [ptr, ec] = std::from_chars(data.data(), data.data() + data.size(), roomNumber);
                    if (ec != std::errc())
                    {
                        responseBody = "Invalid room number format";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    auto* room = m_roomsManager.getRoomByNumber(roomNumber);
                    if (room)
                    {
                        m_roomsManager.removeRoom(roomNumber, 35);
                        responseBody = std::format("Room number '{}' has been successfully broken", roomNumber);
                        statusCode = http::status::ok;
                    }
                    else
                    {
                        responseBody = std::format("Room number '{}' not found", roomNumber);
                        statusCode = http::status::not_found;
                    }
                }
                catch (const std::exception& e)
                {
                    Utils::Logger::log("Error: " + std::string{ e.what() }, Utils::LogType::Error, "HttpSession::handleBreakroom");
                    responseBody = "Server error while processing the request";
                    statusCode = http::status::internal_server_error;
                }
            }

            void handleKick(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request)
            {
                try
                {
                    boost::json::object obj;
                    if (!validateRequest(m_request, responseBody, statusCode, obj, 3))
                        return;

                    if (!obj.contains("TargetNickname"))
                    {
                        responseBody = "Invalid request (missing required field: TargetNickname)";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    auto targetNickname = obj["TargetNickname"].as_string().c_str();
                    if (auto targetSession = m_sessionsManager.findSessionByName(targetNickname))
                    {
                        auto roomNum = targetSession->getPlayer().getRoomNumber();
                        if (auto* room = m_roomsManager.getRoomByNumber(roomNum))
                        {
                            if (!room->kickPlayer(targetNickname))
                            {
                                responseBody = std::format("Failed to kick player '{}'.", targetNickname);
                                statusCode = http::status::bad_request;
                            }
                            else
                            {
                                responseBody = std::format("Player '{}' was successfully kicked from the room.", targetNickname);
                                statusCode = http::status::ok;
                                if (room->getAllPlayers().empty())
                                {
                                    m_roomsManager.removeRoom(roomNum);
                                }
                            }
                        }
                        else
                        {
                            responseBody = std::format("Room number '{}' not found", roomNum);
                            statusCode = http::status::not_found;
                        }
                    }
                    else
                    {
                        responseBody = std::format("Session for player '{}' not found", targetNickname);
                        statusCode = http::status::not_found;
                    }
                }
                catch (const std::exception& e)
                {
                    Utils::Logger::log("Error: " + std::string{ e.what() }, Utils::LogType::Error, "HttpSession::handleKick");
                    responseBody = "Server error while processing the request";
                    statusCode = http::status::internal_server_error;
                }
            }

            void handleHostChange(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request)
            {
                try
                {
                    boost::json::object obj;
                    if (!validateRequest(m_request, responseBody, statusCode, obj, 3))
                        return;

                    if (!obj.contains("TargetNickname"))
                    {
                        responseBody = "Invalid request (missing required field: TargetNickname)";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    auto targetNickname = obj["TargetNickname"].as_string().c_str();
                    if (auto targetSession = m_sessionsManager.findSessionByName(targetNickname))
                    {
                        if (auto* room = m_roomsManager.getRoomByNumber(targetSession->getPlayer().getRoomNumber()))
                        {
                            const bool changed = room->changeHostByNickname(targetNickname);
                            if (!changed)
                            {
                                responseBody = std::format("Failed to change host to player '{}'.", targetNickname);
                                statusCode = http::status::bad_request;
                            }
                            else
                            {
                                responseBody = std::format("Player '{}' is now the host of the room.", targetNickname);
                                statusCode = http::status::ok;
                            }
                        }
                        else
                        {
                            responseBody = std::format("Room number '{}' not found", targetSession->getPlayer().getRoomNumber());
                            statusCode = http::status::not_found;
                        }
                    }
                    else
                    {
                        responseBody = std::format("Session for player '{}' not found", targetNickname);
                        statusCode = http::status::not_found;
                    }
                }
                catch (const std::exception& e)
                {
                    Utils::Logger::log("Error: " + std::string{ e.what() }, Utils::LogType::Error, "HttpSession::handleHostChange");
                    responseBody = "Server error while processing the request";
                    statusCode = http::status::internal_server_error;
                }
            }

            void handleRoomTitleChange(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request)
            {
                try
                {
                    boost::json::object obj;
                    if (!validateRequest(m_request, responseBody, statusCode, obj, 3))
                        return;

                    if (!obj.contains("RoomNumber") || !obj.contains("NewTitle"))
                    {
                        responseBody = "Invalid request (missing required field: RoomNumber or NewTitle)";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    auto data = obj["RoomNumber"].as_string();
                    std::uint32_t roomNumber;
                    auto [ptr, ec] = std::from_chars(data.data(), data.data() + data.size(), roomNumber);
                    if (ec != std::errc())
                    {
                        responseBody = "Invalid room number format";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    auto* room = m_roomsManager.getRoomByNumber(roomNumber);
                    if (room)
                    {
                        if (room->hasMatchStarted())
                        {
                            responseBody = "Cannot change room title while the match is ongoing";
                            statusCode = http::status::bad_request;
                            return;
                        }
                        auto newTitle = obj["NewTitle"].as_string();

                        if (newTitle.size() > 29)
                        {
                            newTitle = std::string(newTitle).substr(0, 29); 
                        }

                        Main::Structures::RoomSettingsUpdateTitle updatedSettings;
                        updatedSettings.roomSettingsUpdateBase = room->getRoomSettingsUpdate().roomSettingsUpdateBase;
                        std::memset(updatedSettings.title, 0, sizeof(updatedSettings.title));
                        std::memcpy(updatedSettings.title, newTitle.c_str(), std::min(newTitle.size(), sizeof(updatedSettings.title) - 1));
                        room->updateTitle(std::string{ newTitle });

                        Common::Network::Packet response;
                        response.setCommand(130, 0, 0, room->getRoomSettings().mode);
                        response.setData(reinterpret_cast<std::uint8_t*>(&updatedSettings), sizeof(updatedSettings));
                        room->broadcastToRoom(response);

                        responseBody = std::format("Room number '{}' 's title was successfully changed", roomNumber);
                        statusCode = http::status::ok;
                    }
                    else
                    {
                        responseBody = std::format("Room number '{}' not found", roomNumber);
                        statusCode = http::status::not_found;
                    }
                }
                catch (const std::exception& e)
                {
                    Utils::Logger::log("Error: " + std::string{ e.what() }, Utils::LogType::Error, "HttpSession::handleRoomTitleChange");
                    responseBody = "Server error while processing the request";
                    statusCode = http::status::internal_server_error;
                }
            }

            /*
                Request: POST /getroominfo
                Header:
                - Content-Type: application/json
                - Authorization: Bearer <JWT TOKEN>

                Body:
                - RoomNumber: (STRING) self explainatory

                Response:
                - 200 OK: room details + player details
                - 400 Bad Request: Missing `RoomNumber` field or bad format (must be INTEGER)
                - 404 Not Found: room not found
                - 500 Internal Server Error: server errors

                Example Response (200 OK):
                {
                    "room": {
                        "Host": "PlayerOne",
                        "RoomNumber": 1234,
                        "RoomTitle": "Epic Battle Arena",
                        "TotalPlayers": 5,
                        "TotalMaxPlayers": 10,
                        "Password": "secret123",
                        "Players": [
                            {
                                "PlayerName": "PlayerOne",
                                "Team": "red",
                                "Ping": 32,
                                "SEID": 1001
                            },
                            {
                                "PlayerName": "PlayerTwo",
                                "Team": "blue",
                                "Ping": 45,
                                "SEID": 1002
                            },
                            {
                                "PlayerName": "PlayerThree",
                                "Team": "red",
                                "Ping": 50,
                                "SEID": 1003
                            }
                        ]
                    }
                }
            */
            void handleGetRoomInfo(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request)
            {
                try
                {
                    boost::json::object obj;
                    if (!validateRequest(m_request, responseBody, statusCode, obj, 3))
                        return;

                    if (!obj.contains("RoomNumber"))
                    {
                        responseBody = "Invalid request (missing required field: RoomNumber)";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    auto data = obj["RoomNumber"].as_string();
                    std::uint32_t roomNumber;
                    auto [ptr, ec] = std::from_chars(data.data(), data.data() + data.size(), roomNumber);
                    if (ec != std::errc())
                    {
                        responseBody = "Invalid room number format";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    auto* room = m_roomsManager.getRoomByNumber(roomNumber);

                    if (room)
                    {
                        auto allPlayers = room->getAllPlayersWithSessions();
                        if (allPlayers.empty())
                        {
                            responseBody = "Error: zero players found in this room!";
                            statusCode = http::status::internal_server_error;
                            return;
                        }
                        auto roomInfo = room->getRoomInfo();
                        boost::json::object roomObj;
                        roomObj["Host"] = allPlayers[0].first.playerName;
                        roomObj["RoomNumber"] = room->getRoomNumber();
                        roomObj["RoomTitle"] = room->getRoomTitle();
                        roomObj["TotalPlayers"] = roomInfo.numPlayers;
                        roomObj["TotalMaxPlayers"] = roomInfo.maxPlayers;
                        roomObj["Password"] = room->getPassword();  

                        roomObj["Map"] = Common::Enums::getMapString(room->getActualMap());
                        roomObj["Mode"] = Common::Enums::getGameModeString(roomInfo.mode);
                        roomObj["WeaponRestriction"] = Common::Enums::getWeaponRestrictionString(roomInfo.weaponRestriction);
                        roomObj["HasMatchStarted"] = roomInfo.matchStarted;
                        roomObj["MatchStartTime"] = room->getMatchStartTime();

                        boost::json::array playersArray;

                        for (const auto& currentPlayer : allPlayers)
                        {
                            boost::json::object playerObj;
                            const std::string teamName = (currentPlayer.first.team == 1) ? "red" :
                                (currentPlayer.first.team == 2) ? "blue" :
                                (currentPlayer.first.team == 0) ? "all" :
                                (currentPlayer.first.team == 4) ? "obs" : "unknown";
                            playerObj["PlayerName"] = currentPlayer.first.playerName;
                            playerObj["Team"] = teamName;
                            playerObj["Ping"] = currentPlayer.second->getPlayer().getPing();
                            playerObj["SEID"] = currentPlayer.second->getId();
                            playerObj["PlayerState"] = Common::Enums::playerStateToString(currentPlayer.first.state);
                            playersArray.push_back(playerObj);
                        }

                        roomObj["Players"] = playersArray;

                        boost::json::object responseJson;
                        responseJson["room"] = roomObj;

                        responseBody = boost::json::serialize(responseJson);
                        statusCode = http::status::ok;
                    }
                    else
                    {
                        responseBody = std::format("Room number '{}' not found", roomNumber);
                        statusCode = http::status::not_found;
                    }
                }
                catch (const std::exception& e)
                {
                    Utils::Logger::log("Error: " + std::string{ e.what() }, Utils::LogType::Error, "HttpSession::handleGetRoomInfo");
                    responseBody = "Server error while processing the request";
                    statusCode = http::status::internal_server_error;
                }
            }


            /*
                Request: POST /getplayerinfo
                Header:
                - Content-Type: application/json
                - Authorization: Bearer <JWT TOKEN>

                Body:
                - TargetNickname: (STRING), OR
                - TargetAccountId: (STRING) 

                Response:
                - 200 OK: detailed player info returned
                - 400 Bad Request: missing required fields, or if using "TargetAccountId" bad format
                - 404 Not Found: Player not found
                - 500 Internal Server Error: server exception

                Example Response (200 OK):
                {
                    "player": {
                        "PlayerName": "PlayerOne",
                        "AccountID": 12345,
                        "SessionID": 6789,
                        "Grade": 3,
                        "CurrentRoom": 101,
                        "Level": 15,
                        "MicroPoints": 1000,
                        "RockTotens": 50,
                        "MatchBanned": true,
                        "BanReason": "obvious cheating",
                        "BannedUntil": "permanent",
                        "Muted": true,
                        "MuteReason": "abusive behavior",
                        "MutedBy": "Admin",
                        "MuteExpiration": "2025-03-21T12:00:00Z",
                        "RoomCreationDisabled": true,
                        "RoomCreationDisabledUntil": "2025-04-01T00:00:00Z"
                    }
                }
            */
            void handleGetPlayerInfo(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request)
            {
                try
                {
                    boost::json::object obj;
                    if (!validateRequest(m_request, responseBody, statusCode, obj, 3))
                        return;

                    if (!(obj.contains("TargetNickname") || obj.contains("TargetAccountId")))
                    {
                        responseBody = "Invalid request (missing fields: TargetNickname or TargetAccountId)";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    const bool isAccountId = obj.contains("TargetAccountId");
                    std::string targetIdentifier = isAccountId ? obj["TargetAccountId"].as_string().c_str() : obj["TargetNickname"].as_string().c_str();

                    uint32_t targetAccountId = 0;
                    if (isAccountId)
                    {
                        auto [ptr, ec] = std::from_chars(targetIdentifier.data(), targetIdentifier.data() + targetIdentifier.size(), targetAccountId);
                        if (ec != std::errc())
                        {
                            responseBody = "Invalid account ID format";
                            statusCode = http::status::bad_request;
                            return;
                        }
                    }

                    auto targetSession = isAccountId ?
                        m_sessionsManager.getSessionByAccountId(targetAccountId) :
                        m_sessionsManager.findSessionByName(targetIdentifier.c_str());

                    if (targetSession)
                    {
                        const auto& ainfo = targetSession->getAccountInfo();

                        auto banInfoOpt = m_scheduler.immediatePersist(std::source_location::current(), 
                            &Main::Persistence::PersistentDatabase::getBanInfoByNickname, targetIdentifier);
                        auto muteInfoOpt = m_scheduler.immediatePersist(std::source_location::current(), 
                            &Main::Persistence::PersistentDatabase::getMuteInfoByNickname, targetIdentifier);
                        auto matchBannedOpt = m_scheduler.immediatePersist(std::source_location::current(), 
                            &Main::Persistence::PersistentDatabase::hasBeenMatchBannedByNick, targetIdentifier);
                        auto roomCreationDisabledUntilOpt = m_scheduler.immediatePersist(std::source_location::current(), 
                            &Main::Persistence::PersistentDatabase::getRoomCreationDisabledUntil, targetIdentifier);

                        boost::json::object playerObj;
                        playerObj["PlayerName"] = targetSession->getAccountInfo().nickname;
                        playerObj["AccountID"] = ainfo.accountID;
                        playerObj["SessionID"] = targetSession->getId();
                        playerObj["Grade"] = ainfo.playerGrade;
                        playerObj["CurrentRoom"] = targetSession->getPlayer().getRoomNumber();
                        playerObj["Level"] = ainfo.playerLevel;
                        playerObj["MicroPoints"] = ainfo.microPoints;
                        playerObj["RockTotens"] = ainfo.rockTotens;

                        if (matchBannedOpt && *matchBannedOpt)
                        {
                            playerObj["MatchBanned"] = true;
                            playerObj["BanReason"] = "obvious cheating";
                            playerObj["BannedUntil"] = "permanent";
                        }
                        else if (banInfoOpt && banInfoOpt->isBanned)
                        {
                            playerObj["Banned"] = true;
                            playerObj["BanReason"] = banInfoOpt->reason;
                            playerObj["BannedUntil"] = banInfoOpt->bannedUntil;
                        }
                        if (muteInfoOpt && muteInfoOpt->isMuted)
                        {
                            playerObj["Muted"] = true;
                            playerObj["MuteReason"] = muteInfoOpt->reason;
                            playerObj["MutedBy"] = muteInfoOpt->mutedBy;
                            playerObj["MuteExpiration"] = muteInfoOpt->mutedUntil;
                        }
                        if (roomCreationDisabledUntilOpt && *roomCreationDisabledUntilOpt != "0" && !roomCreationDisabledUntilOpt->empty())
                        {
                            playerObj["RoomCreationDisabled"] = true;
                            playerObj["RoomCreationDisabledUntil"] = *roomCreationDisabledUntilOpt;
                        }
                        boost::json::object responseJson;
                        responseJson["player"] = playerObj;

                        responseBody = boost::json::serialize(responseJson);
                        statusCode = http::status::ok;
                    }
                    else
                    {
                        responseBody = std::format("Player '{}' not found", targetIdentifier);
                        statusCode = http::status::not_found;
                    }
                }
                catch (const std::exception& e)
                {
                    Utils::Logger::log("Error: " + std::string{ e.what() }, Utils::LogType::Error, "HttpSession::handleGetPlayerInfo");
                    responseBody = "Server error while processing the request";
                    statusCode = http::status::internal_server_error;
                }
            }


            // Example:
            /*
               {
                  "Nickname": "Player123",  ---STRING
                  "ItemID": "10042",        ---STRING
                  "Message": "Congratulations on your victory! Here's a special item for you."    --- STRING
               }

               RETURN CODES
               - Invalid JWT: https::UNAUTHORIZED
               - bad body or bad fields, bad itemID (not found) or message > 255 chars: http::BAD_REQUEST
               - player not found neither in DB nor online: BAD_REQUEST ==> you can continue search in other servers
               - success => https::SUCCESS ==> immediately stop sending gift to this user in other servers
            */
            void handleReward(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request)
            {
                try
                {
                    auto tokenOpt = extractToken(m_request);
                    if (!tokenOpt)
                    {
                        responseBody = "Invalid request (invalid JWT token provided)";
                        statusCode = http::status::unauthorized;
                        return;
                    }

                    const std::string secret = "secret_temp";
                    if (!verifyTokenAndGrade(*tokenOpt, secret, 4))
                    {
                        responseBody = "Invalid request (invalid JWT token provided or too low grade)";
                        statusCode = http::status::unauthorized;
                        return;
                    }

                    auto body = boost::json::parse(m_request.body());
                    if (!body.is_object())
                    {
                        responseBody = "Invalid request (request body is not JSON object)";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    auto& obj = body.as_object();

                    if (!(obj.contains("Nickname") && obj.contains("ItemID") && obj.contains("Message")))
                    {
                        responseBody = "Invalid request (missing one or more required fields)";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    std::string nickname = obj["Nickname"].as_string().c_str();
                    std::string itemIdStr = obj["ItemID"].as_string().c_str();
                    std::string message = obj["Message"].as_string().c_str();

                    if (message.size() > Common::Constants::maxMailboxMessage)
                    {
                        responseBody = "Invalid request (message exceeds 255 character limit)";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    uint32_t itemId = 0;
                    auto [ptr, ec] = std::from_chars(itemIdStr.data(), itemIdStr.data() + itemIdStr.size(), itemId);
                    if (ec != std::errc())
                    {
                        responseBody = "Invalid request (invalid ItemID format)";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    if (!Main::CdbUtils::itemExists(itemId))
                    {
                        responseBody = std::format("Invalid request (item ID {} does not exist)", itemId);
                        statusCode = http::status::bad_request;
                        return;
                    }

                    auto targetSession = m_sessionsManager.findSessionByName(nickname.c_str());
                    if (!targetSession)
                    {
                        if (!m_scheduler.immediatePersist(std::source_location::current(), LIFT_MEMBER(storeGiftbox), nickname, message, itemId))
                        {
                            responseBody = std::format("Error: player '{}' not found or their giftbox is full", nickname);
                            statusCode = http::status::bad_request;
                            return;
                        }
                        else
                        {
                            responseBody = std::format("Success: player '{}' is offline, reward stored in giftbox", nickname);
                            statusCode = http::status::ok;
                            return;
                        }
                    }
                    else
                    {
                        if (!targetSession->receiveGift(itemId, message))
                        {
                            responseBody = std::format("Error: player '{}' has not enough space inside giftbox (max 100)", nickname);
                            statusCode = http::status::bad_request;
                            return;
                        }
                        else
                        {
                            responseBody = std::format("Success: player '{}' is online and received the reward", nickname);
                            statusCode = http::status::ok;
                            return;
                        }
                    }
                }
                catch (const std::exception& e)
                {
                    Utils::Logger::log("Error: " + std::string{ e.what() }, Utils::LogType::Error, "HttpSession::handleReward");
                    responseBody = "Server error while processing the request";
                    statusCode = http::status::internal_server_error;
                }
            }



            // Example:
              /*
                  {
                      "newCapsuleEventStartDate": 1651990000,
                      "newCapsuleEventEndDate": 1652000000,
                      "newMpPrice" : 500,
                      "newRtPrice" : 800
                  }
              */
            void handleCapsuleEvent(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request)
            {
                try
                {
                    auto tokenOpt = extractToken(m_request);
                    if (!tokenOpt)
                    {
                        responseBody = "Invalid request (invalid JWT token provided)";
                        statusCode = http::status::unauthorized;
                        return;
                    }

                    const std::string secret = "secret_temp";
                    if (!verifyTokenAndGrade(*tokenOpt, secret, 4))
                    {
                        responseBody = "Invalid request (invalid JWT token provided or too low grade)";
                        statusCode = http::status::unauthorized;
                        return;
                    }

                    auto body = boost::json::parse(m_request.body());
                    if (!body.is_object())
                    {
                        responseBody = "Invalid request (request body is not a JSON object)";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    auto& obj = body.as_object();

                    if (!obj.contains("newCapsuleEventStartDate") ||
                        !obj.contains("newCapsuleEventEndDate") ||
                        !obj.contains("newMpPrice") ||
                        !obj.contains("newRtPrice"))
                    {
                        responseBody = "Invalid request (missing required fields: newCapsuleEventStartDate, newCapsuleEventEndDate, newMpPrice, newRtPrice)";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    std::uint32_t newCapsuleEventStartDate = obj["newCapsuleEventStartDate"].as_uint64();
                    std::uint32_t newCapsuleEventEndDate = obj["newCapsuleEventEndDate"].as_uint64();
                    std::uint32_t newMpPrice = obj["newMpPrice"].as_uint64();
                    std::uint32_t newRtPrice = obj["newRtPrice"].as_uint64();

                    if (newMpPrice < 0 || newMpPrice > 2000)
                    {
                        responseBody = "Invalid value for newMpPrice (must be between 0 and 2000)";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    if (newRtPrice < 0 || newRtPrice > 800)
                    {
                        responseBody = "Invalid value for newRtPrice (must be between 0 and 800)";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    Main::Structures::CapsuleListDatabase capsuleEvent;
                    capsuleEvent.saleEventStartDate = newCapsuleEventStartDate;
                    capsuleEvent.saleEventEndDate = newCapsuleEventEndDate;
                    capsuleEvent.newMpPrice = newMpPrice;
                    capsuleEvent.newRtPrice = newRtPrice;

                    auto updateResultOpt = m_scheduler.immediatePersist(std::source_location::current(),
                        &Main::Persistence::PersistentDatabase::updateCapsuleEvent, capsuleEvent);

                    if (updateResultOpt)
                    {
                        responseBody = "Capsule event info updated successfully";
                        statusCode = http::status::ok;
                    }
                    else
                    {
                        responseBody = "Failed to update capsule event info";
                        statusCode = http::status::bad_request;
                    }
                }
                catch (const std::exception& e)
                {
                    Utils::Logger::log("Error: " + std::string{ e.what() }, Utils::LogType::Error, "HttpSession::handleCapsuleEvent");
                    responseBody = "Server error while processing the request";
                    statusCode = http::status::internal_server_error;
                }
            }


            template<typename F>
            void handleGenericEventInfoUpdate(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request,
                const std::string& startFieldName, const std::string& endFieldName, F updater)
            {
                try
                {
                    auto tokenOpt = extractToken(m_request);
                    if (!tokenOpt)
                    {
                        responseBody = "Invalid request (invalid JWT token provided)";
                        statusCode = http::status::unauthorized;
                        return;
                    }

                    const std::string secret = "secret_temp";
                    if (!verifyTokenAndGrade(*tokenOpt, secret, 4))
                    {
                        responseBody = "Invalid request (invalid JWT token provided or too low grade)";
                        statusCode = http::status::unauthorized;
                        return;
                    }

                    auto body = boost::json::parse(m_request.body());
                    if (!body.is_object())
                    {
                        responseBody = "Invalid request (request body is not a JSON object)";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    const auto& obj = body.as_object();

                    if (!obj.contains(startFieldName) || !obj.contains(endFieldName))
                    {
                        responseBody = "Invalid request (missing required fields)";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    Main::Structures::EventMissionInfo info;
                    info.startDate = obj.at(startFieldName).as_uint64();
                    info.endDate = obj.at(endFieldName).as_uint64();

                    if (updater(info))
                    {
                        responseBody = "Event info updated successfully";
                        statusCode = http::status::ok;
                    }
                    else
                    {
                        responseBody = "Failed to update event info";
                        statusCode = http::status::bad_request;
                    }
                }
                catch (const std::exception& e)
                {
                    Utils::Logger::log("Error: " + std::string{ e.what() }, Utils::LogType::Error, "HttpSession::handleGenericEventInfoUpdate");
                    responseBody = "Server error while processing the request";
                    statusCode = http::status::internal_server_error;
                }
            }

            void handleTradeEvent(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request)
            {
                handleGenericEventInfoUpdate(
                    responseBody,
                    statusCode,
                    m_request,
                    "newTradeEventStartDate",
                    "newTradeEventEndDate",
                    [this](const Main::Structures::EventMissionInfo& info)
                    {
                        return m_scheduler.immediatePersist(std::source_location::current(),
                        &Main::Persistence::PersistentDatabase::updateTradeEventsInfo, info);
                    }
                );
            }

            void handleEventMission(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request)
            {
                handleGenericEventInfoUpdate(
                    responseBody,
                    statusCode,
                    m_request,
                    "newEventMissionStartDate",
                    "newEventMissionEndDate",
                    [this](const Main::Structures::EventMissionInfo& info)
                    {
                        return m_scheduler.immediatePersist(std::source_location::current(),
                        &Main::Persistence::PersistentDatabase::updateEventMissionsInfo, info);
                    }
                );
            }


            // Example:
             /*
                 {
                     "newExpMpBonusStartDate": 1651990000,
                     "newExpMpBonusEndDate": 1652000000,
                     "newExpBonusPercent": 20,
                     "newMpBonusPercent": 15
                 }
             */
            void handleExpMpBonus(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request)
            {
                try
                {
                    auto tokenOpt = extractToken(m_request);
                    if (!tokenOpt)
                    {
                        responseBody = "Invalid request (invalid JWT token provided)";
                        statusCode = http::status::unauthorized;
                        return;
                    }

                    const std::string secret = "secret_temp";
                    if (!verifyTokenAndGrade(*tokenOpt, secret, 4))
                    {
                        responseBody = "Invalid request (invalid JWT token provided or too low grade)";
                        statusCode = http::status::unauthorized;
                        return;
                    }

                    auto body = boost::json::parse(m_request.body());
                    if (!body.is_object())
                    {
                        responseBody = "Invalid request (request body is not a JSON object)";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    auto& obj = body.as_object();

                    if (!obj.contains("newExpMpBonusStartDate") ||
                        !obj.contains("newExpMpBonusEndDate") ||
                        !obj.contains("newExpBonusPercent") ||
                        !obj.contains("newMpBonusPercent"))
                    {
                        responseBody = "Invalid request (missing required fields: newExpMpBonusStartDate, newExpMpBonusEndDate, newExpBonusPercent, newMpBonusPercent)";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    std::uint32_t newExpMpBonusStartDate = obj["newExpMpBonusStartDate"].as_uint64();
                    std::uint32_t newExpMpBonusEndDate = obj["newExpMpBonusEndDate"].as_uint64();
                    std::uint32_t newExpBonusPercent = obj["newExpBonusPercent"].as_uint64();
                    std::uint32_t newMpBonusPercent = obj["newMpBonusPercent"].as_uint64();

                    if (newExpBonusPercent < 5 || newExpBonusPercent > 100)
                    {
                        responseBody = "Invalid value for newExpBonusPercent (must be between 5 and 100)";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    if (newMpBonusPercent < 5 || newMpBonusPercent > 100)
                    {
                        responseBody = "Invalid value for newMpBonusPercent (must be between 5 and 100)";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    Main::Structures::ExpMpBonusInfo expMpBonusInfo;
                    expMpBonusInfo.startDate = newExpMpBonusStartDate;
                    expMpBonusInfo.endDate = newExpMpBonusEndDate;
                    expMpBonusInfo.expBonusPercent = newExpBonusPercent;
                    expMpBonusInfo.mpBonusPercent = newMpBonusPercent;

                    auto updateResultOpt = m_scheduler.immediatePersist(std::source_location::current(),
                        &Main::Persistence::PersistentDatabase::updateExpMpBonusInfo, expMpBonusInfo);

                    if (updateResultOpt)
                    {
                        responseBody = "Experience and MP bonus info updated successfully";
                        statusCode = http::status::ok;
                    }
                    else
                    {
                        responseBody = "Failed to update experience and MP bonus info";
                        statusCode = http::status::bad_request;
                    }
                }
                catch (const std::exception& e)
                {
                    Utils::Logger::log("Error: " + std::string{ e.what() }, Utils::LogType::Error, "HttpSession::handleExpMpBonus");
                    responseBody = "Server error while processing the request";
                    statusCode = http::status::internal_server_error;
                }
            }

        };
    }
}

#endif