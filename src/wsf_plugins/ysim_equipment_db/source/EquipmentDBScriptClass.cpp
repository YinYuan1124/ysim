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
// 二次开发（ext-05）：EquipmentDB 脚本包装实现
//
// 返回容器模式均有核心先例：
//   Array 返回 —— WsfScriptGroupClass::MembersByName（vector<UtScriptData> + cMANAGE）
//   Map 返回   —— WsfScriptTrackClass::Types（Map<string,string> 按需取模板类）

#include "EquipmentDBScriptClass.hpp"

#include <string>
#include <vector>

#include "EquipmentDB.hpp"
#include "EquipmentDBConfig.hpp"
#include "UtLog.hpp"
#include "UtMemory.hpp"
#include "UtScriptBasicTypes.hpp"
#include "UtScriptContext.hpp"
#include "UtScriptData.hpp"
#include "UtScriptTypes.hpp"
#include "WsfScenario.hpp"
#include "script/WsfScriptContext.hpp"

namespace
{
   //! Map<string,string> 模板脚本类（按需实例化，WsfScriptTrackClass::Types 同款模式）
   UtScriptMap& GetMapClass(UtScriptContext& aContext)
   {
      static UtScriptMap& sScriptMapClass = (UtScriptMap&)(*aContext.GetTypes()->GetClass("Map<string,string>"));
      return sScriptMapClass;
   }
} // namespace

namespace ysim
{
namespace equipmentdb
{
   EquipmentDBScriptClass::EquipmentDBScriptClass(const std::string& aClassName, UtScriptTypes* aTypesPtr)
      : WsfScriptObjectClass(aClassName, aTypesPtr)
   {
      SetClassName("EquipmentDB");
      mConstructible = true;

      AddMethod(ut::make_unique<Open>());
      AddMethod(ut::make_unique<Close>());
      AddMethod(ut::make_unique<IsOpen>());
      AddMethod(ut::make_unique<GetEquipmentCount>());
      AddMethod(ut::make_unique<FindByName>());
      AddMethod(ut::make_unique<GetLinkedIDs>());
      AddMethod(ut::make_unique<FindFiltered>());
      AddMethod(ut::make_unique<GetSummary>());
      AddMethod(ut::make_unique<GetRecord>());
      AddMethod(ut::make_unique<GetColumnNames>());
      AddMethod(ut::make_unique<GetEnumDescription>());
      AddMethod(ut::make_unique<GetInfo>());
      AddMethod(ut::make_unique<SetMaxRows>());
   }

   void* EquipmentDBScriptClass::Create(const UtScriptContext& aInstance)
   {
      auto* dbPtr = new EquipmentDB();
      // 场景级默认 max_rows（想定输入块配置；无扩展配置时保持对象默认 100）
      WsfScenario* scenarioPtr = WsfScriptContext::GetSCENARIO(aInstance);
      if (scenarioPtr != nullptr)
      {
         const DbConfig* cfgPtr = FindScenarioConfig(*scenarioPtr);
         if (cfgPtr != nullptr)
         {
            dbPtr->SetMaxRows(cfgPtr->mMaxRows);
         }
      }
      return dbPtr;
   }

   void EquipmentDBScriptClass::Destroy(void* aObjectPtr)
   {
      delete static_cast<EquipmentDB*>(aObjectPtr);
   }

   std::string EquipmentDBScriptClass::ToString(void* aObjectPtr) const
   {
      auto* dbPtr = static_cast<EquipmentDB*>(aObjectPtr);
      return std::string("EquipmentDB(") + dbPtr->GetPath() + ")";
   }

