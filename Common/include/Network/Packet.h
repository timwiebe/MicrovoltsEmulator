#ifndef COMMON_PACKET_H
#define COMMON_PACKET_H

#include <stdlib.h>
#include <vector>

#include "../Protocol/TcpHeader.h"
#include "../Protocol/CommandHeader.h"
#include "../Cryptography/Crypt.h"
#include <optional>
#include "../../include/Network/Packet.h"
#include "../../include/Enums/MiscellaneousEnums.h"
#include "../../include/Utils/Parser.h"
#include <cstring> 

#include <iostream>

namespace Common
{
    namespace Network
    {
        enum class PacketType { ENCRYPTED, UNECRYPTED };

        template<PacketType T>
        class PacketImpl
        {
        public:
            Common::Protocol::TcpHeader m_header{};
            Common::Protocol::CommandHeader m_command{};
            std::vector<uint8_t> m_data{};

        public:
            PacketImpl() = default;

            PacketImpl(std::size_t memoryToReserve, std::uint16_t order, std::uint8_t option)
            {
                m_data.reserve(memoryToReserve);
                setCommand(order, 0, 0, option);
            }

            void setCommand(std::uint16_t order, std::uint8_t mission, std::uint8_t extra, std::uint8_t option)
            {
                m_command = Common::Protocol::CommandHeader{ mission, order, extra, option };
            }

            void setTcpHeader(std::uint32_t sessionId, std::uint32_t crypt) requires (T == PacketType::ENCRYPTED)
            {
                m_header.initialize(sessionId, crypt, sizeof(m_header) + sizeof(m_command) + m_data.size());
            }

            void setTcpHeader(std::uint32_t sessionId) requires (T == PacketType::UNECRYPTED)
            {
                m_header.initialize(sessionId, Common::Enums::NO_ENCRYPTION, sizeof(m_header) + sizeof(m_command) + m_data.size());
            }

            void setSession(std::uint16_t session)
            {
                m_header.setSessionId(session);
            }

            void setMission(std::uint8_t mission)
            {
                m_command.setMission(mission);
            }

            void setOrder(std::uint16_t order)
            {
                m_command.setOrder(order);
            }

            void setSize(std::uint32_t size)
            {
                m_header.setSize(size);
            }

            void setExtra(std::uint8_t extra)
            {
                m_command.setExtra(extra);
            }

            void setOption(std::uint8_t option)
            {
                m_command.setOption(option);
            }

            void setData(const std::uint8_t* data, uint16_t size)
            {
                m_data.resize(size); 
                std::memcpy(m_data.data(), data, size); 
                m_header.setSize(8 /*header size*/ + size);
            }

            const std::uint8_t* const getData() const
            {
                return m_data.data();
            }

            std::uint8_t* getMutableData()
            {
                return m_data.data();
            }

            const Common::Protocol::TcpHeader& getTcpHeader() const noexcept
            {
                return m_header;
            }

            const Common::Protocol::CommandHeader& getCommandHeader() const noexcept
            {
                return m_command;
            }

            std::uint16_t getSession() const
            {
                return m_header.getSessionId();
            }

            std::uint8_t getMission() const
            {
                return m_command.getMission();
            }

            std::uint16_t getOrder() const
            {
                return m_command.getOrder();
            }

            std::uint8_t getExtra() const
            {
                return m_command.getExtra();
            }

            std::uint8_t getOption() const
            {
                return m_command.getOption();
            }

            std::uint32_t getDataSize() const
            {
                return static_cast<std::uint32_t>(m_data.size());
            }

            std::uint32_t getFullSize() const
            {
                return static_cast<std::uint32_t>(sizeof(Common::Protocol::TcpHeader) + sizeof(Common::Protocol::CommandHeader) + m_data.size());
            }

            void processIncomingPacket(std::uint8_t* data, std::uint16_t size, std::uint32_t crypt_key) requires (T == PacketType::ENCRYPTED)
            {
                constexpr std::size_t headerSize = sizeof(Common::Protocol::TcpHeader);
                constexpr std::size_t commandSize = sizeof(Common::Protocol::CommandHeader);

                if (size < headerSize) return;
                Common::Cryptography::Crypt crypt;
                crypt.KeySetup(0);
                crypt.RC5Decrypt32(data, &m_header, headerSize);

                const std::uint16_t messageSize = static_cast<std::uint16_t>(m_header.getSize()) - headerSize;
                if (messageSize <= 0) return;

                std::vector<std::uint8_t> decryptedBytes(data + headerSize, data + headerSize + messageSize);
                switch (static_cast<Common::Enums::EncryptionType>(m_header.getCrypt()))
                {
                case Common::Enums::EncryptionType::NO_ENCRYPTION:
                    break;

                case Common::Enums::EncryptionType::DEFAULT_ENCRYPTION:
                    crypt.KeySetup(0);
                    crypt.RC5Decrypt64(decryptedBytes.data(), decryptedBytes.data(), messageSize);
                    break;

                case Common::Enums::EncryptionType::DEFAULT_LARGE_ENCRYPTION:
                    crypt.KeySetup(0);
                    crypt.RC6Decrypt128(decryptedBytes.data(), decryptedBytes.data(), messageSize);
                    break;

                case Common::Enums::EncryptionType::USER_ENCRYPTION:
                    crypt.KeySetup(crypt_key);
                    crypt.RC5Decrypt64(decryptedBytes.data(), decryptedBytes.data(), messageSize);
                    break;

                case Common::Enums::EncryptionType::USER_LARGE_ENCRYPTION:
                    crypt.KeySetup(crypt_key);
                    crypt.RC6Decrypt128(decryptedBytes.data(), decryptedBytes.data(), messageSize);
                    break;

                default:
                    return;
                }

                std::memcpy(&m_command, decryptedBytes.data(), commandSize);

                if (messageSize > commandSize)
                {
                    setData(decryptedBytes.data() + commandSize, messageSize - commandSize);
                }
                else
                {
                    setData(nullptr, 0);
                }
            }

