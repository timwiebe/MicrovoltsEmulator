#include "../include/CastServer.h"
#include "Network/Session.h"
#include "../include/Handlers/SimpleHandlers.h"
#include "../include/Handlers/PlayerPositionHandler.h"
#include "../include/Handlers/IpcMainHandlers.h"
#include "../include/Handlers/WeaponKillHandlers.h"
#include <chrono>

namespace Cast
{
	void CastServer::tickPositionFlush()
	{
		for (auto& room : m_roomsManager.getAllRooms()) 
		{
			room->flushPendingPositions();
		}

		m_positionTimer->expires_after(std::chrono::milliseconds(50));
		m_positionTimer->async_wait([this](auto) { tickPositionFlush(); });
	}

	CastServer::CastServer(ioContext& io_context, const std::string& serverIp, std::uint16_t port, std::uint16_t mainPort, std::uint16_t serverId)
		: m_io_context{ io_context }
		, m_acceptor{ io_context, tcp::endpoint(asio::ip::address::from_string(serverIp), port) }
		, m_serverId{ serverId }
		, m_mainServerAcceptor{ io_context, tcp::endpoint(asio::ip::address::from_string(Common::Utils::SetupParser::getInstance().getSelfCastServerInfo().ip), mainPort) }
	{
		using namespace std::chrono;
		
		m_sessionsManager.setRoomsManager(&m_roomsManager);
		m_positionTimer = std::make_shared<asio::steady_timer>(m_io_context);
		tickPositionFlush();

		namespace CN = Common::Network;
		using namespace Cast::Network;

		// Cast <=> Main IPC
		CN::Session::addCallback<CN::PacketType::UNECRYPTED, CN::Session>(Common::Constants::M2C_mapId, [&](const CN::UnecryptedPacket& request,
			std::shared_ptr<CN::Session> session) { Cast::Handlers::handleMapId(request, session, m_roomsManager); });
		CN::Session::addCallback<CN::PacketType::UNECRYPTED, CN::Session>(Common::Constants::M2C_assassinModeInfo, [&](const CN::UnecryptedPacket& request,
			std::shared_ptr<CN::Session> session) { Cast::Handlers::handleAssassinMode(request, session, m_roomsManager); });
		CN::Session::addCallback<CN::PacketType::UNECRYPTED, CN::Session>(Common::Constants::M2C_playerTeamInfoBatch, [&](const CN::UnecryptedPacket& request,
			std::shared_ptr<CN::Session> session) { Cast::Handlers::handlePlayerTeamInfoBatch(request, session, m_roomsManager); });
		CN::Session::addCallback<CN::PacketType::UNECRYPTED, CN::Session>(Common::Constants::M2C_roomNumber, [&](const CN::UnecryptedPacket& request,
			std::shared_ptr<CN::Session> session) { Cast::Handlers::handleRoomNumber(request, session, m_roomsManager); });
		CN::Session::addCallback<CN::PacketType::UNECRYPTED, CN::Session>(Common::Constants::A2M_passIp, [&](const CN::UnecryptedPacket& request,
			std::shared_ptr<CN::Session> session) { Cast::Handlers::handleIpReq(request, session); });
		CN::Session::addCallback<CN::PacketType::UNECRYPTED, CN::Session>(Common::Constants::M2C_Invisibility, [&](const CN::UnecryptedPacket& request,
			std::shared_ptr<CN::Session> session) { Cast::Handlers::handleInvisibleCmd(request, session, m_roomsManager, m_sessionsManager); });


		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(71, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) { Cast::Handlers::pongHandler(request, session, m_roomsManager, m_sessionsManager, m_serverId); });

