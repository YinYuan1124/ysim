// ****************************************************************************
// CUI
//
// The Advanced Framework for Simulation, Integration, and Modeling (AFSIM)
//
// Copyright 2020 Infoscitex, a DCS Company. All rights reserved.
//
// The use, dissemination or disclosure of data in this file is subject to
// limitation or restriction. See accompanying README and LICENSE for details.
// ****************************************************************************

#ifndef UT_MSG_CATALOG_HPP
#define UT_MSG_CATALOG_HPP

// ============================================================================
// 二次开发（i18n）：L2 消息层 —— 消息 ID 化目录（MsgCatalog）
//
// 设计目标（对应《AFSIM 汉化 i18n 总体设计》决策 1"消息 ID 化"）：
//   1. 业务代码不再硬编码英文报错句子，只 emit 消息 ID + 结构化参数；
//   2. 日志双通道 —— 落盘/日志走英文原文 + 消息 ID（官方排错可 grep、可对照），
//      界面显示走当前语言译文（中文用户看中文）；
//   3. 英文文案可自由修订而译文不受影响；后续扩展语言边际成本趋近于零。
//
// 分层约束：
//   - 本类位于 util 基础库，不依赖 Qt（内核层可直接使用）；
//   - 译文查找发生在消费端（L1/L2），L3 内核只传 ID 与参数；
//   - 目录表编译期内置（见 UtMsgCatalog.cpp），无运行时文件依赖，
//     后续若需外置覆盖可在此类内部扩展，接口不变。
//
// 使用方式：
//   // ① 业务侧：只发 ID 和参数（勿拼句子）
//   ut::msg::Emit(ut::msg::MSG_WIZARD_INVALID_DIR,
//                 {{"dir", aDirName}, {"path", aDirPath.GetSystemPath()}});
//   // ② 界面侧：按用户语言格式化（L1 查询通道）
//   std::string text = ut::msg::Format(id, args, ut::msg::Lang::cZH);
// ============================================================================

#include <cstdint>
#include <map>
#include <string>

#include "ut_export.h"

namespace ut
{
namespace msg
{
   // ============================ 消息 ID 分段表 ============================
   // 二次开发（i18n）：ID 分段原则 —— 高 16 位为域编号，低 16 位为域内序号。
   // ID 一经分配永不复用、永不移除（保证日志/文档中的 ID 长期有效）。
   //   0x0001xxxx 想定解析（scenario parse）
   //   0x0002xxxx 平台/运动（platform / mover）
   //   0x0003xxxx 传感器（sensor）
   //   0x0004xxxx 交战（engagement）
   //   0x0005xxxx 通信（comm）
   //   0x0006xxxx 工具/UI（wizard / warlock 等应用层）
   // ========================================================================
   constexpr uint32_t MSG_SCENARIO_GROUP     = 0x00010000;
   constexpr uint32_t MSG_PLATFORM_GROUP     = 0x00020000;
   constexpr uint32_t MSG_SENSOR_GROUP       = 0x00030000;
   constexpr uint32_t MSG_ENGAGEMENT_GROUP   = 0x00040000;
   constexpr uint32_t MSG_COMM_GROUP         = 0x00050000;
   constexpr uint32_t MSG_TOOL_UI_GROUP      = 0x00060000;

   // ---- 想定解析/启动段（0x0001xxxx）已分配的消息 ID ----
   constexpr uint32_t MSG_SIM_CREATE_FAILED      = 0x00010001; ///< 仿真对象创建失败
   constexpr uint32_t MSG_EVENTPIPE_OR_DUP       = 0x00010002; ///< event_pipe message_id_or 重复
   constexpr uint32_t MSG_EVENTPIPE_MSGID_DUP    = 0x00010003; ///< event_pipe message_id 重复

