#ifndef AUTH_SERVERCHANNELINFO_H
#define AUTH_SERVERCHANNELINFO_H

#include <vector>
#include <cstdint>

namespace Auth
{
	namespace Structures
	{
PACK_PUSH(1)
struct ChannelsInfo
		{
			std::vector<std::uint32_t> channels{};

			explicit ChannelsInfo(const std::vector<std::uint32_t>& givenChannels)
			{
				channels.resize(givenChannels.size());
				for (std::size_t idx = 0; std::uint32_t & current : channels)
				{
					current = static_cast<std::uint32_t>((idx + 1) | givenChannels[idx++]);
				}
			}
		};
PACK_POP()
	}
}

#endif