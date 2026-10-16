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

#include "UtMsgCatalog.hpp"

// 二次开发（i18n）：TranslateTaggedToZh 的模板切分使用 vector
#include <vector>

#include <cstdio>

#include "UtLog.hpp"

namespace
{
// ============================================================================
// 二次开发（i18n）：内置消息目录表（编译期）。
// 维护规则：
//   1. 表按 ID 升序排列 —— Lookup 依赖升序做二分查找，插入新条目后保持有序；
//   2. 占位符统一为 {参数名}，中英文模板的参数名必须一致；
//   3. 新增消息先在 UtMsgCatalog.hpp 分配 ID 常量，再在此登记条目；
//   4. 已发布的 ID/文案只修错别字级别的问题，不做语义改写（保护外部对照表）。
// ============================================================================
const ut::msg::Entry cCatalog[] = {
   // ---- 想定解析/启动段（0x0001xxxx）----
   // 仿真对象创建失败（WsfStandardApplication::CreateSimulation 捕获 WsfException 时输出）
   {ut::msg::MSG_SIM_CREATE_FAILED,
    "Creation of simulation failed.",
    "仿真对象创建失败。",
    "仿真构造函数（构造平台列表、事件管道等）抛出了 WsfException",
    "查看紧随其后的异常详情 Note；检查想定中平台/事件相关配置"},
   // event_pipe schema 中 message_id_or 值被复用（用于构造消息命名空间，必须唯一）
   {ut::msg::MSG_EVENTPIPE_OR_DUP,
    "WsfEventPipe error: message_id_or value {value}, is reused.",
    "WsfEventPipe 错误: message_id_or 值 {value} 被重复使用。",
    "event_pipe schema 用 message_id_or 生成消息命名空间，相同值会导致消息 ID 冲突",
    "修改 event_pipe 设置文件中的 message_id_or，保证各 schema 间取值唯一"},
   // event_pipe schema 中 message_id 值被复用（经命名空间位运算后的最终 ID 冲突）
   {ut::msg::MSG_EVENTPIPE_MSGID_DUP,
    "WsfEventPipe error: message_id value {value} is reused.",
    "WsfEventPipe 错误: message_id 值 {value} 被重复使用。",
    "同一 message_id_or 命名空间下出现了重复的消息编号",
    "检查 Notes 中的 message_id_or 与 message id，为冲突消息分配新编号"},

   // ---- 想定解析/脚本段（0x0001xxxx）script 域（第十九批）----
   // 定时回调方法执行失败
   {ut::msg::MSG_SCRIPT_CB_EXEC_FAIL,
    "Cannot execute callback method.",
    "无法执行回调方法。",
    "脚本回调方法执行出错（脚本异常或脚本不存在）",
    "查看 Notes 中的回调与平台信息，检查回调脚本定义"},
   // 脚本解析失败
   {ut::msg::MSG_SCRIPT_CTX_PARSE_FAIL,
    "Failed to parse script.",
    "脚本解析失败。",
    "脚本源码存在语法错误",
    "查看 Note 中 UtInput 的异常信息，修正脚本语法"},
   // WsfScriptContext::Initialize 调用约定不满足
   {ut::msg::MSG_SCRIPT_CTX_INVALID_INIT,
    "Invalid call to WsfScriptContext::Initialize().",
    "WsfScriptContext::Initialize() 调用无效。",
    "调用时平台指针为空，或定义了部件类名但部件指针为空",
    "检查脚本上下文初始化调用链，保证平台/部件指针有效"},
   // 脚本上下文初始化失败
   {ut::msg::MSG_SCRIPT_CTX_INIT_FAIL,
    "Failed to initialize the script context.",
    "脚本上下文初始化失败。",
    "底层 UtScriptContext::Initialize 返回失败",
    "检查脚本环境配置"},
   // 脚本签名与期望原型不符
   {ut::msg::MSG_SCRIPT_CTX_BAD_SIGNATURE,
    "Unexpected script signature.",
    "脚本签名不符。",
    "脚本形参/返回类型与调用方期望的原型不一致",
    "对照 Notes 中的期望与实际签名，修正脚本函数定义"},
   // AddTimedEvents 内部错误
   {ut::msg::MSG_SCRIPT_CTX_TIMED_EVENTS_ERR,
    "Internal error in WsfScriptContext::AddTimedEvents.",
    "WsfScriptContext::AddTimedEvents 内部错误。",
    "定时事件回调的重复方式既非固定间隔也非随机间隔",
    "联系开发方排查内部事件类型配置"},
   // 脚本错误输出头（后附脚本名与行号）
   {ut::msg::MSG_SCRIPT_ERROR_HEADER,
    "Error in script.",
    "脚本出错。",
    "脚本执行/校验过程检测到错误，详细信息见 Notes",
    "按 Notes 中的脚本名与行号定位问题脚本"},
   // 脚本处理中抛出 C++ 异常
   {ut::msg::MSG_SCRIPT_OBJ_CLASS_EXCEPTION,
    "Exception thrown while processing script.",
    "处理脚本时抛出异常。",
    "脚本对象构造/方法执行过程中抛出标准异常",
    "查看 Notes 中异常类型与信息，检查脚本输入"},
   // 回调名未知
   {ut::msg::MSG_SCRIPT_OBS_UNKNOWN_CB,
    "Unknown callback name.",
    "未知回调名。",
    "回调启用/禁用命令引用的事件回调不存在",
    "核对 Note 中回调名拼写，确认对应回调命令已定义"},
   // 回调找不到脚本
   {ut::msg::MSG_SCRIPT_OBS_NO_SCRIPT,
    "Unable to find script for callback.",
    "无法找到回调对应的脚本。",
    "回调命令指定的脚本名在脚本上下文中不存在",
    "核对 Note 中脚本名，确认脚本已在上下文中定义"},
   // 消息处理器无法解析脚本名
   {ut::msg::MSG_SCRIPT_MSGH_NO_SCRIPT,
    "WsfScriptMessageHandler unable to resolve script name.",
    "WsfScriptMessageHandler 无法解析脚本名。",
    "消息处理器指定的脚本名在脚本上下文中找不到",
    "核对 Note 中脚本名，确认脚本已定义且无拼写错误"},
   // 脚本方法找不到 comm 设备
   {ut::msg::MSG_SCRIPT_PLAT_NO_COMM,
    "Could not find comm.",
    "找不到 comm 设备。",
    "TurnPartOn/Off 类脚本方法指定的通信设备在平台上不存在",
    "核对 Note 中设备名与平台组件列表"},
   // 脚本方法找不到 processor
   {ut::msg::MSG_SCRIPT_PLAT_NO_PROC,
    "Could not find processor.",
    "找不到 processor。",
    "TurnPartOn/Off 类脚本方法指定的处理器在平台上不存在",
    "核对 Note 中处理器名与平台组件列表"},
   // 脚本方法找不到 mover
   {ut::msg::MSG_SCRIPT_PLAT_NO_MOVER,
    "Could not find mover.",
    "找不到 mover。",
    "TurnPartOn/Off 类脚本方法指定的运动体在平台上不存在",
    "核对 Note 中运动体名与平台组件列表"},
   // 脚本方法找不到 sensor
   {ut::msg::MSG_SCRIPT_PLAT_NO_SENSOR,
    "Could not find sensor.",
    "找不到 sensor。",
    "TurnPartOn/Off 类脚本方法指定的传感器在平台上不存在",
    "核对 Note 中传感器名与平台组件列表"},
   // 脚本方法找不到 router
   {ut::msg::MSG_SCRIPT_PLAT_NO_ROUTER,
    "Could not find router.",
    "找不到 router。",
    "TurnPartOn/Off 类脚本方法指定的路由器在平台上不存在",
    "核对 Note 中路由器名与平台组件列表"},
   // 脚本执行失败
   {ut::msg::MSG_SCRIPT_EXEC_FAIL,
    "Could not execute script.",
    "无法执行脚本。",
    "ExecuteScript 返回失败（脚本不存在或执行出错）",
    "核对 Note 中脚本名，检查脚本定义与执行错误信息"},
   // 航路点索引越界
   {ut::msg::MSG_SCRIPT_ROUTE_IDX_RANGE,
    "Index out of range.",
    "索引越界。",
    "WsfRoute.Insert/Remove 指定的航路点索引超出当前航路长度",
    "对照 Notes 中索引与长度，修正脚本中的索引值"},
   // WsfScriptStateMachineProcessor::Initialize 调用无效
   {ut::msg::MSG_SCRIPT_SMP_INVALID_INIT,
    "Invalid call to WsfScriptStateMachineProcessor::Initialize().",
    "WsfScriptStateMachineProcessor::Initialize() 调用无效。",
    "初始化时状态机脚本变量指针为空",
    "检查状态机处理器的脚本上下文初始化流程"},
   // 状态机入口状态索引无效
   {ut::msg::MSG_SCRIPT_SM_BAD_ENTRY_IDX,
    "WsfScriptStateMachine: Invalid entry state index.",
    "WsfScriptStateMachine: 入口状态索引无效。",
    "当前状态索引超出状态列表范围",
    "对照 Notes 中索引与上限，检查状态机状态配置"},
   // 状态机转移状态索引无效
   {ut::msg::MSG_SCRIPT_SM_BAD_TRANS_IDX,
    "WsfScriptStateMachine: Invalid transition state index.",
    "WsfScriptStateMachine: 转移状态索引无效。",
    "next_state 解析出的状态索引超出状态列表范围",
    "对照 Notes 中索引与上限，检查 next_state 定义"},
   // 状态机无任何状态
   {ut::msg::MSG_SCRIPT_SM_EMPTY,
    "WsfScriptStateMachine: Empty state machine.",
    "WsfScriptStateMachine: 空状态机。",
    "状态机未定义任何状态",
    "在状态机定义中至少添加一个 state"},
   // 状态机状态初始化失败
   {ut::msg::MSG_SCRIPT_SM_STATE_INIT_FAIL,
    "WsfScriptStateMachine: Unable to initialize states.",
    "WsfScriptStateMachine: 无法初始化状态。",
    "某个状态的 Initialize 返回失败，详情见 Notes",
    "按 Notes 中的状态名定位初始化失败的状态"},
   // next_state 转移初始化失败
   {ut::msg::MSG_SCRIPT_SM_NEXT_STATE_INIT,
    "WsfScriptStateMachine: Unable to initialize 'next_state'.",
    "WsfScriptStateMachine: 无法初始化 'next_state'。",
    "状态转移对象初始化失败",
    "检查 Notes 中当前状态与目标状态的转移定义"},
   // next_state 不存在
   {ut::msg::MSG_SCRIPT_SM_NO_NEXT_STATE,
    "WsfScriptStateMachine: 'next_state' does not exist.",
    "WsfScriptStateMachine: 'next_state' 不存在。",
    "转移定义引用的 next_state 未在状态机中定义",
    "核对 Notes 中目标状态名，补充对应 state 定义"},
   // 状态机找不到脚本
   {ut::msg::MSG_SCRIPT_SM_NO_SCRIPT,
    "Unable to find script.",
    "无法找到脚本。",
    "状态机的 entry/exit 脚本名在上下文中找不到",
    "核对 Note 中脚本名，确认脚本已定义"},

   // ---- 平台/运动段（0x0002xxxx）----
   // 添加平台时传入空指针（调用方构造失败后未判空）
   {ut::msg::MSG_PLATFORM_NULL_PTR,
    "WsfSimulation: Attempting to add a null platform pointer.",
    "WsfSimulation: 试图添加空平台指针。",
    "调用方在平台对象构造失败后仍调用了 AddPlatform",
    "检查平台创建代码的错误分支，构造失败时应终止添加流程"},
   // 同一平台对象被添加两次（指针相同）
   {ut::msg::MSG_PLATFORM_ADDED_TWICE,
    "WsfSimulation: Attempting to add the same platform twice.",
    "WsfSimulation: 试图重复添加同一平台：名称 {name}，索引 {index}。",
    "同一平台对象指针被多次加入仿真（如脚本重复调用 AddPlatform）",
    "检查脚本/事件中是否有对同一平台的重复添加逻辑"},
   // 平台缺少名称
   {ut::msg::MSG_PLATFORM_NO_NAME,
    "WsfSimulation: Trying to add a platform that does not have a name.",
    "WsfSimulation: 试图添加未命名的平台。",
    "平台对象未调用 SetName 或名称为空",
    "为平台指定名称后再添加（可让仿真自动分配默认名）"},
   // 与现有平台同名
   {ut::msg::MSG_PLATFORM_DUP_NAME,
    "WsfSimulation: Attempting to add platform with the same name as an existing platform.",
    "WsfSimulation: 试图添加与现有平台同名的平台：名称 {name}。",
    "平台名在仿真中必须唯一，重名会导致按名检索歧义",
    "修改想定或脚本中的平台名，保证唯一性"},
   // 在仿真 Initialize() 之前用仿真时间添加平台
   {ut::msg::MSG_PLATFORM_BEFORE_INIT,
    "WsfSimulation: Attempting to add a platform with a sim-time before Initialize() was called.",
    "WsfSimulation: 试图在调用 Initialize() 之前添加带仿真时间的平台。",
    "仿真尚未初始化完成，此时添加的带时间平台会破坏事件时序",
    "将平台添加操作移到仿真 Start/Initialize 完成之后"},
   // 平台组件预初始化失败
   {ut::msg::MSG_COMPONENT_PREINIT_FAIL,
    "Platform component failed pre-initialization.",
    "平台组件预初始化失败：平台 {platform}，组件 {component}。",
    "组件 PreInitialize 返回 false（配置缺失或依赖不满足）",
    "根据消息中的平台名/组件名定位，检查该组件的想定配置"},
   // 平台组件第一阶段初始化失败
   {ut::msg::MSG_COMPONENT_INIT1_FAIL,
    "Platform component failed phase one initialization.",
    "平台组件第一阶段初始化失败：平台 {platform}，组件 {component}。",
    "组件 Initialize 返回 false（依赖组件未就绪或参数非法）",
    "根据消息中的平台名/组件名定位，检查依赖关系与参数取值"},
   // 平台组件第二阶段初始化失败
   {ut::msg::MSG_COMPONENT_INIT2_FAIL,
    "Platform component failed phase two initialization.",
    "平台组件第二阶段初始化失败：平台 {platform}，组件 {component}。",
    "组件 Initialize2 返回 false（跨组件交互校验失败等）",
    "根据消息中的平台名/组件名定位，检查组件间交互配置"},
   // 回调组件不存在
   {ut::msg::MSG_CALLBACK_NOT_FOUND,
    "Platform unable to invoke callback.",
    "平台无法调用指定的回调：时刻 {t}，平台 {platform}，回调 {callback}。",
    "按名称查找回调组件返回空（未定义或未挂到该平台）",
    "根据消息中的回调名检查平台定义中是否包含该 callback 组件"},
   // 任务管理器 track processor 类型错误（WsfTaskManager::Initialize）
   {ut::msg::MSG_PLAT_TASK_BAD_TRACKPROC,
    "Track processor must be of type WSF_TRACK_PROCESSOR.",
    "任务管理器引用的航迹处理器必须是 WSF_TRACK_PROCESSOR 类型（详见附注 Processor 名）。",
    "track_processor 命令指定的处理器不是 WSF_TRACK_PROCESSOR 类型",
    "将 track_processor 指向的处理器改为 WSF_TRACK_PROCESSOR 类型，或去掉该引用"},
   // 任务管理器找不到 track processor
   {ut::msg::MSG_PLAT_TASK_NO_TRACKPROC,
    "Track processor could not be found.",
    "任务管理器找不到指定的航迹处理器（详见附注 Processor 名）。",
    "track_processor 命令指定的处理器名在平台上不存在",
    "核对平台定义中的处理器名与 track_processor 取值是否一致"},
   // AcquireResource 资源类型未知
   {ut::msg::MSG_PLAT_TASK_UNKNOWN_RES_ACQ,
    "Unknown resource type in AcquireResource.",
    "AcquireResource 遇到未知资源类型。",
    "脚本或任务调用了未注册的资源类型",
    "检查任务脚本中资源类型拼写，确认使用框架支持的资源类型"},
   // ReleaseResource 资源类型未知
   {ut::msg::MSG_PLAT_TASK_UNKNOWN_RES_REL,
    "Unknown resource type in ReleaseResource.",
    "ReleaseResource 遇到未知资源类型。",
    "脚本或任务调用了未注册的资源类型",
    "检查任务脚本中资源类型拼写，确认使用框架支持的资源类型"},
   // 地址找不到 comm 设备
   {ut::msg::MSG_PLAT_TASK_NO_COMM_FOR_ADDR,
    "Unable to find comm device for address.",
    "按地址查找通信设备失败。",
    "目标地址在仿真中无对应 comm 设备（地址未分配或平台未部署）",
    "确认目标平台已部署且其 comm 地址配置正确"},
   // 发消息时 comm 设备不存在
   {ut::msg::MSG_PLAT_TASK_SEND_NO_COMM,
    "Unable to send message. Comm device does not exist.",
    "无法发送消息：通信设备不存在。",
    "发送方 comm 组件已被删除或未初始化",
    "检查发送方平台的 comm 组件定义与生命周期"},
   // 发消息时接收者已不存在
   {ut::msg::MSG_PLAT_TASK_SEND_NO_RCPT,
    "Unable to send message. Recipient no longer exists.",
    "无法发送消息：接收方已不存在。",
    "消息排队期间接收方平台被删除",
    "确认接收方平台的存活区间覆盖消息发送时刻"},
   // 发消息时目标处理器不存在
   {ut::msg::MSG_PLAT_TASK_SEND_NO_TGT_PROC,
    "Unable to send message. Target processor does not exist.",
    "无法发送消息：目标处理器不存在。",
    "消息指向的处理器在接收平台上不存在",
    "核对消息目的处理器名与接收平台定义中的处理器名"},
   // 任务分配内部错误
   {ut::msg::MSG_PLAT_TASK_ASSIGN_INTERNAL,
    "Task assignment internal error 1.",
    "任务分配过程出现内部错误 1。",
    "框架内部状态不一致（任务与分配表失配）",
    "属框架内部缺陷，请记录想定与复现步骤反馈开发方"},
   // 取消任务时任务不存在
   {ut::msg::MSG_PLAT_TASK_CANCEL_NOT_FOUND,
    "Cannot find received task to cancel.",
    "取消任务时找不到对应的已接收任务。",
    "取消请求到达时任务已完成或从未被该平台接收",
    "检查任务取消的时序，确认任务在被取消时仍处于接收状态"},
   // aero 展弦比为零
   {ut::msg::MSG_PLAT_AERO_ZERO_ASPECT,
    "'aero' properties must specify a non-zero 'aspect_ratio'.",
    "aero 定义必须指定非零的 aspect_ratio（展弦比）。",
    "未提供展弦比或取值为 0",
    "在 aero 块中补充非零的 aspect_ratio 值"},
   // aero 奥斯瓦尔德因子为零
   {ut::msg::MSG_PLAT_AERO_ZERO_OSWALD,
    "'aero' must specify a non-zero value for 'oswalds_factor'.",
    "aero 定义必须为 oswalds_factor（奥斯瓦尔德效率因子）指定非零值。",
    "未提供奥斯瓦尔德因子或取值为 0",
    "在 aero 块中补充非零的 oswalds_factor 值（通常在 0.7~0.9 之间）"},
   // aero 缺 zero_lift_cd
   {ut::msg::MSG_PLAT_AERO_NO_ZERO_LIFT_CD,
    "'aero' 'zero_lift_cd' was not provided.",
    "aero 定义缺少 zero_lift_cd（零升阻力系数）。",
    "未提供零升阻力系数，升致阻力计算无法进行",
    "在 aero 块中补充 zero_lift_cd 值"},
   // aero 阻力曲线 Mach 数不足
   {ut::msg::MSG_PLAT_AERO_TOO_FEW_MACH,
    "'aero' must specify at least two Mach numbers for drag curve interpolation.",
    "aero 定义必须至少提供两个 Mach 数用于阻力曲线插值。",
    "drag_curve 数据点不足两个，无法构建插值表",
    "补充至少两个 Mach 数的阻力数据点"},
   // aero 跨音速段阻力系数未上升
   {ut::msg::MSG_PLAT_AERO_NO_TRANSONIC_RISE,
    "'aero' drag coefficient must rise through the transonic region.",
    "aero 的阻力系数必须在跨音速段呈上升趋势。",
    "跨音速区阻力系数未体现物理上应有的增长",
    "调整跨音速段阻力数据，使阻力系数随 Mach 数上升"},
   // aero 跨音速起止 Mach 颠倒
   {ut::msg::MSG_PLAT_AERO_BAD_TRANSONIC,
    "'aero' beginning transonic Mach number must be greater than ending Mach number.",
    "aero 的跨音速起始 Mach 数必须大于结束 Mach 数。",
    "transonic 区域起止值顺序颠倒",
    "按框架约定修正跨音速段起止 Mach 数的大小关系"},
   // aero 超音速 Mach 未高于跨音速段
   {ut::msg::MSG_PLAT_AERO_BAD_SUPERSONIC,
    "'aero' ending supersonic Mach number must be above the transonic range.",
    "aero 的超音速结束 Mach 数必须高于跨音速范围。",
    "超音速段上界未超出跨音速段，区间重叠",
    "调整超音速结束 Mach 数，确保高于跨音速段上界"},
   // 公路运动体找不到路线
   {ut::msg::MSG_PLAT_ROAD_NO_ROUTE,
    "Platform was unable to find route.",
    "平台找不到路线。",
    "公路运动体引用的路线名未定义",
    "核对想定中的路线定义与公路运动体的引用名"},
   // 公路运动体最短路径生成失败
   {ut::msg::MSG_PLAT_ROAD_NO_PATH,
    "Platform could not generate shortest path from initial position to destination.",
    "平台无法生成从初始位置到目的地的最短路径。",
    "路网中初始位置与目的地之间无连通路径",
    "检查路网拓扑连通性，或调整运动体的初始位置与目的地"},
   // start/end_position 必须成对
   {ut::msg::MSG_PLAT_ROAD_POS_PAIR,
    "If either 'start_position' or 'end_position' is specified, then both must be specified.",
    "start_position 与 end_position 必须同时指定（或同时缺省）。",
    "只指定了起点或终点之一",
    "成对提供 start_position 与 end_position，或全部删除"},
   // 公路运动体缺 speed 指定
   {ut::msg::MSG_PLAT_ROAD_NO_SPEED,
    "Road Mover's 'speed' must specified.",
    "公路运动体必须指定 speed（速度）。",
    "未提供运动速度",
    "在公路运动体定义中补充 speed 值"},
   // 交换处理器 container 初始化失败
   {ut::msg::MSG_PLAT_EXCH_CONTAINER_INIT,
    "Platform's container failed initialization.",
    "平台的交换容器初始化失败（详见附注 Container 名）。",
    "container 组件 Initialize 返回失败",
    "检查对应 container 定义（容量等参数是否有效）"},
   // 交换处理器 transactor 初始化失败
   {ut::msg::MSG_PLAT_EXCH_TRANSACTOR_INIT,
    "Platform's transactor failed initialization.",
    "平台的交换事务器初始化失败（详见附注 Transactor 名）。",
    "transactor 组件 Initialize 返回失败",
    "检查对应 transactor 定义（交换内容等参数是否有效）"},
   // on_transaction_complete 脚本校验失败
   {ut::msg::MSG_PLAT_EXCH_SCRIPT_COMPLETE,
    "Could not validate the 'on_transaction_complete' script.",
    "无法校验 on_transaction_complete 脚本。",
    "脚本不存在或语法/签名不符合回调要求",
    "核对脚本定义与签名是否满足事务完成回调约定"},
   // on_request_failed 脚本校验失败
   {ut::msg::MSG_PLAT_EXCH_SCRIPT_REQ_FAIL,
    "Could not validate the 'on_request_failed' script.",
    "无法校验 on_request_failed 脚本。",
    "脚本不存在或语法/签名不符合回调要求",
    "核对脚本定义与签名是否满足请求失败回调约定"},
   // 找不到 MESSAGE 变量
   {ut::msg::MSG_PLAT_MSGPROC_NO_MESSAGE_VAR,
    "Unable to locate MESSAGE variable.",
    "找不到 MESSAGE 变量。",
    "on_message 脚本的 MESSAGE 局部变量未注册（脚本上下文异常）",
    "检查 on_message 脚本是否通过标准消息处理器入口调用"},
   // 找不到消息处理器脚本
   {ut::msg::MSG_PLAT_MSGPROC_NO_SCRIPT,
    "Unable to locate script.",
    "找不到指定的消息处理器脚本（详见附注 Script 名）。",
    "script 命令指定的脚本名未在平台脚本集中定义",
    "核对脚本名拼写与平台上的脚本定义"},
   // 消息处理器脚本引用无法解析
   {ut::msg::MSG_PLAT_MSGPROC_SCRIPT_UNRESOLVED,
    "Unable to resolve reference to script.",
    "无法解析消息处理器的脚本引用（详见附注 Script 名）。",
    "初始化阶段按名称解析脚本失败",
    "确认脚本已在平台脚本集中注册且名称一致"},

   // ---- 平台/运动段（0x0002xxxx）processor 域（第十一批）----
   // 制导计算机未定义任何阶段
   {ut::msg::MSG_GC_NO_PHASES,
    "No phases have been defined.",
    "未定义任何制导阶段（phase）。",
    "guidance 计算机块中没有任何 phase 定义",
    "在 processor 定义中补充至少一个 phase 块"},
   // 初始阶段名无效
   {ut::msg::MSG_GC_BAD_INIT_PHASE,
    "Invalid initial_phase name.",
    "initial_phase（初始阶段）名称无效（详见附注 Name）。",
    "指定的初始阶段名未在 phase 列表中定义",
    "核对 initial_phase 与 phase 定义的名称是否一致"},
   // 制导阶段初始化失败
   {ut::msg::MSG_GC_PHASE_INIT_FAIL,
    "Initialization of phase failed.",
    "制导阶段初始化失败（详见附注 Phase）。",
    "阶段内的命令或脚本初始化未通过",
    "检查该阶段块中的命令与脚本定义"},
   // 找不到阶段引用的制导程序
   {ut::msg::MSG_GC_NO_PROGRAM,
    "Unable to find program referenced by phase.",
    "找不到阶段引用的制导程序（详见附注 Program/Phase）。",
    "use_program 引用的程序名未定义",
    "核对 use_program 与 program 定义的名称是否一致"},
   // 制导程序初始化失败
   {ut::msg::MSG_GC_PROG_INIT_FAIL,
    "Failed to initialize program.",
    "制导程序初始化失败（详见附注 Program）。",
    "程序参数缺失或依赖条件不满足",
    "检查程序定义中的参数配置"},
   // aimpoint_azimuth_offset 方向值无效
   {ut::msg::MSG_GC_BAD_AZ_DIR,
    "Invalid value of 'aimpoint_azimuth_offset' direction.",
    "aimpoint_azimuth_offset（瞄准点方位偏移）的方向值无效（详见附注 Direction）。",
    "direction 取值不是 left/right/either 之一",
    "将 direction 取值改为 left、right 或 either"},
   // guidance_target 取值无效
   {ut::msg::MSG_GC_BAD_TARGET,
    "Invalid value for 'guidance_target'.",
    "guidance_target（制导目标）取值无效（详见附注 Value）。",
    "取值不在支持的枚举范围内",
    "按文档核对 guidance_target 的合法取值"},
   // 未知的比例导引方法
   {ut::msg::MSG_GC_BAD_PN_METHOD,
    "Unknown 'proportional_navigation_method'.",
    "未知的 proportional_navigation_method（比例导引方法，详见附注 Value）。",
    "取值不是 pure 或 augmented",
    "将取值改为 pure 或 augmented"},
   // 脚本命令指定的阶段名无效
   {ut::msg::MSG_GC_SCRIPT_BAD_PHASE,
    "Invalid phase name specified on a script command.",
    "脚本命令指定的制导阶段名无效（详见附注 Phase）。",
    "脚本命令引用的阶段名未在 phase 列表中定义",
    "核对脚本命令中的阶段名与 phase 定义"},
   // 未知的阶段切换变量
   {ut::msg::MSG_GC_BAD_PHASE_VAR,
    "Unknown phase change variable.",
    "未知的阶段切换变量（详见附注 Platform/Computer）。",
    "phase_change 的变量名不在支持列表中",
    "核对 phase_change 的变量名取值"},
   // 请求的制导阶段不存在
   {ut::msg::MSG_GC_PHASE_MISSING,
    "Requested guidance phase does not exist.",
    "请求的制导阶段不存在（详见附注 Requested Phase）。",
    "SelectPhase 等命令引用的阶段名未定义",
    "核对命令中的阶段名与 phase 定义"},
   // next_phase 引用未定义阶段
   {ut::msg::MSG_GC_NEXT_PHASE_UNDEF,
    "Phase has a 'next_phase' command that refers to undefined phase.",
    "阶段的 next_phase 命令引用了未定义的阶段（详见附注 Phase/Undefined Phase）。",
    "next_phase 指定的阶段名不在 phase 列表中",
    "补充对应 phase 定义或修正 next_phase 的阶段名"},
   // 瞄准点方位/距离偏移量须成对
   {ut::msg::MSG_GC_OFFSET_PAIR,
    "'aimpoint_azimuth_offset' and 'aimpoint_range_offset' must both be specified if either is specified.",
    "aimpoint_azimuth_offset 与 aimpoint_range_offset 必须同时指定（或同时缺省）。",
    "两个瞄准点偏移量只指定了其一",
    "成对提供两个偏移量，或全部删除"},
   // 制导程序仅适用于 WSF_GUIDED_MOVER
   {ut::msg::MSG_GP_NEEDS_GUIDED_MOVER,
    "Guidance program is valid only with WSF_GUIDED_MOVER.",
    "该制导程序仅适用于 WSF_GUIDED_MOVER 运动体（详见附注 Type）。",
    "平台运动体类型不是 WSF_GUIDED_MOVER",
    "为平台配置 WSF_GUIDED_MOVER 类型的运动体"},
   // 期望飞行路径角未定义（两处调用点共用）
   {ut::msg::MSG_GP_NO_FPA,
    "The desired flight path angle is undefined.",
    "期望的飞行路径角未定义（详见附注 Platform/Computer）。",
    "程序与发射计算机均未提供有效的飞行路径角",
    "在程序或发射计算机中指定有效的飞行路径角"},
   // 找不到资产评分函数脚本
   {ut::msg::MSG_PP_NO_ASSET_SCRIPT,
    "Unable to find script for asset importance function.",
    "找不到资产重要性函数的脚本（详见附注 Script）。",
    "asset_scoring_function 指定的脚本未定义",
    "核对脚本名与平台脚本集中的定义"},
   // 资产评分函数签名不符
   {ut::msg::MSG_PP_ASSET_SIG,
    "Perception processor asset scoring function received unexpected signature.",
    "感知处理器的资产评分函数签名不符合要求（详见附注 Received/Expected）。",
    "脚本原型与 double(WsfAssetPerception) 期望签名不一致",
    "按期望签名修正脚本的参数与返回类型"},
   // 找不到威胁评分函数脚本
   {ut::msg::MSG_PP_NO_THREAT_SCRIPT,
    "Unable to find script for threat importance function.",
    "找不到威胁重要性函数的脚本（详见附注 Script）。",
    "threat_scoring_function 指定的脚本未定义",
    "核对脚本名与平台脚本集中的定义"},
   // 威胁评分函数签名不符
   {ut::msg::MSG_PP_THREAT_SIG,
    "Perception processor threat scoring function received unexpected signature.",
    "感知处理器的威胁评分函数签名不符合要求（详见附注 Received/Expected）。",
    "脚本原型与 double(WsfLocalTrack) 期望签名不一致",
    "按期望签名修正脚本的参数与返回类型"},
   // 空对地模式与上方引爆判据冲突
   {ut::msg::MSG_FUSE_A2G_AGL_SET,
    "Incompatible inputs with 'air_to_ground_mode' set, and 'detonate_above_height_agl' set.",
    "输入冲突：已设置 air_to_ground_mode 的同时又设置了 detonate_above_height_agl。",
    "空对地模式强制在下降穿过指定高度时引爆，与上方引爆判据冲突",
    "删除 detonate_above_height_agl 或关闭 air_to_ground_mode"},
   // 空对地模式与上方引爆高度冲突
   {ut::msg::MSG_FUSE_A2G_AGL_NONZERO,
    "Incompatible inputs, with 'air_to_ground_mode' set, and non-zero 'detonate_above_height_agl' set.",
    "输入冲突：已设置 air_to_ground_mode 且 detonate_above_height_agl 为非零值。",
    "空对地模式下引爆高度必须保持为 0",
    "将 detonate_above_height_agl 置 0 或删除该设置"},
   // 创建武器交战对象失败
   {ut::msg::MSG_FUSE_NO_ENGAGEMENT,
    "Failed to create weapon engagement object.",
    "创建武器交战对象失败（详见附注 Platform/Fuse）。",
    "武器交战对象 Assign 分配失败",
    "检查平台武器与交战相关配置"},
   // classifier 处理器类型错误
   {ut::msg::MSG_WTP_BAD_CLASSIFIER,
    "CreateThreatList: 'classifier' is not type 'WsfClassificationProcessor'.",
    "CreateThreatList：classifier 不是 WsfClassificationProcessor 类型。",
    "classifier 指定的处理器类型不正确",
    "将 classifier 指向 WsfClassificationProcessor 类型的处理器"},
   // 找不到武器的 Pk 计算器
   {ut::msg::MSG_WTP_NO_PK_CALC,
    "GetThreatList: No pkCalculator found for weapon.",
    "GetThreatList：找不到武器的 pkCalculator（杀伤概率计算器，详见附注 Weapon）。",
    "武器与候选目标实体类型组合缺少 Pk 表定义",
    "为该武器/实体类型组合补充 pk_table 或在 wx 表中登记"},
   // 找不到量子分配器脚本
   {ut::msg::MSG_QA_NO_SCRIPT,
    "Unable to find script for quantum allocator.",
    "找不到量子分配器的脚本（详见附注 Type）。",
    "分配器指定的脚本方法未定义",
    "核对脚本名与平台脚本集中的定义"},
   // 量子分配器脚本签名无效
   {ut::msg::MSG_QA_BAD_SCRIPT,
    "Invalid inputs or outputs for script as quantum allocator.",
    "量子分配器脚本的输入输出无效（详见附注 Type）。",
    "脚本签名与 Map<WsfAssetPerception,WsfQuantumTask> 分配器约定不符",
    "按分配器脚本约定修正函数签名（返回 Map，输入 Array<Array<double>> 等）"},
   // 找不到量子生成器脚本
   {ut::msg::MSG_QG_NO_SCRIPT,
    "Unable to find script for quantum generator.",
    "找不到量子生成器的脚本（详见附注 Type）。",
    "生成器指定的脚本方法未定义",
    "核对脚本名与平台脚本集中的定义"},
   // 量子生成器脚本签名无效
   {ut::msg::MSG_QG_BAD_SCRIPT,
    "Invalid inputs or outputs for script as quantum generator.",
    "量子生成器脚本的输入输出无效（详见附注 Type）。",
    "脚本签名与 Array<WsfQuantumTask> 生成器约定不符",
    "按生成器脚本约定修正函数签名（输入 Array<WsfLocalTrack> 与 Array<WsfAssetPerception>）"},
   // 找不到量子评估器脚本
   {ut::msg::MSG_QE_NO_SCRIPT,
    "Unable to find script for quantum evaluator.",
    "找不到量子评估器的脚本（详见附注 Type）。",
    "评估器指定的脚本方法未定义",
    "核对脚本名与平台脚本集中的定义"},
   // 量子评估器脚本签名无效
   {ut::msg::MSG_QE_BAD_SCRIPT,
    "Invalid inputs or outputs for script as quantum evaluator.",
    "量子评估器脚本的输入输出无效（详见附注 Type）。",
    "脚本签名与 double(WsfQuantumTask, WsfAssetPerception) 评估器约定不符",
    "按评估器脚本约定修正函数签名"},
   // uplink_path 引用未知传感器或通信设备
   {ut::msg::MSG_UPLINK_BAD_PATH,
    "Unknown sensor or comm specified for uplink_path.",
    "uplink_path 指定了未知的传感器或通信设备（详见附注 Sensor/Comm）。",
    "weapon_uplink_path 引用的传感器或 comm 未在平台上定义",
    "核对平台上的传感器与 comm 名称"},
   // 武器跟踪处理器找不到传感器
   {ut::msg::MSG_WTRACK_NO_SENSOR,
    "Sensor does not exist.",
    "传感器不存在（详见附注 Sensor）。",
    "武器跟踪处理器引用的传感器未在平台上定义",
    "核对传感器名称与平台定义"},
   // 找不到要拒绝的任务
   {ut::msg::MSG_QT_NO_TASK_REJECT,
    "Cannot find task to reject.",
    "找不到要拒绝的任务（详见附注 Task/Task Type/Resource）。",
    "拒绝请求到达时任务已不在接收/清除列表中",
    "检查任务拒绝的时序，确认任务尚未被移除"},

   // ---- 平台/运动段（0x0002xxxx）mover 域散点（第十二批）----
   // 混合运动体缺 follower_mover
   {ut::msg::MSG_HM_NO_FOLLOWER,
    "Must specify a valid follower_mover inside the hybrid mover block.",
    "hybrid_mover 块内必须指定有效的 follower_mover（跟随运动体）。",
    "hybrid_mover 块中未定义 follower_mover 子块",
    "在 hybrid_mover 块内补充 follower_mover 定义"},
   // 混合运动体缺 waypoint_mover
   {ut::msg::MSG_HM_NO_WAYPOINT,
    "Must specify a valid waypoint_mover inside the hybrid mover block.",
    "hybrid_mover 块内必须指定有效的 waypoint_mover（航路点运动体）。",
    "hybrid_mover 块中未定义 waypoint_mover 子块",
    "在 hybrid_mover 块内补充 waypoint_mover 定义"},
   // 混合运动体缺 current_mover 类型
   {ut::msg::MSG_HM_NO_CURRENT_TYPE,
    "Must specify a current_mover type inside the hybrid mover block.",
    "hybrid_mover 块内必须指定 current_mover 类型。",
    "未指定初始启用的 current_mover 类型",
    "补充 current_mover <类型> 指定"},
   // aero 初始化时未关联平台
   {ut::msg::MSG_AERO_NO_PLATFORM,
    "'aero' was not supplied a WsfPlatform during Initialize().",
    "aero（气动模型）在 Initialize() 时未关联平台（详见附注 T）。",
    "初始化时平台指针为空",
    "检查 aero 模型与平台组件的装配顺序"},
   // aero 参考面积为零
   {ut::msg::MSG_AERO_ZERO_REF_AREA,
    "'aero' 'reference_area' was Zero.",
    "aero 的 reference_area（参考面积）为零（详见附注 T）。",
    "未指定 reference_area",
    "在 aero 块内指定非零 reference_area"},
   // 偏移运动体缺参考平台
   {ut::msg::MSG_OM_NO_REF_PLATFORM,
    "Reference platform not specified.",
    "未指定参考平台（详见附注 Platform/Mover）。",
    "offset mover 缺 reference_platform 指定",
    "在 mover 块内补充 reference_platform"},
   // 目标点附近找不到有效点（两处调用点共用）
   {ut::msg::MSG_PF_NO_VALID_POINT,
    "FindClosestValidPoint() could not find a valid point near target.",
    "FindClosestValidPoint() 在目标点附近找不到有效点（详见附注 Target WCS 坐标）。",
    "目标点远离路网/导航网格的任何有效节点",
    "将目标移动到有效节点附近或补充路网覆盖"},
   // 路径查找器初始化失败
   {ut::msg::MSG_PF_INIT_FAIL,
    "Failed to initialize path finder.",
    "路径查找器初始化失败（详见附注 Path Finder）。",
    "路径查找器资源或输入未通过初始化",
    "检查 path_finder 定义与路网数据"},
   // 源/目的节点间无关联路线
   {ut::msg::MSG_RN_NO_ROUTE_ASSOC,
    "Could not find the associated route for source and destination nodes.",
    "找不到源节点与目的节点之间的关联路线（详见附注 Source/Destination Node）。",
    "两节点之间没有直接路线连接",
    "在路网中补充连接两节点的路线"},
   // 表格驱动油耗缺油耗率表
   {ut::msg::MSG_TRF_NO_TABLE,
    "Fuel did not define a Fuel Rate Table.",
    "燃油未定义油耗率表（详见附注 Fuel Rate Table 名）。",
    "表格驱动油耗模型未定义任何油耗率表",
    "在 fuel 块内定义至少一个油耗率表"},
   // 表格驱动油耗当前模式无表
   {ut::msg::MSG_TRF_NO_MODE_TABLE,
    "There is no table matching the current 'mode'.",
    "没有与当前 mode（模式）匹配的油耗表（详见附注 Fuel Rate Table 名）。",
    "当前平台/燃料 mode 没有对应油耗率表",
    "为该 mode 定义油耗率表或核对 mode 名称"},
   // 表格驱动油耗模式缺 fuel_table（两处调用点共用）
   {ut::msg::MSG_TRF_NO_FUEL_TABLE,
    "No fuel_table for mode.",
    "当前 mode（模式）没有 fuel_table（详见附注 Mode）。",
    "计算油耗率时找不到当前 mode 的表",
    "为该 mode 定义 fuel_table 或核对 mode 名称"},
   // 变速率油耗缺油耗率表
   {ut::msg::MSG_VRF_NO_TABLE,
    "Fuel did not define a Fuel Rate Table.",
    "燃油未定义油耗率表（详见附注 Fuel 名）。",
    "变速率油耗模型未定义任何油耗率表",
    "在 fuel 块内定义至少一个油耗率表"},
   // 变速率油耗表无高度数据
   {ut::msg::MSG_VRF_NO_ALTITUDES,
    "Fuel Rate Table does not contain any defined altitudes.",
    "油耗率表未定义任何高度层数据（详见附注 Fuel 名）。",
    "油耗率表内没有高度层数据",
    "在表中补充至少一个高度层数据"},
   // 变速率油耗当前模式无表
   {ut::msg::MSG_VRF_NO_MODE_TABLE,
    "There is no Fuel Rate Table matching the current 'mode'.",
    "没有与当前 mode（模式）匹配的油耗率表（详见附注 Fuel/Mode）。",
    "当前 mode 没有对应油耗率表",
    "为该 mode 定义油耗率表或核对 mode 名称"},
   // 变速率油耗计算时模式无表
   {ut::msg::MSG_VRF_NO_TABLE_MODE,
    "Platform has no table in variable rate fuel for mode.",
    "平台在变速率油耗模型中该 mode（模式）无表（详见附注 Mode）。",
    "计算油耗率时找不到当前 mode 的表",
    "为该 mode 定义油耗率表或核对 mode 名称"},
   // 可受油但未指定最大受油率
   {ut::msg::MSG_TF_NO_MAX_RATE,
    "Fuel is allowed to receive, but was not given a max receive rate.",
    "燃油允许受油但未指定最大受油率（详见附注 Fuel 名）。",
    "设置了受油方式但缺 max_receive_rate",
    "补充 max_receive_rate 指定"},
   // 加油机无法向受油机供油（三处调用点共用，Debug 门控）
   {ut::msg::MSG_TF_CANNOT_SUPPLY,
    "Tanker cannot supply fuel to receiver.",
    "加油机无法向受油机供油（原因详见附注）。",
    "受油机未开放受油口/无匹配加油站位/受油机已在加油",
    "按附注原因核对受油机与加油站位配置"},
   // 无法打开 TSPI 文件
   {ut::msg::MSG_TSPI_OPEN_FAIL,
    "Cannot open TSPI file.",
    "无法打开 TSPI 文件（详见附注 File）。",
    "文件不存在或无访问权限",
    "核对 tspi_mover 文件路径"},
   // 最短路径代价矩阵为空
   {ut::msg::MSG_SP_EMPTY_MATRIX,
    "WsfShortestPath: Cost Matrix is empty.",
    "最短路径组件的代价矩阵为空。",
    "未提供任何路网节点代价数据",
    "检查路网节点与路线定义"},
   // 最短路径代价矩阵非方阵
   {ut::msg::MSG_SP_NOT_SQUARE,
    "WsfShortestPath: Cost Matrix is not square.",
    "最短路径组件的代价矩阵不是方阵。",
    "矩阵行列数不一致",
    "检查路网节点连接关系的对称定义"},
   // 最短路径代价矩阵不对称
   {ut::msg::MSG_SP_NOT_SYMMETRIC,
    "WsfShortestPath: Cost Matrix is not symmetric.",
    "最短路径组件的代价矩阵不对称。",
    "无向边代价 m[n] 与 n[m] 不相等",
    "检查路线双向代价定义"},
   // 最短路径节点索引越界
   {ut::msg::MSG_SP_BAD_NODE_IDX,
    "Node indexing error in WsfShortestPath::FindShortestPath",
    "最短路径查找的节点索引越界（详见附注 From/To Node 与 Max）。",
    "节点索引超出代价矩阵规模",
    "检查节点编号与路网规模"},

   // ---- 平台/运动段（0x0002xxxx）wsf_mil mover 域（第十三批）----
   // 直线运动体未指定轨迹类型
   {ut::msg::MSG_SLM_NO_TRAJECTORY,
    "No trajectory type was specified.",
    "未指定轨迹类型（详见附注 Platform）。",
    "mover 定义中缺少 trajectory 指定",
    "补充 trajectory 关键字指定"},
   // 直线运动体指定了多个轨迹类型
   {ut::msg::MSG_SLM_MULTIPLE_TRAJECTORY,
    "Cannot specify more than one trajectory type.",
    "不能同时指定多个轨迹类型（详见附注 Platform）。",
    "trajectory 指定重复",
    "只保留一个 trajectory 类型指定"},
   // 直线运动体弹射速度为零
   {ut::msg::MSG_SLM_ZERO_EJECTION,
    "Ejection velocity is zero.",
    "弹射速度为零（详见附注 Platform）。",
    "提升弹射（pop_up）模式下速度为 0",
    "指定非零的提升速度"},
   // 拖曳运动体初始化时找不到武器交战对象
   {ut::msg::MSG_TOWED_NO_ENGAGEMENT,
    "WsfTowedMover could not find weapon engagement on initialization.",
    "拖曳运动体初始化时找不到武器交战对象（详见附注 Platform/Mover）。",
    "拖曳载荷所在平台未挂在任何交战对象下",
    "检查武器交战与拖曳载荷的平台装配关系"},
   // 阶段初始化失败（GuidedMover/TBM_Mover 两处调用点共用）
   {ut::msg::MSG_GM_TBM_STAGE_INIT_FAIL,
    "Platform failed to initialize stage.",
    "平台阶段初始化失败（详见附注 Platform/Stage）。",
    "stage 中的定义参数不完整或校验未通过",
    "检查该 stage 块中的定义"},
   // divert_thrust 须与 divert_fuel_mass 同用
   {ut::msg::MSG_GM_NEED_DIVERT_FUEL_MASS,
    "'divert_fuel_mass' must be specified with 'divert_thrust'.",
    "使用 divert_thrust 时必须同时指定 divert_fuel_mass。",
    "divert_thrust 已指定但缺 divert_fuel_mass",
    "补充 divert_fuel_mass 指定"},
   // divert_thrust 须与 divert_fuel_flow_rate 同用
   {ut::msg::MSG_GM_NEED_DIVERT_FUEL_RATE,
    "'divert_fuel_flow_rate' must be specified with 'divert_thrust'.",
    "使用 divert_thrust 时必须同时指定 divert_fuel_flow_rate。",
    "divert_thrust 已指定但缺 divert_fuel_flow_rate",
    "补充 divert_fuel_flow_rate 指定"},
   // nozzle_exit_area 与推力导出值不一致
   {ut::msg::MSG_GM_NOZZLE_AREA_INCONSISTENT,
    "The input value of 'nozzle_exit_area' is not consistent with the value of the derived from 'vacuum_thrust' and 'sea_level_thrust'.",
    "nozzle_exit_area 的输入值与由 vacuum_thrust 和 sea_level_thrust 导出的值不一致（详见附注 Input/Derived）。",
    "真空/海平面推力与喷管出口面积参数不自洽",
    "修正 nozzle_exit_area 或推力参数使关系式成立"},
   // 无法计算 sea_level_specific_impulse
   {ut::msg::MSG_GM_CANNOT_CALC_SL_SI,
    "Unable to calculate 'sea_level_specific_impulse'.",
    "无法计算 sea_level_specific_impulse（海平面比冲）。",
    "缺 sea_level_thrust 或 burn_rate 等输入",
    "补充推力/燃耗率等输入参数"},
   // 无法计算 vacuum_specific_impulse
   {ut::msg::MSG_GM_CANNOT_CALC_VAC_SI,
    "Unable to calculate 'vacuum_specific_impulse'.",
    "无法计算 vacuum_specific_impulse（真空比冲）。",
    "缺 vacuum_thrust 或 burn_rate 等输入",
    "补充推力/燃耗率等输入参数"},
   // 计算出的比值不相等
   {ut::msg::MSG_GM_RATIOS_NOT_EQUAL,
    "Ratios computed are not equal and should be!",
    "计算出的两个比值应当相等但不相等（详见附注 两个比值）。",
    "推力/比冲参数之间关系不自洽",
    "按附注提示修正推力或比冲参数"},
   // vacuum/sea_level_thrust 须指定 nozzle_exit_area
   {ut::msg::MSG_GM_NEED_NOZZLE_AREA,
    "'nozzle_exit_area' must be specified for 'vacuum_thrust' or 'sea_level_thrust'.",
    "使用 vacuum_thrust 或 sea_level_thrust 时必须指定 nozzle_exit_area。",
    "指定了真空或海平面推力但缺喷管出口面积",
    "补充 nozzle_exit_area 指定"},
   // 无法确定 empty/fuel/total_mass
   {ut::msg::MSG_GM_CANNOT_DET_MASS,
    "Unable to determine 'empty_mass', 'fuel_mass', or 'total_mass'.",
    "无法确定 empty_mass、fuel_mass 或 total_mass。",
    "三个质量参数指定不足",
    "至少补充两个质量参数"},
   // total/empty/fuel_mass 不一致
   {ut::msg::MSG_GM_MASS_INCONSISTENT,
    "Inconsistent 'total_mass', 'empty_mass', and 'fuel_mass'.",
    "total_mass、empty_mass 与 fuel_mass 不一致。",
    "total_mass 不等于 empty_mass + fuel_mass",
    "修正质量参数满足 total = empty + fuel"},
   // 无法确定 thrust/burn_rate/specific_impulse
   {ut::msg::MSG_GM_CANNOT_DET_THRUST,
    "Unable to determine 'thrust', 'burn_rate', or 'specific_impulse'.",
    "无法确定 thrust、burn_rate 或 specific_impulse。",
    "三个推进参数指定不足",
    "至少补充两个推进参数"},
   // vacuum_thrust 相关三量不一致
   {ut::msg::MSG_GM_VAC_THRUST_INCONSISTENT,
    "Inconsistent 'vacuum_thrust', 'vacuum_specific_impulse', and 'burn_rate'.",
    "vacuum_thrust、vacuum_specific_impulse 与 burn_rate 不一致。",
    "推力 = 比冲×燃耗率×g0 关系式不成立",
    "修正其中之一使关系式成立"},
   // sea_level_thrust 相关三量不一致
   {ut::msg::MSG_GM_SL_THRUST_INCONSISTENT,
    "Inconsistent 'sea_level_thrust', 'sea_level_specific_impulse', and 'burn_rate'.",
    "sea_level_thrust、sea_level_specific_impulse 与 burn_rate 不一致。",
    "推力 = 比冲×燃耗率×g0 关系式不成立",
    "修正其中之一使关系式成立"},
   // thrust/burn_rate/specific_impulse 不一致
   {ut::msg::MSG_GM_THRUST_INCONSISTENT,
    "Inconsistent 'thrust', 'burn_rate', and 'specific_impulse'.",
    "thrust、burn_rate 与 specific_impulse 不一致。",
    "推力 = 比冲×燃耗率×g0 关系式不成立",
    "修正其中之一使关系式成立"},
   // 燃料消耗超过 fuel_mass
   {ut::msg::MSG_GM_FUEL_EXCEEDS,
    "Fuel consumed is greater than 'fuel_mass'.",
    "燃料消耗量超过了 fuel_mass。",
    "燃耗计算累计值超过初始燃料量",
    "检查 burn_time 与 burn_rate 的匹配关系"},
   // aero 类型未识别（参数化）
   {ut::msg::MSG_GM_AERO_TYPE_BAD,
    "'aero' type: {aeroType}",
    "aero（气动模型）类型：{aeroType}（未能识别）。",
    "指定的 aero 类型名未在类型列表中注册",
    "核对 aero 类型名与注册列表"},
   // 须指定 aero 类型或 aero none
   {ut::msg::MSG_GM_AERO_MUST_SPECIFY,
    "'aero <aero-type>' or 'aero none' must be specified.",
    "必须指定 'aero <类型>' 或 'aero none'。",
    "mover 块内未指定 aero",
    "补充 aero 类型指定或声明 aero none"},
   // cep_value 与 cep_table 不能同时指定
   {ut::msg::MSG_TBM_CEP_BOTH,
    "Platform cannot supply both a 'cep_value' and a 'cep_table'.",
    "平台不能同时指定 cep_value 和 cep_table（详见附注 Platform）。",
    "两种圆概率误差指定方式冲突",
    "只保留 cep_value 或 cep_table 其一"},
   // TBM 弹道无法到达目标点
   {ut::msg::MSG_TBM_CANNOT_REACH,
    "Platform TBM Trajectory cannot reach the desired location.",
    "TBM 弹道无法到达期望位置（详见附注 Platform）。",
    "目标点超出该弹道的最大射程",
    "缩短射程或增大推进能量"},
   // 最大射程迭代次数超限
   {ut::msg::MSG_TBM_MAXRANGE_ITER,
    "Platform exceeded iteration count determining maximum range.",
    "确定最大射程的迭代次数超限（详见附注 Platform）。",
    "最大射程搜索不收敛",
    "检查质量/推力参数的合理性"},
   // 三段飞行时间须至少一个大于零
   {ut::msg::MSG_TBM_NEED_COAST_TIME,
    "At least one of 'pre_burn_coast_time', 'burn_time', or 'post_burn_coast_time' must be greater than zero.",
    "pre_burn_coast_time、burn_time、post_burn_coast_time 必须至少有一个大于零。",
    "三段飞行时间全部未指定或为零",
    "至少指定一段非零的飞行时间"},
   // 未指定 total_mass 须给出 empty+fuel
   {ut::msg::MSG_TBM_NEED_MASS_DETAIL,
    "If 'total_mass' is not specified then you must specify 'empty_mass' and 'fuel_mass'.",
    "未指定 total_mass 时必须同时指定 empty_mass 和 fuel_mass。",
    "质量参数指定不足",
    "补充 empty_mass 与 fuel_mass 或直接给出 total_mass"},
   // fuel_mass 与 fuel_mass_fraction 不能同时非零
   {ut::msg::MSG_TBM_FUEL_AND_FRACTION,
    "'fuel_mass' and 'fuel_mass_fraction' cannot both be non-zero.",
    "fuel_mass 与 fuel_mass_fraction 不能同时非零。",
    "两种燃料量指定方式冲突",
    "只保留其一"},
   // fuel mass 须小于 total mass
   {ut::msg::MSG_TBM_FUEL_GE_TOTAL,
    "'fuel mass' must be less than the total mass.",
    "fuel mass 必须小于 total mass（详见附注 Fuel/Total）。",
    "燃料量大于等于总质量",
    "修正 fuel_mass 或 total_mass"},
   // 须指定 fuel_mass 或 fuel_mass_fraction
   {ut::msg::MSG_TBM_NEED_FUEL_OR_FRACTION,
    "'fuel_mass' or 'fuel_mass_fraction' must be specified.",
    "必须指定 fuel_mass 或 fuel_mass_fraction。",
    "两种燃料量指定方式均未提供",
    "补充其中之一"},
   // 须指定 thrust 或 specific_impulse
   {ut::msg::MSG_TBM_THRUST_OR_SI,
    "Either 'thrust' or 'specific_impulse' must be specified.",
    "必须指定 thrust 或 specific_impulse 其一。",
    "两个推进参数均未提供",
    "补充其中之一"},
   // thrust 与 specific_impulse 不能同时指定
   {ut::msg::MSG_TBM_THRUST_AND_SI,
    "'thrust' and 'specific_impulse' cannot both be specified.",
    "thrust 与 specific_impulse 不能同时指定。",
    "两种推力参数指定方式冲突",
    "只保留其一"},
   // total/empty/fuel_mass 不一致
   {ut::msg::MSG_TBM_MASS_NOT_CONSISTENT,
    "'total_mass', 'empty_mass' and 'fuel_mass' are not consistent.",
    "total_mass、empty_mass 与 fuel_mass 不一致。",
    "total_mass 不等于 empty_mass + fuel_mass",
    "修正质量参数满足 total = empty + fuel"},
   // 导航误差最终报告引出句（析构时 GPS 非激活）
   {ut::msg::MSG_NAV_ERROR_INS_REPORT,
    "WsfNavigationError() Report:",
    "导航误差报告：",
    "平台销毁时 GPS 处于非激活状态，输出最终 INS 位置误差报告",
    "状态报告，无需修复"},
   // 目标点附近找不到有效点
   {ut::msg::MSG_PF_FIND_CLOSEST_FAIL,
    "FindClosestValidPoint() could not find a valid point near target.",
    "FindClosestValidPoint() 未能在目标点附近找到有效点。",
    "目标点周边网格节点均不可用（受限区域）",
    "检查目标点是否落在受限区域或导航网格之外"},
   // 路径制导未挂接运动体
   {ut::msg::MSG_PG_NOT_ATTACHED,
    "WsfPathGuidance has not been attached to a mover.",
    "WsfPathGuidance 尚未挂接到运动体。",
    "初始化时运动体指针为空",
    "检查 mover 的路径制导挂接流程"},
   // 路线网络 InsertArea 失败
   {ut::msg::MSG_RN_INSERT_AREA_FAIL,
    "Failure in WsfRouteNetwork: InsertArea().",
    "WsfRouteNetwork 路线段插入失败（InsertArea）。",
    "路线段插入空间索引树失败",
    "检查路线段几何是否退化（零长度线段）"},

   // ---- 平台/运动段（0x0002xxxx）traffic 交通域（第十九批）----
   // 机场引用了未定义的 aircraft_type
   {ut::msg::MSG_AIR_BAD_AC_TYPE,
    "Invalid 'aircraft_type'.",
    "无效的 'aircraft_type'。",
    "机场定义引用的飞机类型未在 platform_type 中定义",
    "核对 Note 中飞机类型名，补充对应平台类型定义"},
   // 机场目的机场未定义
   {ut::msg::MSG_AIR_UNDEF_DEST,
    "Undefined airbase destination.",
    "机场目的机场未定义。",
    "机场目的地列表引用的机场不存在",
    "核对 Note 中目的机场名，确保先定义该机场"},
   // 无法添加机场平台
   {ut::msg::MSG_AIR_NO_ADD_PLATFORM,
    "Unable to add airbase platform.",
    "无法添加机场平台。",
    "机场对应平台加入仿真失败（平台重名等）",
    "查看 Note 中平台名，检查是否有同名平台"},
   // 找不到本地可用飞机类型
   {ut::msg::MSG_AIR_NO_LOCAL_AC,
    "Could not find a local aircraft type to use.",
    "找不到可用的本地飞机类型。",
    "机场未配置任何可在本地起降的飞机类型",
    "在机场定义中补充 aircraft_type 配置"},
   // 无飞机类型可降目的机场
   {ut::msg::MSG_AIR_NO_LAND_AC,
    "No aircraft type may land at destination.",
    "没有飞机类型可以降落到目的机场。",
    "出发机场的飞机类型均不满足目的机场的降落要求",
    "核对 Note 中目的机场，调整机场的飞机类型配置"},
   // 找不到平台对应航班
   {ut::msg::MSG_AIR_NO_FLIGHT,
    "Unable to locate flight for platform.",
    "无法找到平台对应的航班。",
    "航班到达/起飞事件处理时平台不在航班列表中",
    "查看 Notes 中的时间与平台名，检查航班删除逻辑"},
   // 无法创建指定类型飞机
   {ut::msg::MSG_AIR_NO_CREATE_AC,
    "Unable to create aircraft of type.",
    "无法创建指定类型的飞机。",
    "按 aircraft_type 克隆平台类型失败",
    "核对 Note 中飞机类型名，确保 platform_type 已定义"},
   // 飞机运动体须为 WSF_AIR_MOVER
   {ut::msg::MSG_AIR_BAD_MOVER_TYPE,
    "Aircraft mover must be a WSF_AIR_MOVER.",
    "飞机运动体必须为 WSF_AIR_MOVER。",
    "机场交通要求飞机平台使用 WSF_AIR_MOVER",
    "按 Notes 中类型信息修正平台类型的 mover 定义"},
   // 无法将平台加入仿真
   {ut::msg::MSG_AIR_NO_ADD_SIM,
    "Unable to add platform to simulation.",
    "无法将平台加入仿真。",
    "航班创建的飞机平台加入仿真失败",
    "查看 Note 中平台名，检查是否有同名平台"},
   // 备降找不到合适机场
   {ut::msg::MSG_AIR_DIVERT_NO_AIRBASE,
    "Diverting flight. Could not find a suitable alternative airbase.",
    "航班备降：找不到合适的备降机场。",
    "目的机场不可用时未找到满足降落条件的备降机场",
    "查看 Notes 中航班状态与目的机场，扩展可降落的机场配置"},
   // 无效路线网络
   {ut::msg::MSG_TRT_INVALID_NET,
    "Invalid route network.",
    "无效路线网络。",
    "交通扩展引用的 route_network 未在想定中定义",
    "核对 Note 中网络名，补充对应 route_network 定义"},
   // 路线网络初始化失败
   {ut::msg::MSG_TRT_NET_INIT_FAIL,
    "Unable to initialize route network.",
    "无法初始化路线网络。",
    "路线网络对象 Initialize 返回失败",
    "查看该网络的初始化错误信息，检查网络定义"},
   // sea_traffic 无法生成实体
   {ut::msg::MSG_SEA_NO_ENTITY,
    "sea_traffic: Could not generate entity, so no more departure events will be scheduled.",
    "sea_traffic: 无法生成实体，不再安排后续离港事件。",
    "按配置生成海上实体失败（平台类型克隆失败等）",
    "检查 sea_traffic 的平台类型与港口配置"},
   // sea_traffic 运动体为空
   {ut::msg::MSG_SEA_NULL_MOVER,
    "sea_traffic: Cannot set the mover's path because the mover is null.",
    "sea_traffic: 运动体为空，无法设置路径。",
    "生成的平台没有配置 mover",
    "查看 Note 中平台名，为该平台类型补充 mover 定义"},
   // sea_traffic 平台加入仿真失败
   {ut::msg::MSG_SEA_ADD_PLATFORM_FAIL,
    "sea_traffic: There was a problem adding the platform to the simulation.",
    "sea_traffic: 平台加入仿真时出现问题。",
    "AddPlatform 返回失败（平台重名等）",
    "查看 Note 中平台名，检查是否有同名平台"},
   // sea_traffic 无效平台类型
   {ut::msg::MSG_SEA_BAD_PLATFORM_TYPE,
    "sea_traffic: Invalid platform type.",
    "sea_traffic: 无效的平台类型。",
    "按 vehicle 定义的平台类型未定义",
    "核对 Note 中类型名，补充对应 platform_type"},
   // sea_traffic 输出文件打开失败
   {ut::msg::MSG_SEA_OUTPUT_OPEN,
    "sea_traffic: Unable to open output file.",
    "sea_traffic: 无法打开输出文件。",
    "生成 sea_traffic.sed 场景导出文件失败（目录无写权限或被占用）",
    "检查工作目录写权限与文件占用"},
   // sea_traffic 航线不存在
   {ut::msg::MSG_SEA_NO_LANE,
    "sea_traffic: Lane does not exist.",
    "sea_traffic: 航线不存在。",
    "配置引用的 sea_lane 未定义",
    "核对 Note 中航线名，补充对应航线定义"},
   // sea_traffic 航线权重和不为 1
   {ut::msg::MSG_SEA_LANE_WEIGHTS,
    "sea_traffic: The sum of the lane weights must be 1.0.",
    "sea_traffic: 各航线权重之和必须为 1.0。",
    "出港航线权重之和偏离 1.0 超过容差",
    "按 Note 中当前总和调整各航线 weight"},
   // sea_traffic 港口权重数不符
   {ut::msg::MSG_SEA_PORT_WEIGHTS,
    "sea_traffic: The number of port weights does not match the number of outgoing lanes.",
    "sea_traffic: 港口权重数量与出港航线数量不一致。",
    "目的港口权重列表长度与出港航线列表长度不一致",
    "对照配置补齐或删除多余的 port weight 条目"},
   // sea_traffic 区域权重和不为 1
   {ut::msg::MSG_SEA_REGION_WEIGHTS,
    "sea_traffic: The sum of the local traffic region weights must be 1.0.",
    "sea_traffic: 各本地交通区域权重之和必须为 1.0。",
    "本地交通区域权重之和偏离 1.0 超过容差",
    "按 Note 中当前总和调整各区域 weight"},
   // sea_traffic 航线须两个港口
   {ut::msg::MSG_SEA_TWO_PORTS,
    "sea_traffic: Must specify two ports to properly define a lane.",
    "sea_traffic: 定义航线必须指定两个港口。",
    "航线两端港口数不足两个",
    "按 Note 中当前港口数，补齐航线两端的 port 定义"},
   // sea_traffic 港口不存在
   {ut::msg::MSG_SEA_NO_PORT,
    "sea_traffic: Port does not exist.",
    "sea_traffic: 港口不存在。",
    "航线引用的港口未定义",
    "核对 Note 中港口名，补充对应港口定义"},
   // sea_traffic 未指定创建比例
   {ut::msg::MSG_SEA_NO_FRACTION,
    "sea_traffic: Must specify the fraction of {type}s to create.",
    "sea_traffic: 必须指定要创建的 {type} 的比例。",
    "船舶类型的 fraction 未指定或小于等于零",
    "在该类型定义中补充大于零的 fraction 值"},
   // sea_traffic 无效待机航线
   {ut::msg::MSG_SEA_BAD_LOITER_ROUTE,
    "sea_traffic: Invalid loiter route.",
    "sea_traffic: 无效的待机航线。",
    "loiter_route 引用的 route 未定义",
    "核对 Note 中航线名，补充对应 route 定义"},
   // osm_traffic 变量类型不符
   {ut::msg::MSG_OSM_BAD_VAR_TYPE,
    "osm_traffic: Unexpected variable type.",
    "osm_traffic: 变量类型不符。",
    "脚本变量实际类型与声明/期望类型不一致",
    "对照 Note 中变量名与期望类型，修改变量声明或赋值"},
   // osm_traffic 网络未定义
   {ut::msg::MSG_OSM_UNDEF_NET,
    "osm_traffic: Undefined network.",
    "osm_traffic: 网络未定义。",
    "osm_traffic 引用的 route network 未定义",
    "核对 Note 中网络名，补充对应网络定义"},
   // osm_traffic 网络有出口无入口
   {ut::msg::MSG_OSM_NET_NO_ENTRANCE,
    "osm_traffic: Network contains exits, but no acceptable entrances. Vehicles cannot be respawned after leaving the network.",
    "osm_traffic: 网络包含出口但没有可用入口，车辆驶离网络后无法重新生成。",
    "网络的生成节点均不满足入口条件",
    "检查网络节点/路段定义，保证至少有一个可生成车辆的入口"},
   // osm_traffic 初始化失败
   {ut::msg::MSG_OSM_INIT_FAIL,
    "osm_traffic: Unable to initialize.",
    "osm_traffic: 无法初始化。",
    "网络有效性检查未通过（见前置错误信息）",
    "查看本错误之前的日志，修正对应的网络配置问题"},
   // osm_traffic 无效平台类型
   {ut::msg::MSG_OSM_BAD_PLATFORM_TYPE,
    "osm_traffic: Invalid platform type.",
    "osm_traffic: 无效的平台类型。",
    "克隆车辆平台类型失败（未定义或克隆异常）",
    "核对 Note 中类型名，补充对应 platform_type"},
   // osm_traffic 车辆类型不存在
   {ut::msg::MSG_OSM_NO_VEHICLE_TYPE,
    "osm_traffic: Vehicle type does not exist.",
    "osm_traffic: 车辆类型不存在。",
    "网络定义的车辆 platform_type 未在想定中定义",
    "核对 Notes 中类型名，补充对应 platform_type"},
   // osm_traffic fraction 和不为 1
   {ut::msg::MSG_OSM_BAD_FRACTION,
    "osm_traffic: The sum of all the vehicle 'fraction' values must be 1.0.",
    "osm_traffic: 所有车辆 'fraction' 值之和必须为 1.0。",
    "车辆比例之和不为 1.0",
    "按 Note 中当前总和调整各车辆类型 fraction"},
   // osm_traffic 车队车辆类型不存在
   {ut::msg::MSG_OSM_NO_CONVOY_TYPE,
    "osm_traffic: Convoy vehicle type does not exist.",
    "osm_traffic: 车队车辆类型不存在。",
    "车队定义的车辆 platform_type 未定义",
    "核对 Notes 中类型名，补充对应 platform_type"},
   // osm_traffic 运动体为空
   {ut::msg::MSG_OSM_NULL_MOVER,
    "osm_traffic: Cannot set the mover's path because the mover is null.",
    "osm_traffic: 运动体为空，无法设置路径。",
    "生成的平台没有配置 mover",
    "查看 Note 中平台名，为该平台类型补充 mover 定义"},
   // 所属平台缺有效运动体
   {ut::msg::MSG_OSM_NO_MOVER,
    "Owner Platform doesn't have a valid mover in XWsfOSM_Traffic::VehiclePathEndedHandler()",
    "所属平台在 XWsfOSM_Traffic::VehiclePathEndedHandler() 中没有有效运动体。",
    "车辆路径结束回调时平台 mover 为空",
    "检查该平台类型的 mover 配置"},
   // road_traffic 网络未定义
   {ut::msg::MSG_ROAD_UNDEF_NET,
    "road_traffic: Undefined network.",
    "road_traffic: 网络未定义。",
    "road_traffic 引用的 route network 未定义",
    "核对 Note 中网络名，补充对应网络定义"},
   // road_traffic 车队运动体类型无效
   {ut::msg::MSG_ROAD_BAD_MOVER,
    "road_traffic: Could not initialize convoy vehicle because an invalid mover type was used.",
    "road_traffic: 使用了无效的运动体类型，无法初始化车队车辆。",
    "车队车辆平台未使用 WSF_ROAD_MOVER 或 WSF_GROUND_MOVER",
    "按 Note 提示改用 WSF_ROAD_MOVER 或 WSF_GROUND_MOVER"},
   // road_traffic 初始化失败
   {ut::msg::MSG_ROAD_INIT_FAIL,
    "road_traffic: Unable to initialize.",
    "road_traffic: 无法初始化。",
    "网络/车辆配置校验未通过（见前置错误信息）",
    "查看本错误之前的日志，修正对应的配置问题"},
   // road_traffic 无效平台类型
   {ut::msg::MSG_ROAD_BAD_PLATFORM_TYPE,
    "road_traffic: Invalid platform type.",
    "road_traffic: 无效的平台类型。",
    "克隆车辆平台类型失败（未定义或克隆异常）",
    "核对 Note 中类型名，补充对应 platform_type"},
   // road_traffic 运动体为空
   {ut::msg::MSG_ROAD_NULL_MOVER,
    "road_traffic: Cannot set the mover's path because the mover is null.",
    "road_traffic: 运动体为空，无法设置路径。",
    "生成的平台没有配置 mover",
    "查看 Note 中平台名，为该平台类型补充 mover 定义"},
   // road_traffic 静止车辆随机航点失败
   {ut::msg::MSG_ROAD_NO_RANDOM_WPT,
    "road_traffic: Unable to create a random waypoint for a stationary vehicle.",
    "road_traffic: 无法为静止车辆创建随机航点。",
    "静止车辆在路网中找不到可用的随机航点",
    "查看 Note 中车辆名，检查路网节点/车道定义"},
   // road_traffic 随机路径生成失败
   {ut::msg::MSG_ROAD_NO_RANDOM_PATH,
    "road_traffic: Unable to create a random path.",
    "road_traffic: 无法创建随机路径。",
    "车辆随机路径生成失败（路网连通性问题）",
    "查看 Note 中车辆名，检查路网连通性"},
   // road_traffic 无法创建新车辆
   {ut::msg::MSG_ROAD_NO_NEW_VEHICLE,
    "road_traffic: Could not create new vehicle.",
    "road_traffic: 无法创建新车辆。",
    "生成替代车辆时失败（平台类型克隆失败等）",
    "检查车辆平台类型定义与网络配置"},
   // road_traffic 车辆类型不存在
   {ut::msg::MSG_ROAD_NO_VEHICLE_TYPE,
    "road_traffic: Vehicle type does not exist.",
    "road_traffic: 车辆类型不存在。",
    "网络定义的车辆 platform_type 未定义",
    "核对 Notes 中类型名，补充对应 platform_type"},
   // road_traffic 车队车辆类型不存在
   {ut::msg::MSG_ROAD_NO_CONVOY_TYPE,
    "road_traffic: Convoy vehicle type does not exist.",
    "road_traffic: 车队车辆类型不存在。",
    "车队定义的车辆 platform_type 未定义",
    "核对 Notes 中类型名，补充对应 platform_type"},

   // ---- 传感器段（0x0003xxxx）----
   // MTI 调整表文件打开失败（WsfMTI_TableManager::ReadCsv）
   {ut::msg::MSG_SENSOR_MTI_OPEN_FAIL,
    "MTI_Table::ReadCsv: Unable to open MTI table file.",
    "MTI 表读取失败：无法打开 MTI 表文件：{file}。",
    "文件路径不存在、无读取权限，或被其他程序占用",
    "核对想定中 mti_adjustment_table 指定的 CSV 路径；相对路径相对想定文件解析"},
   // MTI 表数值列非单调递增（数据校验失败）
   {ut::msg::MSG_SENSOR_MTI_NOT_ASCENDING,
    "MTI_Table::ReadCsv MTI_AdjustmentTable2: Values must be monotonically ascending.",
    "MTI 表读取失败：MTI_AdjustmentTable2 的数值必须单调递增。",
    "表第 3 列（闭合速率）出现小于前一行的值，查表依赖升序保证插值正确",
    "检查 CSV 第 3 列数值是否按升序排列，删除重复或乱序行"},
   // MTI 表行解析失败（列数不足或格式非法）
   {ut::msg::MSG_SENSOR_MTI_ROW_FAIL,
    "MTI_Table::ReadCsv: Failed reading row.",
    "MTI 表读取失败：行解析失败：行号 {row}，行内值序 {value}。",
    "该行的字段数或数值格式不符合表结构要求",
    "按注释格式检查该行：每行应包含规定数量的数值字段，逗号分隔"},

   // clutter_model 意外克隆（WsfRadarSensor，两处调用点共用）
   {ut::msg::MSG_SENSOR_RADAR_CLUTTER_CLONE,
    "Unexpected clone of clutter_model.",
    "clutter_model 出现意外的克隆操作。",
    "派生传感器的 Clone 流程重复克隆杂波模型",
    "属框架内部缺陷，请记录想定与复现步骤反馈开发方"},
   // clutter_model 未定义
   {ut::msg::MSG_SENSOR_RADAR_CLUTTER_UNDEF,
    "Undefined 'clutter_model'.",
    "clutter_model 未定义（详见附注 Model 名）。",
    "clutter_model 指定的杂波模型类型未注册",
    "核对杂波模型类型名拼写，或先加载对应插件"},
   // clutter_model 初始化失败
   {ut::msg::MSG_SENSOR_RADAR_CLUTTER_INIT,
    "Failed to initialize 'clutter_model'.",
    "clutter_model 初始化失败。",
    "杂波模型初始化参数缺失或不合法",
    "检查杂波模型的参数配置是否满足其初始化要求"},
   // on_success 模式名未知
   {ut::msg::MSG_SENSOR_MODE_BAD_ON_SUCCESS,
    "Unknown mode name for 'on_success'.",
    "on_success 引用了未知的传感器模式名（详见附注 Mode 名）。",
    "on_success 指定的模式名未在传感器中定义",
    "核对 on_success 的模式名与传感器模式定义"},
   // on_failure 模式名未知
   {ut::msg::MSG_SENSOR_MODE_BAD_ON_FAILURE,
    "Unknown mode name for 'on_failure'.",
    "on_failure 引用了未知的传感器模式名（详见附注 Mode 名）。",
    "on_failure 指定的模式名未在传感器中定义",
    "核对 on_failure 的模式名与传感器模式定义"},
   // 频带低频大于高频
   {ut::msg::MSG_SENSOR_PASSIVE_BAD_FREQ_BAND,
    "Passive sensor cannot set frequency band.",
    "无源传感器无法设置频带：低频大于高频（详见附注 Low/High 值）。",
    "frequency_band 的下限值大于上限值",
    "修正 frequency_band 的取值，使低频小于高频"},
   // 无效传感器模式
   {ut::msg::MSG_SENSOR_PASSIVE_BAD_MODE,
    "Invalid mode set for passive sensor.",
    "为无源传感器设置了无效模式（详见附注 Mode 名）。",
    "SetMode 引用的模式名在传感器中不存在",
    "核对脚本/任务中的模式名与传感器模式定义"},
   // 无效波束索引
   {ut::msg::MSG_SENSOR_PASSIVE_BAD_BEAM_IDX,
    "Invalid beam index used to request the number of frequency bands.",
    "请求频带数时使用了无效的波束索引（须 1 <= N <= 波束数）。",
    "波束索引超出传感器波束数量范围",
    "检查脚本中的波束索引取值"},
   // 未定义频带
   {ut::msg::MSG_SENSOR_PASSIVE_NO_BANDS,
    "No frequency bands defined for passive sensor.",
    "无源传感器未定义任何频带。",
    "缺少 frequency_band 定义",
    "在传感器定义中补充至少一个 frequency_band"},
   // dwell/revisit 须全频带指定（4 处调用点共用）
   {ut::msg::MSG_SENSOR_PASSIVE_DWELL_ALL,
    "Must specify 'dwell_time' and 'revisit_time' on all bands if used on any band.",
    "dwell_time 与 revisit_time 一旦在某频带使用，就必须在所有频带上指定。",
    "部分频带定义了 dwell/revisit 而其他频带没有",
    "为全部频带统一补充 dwell_time 与 revisit_time，或全部删除"},
   // dwell/revisit 须成对指定（4 处调用点共用）
   {ut::msg::MSG_SENSOR_PASSIVE_DWELL_PAIR,
    "Must specify both 'dwell_time' and 'revisit_time' if either is used.",
    "dwell_time 与 revisit_time 必须成对指定。",
    "只指定了 dwell_time 或 revisit_time 之一",
    "成对提供 dwell_time 与 revisit_time，或全部删除"},
   // 频带返回失败（PSOS 模式）
   {ut::msg::MSG_SENSOR_PASSIVE_RET_PSOS,
    "Band could not be returned.",
    "频带无法归还（PSOS 模式）：归还时 dwell 与 revisit 时间必须非零。",
    "PSOS 模式下归还频带时 dwell/revisit 为零",
    "归还频带前确保两项时间均为非零值"},
   // 频带返回失败（非 PSOS）
   {ut::msg::MSG_SENSOR_PASSIVE_RET_NO_PSOS,
    "Band could not be returned.",
    "频带无法归还（非 PSOS 模式）：归还时 dwell 与 revisit 时间必须为零。",
    "非 PSOS 模式下归还频带时 dwell/revisit 非零",
    "归还频带前确保两项时间均为零"},
   // 频带不存在
   {ut::msg::MSG_SENSOR_PASSIVE_BAND_NO_EXIST,
    "Frequency band does not exist in passive sensor.",
    "无源传感器中不存在指定频带。",
    "引用的频带索引/标识不在传感器定义范围内",
    "核对频带定义与引用值"},
   // 频带添加失败（PSOS 模式）
   {ut::msg::MSG_SENSOR_PASSIVE_ADD_PSOS,
    "New passive sensor band could not be added.",
    "无法添加新的传感器频带（PSOS 模式）：添加时 dwell 与 revisit 时间必须非零。",
    "PSOS 模式下添加频带时 dwell/revisit 为零",
    "添加频带前确保两项时间均为非零值"},
   // 频带添加失败（非 PSOS）
   {ut::msg::MSG_SENSOR_PASSIVE_ADD_NO_PSOS,
    "New passive sensor band could not be added.",
    "无法添加新的传感器频带（非 PSOS 模式）：添加时 dwell 与 revisit 时间必须为零。",
    "非 PSOS 模式下添加频带时 dwell/revisit 非零",
    "添加频带前确保两项时间均为零"},
   // frame_time 未指定
   {ut::msg::MSG_SENSOR_PASSIVE_NO_FRAME_TIME,
    "Unspecified frame_time.",
    "未指定 frame_time（帧时间）。",
    "需要帧时间参数但未提供",
    "在传感器定义中补充 frame_time 值"},
   // 无效波束号
   {ut::msg::MSG_SENSOR_PASSIVE_BAD_BEAM_NUM,
    "Invalid beam number.",
    "无效波束号（详见附注 Beam 值）。",
    "波束号超出传感器波束数量范围",
    "检查脚本/任务中的波束号取值"},
   // 波束导向器模式缺 type 指定（WsfBeamDirector 模式定义）
   {ut::msg::MSG_BD_NO_TYPE,
    "Must specify a type for this mode.",
    "波束导向器模式必须指定 type。",
    "波束导向器模式定义缺少 type 命令",
    "在波束导向器模式定义中增加有效的 type 指定"},
   // 双基地误差模型方位角标准差超限
   {ut::msg::MSG_BISTATIC_AZ_SIGMA,
    "Azimuth standard deviation greater than maximum allowed value",
    "双基地误差模型的方位角标准差超过允许的最大值。",
    "initial_sigma 指定的方位角标准差大于角度上限",
    "减小方位角标准差取值"},
   // 双基地误差模型俯仰角标准差超限
   {ut::msg::MSG_BISTATIC_EL_SIGMA,
    "Elevation standard deviation greater than maximum allowed value",
    "双基地误差模型的俯仰角标准差超过允许的最大值。",
    "initial_sigma 指定的俯仰角标准差大于角度上限",
    "减小俯仰角标准差取值"},
   // 双基地误差表 domain 无法识别
   {ut::msg::MSG_BISTATIC_BAD_DOMAIN,
    "SensorMode::BistaticTable Domain not recognized.",
    "双基地误差表无法识别指定的 domain。",
    "bistatic_table 的 domain 取值非法",
    "核对 bistatic_table 的 domain 取值"},
   // 激光指示器缺 laser_designations 扩展
   {ut::msg::MSG_LASER_DESIG_NO_EXT,
    "Extension 'laser_designations' not available.  Cannot initialize WSF_LASER_DESIGNATOR.",
    "缺少 laser_designations 扩展，无法初始化 WSF_LASER_DESIGNATOR。",
    "想定未启用激光指示扩展",
    "确认想定中已启用 laser_designations 扩展"},
   // 激光指示器模式传感器类型错误
   {ut::msg::MSG_LASER_DESIG_BAD_SENSOR,
    "WsfLaserDesignator LaserDesignationMode not provided with proper sensor type.",
    "激光指示器模式未关联正确的传感器类型。",
    "LaserDesignationMode 关联的传感器类型不匹配",
    "核对模式关联的传感器类型为激光指示器"},
   // 激光跟踪器缺 laser_designations 扩展
   {ut::msg::MSG_LASER_TRACKER_NO_EXT,
    "Could not find 'laser_designations'. Required for WSF_LASER_TRACKER.",
    "找不到 laser_designations 扩展，WSF_LASER_TRACKER 必需该扩展。",
    "想定未启用激光指示扩展",
    "确认想定中已启用 laser_designations 扩展"},
   // 误差模型不适用于表面波雷达
   {ut::msg::MSG_SW_RADAR_EM_BAD_SENSOR,
    "Specified 'error_model' not able to be used on sensor types that are not WSF_SURFACE_WAVE_RADAR_SENSOR.",
    "该误差模型只能用于 WSF_SURFACE_WAVE_RADAR_SENSOR 类型传感器。",
    "error_model 与传感器类型不匹配",
    "更换误差模型或改用表面波雷达传感器"},
   // 误差模型不适用于超视距雷达
   {ut::msg::MSG_OTH_EM_BAD_SENSOR,
    "error_model: Not able to be used on sensor types that are not WSF_OTH_RADAR_SENSOR.",
    "该误差模型只能用于 WSF_OTH_RADAR_SENSOR 类型传感器。",
    "error_model 与传感器类型不匹配",
    "更换误差模型或改用超视距雷达传感器"},
   // EOIR 传感器缺 frame_time
   {ut::msg::MSG_EOIR_NO_FRAME_TIME,
    "'frame_time' not specified.",
    "EOIR 传感器未指定 frame_time。",
    "模式定义中缺少 frame_time 命令",
    "增加 frame_time 指定"},
   // EOIR 缺角分辨率或像素数
   {ut::msg::MSG_EOIR_RES_OR_PIXEL,
    "Either 'angular_resolution' or 'pixel_count' must be specified.",
    "必须指定 angular_resolution 或 pixel_count 之一。",
    "成像参数缺失导致无法计算角分辨率",
    "二者至少指定其一"},
   // EOIR 角分辨率过小
   {ut::msg::MSG_EOIR_RES_TOO_SMALL,
    "'angular_resolution' too small. Please increase.",
    "angular_resolution 过小，请增大取值。",
    "角分辨率小于允许的最小值",
    "增大 angular_resolution 取值"},
   // SAR 未指定 resolution
   {ut::msg::MSG_SAR_NO_RESOLUTION,
    "'resolution' must be specified if 'dwell_time' is not specified.",
    "未指定 dwell_time 时必须指定 resolution。",
    "SAR 模式缺少成像分辨率与驻留时间",
    "为 SAR 模式增加 resolution 指定"},
   // SAR 未指定 dwell_time
   {ut::msg::MSG_SAR_NO_DWELL_TIME,
    "'dwell_time' must be specified if 'resolution' is not specified.",
    "未指定 resolution 时必须指定 dwell_time。",
    "SAR 模式缺少成像分辨率与驻留时间",
    "为 SAR 模式增加 dwell_time 指定"},
   // SAR 误差统计未初始化
   {ut::msg::MSG_SAR_ERR_STATS_NO_INIT,
    "SAR Sensor Error Statistics were NOT initialized.",
    "SAR 传感器误差统计未初始化。",
    "误差统计表缺失或加载失败",
    "核对 SAR 误差统计配置"},
   // LADAR 发射机缺 PRF
   {ut::msg::MSG_LADAR_NO_PRF,
    "Either transmitter 'pulse_repetition_frequency' or 'pulse_repetition_interval' must be specified.",
    "发射机必须指定 pulse_repetition_frequency 或 pulse_repetition_interval 之一。",
    "LADAR 发射机缺少脉冲重复参数",
    "为发射机增加二者之一"},
   // LADAR 缺检测阈值
   {ut::msg::MSG_LADAR_NO_THRESH,
    "'detection_threshold' or 'detection_probability' must be specified.",
    "必须指定 detection_threshold 或 detection_probability 之一。",
    "LADAR 模式缺少检测判据",
    "二者至少指定其一"},
   // EOIR/IRST 模式未定义频带
   {ut::msg::MSG_EOIR_MODE_NO_BAND,
    "Band not defined.",
    "EOIR/IRST 传感器模式未定义频带。",
    "模式定义中缺少频带（波长范围）",
    "为模式增加频带定义"},
   // EOIR/IRST 缺噪声等效辐照度
   {ut::msg::MSG_EOIR_MODE_NO_NEI,
    "'noise_equivalent_irradiance' not defined.",
    "EOIR/IRST 模式未定义 noise_equivalent_irradiance。",
    "红外探测灵敏度参数缺失",
    "增加 noise_equivalent_irradiance 指定"},
   // EOIR/IRST 缺检测阈值
   {ut::msg::MSG_EOIR_MODE_NO_THRESH,
    "'detection_threshold' not defined.",
    "EOIR/IRST 模式未定义 detection_threshold。",
    "模式定义中缺少检测阈值",
    "增加 detection_threshold 指定"},
   // EOIR/IRST 衰减模型初始化失败
   {ut::msg::MSG_EOIR_MODE_ATTEN_INIT,
    "Failed to initialize 'attenuation_model'.",
    "attenuation_model 初始化失败。",
    "衰减模型类型或参数无效",
    "核对衰减模型类型与参数"},
   // OTH clutter_model 意外克隆
   {ut::msg::MSG_OTH_CLUTTER_CLONE,
    "Unexpected clone of clutter_model.",
    "OTH 雷达不允许克隆 clutter_model。",
    "OTH 雷达的杂波模型不支持克隆复制",
    "检查 OTH 雷达 clutter_model 配置"},
   // OTH clutter_model 未定义
   {ut::msg::MSG_OTH_CLUTTER_UNDEF,
    "clutter_model has not been defined.",
    "clutter_model 尚未定义。",
    "引用杂波模型时尚未定义",
    "先定义 clutter_model 再引用"},
   // OTH clutter_model 初始化失败
   {ut::msg::MSG_OTH_CLUTTER_INIT,
    "Failed to initialize clutter_model.",
    "clutter_model 初始化失败。",
    "杂波模型类型或参数无效",
    "核对杂波模型类型与参数"},
   // OTH mti_processor 缺 PRF
   {ut::msg::MSG_OTH_MTI_NO_PRF,
    "'pulse_repetition_frequency' must be specified with 'mti_processor'.",
    "使用 mti_processor 时必须指定 pulse_repetition_frequency。",
    "MTI 处理器缺少脉冲重复频率",
    "增加 pulse_repetition_frequency 指定"},
   // OTH mti_processor 缺 maximum_range
   {ut::msg::MSG_OTH_MTI_NO_MAX_RANGE,
    "'maximum_range' must be specified in 'mti_processor'.",
    "mti_processor 中必须指定 maximum_range。",
    "MTI 处理器缺少最大作用距离",
    "增加 maximum_range 指定"},
   // OTH mti_processor 脉间周期须成对
   {ut::msg::MSG_OTH_MTI_NO_IPP,
    "'interpulse_period_1 and 2' must both be specified in 'mti_processor'.",
    "mti_processor 中必须同时指定 interpulse_period_1 与 interpulse_period_2。",
    "MTI 处理器脉间周期未成对指定",
    "补齐两个脉间周期参数"},
   // OTH MTI 修正表重复指定
   {ut::msg::MSG_OTH_MTI_DUP_TABLE,
    "WsfOTH_RadarSensor::RadarBeam::ProcessInput: An MTI adjustment table has already been specified.",
    "MTI 修正表已存在，不允许重复指定。",
    "同一波束内重复定义 MTI adjustment table",
    "移除重复的 MTI adjustment table 定义"},
   // 外部传感器初始化失败
   {ut::msg::MSG_EXT_SENSOR_INIT_FAIL,
    "Initialization failed for external sensor.",
    "外部传感器初始化失败。",
    "外部辐射源对应传感器初始化失败",
    "检查传感器类型定义与初始化输入"},

   // ---- 传感器段（0x0003xxxx）sensor 域（第十九批）----
   // 组合传感器缺 operating_mode
   {ut::msg::MSG_CS_NO_OPERATING_MODE,
    "'operating_mode' has not been defined.",
    "未定义 'operating_mode'。",
    "组合传感器未指定工作模式",
    "在组合传感器定义中补充 operating_mode 命令"},
   // 组合传感器未定义成员传感器
   {ut::msg::MSG_CS_NO_CONSTITUENTS,
    "No constituent sensors defined.",
    "未定义成员传感器。",
    "组合传感器未包含任何成员传感器",
    "在定义中补充构成组合传感器的成员传感器名"},
   // 成员传感器与组合传感器同名
   {ut::msg::MSG_CS_SELF_CONSTITUENT,
    "Constituent sensor cannot be the same as the composite sensor.",
    "成员传感器不能是组合传感器自身。",
    "成员列表中引用了组合传感器本身",
    "核对 Note 中传感器名，移除自引用条目"},
   // 成员传感器未定义
   {ut::msg::MSG_CS_CONSTITUENT_UNDEF,
    "Constituent sensor was not defined",
    "成员传感器未定义。",
    "成员列表引用的传感器在平台上不存在",
    "核对 Note 中传感器名，确保成员传感器已在平台定义"},
   // 成员传感器须有模式列表
   {ut::msg::MSG_CS_CONSTITUENT_NO_MODES,
    "Constituent sensor must have a mode list.",
    "成员传感器必须拥有模式列表。",
    "参与模式共享的成员传感器未定义模式列表",
    "核对 Note 中传感器名，为其补充模式定义"},
   // 成员传感器须有调度器与跟踪器
   {ut::msg::MSG_CS_CONSTITUENT_NO_SCHED,
    "The constituent sensor must have a scheduler and a tracker.",
    "成员传感器必须拥有调度器与跟踪器。",
    "首个成员传感器缺少 scheduler 或 tracker",
    "核对 Note 中传感器名，补齐其调度器/跟踪器配置"},
   // 成员传感器内部链路添加失败
   {ut::msg::MSG_CS_INTERNAL_LINK_FAIL,
    "Failed to add internal link for sensor.",
    "成员传感器内部链路添加失败。",
    "向组合传感器添加内部链路接收者时失败（通常为重复添加）",
    "核对 Note 中传感器名，检查是否重复加入组合"},
   // MarcumSwerling case 无效
   {ut::msg::MSG_MS_BAD_CASE,
    "MarcumSwerling: Invalid case.",
    "MarcumSwerling: 无效的 case。",
    "MarcumSwerling 检测模型 case 取值超出 0-4 范围",
    "按 Notes 中期望值修正 case 取值"},
   // MarcumSwerling 脉冲积分数无效
   {ut::msg::MSG_MS_BAD_NUM_PULSES,
    "MarcumSwerling:: Invalid number of pulses integrated.",
    "MarcumSwerling: 无效的脉冲积分数。",
    "脉冲积分数小于等于零",
    "按 Notes 中期望值修正脉冲积分数"},
   // MarcumSwerling 虚警概率无效
   {ut::msg::MSG_MS_BAD_PFA,
    "MarcumSwerling: Invalid probability of false alarm.",
    "MarcumSwerling: 无效的虚警概率。",
    "虚警概率不在 (0,1) 开区间内",
    "按 Notes 中期望值修正虚警概率"},
   // 连续波检测阈值未定义
   {ut::msg::MSG_PD_NO_CONT_THRESH,
    "Continuous detection threshold not defined.",
    "未定义连续波检测阈值。",
    "无源探测器连续波信号的检测阈值表缺失或为空",
    "补充 continuous 信号的 detection_threshold 定义"},
   // 脉冲检测阈值未定义
   {ut::msg::MSG_PD_NO_PULSED_THRESH,
    "Pulsed detection threshold not defined.",
    "未定义脉冲检测阈值。",
    "无源探测器脉冲信号的检测阈值表缺失或为空",
    "补充 pulsed 信号的 detection_threshold 定义"},
   // 连续波灵敏度未定义
   {ut::msg::MSG_PD_NO_CONT_SENS,
    "Continuous sensitivity not defined.",
    "未定义连续波灵敏度。",
    "无源探测器连续波信号的灵敏度表缺失或为空",
    "补充 continuous 信号的 detection_sensitivity 定义"},
   // 脉冲灵敏度未定义
   {ut::msg::MSG_PD_NO_PULSED_SENS,
    "Pulsed sensitivity not defined.",
    "未定义脉冲灵敏度。",
    "无源探测器脉冲信号的灵敏度表缺失或为空",
    "补充 pulsed 信号的 detection_sensitivity 定义"},
   // 须定义检测阈值或灵敏度
   {ut::msg::MSG_PD_NO_THRESH_OR_SENS,
    "Must define 'detection_threshold' or 'detection_sensitivity'.",
    "必须定义 'detection_threshold' 或 'detection_sensitivity'。",
    "无源探测器既未定义检测阈值也未定义灵敏度",
    "按信号类型补充 detection_threshold 或 detection_sensitivity"},
   // data_table 型 MTI 缺数据表
   {ut::msg::MSG_MTI_ADJ_NO_TABLE,
    "Must specify 'mti_data_table' and valid data for MTI type of 'data_table'.",
    "MTI 类型为 'data_table' 时必须指定 'mti_data_table' 及有效数据。",
    "data_table 型 MTI 处理器未提供修正数据表",
    "补充 mti_data_table 命令及有效的表数据"},
   // MTI 处理器要求雷达模式
   {ut::msg::MSG_MTI_ADJ_BAD_MODE,
    "Sensor Mode must be of type WsfRadarSensor:RadarMode.",
    "传感器模式必须为 WsfRadarSensor:RadarMode 类型。",
    "MTI 处理器被用于非雷达模式",
    "将 mti_processor 移到雷达传感器模式定义中"},
   // MTI 处理器缺 PRF
   {ut::msg::MSG_MTI_ADJ_NO_PRF,
    "Must specify 'pulse_repetition_frequency' with 'mti_processor'.",
    "使用 'mti_processor' 时必须指定 'pulse_repetition_frequency'。",
    "mti_processor 对应波束的发射机未指定 PRF",
    "为该波束的 transmitter 补充 pulse_repetition_frequency"},
   // MTI 处理器缺最大距离
   {ut::msg::MSG_MTI_ADJ_NO_MAX_RANGE,
    "Must specify 'maximum_range' in 'mti_processor'",
    "必须在 'mti_processor' 中指定 'maximum_range'。",
    "MTI 处理器未指定最大探测距离",
    "在 mti_processor 块内补充 maximum_range"},
   // MTI 处理器脉间周期须成对
   {ut::msg::MSG_MTI_ADJ_NO_IPP,
    "Must specify both 'interpulse_period_1 and 2' in 'mti_processor'",
    "必须在 'mti_processor' 中同时指定 'interpulse_period_1 和 2'。",
    "双延迟对消器需要两个脉间周期，仅指定了其中一个",
    "在 mti_processor 块内成对补充 interpulse_period_1/2（或只指定 PRF）"},
   // 默认跟踪器要求模式列表
   {ut::msg::MSG_DST_NO_MODE_LIST,
    "A mode list is required for WsfDefaultSensorTracker.",
    "WsfDefaultSensorTracker 需要模式列表。",
    "使用默认跟踪器的传感器未定义模式列表",
    "为传感器补充至少一个模式定义"},
   // MTI 表 CSV 读取异常
   {ut::msg::MSG_MTI_TABLE_READCSV_EXC,
    "MTI_Table::ReadCsv: Caught exception!!!",
    "MTI_Table::ReadCsv: 捕获到异常。",
    "CSV 表读取过程中抛出标准异常",
    "按 Note 中行号检查 CSV 数据格式"},
   // 复合 MTI 表文件打开失败
   {ut::msg::MSG_MTI_TABLE_COMPOUND_OPEN,
    "MTI_Table::ReadCsvCompound: Unable to open MTI table file.",
    "MTI_Table::ReadCsvCompound: 无法打开 MTI 表文件。",
    "复合 MTI 修正表文件无法打开",
    "核对 Note 中文件路径，确认文件存在且可读"},
   // 复合 MTI 表解析异常
   {ut::msg::MSG_MTI_TABLE_COMPOUND_PARSE,
    "Exception thrown in MTI_Table::ReadCsvCompound. Error parsing table.",
    "MTI_Table::ReadCsvCompound 中抛出异常：表解析出错。",
    "复合表单元格解析失败",
    "查看 Notes 中异常信息，检查表数据格式"},
   // 复合 MTI 表行读取失败
   {ut::msg::MSG_MTI_TABLE_COMPOUND_ROW,
    "MTI_TABLE::ReadCsvCompound: Failed reading row.",
    "MTI_TABLE::ReadCsvCompound: 行读取失败。",
    "行内数值列数与轴定义不符",
    "按 Notes 中行号与列数检查该行数据"},
   // 复合 MTI 表读取异常
   {ut::msg::MSG_MTI_TABLE_COMPOUND_EXC,
    "Exception thrown in MTI_TABLE::ReadCsvCompound.",
    "MTI_TABLE::ReadCsvCompound 中抛出异常。",
    "复合表读取过程中抛出标准异常",
    "按 Note 中行号与异常信息检查 CSV 数据"},
   // MTI 表 CSV 写打开失败
   {ut::msg::MSG_MTI_TABLE_WRITECSV_OPEN,
    "MTI_TABLE::WriteCsv: Unable to open file.",
    "MTI_TABLE::WriteCsv: 无法打开文件。",
    "MTI 表 CSV 输出文件无法打开",
    "核对 Note 中文件路径与目录写权限"},
   // MTI 表二进制写打开失败
   {ut::msg::MSG_MTI_TABLE_WRITEBIN_OPEN,
    "MTI_TABLE::WriteBinary: Unable to open file.",
    "MTI_TABLE::WriteBinary: 无法打开文件。",
    "MTI 表二进制输出文件无法打开",
    "核对 Note 中文件路径与目录写权限"},
   // MTI 表二进制写 speedAxis 失败
   {ut::msg::MSG_MTI_TABLE_WRITEBIN_SPEED,
    "MTI_TABLE::WriteBinary: Writing data speedAxis.",
    "MTI_TABLE::WriteBinary: 写入 speedAxis 数据失败。",
    "二进制写出速度轴数据时流写入失败",
    "检查磁盘空间与文件是否被占用"},
   // MTI 表二进制写 adjustmentAxis 失败
   {ut::msg::MSG_MTI_TABLE_WRITEBIN_ADJ,
    "MTI_TABLE::WriteBinary: Writing data adjustmentAxis.",
    "MTI_TABLE::WriteBinary: 写入 adjustmentAxis 数据失败。",
    "二进制写出修正量轴数据时流写入失败",
    "检查磁盘空间与文件是否被占用"},
   // MTI 表二进制读打开失败
   {ut::msg::MSG_MTI_TABLE_READBIN_OPEN,
    "MTI_TABLE::ReadBinary: Unable to open file.",
    "MTI_TABLE::ReadBinary: 无法打开文件。",
    "MTI 表二进制输入文件无法打开",
    "核对 Note 中文件路径，确认文件存在且可读"},
   // MTI 表二进制读 speedAxis 失败
   {ut::msg::MSG_MTI_TABLE_READBIN_SPEED,
    "MTI_TABLE::ReadBinary: Reading data speedAxis.",
    "MTI_TABLE::ReadBinary: 读取 speedAxis 数据失败。",
    "二进制读取速度轴数据时流读取失败",
    "确认文件为 WriteBinary 生成的有效格式"},
   // MTI 表二进制读 adjustmentAxis 失败
   {ut::msg::MSG_MTI_TABLE_READBIN_ADJ,
    "MTI_TABLE::ReadBinary: Reading data adjustmentAxis.",
    "MTI_TABLE::ReadBinary: 读取 adjustmentAxis 数据失败。",
    "二进制读取修正量轴数据时流读取失败",
    "确认文件为 WriteBinary 生成的有效格式"},
   // 找不到频率对应的修正表
   {ut::msg::MSG_MTI_TABLE_NO_FREQ,
    "MTI_AdjustmentTableCompound: No MTI adjustment table was found for frequency.",
    "MTI_AdjustmentTableCompound: 找不到该频率对应的 MTI 修正表。",
    "复合表中没有覆盖指定频率的修正表",
    "核对 Note 中频率值，扩展修正表的频率覆盖范围"},
   // 默认调度器要求模式列表
   {ut::msg::MSG_DSS_NO_MODE_LIST,
    "A mode list is required for WsfDefaultSensorScheduler.",
    "WsfDefaultSensorScheduler 需要模式列表。",
    "使用默认调度器的传感器未定义模式列表",
    "为传感器补充至少一个模式定义"},
   // 默认调度器缺 frame_time
   {ut::msg::MSG_DSS_NO_FRAME_TIME,
    "WsfDefaultSensorScheduler: 'frame_time' not specified.",
    "WsfDefaultSensorScheduler: 未指定 'frame_time'。",
    "某个模式未指定帧时间",
    "按 Notes 中传感器与模式名补充 frame_time"},
   // 请求的传感器模式无效
   {ut::msg::MSG_DSS_BAD_MODE_REQ,
    "Invalid mode requested for sensor.",
    "请求的传感器模式无效。",
    "请求的模式名在传感器模式列表中不存在",
    "按 Notes 中传感器与请求的模式名核对模式定义"},
   // 物理扫描调度器要求模式列表
   {ut::msg::MSG_PSSS_NO_MODE_LIST,
    "A mode list is required for {type}.",
    "{type} 需要模式列表。",
    "使用物理扫描调度器的传感器未定义模式列表",
    "为传感器补充至少一个模式定义"},
   // 物理扫描调度器缺 update_interval
   {ut::msg::MSG_PSSS_NO_UPDATE_INT,
    "An 'update_interval' is required for {type} scheduler.",
    "{type} 调度器需要 'update_interval'。",
    "传感器未指定更新间隔",
    "核对 Note 中传感器名，补充 update_interval"},
   // 物理扫描调度器缺 frame_time
   {ut::msg::MSG_PSSS_NO_FRAME_TIME,
    "Unspecified 'frame_time'.",
    "未指定 'frame_time'。",
    "某个模式未指定帧时间",
    "按 Notes 中传感器与模式名补充 frame_time"},
   // 误差模型仅适用雷达传感器
   {ut::msg::MSG_RSEM_NOT_RADAR,
    "Unable to use 'error_model' on sensor types that are note WSF_RADAR_SENSORs.",
    "无法在非 WSF_RADAR_SENSOR 类型传感器上使用 'error_model'。",
    "雷达误差模型被用于非雷达传感器",
    "核对 Note 中模型名，将 error_model 移到雷达传感器定义中"},
   // 扇区扫描缺方位扫描率
   {ut::msg::MSG_SSSS_NO_AZ_RATE,
    "You must define an 'azimuth_rate' or the sensor's 'azimuth_slew_rate'.",
    "必须定义 'azimuth_rate' 或传感器的 'azimuth_slew_rate'。",
    "扇区扫描模式缺方位扫描率且传感器未定义方位转动速率",
    "补充 azimuth_rate 或传感器的 azimuth_slew_rate"},
   // 扇区扫描缺俯仰扫描率
   {ut::msg::MSG_SSSS_NO_EL_RATE,
    "You must define an 'elevation_rate' or the sensor's 'elevation_slew_rate'.",
    "必须定义 'elevation_rate' 或传感器的 'elevation_slew_rate'。",
    "扇区扫描模式缺俯仰扫描率且传感器未定义俯仰转动速率",
    "补充 elevation_rate 或传感器的 elevation_slew_rate"},
   // 扇区扫描调度器缺 update_interval
   {ut::msg::MSG_SSSS_NO_UPDATE_INT,
    "'update_interval' is required for 'sector_scan' scheduler.",
    "'sector_scan' 调度器需要 'update_interval'。",
    "扇区扫描传感器未指定更新间隔",
    "按 Notes 中平台与传感器名补充 update_interval"},
   // 调度器块缺结束命令
   {ut::msg::MSG_SST_NO_END_SCHED,
    "Missing block terminator.",
    "缺少块结束命令。",
    "scheduler 块未以 end_scheduler 结束",
    "按 Note 提示补充 end_scheduler 结束命令"},
   // 信号处理器初始化失败
   {ut::msg::MSG_SSP_INIT_FAIL,
    "Initialization failed for 'signal_processor'.",
    "'signal_processor' 初始化失败。",
    "某个信号处理器初始化返回失败",
    "按 Note 中类型名查看该处理器的具体初始化错误"},
   // 旋转调度器要求模式列表
   {ut::msg::MSG_SPIN_NO_MODE_LIST,
    "A mode list is required for WsfSpinSensorScheduler",
    "WsfSpinSensorScheduler 需要模式列表。",
    "使用旋转扫描调度器的传感器未定义模式列表",
    "为传感器补充至少一个模式定义"},
   // 旋转调度器缺 frame_time
   {ut::msg::MSG_SPIN_NO_FRAME_TIME,
    "Sensor {sensor}, mode {mode}: frame_time not specified",
    "传感器 {sensor}，模式 {mode}: 未指定 frame_time。",
    "某个模式未指定帧时间",
    "为该模式补充 frame_time 定义"},
   // 无源探测器脉冲灵敏度未定义（原文无 d 变体）
   {ut::msg::MSG_PD_NO_PULSE_SENS,
    "Pulse sensitivity not defined.",
    "未定义脉冲灵敏度。",
    "无源探测器脉冲信号的灵敏度表缺失或为空",
    "补充 pulsed 信号的 detection_sensitivity 定义"},

   // ---- 交战/武器段（0x0004xxxx）----
   // 平台武器引用的 weapon_effects 类型未定义（WsfWeapon::Initialize）
   {ut::msg::MSG_WEAPON_EFFECTS_UNDEFINED,
    "Undefined weapon_effects for platform weapon.",
    "平台武器引用的 weapon_effects 未定义：平台 {platform}，武器 {weapon}，weapon_effects '{effects}'。",
    "武器定义中 weapon_effects 命令指定的类型未在想定中定义",
    "核对武器类型定义中 weapon_effects 取值，确保对应的 WsfWeaponEffects 类型已定义"},
   // 武器平台扩展初始化武器交战失败
   {ut::msg::MSG_WEAPON_ENGAGE_INIT_FAIL,
    "Weapon Platform Extension: Unable to initialize weapon engagement.",
    "武器平台扩展初始化武器交战失败：扩展 {extension}。",
    "交战对象初始化时依赖的资源未就绪或配置不完整",
    "检查平台扩展及其依赖的武器交战相关定义是否完整"},
   // 发射的平台类型未定义（WsfExplicitWeapon::Initialize）
   {ut::msg::MSG_WEAPON_LAUNCHED_TYPE_UNDEF,
    "Launched platform type is not defined.",
    "发射的平台类型未定义：平台 {platform}，武器 {weapon}，发射平台类型 '{type}'。",
    "武器定义中 launched_platform_type 指定的平台类型未在想定中定义",
    "确认该 platform_type 已在想定中定义且名称拼写一致"},
   // 发射计算机解缺失或不完整，发射被抑制
   {ut::msg::MSG_WEAPON_LAUNCH_SOL_MISSING,
    "Missing or incomplete launch computer solution - launch suppressed.",
    "发射计算机解缺失或不完整，已抑制发射：时刻 {t}，平台 {platform}，武器 {weapon}。",
    "发射计算机无法为当前目标状态算出完整解（如目标不在发射包线内）",
    "检查发射计算机参数与目标射程/速度是否匹配"},
   // 延迟发射的武器平台初始化失败
   {ut::msg::MSG_WEAPON_DEFERRED_INIT_FAIL,
    "Failed to initialize deferred launch weapon platform.",
    "延迟发射的武器平台初始化失败：时刻 {t}，平台 {platform}，武器 {weapon}。",
    "发射时刻创建的武器平台（导弹等）组件初始化失败",
    "检查发射平台类型中各组件（导引头/推进等）的初始化条件"},
   // 无法将武器加入仿真
   {ut::msg::MSG_WEAPON_ADD_TO_SIM_FAIL,
    "Unable to add weapon to the simulation.",
    "无法将武器加入仿真：平台 {platform}，武器 {weapon}，武器平台名 '{weapon_platform}'。",
    "仿真拒绝添加武器平台（如平台重名或状态非法）",
    "检查武器平台名称是否与现有平台冲突"},
   // 发射计算机收到空武器指针
   {ut::msg::MSG_LC_NULL_WEAPON,
    "Weapon pointer was null.",
    "发射计算机收到的武器指针为空。",
    "脚本或内部调用向发射计算机传入了空指针",
    "检查调用链确保武器对象已创建且有效"},
   // 三选二输入约束不满足（发射计算机）
   {ut::msg::MSG_LC_SPECIFY_TWO,
    "Must specify any 2 of the following:",
    "必须指定以下三项中的任意两项：maximum_time_of_flight / coast_duration / thrust_duration。",
    "发射计算机要求三项时长输入三选二，当前一项都未提供",
    "在发射计算机定义中补充其中至少两项输入"},
   // 计算出负推力持续时间
   {ut::msg::MSG_LC_NEG_THRUST_DURATION,
    "A negative thrust duration was calculated.",
    "计算得到的推力持续时间为负值。",
    "maximum_time_of_flight 与 coast_duration 组合推导出负推力时长（输入不自洽）",
    "核对三项时长的取值组合是否自洽"},
   // 计算出负滑行持续时间
   {ut::msg::MSG_LC_NEG_COAST_DURATION,
    "A negative coast duration was calculated.",
    "计算得到的滑行持续时间为负值。",
    "maximum_time_of_flight 与 thrust_duration 组合推导出负滑行时长（输入不自洽）",
    "核对三项时长的取值组合是否自洽"},
   // 未提供燃尽速度
   {ut::msg::MSG_LC_NO_BURNOUT_SPEED,
    "Burnout speed was not provided.",
    "未提供燃尽速度（burnout_speed）。",
    "发射计算机必需输入缺失",
    "在发射计算机定义中补充 burnout_speed"},
   // 未提供最小末速
   {ut::msg::MSG_LC_NO_MIN_TERMINAL_SPEED,
    "Minimum terminal speed was not provided.",
    "未提供最小末速（minimum_terminal_speed）。",
    "发射计算机必需输入缺失",
    "在发射计算机定义中补充 minimum_terminal_speed"},
   // ATA 发射计算机分箱数校验（WsfATA_LaunchComputer::ProcessInputInnerBlock）
   {ut::msg::MSG_ATA_LC_ASPECT_BIN_COUNT,
    "'aspect_angle_bin_count' must be 3 or greater.",
    "'aspect_angle_bin_count' 必须大于等于 3：当前值 {current}。",
    "ATA 发射计算机查表网格分箱数过少，无法完成插值",
    "将 aspect_angle_bin_count 增大到 3 或以上"},
   {ut::msg::MSG_ATA_LC_AZIMUTH_BIN_COUNT,
    "'azimuth_bin_count' must be 3 or greater.",
    "'azimuth_bin_count' 必须大于等于 3：当前值 {current}。",
    "ATA 发射计算机查表网格分箱数过少，无法完成插值",
    "将 azimuth_bin_count 增大到 3 或以上"},
   {ut::msg::MSG_ATA_LC_ELEVATION_BIN_COUNT,
    "'elevation_bin_count' must be 3 or greater.",
    "'elevation_bin_count' 必须大于等于 3：当前值 {current}。",
    "ATA 发射计算机查表网格分箱数过少，无法完成插值",
    "将 elevation_bin_count 增大到 3 或以上"},
   {ut::msg::MSG_ATA_LC_RANGE_RATE_BIN_COUNT,
    "'range_rate_bin_count' must be 3 or greater.",
    "'range_rate_bin_count' 必须大于等于 3：当前值 {current}。",
    "ATA 发射计算机查表网格分箱数过少，无法完成插值",
    "将 range_rate_bin_count 增大到 3 或以上"},
   {ut::msg::MSG_ATA_LC_RANGE_BIN_COUNT,
    "'range_bin_count' must be 3 or greater.",
    "'range_bin_count' 必须大于等于 3：当前值 {current}。",
    "ATA 发射计算机查表网格分箱数过少，无法完成插值",
    "将 range_bin_count 增大到 3 或以上"},
   // ATA 方位角限幅非正
   {ut::msg::MSG_ATA_LC_AZIMUTH_LIMIT,
    "'azimuth_limit' must be greater than zero.",
    "'azimuth_limit' 必须大于零：当前值 {current}。",
    "ATA 发射计算机方位角搜索范围配置非法",
    "将 azimuth_limit 设为正值"},
   // ATA 上下限大小关系颠倒（3 条）
   {ut::msg::MSG_ATA_LC_ELEVATION_LIMITS,
    "'elevation_upper_limit' must be set greater than 'elevation_lower_limit'.",
    "'elevation_upper_limit' 必须大于 'elevation_lower_limit'：上界 {upper}，下界 {lower}。",
    "俯仰角上下限大小关系颠倒",
    "调整 elevation_upper_limit 与 elevation_lower_limit 的大小关系"},
   {ut::msg::MSG_ATA_LC_RANGE_LIMITS,
    "'range_upper_limit' must be set greater than 'range_lower_limit'.",
    "'range_upper_limit' 必须大于 'range_lower_limit'：上界 {upper}，下界 {lower}。",
    "射程上下限大小关系颠倒",
    "调整 range_upper_limit 与 range_lower_limit 的大小关系"},
   {ut::msg::MSG_ATA_LC_RANGE_RATE_LIMITS,
    "'range_rate_upper_limit' must be set greater than 'range_rate_lower_limit'.",
    "'range_rate_upper_limit' 必须大于 'range_rate_lower_limit'：上界 {upper}，下界 {lower}。",
    "径向速度上下限大小关系颠倒",
    "调整 range_rate_upper_limit 与 range_rate_lower_limit 的大小关系"},
   // ATA 发射计算机 Range 值非连续递增
   {ut::msg::MSG_ATA_LC_RANGE_NOT_INCREASING,
    "Launch computer Range values must be continuously increasing.",
    "发射计算机的 Range 值必须连续递增：平台 {platform}。",
    "Range 表值出现相等或递减，插值要求升序",
    "检查 Range 表输入值并改为连续递增"},

   // SAM 横向偏移数组元素数不足
   {ut::msg::MSG_SAM_LC_OFFSETS_SIZE,
    "WsfSAM_LC size of lateral offsets array must be greater than 1.",
    "SAM 发射计算机的横向偏移数组元素数必须大于 1。",
    "lateral_offsets 数组只有 0 或 1 个元素",
    "为 lateral_offsets 提供至少两个值"},
   // SAM 横向偏移数组非单调递增
   {ut::msg::MSG_SAM_LC_OFFSETS_MONO,
    "WsfSAM_LC must receive a monotonically increasing array of lateral offsets.",
    "SAM 发射计算机的横向偏移数组必须单调递增。",
    "lateral_offsets 数组存在非递增的相邻元素",
    "按升序整理 lateral_offsets 数组取值"},
   // SAM 高度数组元素数不足
   {ut::msg::MSG_SAM_LC_ALT_SIZE,
    "WsfSAM_LC size of altitudes array must be greater than 1.",
    "SAM 发射计算机的高度数组元素数必须大于 1。",
    "altitudes 数组只有 0 或 1 个元素",
    "为 altitudes 提供至少两个值"},
   // SAM 高度数组非单调递增
   {ut::msg::MSG_SAM_LC_ALT_MONO,
    "WsfSAM_LC must receive a monotonically increasing array of altitudes.",
    "SAM 发射计算机的高度数组必须单调递增。",
    "altitudes 数组存在非递增的相邻元素",
    "按升序整理 altitudes 数组取值"},
   // SAM 目标距离数组元素数不足
   {ut::msg::MSG_SAM_LC_RANGE_SIZE,
    "WsfSAM_LC size of target ranges array must be greater than 1.",
    "SAM 发射计算机的目标距离数组元素数必须大于 1。",
    "target_ranges 数组只有 0 或 1 个元素",
    "为 target_ranges 提供至少两个值"},
   // SAM 目标距离数组非单调递增
   {ut::msg::MSG_SAM_LC_RANGE_MONO,
    "WsfSAM_LC must receive a monotonically increasing array of target ranges.",
    "SAM 发射计算机的目标距离数组必须单调递增。",
    "target_ranges 数组存在非递增的相邻元素",
    "按升序整理 target_ranges 数组取值"},
   // SAM 目标速度数组元素数不足
   {ut::msg::MSG_SAM_LC_SPEED_SIZE,
    "WsfSAM_LC size of target speeds array must be greater than 1.",
    "SAM 发射计算机的目标速度数组元素数必须大于 1。",
    "target_speeds 数组只有 0 或 1 个元素",
    "为 target_speeds 提供至少两个值"},
   // SAM 目标速度数组非单调递增
   {ut::msg::MSG_SAM_LC_SPEED_MONO,
    "WsfSAM_LC must receive a monotonically increasing array of target speeds.",
    "SAM 发射计算机的目标速度数组必须单调递增。",
    "target_speeds 数组存在非递增的相邻元素",
    "按升序整理 target_speeds 数组取值"},
   // SAM 飞行时间插值失败
   {ut::msg::MSG_SAM_LC_TOF_INTERP_FAIL,
    "Time of Flight interpolation cannot be performed.",
    "无法执行飞行时间插值（详见附注各表条目数）。",
    "某个维度的表条目数不足两个",
    "检查 offsets/altitudes/ranges/speeds 各表的条目数量"},
   // BM 未提供弹道表
   {ut::msg::MSG_BM_LC_NO_TABLE,
    "Surface_to_surface or surface_to_air table must be provided.",
    "必须提供 surface_to_surface 或 surface_to_air 弹道表。",
    "两种弹道表均未定义",
    "在发射计算机定义中补充任一弹道表"},
   // BM 弹道目标数据过期
   {ut::msg::MSG_BM_LC_TARGET_OBSOLETE,
    "Ballistic target data for target is obsolete.",
    "目标的弹道数据已过期（详见附注时刻与误差）。",
    "目标实际位移超出弹道预测容差",
    "提高弹道表精度或更新目标跟踪数据"},
   // BM 文件输入错误（3 处调用点共用）
   {ut::msg::MSG_BM_LC_INPUT_ERROR,
    "Input error encountered.",
    "读取弹道文件时遇到输入错误（详见附注 File 名）。",
    "文件内容格式不符合弹道表要求",
    "按弹道表格式检查该行内容"},
   // BM 弹道文件打开失败
   {ut::msg::MSG_BM_LC_FILE_OPEN_FAIL,
    "Unable to open file.",
    "无法打开弹道文件（详见附注 File 名）。",
    "文件不存在或无访问权限",
    "核对文件路径与文件名"},
   // BM 地面距离非递增
   {ut::msg::MSG_BM_LC_GROUND_NOT_ASC,
    "Ground ranges must be ascending.",
    "地面距离值必须递增。",
    "同一弹道内 ground_range 出现非递增序列",
    "按升序整理该弹道的地面距离取值"},
   // BM 飞行时间非单调递增
   {ut::msg::MSG_BM_LC_TOF_NOT_ASC,
    "Time of flight values must be monotonically ascending.",
    "飞行时间值必须单调递增。",
    "同一弹道内飞行时间出现非递增序列",
    "按升序整理该弹道的飞行时间取值"},
   // BM 文件含空弹道
   {ut::msg::MSG_BM_LC_EMPTY_TRAJ,
    "File contains an empty trajectory.",
    "弹道文件包含空弹道（详见附注 File 名）。",
    "某弹道的数据点数为 0",
    "删除空弹道段或补充数据点"},
   // BM 文件无弹道
   {ut::msg::MSG_BM_LC_NO_TRAJ,
    "No trajectories in file.",
    "弹道文件中没有任何弹道（详见附注 File 名）。",
    "文件未包含有效弹道定义",
    "检查文件内容是否为合法弹道表"},

   // 表格化杀伤未指定 CEP/Pk
   {ut::msg::MSG_TAB_LETH_NO_CEP_PK,
    "WsfTabulatedLethality did not specify any CEP  or Pk values for target.",
    "表格化杀伤模型未给目标指定任何 CEP 或 Pk 值（详见附注 Lethality/Target 名）。",
    "某目标的 CEP-Pk 表为空",
    "为该目标补充 CEP/Pk 数据对"},
   // 表格化杀伤 Pk 未随 CEP 递减
   {ut::msg::MSG_TAB_LETH_PK_NOT_DEC,
    "WsfTabulatedLethality: Not all Pk values decrease with increasing CEP.",
    "表格化杀伤模型的 Pk 值必须随 CEP 增大而减小。",
    "CEP-Pk 表中存在随 CEP 增大而不减小的 Pk 值",
    "按物理规律修正 Pk 表（CEP 越大命中概率越低）"},
   // HEL 杀伤未定义效应
   {ut::msg::MSG_HEL_LETH_NO_EFFECT,
    "HEL_Lethality: No effect defined for entry.",
    "高能激光杀伤模型未给条目定义效应（详见附注 Entry 名）。",
    "杀伤条目缺少效应定义",
    "为该条目补充效应定义"},
   // HEL Pk-能量表行数不足
   {ut::msg::MSG_HEL_LETH_TABLE_ROWS,
    "Pk-energy tables must have at least two rows.",
    "Pk-能量表必须至少包含两行。",
    "表数据行数少于 2，无法插值",
    "补充至少两行能量-Pk 数据"},
   // 分级杀伤未指定半径/Pk
   {ut::msg::MSG_GRAD_LETH_NO_RADIUS_PK,
    "No radius/Pk values were specified.",
    "分级杀伤模型未指定任何半径/Pk 值。",
    "缺少半径-Pk 数据对",
    "补充 radius/Pk 数据对"},
   // 分级杀伤 Pk 未随脱靶量递减
   {ut::msg::MSG_GRAD_LETH_PK_NOT_DEC,
    "Pk values do not decrease with increasing miss distance.",
    "Pk 值必须随脱靶量增大而减小。",
    "半径-Pk 表中存在随脱靶量增大而不减小的 Pk 值",
    "按物理规律修正 Pk 表（脱靶量越大杀伤概率越低）"},
   // 高能激光类型无法识别
   {ut::msg::MSG_HEL_LASER_BAD_TYPE,
    "Value for laser_type not recognized.",
    "laser_type 的取值无法识别。",
    "laser_type 值不在支持列表中",
    "核对 laser_type 拼写与支持的类型列表"},
   // 高能激光相交测试无效（两处调用点共用）
   {ut::msg::MSG_HEL_LASER_INTERSECT,
    "WsfHighEnergyLaser::UpdateEngagementGeometry: Intersect test for target is invalid. Check the offset name.",
    "高能激光对目标的相交测试无效，请检查偏移量名称（详见附注 T/Target 名）。",
    "目标偏移量名称无效导致几何相交测试失败",
    "核对目标平台的 offset 名称定义"},
   // MFK 表概率和不为一
   {ut::msg::MSG_MFL_MFK_NOT_UNITY,
    "MFK Table specified probabilities that do not sum to Unity.",
    "MFK 表指定的概率之和不为 1。",
    "机动火力杀伤表的各状态概率之和偏离 1",
    "修正 MFK 表概率使各行/列之和为 1"},
   // CM 易损性同时指定 PkFactor 与表
   {ut::msg::MSG_MFL_CM_BOTH,
    "CM Vulnerabilities may use PkFactor or Table, but not both.",
    "CM 易损性只能使用 PkFactor 或 Table 之一，不能同时指定。",
    "PkFactor 与 Table 同时出现",
    "删除其中一项，保留 PkFactor 或 Table"},
   // 机动火力杀伤表未找到
   {ut::msg::MSG_MFL_TABLE_NOT_FOUND,
    "Table was not found.",
    "找不到指定的表（详见附注 Name）。",
    "引用的杀伤表名未在场景中定义",
    "核对表名拼写与表定义"},
   // 球形杀伤最大半径不大于最小值
   {ut::msg::MSG_SPH_LETH_RADIUS,
    "WsfSphericalLethality Maximum radius must be greater than Minimum.",
    "球形杀伤模型的最大半径必须大于最小半径。",
    "maximum_radius 不大于 minimum_radius",
    "调整半径取值使最大值大于最小值"},
   // 球形杀伤最大伤害不大于最小值
   {ut::msg::MSG_SPH_LETH_DAMAGE,
    "WsfSphericalLethality Maximum damage must be greater than Minimum.",
    "球形杀伤模型的最大伤害必须大于最小伤害。",
    "maximum_damage 不大于 minimum_damage",
    "调整伤害取值使最大值大于最小值"},
   // 球形杀伤阈值不小于最大伤害
   {ut::msg::MSG_SPH_LETH_THRESHOLD,
    "WsfSphericalLethality damage Threshold must be less than Max Damage.",
    "球形杀伤模型的伤害阈值必须小于最大伤害。",
    "threshold 不小于 maximum_damage",
    "调整阈值使其小于最大伤害"},
   // 发射交接无法确定发射平台（WsfLaunchHandoffData::SetLaunchPlatformData）
   {ut::msg::MSG_HANDOFF_NO_LAUNCH_PLATFORM,
    "WsfLaunchHandoff cannot determine launch platform.",
    "发射交接数据无法确定发射平台。",
    "发射交接时既未提供发射平台指针，也无法从发射系统取得所属平台",
    "确认交战配置中发射平台或发射系统（articulated part）定义有效"},
   // ATG 发射计算机未包含任何 LAR（WsfATG_LaunchComputer::Initialize）
   {ut::msg::MSG_ATG_LC_NO_LARS,
    "The WsfATG_LaunchComputer in platform does not contain any LARs for launch evaluation.",
    "平台的 ATG 发射计算机未包含任何可用于发射评估的 LAR。",
    "非计算机生成模式下 WsfATG_LaunchComputer 未定义 weapon_lar 块",
    "在发射计算机定义中至少增加一个 weapon_lar 配置块"},
   // ATG 发射计算机父武器缺 update_interval（多 LAR 须周期更新择优）
   {ut::msg::MSG_ATG_LC_NO_UPDATE_INTERVAL,
    "The WsfATG_LaunchComputer's parent weapon requires a defined update_interval.",
    "ATG 发射计算机的父武器需要定义 update_interval。",
    "武器定义了多个 LAR 但父武器未指定 update_interval",
    "为父武器指定非零 update_interval"},
   // 空空拦截表文件解析失败（WsfAirToAirLaunchComputer::ATA_InterceptTable::Load）
   {ut::msg::MSG_A2A_LC_FILE_PARSE,
    "Could not parse the file given.",
    "无法解析给定的空空拦截表文件。",
    "拦截表文件内容不符合 launch_computer_table 语法",
    "核对文件内容与 launch_computer_table 语法要求"},
   // 光斑指示激光武器找不到波束导向器（WsfCuedLaserWeapon::Initialize）
   {ut::msg::MSG_CUED_LASER_NO_BEAM_DIR,
    "WsfBeamDirector specified for WsfCuedLaserWeapon not found on platform",
    "平台上找不到 WsfCuedLaserWeapon 指定的波束导向器。",
    "武器 beam_director 指定的组件在平台上不存在",
    "核对武器 beam_director 取值与平台上 WsfBeamDirector 组件名"},
   // 轨道发射计算机文件打开失败（读入轨道表）
   {ut::msg::MSG_ORBITAL_LC_OPEN_READ,
    "Unable to open file.",
    "无法打开轨道文件进行读取。",
    "文件不存在或无读取权限",
    "核对文件路径与文件读取权限"},
   // 轨道发射计算机文件行读取失败（行内字段与轨道表格式不符）
   {ut::msg::MSG_ORBITAL_LC_LINE_READ,
    "Error reading line of file.",
    "读取轨道文件行时出错。",
    "行内字段数量或类型与轨道表格式不符",
    "按注释格式逐列核对出错行的内容"},
   // 轨道发射计算机文件打开失败（写出轨道数据）
   {ut::msg::MSG_ORBITAL_LC_OPEN_WRITE,
    "Unable to open file.",
    "无法打开轨道文件进行写入。",
    "输出目录不存在或无写入权限",
    "核对输出目录存在及文件写入权限"},
   // ATG 武器 LAR 未定义区域（WsfATG_WeaponLAR::Initialize）
   {ut::msg::MSG_ATG_LAR_NO_ZONE,
    "Zone was not defined.",
    "ATG 武器 LAR 未定义区域。",
    "weapon_lar 块中未指定 use_zone",
    "在 weapon_lar 中增加 use_zone 指定"},

   // ---- 交战段（0x0004xxxx）dis 武器转移（第十四批）----
   // DIS 武器转移无法克隆武器类型
   {ut::msg::MSG_DIS_WXFR_NO_CLONE,
    "Incoming weapon transfer. Unable to clone weapon type.",
    "收到 DIS 武器转移：无法克隆指定的武器类型（详见附注 Weapon System Type）。",
    "武器系统类型未在本地武器类型列表中注册",
    "核对武器类型名或补充对应 weapon_type 定义"},
   // DIS 武器转移无法添加武器对象
   {ut::msg::MSG_DIS_WXFR_NO_ADD,
    "Incoming weapon transfer. Unable to add weapon object.",
    "收到 DIS 武器转移：无法将武器对象添加到发射平台（详见附注 Weapon System Type）。",
    "平台组件添加失败",
    "检查平台组件配置"},
   // DIS 武器转移无法初始化武器对象
   {ut::msg::MSG_DIS_WXFR_NO_INIT,
    "Incoming weapon transfer. Unable to initialize weapon object.",
    "收到 DIS 武器转移：无法初始化武器对象（详见附注 Weapon System Type）。",
    "武器初始化校验未通过",
    "检查武器类型定义参数"},
   // DIS 武器转移本地武器发射失败
   {ut::msg::MSG_DIS_WXFR_FIRE_FAIL,
    "Incoming weapon transfer. Local weapon firing failed.",
    "收到 DIS 武器转移：本地武器发射失败。",
    "Fire() 返回失败",
    "检查武器/平台状态与发射条件"},
   // DIS 武器转移发射后交战指针为空
   {ut::msg::MSG_DIS_WXFR_NO_ENGAGE,
    "Incoming weapon transfer. Weapon engagement pointer is zero after weapon firing.",
    "收到 DIS 武器转移：武器发射后交战对象指针为空。",
    "Fire() 成功但未返回有效交战对象",
    "检查武器交战配置"},

   // ---- 交战段（0x0004xxxx）ew 电子战域（第十五批）----
   // EW 筛选器初始化失败
   {ut::msg::MSG_EW_SC_SCREENER_INIT,
    "EW failed to initialize screener for mode.",
    "EW（电子战）筛选器初始化失败（详见附注 Mode）。",
    "false_target_screeners 配置或脚本校验未通过",
    "检查该模式的筛选器定义"},
   // EW 干扰方位探测器初始化失败
   {ut::msg::MSG_EW_SC_JSD_INIT,
    "EW failed to initialize jam strobe detector for mode.",
    "EW（电子战）干扰方位探测器初始化失败（详见附注 Mode）。",
    "jam_strobe_detector 配置校验未通过",
    "检查该模式的干扰方位探测器定义"},
   // 雷达/平台分布行为组合不允许
   {ut::msg::MSG_FT_RADAR_PLAT_COMBO,
    "Initializing false-target effect, the combination of 'random_to_multiple_radars' and 'consistent_to_multiple_platforms' is not allowed.",
    "假目标效果初始化：不允许同时使用 random_to_multiple_radars 与 consistent_to_multiple_platforms 组合（详见附注 False Target）。",
    "两个分布行为参数语义冲突",
    "修改其中之一的分布行为参数"},
   // 一致雷达分布缺参考质心
   {ut::msg::MSG_FT_NO_CENTROID,
    "Initializing false-target effect, false target reference centroid not set for 'consistent_to_multiple_radars' distribution.",
    "假目标效果初始化：consistent_to_multiple_radars 分布未设置参考质心（详见附注 False Target）。",
    "一致分布需要统一的参考质心",
    "补充 reference_centroid 或 reference position 指定"},
   // 假目标参考质心纬度输入不完整
   {ut::msg::MSG_FT_NO_LAT,
    "In Input for false-target effect, false target reference centroid latitude or position not properly entered.",
    "假目标效果输入：参考质心纬度或位置输入不完整（详见附注 False Target）。",
    "reference_centroid 纬度字段缺失",
    "按纬度/经度或完整位置格式输入参考质心"},
   // 假目标参考质心经度输入不完整
   {ut::msg::MSG_FT_NO_LON,
    "In Input for false-target effect, false target reference centroid longitude or position not properly entered.",
    "假目标效果输入：参考质心经度或位置输入不完整（详见附注 False Target）。",
    "reference_centroid 经度字段缺失",
    "按纬度/经度或完整位置格式输入参考质心"},
   // EW 效果缺天线方向图（SLC/SLB 两文件共用）
   {ut::msg::MSG_EW_ANT_NO_DEF,
    "Antenna pattern not defined.",
    "未定义天线方向图（详见附注 Effect）。",
    "效果定义中缺 antenna pattern 指定",
    "补充 antenna_pattern 或引用天线类型"},
   // SLC 效果缺对消比表（两处调用点共用）
   {ut::msg::MSG_EW_SLC_NO_CANCEL,
    "'cancelation_ratios' were not entered.",
    "未输入 cancelation_ratios（对消比，详见附注 Effect）。",
    "旁瓣对消效果缺对消比表",
    "补充 cancelation_ratios 指定"},
   // SLC 效果对消器数目缺对消比
   {ut::msg::MSG_EW_SLC_NO_CANCEL_FOR_NJ,
    "'cancelation_ratios' not entered for 'number_canceled_jammers' or 'jammer_canceled'",
    "对 number_canceled_jammers 或 jammer_canceled 指定的数目未输入对应的 cancelation_ratios（详见附注 Effect/Number Not Entered）。",
    "对消器数目与对消比表条目数不匹配",
    "按数目补充完整的对消比表"},
   // 假目标效果初始化假目标失败
   {ut::msg::MSG_EW_FTE_FT_INIT_FAIL,
    "Initializing false-target.",
    "假目标初始化失败（详见附注 False Target）。",
    "假目标内部参数校验未通过",
    "检查假目标定义参数"},
   // 假目标效果找不到假目标定义
   {ut::msg::MSG_EW_FTE_NO_FT,
    "Initializing false-target effect. False target not found.",
    "假目标效果初始化：找不到指定的假目标定义（详见附注 False Target）。",
    "引用的假目标未在假目标类型列表中注册",
    "核对假目标名或补充 false_target 定义"},
   // 干扰方位探测器不报告任何量
   {ut::msg::MSG_EW_JSD_NOTHING_REPORT,
    "Jam strobe detector does not report anything.",
    "干扰方位探测器不报告任何量测。",
    "方位与仰角报告开关均未开启",
    "开启 report_bearing 或 report_elevation 至少其一"},
   // 基础效果数据无效无法传播
   {ut::msg::MSG_EW_EFF_NO_DATA,
    "Cannot propagate effect data due to invalid base effect data.",
    "基础效果数据无效，无法传播效果数据（详见附注 Effect）。",
    "基类效果数据映射缺失",
    "检查效果的系统类型数据定义"},

   // ---- 通信段（0x0005xxxx）----
   // 通信设备指定的 router 组件不在所属平台上（WsfCommNetworkManager::PlatformsInitialized）
   {ut::msg::MSG_COMM_ROUTER_NOT_FOUND,
    "Assigned router not found on platform.",
    "平台上找不到通信设备指定的路由器：通信设备 {comm}，路由器 {router}。",
    "comm 定义中 router 命令指定的名称与平台上的 router 组件名不一致",
    "核对平台定义中 router 组件名与 comm 的 router 命令取值是否一致"},
   // 通信设备入网失败：按网络名加入（AddComm 返回空地址）
   {ut::msg::MSG_COMM_ASSIGN_NETWORK_FAIL,
    "Error assigning comm to network.",
    "通信设备入网失败：网络 {network}，通信设备 {comm}。",
    "网络成员数达到上限（用户自定义网络默认上限 255，默认网络除外），分配地址失败",
    "增大网络容量，或将通信设备加入其他网络；非用户定义网络上限为 255"},
   // 同名网络创建失败（外部地址关联的网络重名）
   {ut::msg::MSG_COMM_CREATE_NETWORK_FAIL,
    "Cannot create network of same name.",
    "无法创建同名网络：地址 {address}，通信设备 {comm}。",
    "以该地址/名称创建网络时发生重名冲突",
    "检查是否重复定义了相同地址的网络，修改网络名或地址"},
   // 通信设备按地址入网失败（AddComm 按地址分配失败）
   {ut::msg::MSG_COMM_ASSIGN_ADDR_FAIL,
    "Error assigning comm to network address.",
    "通信设备按地址入网失败：地址 {address}，通信设备 {comm}。",
    "目标网络成员数已满，无法在该地址对应的网络中分配成员地址",
    "增大网络容量，或将通信设备加入其他网络"},
   // 外部链路创建：目标平台不存在
   {ut::msg::MSG_COMM_EXT_LINK_NO_PLATFORM,
    "Platform not found during external link creation in network manager.",
    "外部链路创建失败：找不到平台：{platform}（发起通信设备 {comm}）。",
    "external_link 指定的平台名未在当前仿真中定义或尚未创建",
    "核对 external_link 中平台名的拼写，确认该平台在想定中已定义"},
   // 外部链路创建：目标平台上的通信设备不存在
   {ut::msg::MSG_COMM_EXT_LINK_NO_COMM,
    "Comm not found during external link creation in network manager.",
    "外部链路创建失败：平台上找不到通信设备：{comm}。",
    "目标平台存在但没有指定名称的通信设备组件",
    "核对目标平台定义中 comm 组件名与 external_link 的取值"},
   // 外部链路创建：目标通信设备地址为空（尚未入网）
   {ut::msg::MSG_COMM_EXT_LINK_NO_ADDR,
    "Comm address not found during external link creation in network manager.",
    "外部链路创建失败：通信设备没有地址（未入网）：{comm}。",
    "目标通信设备尚未被分配网络地址（未加入任何网络）",
    "确认目标通信设备已通过 network/join 等命令完成入网后再建立外部链路"},
   // 通信介质初始化：未定义任何模式
   {ut::msg::MSG_COMM_MEDIUM_NO_MODES,
    "No modes defined.",
    "通信介质未定义任何模式：介质 {medium}。",
    "medium 定义块中缺少 mode 子命令，无模式则无法计算传输时延等参数",
    "在介质定义中至少添加一个 mode 子命令"},
   // 通信介质初始化：默认模式名不存在
   {ut::msg::MSG_COMM_MEDIUM_BAD_DEFAULT,
    "Default mode name does not exist.",
    "通信介质的默认模式名不存在：模式名 {mode}。",
    "default_mode 命令指定的名称与已定义的 mode 名称不一致",
    "核对 default_mode 取值与 mode 定义名是否一致（区分大小写）"},
   // 发送时指定的介质模式不存在（Guided/Unguided 介质共用此 ID）
   {ut::msg::MSG_COMM_MEDIUM_NO_MODE,
    "A medium mode was designated for transmission that does not exist.",
    "发送消息时指定的介质模式不存在。",
    "发送请求引用的介质模式名在当前介质定义中不存在（已被删除或拼写错误）",
    "核对发送侧指定的模式名与介质定义中的 mode 名是否一致"},
   // 重复请求发送正在传输中的消息（Guided/Unguided 介质共用此 ID）
   {ut::msg::MSG_COMM_MEDIUM_DUP_MSG,
    "A transmit message request was made for a message thats already being transmitted",
    "重复请求发送正在传输中的消息。",
    "同一消息标识（发送方+接收方+消息 ID）在介质中已有进行中的传输记录",
    "检查上层是否对同一消息重复调用了发送接口；此错误通常意味着上层逻辑缺陷"},
   // 默认网络 comm 对象数达上限（WsfCommNetworkManager::AddComm）
   {ut::msg::MSG_COMM_DEFAULT_NET_FULL,
    "Maximum number of comm objects defined on default network.",
    "默认网络上的通信设备数量已达上限。",
    "默认网络容量固定，接入的 comm 对象超出内置上限",
    "将部分通信设备显式加入用户自定义网络，减少默认网络接入量"},
   // comm 入网参数指定过多（network/address 多种方式并存）
   {ut::msg::MSG_COMM_TOO_MANY_INPUTS,
    "Too many inputs for comm.",
    "通信设备的网络参数指定过多：见附注中的具体取值。",
    "同一 comm 同时指定了 address 与 network 等多种入网方式，产生歧义",
    "只保留一种入网方式（network 名或 address），删除多余配置"},
   // comm 缺少 router 关联（NetworkManager 平台初始化）
   {ut::msg::MSG_COMM_MISSING_ROUTER_ASSIGN,
    "Missing router assignment for comm.",
    "通信设备缺少 router 关联：时刻 {t}，通信设备 {comm}。",
    "comm 未通过 router 命令关联到平台上的路由器组件",
    "在 comm 定义中补充 router 命令，且名称与平台 router 组件一致"},
   // 平台缺少 comm 引用的 router（NetworkManager::PlatformInitialized）
   {ut::msg::MSG_COMM_ROUTER_MISSING_ON_PLAT,
    "Missing router on platform.",
    "平台上缺少通信设备引用的路由器：平台 {platform}，路由器 {router}。",
    "comm 的 router 指定的名称在平台上无对应 router 组件",
    "核对平台定义中 router 组件名与 comm 的 router 命令取值是否一致"},
   // 介质类型不允许添加到该对象（WsfCommMediumTypes）
   {ut::msg::MSG_COMM_MEDIUM_TYPE_INVALID,
    "Cannot add invalid medium type.",
    "无法添加无效的介质类型：介质类型 {medium}。",
    "该介质类型不允许用于此对象（类型不匹配或未注册）",
    "检查 medium 类型名拼写，并确认该对象允许此介质类型"},
   // OSPF 路由器缺少 area 指定
   {ut::msg::MSG_COMM_OSPF_NO_AREA,
    "Missing area designation on OSPF enabled router.",
    "OSPF 路由器缺少 area 指定：平台 {platform}，路由器 {router}。",
    "启用 OSPF 协议的路由器未配置任何区域",
    "在 router 的 OSPF 协议定义中补充 area 指定"},
   // OSPF 区域未连到骨干区域
   {ut::msg::MSG_COMM_OSPF_AREA_DISCONNECTED,
    "OSPF area not connected to designated backbone area.",
    "OSPF 区域未连接到指定的骨干区域：区域 {area}。",
    "该区域与骨干区域之间无连通路径（OSPF 要求所有区域经骨干互通）",
    "调整网络拓扑或区域设计，使该区域与骨干区域连通"},
   // OSPF backbone 配置不一致
   {ut::msg::MSG_COMM_OSPF_BACKBONE_INCONSIST,
    "Inconsistent backbone setting.",
    "OSPF backbone 配置不一致：地址 {address}。",
    "同一地址在不同路由器上的 backbone 属性设置矛盾",
    "统一各路由器上该地址的 backbone 设置"},
   // 路由器发送时源地址为空（WsfCommRouter::Send）
   {ut::msg::MSG_COMM_ROUTER_SEND_NO_SOURCE,
    "Invalid send attempt, missing source address in router send method.",
    "路由器发送消息的源地址为空，发送尝试无效。",
    "消息携带的源地址未初始化即调用路由器发送",
    "检查消息构造流程，确保源地址已分配"},
   // comm 接口自动链路创建失败（WsfCommRouter::AutomateAddedLinkage）
   {ut::msg::MSG_COMM_ROUTER_AUTO_LINK_FAIL,
    "Automated linking for comm interface has failed.",
    "通信设备接口自动链路创建失败：通信设备 {comm}，路由器 {router}。",
    "网络类型不允许为该路由器接口创建链路",
    "确认网络类型支持接口链路创建，或改用允许的网络类型"},

   // ---- 通信段（0x0005xxxx）DIS 协议库（第十六批）----
   // DIS_VERSION 环境变量值非法
   {ut::msg::MSG_DIS_ENV_BAD_VERSION,
    "Invalid value for 'DIS_VERSION'.",
    "DIS_VERSION 环境变量值非法（详见附注 Value）。",
    "DIS 协议版本号超出范围（0-6）",
    "设置为 0-6 之间的协议版本号"},
   // DIS_EXERCISE 环境变量值非法
   {ut::msg::MSG_DIS_ENV_BAD_EXERCISE,
    "Invalid value for 'DIS_EXERCISE'.",
    "DIS_EXERCISE 环境变量值非法（详见附注 Value）。",
    "演习号超出范围（0-255）",
    "设置为 0-255 之间的演习号"},
   // DIS_SITE 环境变量值非法
   {ut::msg::MSG_DIS_ENV_BAD_SITE,
    "Invalid value for 'DIS_SITE'.",
    "DIS_SITE 环境变量值非法（详见附注 Value）。",
    "站点号超出范围（0-65535）",
    "设置为 0-65535 之间的站点号"},
   // DIS_APPLICATION 环境变量值非法
   {ut::msg::MSG_DIS_ENV_BAD_APPLICATION,
    "Invalid value for 'DIS_APPLICATION'.",
    "DIS_APPLICATION 环境变量值非法（详见附注 Value）。",
    "应用号超出范围（0-65535）",
    "设置为 0-65535 之间的应用号"},
   // DIS_HRT_BEAT_TIMER 环境变量值非法
   {ut::msg::MSG_DIS_ENV_BAD_HBT,
    "Invalid value for 'DIS_HRT_BEAT_TIMER'.",
    "DIS_HRT_BEAT_TIMER 环境变量值非法（详见附注 Value）。",
    "心跳定时器为负数",
    "设置为非负的心跳秒数"},
   // DIS_HRT_BEAT_MPLIER 环境变量值非法
   {ut::msg::MSG_DIS_ENV_BAD_HBT_MPLIER,
    "Invalid value for 'DIS_HRT_BEAT_MPLIER'.",
    "DIS_HRT_BEAT_MPLIER 环境变量值非法（详见附注 Value）。",
    "心跳倍率为负数",
    "设置为非负的心跳倍率"},
   // PDU 类型读取失败
   {ut::msg::MSG_DIS_PDU_TYPE_READ_FAIL,
    "Error reading DisPdu type of PDU.",
    "读取 PDU 的 DisPdu 类型时出错（详见附注 Position）。",
    "接收缓冲区中的 PDU 数据损坏或格式非法",
    "检查发送方 DIS 实现的 PDU 编码"},
   // PDU 数据尺寸不匹配
   {ut::msg::MSG_DIS_PDU_SIZE_MISMATCH,
    "Mismatched size reading data.",
    "读取数据时尺寸不匹配（详见附注 Interface Name/PDU Type）。",
    "PDU 实际长度与声明的数据长度不一致",
    "检查发送方 PDU 长度字段的编码"},
   // 信号 PDU 长度非 32 位边界
   {ut::msg::MSG_DIS_SIG_NO_32BIT_BOUNDARY,
    "DisSignal::IsValid(): PDU does not have a length on a 32-bit boundary.",
    "信号 PDU 校验失败：PDU 长度未落在 32 位边界上（详见附注 PDU Sent From）。",
    "PDU 总位数不是 32 的整数倍",
    "检查发送方信号 PDU 的长度填充"},
   // 数据库索引编码类长度须为 96
   {ut::msg::MSG_DIS_SIG_DB_LEN_NOT_96,
    "Data Length field must be 96 if the Encoding Class is Database index.",
    "编码类为数据库索引时，数据长度字段必须为 96（详见附注 Encoding/PDU Sent From）。",
    "数据库索引编码类的 Data Length 不为 96 位",
    "按 DIS 标准将数据库索引编码的长度置 96"},
   // 信号 PDU 数据长度与头部位数不符
   {ut::msg::MSG_DIS_SIG_LEN_BITS_MISMATCH,
    "Data Length field does not match the number of bits specified in the PDU Header.",
    "数据长度字段与 PDU 头中声明的位数不匹配（详见附注 Data Length/Specified Bits/Total PDU Size）。",
    "Data Length 与 PDU Header 长度字段不一致",
    "检查发送方信号 PDU 的长度字段编码"},
   // DisRadioId 流提取失败
   {ut::msg::MSG_DISIO_RADIOID_READ_FAIL,
    "DisIO::operator>>(istream&, DisRadioId&) stream failed.",
    "DisRadioId 的流提取操作失败（stream failed）。",
    "流中数据不足以完成电台标识的提取",
    "检查输入流中的电台标识数据完整性"},
   // DisRadioId 流提取前流状态异常
   {ut::msg::MSG_DISIO_RADIOID_READ_NOT_GOOD,
    "DisIO::operator>>(istream&, DisRadioId&) stream not good.",
    "DisRadioId 的流提取操作前流状态已异常（stream not good）。",
    "提取前输入流已处于异常状态",
    "检查上游流操作是否已置位错误标志"},
   // DisRadioId Pack 流写入失败
   {ut::msg::MSG_DIS_RADIOID_PACK_FAIL,
    "DisRadioId::Pack stream failed.",
    "DisRadioId 序列化打包（Pack）时流写入失败。",
    "流写入过程中出错",
    "检查输出流（网络/文件）状态与容量"},
   // DisRadioId Pack 输入流状态异常
   {ut::msg::MSG_DIS_RADIOID_PACK_NOT_GOOD,
    "DisRadioId::Pack was sent a stream that was not good.",
    "DisRadioId 序列化打包（Pack）收到的流状态异常（not good）。",
    "打包前输出流已处于异常状态",
    "检查上游流操作是否已置位错误标志"},
   // DisRadioId UnPack 流读取失败
   {ut::msg::MSG_DIS_RADIOID_UNPACK_FAIL,
    "DisRadioId::UnPack stream failed.",
    "DisRadioId 反序列化解包（UnPack）时流读取失败。",
    "流读取过程中出错",
    "检查输入流（网络/文件）中的数据完整性"},
   // DisRadioId UnPack 输入流状态异常
   {ut::msg::MSG_DIS_RADIOID_UNPACK_NOT_GOOD,
    "DisRadioId::UnPack was sent a stream that was not good.",
    "DisRadioId 反序列化解包（UnPack）收到的流状态异常（not good）。",
    "解包前输入流已处于异常状态",
    "检查上游流操作是否已置位错误标志"},
   // 信号 PDU 拷贝收到无效数据指针
   {ut::msg::MSG_DIS_SIG_COPY_BAD_PTR,
    "DisSignal::Copy was passed a signal pdu with a invalid data pointer.",
    "信号 PDU 拷贝（Copy）收到了无效的数据指针。",
    "拷贝源对象的数据指针为空",
    "检查源信号 PDU 的构造与数据填充逻辑"},
   // 上一个良好的 PDU 头（诊断转储引出句）
   {ut::msg::MSG_DIS_FACTORY_LAST_GOOD_HEADER,
    "Last good PDU Header was:",
    "上一个良好的 PDU 头为（详见十六进制转储附注）。",
    "接收缓冲区中出现损坏 PDU，输出上一正常 PDU 头以供对比",
    "对照转储内容检查发送方 PDU 编码"},
   // 接收缓冲区首个 PDU 即出错
   {ut::msg::MSG_DIS_FACTORY_FIRST_PDU,
    "This was the first PDU in the receive buffer.",
    "这是接收缓冲区中的第一个 PDU（无上一正常 PDU 可对照）。",
    "缓冲区首个 PDU 即解析失败",
    "检查发送方 DIS 实现的 PDU 编码与字节序设置"},
   // DIS 动作响应非加入演习请求（WsfDisActionResponse）
   {ut::msg::MSG_DIS_ACTION_RESP_NOT_JOIN,
    "WsfDisActionRequest: Initializing was not a Join Exercise Request.",
    "WsfDisActionRequest：初始化请求并非加入演习请求。",
    "收到非 JOIN_EXERCISE 类型的动作响应",
    "无需修复，该响应已被忽略"},
   // DIS 可靠动作响应非加入演习请求（WsfDisActionResponseR）
   {ut::msg::MSG_DIS_ACTION_RESPR_NOT_JOIN,
    "WsfDisActionRequestR: Initializing was not a Join Exercise Request.",
    "WsfDisActionRequestR：初始化请求并非加入演习请求。",
    "收到非 JOIN_EXERCISE 类型的可靠动作响应",
    "无需修复，该响应已被忽略"},
   // DIS 接口设备打开失败（WsfDisInterface 两处共用）
   {ut::msg::MSG_DIS_IFACE_OPEN_DEVICE_FAIL,
    "Unable to open dis_interface device.",
    "无法打开 dis_interface 设备。",
    "DIS 网络设备创建或初始化抛出异常",
    "检查设备配置与网络权限"},
   // 无法设置外观脚本变量
   {ut::msg::MSG_DIS_IFACE_SET_APPEAR_VAR_FAIL,
    "Unable to set appearance script variable.",
    "无法设置外观脚本变量。",
    "关节参数对应的脚本变量不存在",
    "检查 appearance 脚本变量定义"},
   // 无法执行外观更新脚本
   {ut::msg::MSG_DIS_IFACE_EXEC_APPEAR_FAIL,
    "Unable to execute appearance update.",
    "无法执行外观更新脚本。",
    "外观更新脚本执行失败",
    "检查脚本内容与签名"},
   // DIS 回放文件打开失败
   {ut::msg::MSG_DIS_PLAYBACK_OPEN_FAIL,
    "Could not open file.",
    "无法打开回放文件。",
    "DIS 回放文件不存在或不可读",
    "检查 playback 文件路径"},
   // DIS 回放文件指定过多
   {ut::msg::MSG_DIS_PLAYBACK_TOO_MANY,
    "Too many 'playback' files specified.",
    "指定的 'playback' 回放文件过多。",
    "回放站点编号超出上限",
    "减少回放文件数量"},
   // event_pipe 输出文件打开失败
   {ut::msg::MSG_EVENTPIPE_OPEN_FAIL,
    "Unable to open event_pipe file.",
    "无法打开 event_pipe 事件管道文件。",
    "输出文件路径无效或不可写",
    "检查 event_pipe 输出文件路径"},
   // XIO 无可用关联连接
   {ut::msg::MSG_XIO_NO_LINKED_CONN,
    "No linked connection available.",
    "无可用的关联连接。",
    "不可靠订阅请求时关联数据连接缺失",
    "检查 XIO 连接配置"},
   // XIO 消息无翻译器
   {ut::msg::MSG_XIO_MSG_NO_TRANSLATOR,
    "XIO: Message has no translator.",
    "XIO：消息没有对应的翻译器。",
    "消息类型未注册翻译器",
    "检查 XIO 消息翻译器注册"},
   // XIO 自动 DIS 映射初始化失败
   {ut::msg::MSG_XIO_AUTO_DIS_MAP_INIT_FAIL,
    "auto_dis_mapping: Initialization failed.",
    "auto_dis_mapping：自动 DIS 映射初始化失败。",
    "自动 DIS 映射对象初始化失败",
    "检查 auto_dis_mapping 配置"},
   // 无法通过 XIO 发送 WsfMessage
   {ut::msg::MSG_XIO_CANNOT_TX_MSG,
    "Cannot transmit WsfMessage over XIO.",
    "无法通过 XIO 发送 WsfMessage 消息。",
    "消息类型不在 XIO 可翻译列表中",
    "注册对应消息的翻译器"},

   // ---- 工具/UI 段（0x0006xxxx）----
   // 目录初始化失败：dir=逻辑目录名（Bin/Data/Root），path=实际路径
   {ut::msg::MSG_WIZARD_INVALID_DIR,
    "Invalid directory [{dir}] [{path}]",
    "目录无效：[{dir}] [{path}]",
    "程序启动时按固定布局检查安装目录，该目录不存在或不可访问",
    "确认安装完整性；从官方发布包重新解压，或检查磁盘与安全软件拦截"},
   // 绘图指令引用了不存在的路由
   {ut::msg::MSG_DRAW_ROUTE_NOT_FOUND,
    "Draw route: Rout not found.",
    "绘制 route: 未找到指定的路由：名称 {route}。",
    "draw 命令引用的 route 名称未在 WsfRouteTypes 中定义",
    "核对想定中 route 定义与 draw 命令中的名称拼写是否一致"},
   // 绘图指令引用了不存在的路由网络
   {ut::msg::MSG_DRAW_NETWORK_NOT_FOUND,
    "Draw route_network: Route not found.",
    "绘制 route_network: 未找到指定的路由网络：名称 {route}。",
    "draw 命令引用的 route_network 名称未在 WsfRouteNetworkTypes 中定义",
    "核对想定中 route_network 定义与 draw 命令中的名称拼写是否一致"},
};
constexpr size_t cCatalogSize = sizeof(cCatalog) / sizeof(cCatalog[0]);

// 二次开发（i18n）：目录表自检 —— 保证升序（乱序会导致二分查找失效）。
// 返回 false 表示表被错误插入打乱，Lookup 将退化为线性查找并仍能工作。
bool CatalogIsSorted()
{
   for (size_t i = 1; i < cCatalogSize; ++i)
   {
      if (cCatalog[i - 1].mId >= cCatalog[i].mId)
      {
         return false;
      }
   }
   return true;
}

// 二次开发（i18n）：按 ID 二分查找目录条目（表已升序；异常时线性兜底）
const ut::msg::Entry* FindEntry(uint32_t aId)
{
   if (CatalogIsSorted())
   {
      size_t lo = 0;
      size_t hi = cCatalogSize;
      while (lo < hi)
      {
         const size_t mid = lo + (hi - lo) / 2;
         if (cCatalog[mid].mId < aId)
         {
            lo = mid + 1;
         }
         else if (cCatalog[mid].mId > aId)
         {
            hi = mid;
         }
         else
         {
            return &cCatalog[mid];
         }
      }
      return nullptr;
   }
   // 表序异常兜底：线性扫描（不丢功能，便于发现维护错误）
   for (size_t i = 0; i < cCatalogSize; ++i)
   {
      if (cCatalog[i].mId == aId)
      {
         return &cCatalog[i];
      }
   }
   return nullptr;
}

// 二次开发（i18n）：占位符替换引擎 —— 把 {name} 替换为参数值。
// 未提供的占位符保留原样（排错时可看出漏传参数），不抛异常、不崩溃。
std::string Substitute(const char* aTemplate, const ut::msg::Args& aArgs)
{
   std::string result(aTemplate);
   for (const auto& kv : aArgs)
   {
      const std::string token = "{" + kv.first + "}";
      size_t       pos        = 0;
      while ((pos = result.find(token, pos)) != std::string::npos)
      {
         result.replace(pos, token.size(), kv.second);
         // 二次开发（i18n）：从替换内容之后继续查找，避免参数值本身含
         // "{name}" 时的死循环
         pos += kv.second.size();
      }
   }
   return result;
}

// 二次开发（i18n）：通道统一格式化 —— 条目缺失时回退 "[msg:ID]"，
// 保证任何情况下排错信息（ID）不丢失
std::string FormatImpl(uint32_t aId, const ut::msg::Args& aArgs, ut::msg::Lang aLang)
{
   const ut::msg::Entry* entry = FindEntry(aId);
   if (entry == nullptr)
   {
      char buf[32];
      std::snprintf(buf, sizeof(buf), "[msg:%08X]", aId);
      return std::string(buf);
   }
   const char* templ = (aLang == ut::msg::Lang::cZH) ? entry->mZh : entry->mEn;
   // 二次开发（i18n）：中文译文缺失时回退英文原文（渐进式翻译的兜底路径）
   if (templ == nullptr || templ[0] == '\0')
   {
      templ = entry->mEn;
   }
   return Substitute(templ, aArgs);
}

// 二次开发（i18n）：双通道日志输出实现 —— 前缀中的 ID 保证落盘日志
// 可 grep、可与错误码对照表互查；语言恒为英文（官方排错通道不受翻译影响）
void EmitImpl(uint32_t aId, const ut::msg::Args& aArgs, bool aIsWarning)
{
   char prefix[16];
   std::snprintf(prefix, sizeof(prefix), aIsWarning ? "[W:%08X] " : "[E:%08X] ", aId);
   const std::string text = std::string(prefix) + FormatImpl(aId, aArgs, ut::msg::Lang::cEN);
   if (aIsWarning)
   {
      ut::log::warning() << text;
   }
   else
   {
      ut::log::error() << text;
   }
}
} // namespace

