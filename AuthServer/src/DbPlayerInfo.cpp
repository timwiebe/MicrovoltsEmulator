
#include "../include/DbPlayerInfo.h"
#include "../include/Structures/AuthAccountInfo.h"
#include "../include/AuthEnums.h"
#include "../include/Network/Packet.h"
#include "../include/AuthServer.h"
#include "../include/AuthUtils.h"

#include <chrono>
#include <format>
#include <string>
#include <utility>
#include <libcppotp/auth.h>
#include "Utils/Logger.h"
#include "Utils/Utils.h"
#include <Enums/PlayerEnums.h>

std::uint32_t generateHash(std::uint32_t accountId)
{
	std::hash<int> hasher;
	std::size_t fullHash = hasher(accountId);
	auto ret = static_cast<std::uint32_t>(fullHash % std::numeric_limits<std::uint32_t>::max());
	return ret;
}

namespace Auth
{
	namespace Persistence
	{
		PersistentDatabase::PersistentDatabase()
		{
			const auto& dbSetup = Common::Utils::SetupParser::getInstance().getDatabaseSetup();
			int retryCount = 0;
			const int maxRetries = 5;

			while (retryCount < maxRetries)
			{
				try
				{
					con = sql::mariadb::get_driver_instance()->connect("tcp://" + dbSetup.ip + ":" + std::to_string(dbSetup.port),
						dbSetup.username, dbSetup.password);
					con->setSchema(dbSetup.databaseName);

					::Utils::Logger::log("Connected to database at " + dbSetup.ip + ":" + std::to_string(dbSetup.port) + ", using database " + dbSetup.databaseName,
						::Utils::LogType::Info);
					return;  
				}
				catch (sql::SQLException& e)
				{
					++retryCount;
					::Utils::Logger::log("Error connecting to database: " + std::string(e.what()) + ", attempt " + std::to_string(retryCount),
						::Utils::LogType::Error);

					if (retryCount < maxRetries)
					{
						std::this_thread::sleep_for(std::chrono::seconds(2 * retryCount));  
					}
					else
					{
						::Utils::Logger::log("Max reconnection attempts reached, giving up.",
							::Utils::LogType::Error);
						throw;  
					}
				}
			}
		}