   // ---- 平台/运动段（0x0002xxxx）已分配的消息 ID ----
   constexpr uint32_t MSG_PLATFORM_NULL_PTR      = 0x00020001; ///< 添加空平台指针
   constexpr uint32_t MSG_PLATFORM_ADDED_TWICE   = 0x00020002; ///< 重复添加同一平台
   constexpr uint32_t MSG_PLATFORM_NO_NAME       = 0x00020003; ///< 添加未命名平台
   constexpr uint32_t MSG_PLATFORM_DUP_NAME      = 0x00020004; ///< 与现有平台同名
   constexpr uint32_t MSG_PLATFORM_BEFORE_INIT   = 0x00020005; ///< Initialize() 前添加平台
   constexpr uint32_t MSG_COMPONENT_PREINIT_FAIL = 0x00020006; ///< 组件预初始化失败
   constexpr uint32_t MSG_COMPONENT_INIT1_FAIL   = 0x00020007; ///< 组件第一阶段初始化失败
   constexpr uint32_t MSG_COMPONENT_INIT2_FAIL   = 0x00020008; ///< 组件第二阶段初始化失败
   constexpr uint32_t MSG_CALLBACK_NOT_FOUND     = 0x00020009; ///< 回调不存在
   constexpr uint32_t MSG_PLAT_TASK_BAD_TRACKPROC  = 0x0002000A; ///< 任务管理器 track processor 类型错误
   constexpr uint32_t MSG_PLAT_TASK_NO_TRACKPROC   = 0x0002000B; ///< 任务管理器找不到 track processor
   constexpr uint32_t MSG_PLAT_TASK_UNKNOWN_RES_ACQ = 0x0002000C; ///< AcquireResource 资源类型未知
   constexpr uint32_t MSG_PLAT_TASK_UNKNOWN_RES_REL = 0x0002000D; ///< ReleaseResource 资源类型未知
   constexpr uint32_t MSG_PLAT_TASK_NO_COMM_FOR_ADDR = 0x0002000E; ///< 地址找不到 comm 设备
   constexpr uint32_t MSG_PLAT_TASK_SEND_NO_COMM    = 0x0002000F; ///< 发消息时 comm 设备不存在
   constexpr uint32_t MSG_PLAT_TASK_SEND_NO_RCPT     = 0x00020010; ///< 发消息时接收者已不存在
   constexpr uint32_t MSG_PLAT_TASK_SEND_NO_TGT_PROC = 0x00020011; ///< 发消息时目标处理器不存在
   constexpr uint32_t MSG_PLAT_TASK_ASSIGN_INTERNAL  = 0x00020012; ///< 任务分配内部错误
   constexpr uint32_t MSG_PLAT_TASK_CANCEL_NOT_FOUND  = 0x00020013; ///< 取消任务时任务不存在
   constexpr uint32_t MSG_PLAT_AERO_ZERO_ASPECT      = 0x00020014; ///< aero 展弦比为零
   constexpr uint32_t MSG_PLAT_AERO_ZERO_OSWALD       = 0x00020015; ///< aero 奥斯瓦尔德因子为零
   constexpr uint32_t MSG_PLAT_AERO_NO_ZERO_LIFT_CD   = 0x00020016; ///< aero 缺 zero_lift_cd
   constexpr uint32_t MSG_PLAT_AERO_TOO_FEW_MACH      = 0x00020017; ///< aero 阻力曲线 Mach 数不足
   constexpr uint32_t MSG_PLAT_AERO_NO_TRANSONIC_RISE = 0x00020018; ///< aero 跨音速段阻力系数未上升
   constexpr uint32_t MSG_PLAT_AERO_BAD_TRANSONIC     = 0x00020019; ///< aero 跨音速起止 Mach 颠倒
   constexpr uint32_t MSG_PLAT_AERO_BAD_SUPERSONIC   = 0x0002001A; ///< aero 超音速 Mach 未高于跨音速段
   constexpr uint32_t MSG_PLAT_ROAD_NO_ROUTE          = 0x0002001B; ///< 公路运动体找不到路线
   constexpr uint32_t MSG_PLAT_ROAD_NO_PATH           = 0x0002001C; ///< 公路运动体最短路径生成失败
   constexpr uint32_t MSG_PLAT_ROAD_POS_PAIR          = 0x0002001D; ///< start/end_position 必须成对
   constexpr uint32_t MSG_PLAT_ROAD_NO_SPEED           = 0x0002001E; ///< 公路运动体缺 speed 指定
   constexpr uint32_t MSG_PLAT_EXCH_CONTAINER_INIT    = 0x0002001F; ///< 交换处理器 container 初始化失败
   constexpr uint32_t MSG_PLAT_EXCH_TRANSACTOR_INIT   = 0x00020020; ///< 交换处理器 transactor 初始化失败
   constexpr uint32_t MSG_PLAT_EXCH_SCRIPT_COMPLETE   = 0x00020021; ///< on_transaction_complete 脚本校验失败
   constexpr uint32_t MSG_PLAT_EXCH_SCRIPT_REQ_FAIL   = 0x00020022; ///< on_request_failed 脚本校验失败
   constexpr uint32_t MSG_PLAT_MSGPROC_NO_MESSAGE_VAR = 0x00020023; ///< 找不到 MESSAGE 变量
   constexpr uint32_t MSG_PLAT_MSGPROC_NO_SCRIPT       = 0x00020024; ///< 找不到消息处理器脚本
   constexpr uint32_t MSG_PLAT_MSGPROC_SCRIPT_UNRESOLVED = 0x00020025; ///< 消息处理器脚本引用无法解析

   // ---- 平台/运动段（0x0002xxxx）processor 域（第十一批）----
   constexpr uint32_t MSG_GC_NO_PHASES           = 0x00020026; ///< 制导计算机未定义任何阶段
   constexpr uint32_t MSG_GC_BAD_INIT_PHASE      = 0x00020027; ///< 初始阶段名无效
   constexpr uint32_t MSG_GC_PHASE_INIT_FAIL     = 0x00020028; ///< 制导阶段初始化失败
   constexpr uint32_t MSG_GC_NO_PROGRAM          = 0x00020029; ///< 找不到阶段引用的制导程序
   constexpr uint32_t MSG_GC_PROG_INIT_FAIL      = 0x0002002A; ///< 制导程序初始化失败
   constexpr uint32_t MSG_GC_BAD_AZ_DIR          = 0x0002002B; ///< aimpoint_azimuth_offset 方向值无效
   constexpr uint32_t MSG_GC_BAD_TARGET         = 0x0002002C; ///< guidance_target 取值无效
   constexpr uint32_t MSG_GC_BAD_PN_METHOD       = 0x0002002D; ///< 未知的比例导引方法
   constexpr uint32_t MSG_GC_SCRIPT_BAD_PHASE    = 0x0002002E; ///< 脚本命令指定的阶段名无效
   constexpr uint32_t MSG_GC_BAD_PHASE_VAR       = 0x0002002F; ///< 未知的阶段切换变量
   constexpr uint32_t MSG_GC_PHASE_MISSING       = 0x00020030; ///< 请求的制导阶段不存在
   constexpr uint32_t MSG_GC_NEXT_PHASE_UNDEF    = 0x00020031; ///< next_phase 引用未定义阶段
   constexpr uint32_t MSG_GC_OFFSET_PAIR         = 0x00020032; ///< 瞄准点方位/距离偏移量须成对
   constexpr uint32_t MSG_GP_NEEDS_GUIDED_MOVER  = 0x00020033; ///< 制导程序仅适用于 WSF_GUIDED_MOVER
   constexpr uint32_t MSG_GP_NO_FPA              = 0x00020034; ///< 期望飞行路径角未定义
   constexpr uint32_t MSG_PP_NO_ASSET_SCRIPT     = 0x00020035; ///< 找不到资产评分函数脚本
   constexpr uint32_t MSG_PP_ASSET_SIG           = 0x00020036; ///< 资产评分函数签名不符
   constexpr uint32_t MSG_PP_NO_THREAT_SCRIPT    = 0x00020037; ///< 找不到威胁评分函数脚本
   constexpr uint32_t MSG_PP_THREAT_SIG          = 0x00020038; ///< 威胁评分函数签名不符
   constexpr uint32_t MSG_FUSE_A2G_AGL_SET       = 0x00020039; ///< 空对地模式与上方引爆判据冲突
   constexpr uint32_t MSG_FUSE_A2G_AGL_NONZERO   = 0x0002003A; ///< 空对地模式与上方引爆高度冲突
   constexpr uint32_t MSG_FUSE_NO_ENGAGEMENT     = 0x0002003B; ///< 创建武器交战对象失败
   constexpr uint32_t MSG_WTP_BAD_CLASSIFIER     = 0x0002003C; ///< classifier 处理器类型错误
   constexpr uint32_t MSG_WTP_NO_PK_CALC         = 0x0002003D; ///< 找不到武器的 Pk 计算器
   constexpr uint32_t MSG_QA_NO_SCRIPT           = 0x0002003E; ///< 找不到量子分配器脚本
   constexpr uint32_t MSG_QA_BAD_SCRIPT          = 0x0002003F; ///< 量子分配器脚本签名无效
   constexpr uint32_t MSG_QG_NO_SCRIPT           = 0x00020040; ///< 找不到量子生成器脚本
   constexpr uint32_t MSG_QG_BAD_SCRIPT          = 0x00020041; ///< 量子生成器脚本签名无效
   constexpr uint32_t MSG_QE_NO_SCRIPT           = 0x00020042; ///< 找不到量子评估器脚本
   constexpr uint32_t MSG_QE_BAD_SCRIPT          = 0x00020043; ///< 量子评估器脚本签名无效
   constexpr uint32_t MSG_UPLINK_BAD_PATH        = 0x00020044; ///< uplink_path 引用未知传感器或通信设备
   constexpr uint32_t MSG_WTRACK_NO_SENSOR       = 0x00020045; ///< 武器跟踪处理器找不到传感器
   constexpr uint32_t MSG_QT_NO_TASK_REJECT      = 0x00020046; ///< 找不到要拒绝的任务