namespace ut
{
namespace msg
{
const Entry* Lookup(uint32_t aId)
{
   return FindEntry(aId);
}

std::string Format(uint32_t aId, const Args& aArgs, Lang aLang)
{
   return FormatImpl(aId, aArgs, aLang);
}

std::string FormatEn(uint32_t aId, const Args& aArgs)
{
   return FormatImpl(aId, aArgs, Lang::cEN);
}

std::string FormatEnTagged(uint32_t aId, const Args& aArgs)
{
   // 二次开发（i18n）：拼接 "[E:ID] " 前缀 —— 与 Emit 的落盘格式保持一致，
   // 供需要保留 AddNote() 流式日志结构的调用点使用
   char prefix[16];
   std::snprintf(prefix, sizeof(prefix), "[E:%08X] ", aId);
   return std::string(prefix) + FormatImpl(aId, aArgs, Lang::cEN);
}

std::string FormatZh(uint32_t aId, const Args& aArgs)
{
   return FormatImpl(aId, aArgs, Lang::cZH);
}

void Emit(uint32_t aId, const Args& aArgs)
{
   EmitImpl(aId, aArgs, false);
}

void EmitWarning(uint32_t aId, const Args& aArgs)
{
   EmitImpl(aId, aArgs, true);
}

std::string GetCauseZh(uint32_t aId)
{
   // 二次开发（i18n）：原因/修复说明仅供诊断面板显示，缺失时返回空串
   const Entry* entry = FindEntry(aId);
   return (entry != nullptr && entry->mCauseZh != nullptr) ? std::string(entry->mCauseZh) : std::string();
}

std::string GetFixZh(uint32_t aId)
{
   const Entry* entry = FindEntry(aId);
   return (entry != nullptr && entry->mFixZh != nullptr) ? std::string(entry->mFixZh) : std::string();
}

// 二次开发（i18n）：模板切分 —— 把模板按 {key} 占位符切为
// 交替序列 literals[0], keys[0], literals[1], keys[1], ..., literals[N]
// （literals 数量恒比 keys 多一）。解析失败（孤立的 '{'/'}'）返回 false。
static bool SplitTemplate(const std::string&          aTemplate,
                          std::vector<std::string>&   aLiterals,
                          std::vector<std::string>&   aKeys)
{
   size_t pos = 0;
   while (true)
   {
      const size_t open = aTemplate.find('{', pos);
      if (open == std::string::npos)
      {
         aLiterals.push_back(aTemplate.substr(pos));
         return true;
      }
      const size_t close = aTemplate.find('}', open + 1);
      if (close == std::string::npos)
      {
         return false;
      }
      aLiterals.push_back(aTemplate.substr(pos, open - pos));
      aKeys.push_back(aTemplate.substr(open + 1, close - open - 1));
      pos = close + 1;
   }
}

std::string TranslateTaggedToZh(const std::string& aText)
{
   // 解析 "[E:XXXXXXXX] " / "[W:XXXXXXXX] " 前缀（长度 13）
   if (aText.size() < 13 || aText[0] != '[' ||
       (aText[1] != 'E' && aText[1] != 'W') || aText[2] != ':' || aText[11] != ']' || aText[12] != ' ')
   {
      return {};
   }
   uint32_t id = 0;
   for (int i = 0; i < 8; ++i)
   {
      const char c = aText[3 + i];
      id <<= 4;
      if (c >= '0' && c <= '9')
      {
         id |= static_cast<uint32_t>(c - '0');
      }
      else if (c >= 'A' && c <= 'F')
      {
         id |= static_cast<uint32_t>(c - 'A' + 10);
      }
      else if (c >= 'a' && c <= 'f')
      {
         id |= static_cast<uint32_t>(c - 'a' + 10);
      }
      else
      {
         return {};
      }
   }
   const std::string enText = aText.substr(13);

   const Entry* entry = FindEntry(id);
   if (entry == nullptr || entry->mEn == nullptr || entry->mZh == nullptr)
   {
      return {};
   }

   // 二次开发（i18n）：反提取 —— 用英文模板定位各参数在英文填充文本中的值。
   // 顺序消费模型：key 前的字面量必须紧贴当前位置，key 的值延伸到
   // 下一字面量首次出现处。任何一步失配即返回空串（调用方回退英文原文）。
   std::vector<std::string> literals;
   std::vector<std::string> keys;
   if (!SplitTemplate(std::string(entry->mEn), literals, keys))
   {
      return {};
   }
   Args args;
   size_t textPos = 0;
   for (size_t i = 0; i < keys.size(); ++i)
   {
      const std::string& lit = literals[i];
      if (enText.compare(textPos, lit.size(), lit) != 0)
      {
         return {};
      }
      textPos += lit.size();
      const std::string& nextLit = literals[i + 1];
      // 二次开发（i18n）：占位符位于模板末尾（尾字面量为空串）时，
      // 参数值延伸到英文文本末尾（find("") 会返回当前位置导致截断，须特判）
      const size_t valueEnd = nextLit.empty() ? enText.size() : enText.find(nextLit, textPos);
      if (valueEnd == std::string::npos)
      {
         return {};
      }
      args[keys[i]] = enText.substr(textPos, valueEnd - textPos);
      textPos = valueEnd;
   }
   const std::string& tail = literals.back();
   if (enText.compare(textPos, tail.size(), tail) != 0 ||
       textPos + tail.size() != enText.size())
   {
      return {};
   }

   // 反提取成功：填充中文模板（占位符缺失时保留 {key} 原样，可排错）
   return Substitute(entry->mZh, args);
}
} // namespace msg
} // namespace ut