   // bool Open(string aPath)：空路径 = 想定块配置的默认库；相对路径相对想定文件目录解析
   UT_DEFINE_SCRIPT_METHOD(EquipmentDBScriptClass, ysim::equipmentdb::EquipmentDB, Open, 1, "bool", "string")
   {
      auto*       dbPtr  = aObjectPtr;
      std::string path   = aVarArgs[0].GetString();
      std::string baseDir;
      if (path.empty())
      {
         WsfScenario* scenarioPtr = WsfScriptContext::GetSCENARIO(aContext);
         if (scenarioPtr != nullptr)
         {
            const DbConfig* cfgPtr = FindScenarioConfig(*scenarioPtr);
            if (cfgPtr != nullptr)
            {
               path    = cfgPtr->mDatabasePath;
               baseDir = cfgPtr->mScenarioDir;
            }
         }
         if (path.empty())
         {
            auto out = ut::log::warning()
                      << "EquipmentDB.Open: no database path given and no scenario default configured.";
            aReturnVal.SetBool(false);
            return;
         }
      }
      dbPtr->SetBaseDir(baseDir);
      aReturnVal.SetBool(dbPtr->Open(path));
   }

   // void Close()
   UT_DEFINE_SCRIPT_METHOD(EquipmentDBScriptClass, ysim::equipmentdb::EquipmentDB, Close, 0, "void", "")
   {
      aObjectPtr->Close();
   }

   // bool IsOpen()
   UT_DEFINE_SCRIPT_METHOD(EquipmentDBScriptClass, ysim::equipmentdb::EquipmentDB, IsOpen, 0, "bool", "")
   {
      aReturnVal.SetBool(aObjectPtr->IsOpen());
   }

   // int GetEquipmentCount(string aTableName)："" 或 "*" = 总数；出错 -1
   UT_DEFINE_SCRIPT_METHOD(EquipmentDBScriptClass, ysim::equipmentdb::EquipmentDB, GetEquipmentCount, 1, "int", "string")
   {
      aReturnVal.SetInt(aObjectPtr->GetEquipmentCount(aVarArgs[0].GetString()));
   }

   // Array<int> FindByName(string aPattern, string aTableName)：LIKE %pattern%，匹配 Name/Name_CN；表名空 = 全部类
   UT_DEFINE_SCRIPT_METHOD(EquipmentDBScriptClass, ysim::equipmentdb::EquipmentDB, FindByName, 2, "Array<int>", "string, string")
   {
      std::vector<int> ids = aObjectPtr->FindByName(aVarArgs[0].GetString(), aVarArgs[1].GetString());
      auto*            resultVecPtr = new std::vector<UtScriptData>();
      resultVecPtr->reserve(ids.size());
      for (int id : ids)
      {
         resultVecPtr->emplace_back(id);
      }
      aReturnVal.SetPointer(new UtScriptRef(resultVecPtr, aReturnClassPtr, UtScriptRef::cMANAGE));
   }

   // Array<int> GetLinkedIDs(string aLinkTable, int aParentID)：链接表约定 ID→ComponentID（spec §2.5/§3.3）
   UT_DEFINE_SCRIPT_METHOD(EquipmentDBScriptClass, ysim::equipmentdb::EquipmentDB, GetLinkedIDs, 2, "Array<int>", "string, int")
   {
      std::vector<int> ids = aObjectPtr->GetLinkedIDs(aVarArgs[0].GetString(), aVarArgs[1].GetInt());
      auto*            resultVecPtr = new std::vector<UtScriptData>();
      resultVecPtr->reserve(ids.size());
      for (int id : ids)
      {
         resultVecPtr->emplace_back(id);
      }
      aReturnVal.SetPointer(new UtScriptRef(resultVecPtr, aReturnClassPtr, UtScriptRef::cMANAGE));
   }

   // Array<int> FindFiltered(string aTable, string aName, string aCountry, string aCategory, string aType, int aYearFrom, int aYearTo)
   // 组合过滤：空串通配/年代 0 不设限；双语 LIKE + 年代闭区间；上限 max_rows
   UT_DEFINE_SCRIPT_METHOD(EquipmentDBScriptClass,
                           ysim::equipmentdb::EquipmentDB,
                           FindFiltered,
                           7,
                           "Array<int>",
                           "string, string, string, string, string, int, int")
   {
      std::vector<int> ids = aObjectPtr->FindFiltered(aVarArgs[0].GetString(),
                                                      aVarArgs[1].GetString(),
                                                      aVarArgs[2].GetString(),
                                                      aVarArgs[3].GetString(),
                                                      aVarArgs[4].GetString(),
                                                      aVarArgs[5].GetInt(),
                                                      aVarArgs[6].GetInt());
      auto* resultVecPtr = new std::vector<UtScriptData>();
      resultVecPtr->reserve(ids.size());
      for (int id : ids)
      {
         resultVecPtr->emplace_back(id);
      }
      aReturnVal.SetPointer(new UtScriptRef(resultVecPtr, aReturnClassPtr, UtScriptRef::cMANAGE));
   }