   // ---- 平台/运动段（0x0002xxxx）mover 域散点（第十二批）----
   constexpr uint32_t MSG_HM_NO_FOLLOWER         = 0x00020047; ///< 混合运动体缺 follower_mover
   constexpr uint32_t MSG_HM_NO_WAYPOINT          = 0x00020048; ///< 混合运动体缺 waypoint_mover
   constexpr uint32_t MSG_HM_NO_CURRENT_TYPE     = 0x00020049; ///< 混合运动体缺 current_mover 类型
   constexpr uint32_t MSG_AERO_NO_PLATFORM       = 0x0002004A; ///< aero 初始化时未关联平台
   constexpr uint32_t MSG_AERO_ZERO_REF_AREA     = 0x0002004B; ///< aero 参考面积为零
   constexpr uint32_t MSG_OM_NO_REF_PLATFORM     = 0x0002004C; ///< 偏移运动体缺参考平台
   constexpr uint32_t MSG_PF_NO_VALID_POINT      = 0x0002004D; ///< 目标点附近找不到有效点
   constexpr uint32_t MSG_PF_INIT_FAIL          = 0x0002004E; ///< 路径查找器初始化失败
   constexpr uint32_t MSG_RN_NO_ROUTE_ASSOC      = 0x0002004F; ///< 源/目的节点间无关联路线
   constexpr uint32_t MSG_TRF_NO_TABLE           = 0x00020050; ///< 表格驱动油耗缺油耗率表
   constexpr uint32_t MSG_TRF_NO_MODE_TABLE      = 0x00020051; ///< 表格驱动油耗当前模式无表
   constexpr uint32_t MSG_TRF_NO_FUEL_TABLE      = 0x00020052; ///< 表格驱动油耗模式缺 fuel_table
   constexpr uint32_t MSG_VRF_NO_TABLE           = 0x00020053; ///< 变速率油耗缺油耗率表
   constexpr uint32_t MSG_VRF_NO_ALTITUDES      = 0x00020054; ///< 变速率油耗表无高度数据
   constexpr uint32_t MSG_VRF_NO_MODE_TABLE      = 0x00020055; ///< 变速率油耗当前模式无表
   constexpr uint32_t MSG_VRF_NO_TABLE_MODE      = 0x00020056; ///< 变速率油耗计算时模式无表
   constexpr uint32_t MSG_TF_NO_MAX_RATE         = 0x00020057; ///< 可受油但未指定最大受油率
   constexpr uint32_t MSG_TF_CANNOT_SUPPLY      = 0x00020058; ///< 加油机无法向受油机供油
   constexpr uint32_t MSG_TSPI_OPEN_FAIL        = 0x00020059; ///< 无法打开 TSPI 文件
   constexpr uint32_t MSG_SP_EMPTY_MATRIX        = 0x0002005A; ///< 最短路径代价矩阵为空
   constexpr uint32_t MSG_SP_NOT_SQUARE          = 0x0002005B; ///< 最短路径代价矩阵非方阵
   constexpr uint32_t MSG_SP_NOT_SYMMETRIC       = 0x0002005C; ///< 最短路径代价矩阵不对称
   constexpr uint32_t MSG_SP_BAD_NODE_IDX        = 0x0002005D; ///< 最短路径节点索引越界

