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
# 二次开发（ext-02）：ysim_xio_gateway XIO 协议网关插件
#
# 设计目标（.trae/specs/ext-02-protocol-gateway/spec.md §4）：
#   定义私有 XIO Packet 族（ID 350-352，packetio 序列化），把平台生命周期/
#   位置姿态经 WsfXIO_Interface 广播给外部 XIO 客户端，并订阅上行 Command
#   Packet；零侵入 WSF 核心源码，DLL 部署至 mission_plugins。
project(ysim_xio_gateway)

include(swdev_project)

file(GLOB SRCS source/*.?pp)

add_library(${PROJECT_NAME} ${SRCS})
# ws2_32：XIO 底层 packetio 所需 Winsock 运行库
target_link_libraries(${PROJECT_NAME} ${WSF_LIBS} ws2_32)

# Set the subfolder that the target will appear under in an IDE's solution explorer.
set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "wsf_plugins")

# 部署到 mission_plugins 目录：mission.exe 启动时自动加载（无需 require_plugin 命令）
swdev_plugin_install(${PROJECT_NAME} mission_plugins)
swdev_warning_level(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/source")
