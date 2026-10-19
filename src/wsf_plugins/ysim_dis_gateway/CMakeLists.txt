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
# 二次开发（ext-02 阶段 B）：ysim_dis_gateway DIS 协议网关插件
#
# 设计目标（.trae/specs/ext-02-protocol-gateway/spec.md §6.3，批次 B1）：
#   Entity Type 七段码映射表（国产装备枚举）集中注入——经官方导出接口
#   WsfExtInterface::GetObjectToEntityTypeMap() 运行期批量注册，零侵入
#   WSF 核心源码；DLL 部署至 mission_plugins 由 mission.exe 自动加载。
project(ysim_dis_gateway)

include(swdev_project)

file(GLOB SRCS source/*.?pp)

add_library(${PROJECT_NAME} ${SRCS})
target_link_libraries(${PROJECT_NAME} ${WSF_LIBS} wsf_mil)

# DIS 头文件依赖链：WsfDisInterface.hpp → WsfDisInput.hpp → WsfDisDevice.hpp
# （source/dis 无前缀）→ Dis*.hpp（tools/dis/source）+ dis_export.h（构建树
# 生成头）——显式补齐三个 include 路径（WSF_LIBS 的 INTERFACE 路径未涵盖）
# 批次 B2 增补：wsf_mil 侧 WsfMilDisInterface.hpp（无前缀，wsf_mil/source）
# 与 dis/WsfDisWeaponEvents.hpp（wsf_mil/source/dis），并链 wsf_mil 库
# （wsf_simdis 插件同款范式：target_link_libraries(... wsf wsf_mil util)）
target_include_directories(${PROJECT_NAME} PRIVATE
   "${CMAKE_CURRENT_SOURCE_DIR}/../../core/wsf/source"
   "${CMAKE_CURRENT_SOURCE_DIR}/../../core/wsf/source/dis"
   "${CMAKE_CURRENT_SOURCE_DIR}/../../core/wsf_mil/source"
   "${CMAKE_CURRENT_SOURCE_DIR}/../../tools/dis/source"
   "${CMAKE_BINARY_DIR}/dis")

# Set the subfolder that the target will appear under in an IDE's solution explorer.
set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "wsf_plugins")

# 部署到 mission_plugins 目录：mission.exe 启动时自动加载（无需 require_plugin 命令）
swdev_plugin_install(${PROJECT_NAME} mission_plugins)
swdev_warning_level(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/source")