   // ---- 平台/运动段（0x0002xxxx）wsf_mil mover 域（第十三批）----
   constexpr uint32_t MSG_SLM_NO_TRAJECTORY         = 0x0002005E; ///< 直线运动体未指定轨迹类型
   constexpr uint32_t MSG_SLM_MULTIPLE_TRAJECTORY    = 0x0002005F; ///< 直线运动体指定了多个轨迹类型
   constexpr uint32_t MSG_SLM_ZERO_EJECTION         = 0x00020060; ///< 直线运动体弹射速度为零
   constexpr uint32_t MSG_TOWED_NO_ENGAGEMENT        = 0x00020061; ///< 拖曳运动体初始化时找不到武器交战对象
   constexpr uint32_t MSG_GM_TBM_STAGE_INIT_FAIL     = 0x00020062; ///< 阶段初始化失败（两文件共用）
   constexpr uint32_t MSG_GM_NEED_DIVERT_FUEL_MASS    = 0x00020063; ///< divert_thrust 须与 divert_fuel_mass 同用
   constexpr uint32_t MSG_GM_NEED_DIVERT_FUEL_RATE   = 0x00020064; ///< divert_thrust 须与 divert_fuel_flow_rate 同用
   constexpr uint32_t MSG_GM_NOZZLE_AREA_INCONSISTENT = 0x00020065; ///< nozzle_exit_area 与推力导出值不一致
   constexpr uint32_t MSG_GM_CANNOT_CALC_SL_SI       = 0x00020066; ///< 无法计算 sea_level_specific_impulse
   constexpr uint32_t MSG_GM_CANNOT_CALC_VAC_SI      = 0x00020067; ///< 无法计算 vacuum_specific_impulse
   constexpr uint32_t MSG_GM_RATIOS_NOT_EQUAL         = 0x00020068; ///< 计算出的比值不相等
   constexpr uint32_t MSG_GM_NEED_NOZZLE_AREA        = 0x00020069; ///< vacuum/sea_level_thrust 须指定 nozzle_exit_area
   constexpr uint32_t MSG_GM_CANNOT_DET_MASS         = 0x0002006A; ///< 无法确定 empty/fuel/total_mass
   constexpr uint32_t MSG_GM_MASS_INCONSISTENT       = 0x0002006B; ///< total/empty/fuel_mass 不一致
   constexpr uint32_t MSG_GM_CANNOT_DET_THRUST       = 0x0002006C; ///< 无法确定 thrust/burn_rate/specific_impulse
   constexpr uint32_t MSG_GM_VAC_THRUST_INCONSISTENT  = 0x0002006D; ///< vacuum_thrust 相关三量不一致
   constexpr uint32_t MSG_GM_SL_THRUST_INCONSISTENT  = 0x0002006E; ///< sea_level_thrust 相关三量不一致
   constexpr uint32_t MSG_GM_THRUST_INCONSISTENT     = 0x0002006F; ///< thrust/burn_rate/specific_impulse 不一致
   constexpr uint32_t MSG_GM_FUEL_EXCEEDS            = 0x00020070; ///< 燃料消耗超过 fuel_mass
   constexpr uint32_t MSG_GM_AERO_TYPE_BAD           = 0x00020071; ///< aero 类型未识别（参数化）
   constexpr uint32_t MSG_GM_AERO_MUST_SPECIFY       = 0x00020072; ///< 须指定 aero 类型或 aero none
   constexpr uint32_t MSG_TBM_CEP_BOTH              = 0x00020073; ///< cep_value 与 cep_table 不能同时指定
   constexpr uint32_t MSG_TBM_CANNOT_REACH           = 0x00020074; ///< TBM 弹道无法到达目标点
   constexpr uint32_t MSG_TBM_MAXRANGE_ITER         = 0x00020075; ///< 最大射程迭代次数超限
   constexpr uint32_t MSG_TBM_NEED_COAST_TIME        = 0x00020076; ///< 三段飞行时间须至少一个大于零
   constexpr uint32_t MSG_TBM_NEED_MASS_DETAIL       = 0x00020077; ///< 未指定 total_mass 须给出 empty+fuel
   constexpr uint32_t MSG_TBM_FUEL_AND_FRACTION      = 0x00020078; ///< fuel_mass 与 fuel_mass_fraction 不能同时非零
   constexpr uint32_t MSG_TBM_FUEL_GE_TOTAL          = 0x00020079; ///< fuel mass 须小于 total mass
   constexpr uint32_t MSG_TBM_NEED_FUEL_OR_FRACTION  = 0x0002007A; ///< 须指定 fuel_mass 或 fuel_mass_fraction
   constexpr uint32_t MSG_TBM_THRUST_OR_SI           = 0x0002007B; ///< 须指定 thrust 或 specific_impulse
   constexpr uint32_t MSG_TBM_THRUST_AND_SI          = 0x0002007C; ///< thrust 与 specific_impulse 不能同时指定
   constexpr uint32_t MSG_TBM_MASS_NOT_CONSISTENT    = 0x0002007D; ///< total/empty/fuel_mass 不一致

