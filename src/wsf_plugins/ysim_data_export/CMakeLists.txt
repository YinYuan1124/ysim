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
# 二次开发（ext-01）：ysim_data_export 仿真数据出口插件
#
# 设计目标（.trae/specs/ext-01-data-export/spec.md §7.3）：
#   通过 WsfObserver 只读订阅仿真事件，将平台生命周期/位置姿态以 JSON 行协议
#   经 TCP 实时推送外部系统，零侵入 WSF 核心源码，DLL 部署至 mission_plugins。
project(ysim_data_export)

include(swdev_project)

file(GLOB SRCS source/*.?pp)

add_library(${PROJECT_NAME} ${SRCS})
# ws2_32：TCP/UDP 数据通道所需 Winsock 运行库
target_link_libraries(${PROJECT_NAME} ${WSF_LIBS} ws2_32)

# 二次开发（ext-01 阶段 C）：静态链接 SQLite（结构化落盘 ysim_export.db）
# 说明：swdev_package_* 宏依赖的 sqlite_* 变量仅在 wizard 子树作用域可见（宏静默跳过），
# 故按 3rd_party 解压约定直接定位构建树内的静态库（无 DLL 运行时部署问题）。
#
# 二次开发（修复 2026-10-19）：SQLite 依赖可选化——此前构建树未解压 sqlite 包时
# 直接 FATAL_ERROR 阻断全局配置（干净构建树/仅构建核心子树的场景即中招）。
# 改为三态缓存选项 YSIM_SQLITE_SUPPORT：
#   AUTO（默认）：优先构建树 3rd_party 解压包 → 系统级 find_package(SQLite3) 回退
#                → 均未命中自动降级为无 SQLite 编译（CSV 落盘/推流通道不受影响）
#   ON ：必须命中（3rd_party 或系统），否则 FATAL_ERROR（显式强依赖语义）
#   OFF：无条件跳过（宏 YSIM_NO_SQLITE 条件编译）
set(YSIM_SQLITE_SUPPORT "AUTO" CACHE STRING "ysim_data_export SQLite support: AUTO/ON/OFF")
set_property(CACHE YSIM_SQLITE_SUPPORT PROPERTY STRINGS AUTO ON OFF)

set(YSIM_SQLITE_FOUND FALSE)
set(YSIM_SQLITE_TYPE "")
if(YSIM_SQLITE_SUPPORT STREQUAL "OFF")
   # OFF：无条件跳过（显式禁用语义，即便 3rd_party 包在场也不链接）
   target_compile_definitions(${PROJECT_NAME} PRIVATE YSIM_NO_SQLITE)
   message(STATUS "ysim_data_export: YSIM_SQLITE_SUPPORT=OFF - SQLite sink disabled by option.")
else() # AUTO/ON：查找链 3rd_party → 系统
   file(GLOB YSIM_SQLITE_ROOTS "${CMAKE_BINARY_DIR}/3rd_party/sqlite-3.*")
   if(YSIM_SQLITE_ROOTS)
      # 多版本并存时取首个（与 wizard 子树解压约定一致的确定性顺序）
      list(GET YSIM_SQLITE_ROOTS 0 YSIM_SQLITE_ROOT)
      target_include_directories(${PROJECT_NAME} PRIVATE "${YSIM_SQLITE_ROOT}/include")
      target_link_libraries(${PROJECT_NAME} debug "${YSIM_SQLITE_ROOT}/debug/sqlite3.lib"
                                       optimized "${YSIM_SQLITE_ROOT}/release/sqlite3.lib")
      set(YSIM_SQLITE_FOUND TRUE)
      set(YSIM_SQLITE_TYPE "3rd_party")
   else()
      # 系统级回退：CMake 内置 FindSQLite3 模块（Linux/macOS 包管理器安装可命中）
      find_package(SQLite3 QUIET)
      if(SQLite3_FOUND)
         target_include_directories(${PROJECT_NAME} PRIVATE ${SQLite3_INCLUDE_DIRS})
         target_link_libraries(${PROJECT_NAME} ${SQLite3_LIBRARIES})
         set(YSIM_SQLITE_FOUND TRUE)
         set(YSIM_SQLITE_TYPE "system")
      endif()
   endif()

   if(YSIM_SQLITE_FOUND)
      message(STATUS "ysim_data_export: sqlite (${YSIM_SQLITE_TYPE}) linked - recorder SQLite sink enabled.")
   elseif(YSIM_SQLITE_SUPPORT STREQUAL "ON")
      message(FATAL_ERROR
         "ysim_data_export: YSIM_SQLITE_SUPPORT=ON but no sqlite found (neither "
         "'${CMAKE_BINARY_DIR}/3rd_party/sqlite-3.*' nor system SQLite3). "
         "Build the wizard subtree once to unpack the 3rd_party package, or set YSIM_SQLITE_SUPPORT=AUTO/OFF.")
   else()
      # AUTO 未命中降级：SQLite Sink 编译期剔除，不阻断配置
      target_compile_definitions(${PROJECT_NAME} PRIVATE YSIM_NO_SQLITE)
      message(WARNING
         "ysim_data_export: sqlite not found - SQLite sink disabled (CSV sink and live streams unaffected). "
         "Set YSIM_SQLITE_SUPPORT=ON to enforce, or build the wizard subtree to provide the 3rd_party package.")
   endif()
endif()

# Set the subfolder that the target will appear under in an IDE's solution explorer.
set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "wsf_plugins")

# 部署到 mission_plugins 目录：mission.exe 启动时自动加载（无需 require_plugin 命令）
swdev_plugin_install(${PROJECT_NAME} mission_plugins)
swdev_warning_level(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/source")
