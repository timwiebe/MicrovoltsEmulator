#ifndef CDB_SINGLETON_H
#define CDB_SINGLETON_H

#include "Cdb.h"
#include <string>
#include "Structures/CdbItemInfo.h"
#include "Structures/CdbWeaponsInfo.h"
#include "Structures/CdbPackageInfos.h"
#include "Structures/CdbCapsulePackageInfo.h"
#include "Structures/CdbVendor.h"
#include "Structures/CdbItemWeapon.h"
#include <unordered_map>
#include <unordered_set>

namespace Common
{
    namespace ConstantDatabase
    {
        template <typename T>
        class CdbSingleton
        {
        private:
            inline static Cdb<T> m_cdb{};
            inline static std::unordered_map<std::uint32_t, std::vector<std::uint32_t>> m_gambleItems; // [type][itemIds]
            inline static std::unordered_map<std::uint32_t, std::uint32_t> m_itemByType; // [itemId][Type]
            inline static std::unordered_set<std::uint32_t> m_shopItemIds;

        public:
            static const Cdb<T>& getInstance()
            {
                return m_cdb;
            }

            static const auto& getGambleItems()
            {
                return m_gambleItems;
            }

            static std::uint32_t getItemType(std::uint32_t itemId)
            {
                return m_itemByType[itemId];
            }

            static bool itemExistsInShop(std::uint32_t itemId) requires (std::same_as<T, CdbVendorInfo>)
            {
                return m_shopItemIds.contains(itemId);
            }

            static void initialize(const std::string& filePath, const std::string& fileName)
                requires (std::same_as<T, Common::ConstantDatabase::CdbWeaponInfo>)
            {
                m_cdb.parse(filePath, fileName);
                auto entries = m_cdb.getEntries();
                for (const auto& [unused, structType] : entries)
                {
                    if ((strcmp(structType.ii_name_time.data(), "Unlimited") == 0) && structType.ii_is_trade && structType.ii_upgradable)
                    {
                        m_gambleItems[structType.ii_type].push_back(static_cast<std::uint32_t>(structType.ii_id));
                    }
                }
            }

            static void initializeItemTypes(const std::string& filePath, const std::string& weaponFileName, const std::string& itemFileName)
                requires (std::same_as<T, Common::ConstantDatabase::CdbItemInfo>)
            {
                Cdb<CdbWeaponInfo> cdbWeapon{};
                cdbWeapon.parse(filePath, weaponFileName);
                auto entries = cdbWeapon.getEntries();
                for (const auto& [unused, structType] : entries)
                {
                    m_itemByType[structType.ii_id] = structType.ii_type;
                }

                Cdb<CdbItemInfo> cdbItem{};
                cdbItem.parse(filePath, itemFileName);
                auto entries2 = cdbItem.getEntries();
                for (const auto& [unused, structType] : entries2)
                {
                    m_itemByType[structType.ii_id] = structType.ii_type;
                }
            }

            static void initialize(const std::string& filePath, const std::string& fileName)
                requires (std::same_as<T, Common::ConstantDatabase::CdbVendorInfo>)
            {
                Cdb<CdbVendorInfo> cdbVendorInfo{};
                cdbVendorInfo.parse(filePath, fileName);
                for (const auto& [vendorEntryId, structType] : cdbVendorInfo.getEntries())
                {
                    m_shopItemIds.insert(structType.vi_list_01);
                    m_shopItemIds.insert(structType.vi_list_01_a);
                    m_shopItemIds.insert(structType.vi_list_01_b);
                    m_shopItemIds.insert(structType.vi_list_01_c);
                    m_shopItemIds.insert(structType.vi_list_01_d);
                    m_shopItemIds.insert(structType.vi_list_02);
                    m_shopItemIds.insert(structType.vi_list_02_a);
                    m_shopItemIds.insert(structType.vi_list_02_b);
                    m_shopItemIds.insert(structType.vi_list_02_c);
                    m_shopItemIds.insert(structType.vi_list_02_d);
                    m_shopItemIds.insert(structType.vi_list_03);
                    m_shopItemIds.insert(structType.vi_list_03_a);
                    m_shopItemIds.insert(structType.vi_list_03_b);
                    m_shopItemIds.insert(structType.vi_list_03_c);
                    m_shopItemIds.insert(structType.vi_list_03_d);
                    m_shopItemIds.insert(structType.vi_list_04);
                    m_shopItemIds.insert(structType.vi_list_04_a);
                    m_shopItemIds.insert(structType.vi_list_04_b);
                    m_shopItemIds.insert(structType.vi_list_04_c);
                    m_shopItemIds.insert(structType.vi_list_04_d);
                }
            }

            static void initialize(const std::string& filePath, const std::string& fileName)
                requires (std::same_as<T, Common::ConstantDatabase::CdbItemInfo>)
            {
                m_cdb.parse(filePath, fileName);
                auto entries = m_cdb.getEntries();
                for (const auto& [unused, structType] : entries)
                {
                    if ((strcmp(structType.ii_name_time.data(), "Unlimited") == 0) && structType.ii_is_trade)
                    {
                        m_gambleItems[structType.ii_type].push_back(static_cast<std::uint32_t>(structType.ii_id));
                    }
                }
            }

            static void initialize(const std::string& filePath, const std::string& fileName)
                requires (std::same_as<T, Common::ConstantDatabase::CdbItemPackageInfo> or std::same_as<T, Common::ConstantDatabase::CdbWeaponPackageInfo>
                or std::same_as<T, Common::ConstantDatabase::CdbCapsulePackageInfo>)
            {
                m_cdb.parse_non_unique_key(filePath, fileName);
            }

            static void initialize(const std::string& filePath, const std::string& fileName)
                requires (not std::same_as<T, Common::ConstantDatabase::CdbItemInfo> and not std::same_as<T, Common::ConstantDatabase::CdbWeaponInfo>
                and not std::same_as<T, Common::ConstantDatabase::CdbItemPackageInfo> and not std::same_as<T, Common::ConstantDatabase::CdbCapsulePackageInfo>
                and not std::same_as<T, Common::ConstantDatabase::CdbVendorInfo>)
            {
                m_cdb.parse(filePath, fileName);
            }

            static void initialize(const Cdb<Common::ConstantDatabase::CdbItemInfo>& itemInfo,
                const Cdb<Common::ConstantDatabase::CdbWeaponInfo>& weaponInfo)
            {
                for (const auto& itemEntry : itemInfo.getEntries())
                {
                    m_cdb.addEntry(itemEntry.second);
                }
                for (const auto& weaponEntry : weaponInfo.getEntries())
                {
                    m_cdb.addEntry(weaponEntry.second);
                }
            }

            static void filterGambleItemsByRareCapsules(const std::vector<Common::ConstantDatabase::CdbCapsulePackageInfo>& rareCapsuleItems)
            {
                std::unordered_set<std::uint32_t> rareIds;
                rareIds.reserve(rareCapsuleItems.size());
                for (const auto& capsule : rareCapsuleItems)
                    rareIds.insert(capsule.gi_itemid);

                for (auto& [type, itemList] : m_gambleItems)
                {
                    itemList.erase(std::remove_if(itemList.begin(), itemList.end(),
                        [&](std::uint32_t id) { return !rareIds.contains(id); }),
                        itemList.end());
                }
            }

            CdbSingleton(CdbSingleton const&) = delete;
            void operator=(CdbSingleton const&) = delete;

        private:
            CdbSingleton()
            {
            }
        };
    }
}
#endif
