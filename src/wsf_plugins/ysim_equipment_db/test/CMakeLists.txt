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
# 二次开发（ext-05 G 阶段）：ysim_equipment_db 单元测试（仿 six_dof_test 模式）
#
# 链接方式说明：插件 DLL 走 wsdev 插件导出（仅 extern "C" 入口符号），
# 无 C++ 类导入库 —— 故领域类源码（EquipmentDB.cpp）直接编入测试 exe，
# 不链接插件目标（six_dof 走 wsf_project_template 全类导出，不适用本插件形态）。
#
# 前置：GTest_FOUND（全局已 find_package）且插件 SQLite 链接已命中
#（YSIM_EQDB_SQLITE_FOUND，父目录作用域变量）且未显式 OFF ——
# 无 SQLite 降级构建下领域方法恒返回错误值，单元测试无意义，静默跳过。

file(GLOB SRCS *.cpp *.hpp)

if(GTest_FOUND AND YSIM_EQDB_SQLITE_FOUND AND NOT YSIM_EQDB_SQLITE_SUPPORT STREQUAL "OFF")
   add_executable(ysim_equipment_db_test ${SRCS} ../source/EquipmentDB.cpp)
   target_link_libraries(ysim_equipment_db_test PUBLIC
      ${WSF_LIBS}
      GTest::Main
      GTest::GTest
      ${SWDEV_THREAD_LIB}
      ${SWDEV_DL_LIB}
   )
   # 测试夹具库需要 sqlite3 C API（与插件同一 3rd_party 解析根）
   target_include_directories(ysim_equipment_db_test PRIVATE "${YSIM_EQDB_SQLITE_ROOT}/include")
   # 领域类头文件目录（EquipmentDB.hpp；不再经插件目标传递）
   target_include_directories(ysim_equipment_db_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../source")
   target_link_libraries(ysim_equipment_db_test PRIVATE
      debug "${YSIM_EQDB_SQLITE_ROOT}/debug/sqlite3.lib"
      optimized "${YSIM_EQDB_SQLITE_ROOT}/release/sqlite3.lib")

   add_test(NAME "ysim_equipment_db" COMMAND ysim_equipment_db_test)
   set_property(TARGET ysim_equipment_db_test PROPERTY FOLDER UnitTests)
else()
   message(STATUS "ysim_equipment_db: unit tests skipped (require GTest + SQLite link).")
endif()
