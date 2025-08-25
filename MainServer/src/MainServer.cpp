#include "../include/MainServer.h"
#include "Network/Session.h"
#include "Network/Packet.h"
#include "../include/Handlers/Player/InitialPlayerInfoHandlers.h"
#include "../include/Handlers/Item/BoughtItemHandler.h"
#include "../include/Handlers/Player/characterSelectionHandler.h"
#include "../include/Handlers/Item/BoughtItemHandler.h"
#include "../include/Handlers/MainLobbyChatHandler.h"
#include "../include/Handlers/Player/EquippedItemsHandler.h"
#include "../include/Handlers/Player/LobbyAccountInfoHandler.h"
#include "../include/Handlers/Player/FriendsGeneralHandler.h"
#include "../include/Handlers/Player/BlockPlayerHandler.h"
#include "../include/Handlers/EventsHandlers.h"
#include "../include/Handlers/Player/PlayerStateHandler.h"
#include "../include/Handlers/Player/PingHandler.h"
#include "../include/Handlers/Item/DeleteItemHandler.h"
#include "../include/Handlers/Item/GeneralItemHandler.h"
#include "../include/Handlers/Player/Mailbox/MailboxHandlers.h"
#include "../include/Handlers/Player/Mailbox/Gifthandlers.h"
#include "../include/Handlers/Item/ItemUpgradeHandler.h"
#include "../include/Handlers/Item/ItemRepairHandler.h"
#include "../include/Handlers/CapsuleSpinHandler.h"
#include "../include/Handlers/Room/RoomCreationHandler.h"
#include "../include/Handlers/Room/RoomsListHandler.h"
#include "../include/Handlers/Room/SimpleSettingHandler.h"
#include "../include/Handlers/Room/RoomMiscHandler.h"
#include "../include/Handlers/Room/RoomLeaveHandler.h"
#include "../include/Handlers/Room/RoomJoinHandler.h"
#include "../include/Handlers/Room/RoomStartHandler.h"
#include "../include/Handlers/Room/RoomChangeHostHandler.h"
#include "../include/Handlers/Room/MatchLeaveHandler.h"
#include "../include/Handlers/Room/EliminationNextRoundHandler.h"
#include "../include/Network/MainSession.h"
#include "../include/Network/AuthSession.h"
#include "../include/Handlers/Room/RoomInviteJoin.h"
#include "../include/Handlers/Room/IngameBatteryHandler.h"
#include "../include/Handlers/Clan/PartyListHandler.h"
#include "../include/Handlers/Clan/PartyCreationHandler.h"
#include "../include/Handlers/Clan/ActiveClansList.h"
#include "../include/Handlers/Clan/PartyJoinHandler.h"
#include "../include/Handlers/Clan/PartySettingsHandler.h"
#include "../include/Handlers/Clan/ClanRegisterHandler.h"
#include "../include/Handlers/Clan/OtherClanJoinHandler.h"
#include "../include/Handlers/Clan/ClanRoomLeaveHandler.h"
#include "../include/Handlers/Trade/TradeAckHandler.h"
#include "../include/Handlers/Trade/TradeInitializationHandler.h"
#include "../include/Handlers/Trade/TradeAddItemHandler.h"
#include "../include/Handlers/Trade/TradeRemoveItemHandler.h"
#include "../include/Handlers/Trade/TradeLockHandler.h"
#include "../include/Handlers/Trade/TradeFinalizeHandler.h"
#include "../include/Handlers/Trade/TradeCancelHandler.h"
#include "../include/Handlers/Item/GambleItemHandler.h"


// IPC Auth<=>Main
#include "../include/Handlers/IPC/AuthMainCallbacks.h"
#include "../include/Handlers/IPC/CastMainCallbacks.h"

#include <source_location>
#include <windows.h>
#include "boost/beast.hpp"
#include "../include/Network/HttpSession.h"