   // ---- 传感器段（0x0003xxxx）已分配的消息 ID ----
   constexpr uint32_t MSG_SENSOR_MTI_OPEN_FAIL      = 0x00030001; ///< MTI 表文件打开失败
   constexpr uint32_t MSG_SENSOR_MTI_NOT_ASCENDING  = 0x00030002; ///< MTI 表数值非单调递增
   constexpr uint32_t MSG_SENSOR_MTI_ROW_FAIL       = 0x00030003; ///< MTI 表行读取失败
   constexpr uint32_t MSG_SENSOR_RADAR_CLUTTER_CLONE = 0x00030004; ///< clutter_model 意外克隆
   constexpr uint32_t MSG_SENSOR_RADAR_CLUTTER_UNDEF = 0x00030005; ///< clutter_model 未定义
   constexpr uint32_t MSG_SENSOR_RADAR_CLUTTER_INIT = 0x00030006; ///< clutter_model 初始化失败
   constexpr uint32_t MSG_SENSOR_MODE_BAD_ON_SUCCESS = 0x00030007; ///< on_success 模式名未知
   constexpr uint32_t MSG_SENSOR_MODE_BAD_ON_FAILURE = 0x00030008; ///< on_failure 模式名未知
   constexpr uint32_t MSG_SENSOR_PASSIVE_BAD_FREQ_BAND = 0x00030009; ///< 频带低频大于高频
   constexpr uint32_t MSG_SENSOR_PASSIVE_BAD_MODE     = 0x0003000A; ///< 无效传感器模式
   constexpr uint32_t MSG_SENSOR_PASSIVE_BAD_BEAM_IDX = 0x0003000B; ///< 无效波束索引
   constexpr uint32_t MSG_SENSOR_PASSIVE_NO_BANDS     = 0x0003000C; ///< 未定义频带
   constexpr uint32_t MSG_SENSOR_PASSIVE_DWELL_ALL    = 0x0003000D; ///< dwell/revisit 须全频带指定
   constexpr uint32_t MSG_SENSOR_PASSIVE_DWELL_PAIR   = 0x0003000E; ///< dwell/revisit 须成对指定
   constexpr uint32_t MSG_SENSOR_PASSIVE_RET_PSOS     = 0x0003000F; ///< 频带返回失败（PSOS 模式）
   constexpr uint32_t MSG_SENSOR_PASSIVE_RET_NO_PSOS   = 0x00030010; ///< 频带返回失败（非 PSOS）
   constexpr uint32_t MSG_SENSOR_PASSIVE_BAND_NO_EXIST = 0x00030011; ///< 频带不存在
   constexpr uint32_t MSG_SENSOR_PASSIVE_ADD_PSOS      = 0x00030012; ///< 频带添加失败（PSOS 模式）
   constexpr uint32_t MSG_SENSOR_PASSIVE_ADD_NO_PSOS   = 0x00030013; ///< 频带添加失败（非 PSOS）
   constexpr uint32_t MSG_SENSOR_PASSIVE_NO_FRAME_TIME = 0x00030014; ///< frame_time 未指定
   constexpr uint32_t MSG_SENSOR_PASSIVE_BAD_BEAM_NUM  = 0x00030015; ///< 无效波束号
   constexpr uint32_t MSG_BD_NO_TYPE               = 0x00030016; ///< 波束导向器模式缺 type
   constexpr uint32_t MSG_BISTATIC_AZ_SIGMA       = 0x00030017; ///< 双基地方位角标准差超限
   constexpr uint32_t MSG_BISTATIC_EL_SIGMA       = 0x00030018; ///< 双基地俯仰角标准差超限
   constexpr uint32_t MSG_BISTATIC_BAD_DOMAIN     = 0x00030019; ///< 双基地误差表 domain 无法识别
   constexpr uint32_t MSG_LASER_DESIG_NO_EXT      = 0x0003001A; ///< 激光指示器缺 laser_designations 扩展
   constexpr uint32_t MSG_LASER_DESIG_BAD_SENSOR  = 0x0003001B; ///< 激光指示器模式传感器类型错误
   constexpr uint32_t MSG_LASER_TRACKER_NO_EXT    = 0x0003001C; ///< 激光跟踪器缺 laser_designations 扩展
   constexpr uint32_t MSG_SW_RADAR_EM_BAD_SENSOR   = 0x0003001D; ///< 误差模型不适用于表面波雷达
   constexpr uint32_t MSG_OTH_EM_BAD_SENSOR       = 0x0003001E; ///< 误差模型不适用于超视距雷达
   constexpr uint32_t MSG_EOIR_NO_FRAME_TIME      = 0x0003001F; ///< EOIR 缺 frame_time
   constexpr uint32_t MSG_EOIR_RES_OR_PIXEL       = 0x00030020; ///< EOIR 缺角分辨率或像素数
   constexpr uint32_t MSG_EOIR_RES_TOO_SMALL      = 0x00030021; ///< EOIR 角分辨率过小
   constexpr uint32_t MSG_SAR_NO_RESOLUTION       = 0x00030022; ///< SAR 未指定 resolution
   constexpr uint32_t MSG_SAR_NO_DWELL_TIME        = 0x00030023; ///< SAR 未指定 dwell_time
   constexpr uint32_t MSG_SAR_ERR_STATS_NO_INIT    = 0x00030024; ///< SAR 误差统计未初始化
   constexpr uint32_t MSG_LADAR_NO_PRF            = 0x00030025; ///< LADAR 发射机缺 PRF/PRF 间隔
   constexpr uint32_t MSG_LADAR_NO_THRESH         = 0x00030026; ///< LADAR 缺检测阈值/检测概率
   constexpr uint32_t MSG_EOIR_MODE_NO_BAND        = 0x00030027; ///< EOIR/IRST 模式未定义频带
   constexpr uint32_t MSG_EOIR_MODE_NO_NEI        = 0x00030028; ///< EOIR/IRST 缺噪声等效辐照度
   constexpr uint32_t MSG_EOIR_MODE_NO_THRESH     = 0x00030029; ///< EOIR/IRST 缺检测阈值
   constexpr uint32_t MSG_EOIR_MODE_ATTEN_INIT     = 0x0003002A; ///< EOIR/IRST 衰减模型初始化失败
   constexpr uint32_t MSG_OTH_CLUTTER_CLONE       = 0x0003002B; ///< OTH clutter_model 意外克隆
   constexpr uint32_t MSG_OTH_CLUTTER_UNDEF       = 0x0003002C; ///< OTH clutter_model 未定义
   constexpr uint32_t MSG_OTH_CLUTTER_INIT        = 0x0003002D; ///< OTH clutter_model 初始化失败
   constexpr uint32_t MSG_OTH_MTI_NO_PRF          = 0x0003002E; ///< OTH mti_processor 缺 PRF
   constexpr uint32_t MSG_OTH_MTI_NO_MAX_RANGE     = 0x0003002F; ///< OTH mti_processor 缺 maximum_range
   constexpr uint32_t MSG_OTH_MTI_NO_IPP          = 0x00030030; ///< OTH mti_processor 脉间周期须成对
   constexpr uint32_t MSG_OTH_MTI_DUP_TABLE        = 0x00030031; ///< OTH MTI 修正表重复指定

