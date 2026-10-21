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
// 二次开发（ext-05）：现代装备数据库领域类 EquipmentDB
//
// 设计要点（.trae/specs/ext-05-equipmentdb/spec.md §3.2/§3.6）：
//   * 继承 WsfObject —— 纳入 WSF 类型体系并暴露给脚本系统
//   * 只读打开（SQLITE_OPEN_READONLY），从机制上杜绝写库/损坏
//   * 表/列名一律标识符白名单校验后加引号，数据值一律参数化绑定（防注入）
//   * 查询失败路径返回约定错误值并记告警日志，不抛异常不崩溃
//   * SQLite 本身即 C ABI，句柄跨 DLL 边界契约稳定
//   * 单线程使用（仿真线程内调用），不跨线程共享句柄

#ifndef YSIM_EQUIPMENTDB_EQUIPMENTDB_HPP
#define YSIM_EQUIPMENTDB_EQUIPMENTDB_HPP

#include <map>
#include <string>
#include <vector>

#include "WsfObject.hpp"

// SQLite C 接口前置声明（实现见 EquipmentDB.cpp，避免头文件传播 sqlite3.h）
typedef struct sqlite3 sqlite3;

namespace ysim
{
namespace equipmentdb
{
   //! 一行装备记录：列名 → 值（值统一转为字符串，NULL → ""）
   using Record = std::map<std::string, std::string>;

   class EquipmentDB : public WsfObject
   {
   public:
      EquipmentDB();
      ~EquipmentDB() override;

      // ---- WsfObject 契约 ----
      const char* GetScriptClassName() const override { return "EquipmentDB"; }

      //! 数据库句柄不可克隆（语义同 WsfUncloneableObject）
      WsfObject* Clone() const override { return nullptr; }

      bool ProcessInput(UtInput& aInput) override;

      // ---- 打开/关闭 ----
      bool        Open(const std::string& aPath);
      void        Close();
      bool        IsOpen() const { return (mDB != nullptr); }
      const std::string& GetPath() const { return mPath; }

      //! 相对路径解析基准目录（想定输入文件目录）；空 = 仅按原样/CWD 解析
      void SetBaseDir(const std::string& aBaseDir) { mBaseDir = aBaseDir; }

      // ---- 查询（返回/错误语义见 spec §3.3） ----
      int                      GetEquipmentCount(const std::string& aTableName) const;
      std::vector<int>         FindByName(const std::string& aPattern, const std::string& aTableName) const;

      //! 关联表导航（F 阶段）：链接表统一约定 ID → ComponentID（46 表实测，spec §2.5），
      //! 故只需表名 + 父 ID；按 ComponentID 升序，上限 max_rows；非法表名/未打开 → 空
      std::vector<int>         GetLinkedIDs(const std::string& aLinkTable, int aParentID) const;

      //! 组合过滤（F 阶段）：DataEquipmentSummary 上五路可选过滤；
      //! 字符串参数空 = 通配，年代 0 = 不设限；NULL 字段自然排除（LIKE/比较对 NULL 恒假）
      std::vector<int>         FindFiltered(const std::string& aTableName,
                                            const std::string& aNamePattern,
                                            const std::string& aCountryPattern,
                                            const std::string& aCategoryPattern,
                                            const std::string& aTypePattern,
                                            int                aYearFrom,
                                            int                aYearTo) const;
      Record                   GetSummary(const std::string& aTableName, int aID) const;
      Record                   GetRecord(const std::string& aTableName, int aID) const;
      std::vector<std::string> GetColumnNames(const std::string& aTableName) const;
      std::string              GetEnumDescription(const std::string& aEnumTable, int aCode) const;
      std::string              GetInfo() const;

      void SetMaxRows(int aMaxRows) { mMaxRows = (aMaxRows >= 1) ? aMaxRows : 1; }
      int  GetMaxRows() const { return mMaxRows; }

      //! 标识符白名单校验：仅 [A-Za-z_][A-Za-z0-9_]* 且长度 <= 128
      static bool IsValidIdentifier(const std::string& aName);

   private:
      EquipmentDB(const EquipmentDB&) = delete;
      EquipmentDB& operator=(const EquipmentDB&) = delete;

      sqlite3*    mDB;      //!< SQLite 只读句柄（nullptr = 未打开）
      std::string mPath;    //!< 实际打开的路径（解析后）
      std::string mBaseDir; //!< 相对路径解析基准
      int         mMaxRows; //!< FindByName 结果上限（防大结果集）
   };
} // namespace equipmentdb
} // namespace ysim

#endif
