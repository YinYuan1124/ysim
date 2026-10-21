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
// 二次开发（ext-05 G 阶段）：EquipmentDB 单元测试（GTest，仿 six_dof_test 模式）
//
// 设计要点（.trae/specs/ext-05-equipmentdb/spec.md §4.2）：
//   * 直测领域类 EquipmentDB —— 脚本方法的逻辑主体（脚本包装为薄封送，行为由
//     tests/equipmentdb 的 37 项端到端断言覆盖）
//   * 自建微型 SQLite 夹具库（本文件内以 sqlite3 C API 生成于 CWD，测试结束删除）：
//     确定性断言，不依赖 51MB 运行库 DB3K_modern.db3
//   * 中文值一律用显式 UTF-8 字节转义（\xE6...），规避 MSVC 窄字面量 GBK 执行编码坑
//   * 夹具行语义见下方 INSERT 注释：国别/类别/型别/年代含 NULL 行，验证自然排除；
//     含 '%' 字面量行，验证 LIKE ESCAPE 转义

#include <cstdio>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "EquipmentDB.hpp"
#include "sqlite3.h"

namespace
{
   // 夹具库文件名（生成于测试进程 CWD，套件结束时删除）
   const char cFixturePath[] = "ysim_eqdb_unit_fixture.db";

   // "美国" UTF-8 字节（显式转义，不经执行代码页）
   const char cUS_CN[] = "\xE7\xBE\x8E\xE5\x9B\xBD";
   // "战隼" UTF-8 字节
   const char cZHANJI[] = "\xE6\x88\x98\xE9\x9A\xBC";

   bool Exec(sqlite3* aDB, const char* aSQL)
   {
      char* err = nullptr;
      if (sqlite3_exec(aDB, aSQL, nullptr, nullptr, &err) != SQLITE_OK)
      {
         ADD_FAILURE() << "fixture SQL failed: " << ((err != nullptr) ? err : "?") << "\n  SQL: " << aSQL;
         sqlite3_free(err);
         return false;
      }
      return true;
   }