   // ---- 交战/武器段（0x0004xxxx）已分配的消息 ID ----
   constexpr uint32_t MSG_WEAPON_EFFECTS_UNDEFINED     = 0x00040001; ///< 平台武器的 weapon_effects 未定义
   constexpr uint32_t MSG_WEAPON_ENGAGE_INIT_FAIL      = 0x00040002; ///< 武器平台扩展初始化交战失败
   constexpr uint32_t MSG_WEAPON_LAUNCHED_TYPE_UNDEF   = 0x00040003; ///< 发射的平台类型未定义
   constexpr uint32_t MSG_WEAPON_LAUNCH_SOL_MISSING    = 0x00040004; ///< 发射计算机解缺失或不完整（发射被抑制）
   constexpr uint32_t MSG_WEAPON_DEFERRED_INIT_FAIL    = 0x00040005; ///< 延迟发射的武器平台初始化失败
   constexpr uint32_t MSG_WEAPON_ADD_TO_SIM_FAIL       = 0x00040006; ///< 无法将武器加入仿真
   constexpr uint32_t MSG_LC_NULL_WEAPON               = 0x00040007; ///< 发射计算机收到空武器指针
   constexpr uint32_t MSG_LC_SPECIFY_TWO               = 0x00040008; ///< 三选二输入缺失（TOF/滑行/推力时长）
   constexpr uint32_t MSG_LC_NEG_THRUST_DURATION       = 0x00040009; ///< 计算出负推力持续时间
   constexpr uint32_t MSG_LC_NEG_COAST_DURATION        = 0x0004000A; ///< 计算出负滑行持续时间
   constexpr uint32_t MSG_LC_NO_BURNOUT_SPEED          = 0x0004000B; ///< 未提供燃尽速度
   constexpr uint32_t MSG_LC_NO_MIN_TERMINAL_SPEED     = 0x0004000C; ///< 未提供最小末速
   constexpr uint32_t MSG_ATA_LC_ASPECT_BIN_COUNT     = 0x0004000D; ///< aspect_angle_bin_count 小于 3
   constexpr uint32_t MSG_ATA_LC_AZIMUTH_BIN_COUNT     = 0x0004000E; ///< azimuth_bin_count 小于 3
   constexpr uint32_t MSG_ATA_LC_ELEVATION_BIN_COUNT   = 0x0004000F; ///< elevation_bin_count 小于 3
   constexpr uint32_t MSG_ATA_LC_RANGE_RATE_BIN_COUNT  = 0x00040010; ///< range_rate_bin_count 小于 3
   constexpr uint32_t MSG_ATA_LC_RANGE_BIN_COUNT       = 0x00040011; ///< range_bin_count 小于 3
   constexpr uint32_t MSG_ATA_LC_AZIMUTH_LIMIT         = 0x00040012; ///< azimuth_limit 非正
   constexpr uint32_t MSG_ATA_LC_ELEVATION_LIMITS      = 0x00040013; ///< elevation 上下限大小颠倒
   constexpr uint32_t MSG_ATA_LC_RANGE_LIMITS          = 0x00040014; ///< range 上下限大小颠倒
   constexpr uint32_t MSG_ATA_LC_RANGE_RATE_LIMITS     = 0x00040015; ///< range_rate 上下限大小颠倒
   constexpr uint32_t MSG_ATA_LC_RANGE_NOT_INCREASING  = 0x00040016; ///< Range 值非连续递增
   constexpr uint32_t MSG_SAM_LC_OFFSETS_SIZE    = 0x00040017; ///< SAM 横向偏移数组元素数不足
   constexpr uint32_t MSG_SAM_LC_OFFSETS_MONO   = 0x00040018; ///< SAM 横向偏移数组非单调递增
   constexpr uint32_t MSG_SAM_LC_ALT_SIZE        = 0x00040019; ///< SAM 高度数组元素数不足
   constexpr uint32_t MSG_SAM_LC_ALT_MONO       = 0x0004001A; ///< SAM 高度数组非单调递增
   constexpr uint32_t MSG_SAM_LC_RANGE_SIZE     = 0x0004001B; ///< SAM 目标距离数组元素数不足
   constexpr uint32_t MSG_SAM_LC_RANGE_MONO      = 0x0004001C; ///< SAM 目标距离数组非单调递增
   constexpr uint32_t MSG_SAM_LC_SPEED_SIZE      = 0x0004001D; ///< SAM 目标速度数组元素数不足
   constexpr uint32_t MSG_SAM_LC_SPEED_MONO      = 0x0004001E; ///< SAM 目标速度数组非单调递增
   constexpr uint32_t MSG_SAM_LC_TOF_INTERP_FAIL = 0x0004001F; ///< SAM 飞行时间插值失败
   constexpr uint32_t MSG_BM_LC_NO_TABLE        = 0x00040020; ///< BM 未提供弹道表
   constexpr uint32_t MSG_BM_LC_TARGET_OBSOLETE  = 0x00040021; ///< BM 弹道目标数据过期
   constexpr uint32_t MSG_BM_LC_INPUT_ERROR     = 0x00040022; ///< BM 文件输入错误（3 处共用）
   constexpr uint32_t MSG_BM_LC_FILE_OPEN_FAIL   = 0x00040023; ///< BM 弹道文件打开失败
   constexpr uint32_t MSG_BM_LC_GROUND_NOT_ASC   = 0x00040024; ///< BM 地面距离非递增
   constexpr uint32_t MSG_BM_LC_TOF_NOT_ASC      = 0x00040025; ///< BM 飞行时间非单调递增
   constexpr uint32_t MSG_BM_LC_EMPTY_TRAJ       = 0x00040026; ///< BM 文件含空弹道
   constexpr uint32_t MSG_BM_LC_NO_TRAJ          = 0x00040027; ///< BM 文件无弹道
   constexpr uint32_t MSG_TAB_LETH_NO_CEP_PK      = 0x00040028; ///< 表格化杀伤未指定 CEP/Pk
   constexpr uint32_t MSG_TAB_LETH_PK_NOT_DEC      = 0x00040029; ///< 表格化杀伤 Pk 未随 CEP 递减
   constexpr uint32_t MSG_HEL_LETH_NO_EFFECT       = 0x0004002A; ///< HEL 杀伤未定义效应
   constexpr uint32_t MSG_HEL_LETH_TABLE_ROWS      = 0x0004002B; ///< HEL Pk-能量表行数不足
   constexpr uint32_t MSG_GRAD_LETH_NO_RADIUS_PK  = 0x0004002C; ///< 分级杀伤未指定半径/Pk
   constexpr uint32_t MSG_GRAD_LETH_PK_NOT_DEC      = 0x0004002D; ///< 分级杀伤 Pk 未随脱靶量递减
   constexpr uint32_t MSG_HEL_LASER_BAD_TYPE       = 0x0004002E; ///< 高能激光类型无法识别
   constexpr uint32_t MSG_HEL_LASER_INTERSECT      = 0x0004002F; ///< 高能激光相交测试无效（两处共用）
   constexpr uint32_t MSG_MFL_MFK_NOT_UNITY         = 0x00040030; ///< MFK 表概率和不为一
   constexpr uint32_t MSG_MFL_CM_BOTH               = 0x00040031; ///< CM 易损性同时指定 PkFactor 与表
   constexpr uint32_t MSG_MFL_TABLE_NOT_FOUND       = 0x00040032; ///< 机动火力杀伤表未找到
   constexpr uint32_t MSG_SPH_LETH_RADIUS           = 0x00040033; ///< 球形杀伤最大半径不大于最小值
   constexpr uint32_t MSG_SPH_LETH_DAMAGE            = 0x00040034; ///< 球形杀伤最大伤害不大于最小值
   constexpr uint32_t MSG_SPH_LETH_THRESHOLD          = 0x00040035; ///< 球形杀伤阈值不小于最大伤害
   constexpr uint32_t MSG_HANDOFF_NO_LAUNCH_PLATFORM  = 0x00040036; ///< 发射交接无法确定发射平台
   constexpr uint32_t MSG_ATG_LC_NO_LARS              = 0x00040037; ///< ATG 发射计算机未包含任何 LAR
   constexpr uint32_t MSG_ATG_LC_NO_UPDATE_INTERVAL   = 0x00040038; ///< ATG 发射计算机父武器缺 update_interval
   constexpr uint32_t MSG_A2A_LC_FILE_PARSE           = 0x00040039; ///< 空空拦截表文件解析失败
   constexpr uint32_t MSG_CUED_LASER_NO_BEAM_DIR      = 0x0004003A; ///< 光斑指示激光武器找不到波束导向器
   constexpr uint32_t MSG_ORBITAL_LC_OPEN_READ        = 0x0004003B; ///< 轨道发射计算机文件打开失败（读）
   constexpr uint32_t MSG_ORBITAL_LC_LINE_READ        = 0x0004003C; ///< 轨道发射计算机文件行读取失败
   constexpr uint32_t MSG_ORBITAL_LC_OPEN_WRITE       = 0x0004003D; ///< 轨道发射计算机文件打开失败（写）
   constexpr uint32_t MSG_ATG_LAR_NO_ZONE             = 0x0004003E; ///< ATG 武器 LAR 未定义区域

