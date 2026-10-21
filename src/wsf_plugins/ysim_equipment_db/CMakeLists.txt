# ****************************************************************************
# CUI
#
# The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
#
# Copyright 2003-2015 The Boeing Company. All rights reserved.
#
# The use, dissemination or disclosure of data in this file is subject to
# limitation or restriction. See accompanying README and LICENSE for details.
# ****************************************************************************
# 二次开发（ext-05）：ysim_equipment_db 现代装备数据库插件
#
# 设计目标（.trae/specs/ext-05-equipmentdb/spec.md）：
#   封装 EquipmentDB 类（继承 WsfObject）注册到 WSF 脚本系统，
#   以只读模式查询 build/db/DB3K_modern.db3 现代装备数据库，
#   零侵入 WSF 核心源码，DLL 部署至 mission_plugins。
project(ysim_equipment_db)

include(swdev_project)

file(GLOB SRCS source/*.?pp)

add_library(${PROJECT_NAME} ${SRCS})

target_link_libraries(${PROJECT_NAME} ${WSF_LIBS})

# 二次开发（ext-05）：SQLite 三态依赖（照抄 ext-01 已验证模式，变量名独立避免串扰）
#   AUTO（默认）：构建树 3rd_party 解压包 → 系统级 find_package(SQLite3) 回退
#                → 均未命中自动降级为无 SQLite 编译（查询方法返回错误态，不崩溃）
#   ON ：必须命中（3rd_party 或系统），否则 FATAL_ERROR（显式强依赖语义）
#   OFF：无条件跳过（宏 YSIM_EQDB_NO_SQLITE 条件编译）
set(YSIM_EQDB_SQLITE_SUPPORT "AUTO" CACHE STRING "ysim_equipment_db SQLite support: AUTO/ON/OFF")
set_property(CACHE YSIM_EQDB_SQLITE_SUPPORT PROPERTY STRINGS AUTO ON OFF)

set(YSIM_EQDB_SQLITE_FOUND FALSE)
set(YSIM_EQDB_SQLITE_TYPE "")
if(YSIM_EQDB_SQLITE_SUPPORT STREQUAL "OFF")
   # OFF：无条件跳过（显式禁用语义，即便 3rd_party 包在场也不链接）
   target_compile_definitions(${PROJECT_NAME} PRIVATE YSIM_EQDB_NO_SQLITE)
   message(STATUS "ysim_equipment_db: YSIM_EQDB_SQLITE_SUPPORT=OFF - EquipmentDB query support disabled by option.")
else() # AUTO/ON：查找链 3rd_party → 系统
   file(GLOB YSIM_EQDB_SQLITE_ROOTS "${CMAKE_BINARY_DIR}/3rd_party/sqlite-3.*")
   if(YSIM_EQDB_SQLITE_ROOTS)
      # 多版本并存时取首个（与 wizard 子树解压约定一致的确定性顺序）
      list(GET YSIM_EQDB_SQLITE_ROOTS 0 YSIM_EQDB_SQLITE_ROOT)
      target_include_directories(${PROJECT_NAME} PRIVATE "${YSIM_EQDB_SQLITE_ROOT}/include")
      target_link_libraries(${PROJECT_NAME} debug "${YSIM_EQDB_SQLITE_ROOT}/debug/sqlite3.lib"
                                       optimized "${YSIM_EQDB_SQLITE_ROOT}/release/sqlite3.lib")
      set(YSIM_EQDB_SQLITE_FOUND TRUE)
      set(YSIM_EQDB_SQLITE_TYPE "3rd_party")
   else()
      # 系统级回退：CMake 内置 FindSQLite3 模块（Linux/macOS 包管理器安装可命中）
      find_package(SQLite3 QUIET)
      if(SQLite3_FOUND)
         target_include_directories(${PROJECT_NAME} PRIVATE ${SQLite3_INCLUDE_DIRS})
         target_link_libraries(${PROJECT_NAME} ${SQLite3_LIBRARIES})
         set(YSIM_EQDB_SQLITE_FOUND TRUE)
         set(YSIM_EQDB_SQLITE_TYPE "system")
      endif()
   endif()

   if(YSIM_EQDB_SQLITE_FOUND)
      message(STATUS "ysim_equipment_db: sqlite (${YSIM_EQDB_SQLITE_TYPE}) linked - EquipmentDB query support enabled.")
   elseif(YSIM_EQDB_SQLITE_SUPPORT STREQUAL "ON")
      message(FATAL_ERROR
         "ysim_equipment_db: YSIM_EQDB_SQLITE_SUPPORT=ON but no sqlite found (neither "
         "'${CMAKE_BINARY_DIR}/3rd_party/sqlite-3.*' nor system SQLite3). "
         "Build the wizard subtree once to unpack the 3rd_party package, or set YSIM_EQDB_SQLITE_SUPPORT=AUTO/OFF.")
   else()
      # AUTO 未命中降级：查询支持编译期剔除，不阻断配置
      target_compile_definitions(${PROJECT_NAME} PRIVATE YSIM_EQDB_NO_SQLITE)
      message(WARNING
         "ysim_equipment_db: sqlite not found - EquipmentDB query support disabled (Open always fails, "
         "queries return error values). Set YSIM_EQDB_SQLITE_SUPPORT=ON to enforce, "
         "or build the wizard subtree to provide the 3rd_party package.")
   endif()
endif()

# Set the subfolder that the target will appear under in an IDE's solution explorer.
set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "wsf_plugins")

# 部署到 mission_plugins 目录：mission.exe 启动时自动加载（无需 require_plugin 命令）
swdev_plugin_install(${PROJECT_NAME} mission_plugins)
swdev_warning_level(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/source")

# 二次开发（ext-05 G 阶段）：单元测试子目录（GTest，仿 six_dof 模式）。
# 须在 SQLite 检测块之后，使 YSIM_EQDB_* 变量对子目录可见。
add_subdirectory(test)
