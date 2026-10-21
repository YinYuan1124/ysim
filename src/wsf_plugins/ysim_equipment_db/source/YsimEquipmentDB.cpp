// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// Copyright 2003-2015 The Boeing Company. All rights reserved.
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************
// 二次开发（ext-05）：ysim_equipment_db 插件入口
//
// 组成：应用扩展（向场景注册场景扩展）+ 场景扩展（想定输入块解析、
//       "EquipmentDB" 脚本类注册、preload 预打开校验）+ 插件导出符号。
// 想定输入块（想定关键字保持英文原样，不翻译）：
//   ysim_equipment_db
//      database <path>    # 默认库路径（相对想定文件目录解析）
//      max_rows <n>        # FindByName 结果上限（默认 100）
//      preload true|false  # 想定加载期预打开验证（默认 true，失败仅告警不中止）
//   end_ysim_equipment_db

#include <memory>
#include <string>

#include "EquipmentDB.hpp"
#include "EquipmentDBConfig.hpp"
#include "EquipmentDBScriptClass.hpp"
#include "UtInput.hpp"
#include "UtInputBlock.hpp"
#include "UtLog.hpp"
#include "UtMemory.hpp"
#include "WsfApplication.hpp"
#include "WsfApplicationExtension.hpp"
#include "WsfPlugin.hpp"
#include "WsfScenario.hpp"
#include "WsfScenarioExtension.hpp"

namespace
{
   constexpr char cEXTENSION_NAME[] = "ysim_equipment_db";

   //! 取文件父目录（兼容 / 与 \）；无分隔符返回空串
   std::string DirName(const std::string& aPath)
   {
      std::string::size_type pos = aPath.find_last_of("/\\");
      return (pos == std::string::npos) ? std::string() : aPath.substr(0, pos);
   }

   //! 场景扩展：想定输入块解析 + 脚本类注册 + preload 预打开校验
   class ScenarioExtension : public WsfScenarioExtension, public ysim::equipmentdb::ConfigProvider
   {
   public:
      ScenarioExtension()           = default;
      ~ScenarioExtension() override = default;

      const ysim::equipmentdb::DbConfig& GetDbConfig() const override { return mData; }

      void AddedToScenario() override
      {
         // 注册脚本类 "EquipmentDB"（wsf_oms_uci 同款注册模式）
         WsfScenario& scenario = GetScenario();
         scenario.GetScriptTypes()->Register(
            ut::make_unique<ysim::equipmentdb::EquipmentDBScriptClass>("EquipmentDB", scenario.GetScriptTypes()));
      }

      void FileLoaded(const std::string& aFileName) override
      {
         // 首个加载文件 = 主想定文件；记录其目录作为相对路径解析基准
         if (mData.mScenarioDir.empty() && !aFileName.empty())
         {
            mData.mScenarioDir = DirName(aFileName);
         }
      }

      bool ProcessInput(UtInput& aInput) override
      {
         std::string command = aInput.GetCommand();
         if (command != cEXTENSION_NAME)
         {
            return false;
         }
         // UtInputBlock 空终止符自动推导为 "end_" + 当前命令名（ext-01 同款）
         UtInputBlock block(aInput);
         while (block.ReadCommand())
         {
            std::string subCommand = aInput.GetCommand();
            if (subCommand == "database")
            {
               aInput.ReadValueQuoted(mData.mDatabasePath);
            }
            else if (subCommand == "max_rows")
            {
               aInput.ReadValue(mData.mMaxRows);
               if (mData.mMaxRows < 1)
               {
                  throw UtInput::BadValue(aInput, "ysim_equipment_db: max_rows must be >= 1");
               }
            }
            else if (subCommand == "preload")
            {
               aInput.ReadValue(mData.mPreload);
            }
            else
            {
               throw UtInput::UnknownCommand(aInput);
            }
         }
         return true;
      }

      bool Complete() override
      {
         // preload 预打开校验：失败仅告警不中止想定加载（spec §3.4 防崩溃原则）
         if (mData.mPreload && !mData.mDatabasePath.empty())
         {
            ysim::equipmentdb::EquipmentDB db;
            db.SetBaseDir(mData.mScenarioDir);
            if (db.Open(mData.mDatabasePath))
            {
               auto out = ut::log::info()
                         << "ysim_equipment_db: preloaded " << db.GetPath() << " ("
                         << db.GetEquipmentCount("") << " equipment records).";
               db.Close();
            }
            // Open 失败时其内部已记录告警；此处不中止加载
         }
         return true;
      }

   private:
      ysim::equipmentdb::DbConfig mData;
   };

   //! 应用扩展：场景创建时注册场景扩展
   class ApplicationExtension : public WsfApplicationExtension
   {
   public:
      void ScenarioCreated(WsfScenario& aScenario) override
      {
         aScenario.RegisterExtension(cEXTENSION_NAME, ut::make_unique<ScenarioExtension>());
      }
   };
} // namespace

namespace ysim
{
namespace equipmentdb
{
   const DbConfig* FindScenarioConfig(WsfScenario& aScenario)
   {
      // 窄契约实现：经 ConfigProvider 接口跨类型取得配置，脚本类不感知扩展类型
      WsfScenarioExtension* extPtr      = aScenario.FindExtension("ysim_equipment_db");
      ConfigProvider*       providerPtr = dynamic_cast<ConfigProvider*>(extPtr);
      return (providerPtr != nullptr) ? &providerPtr->GetDbConfig() : nullptr;
   }
} // namespace equipmentdb
} // namespace ysim

// ----------------------------------------------------------------------------
// 插件注册入口（WsfPluginManager 经导出符号自动调用）
// ----------------------------------------------------------------------------

void Register_ysim_equipment_db(WsfApplication& aApplication)
{
   if (!aApplication.ExtensionIsRegistered(cEXTENSION_NAME))
   {
      aApplication.RegisterExtension(cEXTENSION_NAME, ut::make_unique<ApplicationExtension>());
   }
}

extern "C"
{
   UT_PLUGIN_EXPORT void WsfPluginVersion(UtPluginVersion& aVersion)
   {
      aVersion = UtPluginVersion(WSF_PLUGIN_API_MAJOR_VERSION, WSF_PLUGIN_API_MINOR_VERSION, WSF_PLUGIN_API_COMPILER_STRING);
   }

   UT_PLUGIN_EXPORT void WsfPluginSetup(WsfApplication& aApplication)
   {
      Register_ysim_equipment_db(aApplication);
   }
}