            void processIncomingPacket(std::uint8_t* data, std::uint16_t size) requires (T == PacketType::UNECRYPTED)
            {
                constexpr std::size_t headerSize = sizeof(Common::Protocol::TcpHeader);
                constexpr std::size_t commandSize = sizeof(Common::Protocol::CommandHeader);
                if (size < headerSize) return;

                std::memcpy(&m_header, data, headerSize);

                const std::uint16_t messageSize = static_cast<std::uint16_t>(m_header.getSize()) - headerSize;
                if (messageSize <= 0 || messageSize > 1450) return;

                std::memcpy(&m_command, data + headerSize, commandSize);

                if (messageSize > commandSize)
                {
                    setData(data + headerSize + commandSize, messageSize - commandSize);
                }
                else
                {
                    setData(nullptr, 0);
                }
            }

            std::vector<std::uint8_t> generateOutgoingPacket(std::uint32_t crypt_key, bool isCryptUsed) const requires (T == PacketType::ENCRYPTED)
            {
                constexpr std::size_t headerSize = sizeof(Common::Protocol::TcpHeader);
                constexpr std::size_t commandSize = sizeof(Common::Protocol::CommandHeader);
                const std::size_t partialSize = commandSize + m_data.size();

                std::vector<std::uint8_t> completeData(headerSize + commandSize + m_data.size(), 0);
                std::memcpy(completeData.data(), &m_header, headerSize);
                std::memcpy(completeData.data() + headerSize, &m_command, commandSize);
                std::memcpy(completeData.data() + headerSize + commandSize, m_data.data(), m_data.size());

                Common::Cryptography::Crypt crypt;

                if (isCryptUsed)
                {
                    crypt.KeySetup(0);
                    crypt.RC5Encrypt32(completeData.data(), completeData.data(), headerSize);
                }

                switch (static_cast<Common::Enums::EncryptionType>(m_header.getCrypt()))
                {
                case Common::Enums::EncryptionType::NO_ENCRYPTION:
                    break;

                case Common::Enums::EncryptionType::DEFAULT_ENCRYPTION:
                    crypt.RC5Encrypt64(completeData.data() + headerSize, completeData.data() + headerSize, partialSize);
                    break;

                case Common::Enums::EncryptionType::DEFAULT_LARGE_ENCRYPTION:
                    crypt.RC6Encrypt128(completeData.data() + headerSize, completeData.data() + headerSize, partialSize);
                    break;

                case Common::Enums::EncryptionType::USER_ENCRYPTION:
                    crypt.KeySetup(crypt_key);
                    crypt.RC5Encrypt64(completeData.data() + headerSize, completeData.data() + headerSize, partialSize);
                    break;

                case Common::Enums::EncryptionType::USER_LARGE_ENCRYPTION:
                    crypt.KeySetup(crypt_key);
                    crypt.RC6Encrypt128(completeData.data() + headerSize, completeData.data() + headerSize, partialSize);
                    break;

                default:
                    // Invalid encryption type
                    break;
                }

                return completeData;
            }

            std::vector<std::uint8_t> generateOutgoingPacket() const requires (T == PacketType::UNECRYPTED)
            {
                constexpr std::size_t headerSize = sizeof(Common::Protocol::TcpHeader);
                constexpr std::size_t commandSize = sizeof(Common::Protocol::CommandHeader);
                const std::size_t partialSize = commandSize + m_data.size();

                std::vector<std::uint8_t> completeData(headerSize + commandSize + m_data.size(), 0);
                std::memcpy(completeData.data(), &m_header, headerSize);
                std::memcpy(completeData.data() + headerSize, &m_command, commandSize);
                std::memcpy(completeData.data() + headerSize + commandSize, m_data.data(), m_data.size());

                return completeData;
            }
        }; 

        using Packet = PacketImpl<PacketType::ENCRYPTED>;
        using UnecryptedPacket = PacketImpl<PacketType::UNECRYPTED>;


    } // end namespace Network
}

#endif