   // ---- 交战段（0x0004xxxx）dis 武器转移（第十四批）----
   constexpr uint32_t MSG_DIS_WXFR_NO_CLONE          = 0x0004003F; ///< DIS 武器转移无法克隆武器类型
   constexpr uint32_t MSG_DIS_WXFR_NO_ADD           = 0x00040040; ///< DIS 武器转移无法添加武器对象
   constexpr uint32_t MSG_DIS_WXFR_NO_INIT           = 0x00040041; ///< DIS 武器转移无法初始化武器对象
   constexpr uint32_t MSG_DIS_WXFR_FIRE_FAIL         = 0x00040042; ///< DIS 武器转移本地武器发射失败
   constexpr uint32_t MSG_DIS_WXFR_NO_ENGAGE         = 0x00040043; ///< DIS 武器转移发射后交战指针为空

   // ---- 交战段（0x0004xxxx）ew 电子战域（第十五批）----
   constexpr uint32_t MSG_EW_SC_SCREENER_INIT       = 0x00040044; ///< EW 筛选器初始化失败
   constexpr uint32_t MSG_EW_SC_JSD_INIT            = 0x00040045; ///< EW 干扰方位探测器初始化失败
   constexpr uint32_t MSG_FT_RADAR_PLAT_COMBO       = 0x00040046; ///< 雷达/平台分布行为组合不允许
   constexpr uint32_t MSG_FT_NO_CENTROID            = 0x00040047; ///< 一致雷达分布缺参考质心
   constexpr uint32_t MSG_FT_NO_LAT                 = 0x00040048; ///< 假目标参考质心纬度输入不完整
   constexpr uint32_t MSG_FT_NO_LON                 = 0x00040049; ///< 假目标参考质心经度输入不完整
   constexpr uint32_t MSG_EW_ANT_NO_DEF             = 0x0004004A; ///< EW 效果缺天线方向图（两文件共用）
   constexpr uint32_t MSG_EW_SLC_NO_CANCEL          = 0x0004004B; ///< SLC 效果缺对消比表
   constexpr uint32_t MSG_EW_SLC_NO_CANCEL_FOR_NJ    = 0x0004004C; ///< SLC 效果对消器数目缺对消比
   constexpr uint32_t MSG_EW_FTE_FT_INIT_FAIL       = 0x0004004D; ///< 假目标效果初始化假目标失败
   constexpr uint32_t MSG_EW_FTE_NO_FT              = 0x0004004E; ///< 假目标效果找不到假目标定义
   constexpr uint32_t MSG_EW_JSD_NOTHING_REPORT     = 0x0004004F; ///< 干扰方位探测器不报告任何量
   constexpr uint32_t MSG_EW_EFF_NO_DATA            = 0x00040050; ///< 基础效果数据无效无法传播