   // Map<string,string> GetSummary(string aTableName, int aID)：DataEquipmentSummary 整行；查无 → 空 Map
   UT_DEFINE_SCRIPT_METHOD(EquipmentDBScriptClass, ysim::equipmentdb::EquipmentDB, GetSummary, 2, "Map<string,string>", "string, int")
   {
      Record rec = aObjectPtr->GetSummary(aVarArgs[0].GetString(), aVarArgs[1].GetInt());
      auto*  mapPtr = new UtScriptMap::Map();
      for (const auto& kv : rec)
      {
         (*mapPtr)[UtScriptData(kv.first)] = UtScriptData(kv.second);
      }
      aReturnVal.SetPointer(new UtScriptRef(mapPtr, &GetMapClass(aContext), UtScriptRef::cMANAGE));
   }

   // Map<string,string> GetRecord(string aTableName, int aID)：明细表整行；查无 → 空 Map
   UT_DEFINE_SCRIPT_METHOD(EquipmentDBScriptClass, ysim::equipmentdb::EquipmentDB, GetRecord, 2, "Map<string,string>", "string, int")
   {
      Record rec = aObjectPtr->GetRecord(aVarArgs[0].GetString(), aVarArgs[1].GetInt());
      auto*  mapPtr = new UtScriptMap::Map();
      for (const auto& kv : rec)
      {
         (*mapPtr)[UtScriptData(kv.first)] = UtScriptData(kv.second);
      }
      aReturnVal.SetPointer(new UtScriptRef(mapPtr, &GetMapClass(aContext), UtScriptRef::cMANAGE));
   }

   // Array<string> GetColumnNames(string aTableName)
   UT_DEFINE_SCRIPT_METHOD(EquipmentDBScriptClass, ysim::equipmentdb::EquipmentDB, GetColumnNames, 1, "Array<string>", "string")
   {
      std::vector<std::string> names = aObjectPtr->GetColumnNames(aVarArgs[0].GetString());
      auto*                    resultVecPtr = new std::vector<UtScriptData>();
      resultVecPtr->reserve(names.size());
      for (const std::string& name : names)
      {
         resultVecPtr->emplace_back(name);
      }
      aReturnVal.SetPointer(new UtScriptRef(resultVecPtr, aReturnClassPtr, UtScriptRef::cMANAGE));
   }

   // string GetEnumDescription(string aEnumTable, int aCode)：查无 → ""
   UT_DEFINE_SCRIPT_METHOD(EquipmentDBScriptClass, ysim::equipmentdb::EquipmentDB, GetEnumDescription, 2, "string", "string, int")
   {
      aReturnVal.SetString(aObjectPtr->GetEnumDescription(aVarArgs[0].GetString(), aVarArgs[1].GetInt()));
   }

   // string GetInfo()：诊断串（路径/状态/上限/总数）
   UT_DEFINE_SCRIPT_METHOD(EquipmentDBScriptClass, ysim::equipmentdb::EquipmentDB, GetInfo, 0, "string", "")
   {
      aReturnVal.SetString(aObjectPtr->GetInfo());
   }

   // void SetMaxRows(int aMaxRows)
   UT_DEFINE_SCRIPT_METHOD(EquipmentDBScriptClass, ysim::equipmentdb::EquipmentDB, SetMaxRows, 1, "void", "int")
   {
      aObjectPtr->SetMaxRows(aVarArgs[0].GetInt());
   }
} // namespace equipmentdb
} // namespace ysim
