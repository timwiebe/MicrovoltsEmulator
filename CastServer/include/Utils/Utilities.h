#ifndef UTILITIES_CAST_H
#define UTILITIES_CAST_H

#include <type_traits>
#include <Network/Packet.h>
#include <source_location>
#include <cstring> 

namespace Cast
{
	namespace Details
	{
		template<typename T, typename PacketType, bool Warn = true>
			requires (std::is_same_v<PacketType, Common::Network::Packet> || std::is_same_v<PacketType, Common::Network::UnecryptedPacket>)
		T parseDataImpl(const PacketType& request, std::uint32_t offset = 0, std::source_location location = std::source_location::current())
		{
			static_assert(std::is_trivially_copyable_v<T>, "Details::parseData requires T to be trivially copyable");

			const std::uint32_t dataAvailable = request.getDataSize() - offset;
			const std::uint32_t copySize = std::min(static_cast<std::uint32_t>(sizeof(T)), dataAvailable);

			T t{};
			std::memcpy(&t, request.getData() + offset, copySize);

			return t;
		}

		template<typename T, typename PacketType, bool Warn = true>
			requires (std::is_same_v<PacketType, Common::Network::Packet> || std::is_same_v<PacketType, Common::Network::UnecryptedPacket>)
		T parseDataFromEndImpl(const PacketType& request, std::uint32_t offsetFromEnd = 0, std::source_location location = std::source_location::current())
		{
			static_assert(std::is_trivially_copyable_v<T>, "Details::parseDataFromEnd requires T to be trivially copyable");

			const std::uint32_t dataSize = request.getDataSize();
			if (offsetFromEnd + sizeof(T) > dataSize)
			{
				if constexpr (Warn)
				{
					std::cerr << "[parseDataFromEnd warning] at " << location.file_name()
						<< ":" << location.line() << " — offsetFromEnd (" << offsetFromEnd
						<< ") + sizeof(T) (" << sizeof(T) << ") exceeds data size (" << dataSize << ")\n";
				}
				return T{};
			}

			T t{};
			std::memcpy(&t, request.getData() + (dataSize - offsetFromEnd - sizeof(T)), sizeof(T));

			return t;
		}

		template<typename T, bool Warn = true>
		T parseDataFromEnd(const Common::Network::Packet& request, std::uint32_t offsetFromEnd = 0, std::source_location location = std::source_location::current())
		{
			return parseDataFromEndImpl<T, Common::Network::Packet, Warn>(request, offsetFromEnd, location);
		}

		template<typename T, bool Warn = true>
		T parseDataFromEnd(const Common::Network::UnecryptedPacket& request, std::uint32_t offsetFromEnd = 0, std::source_location location = std::source_location::current())
		{
			return parseDataFromEndImpl<T, Common::Network::UnecryptedPacket, Warn>(request, offsetFromEnd, location);
		}

		template<typename T, bool Warn = true>
		T parseData(const Common::Network::Packet& request, std::uint32_t offset = 0, std::source_location location = std::source_location::current())
		{
			return parseDataImpl<T, Common::Network::Packet, Warn>(request, offset, location);
		}

		template<typename T, bool Warn = true>
		T parseData(const Common::Network::UnecryptedPacket& request, std::uint32_t offset = 0, std::source_location location = std::source_location::current())
		{
			return parseDataImpl<T, Common::Network::UnecryptedPacket, Warn>(request, offset, location);
		}

		inline bool mustBroadcastDeath(std::uint32_t mode)
		{
			return (mode == Common::Enums::Elimination || mode == Common::Enums::CaptureTheBattery
				|| mode == Common::Enums::BombBattle || mode == Common::Enums::Clan_BombBattle
				|| mode == Common::Enums::Clan_CaptureTheBattery || mode == Common::Enums::Clan_Elimination
				|| mode == Common::Enums::ZombieMode);

		}
	}
}
#endif