		bool PersistentDatabase::updateLastLoggedNow(std::uint32_t aid, const std::string& ip)
		{
			try
			{
				auto now = std::chrono::utc_clock::now();
				std::string currentUtcTime = std::format("{:%Y-%m-%d %H:%M:%S}", now);

				std::string updateQueryStr = "UPDATE Users SET LastLogged = ?, LastIP = ? WHERE AccountID = ?";
				std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement(updateQueryStr));

				stmt->setString(1, currentUtcTime);
				stmt->setString(2, ip);
				stmt->setUInt(3, aid);

				return stmt->executeUpdate() > 0;
			}
			catch (const sql::SQLException& e)
			{
				::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), ::Utils::LogType::Error, "PersistentDatabase::updateLastLoggedNow");
				return false;
			}
		}
		
		void PersistentDatabase::addHash(std::uint32_t accountID, std::uint32_t key)
		{
			try
			{
				std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("UPDATE Users SET AccountKey = ? WHERE AccountID = ?"));
				stmt->setUInt(1, key);
				stmt->setUInt(2, accountID);
				stmt->executeUpdate();
			}
			catch (const sql::SQLException& e)
			{
				::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), ::Utils::LogType::Error, "PersistentDatabase::addHash");
			}
		}

		std::pair<Common::Network::Packet, Auth::Structures::BasicAccountInfo> 
			PersistentDatabase::getPlayerInfo(const std::string& username, const std::string& password, bool is2fa)
		{
			Common::Network::Packet playerInfo;
			Auth::Structures::BasicAccountInfo playerInfoStructure{};

			const std::string queryStr = is2fa
				? "SELECT Users.*, Clans.Clanname, Clans.ClanFrontIcon, Clans.ClanBackIcon "
				"FROM Users LEFT JOIN Clans ON Users.ClanID = Clans.ClanId WHERE Username = ?"
				: "SELECT Users.*, Clans.Clanname, Clans.ClanFrontIcon, Clans.ClanBackIcon "
				"FROM Users LEFT JOIN Clans ON Users.ClanID = Clans.ClanId WHERE Username = ? AND Password = ?";

			try
			{
				std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement(queryStr));
				stmt->setString(1, username);
				if (!is2fa) stmt->setString(2, password);

				std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
				if (res->next())
				{
					const std::uint32_t playerGrade = static_cast<std::uint32_t>(res->getInt("Grade"));
					const std::string secret = res->getString("Secret").c_str();
					if (playerGrade >= Common::Enums::PlayerGrade::GRADE_ES && secret.empty())
					{ // >= MOD grade must mandatorily have 2FA enabled
						playerInfo.setExtra(Auth::Enums::Login::INCORRECT);
						return std::pair{ playerInfo, playerInfoStructure };
					}
					
					const std::string suspendedUntil = res->getString("SuspendedUntil").c_str();
					auto const time = std::chrono::utc_clock::now();
					const std::string current_time = std::format("{:%Y-%m-%d %X}", time);

					if (suspendedUntil <= current_time)
					{
						playerInfo.setExtra(Auth::Enums::Login::SUCCESS);

						playerInfoStructure.accountId = static_cast<std::uint32_t>(res->getInt("AccountID"));
						std::strncpy(playerInfoStructure.playerName, res->getString("Nickname").c_str(), sizeof(playerInfoStructure.playerName) - 1);
						playerInfoStructure.playerName[sizeof(playerInfoStructure.playerName) - 1] = '\0';
						std::strncpy(playerInfoStructure.clanName, res->getString("Clanname").c_str(), sizeof(playerInfoStructure.clanName) - 1);
						playerInfoStructure.clanName[sizeof(playerInfoStructure.clanName) - 1] = '\0';

						playerInfo.setOption(static_cast<std::uint32_t>(res->getInt("Grade")));
						playerInfoStructure.level = static_cast<std::uint32_t>(res->getInt("Level")) + 1;
						playerInfoStructure.exp = static_cast<std::uint32_t>(res->getInt("Experience"));
						playerInfoStructure.kills = static_cast<std::uint32_t>(res->getInt("Kills"));
						playerInfoStructure.deaths = static_cast<std::uint32_t>(res->getInt("Deaths"));
						playerInfoStructure.assists = static_cast<std::uint32_t>(res->getInt("Assists"));
						playerInfoStructure.wins = static_cast<std::uint32_t>(res->getInt("Wins"));
						playerInfoStructure.losses = static_cast<std::uint32_t>(res->getInt("Loses"));
						playerInfoStructure.draws = static_cast<std::uint32_t>(res->getInt("Draws"));
						playerInfoStructure.clanIconFrontID = static_cast<std::uint16_t>(res->getInt("ClanFrontIcon"));
						playerInfoStructure.clanIconBackID = static_cast<std::uint16_t>(res->getInt("ClanBackIcon"));
						playerInfoStructure.hashKey = generateHash(playerInfoStructure.accountId);
					}
					else
					{
						playerInfo.setExtra(Auth::Enums::Login::SUSPENDED);
					}
				}
				else
				{
					playerInfo.setExtra(Auth::Enums::Login::INCORRECT);
				}
			}
			catch (const sql::SQLException& e)
			{
				::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), ::Utils::LogType::Error, "PersistentDatabase::getPlayerInfo");
				playerInfo.setExtra(Auth::Enums::Login::DATA_ERROR);
			}

			if (playerInfoStructure.accountId != 0)
			{
				addHash(playerInfoStructure.accountId, playerInfoStructure.hashKey);
			}
			playerInfo.setData(reinterpret_cast<std::uint8_t*>(&playerInfoStructure), sizeof(playerInfoStructure));

			return std::pair{ playerInfo, playerInfoStructure };
		}

		std::pair<Common::Network::Packet, Auth::Structures::BasicAccountInfo>
			PersistentDatabase::twoFactorLogin(const std::string& username, const std::string& password, const std::string& nonHashedPassword, 
				const std::string& secret, bool isLoginOk)
		{
			Common::Network::Packet playerInfo;

			auto verifyUserCredentials = [this, &username, &password, &playerInfo]() -> std::pair<Common::Network::Packet, Auth::Structures::BasicAccountInfo> 
			{
				std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("SELECT Password FROM Users WHERE Username = ? AND Password = ?"));
				stmt->setString(1, username);
				stmt->setString(2, password);
				std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

				if (res->next())
				{ // correct password
					std::unique_ptr<sql::PreparedStatement> updateStmt(con->prepareStatement("UPDATE Users SET LoginOK = TRUE WHERE Username = ?"));
					updateStmt->setString(1, username);
					updateStmt->executeUpdate();

					// nb. this serves as an indication that the password is correct, and that the token must now be inserted for 2nd phase
					playerInfo.setExtra(Auth::Enums::Login::USE_TOKEN);
					return std::pair{ playerInfo, Auth::Structures::BasicAccountInfo{} };
				}
				else 
				{
					playerInfo.setExtra(Auth::Enums::Login::INCORRECT);
					return std::pair{ playerInfo, Auth::Structures::BasicAccountInfo{} };
				}
			};

			try
			{
				if (!isLoginOk)
				{ // Phase 1: given password is user password. Check that it's correct and return immediately, so they can login again for phase 2 (token)
					return verifyUserCredentials();
				}
				else
				{ // Phase 2: token verification
					auto verifiedCredentials = verifyUserCredentials();
					if (verifiedCredentials.first.getExtra() == Auth::Enums::Login::USE_TOKEN)
					{ // the user inputted their (correct) password again even though they already did it, notify them to input 2fa token again
						return verifiedCredentials;
					}
					else
					{ // otherwise, check if they inputted the correct 2fa token and reset LoginOK to re-ask for the original password if the token is wrong
						std::unique_ptr<sql::PreparedStatement> updateStmt(con->prepareStatement("UPDATE Users SET LoginOK = FALSE WHERE Username = ?"));
						updateStmt->setString(1, username);
						updateStmt->executeUpdate();

						auto generatedToken = auth::generateToken(secret);

						std::ostringstream tokenStream;
						tokenStream << std::setw(6) << std::setfill('0') << generatedToken;
						auto tokenString = tokenStream.str();

						if (nonHashedPassword != tokenString)
						{
							playerInfo.setExtra(Auth::Enums::Login::INCORRECT);
							return std::pair{ playerInfo, Auth::Structures::BasicAccountInfo{} };
						}
						return getPlayerInfo(username, password, true);
					}
				}
			}
			catch (const sql::SQLException& e)
			{
				::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), ::Utils::LogType::Error, "PersistentDatabase::twoFactorLogin");
				playerInfo.setExtra(Auth::Enums::Login::DATA_ERROR);
				return std::pair{ playerInfo, Auth::Structures::BasicAccountInfo{} };
			}
		}

		std::pair<Common::Network::Packet, Auth::Structures::BasicAccountInfo> 
			PersistentDatabase::getPlayerInfo(const std::string& username, const std::string& password)
		{
			const std::string hashedPassword = Common::Utils::calculateHashCryptoPP<CryptoPP::SHA256>(password);
			const std::string secretQuery = "SELECT Secret, LoginOK, Grade FROM Users WHERE Username = ?";
			std::unique_ptr<sql::PreparedStatement> secretStmt(con->prepareStatement(secretQuery));
			secretStmt->setString(1, username);

			std::unique_ptr<sql::ResultSet> secretRes(secretStmt->executeQuery());
			if (secretRes->next())
			{
				if (const std::string secret = secretRes->getString("Secret").c_str(); !secret.empty())
				{
					return twoFactorLogin(username, hashedPassword, password, secret, secretRes->getBoolean("LoginOK"));
				}
			}
			return getPlayerInfo(username, hashedPassword, false);
		}
	};
}