   // 夹具库：DataEquipmentSummary 8 行（7 架飞机 + 1 个传感器）+ 链接表 + 枚举表 + 明细表
   // 行语义：1/2 美国 F-16（含中文名/中文国别），3 俄国无中文国别，4 俄国 NULL 型别/年代，
   //        5 NULL 国别，6 俄国 MiG-29，7 名含 '%' 字面量，50 传感器（跨表过滤用）
   bool CreateFixtureDb()
   {
      std::remove(cFixturePath);
      sqlite3* db = nullptr;
      if (sqlite3_open(cFixturePath, &db) != SQLITE_OK)
      {
         sqlite3_close(db);
         ADD_FAILURE() << "cannot create fixture db";
         return false;
      }
      const char* ddl[] = {
         "CREATE TABLE DataEquipmentSummary ("
         "TableName TEXT, ID INTEGER, Name TEXT, Name_CN TEXT, "
         "CountryName TEXT, CountryName_CN TEXT, "
         "CategoryName TEXT, CategoryName_CN TEXT, "
         "TypeName TEXT, TypeName_CN TEXT, YearCommissioned INTEGER)",

         "INSERT INTO DataEquipmentSummary VALUES "
         "('DataAircraft',1,'F-16A Falcon','F-16 \xE6\x88\x98\xE9\x9A\xBC',"
         "'United States','\xE7\xBE\x8E\xE5\x9B\xBD',"
         "'Fixed Wing','Fixed Wing','Fighter','Fighter',1978)",
         "INSERT INTO DataEquipmentSummary VALUES "
         "('DataAircraft',2,'F-16C Falcon','F-16C','United States','\xE7\xBE\x8E\xE5\x9B\xBD',"
         "'Fixed Wing','Fixed Wing','Fighter','Fighter',1984)",
         "INSERT INTO DataEquipmentSummary VALUES "
         "('DataAircraft',3,'Tu-22RDK','Tu-22RDK','Russia',NULL,'Fixed Wing',NULL,'Bomber',NULL,1976)",
         "INSERT INTO DataEquipmentSummary VALUES "
         "('DataAircraft',4,'Ka-27',NULL,'Russia',NULL,'Helicopter',NULL,NULL,NULL,NULL)",
         "INSERT INTO DataEquipmentSummary VALUES "
         "('DataAircraft',5,'AH-64 Apache','AH-64',NULL,NULL,'Helicopter',NULL,NULL,NULL,1986)",
         "INSERT INTO DataEquipmentSummary VALUES "
         "('DataAircraft',6,'MiG-29','MiG-29','Russia',NULL,'Fixed Wing',NULL,'Fighter',NULL,1982)",
         "INSERT INTO DataEquipmentSummary VALUES "
         "('DataAircraft',7,'100% Bob',NULL,NULL,NULL,'Fixed Wing',NULL,NULL,NULL,1990)",
         "INSERT INTO DataEquipmentSummary VALUES "
         "('DataSensor',50,'APG-68 Radar',NULL,'United States',NULL,'Sensor',NULL,NULL,NULL,1980)",

         // 链接表：约定 ID → ComponentID（spec §2.5）；父 2 有 6 组件（截断用）
         "CREATE TABLE DataAircraftSensors (ID INTEGER, ComponentID INTEGER)",
         "INSERT INTO DataAircraftSensors VALUES (1,10)",
         "INSERT INTO DataAircraftSensors VALUES (1,11)",
         "INSERT INTO DataAircraftSensors VALUES (1,12)",
         "INSERT INTO DataAircraftSensors VALUES (2,30)",
         "INSERT INTO DataAircraftSensors VALUES (2,31)",
         "INSERT INTO DataAircraftSensors VALUES (2,32)",
         "INSERT INTO DataAircraftSensors VALUES (2,33)",
         "INSERT INTO DataAircraftSensors VALUES (2,34)",
         "INSERT INTO DataAircraftSensors VALUES (2,35)",
         "INSERT INTO DataAircraftSensors VALUES (3,20)",

         "CREATE TABLE EnumAircraftType (ID INTEGER, Description TEXT)",
         "INSERT INTO EnumAircraftType VALUES (2002,'Multirole (Fighter/Attack)')",
         "INSERT INTO EnumAircraftType VALUES (1,'Fighter')",

         "CREATE TABLE DataAircraft (ID INTEGER, Name TEXT)",
         "INSERT INTO DataAircraft VALUES (1,'F-16A Falcon')",
      };
      bool ok = true;
      for (const char* sql : ddl)
      {
         ok = Exec(db, sql) && ok;
      }
      sqlite3_close(db);
      return ok;
   }
} // namespace

class EquipmentDBTest : public ::testing::Test
{
protected:
   // GTest 1.8.0 命名（SetUpTestSuite 为 1.10+ 改名，本仓库 3rd_party 锁 1.8）
   static void SetUpTestCase()
   {
      sFixtureReady = CreateFixtureDb();
      ASSERT_TRUE(sFixtureReady);
   }

   static void TearDownTestCase()
   {
      if (sFixtureReady)
      {
         std::remove(cFixturePath);
      }
   }

   void SetUp() override
   {
      mDb = new ysim::equipmentdb::EquipmentDB();
      ASSERT_TRUE(mDb->Open(cFixturePath));
   }

   void TearDown() override
   {
      delete mDb;
   }

   ysim::equipmentdb::EquipmentDB* mDb = nullptr;
   static bool                      sFixtureReady;
};

bool EquipmentDBTest::sFixtureReady = false;

using ysim::equipmentdb::EquipmentDB;
using Ids = std::vector<int>;

// ---- 打开/关闭与错误语义 ----

TEST_F(EquipmentDBTest, OpenAndCloseLifecycle)
{
   EXPECT_TRUE(mDb->IsOpen());
   EXPECT_NE(mDb->GetInfo().find("open"), std::string::npos);
   mDb->Close();
   EXPECT_FALSE(mDb->IsOpen());
   mDb->Close(); // 幂等，不崩溃
   EXPECT_TRUE(mDb->Open(cFixturePath)); // 重开
   EXPECT_TRUE(mDb->IsOpen());
}

TEST_F(EquipmentDBTest, OpenMissingFileFails)
{
   EquipmentDB db;
   EXPECT_FALSE(db.Open("ysim_eqdb_no_such_file.db"));
   EXPECT_FALSE(db.IsOpen());
}

