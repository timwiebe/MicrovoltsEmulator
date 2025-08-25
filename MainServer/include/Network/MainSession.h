#ifndef MAIN_SESSION_HEADER
#define MAIN_SESSION_HEADER

#include "Network/Session.h"
#include "../Structures/AccountInfo/MainAccountInfo.h"
#include "../Structures/Item/MainItem.h"
#include "../Structures/Item/MainEquippedItem.h"
#include "../Persistence/MainScheduler.h"
#include "../Persistence/MainDatabaseManager.h"
#include "../Structures/PlayerLists/BlockedPlayer.h"
#include "../Structures/PlayerLists/Friend.h"
#include "../Structures/Mailbox.h"
#include "Enums/GameEnums.h"

#include "../Classes/Player.h"
#include <functional>
#include <chrono>
#include <asio.hpp>
#include "../Structures/EndScoreboard.h"
#include "Utils/Constants.h"
#include "../Structures/ClientData/Structures.h"
#include <random>
#include <algorithm>
#include "../Structures/TradeSystem/TradeSystemItem.h"
#include <AntiCheat/AntiCheat.h>

namespace Main
{
	namespace Network
	{
		class Session : public Common::Network::Session
		{
		private:
			Main::Classes::Player m_player{};
			Main::Persistence::MainScheduler& m_scheduler;
			Common::Network::Packet m_packet{};
			Ac::AntiCheatManager& m_acManager;
			std::mt19937 m_gen{ std::random_device{}() };
			std::uniform_int_distribution<int> m_dist{ 1, 100 };

			std::unordered_map<std::uint32_t, std::uint32_t> m_eventMissions;
			std::unordered_set<int> m_packetReplicaWhitelist{ 71, 81, 86, 87, 101, 284 };

			bool spawnCouponCommon(const std::uint32_t total, bool useAddItem);

		public:
			std::uint16_t m_totalBossBattleRespawnsLeft = 3;
			std::uint64_t m_matchStartTime{};
			bool m_hasCheckedMatchBan = false;
			bool m_isInvisible{};
			bool m_hasBeenMatchBanned{};
			std::string m_hwid{ "" };
			std::uint32_t m_pingPacketCounter = 0;
			
			using Item = Main::Structures::Item;
			using EquippedItem = Main::Structures::EquippedItem;
			using DetailedEquippedItem = Main::Structures::DetailedEquippedItem;
			using BoughtItem = Main::Structures::BoughtItem;
			using AccountInfo = Main::Structures::AccountInfo;
			using tcp = asio::ip::tcp;

			explicit Session(Main::Persistence::MainScheduler& scheduler, tcp::socket&& socket, std::function<void(std::size_t)> fnct,
				Ac::AntiCheatManager& ac);

			void persistNow()
			{
				::Utils::Logger::log("Persisting for: " + std::to_string(m_player.getAccountID()), ::Utils::LogType::Info, "MainSession::persistNow");
				m_scheduler.persistFor(m_player.getAccountID());
			}

		private:
			// For many packets, the clients assumes certain extras and logic:
			// - (1) If the packet is empty => extra 6, no data
			// - (2) If the packet is non-empty, and its size in bytes is < 1440, then send the single packet with extra = 37
			// - (3) Otherwise, split the packet into sub-packets, where the first sub-packet's extra = 37, and all other sub-packet extra = 0 (this function performs this step)
			// Assumption: Before this function is called, points (1) and (2) were performed, and m_packet is already set to perform point (3) here
			// (data is also valid)
			template <typename T>
			void sendSeparatePackets(const std::vector<T>& data)
			{
				const std::size_t maxPayloadSize = Common::Constants::maxPacketBytes - Common::Constants::headerSize;
				const std::size_t numItemsPerPacket = maxPayloadSize / sizeof(T);
				const std::size_t totalPackets = (data.size() + numItemsPerPacket - 1) / numItemsPerPacket;

				if (numItemsPerPacket == 0) return;
				for (std::size_t packetIndex = 0; packetIndex < totalPackets; ++packetIndex)
				{
					const std::size_t startIndex = packetIndex * numItemsPerPacket;
					const std::size_t currentSize = (numItemsPerPacket < data.size() - startIndex) ? numItemsPerPacket : (data.size() - startIndex);

					const T* currentData = data.data() + startIndex;
					m_packet.setExtra(packetIndex == 0 ? 37 : 0);
					m_packet.setData(reinterpret_cast<const std::uint8_t*>(currentData), currentSize * sizeof(T));
					m_packet.setOption(currentSize);

					asyncWrite(m_packet);
				}
			}

