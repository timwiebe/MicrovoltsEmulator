	
#ifndef CDB_MANAGER
#define CDB_MANAGER

#include <initializer_list>
#include <string>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <vector>
#include "../Utils/Logger.h"
#include <cstring> 

namespace Common
{
	namespace ConstantDatabase
	{
		template<typename T>
		class Cdb
		{
		private:
			std::unordered_map<std::uint32_t, T> m_entries_one_to_one; // [key][value] 
			std::unordered_map<std::uint32_t, std::vector<T>> m_entries_one_to_many;  // [key][value1, value2, value3, ...] 
			std::size_t m_entry_num = 0;

			template<typename U>
			friend class Cdb;
			
			template<typename MapType>
			constexpr void insert_entry(MapType& entries, const T& value)
			{
				if constexpr (std::is_same_v<MapType, std::unordered_map<std::uint32_t, T>>) 
				{
					if (entries.contains(value.getId())) 
					{
						::Utils::Logger::log("Duplicate key found in cached CDB entries: " + std::to_string(value.getId()) + ", T = " + typeid(T).name(), 
							::Utils::LogType::Warning);
					}
					entries[value.getId()] = value;
				}
				else if constexpr (std::is_same_v<MapType, std::unordered_map<std::uint32_t, std::vector<T>>>) 
				{
					entries[value.getId()].push_back(value);
				}
			}

			template<typename MapType>
			constexpr void parse_helper(const std::filesystem::path& directory, const std::string& file_name, MapType& entries)
			{
				const std::filesystem::path file_path = directory / file_name;
				std::ifstream input{ file_path.string(), std::ios::binary };

				// 1) Retrieve the total number of keys
				std::uint32_t total_keys = 0;
				input.read(reinterpret_cast<char*>(&total_keys), sizeof(std::uint32_t));

				// Skip header
				const std::size_t header_bytes_num = 4 + total_keys * 34;
				input.seekg(header_bytes_num, std::ios::beg);

				// 2) Retrieve the entries
				std::size_t i = 0;
				while (input.good() && input.peek() != EOF)
				{
					std::array<char, sizeof(T)> buffer;
					input.read(buffer.data(), sizeof(T));

					T value;
					std::memcpy(&value, buffer.data(), sizeof(T));

					insert_entry(entries, value);
					i = i >= total_keys ? 0 : i;
				}
			}

		public:
			constexpr Cdb() = default;

		public:
			constexpr void parse(const std::filesystem::path& directory, const std::string& file_name)
			{
				parse_helper<std::unordered_map<std::uint32_t, T>>(directory, file_name, m_entries_one_to_one);
			}

			constexpr void addEntry(const T& entry)
			{
				m_entries_one_to_one[entry.getId()] = entry;
			}

			constexpr void parse_non_unique_key(const std::filesystem::path& directory, const std::string& file_name)
			{
				parse_helper(directory, file_name, m_entries_one_to_many);
			}

			constexpr void reset()
			{
				m_entries_one_to_one.clear();
				m_entry_num = 0;
			}

			// Searches for the entry by using the primary key's value of the Cdb structure (O(1))
			[[nodiscard]] constexpr const T* getEntry(std::size_t key) const
			{
				auto it = m_entries_one_to_one.find(key);
				if (it != m_entries_one_to_one.end())
				{
					return &(it->second);
				}
				return nullptr;
			}

			constexpr const std::unordered_map<std::uint32_t, T>& getEntries() const
			{
				return m_entries_one_to_one;
			}

			[[nodiscard]] constexpr std::optional<std::vector<T>> getEntriesFor(std::size_t key) const
			{
				auto it = m_entries_one_to_many.find(key);
				if (it != m_entries_one_to_many.end())
				{
					return it->second;
				}
				return std::nullopt;
			}
		};
	}
}

#endif
