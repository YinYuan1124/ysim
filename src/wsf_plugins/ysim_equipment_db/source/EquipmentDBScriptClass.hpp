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
// 二次开发（ext-05）："EquipmentDB" 脚本包装类
//
// 注册到想定脚本类型表后，想定脚本按 WSF 脚本语法使用：
//   EquipmentDB equipdb;
//   bool ok = equipdb.Open("");            // "" = 想定块配置的默认库
//   int n = equipdb.GetEquipmentCount("DataAircraft");
//   Map<string, string> s = equipdb.GetSummary("DataAircraft", 122);
// 完整方法契约见 .trae/specs/ext-05-equipmentdb/spec.md §3.3。

#ifndef YSIM_EQUIPMENTDB_SCRIPTCLASS_HPP
#define YSIM_EQUIPMENTDB_SCRIPTCLASS_HPP

#include <string>

#include "UtScriptClass.hpp"
#include "script/WsfScriptObjectClass.hpp"

namespace ysim
{
namespace equipmentdb
{
   class EquipmentDB;

   class EquipmentDBScriptClass : public WsfScriptObjectClass
   {
   public:
      EquipmentDBScriptClass(const std::string& aClassName, UtScriptTypes* aTypesPtr);

      void*       Create(const UtScriptContext& aInstance) override;
      void        Destroy(void* aObjectPtr) override;
      std::string ToString(void* aObjectPtr) const override;

      UT_DECLARE_SCRIPT_METHOD(Open);
      UT_DECLARE_SCRIPT_METHOD(Close);
      UT_DECLARE_SCRIPT_METHOD(IsOpen);
      UT_DECLARE_SCRIPT_METHOD(GetEquipmentCount);
      UT_DECLARE_SCRIPT_METHOD(FindByName);
      UT_DECLARE_SCRIPT_METHOD(GetLinkedIDs);
      UT_DECLARE_SCRIPT_METHOD(FindFiltered);
      UT_DECLARE_SCRIPT_METHOD(GetSummary);
      UT_DECLARE_SCRIPT_METHOD(GetRecord);
      UT_DECLARE_SCRIPT_METHOD(GetColumnNames);
      UT_DECLARE_SCRIPT_METHOD(GetEnumDescription);
      UT_DECLARE_SCRIPT_METHOD(GetInfo);
      UT_DECLARE_SCRIPT_METHOD(SetMaxRows);
   };
} // namespace equipmentdb
} // namespace ysim

#endif
