#ifndef CONVERT_RT_TO_COUPONS
#define CONVERT_RT_TO_COUPONS


#include "../ICommand.h"
#include "../ChatCommands.h"
#include "../../MainServer.h"

namespace Main
{
	namespace Command
	{
		class Rt2Coupons final : public ICommand
		{
		private:
			std::uint32_t m_rtToConvert{};

			bool parseCommand(const std::string& providedCommand) override
			{
				std::smatch match;
				if (std::regex_match(providedCommand, match, m_pattern))
				{
					const std::string& matched_str = match[1].str();
					const std::from_chars_result result = std::from_chars(matched_str.data(), matched_str.data() + matched_str.size(), m_rtToConvert);
					if (result.ec == std::errc())
					{
						return true;
					}
				}
				return false;
			}

		public:
			explicit Rt2Coupons(const Common::Enums::PlayerGrade requiredGrade)
                : ICommand{ requiredGrade, "/rt2coupons <total rocktokens> (1 coupon = 3000 RT)", R"(^\S+\s(\d+)$)" }
            {
			}

			void execute(const std::string& command, std::shared_ptr<Main::Network::Session> session, MN::SessionsManager& sessionsManager,
				MC::RoomsManager& roomsManager, MP::MainScheduler&, std::uint32_t roomNumber,
				Main::MainServer& mainSv) override
			{
                if (!parseCommand(command))
                {
                    session->sendMessage("parsing error");
                    return;
                }

                auto& accountInfo = session->getAccountInfo();

                constexpr std::uint32_t couponCost = 5'000;
                constexpr std::uint32_t maxCoupons = 250;

                if (accountInfo.rockTotens < couponCost)
                {
                    session->sendMessage("Error: you must have at least 5'000 RockTokens");
                    return;
                }
                if (m_rtToConvert > accountInfo.rockTotens)
                {
                    session->sendMessage("Error: you must specify an amount of RT that you currently have!");
                    return;
                }

                const std::uint32_t currentCoupons = session->getPlayer().getTotalCoupons();
                if (currentCoupons >= maxCoupons)
                {
                    session->sendMessage("Error: You already have the maximum of 250 coupons.");
                    return;
                }

                std::uint32_t couponsToSpawn = m_rtToConvert / couponCost;
                if (couponsToSpawn == 0)
                {
                    session->sendMessage("Error: you must convert at least 5'000 RockTokens (1 coupon).");
                    return;
                }

                if (currentCoupons + couponsToSpawn > maxCoupons)
                {
                    couponsToSpawn = maxCoupons - currentCoupons;
                    session->sendMessage("Note: You can only hold " + std::to_string(maxCoupons) + " coupons. Conversion limited to " + std::to_string(couponsToSpawn) + ".");
                }

                if (session->spawnCouponImmediate(couponsToSpawn))
                {
                    const std::uint32_t rtSpent = couponsToSpawn * couponCost;
                    session->setAccountRockTotens(accountInfo.rockTotens - rtSpent);
                    session->sendCurrency();
                    session->sendMessage("Success: converted " + std::to_string(rtSpent) + " RT into " + std::to_string(couponsToSpawn) + " coupon(s).");
                }
                else
                {
                    session->sendMessage("Error: failed to spawn coupons - please report this issue");
                }
			}
		};

		REGISTER_CMD(Rt2Coupons, Common::Enums::PlayerGrade::GRADE_NORMAL)
	}
}

#endif