			template <typename Predicate>
			bool checkEquippedItems(Predicate&& pred, const std::string& failMessage)
			{
				const auto equippedItems = m_player.getEquippedItemsFor(m_player.getAccountInfo().latestSelectedCharacter);

				for (const auto& equippedItem : equippedItems)
				{
					if (equippedItem.serialInfo.itemNumber == 0)
						continue;

					if (!pred(static_cast<Common::Enums::ItemType>(equippedItem.type), equippedItem.id))
					{
						sendMessage("Item Type " + std::to_string(equippedItem.type) + " " + failMessage);
						return false;
					}
				}
				return true;
			}

		public:
			template<Main::Enums::ItemCurrencyType CT>
			void openCashBox(std::uint32_t itemId, std::uint32_t value)
			{
				bool cashSet = false;
				if constexpr (CT == Main::Enums::ITEM_MP)
					cashSet = setAccountMicroPoints(m_player.getAccountInfo().microPoints + value);
				else if constexpr (CT == Main::Enums::ITEM_RT)
					cashSet = setAccountRockTotens(m_player.getAccountInfo().rockTotens + value);
				else if constexpr (CT == Main::Enums::ITEM_COIN)
					cashSet = setAccountCoins(m_player.getAccountInfo().coins + value);
					
				if (cashSet || CT == Main::Enums::ITEM_COUPON)
				{
					if (CT == Main::Enums::ITEM_COUPON)
					{
						if (m_player.getTotalCoupons() >= 250)
						{
							sendMessage("Max coupon limit (250) has been reached. Open this box when you have less coupons!");
						}
						else
						{
							spawnCoupon(value);
						}
					}

					m_packet.setCommand(102, 1, 0, 1);
					Main::Structures::BoxItem box{ itemId };
					m_packet.setData(nullptr, 0);
					m_packet.setData(reinterpret_cast<std::uint8_t*>(&box), sizeof(box));
					asyncWrite(m_packet);
				}
			}

			const Main::Classes::Player& getPlayer() const noexcept { return m_player; }

			bool tryRemoveCoupons(std::uint32_t totalCouponsNeeded);

			void sendEmptyPacket(std::size_t order, std::uint32_t mission = 0);

			std::size_t getSessionId() const;

			void onPacket(std::vector<std::uint8_t>& data) override;

			bool sendOfflineMailbox(Main::Structures::Mailbox mailbox);

			bool sendOnlineMailbox(std::shared_ptr<Session> target, Main::Structures::Mailbox mailbox);

			void addMailboxReceived(const Main::Structures::Mailbox& mailbox);

			void sendUnreadMailboxes();

			void addMailboxSent(const Main::Structures::Mailbox& mailbox);

			void addGiftboxReceived(const Main::Structures::Giftbox& mailbox);

			void deleteGiftbox(std::uint32_t timestamp);

			void displayGiftboxes(Main::Enums::MailboxMission giftboxType);

			void displayMailboxes(Main::Enums::MailboxMission mailboxType);
				
		private:
			bool deleteSentMailbox(std::uint32_t timestamp);

			bool deleteReceivedMailbox(std::uint32_t timestamp);

		public:
			bool deleteMailbox(Main::Enums::MailboxMission type, std::uint32_t timestamp);

			void setReceivedGiftboxes(const std::vector<Main::Structures::Giftbox>& giftboxes);

			void setMailbox(const std::vector<Main::Structures::Mailbox>& mailbox, bool sent);

			void sendAccountInfoConfirmation();

			void setAccountInfo(const AccountInfo& accountInfo);

			const AccountInfo& getAccountInfo() const;

			void addBatteryObtainedInMatch(std::uint32_t newBattery);

			void setMatchStartTime();

			void sendEventMission(const ClientData::EventMissionPoint& eventMission);

			void sendEventMissionReward(std::uint32_t eventIndex);

			void setEventMissions(const std::unordered_map<std::uint32_t, std::uint32_t>& activeEventIds);

			std::uint64_t getMatchStartTime() const noexcept { return m_matchStartTime; }

			void sendFriendList(std::vector<Main::Structures::Friend>& pendingFriends, std::uint32_t serverId);

