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

   // ---- 想定解析/脚本段（0x0001xxxx）script 域（第十九批）----
   constexpr uint32_t MSG_SCRIPT_CB_EXEC_FAIL         = 0x00010004; ///< 无法执行回调方法
   constexpr uint32_t MSG_SCRIPT_CTX_PARSE_FAIL       = 0x00010005; ///< 脚本解析失败
   constexpr uint32_t MSG_SCRIPT_CTX_INVALID_INIT     = 0x00010006; ///< WsfScriptContext::Initialize 无效调用（3 处共用）
   constexpr uint32_t MSG_SCRIPT_CTX_INIT_FAIL        = 0x00010007; ///< 脚本上下文初始化失败（3 处共用）
   constexpr uint32_t MSG_SCRIPT_CTX_BAD_SIGNATURE    = 0x00010008; ///< 脚本签名不符
   constexpr uint32_t MSG_SCRIPT_CTX_TIMED_EVENTS_ERR = 0x00010009; ///< AddTimedEvents 内部错误
   constexpr uint32_t MSG_SCRIPT_ERROR_HEADER         = 0x0001000A; ///< 脚本出错（错误头）
   constexpr uint32_t MSG_SCRIPT_OBJ_CLASS_EXCEPTION   = 0x0001000B; ///< 脚本处理中抛出异常
   constexpr uint32_t MSG_SCRIPT_OBS_UNKNOWN_CB       = 0x0001000C; ///< 未知回调名
   constexpr uint32_t MSG_SCRIPT_OBS_NO_SCRIPT        = 0x0001000D; ///< 回调找不到脚本
   constexpr uint32_t MSG_SCRIPT_MSGH_NO_SCRIPT       = 0x0001000E; ///< 消息处理器无法解析脚本名
   constexpr uint32_t MSG_SCRIPT_PLAT_NO_COMM         = 0x0001000F; ///< 找不到 comm 设备（2 处共用）
   constexpr uint32_t MSG_SCRIPT_PLAT_NO_PROC         = 0x00010010; ///< 找不到 processor（2 处共用）
   constexpr uint32_t MSG_SCRIPT_PLAT_NO_MOVER        = 0x00010011; ///< 找不到 mover（2 处共用）
   constexpr uint32_t MSG_SCRIPT_PLAT_NO_SENSOR       = 0x00010012; ///< 找不到 sensor（2 处共用）
   constexpr uint32_t MSG_SCRIPT_PLAT_NO_ROUTER        = 0x00010013; ///< 找不到 router（2 处共用）
   constexpr uint32_t MSG_SCRIPT_EXEC_FAIL             = 0x00010014; ///< 无法执行脚本（6 处共用）
   constexpr uint32_t MSG_SCRIPT_ROUTE_IDX_RANGE       = 0x00010015; ///< 航路点索引越界（2 处共用）
   constexpr uint32_t MSG_SCRIPT_SMP_INVALID_INIT     = 0x00010016; ///< WsfScriptStateMachineProcessor::Initialize 无效调用
   constexpr uint32_t MSG_SCRIPT_SM_BAD_ENTRY_IDX      = 0x00010017; ///< 状态机入口状态索引无效（2 处共用）
   constexpr uint32_t MSG_SCRIPT_SM_BAD_TRANS_IDX     = 0x00010018; ///< 状态机转移状态索引无效（2 处共用）
   constexpr uint32_t MSG_SCRIPT_SM_EMPTY             = 0x00010019; ///< 空状态机
   constexpr uint32_t MSG_SCRIPT_SM_STATE_INIT_FAIL   = 0x0001001A; ///< 状态机状态初始化失败
   constexpr uint32_t MSG_SCRIPT_SM_NEXT_STATE_INIT   = 0x0001001B; ///< next_state 初始化失败
   constexpr uint32_t MSG_SCRIPT_SM_NO_NEXT_STATE     = 0x0001001C; ///< next_state 不存在
   constexpr uint32_t MSG_SCRIPT_SM_NO_SCRIPT          = 0x0001001D; ///< 状态机找不到脚本

   // ---- 想定解析段（0x0001xxxx）warning 级（第二十批）----
   constexpr uint32_t MSG_W_ZONE_POINTS_CIRCULAR    = 0x0001001E; ///< 极坐标/经纬点与圆形区域共用
   constexpr uint32_t MSG_W_ZONE_POINTS_ELLIPTICAL  = 0x0001001F; ///< 极坐标/经纬点与椭圆区域共用
   constexpr uint32_t MSG_W_ZONE_RADIUS_ANGLE_POLY   = 0x00010020; ///< 半径/角度约束与多边形区域共用
   constexpr uint32_t MSG_W_ZONE_POINTS_SPHERICAL    = 0x00010021; ///< 极坐标/经纬点与球形区域共用
   constexpr uint32_t MSG_W_ZONE_CMD_NOT_POLY        = 0x00010022; ///< 命令不适用于多边形区域（4 处共用）
   constexpr uint32_t MSG_W_ZONE_CMD_ONLY_POLY       = 0x00010023; ///< 命令仅适用于多边形区域（3 处共用）
   constexpr uint32_t MSG_W_ZONE_SHAPE_CONFLICT      = 0x00010024; ///< 区域定义形状类型冲突
   constexpr uint32_t MSG_W_ZONE_MIN_ALT_CONFLICT    = 0x00010025; ///< 区域定义最低高度冲突
   constexpr uint32_t MSG_W_ZONE_MAX_ALT_CONFLICT    = 0x00010026; ///< 区域定义最高高度冲突
   constexpr uint32_t MSG_W_ZONE_MIN_RADIUS_CONFLICT = 0x00010027; ///< 区域定义最小半径冲突
   constexpr uint32_t MSG_W_ZONE_MAX_RADIUS_CONFLICT = 0x00010028; ///< 区域定义最大半径冲突
   constexpr uint32_t MSG_W_ZONE_LONG_AXIS_CONFLICT  = 0x00010029; ///< 区域定义纵轴冲突
   constexpr uint32_t MSG_W_ZONE_LAT_AXIS_CONFLICT   = 0x0001002A; ///< 区域定义横轴冲突
   constexpr uint32_t MSG_W_ZONE_START_ANGLE_CONFLICT = 0x0001002B; ///< 区域定义起始角冲突
   constexpr uint32_t MSG_W_ZONE_STOP_ANGLE_CONFLICT  = 0x0001002C; ///< 区域定义终止角冲突
   constexpr uint32_t MSG_W_ZONE_IGNORE_POLYGONAL    = 0x0001002D; ///< 忽略多边形点列指定
   constexpr uint32_t MSG_W_ZONE_REL_CONFLICT        = 0x0001002E; ///< 区域参照关系冲突
   constexpr uint32_t MSG_W_ZONE_NO_REF_LOC          = 0x0001002F; ///< 缺少参照位置
   constexpr uint32_t MSG_W_ZONE_REF_PROBLEMS        = 0x00010030; ///< 区域与参照区间存在冲突（汇总）
   constexpr uint32_t MSG_W_ZONE_MODIFIERS_IGNORED   = 0x00010031; ///< 相对区域修饰值被忽略
   constexpr uint32_t MSG_W_ZONE_HEADING_IGNORED     = 0x00010032; ///< heading 输入被忽略
   constexpr uint32_t MSG_W_ZONE_ANGLES_IGNORED      = 0x00010033; ///< start/end_angle 输入被忽略
   constexpr uint32_t MSG_W_ZONE_MIN_RADIUS_IGNORED  = 0x00010034; ///< minimum_radius 输入被忽略
   constexpr uint32_t MSG_W_ZRF_UNABLE_AVOID         = 0x00010035; ///< 无法避开区域（ZoneRouteFinder）
   constexpr uint32_t MSG_W_SCN_TRUNC_INIT_RUN       = 0x00010036; ///< initial_run_number 被截断
   constexpr uint32_t MSG_W_SCN_TRUNC_FINAL_RUN      = 0x00010037; ///< final_run_number 被截断
   constexpr uint32_t MSG_W_SCN_NULL_PLATFORM         = 0x00010038; ///< WsfScenario 添加空平台指针
   constexpr uint32_t MSG_W_SCN_ADD_TWICE            = 0x00010039; ///< WsfScenario 重复添加同一平台
   constexpr uint32_t MSG_W_SCN_NO_NAME              = 0x0001003A; ///< WsfScenario 添加未命名平台
   constexpr uint32_t MSG_W_OTL_DERIVED_REDEFINED    = 0x0001003B; ///< 派生类型被重复定义
   constexpr uint32_t MSG_W_CONSOLE_OPEN_FAIL        = 0x0001003C; ///< 控制台重定向文件打开失败
   constexpr uint32_t MSG_W_EVENTOUT_DEPRECATED      = 0x0001003D; ///< PLATFORM_KILLED 事件已废弃
   constexpr uint32_t MSG_W_SIM_DELETE_NULL_PLATFORM = 0x0001003E; ///< 删除空平台指针
   constexpr uint32_t MSG_W_SIM_SCRIPT_DEFS_CHANGED  = 0x0001003F; ///< 脚本定义已变更可能降性能
   constexpr uint32_t MSG_W_SIM_COMPLETE_BAD_STATE   = 0x00010040; ///< 非期望状态无法完成仿真
   constexpr uint32_t MSG_W_SIM_RESET_BAD_STATE      = 0x00010041; ///< 非期望状态无法重置仿真
   constexpr uint32_t MSG_W_SIM_TERMINATE_BAD_STATE  = 0x00010042; ///< 非期望状态无法终止仿真
   constexpr uint32_t MSG_W_SIM_ONPLATFORM_BAD_SIG   = 0x00010043; ///< on_platform_injection 签名错误
   constexpr uint32_t MSG_W_APP_LOGHOST_BAD          = 0x00010044; ///< -log-server-host 参数处理失败
   constexpr uint32_t MSG_W_APP_LOGPORT_BAD          = 0x00010045; ///< -log-server-port 参数处理失败
   constexpr uint32_t MSG_W_APP_LOGCONN_FAIL         = 0x00010046; ///< 日志客户端连接服务器失败
   constexpr uint32_t MSG_W_FSS_FRAME_EXCEEDED       = 0x00010047; ///< 帧时间超限
   constexpr uint32_t MSG_W_PROFILING_ARG_ONE        = 0x00010048; ///< 性能分析选项须一个参数（2 处共用）
   constexpr uint32_t MSG_W_MTM_SKIP_SENSOR_UPD      = 0x00010049; ///< 跳出传感器更新
   constexpr uint32_t MSG_W_MTM_PLATFORM_NOT_FOUND   = 0x0001004A; ///< 多线程管理找不到平台
   constexpr uint32_t MSG_W_PLUGIN_LOAD_FAIL         = 0x0001004B; ///< 插件加载失败（3 处共用）
   constexpr uint32_t MSG_W_SCRIPT_ATTIME_DEP       = 0x0001004C; ///< at_time 已废弃
   constexpr uint32_t MSG_W_SCRIPTPLAT_BAD_CONFIG   = 0x0001004D; ///< 平台配置状态无效
   constexpr uint32_t MSG_W_SCRIPTPLAT_BAD_CONCEAL  = 0x0001004E; ///< 隐蔽系数无效（2 处共用）
   constexpr uint32_t MSG_W_SCRIPTPLAT_NO_ZONE      = 0x0001004F; ///< 找不到区域
   constexpr uint32_t MSG_W_SCRIPTROUTE_WP_AT_DEP  = 0x00010050; ///< GetWaypointAt 已废弃
   constexpr uint32_t MSG_W_SCRIPTROUTE_PAUSE_DEP   = 0x00010051; ///< PauseTime 已废弃
   constexpr uint32_t MSG_W_SCRIPTROUTE_SET_PAUSE_DEP = 0x00010052; ///< SetPauseTime 已废弃
   constexpr uint32_t MSG_W_SCRIPTZONE_NO_POINTS    = 0x00010053; ///< 区域无法设置点列
   constexpr uint32_t MSG_W_SCRIPTCOMM_NULL_GETTER   = 0x00010054; ///< 脚本无法向 comm 发送消息

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

   // ---- 平台/运动段（0x0002xxxx）mover 域散点补遗（第十八批）----
   constexpr uint32_t MSG_NAV_ERROR_INS_REPORT   = 0x0002007E; ///< 导航误差最终报告引出句
   constexpr uint32_t MSG_PF_FIND_CLOSEST_FAIL   = 0x0002007F; ///< 目标点附近找不到有效点
   constexpr uint32_t MSG_PG_NOT_ATTACHED        = 0x00020080; ///< 路径制导未挂接运动体
   constexpr uint32_t MSG_RN_INSERT_AREA_FAIL   = 0x00020081; ///< 路线网络 InsertArea 失败

   // ---- 平台/运动段（0x0002xxxx）traffic 交通域（第十九批）----
   constexpr uint32_t MSG_AIR_BAD_AC_TYPE          = 0x00020082; ///< 机场引用未定义的 aircraft_type
   constexpr uint32_t MSG_AIR_UNDEF_DEST           = 0x00020083; ///< 机场目的机场未定义
   constexpr uint32_t MSG_AIR_NO_ADD_PLATFORM      = 0x00020084; ///< 无法添加机场平台
   constexpr uint32_t MSG_AIR_NO_LOCAL_AC          = 0x00020085; ///< 找不到可用本地飞机类型
   constexpr uint32_t MSG_AIR_NO_LAND_AC           = 0x00020086; ///< 无飞机类型可降目的机场
   constexpr uint32_t MSG_AIR_NO_FLIGHT            = 0x00020087; ///< 找不到平台对应航班
   constexpr uint32_t MSG_AIR_NO_CREATE_AC         = 0x00020088; ///< 无法创建指定类型飞机
   constexpr uint32_t MSG_AIR_BAD_MOVER_TYPE       = 0x00020089; ///< 飞机运动体须为 WSF_AIR_MOVER
   constexpr uint32_t MSG_AIR_NO_ADD_SIM           = 0x0002008A; ///< 无法将平台加入仿真
   constexpr uint32_t MSG_AIR_DIVERT_NO_AIRBASE   = 0x0002008B; ///< 备降时找不到合适备降机场
   constexpr uint32_t MSG_TRT_INVALID_NET          = 0x0002008C; ///< 无效路线网络
   constexpr uint32_t MSG_TRT_NET_INIT_FAIL        = 0x0002008D; ///< 路线网络初始化失败
   constexpr uint32_t MSG_SEA_NO_ENTITY            = 0x0002008E; ///< sea_traffic 无法生成实体
   constexpr uint32_t MSG_SEA_NULL_MOVER           = 0x0002008F; ///< sea_traffic 运动体为空无法设路径
   constexpr uint32_t MSG_SEA_ADD_PLATFORM_FAIL    = 0x00020090; ///< sea_traffic 平台加入仿真失败
   constexpr uint32_t MSG_SEA_BAD_PLATFORM_TYPE    = 0x00020091; ///< sea_traffic 无效平台类型
   constexpr uint32_t MSG_SEA_OUTPUT_OPEN          = 0x00020092; ///< sea_traffic 输出文件打开失败
   constexpr uint32_t MSG_SEA_NO_LANE              = 0x00020093; ///< sea_traffic 航线不存在
   constexpr uint32_t MSG_SEA_LANE_WEIGHTS         = 0x00020094; ///< sea_traffic 航线权重和须为 1.0
   constexpr uint32_t MSG_SEA_PORT_WEIGHTS         = 0x00020095; ///< sea_traffic 港口权重数与航线数不符
   constexpr uint32_t MSG_SEA_REGION_WEIGHTS       = 0x00020096; ///< sea_traffic 本地交通区域权重和须为 1.0
   constexpr uint32_t MSG_SEA_TWO_PORTS            = 0x00020097; ///< sea_traffic 航线须指定两个港口
   constexpr uint32_t MSG_SEA_NO_PORT             = 0x00020098; ///< sea_traffic 港口不存在（2 处共用）
   constexpr uint32_t MSG_SEA_NO_FRACTION          = 0x00020099; ///< sea_traffic 未指定创建比例（参数化）
   constexpr uint32_t MSG_SEA_BAD_LOITER_ROUTE     = 0x0002009A; ///< sea_traffic 无效待机航线
   constexpr uint32_t MSG_OSM_BAD_VAR_TYPE         = 0x0002009B; ///< osm_traffic 变量类型不符
   constexpr uint32_t MSG_OSM_UNDEF_NET            = 0x0002009C; ///< osm_traffic 网络未定义
   constexpr uint32_t MSG_OSM_NET_NO_ENTRANCE      = 0x0002009D; ///< osm_traffic 网络有出口无入口
   constexpr uint32_t MSG_OSM_INIT_FAIL            = 0x0002009E; ///< osm_traffic 初始化失败
   constexpr uint32_t MSG_OSM_BAD_PLATFORM_TYPE    = 0x0002009F; ///< osm_traffic 无效平台类型
   constexpr uint32_t MSG_OSM_NO_VEHICLE_TYPE      = 0x000200A0; ///< osm_traffic 车辆类型不存在
   constexpr uint32_t MSG_OSM_BAD_FRACTION         = 0x000200A1; ///< osm_traffic 车辆 fraction 和须为 1.0
   constexpr uint32_t MSG_OSM_NO_CONVOY_TYPE       = 0x000200A2; ///< osm_traffic 车队车辆类型不存在
   constexpr uint32_t MSG_OSM_NULL_MOVER           = 0x000200A3; ///< osm_traffic 运动体为空无法设路径
   constexpr uint32_t MSG_OSM_NO_MOVER              = 0x000200A4; ///< 所属平台缺有效运动体
   constexpr uint32_t MSG_ROAD_UNDEF_NET           = 0x000200A5; ///< road_traffic 网络未定义
   constexpr uint32_t MSG_ROAD_BAD_MOVER           = 0x000200A6; ///< road_traffic 车队运动体类型无效
   constexpr uint32_t MSG_ROAD_INIT_FAIL           = 0x000200A7; ///< road_traffic 初始化失败
   constexpr uint32_t MSG_ROAD_BAD_PLATFORM_TYPE    = 0x000200A8; ///< road_traffic 无效平台类型
   constexpr uint32_t MSG_ROAD_NULL_MOVER          = 0x000200A9; ///< road_traffic 运动体为空无法设路径
   constexpr uint32_t MSG_ROAD_NO_RANDOM_WPT       = 0x000200AA; ///< road_traffic 静止车辆随机航点生成失败
   constexpr uint32_t MSG_ROAD_NO_RANDOM_PATH      = 0x000200AB; ///< road_traffic 随机路径生成失败
   constexpr uint32_t MSG_ROAD_NO_NEW_VEHICLE      = 0x000200AC; ///< road_traffic 无法创建新车辆
   constexpr uint32_t MSG_ROAD_NO_VEHICLE_TYPE     = 0x000200AD; ///< road_traffic 车辆类型不存在
   constexpr uint32_t MSG_ROAD_NO_CONVOY_TYPE      = 0x000200AE; ///< road_traffic 车队车辆类型不存在

   // ---- 平台/运动段（0x0002xxxx）warning 级（第二十批）----
   constexpr uint32_t MSG_W_PLAT_PRECESSION_DEP    = 0x000200AF; ///< precession_nutation_update_interval 已废弃
   constexpr uint32_t MSG_W_PLAT_SIDE_CHANGE_INIT  = 0x000200B0; ///< 初始化后改 side 被拒
   constexpr uint32_t MSG_W_PLAT_SWAPMOVER_EXT     = 0x000200B1; ///< 外部控制平台 SwapMover 失败
   constexpr uint32_t MSG_W_XMTR_POWER_ZERO       = 0x000200B2; ///< 发射机功率为零仍继续初始化
   constexpr uint32_t MSG_W_XMTR_FREQ_ZERO        = 0x000200B3; ///< 发射机频率为零仍继续初始化
   constexpr uint32_t MSG_W_ABT_CREATE_SHARED_BB  = 0x000200B4; ///< 共享黑板已存在无法创建
   constexpr uint32_t MSG_W_ABT_DELETE_BB_VAR     = 0x000200B5; ///< 黑板变量不存在无法删除
   constexpr uint32_t MSG_W_ABT_DEL_SBB_VAR_FROM  = 0x000200B6; ///< 共享黑板变量不存在无法删除
   constexpr uint32_t MSG_W_ABT_DEL_SBB_VAR_NO_SBB = 0x000200B7; ///< 共享黑板不存在无法删除变量
   constexpr uint32_t MSG_W_ABT_GET_BB_VAR        = 0x000200B8; ///< 黑板变量不存在
   constexpr uint32_t MSG_W_ABT_GET_SBB_VAR_ON    = 0x000200B9; ///< 共享黑板上找不到变量
   constexpr uint32_t MSG_W_ABT_GET_SBB_VAR_NO_SBB = 0x000200BA; ///< 共享黑板不存在找不到变量
   constexpr uint32_t MSG_W_ABT_SET_SBB_VAR_CREATE = 0x000200BB; ///< 共享黑板不存在正创建（SetVar）
   constexpr uint32_t MSG_W_ABT_SET_SBB_TRACKVAR_CREATE = 0x000200BC; ///< 共享黑板不存在正创建（SetTrackVar）
   constexpr uint32_t MSG_W_ABT_SET_SBB_PLATVAR_CREATE = 0x000200BD; ///< 共享黑板不存在正创建（SetPlatformVar）
   constexpr uint32_t MSG_W_ABT_SHARE_BB_DUP      = 0x000200BE; ///< 黑板已存在无法共享（2 处共用）
   constexpr uint32_t MSG_W_EXTLINK_RESOLVE       = 0x000200BF; ///< 无法解析外部链路（5 处共用）
   constexpr uint32_t MSG_W_EXTLINK_RESOLVE_PEER  = 0x000200C0; ///< 无法解析外部链路（对等接收者）
   constexpr uint32_t MSG_W_EXTLINK_FIND          = 0x000200C1; ///< 找不到外部链路
   constexpr uint32_t MSG_W_EXTLINK_ADD           = 0x000200C2; ///< 无法添加外部链路（4 处共用）
   constexpr uint32_t MSG_W_EXTLINK_ADD_GROUP     = 0x000200C3; ///< 无法添加组外部链路
   constexpr uint32_t MSG_W_EXTLINK_SEND_ABORT    = 0x000200C4; ///< 外部链路消息发送中止（2 处共用）
   constexpr uint32_t MSG_W_INTLINK_PART_UNABLE  = 0x000200C5; ///< 平台部件无法链接
   constexpr uint32_t MSG_W_TERRAIN_INVALID_MAP_CMD = 0x000200C6; ///< 无效地图配置命令
   constexpr uint32_t MSG_W_TERRAIN_QUERY_UNKNOWN = 0x000200C7; ///< 地形接口不认识的查询类型
   constexpr uint32_t MSG_W_TM_HANDOFF_INIT_FAIL  = 0x000200C8; ///< 移交轨迹初始化失败
   constexpr uint32_t MSG_W_TSC_SHOW_TRANSITIONS_DEP = 0x000200C9; ///< show_transitions 已废弃
   constexpr uint32_t MSG_W_ATTR_ATTRIBUTE_DEP    = 0x000200CA; ///< attribute 容器 attribute 命令已废弃
   constexpr uint32_t MSG_W_MASK_DEFAULT_STATE_MISSING = 0x000200CB; ///< default 状态不存在
   constexpr uint32_t MSG_W_NETIF_DISABLED         = 0x000200CC; ///< 按用户输入禁用接口
   constexpr uint32_t MSG_W_MSGTABLE_SUBTYPE_IGNORED = 0x000200CD; ///< default 类型 subtype 定义被忽略
   constexpr uint32_t MSG_W_PAVAIL_EXCL_ZONE_UNDEF = 0x000200CE; ///< 排除区未定义
   constexpr uint32_t MSG_W_PAVAIL_INCL_ZONE_UNDEF = 0x000200CF; ///< 包含区未定义
   constexpr uint32_t MSG_W_LOSM_INCONSISTENT_STATE = 0x000200D0; ///< LOS 管理器状态数据不一致
   constexpr uint32_t MSG_W_RADARSIG_UNDEF         = 0x000200D1; ///< 雷达特征未定义用默认（2 处共用）
   constexpr uint32_t MSG_W_SIGLIST_STATE_UNDEF    = 0x000200D2; ///< 特征状态未定义
   constexpr uint32_t MSG_W_EQFOV_NO_CUE           = 0x000200D3; ///< 铰接部件无法提供当前指向
   constexpr uint32_t MSG_W_ABT_SBBVAR_EXISTS_NO_SBB = 0x000200D4; ///< SharedBlackboardVarExists 黑板不存在
   // ---- 运动段（0x0002xxxx）mover warning 级（第二十批）----
   constexpr uint32_t MSG_W_FUEL_GT_MAX               = 0x000200D5; ///< 初始油量超最大油量被约束
   constexpr uint32_t MSG_W_FUEL_ZERO_INIT            = 0x000200D6; ///< 初始油量为零或负
   constexpr uint32_t MSG_W_WPMOVER_PAUSED            = 0x000200D7; ///< 暂停状态无法执行航线
   constexpr uint32_t MSG_W_OFFSETMOVER_TETHER         = 0x000200D8; ///< tether 附着类型仍在开发
   constexpr uint32_t MSG_W_ROUTENET_NOT_ADDED        = 0x000200D9; ///< 航线未加入路径网络
   constexpr uint32_t MSG_W_ROUTENET_NO_SHORTPATH      = 0x000200DA; ///< 无法求解最短路径
   constexpr uint32_t MSG_W_ROUTEMOVER_BAD_LATLON      = 0x000200DB; ///< 航点经纬度超出范围
   constexpr uint32_t MSG_W_NAVERR_FILE_OPEN           = 0x000200DC; ///< 导航时间历史文件打开失败
   constexpr uint32_t MSG_W_TANKEDFUEL_AMBIGUOUS       = 0x000200DD; ///< 加油供给配置歧义
   constexpr uint32_t MSG_W_ROUTE_TRANSFORM_DEP       = 0x000200DE; ///< transform_route 已废弃
   constexpr uint32_t MSG_W_MATH3D_SWITCH_ON_PASSING  = 0x000200DF; ///< switch_on_passing 未实现被忽略
   constexpr uint32_t MSG_W_MATH3D_ALT_OFFSET          = 0x000200E0; ///< altitude_offset 未实现被忽略
   constexpr uint32_t MSG_W_MATH3D_TURN_FAILURE        = 0x000200E1; ///< on_turn_failure 未实现被忽略
   // ---- 处理器段（0x0002xxxx）processor warning 级（第二十批）----
   constexpr uint32_t MSG_W_TASKMGR_BAD_SENSOR_MODE    = 0x000200E2; ///< 轨迹请求的传感器模式无效
   constexpr uint32_t MSG_W_TASKMGR_STARTTRACK_FAIL    = 0x000200E3; ///< StartTracking 失败
   constexpr uint32_t MSG_W_TASKMGR_STOPTRACK_FAIL      = 0x000200E4; ///< StopTracking 失败
   constexpr uint32_t MSG_W_TASKMGR_REPORT_COMPLETE    = 0x000200E5; ///< 上报任务完成状态
   constexpr uint32_t MSG_W_TASKMGR_REPORT_PROGRESS    = 0x000200E6; ///< 上报进度状态找不到受派者
   constexpr uint32_t MSG_W_TASKMGR_MSG_TIMEOUT        = 0x000200E7; ///< 消息超时
   constexpr uint32_t MSG_W_TASKMGR_NO_TASK            = 0x000200E8; ///< 找不到任务
   constexpr uint32_t MSG_W_PROCTYPES_TM_DEP          = 0x000200E9; ///< WSF_TRACK_MANAGER 已废弃（2 处共用）
   constexpr uint32_t MSG_W_SENPROC_ATTACH_FAIL        = 0x000200EA; ///< 传感器处理器找不到要挂接的传感器
   constexpr uint32_t MSG_W_SENPROC_REMOVE_FAIL        = 0x000200EB; ///< 传感器处理器找不到要移除的传感器
   constexpr uint32_t MSG_W_TRACKPROC_NO_EXTLINKS       = 0x000200EC; ///< 无外部通信链路不上报轨迹
   // ---- 签名段（0x0002xxxx）wsf_mil warning 级（第二十批 C）----
   constexpr uint32_t MSG_W_EXTDETON_INIT_FAIL         = 0x000200ED; ///< ExtDetonation 无法初始化交战
   constexpr uint32_t MSG_W_ACOUSTIC_NOT_DEF          = 0x000200EE; ///< 声学签名未定义用默认
   constexpr uint32_t MSG_W_ACOUSTIC_NOT_DEF2         = 0x000200EF; ///< 声学签名未定义用默认（带引号变体）
   constexpr uint32_t MSG_W_ACOUSTIC_PLAT_VAR         = 0x000200F0; ///< 平台声学签名未定义用默认（参数化）
   constexpr uint32_t MSG_W_COMPOPT_IR_BY_OPT         = 0x000200F1; ///< 红外签名由光学签名模型提供
   constexpr uint32_t MSG_W_BISTATIC_NO_DEF_POL       = 0x000200F2; ///< 双基地状态缺 default 极化
   constexpr uint32_t MSG_W_BISTATIC_MIN_AZEL         = 0x000200F3; ///< 双基地状态方位/俯仰集不足
   constexpr uint32_t MSG_W_SURFCLUT_LOW_ALT          = 0x000200F4; ///< 天线高度低于浪高
   constexpr uint32_t MSG_W_IR_NOT_DEF                = 0x000200F5; ///< 红外签名未定义用默认
   constexpr uint32_t MSG_W_MILRADAR_NO_MONO          = 0x000200F6; ///< 雷达签名缺单基地定义
   constexpr uint32_t MSG_W_MILRADAR_NO_DEF_STATE     = 0x000200F7; ///< 雷达签名缺 default 状态
   constexpr uint32_t MSG_W_MILRADAR_NO_DEF_POL       = 0x000200F8; ///< 雷达签名状态缺 default 极化
   constexpr uint32_t MSG_W_INHCON_NOT_DEF            = 0x000200F9; ///< 固有对比度未定义用默认
   constexpr uint32_t MSG_W_OPTSIG_NOT_DEF            = 0x000200FA; ///< 光学签名未定义用默认
   constexpr uint32_t MSG_W_OPTREFL_NOT_DEF           = 0x000200FB; ///< 光学反射率未定义用默认
   // ---- 导弹运动段（0x0002xxxx）wsf_mil mover warning 级（第二十批 C）----
   constexpr uint32_t MSG_W_GMB_NO_HISTFILE           = 0x000200FC; ///< 无法打开时间历史文件
   constexpr uint32_t MSG_W_GMB_STAGING_MSG           = 0x000200FD; ///< 级间分离/点火失败消息（2 处共用）
   constexpr uint32_t MSG_W_GM_RK_ACCURACY            = 0x000200FE; ///< RK 积分精度受限
   constexpr uint32_t MSG_W_GM_BAD_BURNRATE           = 0x000200FF; ///< 燃烧率可能计算有误
   // ---- wsf_mil 处理器段（0x0002xxxx）warning 级（第二十批 C）----
   constexpr uint32_t MSG_W_WTP_NO_INTERCEPT          = 0x00020100; ///< 无法计算拦截点
   constexpr uint32_t MSG_W_WTP_NO_VALID_LOC          = 0x00020101; ///< 轨迹无有效位置
   constexpr uint32_t MSG_W_WTP_NO_VALID_VEL          = 0x00020102; ///< 轨迹无有效速度
   constexpr uint32_t MSG_W_WTP_NO_CLASSIFIER         = 0x00020103; ///< 无法访问分类处理器
   constexpr uint32_t MSG_W_FUSE_GROSS_LT_FINE        = 0x00020104; ///< 粗更新间隔须大于精更新间隔
   constexpr uint32_t MSG_W_FUSE_DISABLE_DEP          = 0x00020105; ///< disable_launch_proximity 已废弃
   constexpr uint32_t MSG_W_IMGPROC_NO_PROCESS        = 0x00020106; ///< 图像处理器不处理该消息
   constexpr uint32_t MSG_W_WTM_FIRESALVO_FAIL        = 0x00020107; ///< 齐射失败
   constexpr uint32_t MSG_W_WTM_STOPJAM_FREQ_FAIL     = 0x00020108; ///< 按频段停扰失败
   constexpr uint32_t MSG_W_WTM_STOPJAM_TRACK_FAIL    = 0x00020109; ///< 按轨迹停扰失败
   constexpr uint32_t MSG_W_GUIDANCE_BAD_MOVER        = 0x0002010A; ///< 运动器类型错误制导关闭

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

   // ---- 传感器段（0x0003xxxx）ext 域（第十八批）----
   constexpr uint32_t MSG_EXT_SENSOR_INIT_FAIL    = 0x00030032; ///< 外部传感器初始化失败

   // ---- 传感器段（0x0003xxxx）sensor 域（第十九批）----
   constexpr uint32_t MSG_CS_NO_OPERATING_MODE     = 0x00030033; ///< 组合传感器缺 operating_mode
   constexpr uint32_t MSG_CS_NO_CONSTITUENTS       = 0x00030034; ///< 组合传感器未定义成员传感器
   constexpr uint32_t MSG_CS_SELF_CONSTITUENT      = 0x00030035; ///< 成员传感器与组合传感器同名
   constexpr uint32_t MSG_CS_CONSTITUENT_UNDEF     = 0x00030036; ///< 成员传感器未定义
   constexpr uint32_t MSG_CS_CONSTITUENT_NO_MODES  = 0x00030037; ///< 成员传感器须有模式列表
   constexpr uint32_t MSG_CS_CONSTITUENT_NO_SCHED  = 0x00030038; ///< 成员传感器须有调度器与跟踪器
   constexpr uint32_t MSG_CS_INTERNAL_LINK_FAIL     = 0x00030039; ///< 成员传感器内部链路添加失败
   constexpr uint32_t MSG_MS_BAD_CASE              = 0x0003003A; ///< MarcumSwerling case 无效
   constexpr uint32_t MSG_MS_BAD_NUM_PULSES        = 0x0003003B; ///< MarcumSwerling 脉冲积分数无效
   constexpr uint32_t MSG_MS_BAD_PFA               = 0x0003003C; ///< MarcumSwerling 虚警概率无效
   constexpr uint32_t MSG_PD_NO_CONT_THRESH        = 0x0003003D; ///< 连续波检测阈值未定义（2 处共用）
   constexpr uint32_t MSG_PD_NO_PULSED_THRESH      = 0x0003003E; ///< 脉冲检测阈值未定义（2 处共用）
   constexpr uint32_t MSG_PD_NO_CONT_SENS          = 0x0003003F; ///< 连续波灵敏度未定义（2 处共用）
   constexpr uint32_t MSG_PD_NO_PULSED_SENS        = 0x00030040; ///< 脉冲灵敏度未定义
   constexpr uint32_t MSG_PD_NO_PULSE_SENS         = 0x00030063; ///< 脉冲灵敏度未定义（无 d 变体原文）（2 处共用）

   // ---- 传感器段（0x0003xxxx）warning 级（第二十批）----
   constexpr uint32_t MSG_W_SECTORSCAN_INSUFFICIENT = 0x00030064; ///< 扇区定义数据不足
   constexpr uint32_t MSG_W_SENSOR_SLAVE_WHEN_ON  = 0x00030065; ///< 传感器开机时禁止改从属状态
   constexpr uint32_t MSG_W_SENSORMODE_NOT_REPORT = 0x00030066; ///< 传感器模式不上报任何内容
   constexpr uint32_t MSG_W_SENSORMODE_MISSING_FIELDS = 0x00030067; ///< 传感器模式无法上报全部请求字段
   constexpr uint32_t MSG_W_SPIN_RANDOM_AZ_IGNORED = 0x00030068; ///< 初始方位角将被随机覆盖
   constexpr uint32_t MSG_W_SPIN_RANDOM_AZ_CONFLICT = 0x00030069; ///< 随机起始方位与初始角冲突
   constexpr uint32_t MSG_W_RADAR_SIGPROC_CONFLICT = 0x0003006A; ///< signal_processor 与探测距离参数冲突
   // ---- wsf_mil 传感器段（0x0003xxxx）warning 级（第二十批 C）----
   constexpr uint32_t MSG_W_SAR_MIN_RES            = 0x0003006B; ///< SAR 分辨率低于近似最小值
   constexpr uint32_t MSG_W_SAR_ANGRES_DEP        = 0x0003006C; ///< angular_resolution 已废弃
   constexpr uint32_t MSG_W_BISTERR_RANGE_IGNORE   = 0x0003006D; ///< 距离误差由模型计算输入值被忽略
   constexpr uint32_t MSG_W_OTH_BAD_IONO          = 0x0003006E; ///< 电离层特征无效
   constexpr uint32_t MSG_W_OTH_FREQ_LT_MUF       = 0x0003006F; ///< 发射频率低于最低可用频率
   constexpr uint32_t MSG_W_OTH_SIGPROC_CONFLICT  = 0x00030070; ///< signal_processor 与探测距离参数冲突
   constexpr uint32_t MSG_W_OTH_RANGE_OVERRIDE    = 0x00030071; ///< 电离层距离计算覆盖距离限制
   constexpr uint32_t MSG_W_OTH_CMD_DEP           = 0x00030072; ///< 命令已废弃（参数化）
   constexpr uint32_t MSG_PD_NO_THRESH_OR_SENS     = 0x00030041; ///< 须定义检测阈值或灵敏度
   constexpr uint32_t MSG_MTI_ADJ_NO_TABLE         = 0x00030042; ///< data_table 型 MTI 缺 mti_data_table
   constexpr uint32_t MSG_MTI_ADJ_BAD_MODE         = 0x00030043; ///< MTI 处理器要求雷达模式
   constexpr uint32_t MSG_MTI_ADJ_NO_PRF           = 0x00030044; ///< MTI 处理器缺 PRF
   constexpr uint32_t MSG_MTI_ADJ_NO_MAX_RANGE     = 0x00030045; ///< MTI 处理器缺 maximum_range
   constexpr uint32_t MSG_MTI_ADJ_NO_IPP           = 0x00030046; ///< MTI 处理器脉间周期须成对
   constexpr uint32_t MSG_DST_NO_MODE_LIST         = 0x00030047; ///< 默认跟踪器要求模式列表
   constexpr uint32_t MSG_MTI_TABLE_READCSV_EXC    = 0x00030048; ///< MTI 表 CSV 读取抛出异常
   constexpr uint32_t MSG_MTI_TABLE_COMPOUND_OPEN  = 0x00030049; ///< 复合 MTI 表文件打开失败
   constexpr uint32_t MSG_MTI_TABLE_COMPOUND_PARSE = 0x0003004A; ///< 复合 MTI 表解析抛出异常
   constexpr uint32_t MSG_MTI_TABLE_COMPOUND_ROW   = 0x0003004B; ///< 复合 MTI 表行读取失败
   constexpr uint32_t MSG_MTI_TABLE_COMPOUND_EXC   = 0x0003004C; ///< 复合 MTI 表读取抛出异常
   constexpr uint32_t MSG_MTI_TABLE_WRITECSV_OPEN  = 0x0003004D; ///< MTI 表 CSV 写打开失败
   constexpr uint32_t MSG_MTI_TABLE_WRITEBIN_OPEN  = 0x0003004E; ///< MTI 表二进制写打开失败
   constexpr uint32_t MSG_MTI_TABLE_WRITEBIN_SPEED = 0x0003004F; ///< MTI 表二进制写 speedAxis 失败
   constexpr uint32_t MSG_MTI_TABLE_WRITEBIN_ADJ   = 0x00030050; ///< MTI 表二进制写 adjustmentAxis 失败
   constexpr uint32_t MSG_MTI_TABLE_READBIN_OPEN   = 0x00030051; ///< MTI 表二进制读打开失败
   constexpr uint32_t MSG_MTI_TABLE_READBIN_SPEED  = 0x00030052; ///< MTI 表二进制读 speedAxis 失败
   constexpr uint32_t MSG_MTI_TABLE_READBIN_ADJ    = 0x00030053; ///< MTI 表二进制读 adjustmentAxis 失败
   constexpr uint32_t MSG_MTI_TABLE_NO_FREQ        = 0x00030054; ///< 找不到频率对应的 MTI 修正表
   constexpr uint32_t MSG_DSS_NO_MODE_LIST         = 0x00030055; ///< 默认调度器要求模式列表
   constexpr uint32_t MSG_DSS_NO_FRAME_TIME        = 0x00030056; ///< 默认调度器缺 frame_time
   constexpr uint32_t MSG_DSS_BAD_MODE_REQ         = 0x00030057; ///< 请求的传感器模式无效
   constexpr uint32_t MSG_PSSS_NO_MODE_LIST        = 0x00030058; ///< 物理扫描调度器要求模式列表（参数化）
   constexpr uint32_t MSG_PSSS_NO_UPDATE_INT       = 0x00030059; ///< 物理扫描调度器缺 update_interval（参数化）
   constexpr uint32_t MSG_PSSS_NO_FRAME_TIME       = 0x0003005A; ///< 物理扫描调度器缺 frame_time
   constexpr uint32_t MSG_RSEM_NOT_RADAR           = 0x0003005B; ///< 误差模型仅适用雷达传感器
   constexpr uint32_t MSG_SSSS_NO_AZ_RATE          = 0x0003005C; ///< 扇区扫描缺方位扫描率
   constexpr uint32_t MSG_SSSS_NO_EL_RATE          = 0x0003005D; ///< 扇区扫描缺俯仰扫描率
   constexpr uint32_t MSG_SSSS_NO_UPDATE_INT       = 0x0003005E; ///< 扇区扫描调度器缺 update_interval
   constexpr uint32_t MSG_SST_NO_END_SCHED         = 0x0003005F; ///< 调度器块缺结束命令
   constexpr uint32_t MSG_SSP_INIT_FAIL           = 0x00030060; ///< 信号处理器初始化失败
   constexpr uint32_t MSG_SPIN_NO_MODE_LIST       = 0x00030061; ///< 旋转调度器要求模式列表
   constexpr uint32_t MSG_SPIN_NO_FRAME_TIME      = 0x00030062; ///< 旋转调度器缺 frame_time（参数化）

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

   // ---- 交战段（0x0004xxxx）warning 级（第二十批）----
   constexpr uint32_t MSG_W_EXT_CLONE_FAIL        = 0x00040051; ///< ExtInterface 无法克隆平台
   constexpr uint32_t MSG_W_EXT_ENTITYID_INUSE    = 0x00040052; ///< 预分配实体 ID 已被占用
   // ---- 电子战段（0x0004xxxx）wsf_mil ew warning 级（第二十批 C）----
   constexpr uint32_t MSG_W_EW_SEL_APPLY_IDX     = 0x00040053; ///< 选择技术失败：系统类型不兼容（按索引）
   constexpr uint32_t MSG_W_EW_SEL_NO_VALID_IDX   = 0x00040054; ///< 选择技术失败：索引处无有效技术
   constexpr uint32_t MSG_W_EW_SEL_IDX_TOO_LARGE = 0x00040055; ///< 选择技术失败：索引超限
   constexpr uint32_t MSG_W_EW_SEL_APPLY_NOAN    = 0x00040056; ///< 选择技术失败：系统类型不兼容
   constexpr uint32_t MSG_W_EW_SEL_NOT_FOUND_ST  = 0x00040057; ///< 选择技术失败：找不到该系统类型技术
   constexpr uint32_t MSG_W_EW_DESEL_APPLY_IDX   = 0x00040058; ///< 取消技术失败：系统类型不兼容（按索引）
   constexpr uint32_t MSG_W_EW_DESEL_NO_VALID_IDX = 0x00040059; ///< 取消技术失败：索引处无有效技术
   constexpr uint32_t MSG_W_EW_DESEL_IDX_TOO_LARGE = 0x0004005A; ///< 取消技术失败：索引超限
   constexpr uint32_t MSG_W_EW_DESEL_APPLY_FAIL   = 0x0004005B; ///< 取消技术失败：系统类型不兼容
   constexpr uint32_t MSG_W_EW_DESEL_NOT_FOUND    = 0x0004005C; ///< 取消技术失败：找不到技术
   constexpr uint32_t MSG_W_EW_DESEL_NOT_FOUND_ST = 0x0004005D; ///< 取消技术失败：找不到该系统类型技术
   constexpr uint32_t MSG_W_EAEP_SEL_NO_VALID     = 0x0004005E; ///< 选择技术失败：索引处无有效技术（冒号变体）
   constexpr uint32_t MSG_W_EAEP_SEL_IDX_LARGE     = 0x0004005F; ///< 选择技术失败：索引超限（冒号变体）
   constexpr uint32_t MSG_W_EAEP_SEL_NOT_FOUND    = 0x00040060; ///< 选择技术失败：找不到技术（冒号变体）
   constexpr uint32_t MSG_W_EA_SETDELTAGAIN_FAIL   = 0x00040061; ///< 无法设置增量增益调制技术
   constexpr uint32_t MSG_W_FT_BAD_SCAN_INT        = 0x00040062; ///< 扫描间隔值无效
   constexpr uint32_t MSG_W_FT_BAD_PERSIST         = 0x00040063; ///< 持续性值无效
   constexpr uint32_t MSG_W_FT_INIT_NFT_REDEF      = 0x00040064; ///< 假目标初始数量重复定义
   constexpr uint32_t MSG_W_SCREEN_SCAN_DEP        = 0x00040065; ///< 筛选器扫描模式已废弃（参数化，2 处共用）
   constexpr uint32_t MSG_W_EFFECTVAL_UNDEF_TYPE   = 0x00040066; ///< 未定义的数值类型字符串
   constexpr uint32_t MSG_W_EFFECTVAL_UNDEF_COH    = 0x00040067; ///< 未定义的相干检查类型字符串
   constexpr uint32_t MSG_W_EACOH_UNDEF            = 0x00040068; ///< EA 相干检查类型未定义
   constexpr uint32_t MSG_W_TECH_SETDELTAGAIN_FAIL = 0x00040069; ///< 无法设置增量增益效果
   constexpr uint32_t MSG_W_SFT_NO_PULSEWIDTH      = 0x0004006A; ///< 雷达未指定脉宽
   constexpr uint32_t MSG_W_RPJ_NO_PW_PRI           = 0x0004006B; ///< 雷达未指定脉宽和/或 PRI
   constexpr uint32_t MSG_W_FT_NO_PULSEWIDTH       = 0x0004006C; ///< 雷达未指定脉宽（FT 效果）
   constexpr uint32_t MSG_W_DST_UNDETECTED_BLIP     = 0x0004006D; ///< 假目标检测收到未检测回波
   constexpr uint32_t MSG_W_EFF_UNDEF_COH_STR      = 0x0004006E; ///< 未定义的相干类型字符串
   // ---- 武器段（0x0004xxxx）wsf_mil weapon warning 级（第二十批 C）----
   constexpr uint32_t MSG_W_WEAP_INHIBIT_DEP      = 0x0004006F; ///< 装填 inhibit 命令已废弃（参数化，2 处共用）
   constexpr uint32_t MSG_W_WEAP_CMD_INCOMPAT      = 0x00040070; ///< 命令与该武器类型不兼容（参数化，6 处共用）
   constexpr uint32_t MSG_W_GRADLETH_RADIUS_EXCEED = 0x00040071; ///< 表最大半径超过显式最大半径
   constexpr uint32_t MSG_W_WEAPENG_UNDEF_EFFECT  = 0x00040072; ///< 未定义的武器效果
   constexpr uint32_t MSG_W_WEAPENG_INIT_EFFECT_FAIL = 0x00040073; ///< 无法初始化武器效果
   constexpr uint32_t MSG_W_RPT_RAW_TRACK_MERGE     = 0x00040074; ///< 原始轨迹合并失败
   constexpr uint32_t MSG_W_EXPL_LAUNCHER_NO_CHAIN = 0x00040075; ///< 发射时找不到 LAUNCHER_COMMANDER 命令链
   constexpr uint32_t MSG_W_EXPL_ADD_CHAIN_FAIL     = 0x00040076; ///< 无法向发射武器添加命令链
   constexpr uint32_t MSG_W_MOBILITY_DEFAULT_TABLE = 0x00040077; ///< 自动插入默认概率表
   constexpr uint32_t MSG_W_HEL_CMD_INCOMPAT       = 0x00040078; ///< 命令与 WSF_HEL_LETHALITY 不兼容（参数化）
   constexpr uint32_t MSG_W_HEL_NO_DAMAGE_RADIUS   = 0x00040079; ///< 目标未指定 damage_radius
   constexpr uint32_t MSG_W_WFX_INCDMG_DEP           = 0x0004007A; ///< 附带损伤允许命令已废弃（参数化，2 处共用）
   constexpr uint32_t MSG_W_BMLC_RANGE_LIMITS       = 0x0004007B; ///< 目标距离超出射程限制
   constexpr uint32_t MSG_W_BMLC_NO_LAUNCH_WINDOW   = 0x0004007C; ///< 无法确定发射窗口结束
   constexpr uint32_t MSG_W_BMLC_TABLE_ALT_TRANS     = 0x0004007D; ///< 发射表高度由降转升异常
   // 二次开发（i18n）：ew 域 EA/EP 同义但文案有差异的变体（忠实原文分别登记）
   constexpr uint32_t MSG_W_EW_SEL_APPLY_AN         = 0x0004007E; ///< 选择技术失败：系统类型不兼容（有 an 变体）
   constexpr uint32_t MSG_W_EW_DESEL_NO_VALID_ST    = 0x0004007F; ///< 取消技术失败：索引处无有效技术（长版）

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

   // ---- 通信段（0x0005xxxx）DIS 协议库（第十六批）----
   constexpr uint32_t MSG_DIS_ENV_BAD_VERSION      = 0x00050016; ///< DIS_VERSION 环境变量值非法
   constexpr uint32_t MSG_DIS_ENV_BAD_EXERCISE     = 0x00050017; ///< DIS_EXERCISE 环境变量值非法
   constexpr uint32_t MSG_DIS_ENV_BAD_SITE         = 0x00050018; ///< DIS_SITE 环境变量值非法
   constexpr uint32_t MSG_DIS_ENV_BAD_APPLICATION  = 0x00050019; ///< DIS_APPLICATION 环境变量值非法
   constexpr uint32_t MSG_DIS_ENV_BAD_HBT          = 0x0005001A; ///< DIS_HRT_BEAT_TIMER 环境变量值非法
   constexpr uint32_t MSG_DIS_ENV_BAD_HBT_MPLIER   = 0x0005001B; ///< DIS_HRT_BEAT_MPLIER 环境变量值非法
   constexpr uint32_t MSG_DIS_PDU_TYPE_READ_FAIL   = 0x0005001C; ///< PDU 类型读取失败
   constexpr uint32_t MSG_DIS_PDU_SIZE_MISMATCH    = 0x0005001D; ///< PDU 数据尺寸不匹配
   constexpr uint32_t MSG_DIS_SIG_NO_32BIT_BOUNDARY = 0x0005001E; ///< 信号 PDU 长度非 32 位边界
   constexpr uint32_t MSG_DIS_SIG_DB_LEN_NOT_96    = 0x0005001F; ///< 数据库索引编码类长度须为 96
   constexpr uint32_t MSG_DIS_SIG_LEN_BITS_MISMATCH = 0x00050020; ///< 信号 PDU 数据长度与头部位数不符

   // ---- 通信段（0x0005xxxx）DIS 协议库（第十七批：流 IO 防御与 PDU 诊断链）----
   constexpr uint32_t MSG_DISIO_RADIOID_READ_FAIL     = 0x00050021; ///< DisRadioId 流提取失败
   constexpr uint32_t MSG_DISIO_RADIOID_READ_NOT_GOOD  = 0x00050022; ///< DisRadioId 流提取前流状态异常
   constexpr uint32_t MSG_DIS_RADIOID_PACK_FAIL        = 0x00050023; ///< DisRadioId Pack 流写入失败
   constexpr uint32_t MSG_DIS_RADIOID_PACK_NOT_GOOD     = 0x00050024; ///< DisRadioId Pack 输入流状态异常
   constexpr uint32_t MSG_DIS_RADIOID_UNPACK_FAIL      = 0x00050025; ///< DisRadioId UnPack 流读取失败
   constexpr uint32_t MSG_DIS_RADIOID_UNPACK_NOT_GOOD  = 0x00050026; ///< DisRadioId UnPack 输入流状态异常
   constexpr uint32_t MSG_DIS_SIG_COPY_BAD_PTR         = 0x00050027; ///< 信号 PDU 拷贝收到无效数据指针
   constexpr uint32_t MSG_DIS_FACTORY_LAST_GOOD_HEADER = 0x00050028; ///< 上一个良好的 PDU 头（诊断转储引出句）
   constexpr uint32_t MSG_DIS_FACTORY_FIRST_PDU       = 0x00050029; ///< 接收缓冲区首个 PDU 即出错

   // ---- 通信段（0x0005xxxx）wsf dis 接口/event_pipe/xio（第十八批）----
   constexpr uint32_t MSG_DIS_ACTION_RESP_NOT_JOIN      = 0x0005002A; ///< DIS 动作响应非加入演习请求
   constexpr uint32_t MSG_DIS_ACTION_RESPR_NOT_JOIN     = 0x0005002B; ///< DIS 可靠动作响应非加入演习请求
   constexpr uint32_t MSG_DIS_IFACE_OPEN_DEVICE_FAIL    = 0x0005002C; ///< DIS 接口设备打开失败（两处共用）
   constexpr uint32_t MSG_DIS_IFACE_SET_APPEAR_VAR_FAIL = 0x0005002D; ///< 无法设置外观脚本变量
   constexpr uint32_t MSG_DIS_IFACE_EXEC_APPEAR_FAIL    = 0x0005002E; ///< 无法执行外观更新脚本
   constexpr uint32_t MSG_DIS_PLAYBACK_OPEN_FAIL        = 0x0005002F; ///< DIS 回放文件打开失败
   constexpr uint32_t MSG_DIS_PLAYBACK_TOO_MANY         = 0x00050030; ///< DIS 回放文件指定过多
   constexpr uint32_t MSG_EVENTPIPE_OPEN_FAIL           = 0x00050031; ///< event_pipe 输出文件打开失败
   constexpr uint32_t MSG_XIO_NO_LINKED_CONN            = 0x00050032; ///< XIO 无可用关联连接
   constexpr uint32_t MSG_XIO_MSG_NO_TRANSLATOR         = 0x00050033; ///< XIO 消息无翻译器
   constexpr uint32_t MSG_XIO_AUTO_DIS_MAP_INIT_FAIL    = 0x00050034; ///< XIO 自动 DIS 映射初始化失败
   constexpr uint32_t MSG_XIO_CANNOT_TX_MSG             = 0x00050035; ///< 无法通过 XIO 发送 WsfMessage

   // ---- 通信段（0x0005xxxx）comm warning 级（第二十批，按真实侦察重排）----
   constexpr uint32_t MSG_W_COMM_INVALID_GATEWAY     = 0x00050036; ///< comm 网关无效
   constexpr uint32_t MSG_W_COMM_CANNOT_CHANGE_GATEWAY = 0x00050037; ///< 无法更改网关地址
   constexpr uint32_t MSG_W_COMMADDR_NON_NUMERIC_FIELD = 0x00050038; ///< 地址字段非数字
   constexpr uint32_t MSG_W_COMMADDR_BAD_FIELD_VALUE  = 0x00050039; ///< 地址字段值超范围
   constexpr uint32_t MSG_W_MEDIUMCMD_WRONG_SCOPE     = 0x0005003A; ///< 此作用域不能设置介质命令
   constexpr uint32_t MSG_W_MEDIUM_DELIVERY_ABORT     = 0x0005003B; ///< 中止条件致消息投递失败
   constexpr uint32_t MSG_W_MEDIUM_REPLACE_BY_ADD     = 0x0005003C; ///< 添加介质将替换已有介质
   constexpr uint32_t MSG_W_MEDIUM_SHARED_EDITS       = 0x0005003D; ///< 介质共享编辑全局生效
   constexpr uint32_t MSG_W_COMMNET_ADDR_NOT_FOUND     = 0x0005003E; ///< 网络成员地址未找到
   constexpr uint32_t MSG_W_COMMNET_UNASSIGNED_ADDR    = 0x0005003F; ///< 网络成员地址未分配被跳过
   constexpr uint32_t MSG_W_COMMNET_NAMED_LINK_MISSING = 0x00050040; ///< 命名链路成员未找到
   constexpr uint32_t MSG_W_COMMNET_ADDR_LINK_MISSING  = 0x00050041; ///< 地址链路成员未找到
   constexpr uint32_t MSG_W_NETMGR_CONFLICTING_NET     = 0x00050042; ///< comm 网络指派冲突
   constexpr uint32_t MSG_W_NETMGR_ADDCONN_FAIL        = 0x00050043; ///< 添加连接失败（4 处共用）
   constexpr uint32_t MSG_W_NETMGR_NULL_COMM           = 0x00050044; ///< 空 comm 引用无法管理
   constexpr uint32_t MSG_W_NETMGR_ALREADY_MANAGED     = 0x00050045; ///< comm 已被网络管理器管理
   constexpr uint32_t MSG_W_NETMGR_REJECT_ADD          = 0x00050046; ///< comm 入指定网络被拒
   constexpr uint32_t MSG_W_NETSTAR_NO_HUB              = 0x00050047; ///< 星型网络无中心节点
   constexpr uint32_t MSG_W_IGMP_ONLY_ONE               = 0x00050048; ///< 每 comm 仅允许一个 IGMP
   constexpr uint32_t MSG_W_OSPF_NEED_L2_MULTICAST     = 0x00050049; ///< OSPF 需二级多播一致性（2 处共用）
   constexpr uint32_t MSG_W_OSPF_NO_MULTICAST_PROTO     = 0x0005004A; ///< 路由器无多播协议
   constexpr uint32_t MSG_W_OSPF_NO_DESIGNATED_RTR      = 0x0005004B; ///< OSPF 无法选出指定路由器
   constexpr uint32_t MSG_W_ROUTER_NO_NETMGR            = 0x0005004C; ///< 路由器缺少网络管理器
   constexpr uint32_t MSG_W_ROUTER_BAD_GATEWAY          = 0x0005004D; ///< 平台网关设置无效
   constexpr uint32_t MSG_W_ROUTER_CMD_NOT_SUPPORTED    = 0x0005004E; ///< 路由器对象不支持该命令
   constexpr uint32_t MSG_W_ROUTER_LOST_GATEWAY         = 0x0005004F; ///< 失去连通性路由器丢失网关
   constexpr uint32_t MSG_W_ROUTER_CANNOT_CHANGE_GW     = 0x00050050; ///< 路由器无法更改网关
   constexpr uint32_t MSG_W_ROUTER_TTL_EXCEEDED         = 0x00050051; ///< 消息超 TTL 被丢弃
   constexpr uint32_t MSG_W_EVENTPIPE_BAD_EVENT_TYPE     = 0x00050052; ///< event_pipe 事件类型无效
   constexpr uint32_t MSG_W_EVENTPIPE_RECORD_DEP        = 0x00050053; ///< Record 带 sim_time 已废弃
   constexpr uint32_t MSG_W_XIO_PDU_BAD_VERSION         = 0x00050054; ///< XIO 收到版本无效的 PDU
   constexpr uint32_t MSG_W_XIOSIM_DROP_TRACK_NO_TARGET  = 0x00050055; ///< 弃轨消息目标索引不存在
   constexpr uint32_t MSG_W_DISIN_NONSTD_DEPRECATED     = 0x00050056; ///< 非标准 DIS 命令已废弃（6 处共用）
   constexpr uint32_t MSG_W_DISIN_AGEOUT_DEPRECATED     = 0x00050057; ///< age_out_time 已废弃
   constexpr uint32_t MSG_W_DISIF_SHADOW_ADD_FAIL       = 0x00050058; ///< 影子平台添加失败（2 处共用）
   constexpr uint32_t MSG_W_DISIF_RANGE_FILTER_NO_PLATFORM = 0x00050059; ///< 距离过滤平台不存在
   constexpr uint32_t MSG_W_DISIF_NULL_ENTITY_ID        = 0x0005005A; ///< 实体状态 PDU 无实体 ID
   constexpr uint32_t MSG_W_DATAR_UNKNOWN_REPLY         = 0x0005005B; ///< DataR 收到未知数据回复
   constexpr uint32_t MSG_W_DATAR_UNPARSED_VARIABLE     = 0x0005005C; ///< DataR 收到未解析变量
   constexpr uint32_t MSG_W_DATAR_CANT_INTERPRET        = 0x0005005D; ///< DataR 无法解释全部返回值
   constexpr uint32_t MSG_W_DATAR_UNKNOWN_INITIATOR     = 0x0005005E; ///< DataR 不识别发起 PDU 类型
   constexpr uint32_t MSG_W_DATAR_TIMEADV_REPORT        = 0x0005005F; ///< DataR 未处理时间推进上报
   constexpr uint32_t MSG_W_TIMEDOUT_UNHANDLED          = 0x00050060; ///< TimedOut 虚函数未重载（10 处共用）
   constexpr uint32_t MSG_W_ARR_RESIGN_NO_RESPONSE      = 0x00050061; ///< 退出演习请求未获充分响应
   constexpr uint32_t MSG_W_ARR_REQ_NOT_INTERPRETED     = 0x00050062; ///< 动作请求无法被 WSF 解释
   constexpr uint32_t MSG_W_ARR_TIMEADV_IGNORED         = 0x00050063; ///< 忽略时间推进请求
   constexpr uint32_t MSG_W_ARR_UNKNOWN_FIXED_DATUM     = 0x00050064; ///< 遇到未知固定数据项
   constexpr uint32_t MSG_W_SETDATAR_CANT_FULLY_RESPOND  = 0x00050065; ///< SetDataR 无法完整响应查询
   constexpr uint32_t MSG_W_SETDATAR_BAD_FIXED_DATUM    = 0x00050066; ///< SetDataR 固定数据项不支持
   constexpr uint32_t MSG_W_SETDATAR_BAD_VARIABLE_DATUM = 0x00050067; ///< SetDataR 可变数据项不支持
   constexpr uint32_t MSG_W_DATARQ_NULL_QUERY            = 0x00050068; ///< DataQueryR 收到空查询
   constexpr uint32_t MSG_W_DATARQ_CANT_RESPOND_ALL      = 0x00050069; ///< DataQueryR 无法响应全部查询
   constexpr uint32_t MSG_W_SETR_POPULATE_NOT_IMPL       = 0x0005006A; ///< SetRecordR 未实现 PopulateReply
   constexpr uint32_t MSG_W_SETR_NOT_PROCESSING         = 0x0005006B; ///< SetRecordR 尚不处理数据
   constexpr uint32_t MSG_W_CER_CANT_COMPLY              = 0x0005006C; ///< CreateEntityR 无法执行请求
   constexpr uint32_t MSG_W_RER_CANT_COMPLY              = 0x0005006D; ///< RemoveEntityR 无法执行请求
   constexpr uint32_t MSG_W_EXCH_NO_TYPE_MAPPING         = 0x0005006E; ///< Exchange 无 entity_type 映射
   constexpr uint32_t MSG_W_EXCH_SUPPLIED_UNKNOWN         = 0x0005006F; ///< sEVENT_SUPPLIED 发往未知参与方
   constexpr uint32_t MSG_W_RQR_NO_DATA_RECORDS           = 0x00050070; ///< RecordQueryR 无数据记录响应
   constexpr uint32_t MSG_W_AR_NO_JOIN_ACTION             = 0x00050071; ///< 不对加入演习请求采取行动
   constexpr uint32_t MSG_W_DQ_NOT_ENABLED                = 0x00050072; ///< 未启用响应传入查询
   constexpr uint32_t MSG_W_ENTITYSTATE_SUSPICIOUS         = 0x00050073; ///< 实体状态 PDU 数据可疑
   constexpr uint32_t MSG_W_DISPLAT_MAX_BEAMS              = 0x00050074; ///< 超过最大波束数量
   constexpr uint32_t MSG_W_DISIF_SHADOW_ADD_FAIL2         = 0x00050075; ///< 影子平台添加失败（原文无句点变体）
   // 二次开发（i18n）：第二十批子批 C wsf_mil warning 级消息 ID（0x0005 段续排，dis 4 + comm 4）
   constexpr uint32_t MSG_W_DISWEAP_NO_ENT_TYPE           = 0x00050076; ///< DIS 武器事件未指定实体类型
   constexpr uint32_t MSG_W_DEFIRE_BAD_RECORD            = 0x00050077; ///< 定向能射击 PDU 记录类型不支持
   constexpr uint32_t MSG_W_DEFIRE_NO_ENGAGE             = 0x00050078; ///< 定向能射击无交战对象
   constexpr uint32_t MSG_W_DISFIRE_INIT_FAIL            = 0x00050079; ///< DIS 射击交战对象初始化失败
   constexpr uint32_t MSG_W_LASER_PRF_OVERRIDE           = 0x0005007A; ///< 激光发射机 PRF 由调制类型等输入决定
   constexpr uint32_t MSG_W_JTIDS_ADD_TWICE              = 0x0005007B; ///< JTIDS 槽位组重复加入网络
   constexpr uint32_t MSG_W_JTIDS_MERGED                 = 0x0005007C; ///< JTIDS 槽位组已合并
   constexpr uint32_t MSG_W_HWSUB_NO_POWER               = 0x0005007D; ///< 潜艇通信发射机未指定功率

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

   /// 警告级流式版本（第二十批新增）：格式化 + "[W:ID]" 前缀 —— 供 warning 日志
   /// 保留 AddNote() 流式结构时使用：
   ///   auto out = ut::log::warning() << ut::msg::FormatWEnTagged(id, args);
   ///   out.AddNote() << "Name: " << ...;
   /// 落盘日志为 "[W:XXXXXXXX] 英文原文"，与 EmitWarning 输出格式一致
   UT_EXPORT std::string FormatWEnTagged(uint32_t aId, const Args& aArgs = {});

   /// 中文通道格式化（界面显示专用快捷方式）
   UT_EXPORT std::string FormatZh(uint32_t aId, const Args& aArgs);

   /// 日志双通道 —— 错误级：以 "[E:XXXXXXXX] 英文原文" 形式写入 ut::log
   /// （落盘永远英文+ID，界面中文由消费端另行 Format）
   UT_EXPORT void Emit(uint32_t aId, const Args& aArgs);

   /// 日志双通道 —— 警告级：以 "[W:XXXXXXXX] 英文原文" 形式写入 ut::log
   UT_EXPORT void EmitWarning(uint32_t aId, const Args& aArgs = {});

   /// 获取条目的中文原因说明（无则返回空串）
   UT_EXPORT std::string GetCauseZh(uint32_t aId);

   /// 获取条目的中文修复建议（无则返回空串）
   UT_EXPORT std::string GetFixZh(uint32_t aId);

   /// L1 消费端翻译：输入带 "[E:ID] "/"[W:ID] " 前缀的日志文本行，
   /// 按 ID 查目录，以英文模板从文本中反提取参数值，再填充中文模板返回。
   /// 任何失败（无前缀 / ID 未注册 / 文本与模板不匹配）返回空串，调用方回退显示原文。
   /// 仅供界面显示通道使用；日志落盘/控制台输出保持原文英文+ID 不变。
   UT_EXPORT std::string TranslateTaggedToZh(const std::string& aText);
} // namespace msg
} // namespace ut

#endif