TEST_F(EquipmentDBTest, OpenNonSqliteFileFails)
{
   const char* garbage = "ysim_eqdb_not_sqlite.bin";
   FILE*       f        = std::fopen(garbage, "wb");
   ASSERT_NE(f, nullptr);
   std::fwrite("THIS IS NOT A DATABASE", 1, 23, f);
   std::fclose(f);
   EquipmentDB db;
   EXPECT_FALSE(db.Open(garbage)); // 文件头校验拦截
   EXPECT_FALSE(db.IsOpen());
   std::remove(garbage);
}

TEST_F(EquipmentDBTest, UnopenedQueriesReturnErrorValues)
{
   EquipmentDB db; // 未打开
   EXPECT_EQ(db.GetEquipmentCount("DataAircraft"), -1);
   EXPECT_TRUE(db.FindByName("F-16", "").empty());
   EXPECT_TRUE(db.GetLinkedIDs("DataAircraftSensors", 1).empty());
   EXPECT_TRUE(db.FindFiltered("DataAircraft", "", "", "", "", 0, 0).empty());
   EXPECT_TRUE(db.GetSummary("DataAircraft", 1).empty());
   EXPECT_TRUE(db.GetRecord("DataAircraft", 1).empty());
   EXPECT_TRUE(db.GetColumnNames("DataAircraft").empty());
   EXPECT_EQ(db.GetEnumDescription("EnumAircraftType", 2002), "");
}

// ---- 标识符白名单 ----

TEST_F(EquipmentDBTest, IdentifierValidation)
{
   EXPECT_TRUE(EquipmentDB::IsValidIdentifier("DataAircraft"));
   EXPECT_TRUE(EquipmentDB::IsValidIdentifier("_x9"));
   EXPECT_TRUE(EquipmentDB::IsValidIdentifier("a1_B2"));
   EXPECT_FALSE(EquipmentDB::IsValidIdentifier(""));
   EXPECT_FALSE(EquipmentDB::IsValidIdentifier("9abc"));   // 数字开头
   EXPECT_FALSE(EquipmentDB::IsValidIdentifier("Bad!"));    // 特殊字符
   EXPECT_FALSE(EquipmentDB::IsValidIdentifier("a b"));     // 空格
   EXPECT_FALSE(EquipmentDB::IsValidIdentifier("a-b"));    // 连字符
   EXPECT_FALSE(EquipmentDB::IsValidIdentifier(std::string(129, 'x'))); // 超长
}

// ---- 计数 ----

TEST_F(EquipmentDBTest, EquipmentCountBasic)
{
   EXPECT_EQ(mDb->GetEquipmentCount("DataAircraft"), 7);
   EXPECT_EQ(mDb->GetEquipmentCount("DataSensor"), 1);
   EXPECT_EQ(mDb->GetEquipmentCount("*"), 8);
   EXPECT_EQ(mDb->GetEquipmentCount(""), 8); // 空串 = 总数
}

TEST_F(EquipmentDBTest, EquipmentCountInvalidTable)
{
   EXPECT_EQ(mDb->GetEquipmentCount("NotATable!"), -1); // 白名单拦截，非崩溃
   EXPECT_EQ(mDb->GetEquipmentCount("NoSuchTable"), 0); // 合法表名：按 TableName 过滤计数，无记录类 = 0
}

// ---- FindByName ----

TEST_F(EquipmentDBTest, FindByNameMatchesEnglishAndChinese)
{
   EXPECT_EQ(mDb->FindByName("F-16", ""), (Ids{1, 2}));           // 英文名
   EXPECT_EQ(mDb->FindByName("F-16", "DataAircraft"), (Ids{1, 2})); // 表过滤
   EXPECT_EQ(mDb->FindByName(cZHANJI, ""), (Ids{1}));              // 中文列命中（Name_CN）
   EXPECT_EQ(mDb->FindByName("APG-68", ""), (Ids{50}));            // 跨表
}

TEST_F(EquipmentDBTest, FindByNameTableFilterExcludesOtherTables)
{
   EXPECT_TRUE(mDb->FindByName("F-16", "DataSensor").empty());
   EXPECT_EQ(mDb->FindByName("APG-68", "DataSensor"), (Ids{50}));
}

