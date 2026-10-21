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
// 二次开发（ext-05）：EquipmentDB 实现
//
// 无 SQLite 降级策略（YSIM_EQDB_NO_SQLITE）：Open() 恒 false → mDB 恒空，
// 全部查询经 mDB 空值守卫自然返回约定错误值 —— 单点 ifdef，避免方法级条件编译。

#include "EquipmentDB.hpp"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "UtInput.hpp"
#include "UtLog.hpp"

#ifndef YSIM_EQDB_NO_SQLITE
#include "sqlite3.h"
#endif

namespace
{
   // ---- 文件/路径小工具（无依赖，避免引入 UtPath） ----

   bool FileExists(const std::string& aPath)
   {
      std::ifstream f(aPath.c_str(), std::ios::binary);
      return f.good();
   }

   //! 文件头 16 字节 == "SQLite format 3\0"（打开前校验，给明确错误信息）
   bool LooksLikeSqlite(const std::string& aPath)
   {
      std::ifstream f(aPath.c_str(), std::ios::binary);
      char          header[16] = {0};
      f.read(header, sizeof(header));
      return f.gcount() == static_cast<std::streamsize>(sizeof(header)) &&
             std::memcmp(header, "SQLite format 3\0", sizeof(header)) == 0;
   }

   //! 取父目录（兼容 / 与 \）；无分隔符返回空串
   std::string DirName(const std::string& aPath)
   {
      std::string::size_type pos = aPath.find_last_of("/\\");
      return (pos == std::string::npos) ? std::string() : aPath.substr(0, pos);
   }

   //! 拼接目录与相对路径；name 自身是绝对路径时原样返回
   std::string JoinPath(const std::string& aDir, const std::string& aName)
   {
      if (aDir.empty() || (!aName.empty() && (aName[0] == '/' || aName[0] == '\\')) ||
          (aName.size() >= 2 && aName[1] == ':'))
      {
         return aName;
      }
      return aDir + "/" + aName;
   }

#ifndef YSIM_EQDB_NO_SQLITE
   //! LIKE 通配符转义：% _ \ 三个特殊字符前置反斜杠（防通配符注入；配合 ESCAPE '\'）
   std::string EscapeLike(const std::string& aPattern)
   {
      std::string escaped;
      escaped.reserve(aPattern.size() * 2);
      for (char c : aPattern)
      {
         if ((c == '%') || (c == '_') || (c == '\\'))
         {
            escaped += '\\';
         }
         escaped += c;
      }
      return escaped;
   }

   //! 语句准备：失败记告警并返回 nullptr（统一入口）
   sqlite3_stmt* Prepare(sqlite3* aDB, const char* aSQL)
   {
      sqlite3_stmt* stmt = nullptr;
      if (sqlite3_prepare_v2(aDB, aSQL, -1, &stmt, nullptr) != SQLITE_OK)
      {
         auto out = ut::log::warning() << "EquipmentDB: SQL prepare failed: " << sqlite3_errmsg(aDB) << ".";
         return nullptr;
      }
      return stmt;
   }

   //! 当前行 → 记录（列名取 sqlite3_column_name，值统一转字符串）
   ysim::equipmentdb::Record ReadRecord(sqlite3_stmt* aStmt)
   {
      ysim::equipmentdb::Record rec;
      const int                  colCount = sqlite3_column_count(aStmt);
      for (int i = 0; i < colCount; ++i)
      {
         const char* name = sqlite3_column_name(aStmt, i);
         rec[(name != nullptr) ? name : "?"] = [aStmt, i]() -> std::string
         {
            switch (sqlite3_column_type(aStmt, i))
            {
            case SQLITE_INTEGER:
               return std::to_string(sqlite3_column_int64(aStmt, i));
            case SQLITE_FLOAT:
            {
               char buf[32];
               std::snprintf(buf, sizeof(buf), "%g", sqlite3_column_double(aStmt, i));
               return std::string(buf);
            }
            case SQLITE_TEXT:
            {
               const unsigned char* txt = sqlite3_column_text(aStmt, i);
               return (txt != nullptr) ? std::string(reinterpret_cast<const char*>(txt)) : std::string();
            }
            case SQLITE_BLOB:
               return "<BLOB " + std::to_string(sqlite3_column_bytes(aStmt, i)) + " bytes>";
            case SQLITE_NULL:
            default:
               return std::string();
            }
         }();
      }
      return rec;
   }
#endif
} // namespace