			std::unordered_map<Main::Structures::Friend, std::weak_ptr<Session>>& getFriendSessions();

			void sendFriendRequest(std::shared_ptr<Main::Network::Session> targetSession, const char* nickname);

			void acceptFriendRequest(std::shared_ptr<Main::Network::Session> senderSession, const Main::Structures::Friend& target, const std::uint8_t* const data);

			bool removeBossBattleTicket();

		private:
			void handleOfflineFriendRequest(const AccountInfo& accountInfo, const char* nickname);
			void handleOnlineFriendRequest(std::shared_ptr<Main::Network::Session> targetSession, const AccountInfo& accountInfo);

		public:
			void sendAccountInfo(Common::Network::Packet& response);

			void setPing(std::uint16_t ping);

			void setFriendList(const std::vector<Main::Structures::Friend>& friendlist);

			void logFriend(Main::Enums::FriendLogType logType, std::uint32_t targetAccountId);

			void updateFriendSession(std::shared_ptr<Main::Network::Session> targetSession, bool remove = false);

			bool blockAccount(std::uint32_t accountId, const char* nickname);

			void unblockAccount(std::uint32_t accountId);

			void addAchievement(std::uint32_t idx);

			void sendBlockedPlayers();

			void setBlockedPlayers(const std::vector<Main::Structures::BlockedPlayer>& blockedPlayers);

			void setHasBeenMatchBanned(bool v);

			void setIsInvisible(bool value);

			bool isInvisible() const noexcept;

			bool hasBeenMatchBanned() const noexcept;

			// call once with default "persist", since removeFriend removes the friend for both players
			void deleteFriend(std::uint32_t targetAccountId, bool persist = true);

			bool prolongItems(const std::vector<Main::Structures::BoughtItemToProlong>& toProlongItems, const std::vector<std::uint64_t>& newExpirations);

			bool upgradeWeapon(std::uint32_t itemId, const Main::Structures::ItemSerialInfo& serialInfo, bool hasParent,
				std::uint8_t mission, std::uint8_t option, bool useEnergyRefund, bool useGlue);

			void resetUpgrade(const Main::ClientData::UpgradeReset& upgradeReset);

			bool useMatchItem(const Main::Structures::ItemSerialInfo& serialInfo, const Common::Network::Packet& request);

			bool useNoPenalty(const Main::Structures::ItemSerialInfo& serialInfo, const Common::Network::Packet& request);

			void refundItem(const Main::ClientData::ItemRefund& itemRefund);

			bool deleteItemBasic(const Main::Structures::ItemSerialInfo& itemSerialInfo, const std::string& caller);
			
			void addItems(const std::vector<Item>& items);

			void addItems(const std::vector<Main::Structures::BoxItem>& boxItems, const Main::ClientData::BoxOpen& boxData, std::uint32_t extra);

			bool addItem(const Item& item, bool isCouponItem = false);

			bool addItem(const Main::Structures::Giftbox& item);

			bool addItems(const std::vector<BoughtItem>& boughtItems, bool areCouponItems = false);

			bool setAccountRockTotens(std::uint32_t rt);

			bool setAccountCoins(std::uint16_t coins);

			bool setAccountMicroPoints(std::uint32_t mp);

			void setAccountLatestCharacterSelected(std::uint16_t latestCharacterSelected);

			bool setLevel(std::uint16_t level);

			bool setExperience(std::uint32_t experience);

			void setPlayerName(const std::string& playerName);

			void addEnergyToItem(const Main::ClientData::ItemAddEnergy& itemAddEnergy, std::uint16_t option, std::uint16_t mission);

			void sendWeeklyReward();

			void sendMonthlyReward();

			void storeHwid();

			void updateHwid(const std::string& hwid);

			void addFriend(const Main::Structures::Friend& ffriend);

			std::optional<Main::Structures::Friend> addOnlineFriend(std::shared_ptr<Main::Network::Session> session);

			void equipItem(const std::uint16_t itemNumber);

			bool replaceItem(const Main::Structures::ItemSerialInfo& serialInfo, std::uint32_t newItemId, const std::string& action);

			bool spawnItemCommand(std::uint32_t itemId, const std::string& action);
			bool spawnCoupon(const std::uint32_t total);
			bool spawnCouponImmediate(const std::uint32_t total);

			bool receiveGift(std::uint32_t itemId, const std::string& giftDescription);