   // ---- 通信段（0x0005xxxx）已分配的消息 ID ----
   constexpr uint32_t MSG_COMM_ROUTER_NOT_FOUND     = 0x00050001; ///< 通信设备指定的 router 不在平台上
   constexpr uint32_t MSG_COMM_ASSIGN_NETWORK_FAIL  = 0x00050002; ///< 通信设备入网失败（按网络名）
   constexpr uint32_t MSG_COMM_CREATE_NETWORK_FAIL  = 0x00050003; ///< 同名网络创建失败
   constexpr uint32_t MSG_COMM_ASSIGN_ADDR_FAIL     = 0x00050004; ///< 通信设备按地址入网失败
   constexpr uint32_t MSG_COMM_EXT_LINK_NO_PLATFORM = 0x00050005; ///< 外部链路创建时平台不存在
   constexpr uint32_t MSG_COMM_EXT_LINK_NO_COMM     = 0x00050006; ///< 外部链路创建时通信设备不存在
   constexpr uint32_t MSG_COMM_EXT_LINK_NO_ADDR     = 0x00050007; ///< 外部链路创建时通信地址为空
   constexpr uint32_t MSG_COMM_MEDIUM_NO_MODES      = 0x00050008; ///< 通信介质未定义任何模式
   constexpr uint32_t MSG_COMM_MEDIUM_BAD_DEFAULT   = 0x00050009; ///< 通信介质默认模式名不存在
   constexpr uint32_t MSG_COMM_MEDIUM_NO_MODE       = 0x0005000A; ///< 发送时指定的介质模式不存在
   constexpr uint32_t MSG_COMM_MEDIUM_DUP_MSG       = 0x0005000B; ///< 重复请求发送正在传输中的消息
   constexpr uint32_t MSG_COMM_DEFAULT_NET_FULL     = 0x0005000C; ///< 默认网络 comm 对象数达上限
   constexpr uint32_t MSG_COMM_TOO_MANY_INPUTS       = 0x0005000D; ///< comm 入网参数指定过多
   constexpr uint32_t MSG_COMM_MISSING_ROUTER_ASSIGN = 0x0005000E; ///< comm 缺少 router 关联
   constexpr uint32_t MSG_COMM_ROUTER_MISSING_ON_PLAT = 0x0005000F; ///< 平台缺少 comm 引用的 router
   constexpr uint32_t MSG_COMM_MEDIUM_TYPE_INVALID   = 0x00050010; ///< 介质类型不允许添加
   constexpr uint32_t MSG_COMM_OSPF_NO_AREA           = 0x00050011; ///< OSPF 路由器缺少 area 指定
   constexpr uint32_t MSG_COMM_OSPF_AREA_DISCONNECTED = 0x00050012; ///< OSPF 区域未连骨干区域
   constexpr uint32_t MSG_COMM_OSPF_BACKBONE_INCONSIST = 0x00050013; ///< OSPF backbone 配置不一致
   constexpr uint32_t MSG_COMM_ROUTER_SEND_NO_SOURCE  = 0x00050014; ///< 路由器发送源地址为空
   constexpr uint32_t MSG_COMM_ROUTER_AUTO_LINK_FAIL = 0x00050015; ///< comm 接口自动链路创建失败

   // ---- 工具/UI 段（0x0006xxxx）已分配的消息 ID ----
   // 目录初始化失败：目录不存在或不可写（wizard 启动环境检查）
   constexpr uint32_t MSG_WIZARD_INVALID_DIR = 0x00060001;
   // 绘图指令引用了不存在的路由/路由网络（WsfDraw 命令解析）
   constexpr uint32_t MSG_DRAW_ROUTE_NOT_FOUND   = 0x00060002;
   constexpr uint32_t MSG_DRAW_NETWORK_NOT_FOUND  = 0x00060003;

   // ============================ 目录条目结构 ============================
   // 二次开发（i18n）：每条消息的完整描述。
   //   mEn      —— 英文原文（日志/落盘通道的唯一权威文本，含 {参数} 占位符）
   //   mZh      —— 中文译文（界面通道，占位符与英文版一致）
   //   mCauseZh —— 中文原因说明（可选，供帮助面板/详细诊断显示）
   //   mFixZh   —— 中文修复建议（可选，同上）
   // =====================================================================
   struct Entry
   {
      uint32_t    mId;
      const char* mEn;
      const char* mZh;
      const char* mCauseZh; ///< 可为 nullptr（无原因说明）
      const char* mFixZh;   ///< 可为 nullptr（无修复建议）
   };

   /// 消息参数表：参数名 → 参数值（UTF-8 字符串；数值由调用方自行转换）
   using Args = std::map<std::string, std::string>;

   /// 消息输出语言
   enum class Lang
   {
      cEN, ///< 英文（日志/落盘通道的固定语言）
      cZH  ///< 简体中文（界面通道按用户偏好选择）
   };

   /// 按消息 ID 查找目录条目；未注册返回 nullptr（调用方应回退英文原文）
   UT_EXPORT const Entry* Lookup(uint32_t aId);

   /// 用指定语言格式化消息：把 Args 填入对应语言模板的 {参数} 占位符。
   /// 条目缺失时回退 "[msg:XXXXXXXX]"（带 ID，保证排错信息不丢失）
   UT_EXPORT std::string Format(uint32_t aId, const Args& aArgs, Lang aLang);

   /// 英文通道格式化（日志/落盘专用快捷方式）
   UT_EXPORT std::string FormatEn(uint32_t aId, const Args& aArgs);

   /// 英文通道格式化 + "[E:ID]" 前缀 —— 供流式日志接口使用。
   /// 适用场景：报错带 AddNote() 结构化补充信息，需要保留 ut::log 流式用法时：
   ///   auto out = ut::log::error() << ut::msg::FormatEnTagged(id, args);
   ///   out.AddNote() << "Name: " << ...;
   /// 落盘日志为 "[E:XXXXXXXX] 英文原文"（可 grep ID），与 Emit 输出格式一致
   UT_EXPORT std::string FormatEnTagged(uint32_t aId, const Args& aArgs);

   /// 中文通道格式化（界面显示专用快捷方式）
   UT_EXPORT std::string FormatZh(uint32_t aId, const Args& aArgs);

   /// 日志双通道 —— 错误级：以 "[E:XXXXXXXX] 英文原文" 形式写入 ut::log
   /// （落盘永远英文+ID，界面中文由消费端另行 Format）
   UT_EXPORT void Emit(uint32_t aId, const Args& aArgs);

   /// 日志双通道 —— 警告级：以 "[W:XXXXXXXX] 英文原文" 形式写入 ut::log
   UT_EXPORT void EmitWarning(uint32_t aId, const Args& aArgs);

   /// 获取条目的中文原因说明（无则返回空串）
   UT_EXPORT std::string GetCauseZh(uint32_t aId);

   /// 获取条目的中文修复建议（无则返回空串）
   UT_EXPORT std::string GetFixZh(uint32_t aId);
} // namespace msg
} // namespace ut

#endif