TEST_F(EquipmentDBTest, FindByNameEscapesPercentWildcards)
{
   // "100%" 被转义为字面量：只命中名称含 "100%" 的行，而非 "100+任意字符"
   EXPECT_EQ(mDb->FindByName("100%", ""), (Ids{7}));
   // 未转义时应把 % 当通配：这里模式 "100" 命中同一行，验证转义不误伤普通查询
   EXPECT_EQ(mDb->FindByName("100", ""), (Ids{7}));
}

TEST_F(EquipmentDBTest, FindByNameMaxRowsTruncation)
{
   mDb->SetMaxRows(1);
   EXPECT_EQ(mDb->FindByName("F-16", ""), (Ids{1})); // 截断且 ID 升序取首
   mDb->SetMaxRows(100);
   EXPECT_EQ(mDb->FindByName("F-16", ""), (Ids{1, 2}));
}

// ---- GetLinkedIDs（F 阶段新增） ----

TEST_F(EquipmentDBTest, LinkedIDsReturnsSortedComponents)
{
   EXPECT_EQ(mDb->GetLinkedIDs("DataAircraftSensors", 1), (Ids{10, 11, 12}));
}

TEST_F(EquipmentDBTest, LinkedIDsTruncatesAtMaxRows)
{
   mDb->SetMaxRows(4);
   EXPECT_EQ(mDb->GetLinkedIDs("DataAircraftSensors", 2), (Ids{30, 31, 32, 33})); // 6 组件截 4
   mDb->SetMaxRows(100);
   EXPECT_EQ(mDb->GetLinkedIDs("DataAircraftSensors", 2).size(), 6u);
}

TEST_F(EquipmentDBTest, LinkedIDsMissingParentYieldsEmpty)
{
   EXPECT_TRUE(mDb->GetLinkedIDs("DataAircraftSensors", 999).empty());
}

TEST_F(EquipmentDBTest, LinkedIDsInvalidTableYieldsEmpty)
{
   EXPECT_TRUE(mDb->GetLinkedIDs("Drop Table!", 1).empty()); // 白名单拦截
}

TEST_F(EquipmentDBTest, LinkedIDsOtherParent)
{
   EXPECT_EQ(mDb->GetLinkedIDs("DataAircraftSensors", 3), (Ids{20}));
}

// ---- FindFiltered（F 阶段新增） ----

TEST_F(EquipmentDBTest, FilteredByCountry)
{
   EXPECT_EQ(mDb->FindFiltered("DataAircraft", "", "United States", "", "", 0, 0), (Ids{1, 2}));
   EXPECT_EQ(mDb->FindFiltered("DataAircraft", "", "Russia", "", "", 0, 0), (Ids{3, 4, 6})); // NULL 国别行 5 被排除
   EXPECT_EQ(mDb->FindFiltered("", "", "United States", "", "", 0, 0), (Ids{1, 2, 50}));     // 全表（含传感器 50）
}

TEST_F(EquipmentDBTest, FilteredByCountryChineseColumn)
{
   // 中文国别列（CountryName_CN）命中：美国 字节串
   EXPECT_EQ(mDb->FindFiltered("", "", cUS_CN, "", "", 0, 0), (Ids{1, 2}));
}

TEST_F(EquipmentDBTest, FilteredByCategoryAndYearRange)
{
   // Fixed Wing 且 1980-1985：2(1984)、6(1982)；1(1978)、3(1976)、7(1990) 出区间
   EXPECT_EQ(mDb->FindFiltered("DataAircraft", "", "", "Fixed Wing", "", 1980, 1985), (Ids{2, 6}));
   // 年代边界含端点
   EXPECT_EQ(mDb->FindFiltered("DataAircraft", "", "", "Fixed Wing", "", 1976, 1978), (Ids{1, 3}));
}

TEST_F(EquipmentDBTest, FilteredByType)
{
   // Fighter 仅在 TypeName；NULL 型别行（4/5/7）被排除
   EXPECT_EQ(mDb->FindFiltered("DataAircraft", "", "", "", "Fighter", 0, 0), (Ids{1, 2, 6}));
}

