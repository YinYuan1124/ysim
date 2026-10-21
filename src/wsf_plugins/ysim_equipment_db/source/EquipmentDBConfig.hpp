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
// 二次开发（ext-05）：场景配置共享窄契约
//
// YsimEquipmentDB.cpp（场景扩展）与 EquipmentDBScriptClass.cpp（脚本类）之间
// 只经由本头文件交互，脚本类不感知扩展具体类型（可维护性：双向解耦）。

#ifndef YSIM_EQUIPMENTDB_CONFIG_HPP
#define YSIM_EQUIPMENTDB_CONFIG_HPP

#include <string>

class WsfScenario;

namespace ysim
{
namespace equipmentdb
{
   //! 想定输入块 ysim_equipment_db 的配置数据
   struct DbConfig
   {
      std::string mDatabasePath;      //!< database：默认库路径（原样值，可为相对路径）
      std::string mScenarioDir;       //!< 首个加载想定文件所在目录（相对路径解析基准）
      int         mMaxRows      = 100; //!< max_rows：FindByName 结果上限
      bool        mPreload      = true; //!< preload：想定加载期预打开验证
   };

   //! 配置提供接口（由场景扩展实现）
   class ConfigProvider
   {
   public:
      virtual ~ConfigProvider() = default;
      virtual const DbConfig& GetDbConfig() const = 0;
   };

   //! 查找场景配置；无 ysim_equipment_db 扩展注册时返回 nullptr
   const DbConfig* FindScenarioConfig(WsfScenario& aScenario);
} // namespace equipmentdb
} // namespace ysim

#endif