		// Room join
		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(140, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) {	m_roomsManager.switchRoomJoinOrExit(session, request.getSession()); });

		// Player respawn request
		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(166, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) {
				if (m_roomsManager.getModeOf(session->getId()) != Common::Enums::BossBattle && request.getDataSize() != 0) return;
				session->isDead = false;
				m_roomsManager.playerForwardToHost(request.getSession(), session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
			});

		// Room creation
		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(277, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) {	m_roomsManager.addRoom(std::make_shared<Cast::Classes::Room>(session->getId(), session),
				session->getId()); });

		// Leaving a room
		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(279, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) { m_roomsManager.switchRoomJoinOrExit(session); });

		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(252, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) { Cast::Handlers::connectionHandler(request, session, m_sessionsManager, m_io_context); });

		// Non-host weapon attack (in this order: mg bullet, mg shooting, melee, bazooka, rifle, shotgun, sniper)
		for (std::size_t order : {146, 147, 149, 151, 152, 153, 154})
		{
			Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(order, [&](const Common::Network::UnecryptedPacket& request,
				std::shared_ptr<Cast::Network::Session> session) { m_roomsManager.playerForwardToHost(request.getSession(), session->getId(),
					const_cast<Common::Network::UnecryptedPacket&>(request));
				});
		}

		// Explosives
		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(272, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) {
				const auto targetUid = Cast::Details::parseDataFromEnd<Main::Structures::UniqueId>(request, 4);
				if (auto targetSession = m_sessionsManager.getSession(targetUid.session))
				{
					m_acManager.submitEvent(std::make_unique<Ac::PacketFloodingEvent>(targetSession, 5, 1000, "Bazooka/Grenade flooding", 272));
				}
				m_roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
			});

		// Weapon reload
		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(275, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) {	session->asyncWrite(const_cast<Common::Network::UnecryptedPacket&>(request)); });

		// Unknown
		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(280, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) {
				m_roomsManager.playerForwardToHost(request.getSession(), session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
			});

		// AI Battle
		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(286, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) {
				if (m_roomsManager.getModeOf(session->getId()) == Common::Enums::AiBattle)
				{
					m_roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
				}
			});

		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(285, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) {
				if (m_roomsManager.getModeOf(session->getId()) == Common::Enums::AiBattle)
				{
					m_roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
				}
			});


		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(281, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) { Cast::Handlers::handlePlayerPosition(request, session, m_roomsManager, m_serverId, m_sessionsManager,
				m_acManager); });

		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(253, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) { Cast::Handlers::handleCrash(request, session, m_roomsManager, m_serverId); });

		// Match ending
		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(254, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) { m_roomsManager.endMatch(session->getId()); });

		// Match leave
		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(256, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) {
				session->setIsInMatch(false);

				auto roomOpt = m_roomsManager.getRoom(session->getId());
				if (!roomOpt) return;
				auto& room = *roomOpt;

				if (room->isArenaMode()) Cast::Handlers::handleArenaMode(m_roomsManager, room);
				room->tryFindNewAssassin(session->getId());
			});

		// Player names 
		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(284, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session)
			{
				// Not sure what this is for -- each player in the room sends this to host (through request.getSession())
				m_roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
			});

		// Player sync (needed because otherwise the player: 1. does not get the time left of the match, and 2. they don't respawn at all)
		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(309, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) { m_roomsManager.hostForwardToPlayer(session->getId(), request.getSession(),
				const_cast<Common::Network::UnecryptedPacket&>(request)); });

		// without this e.g. the bomb in "bomb battle" doesn't exist seemingly
		/*
		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(92, [&](const Common::Network::Packet& request,
			Cast::Network::Session& session) { Cast::Handlers::playerToHost(request, session, m_roomsManager); });
		*/

		// Room tick sync request:
		// When Option==9 in packet order 257: the non host's client sends packet 79 to the server, which dispatches to the host
		// This packet asks the host to provide the room sync to the non-host
		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(
			79,
			[&](const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session) {
				m_roomsManager.playerForwardToHost(request.getSession(), session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
			}
		);


		// In-room info request, apparenly contains "isDeath" data??
		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(306, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session)
			{
				Cast::Handlers::roomInfoJoinHandler(request, session, m_roomsManager, m_sessionsManager);
			});

		// CTB battery respawn for non host
		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(90, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) { m_roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request)); });

		// Zombie team broadcast for players that join the match late
		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(78, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) { m_roomsManager.playerForwardToHost(request.getSession(), session->getId(),
				const_cast<Common::Network::UnecryptedPacket&>(request)); });

		// Next round handler, this is covered in main, not sure why client sends it in cast -- but in any case cast sv must do nothing with it
		// otherwise if we resend this packet (host=>player or player=>host or broadcast) next round never starts
		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(259, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) {});

		// CTB battery respawn for host, Other weapon reload
		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(165, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) { m_roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request)); });

		// Bomb Battle related -- NON host
		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(155, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) { m_roomsManager.playerForwardToHost(request.getSession(), session->getId(),
				const_cast<Common::Network::UnecryptedPacket&>(request)); });

		// Bomb battle drop bomb 
		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(156, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) {  m_roomsManager.playerForwardToHost(request.getSession(), session->getId(),
				const_cast<Common::Network::UnecryptedPacket&>(request)); });

		// Bomb battle for host
		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(271, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) { m_roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request)); });

		// Room info:
		// This is a request from the client to get the Room Info (e.g. how many wins in which team, etc)
		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(255, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) { /* cannot find this on the client, do nothing for now */ });

		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(258, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) { Cast::Handlers::handleMatchStart(request, session, m_roomsManager); });

		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(257, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) { Cast::Handlers::handleMatchInitialLoading(request, session, m_roomsManager, m_serverId); });

		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(276, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) { Cast::Handlers::handlePlayerRespawn(request, session, m_roomsManager, m_sessionsManager,
				m_acManager); });

		// Room tick providing:
		// After the host client receives packet 78 from the non-host, it provides the non-host with the updated room tick
		// Without this handler, the player never respawns (not even TAB shows anything) => The player keeps being in a waiting initial state
		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(408, [&](const Common::Network::UnecryptedPacket& request,
				std::shared_ptr<Cast::Network::Session> session) {
				m_roomsManager.hostForwardToPlayer(session->getId(), request.getSession(),
					const_cast<Common::Network::UnecryptedPacket&>(request));
			}
		);


		// Items respawning
		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(260, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) {
				auto response = request;
				m_roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));

			});

		// Zombie respawn
		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(261, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) { m_roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request)); });

		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(262, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) { Cast::Handlers::handleItemPickup<Common::Enums::HOST>(request, session, m_roomsManager); });

		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(263, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) { Cast::Handlers::handleZombieAbility<Common::Enums::HOST>(request, session, m_roomsManager); });

		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(96, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) { Cast::Handlers::handleItemPickup<Common::Enums::NON_HOST>(request, session, m_roomsManager); });

		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(94, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) { Cast::Handlers::handleItemPickup<Common::Enums::NON_HOST>(request, session, m_roomsManager); });

		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(102, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) { Cast::Handlers::handleZombieAbility<Common::Enums::NON_HOST>(request, session, m_roomsManager); });

		// Reminder: do not broadcast the following packets to the whole room, otherwise "next round" elimination bug happens
		// Boss battle - main npcs movement/position, including boss position & small npcs positions
		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(282, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session)
			{
				m_roomsManager.broadcastToMatchExceptSelf(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
			});


		// Boss battle - npcs projectiles
		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(326, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) {
				if (m_roomsManager.getModeOf(session->getId()) == Common::Enums::BossBattle)
				{
					m_roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
				}
				else
				{
					m_roomsManager.hostForwardToPlayer(request.getSession(), session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
				}
			});

		// Boss battle - npcs respawn
		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(328, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) {
				if (m_roomsManager.getModeOf(session->getId()) == Common::Enums::BossBattle)
				{
					m_roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
				}
				else
				{
					m_roomsManager.hostForwardToPlayer(request.getSession(), session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
				}
			});

		// Boss battle - npc boss attack -- this is already sent to all clients
		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(331, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) {
				if (m_roomsManager.getModeOf(session->getId()) == Common::Enums::BossBattle)
				{
					m_roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
				}
				else
				{
					m_roomsManager.hostForwardToPlayer(request.getSession(), session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
				}
			});

		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(304, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) {
				if (m_roomsManager.getModeOf(session->getId()) == Common::Enums::BossBattle)
				{
					m_roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
				}
				else
				{
					m_roomsManager.hostForwardToPlayer(request.getSession(), session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
				}
			});


		// Shotgun / Mg damage
		for (std::size_t order : { 266 /*mg*/, 269 /* shotgun */})
		{
			Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(order, [&](const Common::Network::UnecryptedPacket& request,
				std::shared_ptr<Cast::Network::Session> session) {
					Cast::Handlers::handleSpecialWeaponDamage(request, session, m_roomsManager, m_sessionsManager, m_acManager);
				});
		}

		// Other weapons damage
		for (std::size_t order : {265, 267, 268, 270})
		{
			Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(order, [&](const Common::Network::UnecryptedPacket& request,
				std::shared_ptr<Cast::Network::Session> session) {
					Cast::Handlers::handleNormalWeaponDamage(request, session, m_roomsManager, m_sessionsManager, m_acManager);
				});
		}

		// Explosives damage
		Common::Network::Session::addCallback<CN::PacketType::UNECRYPTED, Session>(264, [&](const Common::Network::UnecryptedPacket& request,
			std::shared_ptr<Cast::Network::Session> session) {
				Cast::Handlers::handleExplosiveDamage(request, session, m_roomsManager, m_sessionsManager);
			});
	}

	void CastServer::asyncAccept()
	{
		m_socket.emplace(m_io_context);
		m_acceptor.async_accept(*m_socket, [&](asio::error_code error)
			{
				auto client = std::make_shared<Cast::Network::Session>(std::move(*CastServer::m_socket),
					std::bind(&Cast::Network::SessionsManager::removeSession, &m_sessionsManager, std::placeholders::_1));
				client->m_checkValidSession = true;
				client->sendConnectionACK(Common::Enums::CAST_SERVER);
				asyncAccept();
			});
	}

	void CastServer::asyncAcceptMainServer()
	{
		m_mainSocket.emplace(m_io_context);
		m_mainServerAcceptor.async_accept(*m_mainSocket, [this](asio::error_code error)
			{
				if (!error)
				{
					asio::ip::tcp::endpoint remoteEndpoint = m_mainSocket->remote_endpoint();

					if (remoteEndpoint.address() == asio::ip::address::from_string(Common::Utils::SetupParser::getInstance().getSelfMainServerInfo().ip) ||
						remoteEndpoint.address() == asio::ip::address::from_string("::1"))
					{
						auto mainIpc = std::make_shared<Common::Network::Session>(std::move(*m_mainSocket), nullptr);
						mainIpc->m_checkValidSession = false;
						mainIpc->sendConnectionACK(Common::Enums::IPC_SERVER);
					}
					else
					{
						::Utils::Logger::log("Unauthorized connection attempt from IP " + remoteEndpoint.address().to_string(), ::Utils::LogType::Warning);
						m_mainSocket->close();
					}
				}
				asyncAcceptMainServer();
			});
	}

}