			bool deleteItem(const Main::Structures::ItemSerialInfo& itemSerialInfoToDelete, const std::string& action);
			
			void logItemInfo(std::uint64_t itemNumber, std::uint32_t itemId, std::uint32_t expiration, const std::string& action);

			bool sendDeletePacket(const Main::Structures::ItemSerialInfo& itemSerialInfoToDelete);

			void sendCurrency();
			void sendCurrency(std::uint32_t newMP, std::uint32_t newRT);

			void sendMp(std::uint32_t mptoAdd);

			void sendRt(std::uint32_t rtToAdd);

			void reduceEquippedItemsDurability();

			void updateItemDurability(std::uint32_t itemNumber, std::uint32_t newDurability);

			void switchItemEquip(std::uint32_t characterId, std::uint64_t itemNumber);

			void unequipItem(std::uint64_t itemType);

			void updateSingleWaveScore(std::uint32_t score, std::uint32_t stage);

			void setLatestItemNumber(std::uint64_t itemNum);

			bool banAccount(std::uint64_t daysDuration, const std::string& reason, bool isMatchBan = false);

			void completeTutorial();

			bool muteAccount(std::uint64_t daysDuration, const std::string& reason, const std::string& mutedBy);

			bool unmuteAccount();

			bool disableRoomCreation(std::uint64_t daysDuration);

			bool enableRoomCreation();

			bool disableVotekick(std::uint64_t daysDuration);

			bool enableVotekick();

			void setRoomCreationDisabled();

			void setVotekickDisabled();

			void setMute(Main::Structures::MuteInfo val);

			void sendLobbyList(const std::vector<std::shared_ptr<Session>>& allSessions);

			void sendClanList(const std::vector<std::shared_ptr<Session>>& allSessions);

			void clear();

			void setPlayerState(Common::Enums::PlayerState playerState);

			bool useInstantRespawn(const Main::Structures::ItemSerialInfo& serialInfo, const Common::Network::Packet& request);

			void addLuckyPoints(std::uint32_t points);

			void setLuckyPoints(std::uint32_t points);

			void setRoomNumber(std::uint16_t roomNumber);

			void setClanRoomNumber(std::uint16_t number);

			void leaveRoom();

			void decreaseRoomNumber();

			void setIsInMatch(bool val);

			void sendBattery(std::uint32_t battery);

			void storeEndMatchStats(std::uint64_t totalPlaytimeSeconds, const Main::Structures::ScoreboardResponse& stats, Main::Enums::MatchEnd matchEnd,
				bool hasLeveledUp, bool isZombieMode, bool isClanMatch);

			void resetKillDeath();

			void resetRecord();

			void expandBattery();

			void expandInventory(std::uint32_t spaceToAdd);

			void sendInventory(std::uint32_t accountID);

			void setLatestWeeklyRewardDate(const std::string& date) { m_player.setLatestWeeklyRewardDate(date); }

			void setLatestMonthlyRewardDate(const std::string& date) { m_player.setLatestMonthlyRewardDate(date); }

			void setCurrentlyTradingWithAccountId(std::uint32_t targetAccountId);

			std::uint32_t getCurrentlyTradingWithAccountId() const;

			bool addTradedItem(std::uint32_t itemId, const Main::Structures::ItemSerialInfo& serialInfo);

			void removeTradedItem(const Main::Structures::ItemSerialInfo& serialInfo);

			void resetTradedItems();

			const std::vector<Main::Structures::TradeBasicItem>& getTradedItems() const;

			void temporarilySealAllItems();

			void unsealAllItems();

			bool deleteItems(const std::vector<Main::Structures::TradeBasicItem>& tradeBasicItems, const std::string& action);

			void spawnItems(const std::vector<Main::Structures::TradeBasicItem>& tradeBasicItems, const std::string& action);

			void addItems(const std::vector<Main::Structures::TradeBasicItem>& tradedItems);

			void addItemFromTrade(const Main::Structures::TradeBasicItem& tradeItem);

			void lockTrade();

			bool hasPlayerLocked() const;

			void resetTradeInfo();

			bool spawnItem(std::uint32_t itemId, const Main::Structures::ItemSerialInfo& itemSerialInfo, const std::string& action);

			bool hasCsdItems();
			bool hasBasicItems();

			void respawnBossBattle();
		};
	}
}
#endif