namespace ysim
{
namespace equipmentdb
{
   EquipmentDB::EquipmentDB()
      : WsfObject()
      , mDB(nullptr)
      , mMaxRows(100)
   {
   }

   EquipmentDB::~EquipmentDB()
   {
      Close();
   }

   bool EquipmentDB::IsValidIdentifier(const std::string& aName)
   {
      if (aName.empty() || aName.size() > 128)
      {
         return false;
      }
      if (!std::isalpha(static_cast<unsigned char>(aName[0])) && aName[0] != '_')
      {
         return false;
      }
      for (char c : aName)
      {
         if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_')
         {
            return false;
         }
      }
      return true;
   }

   void EquipmentDB::Close()
   {
#ifndef YSIM_EQDB_NO_SQLITE
      if (mDB != nullptr)
      {
         sqlite3_close_v2(mDB);
         mDB = nullptr;
      }
#endif
   }

   bool EquipmentDB::Open(const std::string& aPath)
   {
      Close();

      // 路径解析：原样（含绝对路径/CWD 相对）→ 基准目录相对（spec §3.4）
      std::string resolved = aPath;
      if (!FileExists(resolved) && !mBaseDir.empty())
      {
         std::string candidate = JoinPath(mBaseDir, aPath);
         if (FileExists(candidate))
         {
            resolved = candidate;
         }
      }
      if (!FileExists(resolved))
      {
         auto out = ut::log::warning() << "EquipmentDB: database file not found: " << resolved << ".";
         return false;
      }
      if (!LooksLikeSqlite(resolved))
      {
         auto out = ut::log::warning() << "EquipmentDB: not a SQLite format 3 file: " << resolved << ".";
         return false;
      }

#ifndef YSIM_EQDB_NO_SQLITE
      sqlite3* db = nullptr;
      if (sqlite3_open_v2(resolved.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK)
      {
         auto out = ut::log::warning()
                   << "EquipmentDB: sqlite open failed: " << ((db != nullptr) ? sqlite3_errmsg(db) : "unknown error")
                   << ".";
         if (db != nullptr)
         {
            sqlite3_close_v2(db);
         }
         return false;
      }
      mDB   = db;
      mPath = resolved;
      auto out = ut::log::info() << "EquipmentDB: opened (read-only) " << resolved << ".";
      return true;
#else
      auto out = ut::log::warning()
                << "EquipmentDB: compiled without SQLite support (YSIM_EQDB_NO_SQLITE); cannot open " << resolved
                << ".";
      return false;
#endif
   }

   bool EquipmentDB::ProcessInput(UtInput& aInput)
   {
      std::string command = aInput.GetCommand();
      if (command == "database")
      {
         std::string path;
         aInput.ReadValueQuoted(path);
         if (!Open(path))
         {
            throw UtInput::BadValue(aInput, "EquipmentDB: database open failed: " + path);
         }
      }
      else if (command == "max_rows")
      {
         int maxRows = 0;
         aInput.ReadValue(maxRows);
         if (maxRows < 1)
         {
            throw UtInput::BadValue(aInput, "EquipmentDB: max_rows must be >= 1");
         }
         SetMaxRows(maxRows);
      }
      else
      {
         return WsfObject::ProcessInput(aInput);
      }
      return true;
   }

   int EquipmentDB::GetEquipmentCount(const std::string& aTableName) const
   {
#ifndef YSIM_EQDB_NO_SQLITE
      if (mDB == nullptr)
      {
         auto out = ut::log::warning() << "EquipmentDB: GetEquipmentCount called on unopened database.";
         return -1;
      }
      sqlite3_stmt* stmt = nullptr;
      if (aTableName.empty() || aTableName == "*")
      {
         stmt = Prepare(mDB, "SELECT COUNT(*) FROM DataEquipmentSummary");
      }
      else
      {
         if (!IsValidIdentifier(aTableName))
         {
            auto out = ut::log::warning() << "EquipmentDB: invalid table identifier '" << aTableName << "'.";
            return -1;
         }
         stmt = Prepare(mDB, "SELECT COUNT(*) FROM DataEquipmentSummary WHERE TableName = ?1");
         if (stmt != nullptr && sqlite3_bind_text(stmt, 1, aTableName.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK)
         {
            sqlite3_finalize(stmt);
            stmt = nullptr;
         }
      }
      if (stmt == nullptr)
      {
         return -1;
      }
      int result = -1;
      int rc     = sqlite3_step(stmt);
      if (rc == SQLITE_ROW)
      {
         result = sqlite3_column_int(stmt, 0);
      }
      else
      {
         auto out = ut::log::warning() << "EquipmentDB: count query failed: " << sqlite3_errmsg(mDB) << ".";
      }
      sqlite3_finalize(stmt);
      return result;
#else
      static_cast<void>(aTableName);
      auto out = ut::log::warning() << "EquipmentDB: compiled without SQLite support; count unavailable.";
      return -1;
#endif
   }

   std::vector<int> EquipmentDB::FindByName(const std::string& aPattern, const std::string& aTableName) const
   {
      std::vector<int> ids;
#ifndef YSIM_EQDB_NO_SQLITE
      if (mDB == nullptr)
      {
         auto out = ut::log::warning() << "EquipmentDB: FindByName called on unopened database.";
         return ids;
      }
      const bool allTables = aTableName.empty() || aTableName == "*";
      if (!allTables && !IsValidIdentifier(aTableName))
      {
         auto out = ut::log::warning() << "EquipmentDB: invalid table identifier '" << aTableName << "'.";
         return ids;
      }

      const std::string like = "%" + EscapeLike(aPattern) + "%";

      // 按是否存在表过滤拼接两个变体（参数占位序号随变体重排，绑定索引须一致）
      std::string sql = "SELECT ID FROM DataEquipmentSummary "
                        "WHERE (Name LIKE ?1 ESCAPE '\\' OR Name_CN LIKE ?1 ESCAPE '\\')";
      int limitIndex = 2;
      if (!allTables)
      {
         sql += " AND TableName = ?2";
         limitIndex = 3;
      }
      sql += " ORDER BY ID LIMIT ?";
      sql += std::to_string(limitIndex);

      sqlite3_stmt* stmt = Prepare(mDB, sql.c_str());
      if (stmt == nullptr)
      {
         return ids;
      }
      bool bindOK = (sqlite3_bind_text(stmt, 1, like.c_str(), -1, SQLITE_TRANSIENT) == SQLITE_OK);
      if (bindOK && !allTables)
      {
         bindOK = (sqlite3_bind_text(stmt, 2, aTableName.c_str(), -1, SQLITE_TRANSIENT) == SQLITE_OK);
      }
      if (bindOK)
      {
         bindOK = (sqlite3_bind_int(stmt, limitIndex, mMaxRows) == SQLITE_OK);
      }
      if (bindOK)
      {
         int rc = sqlite3_step(stmt);
         while (rc == SQLITE_ROW)
         {
            ids.push_back(sqlite3_column_int(stmt, 0));
            rc = sqlite3_step(stmt);
         }
         if (rc != SQLITE_DONE)
         {
            auto out = ut::log::warning() << "EquipmentDB: FindByName query failed: " << sqlite3_errmsg(mDB) << ".";
         }
      }
      else
      {
         auto out = ut::log::warning() << "EquipmentDB: FindByName bind failed.";
      }
      sqlite3_finalize(stmt);
#else
      static_cast<void>(aPattern);
      static_cast<void>(aTableName);
      auto out = ut::log::warning() << "EquipmentDB: compiled without SQLite support; search unavailable.";
#endif
      return ids;
   }

   std::vector<int> EquipmentDB::GetLinkedIDs(const std::string& aLinkTable, int aParentID) const
   {
      std::vector<int> ids;
#ifndef YSIM_EQDB_NO_SQLITE
      if (mDB == nullptr)
      {
         auto out = ut::log::warning() << "EquipmentDB: GetLinkedIDs called on unopened database.";
         return ids;
      }
      if (!IsValidIdentifier(aLinkTable))
      {
         auto out = ut::log::warning() << "EquipmentDB: invalid link table identifier '" << aLinkTable << "'.";
         return ids;
      }
      // 链接表统一约定 ID → ComponentID（46 表实测一致，spec §2.5），故固定两列名；
      // 表名已过白名单校验，拼接安全；父 ID 与上限参数化绑定
      const std::string sql =
         "SELECT ComponentID FROM \"" + aLinkTable + "\" WHERE ID = ?1 ORDER BY ComponentID LIMIT ?2";
      sqlite3_stmt* stmt = Prepare(mDB, sql.c_str());
      if (stmt == nullptr)
      {
         return ids;
      }
      if ((sqlite3_bind_int(stmt, 1, aParentID) != SQLITE_OK) ||
          (sqlite3_bind_int(stmt, 2, mMaxRows) != SQLITE_OK))
      {
         auto out = ut::log::warning() << "EquipmentDB: GetLinkedIDs bind failed.";
         sqlite3_finalize(stmt);
         return ids;
      }
      int rc = sqlite3_step(stmt);
      while (rc == SQLITE_ROW)
      {
         ids.push_back(sqlite3_column_int(stmt, 0));
         rc = sqlite3_step(stmt);
      }
      if (rc != SQLITE_DONE)
      {
         auto out = ut::log::warning() << "EquipmentDB: GetLinkedIDs query failed: " << sqlite3_errmsg(mDB) << ".";
      }
      sqlite3_finalize(stmt);
#else
      static_cast<void>(aLinkTable);
      static_cast<void>(aParentID);
      auto out = ut::log::warning() << "EquipmentDB: compiled without SQLite support; link navigation unavailable.";
#endif
      return ids;
   }

   std::vector<int> EquipmentDB::FindFiltered(const std::string& aTableName,
                                              const std::string& aNamePattern,
                                              const std::string& aCountryPattern,
                                              const std::string& aCategoryPattern,
                                              const std::string& aTypePattern,
                                              int                aYearFrom,
                                              int                aYearTo) const
   {
      std::vector<int> ids;
#ifndef YSIM_EQDB_NO_SQLITE
      if (mDB == nullptr)
      {
         auto out = ut::log::warning() << "EquipmentDB: FindFiltered called on unopened database.";
         return ids;
      }
      const bool allTables = aTableName.empty() || aTableName == "*";
      if (!allTables && !IsValidIdentifier(aTableName))
      {
         auto out = ut::log::warning() << "EquipmentDB: invalid table identifier '" << aTableName << "'.";
         return ids;
      }

      // 五路可选过滤动态拼接（空串通配/年代 0 不设限）；数据值全部按出现顺序参数化绑定。
      // 绑定队列：isInt=0 时 text 生效，isInt=1 时 intVal 生效 —— 与 SQL 中 ? 出现顺序一一对应
      struct BindVal
      {
         bool        isInt;
         std::string text;
         int         intVal;
      };
      std::vector<BindVal> binds;

      std::string sql = "SELECT ID FROM DataEquipmentSummary WHERE 1=1";
      if (!allTables)
      {
         sql += " AND TableName = ?";
         binds.push_back({false, aTableName, 0});
      }
      // 双语 LIKE 过滤片段：en 列与 _CN 列各一个 ?（同一值绑定两次，ESCAPE '\' 转义）
      const char* likeFragments[4][2] = {{"Name", "Name_CN"},
                                        {"CountryName", "CountryName_CN"},
                                        {"CategoryName", "CategoryName_CN"},
                                        {"TypeName", "TypeName_CN"}};
      const std::string* patterns[4] = {&aNamePattern, &aCountryPattern, &aCategoryPattern, &aTypePattern};
      for (int i = 0; i < 4; ++i)
      {
         if (patterns[i]->empty())
         {
            continue;
         }
         const std::string like = "%" + EscapeLike(*patterns[i]) + "%";
         sql += " AND (" + std::string(likeFragments[i][0]) + " LIKE ? ESCAPE '\\'" +
                " OR " + std::string(likeFragments[i][1]) + " LIKE ? ESCAPE '\\')";
         binds.push_back({false, like, 0});
         binds.push_back({false, like, 0});
      }
      if (aYearFrom > 0)
      {
         sql += " AND YearCommissioned >= ?";
         binds.push_back({true, "", aYearFrom});
      }
      if (aYearTo > 0)
      {
         sql += " AND YearCommissioned <= ?";
         binds.push_back({true, "", aYearTo});
      }
      sql += " ORDER BY ID LIMIT ?";
      binds.push_back({true, "", mMaxRows});

      sqlite3_stmt* stmt = Prepare(mDB, sql.c_str());
      if (stmt == nullptr)
      {
         return ids;
      }
      bool bindOK = true;
      for (size_t i = 0; i < binds.size() && bindOK; ++i)
      {
         const int idx = static_cast<int>(i) + 1;
         bindOK = binds[i].isInt ? (sqlite3_bind_int(stmt, idx, binds[i].intVal) == SQLITE_OK)
                                 : (sqlite3_bind_text(stmt, idx, binds[i].text.c_str(), -1, SQLITE_TRANSIENT) ==
                                    SQLITE_OK);
      }
      if (bindOK)
      {
         int rc = sqlite3_step(stmt);
         while (rc == SQLITE_ROW)
         {
            ids.push_back(sqlite3_column_int(stmt, 0));
            rc = sqlite3_step(stmt);
         }
         if (rc != SQLITE_DONE)
         {
            auto out = ut::log::warning() << "EquipmentDB: FindFiltered query failed: " << sqlite3_errmsg(mDB) << ".";
         }
      }
      else
      {
         auto out = ut::log::warning() << "EquipmentDB: FindFiltered bind failed.";
      }
      sqlite3_finalize(stmt);
#else
      static_cast<void>(aTableName);
      static_cast<void>(aNamePattern);
      static_cast<void>(aCountryPattern);
      static_cast<void>(aCategoryPattern);
      static_cast<void>(aTypePattern);
      static_cast<void>(aYearFrom);
      static_cast<void>(aYearTo);
      auto out = ut::log::warning() << "EquipmentDB: compiled without SQLite support; filtered search unavailable.";
#endif
      return ids;
   }

   Record EquipmentDB::GetSummary(const std::string& aTableName, int aID) const
   {
      Record rec;
#ifndef YSIM_EQDB_NO_SQLITE
      if (mDB == nullptr)
      {
         auto out = ut::log::warning() << "EquipmentDB: GetSummary called on unopened database.";
         return rec;
      }
      if (!IsValidIdentifier(aTableName))
      {
         auto out = ut::log::warning() << "EquipmentDB: invalid table identifier '" << aTableName << "'.";
         return rec;
      }
      sqlite3_stmt* stmt = Prepare(mDB, "SELECT * FROM DataEquipmentSummary WHERE TableName = ?1 AND ID = ?2");
      if (stmt == nullptr)
      {
         return rec;
      }
      if ((sqlite3_bind_text(stmt, 1, aTableName.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) ||
          (sqlite3_bind_int(stmt, 2, aID) != SQLITE_OK))
      {
         auto out = ut::log::warning() << "EquipmentDB: GetSummary bind failed.";
         sqlite3_finalize(stmt);
         return rec;
      }
      if (sqlite3_step(stmt) == SQLITE_ROW)
      {
         rec = ReadRecord(stmt);
      }
      sqlite3_finalize(stmt);
#else
      static_cast<void>(aTableName);
      static_cast<void>(aID);
      auto out = ut::log::warning() << "EquipmentDB: compiled without SQLite support; summary unavailable.";
#endif
      return rec;
   }

   Record EquipmentDB::GetRecord(const std::string& aTableName, int aID) const
   {
      Record rec;
#ifndef YSIM_EQDB_NO_SQLITE
      if (mDB == nullptr)
      {
         auto out = ut::log::warning() << "EquipmentDB: GetRecord called on unopened database.";
         return rec;
      }
      if (!IsValidIdentifier(aTableName))
      {
         auto out = ut::log::warning() << "EquipmentDB: invalid table identifier '" << aTableName << "'.";
         return rec;
      }
      // 表名已过白名单校验，拼接安全；ID 参数化绑定
      const std::string sql = "SELECT * FROM \"" + aTableName + "\" WHERE ID = ?1";
      sqlite3_stmt*     stmt = Prepare(mDB, sql.c_str());
      if (stmt == nullptr)
      {
         return rec;
      }
      if (sqlite3_bind_int(stmt, 1, aID) != SQLITE_OK)
      {
         auto out = ut::log::warning() << "EquipmentDB: GetRecord bind failed.";
         sqlite3_finalize(stmt);
         return rec;
      }
      const int rc = sqlite3_step(stmt);
      if (rc == SQLITE_ROW)
      {
         rec = ReadRecord(stmt);
      }
      else if (rc != SQLITE_DONE)
      {
         auto out = ut::log::warning() << "EquipmentDB: GetRecord query failed: " << sqlite3_errmsg(mDB) << ".";
      }
      sqlite3_finalize(stmt);
#else
      static_cast<void>(aTableName);
      static_cast<void>(aID);
      auto out = ut::log::warning() << "EquipmentDB: compiled without SQLite support; record unavailable.";
#endif
      return rec;
   }

   std::vector<std::string> EquipmentDB::GetColumnNames(const std::string& aTableName) const
   {
      std::vector<std::string> names;
#ifndef YSIM_EQDB_NO_SQLITE
      if (mDB == nullptr)
      {
         auto out = ut::log::warning() << "EquipmentDB: GetColumnNames called on unopened database.";
         return names;
      }
      if (!IsValidIdentifier(aTableName))
      {
         auto out = ut::log::warning() << "EquipmentDB: invalid table identifier '" << aTableName << "'.";
         return names;
      }
      const std::string sql = "PRAGMA table_info(\"" + aTableName + "\")";
      sqlite3_stmt*     stmt = Prepare(mDB, sql.c_str());
      if (stmt == nullptr)
      {
         return names;
      }
      int rc = sqlite3_step(stmt);
      while (rc == SQLITE_ROW)
      {
         const unsigned char* name = sqlite3_column_text(stmt, 1);
         if (name != nullptr)
         {
            names.emplace_back(reinterpret_cast<const char*>(name));
         }
         rc = sqlite3_step(stmt);
      }
      sqlite3_finalize(stmt);
#else
      static_cast<void>(aTableName);
      auto out = ut::log::warning() << "EquipmentDB: compiled without SQLite support; schema unavailable.";
#endif
      return names;
   }

   std::string EquipmentDB::GetEnumDescription(const std::string& aEnumTable, int aCode) const
   {
#ifndef YSIM_EQDB_NO_SQLITE
      if (mDB == nullptr)
      {
         auto out = ut::log::warning() << "EquipmentDB: GetEnumDescription called on unopened database.";
         return std::string();
      }
      if (!IsValidIdentifier(aEnumTable))
      {
         auto out = ut::log::warning() << "EquipmentDB: invalid enum table identifier '" << aEnumTable << "'.";
         return std::string();
      }
      const std::string sql = "SELECT Description FROM \"" + aEnumTable + "\" WHERE ID = ?1";
      sqlite3_stmt*     stmt = Prepare(mDB, sql.c_str());
      if (stmt == nullptr)
      {
         return std::string();
      }
      std::string result;
      if (sqlite3_bind_int(stmt, 1, aCode) == SQLITE_OK)
      {
         if (sqlite3_step(stmt) == SQLITE_ROW)
         {
            const unsigned char* txt = sqlite3_column_text(stmt, 0);
            if (txt != nullptr)
            {
               result = reinterpret_cast<const char*>(txt);
            }
         }
      }
      else
      {
         auto out = ut::log::warning() << "EquipmentDB: GetEnumDescription bind failed.";
      }
      sqlite3_finalize(stmt);
      return result;
#else
      static_cast<void>(aEnumTable);
      static_cast<void>(aCode);
      auto out = ut::log::warning() << "EquipmentDB: compiled without SQLite support; enum unavailable.";
      return std::string();
#endif
   }

   std::string EquipmentDB::GetInfo() const
   {
      std::ostringstream oss;
      oss << "EquipmentDB: " << (IsOpen() ? "open" : "closed") << ", path=" << mPath << ", max_rows=" << mMaxRows;
#ifndef YSIM_EQDB_NO_SQLITE
      if (mDB != nullptr)
      {
         oss << ", sqlite=" << sqlite3_libversion() << ", equipment_total=" << GetEquipmentCount("");
      }
#else
      oss << ", sqlite=unavailable";
#endif
      return oss.str();
   }
} // namespace equipmentdb
} // namespace ysim