TEST_F(EquipmentDBTest, FilteredByYearOnly)
{
   // 1980-1990 全表：2/5/6/7（飞机）+ 50（传感器）；NULL 年代行 4 被排除；1(1978)、3(1976) 出区间
   EXPECT_EQ(mDb->FindFiltered("", "", "", "", "", 1980, 1990), (Ids{2, 5, 6, 7, 50}));
   // 年代 0 = 不设限
   EXPECT_EQ(mDb->FindFiltered("DataAircraft", "", "", "", "", 0, 0).size(), 7u);
}

TEST_F(EquipmentDBTest, FilteredByTable)
{
   EXPECT_EQ(mDb->FindFiltered("DataSensor", "", "", "", "", 0, 0), (Ids{50}));
   EXPECT_EQ(mDb->FindFiltered("DataAircraft", "", "", "", "", 0, 0).size(), 7u);
}

TEST_F(EquipmentDBTest, FilteredCombinedDimensions)
{
   // 名称 + 国别 + 年代三路组合：F-16、美国、1978-1990 → 1、2
   EXPECT_EQ(mDb->FindFiltered("DataAircraft", "F-16", "United States", "", "", 1978, 1990), (Ids{1, 2}));
}

TEST_F(EquipmentDBTest, FilteredNoFiltersReturnsAll)
{
   // 全部通配 + 空表名 = 整库 8 行（上限 100 内）
   EXPECT_EQ(mDb->FindFiltered("", "", "", "", "", 0, 0).size(), 8u);
}

TEST_F(EquipmentDBTest, FilteredMaxRowsTruncation)
{
   mDb->SetMaxRows(3);
   EXPECT_EQ(mDb->FindFiltered("DataAircraft", "", "", "", "", 0, 0), (Ids{1, 2, 3})); // ID 升序取前 3
   mDb->SetMaxRows(100);
}

TEST_F(EquipmentDBTest, FilteredInvalidTableYieldsEmpty)
{
   EXPECT_TRUE(mDb->FindFiltered("Bad Table!", "", "", "", "", 0, 0).empty());
}

TEST_F(EquipmentDBTest, FilteredEscapesPercentWildcards)
{
   // 名称含 % 字面量：转义后仅命中行 7（"100% Bob"），且不被当作通配
   EXPECT_EQ(mDb->FindFiltered("DataAircraft", "100%", "", "", "", 0, 0), (Ids{7}));
}

// ---- 整行读取/列名/枚举 ----

TEST_F(EquipmentDBTest, SummaryRowByTableNameAndId)
{
   auto rec = mDb->GetSummary("DataAircraft", 1);
   EXPECT_EQ(rec.size(), 11u); // 夹具汇总表 11 列
   EXPECT_EQ(rec["Name"], "F-16A Falcon");
   EXPECT_EQ(rec["Name_CN"], std::string("F-16 ") + cZHANJI);
   EXPECT_EQ(rec["YearCommissioned"], "1978"); // INTEGER → 十进制串
}

TEST_F(EquipmentDBTest, SummaryMissingIdYieldsEmpty)
{
   EXPECT_TRUE(mDb->GetSummary("DataAircraft", 999).empty());
   EXPECT_TRUE(mDb->GetSummary("NoSuchTable", 1).empty());
}

TEST_F(EquipmentDBTest, RecordRowAndColumnNames)
{
   auto rec = mDb->GetRecord("DataAircraft", 1); // 明细表（非汇总表）
   EXPECT_EQ(rec["Name"], "F-16A Falcon");
   EXPECT_EQ(rec["ID"], "1");
   auto cols = mDb->GetColumnNames("DataAircraft");
   EXPECT_EQ(cols, (std::vector<std::string>{"ID", "Name"}));
   EXPECT_TRUE(mDb->GetColumnNames("Nope!").empty());
   // 夹具中不存在的明细表：prepare 失败 → 空 Map（不崩溃）
   EXPECT_TRUE(mDb->GetRecord("DataWeapon", 1).empty());
}

TEST_F(EquipmentDBTest, EnumDescriptionLookup)
{
   EXPECT_EQ(mDb->GetEnumDescription("EnumAircraftType", 2002), "Multirole (Fighter/Attack)");
   EXPECT_EQ(mDb->GetEnumDescription("EnumAircraftType", 9999), ""); // 查无 → 空串
   EXPECT_EQ(mDb->GetEnumDescription("Enum!", 1), "");               // 白名单拦截
}