namespace Main
{
	MainServer::MainServer(ioContext& io_context, boost::asio::io_context& boost_io_context, const Common::Utils::MainSetup& mainSetup, std::uint32_t websitePort)
		: m_io_context{ io_context }
		, m_io_context_boost{ boost_io_context }
		, m_acceptor{ io_context, tcp::endpoint(asio::ip::address::from_string(mainSetup.ip), mainSetup.port) }
		, m_ipcServerAcceptor{ io_context, tcp::endpoint(asio::ip::address::from_string(mainSetup.ip), mainSetup.ipcPort) }
		, m_httpServerAcceptor{ boost_io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(mainSetup.ip), websitePort) }
		, m_serverId{ static_cast<std::uint16_t>(mainSetup.serverNumber) }
		, m_database{ }
		, m_isPublic{ mainSetup.isPublic }
		, m_scheduler{ 5, m_database }
	{
		m_chatCommands.m_scheduler = &m_scheduler;

		// Events
		m_capsuleSaleEvent = m_scheduler.immediatePersist(std::source_location::current(),
			&Main::Persistence::PersistentDatabase::getCapsuleEvent).value_or(Main::Structures::CapsuleListDatabase{});
		m_eventMissionInfo = m_scheduler.immediatePersist(std::source_location::current(),
			&Main::Persistence::PersistentDatabase::getEventMissionsInfo).value_or(Main::Structures::EventMissionInfo{});
		m_expMpEvent = m_scheduler.immediatePersist(std::source_location::current(),
			&Main::Persistence::PersistentDatabase::getExpMpBonusInfo).value_or(Main::Structures::ExpMpBonusInfo{});
		m_tradeSystemEvent = m_scheduler.immediatePersist(std::source_location::current(),
			&Main::Persistence::PersistentDatabase::getTradeEventsInfo).value_or(Main::Structures::EventMissionInfo{});

		const auto durationSinceEpoch = std::chrono::system_clock::now().time_since_epoch();
		m_timeSinceLastRestart = static_cast<std::uint64_t>(duration_cast<std::chrono::milliseconds>(durationSinceEpoch).count());
		initializeGeneralItemCallbacks();
		
		namespace CN = Common::Network;
		namespace MN = Main::Network;

		// IPC callbacks AuthServer <=> MainServer
		CN::Session::addCallback<CN::PacketType::UNECRYPTED, CN::Session>(Common::Constants::A2M_disconnectOnlinePlayer, [&](const CN::UnecryptedPacket& request,
			std::shared_ptr<CN::Session> session) { Main::Handlers::disconnectIfOnline(request, session, m_sessionsManager); });
		CN::Session::addCallback<CN::PacketType::UNECRYPTED, CN::Session>(Common::Constants::A2M_passIp, [&](const CN::UnecryptedPacket& request,
			std::shared_ptr<CN::Session> session) { Main::Handlers::handleIpReq(request, session); });
		CN::Session::addCallback<CN::PacketType::UNECRYPTED, CN::Session>(Common::Constants::A2M_getPlayersPerServer, [&](const CN::UnecryptedPacket& request,
			std::shared_ptr<CN::Session> session) { Main::Handlers::handlePlayerCountRequest(request, session, m_sessionsManager); });
		CN::Session::addCallback<CN::PacketType::UNECRYPTED, CN::Session>(Common::Constants::M2C_sessionId, [&](const CN::UnecryptedPacket& request,
			std::shared_ptr<CN::Session> session) { Main::Handlers::getSessionIdFor(request, session, m_sessionsManager); });
		CN::Session::addCallback<CN::PacketType::UNECRYPTED, CN::Session>(Common::Constants::C2M_updatePlayerState, [&](const CN::UnecryptedPacket& request,
			std::shared_ptr<CN::Session> session) { Main::Handlers::getPlayerStateUpdate(request, session, m_sessionsManager); });


		// Missing:
		// 73 => Disconnect request
		// 59, 168, 307, 82, 284
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(59, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { // In-game achievements
				session->addAchievement(request.getOption());
			});

		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(168, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session)
			{ 
				std::uint32_t parsed = Main::Details::parseData<std::uint32_t>(request);
				if (request.getExtra() == 8)
				{ // event mission
					if (parsed < 0 || parsed > Common::Constants::totalEventMissions)
					{
						session->sendMessage("Server error while retrieving reward info, please report this bug");
						return;
					}
					session->sendEventMissionReward(parsed);
				}
				else if (request.getMission() == 1 && request.getExtra() == 2 && parsed == 5)
				{ // mystery capsule
					if (session->spawnItemCommand(4510015, "Mystery Capsule spawned automatically from the server"))
					{
						session->sendMessage("You obtained a Mystery Capsule!");
					}
				}
			});

		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(284, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) {
				//session->asyncWrite(const_cast<Common::Network::Packet&>(request));
			});

		// Game callbacks
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(52, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handlePlayerBlock(request, session, m_sessionsManager, m_scheduler,
				Details::parseData<std::array<char, 16>>(request)); });
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(53, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { session->unblockAccount(Details::parseData<std::uint32_t>(request)); });
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(54, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { session->sendBlockedPlayers(); });
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(57, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { session->sendClanList(m_sessionsManager.getAllSessionsVec()); });
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(61, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleGeneralFriendRequests(request, session, m_sessionsManager); });
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(62, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleFriendDeletion(request, session, m_sessionsManager, Details::parseData<std::uint32_t>(request)); });
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(63, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) {
				START_BENCHMARK
				auto friendlist = m_scheduler.immediatePersist(std::source_location::current(), 
					&Main::Persistence::PersistentDatabase::loadPendingFriendRequests, session->getAccountInfo().accountID);
				session->sendFriendList(friendlist, m_serverId); 
				END_BENCHMARK(session.sendFriendList, session)
			});

		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(66, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleMailboxGiftSend(request, session, m_serverId); });
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(67, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session>  session) { session->displayGiftboxes(static_cast<Main::Enums::MailboxMission>(request.getMission())); });
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(68, [&](const Common::Network::Packet& request, 
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleInitialPlayerInfos(request, session, m_sessionsManager, m_scheduler, m_timeSinceLastRestart,
				m_serverId, m_isServerOffline, m_isPublic, m_eventMissionInfo, m_expMpEvent); });
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(71, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handlePing(request, session, m_roomsManager, Details::parseData<Main::ClientData::Ping>(request), m_scheduler); });

		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(74, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleCharacterSelection(request, session, m_roomsManager); });
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(84, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { session->sendLobbyList(m_sessionsManager.getAllSessionsVec()); });
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(85, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleLobbyAccountInfo(request, session, m_sessionsManager, m_serverId,
				Details::parseData<Main::Structures::UniqueId>(request)); });
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(86, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleBattery(request, session, m_roomsManager, Details::parseData<Main::Structures::UniqueId>(request)); });
		
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(87, Main::Handlers::handleBoughtItem);

		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(88, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleEquippedItemSwitch(request, session, m_roomsManager); });
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(89, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleItemDelete(request, session); });

		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(91, Main::Handlers::handleCouponBoughtItem);

		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(92, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session)
			{
				Main::Handlers::handleCapsuleSpin(request, session, m_sessionsManager, Details::parseData<Main::ClientData::CapsuleSpin>(request), m_capsuleSaleEvent); 
			});

		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(97, Main::Handlers::handleItemRepair);

		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(100, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { session->refundItem(Details::parseData<Main::ClientData::ItemRefund>(request)); });
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(101, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleItemUpgrade(request, session); });

		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(102, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleGeneralItem(request, session, Details::parseData<ClientData::BoxOpen>(request),
			m_generalItemCallbacks, m_cashItemsCallbacks, m_inMatchItemIds); });

		// clientData: {totalToDelete (4bytes), accountID (4bytes), timestamp (4bytes)} => accountID ignored
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(103, [&](const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session) {
				START_BENCHMARK
				std::uint32_t offset = 4; 
				for (std::uint32_t i = 0; i < Details::parseData<std::uint32_t>(request, 0); ++i) {
					offset += (i == 0 ? 4 : 8);
					session->deleteMailbox(static_cast<Main::Enums::MailboxMission>(request.getMission()), Details::parseData<std::uint32_t>(request, offset));
				}
				END_BENCHMARK(session.deleteMailbox, session)
		}); 
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(104, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleMailboxCommunication(request, session, m_sessionsManager, Details::parseData<Main::ClientData::MailboxMessage>(request)); });
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(105, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleReadMailbox(request, session, m_scheduler, Details::parseData<std::uint32_t>(request, 8)); });
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(106, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { session->displayMailboxes(static_cast<Main::Enums::MailboxMission>(request.getMission())); });
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(107, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleRoomStart(request, session, m_roomsManager, m_clansManager, m_timeSinceLastRestart); });
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(108, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleEliminationNextRound2(request, session, m_roomsManager); });
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(124, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleSimpleRoomSetting<Main::Enums::SETTING_ITEM>(request, session, m_roomsManager); }); // Item on/off
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(125, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleRoomMiscellaneous(request, session, m_roomsManager, m_timeSinceLastRestart); }); // Mode, Team Balance, Weapon Restriction, map, votekick
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(126, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleRoomMiscellaneous(request, session, m_roomsManager, m_timeSinceLastRestart); });
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(127, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleSimpleRoomSetting<Main::Enums::SETTING_OPEN>(request, session, m_roomsManager); }); // open on/off
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(128, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleHostChange(request, session, m_roomsManager); });
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(129, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleRoomMiscellaneous(request, session, m_roomsManager, m_timeSinceLastRestart); }); // Password
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(130, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleRoomMiscellaneous(request, session, m_roomsManager, m_timeSinceLastRestart); }); // Title etc.
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(131, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleSimpleRoomSetting<Main::Enums::SETTING_MAP>(request, session, m_roomsManager); }); // Map (normal click)
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(132, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleSimpleRoomSetting<Main::Enums::SETTING_PLAYERS_PER_TEAM>(request, session, m_roomsManager); }); // NvsN setting
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(133, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleSimpleRoomSetting<Main::Enums::SETTING_OBSERVER>(request, session, m_roomsManager); }); // ObserverMode,
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(134, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleSimpleRoomSetting<Main::Enums::SETTING_SPECIFIC>(request, session, m_roomsManager); }); // total kills/rounds...
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(135, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleSimpleRoomSetting<Main::Enums::SETTING_TIME>(request, session, m_roomsManager); });  // time
					CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(138, [&](const Common::Network::Packet& request,
						std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleRoomCreation(request, session, m_roomsManager, m_roomCreationEnabled); });
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(140, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleRoomJoin(request, session, m_roomsManager); });
				CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(141, [&](const Common::Network::Packet& request,
					std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleRoomLeave(request, session, m_sessionsManager, m_roomsManager, m_clansManager,
				Details::parseData<Main::Structures::UniqueId, false>(request)); });
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(142, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleRoomsList(request, session, m_roomsManager); });
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(158, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handlePlayerState(request, session, m_roomsManager, m_capsuleSaleEvent); });
					CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(159, [&](const Common::Network::Packet& request,
						std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleRoomMiscellaneous(request, session, m_roomsManager, m_timeSinceLastRestart); }); // Team switch
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(162, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleLobbyChatMessage(request, session, m_sessionsManager, m_chatCommands, m_roomsManager, m_scheduler, *this); });
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(161, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleRoomChatMessage(request, session, m_sessionsManager, m_chatCommands, m_roomsManager, m_scheduler, *this); });
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(200, Main::Handlers::handleGambleItem);
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(256, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleMatchLeave(request, session, m_sessionsManager, m_roomsManager, m_clansManager); });
		
		
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(259, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleEliminationNextRound(request, session, m_roomsManager); });

		/*
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(202, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleModeEvents(request, session, m_database); });

		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(232, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleMapEvents(request, session, m_database); });
		*/
				
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(163, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleJoinAndInvites(request, session, m_roomsManager, m_sessionsManager, m_serverId); });
		
		
		// This handler also needs to refactored + checked (not fully working, e.g. for level up, different match ending scoreboards, etc)
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(254, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleMatchEnding(request, session, m_roomsManager, m_clansManager, m_scheduler, m_expMpEvent,
				m_eventMissionInfo); });

		// CTB respawn 
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(160, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::unknown(request, session, m_roomsManager); });
		
		// Bomb Battle for host
		/*
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(163, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::unknown(request, session, m_roomsManager); });
			*/

		// Boss battle - respawning
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(329, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { session->respawnBossBattle(); });

		// CLANS -- TBA
		// CLAN TODO ====> Handle client crash or client closed!
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(58, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handlePartyList(request, session, m_clansManager); });

		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(109, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handlePartyCreation(request, session, m_clansManager); });

		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(110, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handlePartyJoin(request, session, m_clansManager, m_roomsManager); });

		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(111, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handlePartyRoomLeave(request, session, m_clansManager, m_roomsManager); });

		// this shows all clans inside "Quick Match"
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(112, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleActiveClansList<112>(request, session, m_clansManager); });

		// this shows all clans inside "Match Lobby"
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(113, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleActiveClansList<113>(request, session, m_clansManager); });

		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(114, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handlePartyLeaderChange(request, session, m_clansManager); });

		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(115, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleClanRegister<115>(request, session, m_clansManager); });

		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(116, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handlePartySettings<116>(request, session, m_clansManager); });

		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(117, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handlePartySettings<117>(request, session, m_clansManager); });

		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(120, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleClanRegister<120>(request, session, m_clansManager); });

		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(121, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleOtherClanJoin(request, session, m_clansManager, m_roomsManager); });

		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(122, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleClanRoomLeave(request, session, m_clansManager, m_roomsManager); });


		// Trade system
		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(191, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleTradeAck(request, session, m_sessionsManager, m_tradeSystemEvent); });

		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(192, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleTradeInitialization(request, session, m_sessionsManager); });

		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(194, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleAddTradeItem(request, session, m_sessionsManager); });

		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(195, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleTradeItemRemoval(request, session, m_sessionsManager); });

		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(196, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleTradeLock(request, session, m_sessionsManager); });

		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(197, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleTradeFinalization(request, session, m_sessionsManager); });

		CN::Session::addCallback<CN::PacketType::ENCRYPTED, MN::Session>(198, [&](const Common::Network::Packet& request,
			std::shared_ptr<Main::Network::Session> session) { Main::Handlers::handleTradeCancellation(request, session, m_sessionsManager); });

	}

	void MainServer::asyncAccept()
	{
		m_socket.emplace(m_io_context);
		m_acceptor.async_accept(*m_socket, [&](asio::error_code error)
			{
				m_sessionsManager.setRoomsManager(&m_roomsManager);
				m_sessionsManager.setClansManager(&m_clansManager);

				auto client = std::make_shared<Main::Network::Session>(m_scheduler, std::move(*MainServer::m_socket),
					std::bind(&Main::Network::SessionsManager::removeSession, &m_sessionsManager, std::placeholders::_1), m_acManager);

				client->sendConnectionACK(Common::Enums::MAIN_SERVER);
				asyncAccept();
			});
	}


	void MainServer::asyncAcceptIpcServer()
	{
		m_ipcSocket.emplace(m_io_context);
		m_ipcServerAcceptor.async_accept(*m_ipcSocket, [this](asio::error_code error)
			{
				if (!error)
				{
					asio::ip::tcp::endpoint remoteEndpoint = m_ipcSocket->remote_endpoint();
					if (remoteEndpoint.address() == asio::ip::address::from_string(Common::Utils::SetupParser::getInstance().getAuthSetup().ip)
						|| remoteEndpoint.address() == asio::ip::address::from_string(Common::Utils::SetupParser::getInstance().getSelfCastServerInfo().ip))
					{
						auto authIpc = std::make_shared<Common::Network::Session>(std::move(*m_ipcSocket), nullptr);
						authIpc->m_checkValidSession = false;
						authIpc->sendConnectionACK(Common::Enums::IPC_SERVER);
					}
					else
					{
						Utils::Logger::log("Unauthorized connection attempt from IP: " + remoteEndpoint.address().to_string(), Utils::LogType::Warning,
							"MainServer::asyncAcceptAuthServer");
						m_ipcSocket->close();
					}
				}
				asyncAcceptIpcServer();
			});
	}

	void MainServer::asyncAcceptHttp(const std::string& websiteIp)
	{
		m_httpSocket.emplace(m_io_context_boost); 

		m_httpServerAcceptor.async_accept(*m_httpSocket, [this, websiteIp](boost::system::error_code error)
			{
				if (!error)
				{
					auto remoteIp = m_httpSocket->remote_endpoint().address().to_string();
					if (remoteIp == websiteIp)
					{
						auto session = std::make_shared<Main::Network::HttpSession>(
							std::move(*m_httpSocket), m_sessionsManager, m_roomsManager, m_scheduler);
						session->start();
					}
					else
					{
						Utils::Logger::log("Unauthorized access attempt from: " + remoteIp, Utils::LogType::Warning, "MainServer::asyncAcceptHttp");
						m_httpSocket->close();
					}
				}
				asyncAcceptHttp(websiteIp);
			});
	}

	void MainServer::initializeGeneralItemCallbacks()
	{
		m_generalItemCallbacks[Main::Enums::KILLDEATH_RESET] = [&](std::shared_ptr<Main::Network::Session> session) { session->resetKillDeath(); };
		m_generalItemCallbacks[Main::Enums::RECORD_RESET] = [&](std::shared_ptr<Main::Network::Session> session) { session->resetRecord(); };
		m_generalItemCallbacks[Main::Enums::BATTERY_EXPANSION] = [&](std::shared_ptr<Main::Network::Session> session) { session->expandBattery(); };
		m_generalItemCallbacks[Main::Enums::INVENTORY_EXPANSION_10] = [&](std::shared_ptr<Main::Network::Session> session) { session->expandInventory(10); };
		m_generalItemCallbacks[Main::Enums::INVENTORY_EXPANSION_20] = [&](std::shared_ptr<Main::Network::Session> session) { session->expandInventory(20); };
		m_generalItemCallbacks[Main::Enums::INVENTORY_EXPANSION_40] = [&](std::shared_ptr<Main::Network::Session> session) { session->expandInventory(40); };
		m_generalItemCallbacks[Main::Enums::INVENTORY_EXPANSION_80] = [&](std::shared_ptr<Main::Network::Session> session) { session->expandInventory(80); };


		// Batteries
		m_generalItemCallbacks[Main::Enums::BATTERY_500_MP] = [&](std::shared_ptr<Main::Network::Session> session) {
			session->sendBattery(500);
			};
		m_generalItemCallbacks[Main::Enums::BATTERY_500_RT] = [&](std::shared_ptr<Main::Network::Session> session) {
			session->sendBattery(500);
			};
		m_generalItemCallbacks[Main::Enums::BATTERY_1000_MP] = [&](std::shared_ptr<Main::Network::Session> session) {
			session->sendBattery(1000);
			};
		m_generalItemCallbacks[Main::Enums::BATTERY_1000_RT] = [&](std::shared_ptr<Main::Network::Session>session) {
			session->sendBattery(1000);
			};
		m_generalItemCallbacks[Main::Enums::BATTERY_1000] = [&](std::shared_ptr<Main::Network::Session> session) {
			session->sendBattery(1000);
			};
		

		// Ingame item callbacks
		m_generalItemCallbacks[Main::Enums::BATTERY_500_MP] = [&](std::shared_ptr<Main::Network::Session> session) {
			session->sendBattery(500);
			};

		m_inMatchItemIds[Main::Enums::INSTANTRESPAWN_1] = [&](std::shared_ptr<Main::Network::Session> session, const Main::Structures::ItemSerialInfo& serial,
			const Common::Network::Packet& request) {
				return session->useInstantRespawn(serial, request);
			};
		m_inMatchItemIds[Main::Enums::INSTANTRESPAWN_2] = [&](std::shared_ptr<Main::Network::Session> session, const Main::Structures::ItemSerialInfo& serial,
			const Common::Network::Packet& request) {
				return session->useInstantRespawn(serial, request);
			};
		m_inMatchItemIds[Main::Enums::INSTANTRESPAWN_3] = [&](std::shared_ptr<Main::Network::Session> session, const Main::Structures::ItemSerialInfo& serial,
			const Common::Network::Packet& request) {
				return session->useInstantRespawn(serial, request);
			};
		m_inMatchItemIds[Main::Enums::INSTANTRESPAWN_4] = [&](std::shared_ptr<Main::Network::Session> session, const Main::Structures::ItemSerialInfo& serial,
			const Common::Network::Packet& request) {
				return session->useInstantRespawn(serial, request);
			};
		m_inMatchItemIds[Main::Enums::INSTANTRESPAWN_5] = [&](std::shared_ptr<Main::Network::Session> session, const Main::Structures::ItemSerialInfo& serial,
			const Common::Network::Packet& request) {
				return session->useInstantRespawn(serial, request);
			};
		m_inMatchItemIds[Main::Enums::INSTANTRESPAWN_6] = [&](std::shared_ptr<Main::Network::Session> session, const Main::Structures::ItemSerialInfo& serial,
			const Common::Network::Packet& request) {
				return session->useInstantRespawn(serial, request);
			};
		m_inMatchItemIds[Main::Enums::INSTANTRESPAWN_7] = [&](std::shared_ptr<Main::Network::Session> session, const Main::Structures::ItemSerialInfo& serial,
			const Common::Network::Packet& request) {
				return session->useInstantRespawn(serial, request);
			};
		m_inMatchItemIds[Main::Enums::INSTANTRESPAWN_8] = [&](std::shared_ptr<Main::Network::Session> session, const Main::Structures::ItemSerialInfo& serial,
			const Common::Network::Packet& request) {
				return session->useInstantRespawn(serial, request);
			};

		// MP boxes
		std::unordered_map<std::uint32_t, std::uint32_t> mpBoxValues = 
		{
			{Main::Enums::MP_100, 100}, {Main::Enums::MP_500, 500}, {Main::Enums::MP_1000, 1000},
			{Main::Enums::MP_2000, 2000}, {Main::Enums::MP_3000, 3000}, {Main::Enums::MP_3500, 3500},
			{Main::Enums::MP_4000, 4000}, {Main::Enums::MP_5000, 5000}, {Main::Enums::MP_6000, 6000},
			{Main::Enums::MP_7000, 7000}, {Main::Enums::MP_8000, 8000}, {Main::Enums::MP_9000, 9000},
			{Main::Enums::MP_10000, 10000}, {Main::Enums::MP_20000, 20000}, {Main::Enums::MP_30000, 30000},
			{Main::Enums::MP_50000, 50000}, {Main::Enums::MP_100000, 100000}, {Main::Enums::MP_500000, 500000},
			{Main::Enums::MP_1000000, 1000000}
		};
		for (const auto& [mpId, value] : mpBoxValues) 
		{
			m_cashItemsCallbacks[mpId] = [value, mpId](std::shared_ptr<Main::Network::Session> session) {
				session->openCashBox<Main::Enums::ITEM_MP>(mpId, value);
				};
		}

		// Coin boxes
		std::unordered_map<std::uint32_t, std::uint32_t> coinValues =
		{
			{Main::Enums::COIN_1, 1}, {Main::Enums::COIN_2, 2}, {Main::Enums::COIN_3, 3},
			{Main::Enums::COIN_4, 4}, {Main::Enums::COIN_5, 5}, {Main::Enums::COIN_6, 6},
			{Main::Enums::COIN_7, 7}, {Main::Enums::COIN_8, 8}, {Main::Enums::COIN_9, 9},
			{Main::Enums::COIN_10, 10}, {Main::Enums::COIN_20, 20}, {Main::Enums::COIN_30, 30},
			{Main::Enums::COIN_40, 40}, {Main::Enums::COIN_50, 50}, {Main::Enums::COIN_60, 60},
			{Main::Enums::COIN_70, 70}, {Main::Enums::COIN_80, 80}, {Main::Enums::COIN_90, 90},
			{Main::Enums::COIN_100, 100},
		};
		for (const auto& [coinId, value] : coinValues) 
		{
			m_cashItemsCallbacks[coinId] = [value, coinId](std::shared_ptr<Main::Network::Session> session) {
				session->openCashBox<Main::Enums::ITEM_COIN>(coinId, value);
				};
		}

		// Coupons
		std::unordered_map<std::uint32_t, std::uint32_t> couponBoxValues = 
		{
			{Main::Enums::COUPON_1, 1}, {Main::Enums::COUPON_1_AGAIN, 1}, {Main::Enums::COUPON_2, 2},
			{Main::Enums::COUPON_3, 3}, {Main::Enums::COUPON_4, 4}, {Main::Enums::COUPON_5, 5},
			{Main::Enums::COUPON_6, 6}, {Main::Enums::COUPON_7, 7}, {Main::Enums::COUPON_8, 8},
			{Main::Enums::COUPON_9, 9}, {Main::Enums::COUPON_10, 10}, {Main::Enums::COUPON_15, 15},
			{Main::Enums::COUPON_20, 20}, {Main::Enums::COUPON_25, 25}, {Main::Enums::COUPON_30, 30},
			{Main::Enums::COUPON_40, 40}, {Main::Enums::COUPON_50, 50}, {Main::Enums::COUPON_100, 100}
		};
		for (const auto& [couponId, value] : couponBoxValues) 
		{
			m_cashItemsCallbacks[couponId] = [value, couponId](std::shared_ptr<Main::Network::Session> session) {
				session->openCashBox<Main::Enums::ITEM_COUPON>(couponId, value);
				};
		}

	}
}
