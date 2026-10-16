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

   // ---- 想定解析段（0x0001xxxx）warning 级（第二十批）----
   // 极坐标/经纬度点与圆形区域共用（ZoneDefinition）
   {ut::msg::MSG_W_ZONE_POINTS_CIRCULAR,
    "Polar or lat/lon points are not intended to be used with circular zones, and may modify the zone in unexpected ways.",
    "极坐标或经纬度点不应与圆形区域共用，可能以意外方式修改该区域。",
    "圆形区域与极坐标/经纬度点定义方式不兼容",
    "圆形区域使用 radius、center 等专用命令定义"},
   // 极坐标/经纬度点与椭圆区域共用（ZoneDefinition）
   {ut::msg::MSG_W_ZONE_POINTS_ELLIPTICAL,
    "Polar or lat/lon points are not intended to be used with elliptical zones, and may modify the zone in unexpected ways.",
    "极坐标或经纬度点不应与椭圆区域共用，可能以意外方式修改该区域。",
    "椭圆区域与极坐标/经纬度点定义方式不兼容",
    "椭圆区域使用 axis 等专用命令定义"},
   // 半径/角度约束与多边形区域共用（ZoneDefinition）
   {ut::msg::MSG_W_ZONE_RADIUS_ANGLE_POLY,
    "Radius or angle constraints are not intended to be used with polygonal zones, and may modify the zone in unexpected ways.",
    "半径或角度约束不应与多边形区域共用，可能以意外方式修改该区域。",
    "多边形区域不支持半径/角度约束",
    "多边形区域仅使用 point 命令定义顶点"},
   // 极坐标/经纬度点与球形区域共用（ZoneDefinition）
   {ut::msg::MSG_W_ZONE_POINTS_SPHERICAL,
    "Polar or lat/lon points are not intended to be used with spherical zones, and may modify the zone in unexpected ways.",
    "极坐标或经纬度点不应与球形区域共用，可能以意外方式修改该区域。",
    "球形区域与极坐标/经纬度点定义方式不兼容",
    "球形区域使用 radius、center 等专用命令定义"},
   // 命令不适用于多边形区域（ZoneDefinition 4 处共用，参数化）
   {ut::msg::MSG_W_ZONE_CMD_NOT_POLY,
    "Command {command} is not intended to be used with polygonal zones, and may modify the zone in unexpected ways.",
    "命令 {command} 不适用于多边形区域，可能以意外方式修改该区域。",
    "该命令与多边形区域定义方式不兼容",
    "多边形区域仅使用 point 命令，移除不适用命令"},
   // 命令仅适用于多边形区域（ZoneDefinition 3 处共用，参数化）
   {ut::msg::MSG_W_ZONE_CMD_ONLY_POLY,
    "Command {command} is only intended to be used with polygonal zones, and may modify the zone in unexpected ways.",
    "命令 {command} 仅适用于多边形区域，可能以意外方式修改该区域。",
    "该命令只对多边形区域有意义",
    "改用多边形区域或移除该命令"},
   // 区域定义形状类型冲突（ZoneDefinition 参照区校验）
   {ut::msg::MSG_W_ZONE_SHAPE_CONFLICT,
    "Zone Definition: Shape type conflict.",
    "区域定义：形状类型冲突。",
    "区域与参照区形状类型不一致",
    "修改区域形状使其与参照区一致"},
   // 区域定义最低高度冲突
   {ut::msg::MSG_W_ZONE_MIN_ALT_CONFLICT,
    "Zone Definition: Minimum altitude conflict.",
    "区域定义：最低高度冲突。",
    "区域与参照区最低高度不一致",
    "修改 minimum_altitude 使其与参照区一致"},
   // 区域定义最高高度冲突
   {ut::msg::MSG_W_ZONE_MAX_ALT_CONFLICT,
    "Zone Definition: Maximum altitude conflict.",
    "区域定义：最高高度冲突。",
    "区域与参照区最高高度不一致",
    "修改 maximum_altitude 使其与参照区一致"},
   // 区域定义最小半径冲突
   {ut::msg::MSG_W_ZONE_MIN_RADIUS_CONFLICT,
    "Zone Definition: Minimum radius conflict.",
    "区域定义：最小半径冲突。",
    "区域与参照区最小半径不一致",
    "修改 minimum_radius 使其与参照区一致"},
   // 区域定义最大半径冲突
   {ut::msg::MSG_W_ZONE_MAX_RADIUS_CONFLICT,
    "Zone Definition: Maximum radius conflict.",
    "区域定义：最大半径冲突。",
    "区域与参照区最大半径不一致",
    "修改 maximum_radius 使其与参照区一致"},
   // 区域定义纵轴冲突
   {ut::msg::MSG_W_ZONE_LONG_AXIS_CONFLICT,
    "Zone Definition: Longitudinal axis conflict.",
    "区域定义：纵轴冲突。",
    "区域与参照区纵轴长度不一致",
    "修改 longitudinal_axis 使其与参照区一致"},
   // 区域定义横轴冲突
   {ut::msg::MSG_W_ZONE_LAT_AXIS_CONFLICT,
    "Zone Definition: Lateral axis conflict.",
    "区域定义：横轴冲突。",
    "区域与参照区横轴长度不一致",
    "修改 lateral_axis 使其与参照区一致"},
   // 区域定义起始角冲突
   {ut::msg::MSG_W_ZONE_START_ANGLE_CONFLICT,
    "Zone Definition: Start angle conflict.",
    "区域定义：起始角冲突。",
    "区域与参照区起始角不一致",
    "修改 start_angle 使其与参照区一致"},
   // 区域定义终止角冲突
   {ut::msg::MSG_W_ZONE_STOP_ANGLE_CONFLICT,
    "Zone Definition: Stop angle conflict.",
    "区域定义：终止角冲突。",
    "区域与参照区终止角不一致",
    "修改 end_angle 使其与参照区一致"},
   // 忽略多边形点列指定
   {ut::msg::MSG_W_ZONE_IGNORE_POLYGONAL,
    "Zone Definition: Ignoring polygonal specification.",
    "区域定义：忽略多边形点列指定。",
    "参照区非多边形，本区域的点列指定无效",
    "删除 point 命令或改用多边形参照区"},
   // 区域参照关系冲突
   {ut::msg::MSG_W_ZONE_REL_CONFLICT,
    "Zone Definition: Zone relationship conflict.",
    "区域定义：区域参照关系冲突。",
    "区域 internal/external 关系与参照区不一致",
    "核对 relative_to 指定"},
   // 缺少参照位置
   {ut::msg::MSG_W_ZONE_NO_REF_LOC,
    "Zone Definition: No reference location.",
    "区域定义：缺少参照位置。",
    "参照区未定义参考点，无法推导本区域位置",
    "为参照区补充 reference_location 等定义"},
   // 区域与参照区间冲突汇总
   {ut::msg::MSG_W_ZONE_REF_PROBLEMS,
    "Zone Definition: Problems encountered between zone and reference zone.",
    "区域定义：区域与参照区之间存在冲突问题。",
    "区域与参照区间存在多项参数冲突",
    "查看本条之前的消息定位各处冲突"},
   // 相对区域修饰值被忽略
   {ut::msg::MSG_W_ZONE_MODIFIERS_IGNORED,
    "Modifier values for relative zone will be ignored.",
    "相对区域的修饰值将被忽略。",
    "参照方式下 offset 等修饰值不参与衰减计算",
    "按需移除修饰值定义"},
   // heading 输入被忽略
   {ut::msg::MSG_W_ZONE_HEADING_IGNORED,
    "'heading' input will be ignored for zone-based attenuation calculations.",
    "基于区域的衰减计算将忽略 'heading' 输入。",
    "多边形区域衰减计算不使用朝向输入",
    "按需移除 heading 定义"},
   // start/end_angle 输入被忽略
   {ut::msg::MSG_W_ZONE_ANGLES_IGNORED,
    "'start_angle' and 'end_angle' input will be ignored for zone-based attenuation calculations.",
    "基于区域的衰减计算将忽略 'start_angle' 与 'end_angle' 输入。",
    "多边形区域衰减计算不使用角度范围输入",
    "按需移除角度范围定义"},
   // minimum_radius 输入被忽略
   {ut::msg::MSG_W_ZONE_MIN_RADIUS_IGNORED,
    "'minimum_radius' input will be ignored for zone-based attenuation calculations.",
    "基于区域的衰减计算将忽略 'minimum_radius' 输入。",
    "多边形区域衰减计算不使用最小半径输入",
    "按需移除 minimum_radius 定义"},
   // 无法避开区域（ZoneRouteFinder）
   {ut::msg::MSG_W_ZRF_UNABLE_AVOID,
    "Unable to avoid zone.",
    "无法避开区域。",
    "区域不是带经纬点的多边形，无法生成绕行路径",
    "改用多边形（经纬度点）定义区域"},
   // initial_run_number 被截断（WsfScenario）
   {ut::msg::MSG_W_SCN_TRUNC_INIT_RUN,
    "Truncating initial_run_number to size of random_seeds list.",
    "initial_run_number 已截断为 random_seeds 列表长度。",
    "initial_run_number 超出 random_seeds 列表长度",
    "核对 initial_run_number 与 random_seeds 列表长度"},
   // final_run_number 被截断（WsfScenario）
   {ut::msg::MSG_W_SCN_TRUNC_FINAL_RUN,
    "Truncating final_run_number to size of random_seeds list.",
    "final_run_number 已截断为 random_seeds 列表长度。",
    "final_run_number 超出 random_seeds 列表长度",
    "核对 final_run_number 与 random_seeds 列表长度"},
   // WsfScenario 添加空平台指针
   {ut::msg::MSG_W_SCN_NULL_PLATFORM,
    "WsfScenario: Attempting to add a null platform pointer.",
    "WsfScenario：试图添加空平台指针。",
    "调用方构造平台失败后未判空",
    "检查平台构造流程的返回值"},
   // WsfScenario 重复添加同一平台
   {ut::msg::MSG_W_SCN_ADD_TWICE,
    "WsfScenario: Attempting to add the same platform twice.",
    "WsfScenario：试图重复添加同一平台。",
    "同一平台对象被多次加入想定输入列表",
    "检查平台添加逻辑去重"},
   // WsfScenario 添加未命名平台
   {ut::msg::MSG_W_SCN_NO_NAME,
    "WsfScenario: Attempting to add a platform that does not have a name.",
    "WsfScenario：试图添加未命名的平台。",
    "平台未设置 name 即被加入想定",
    "为平台补充 name 定义"},
   // 派生类型被重复定义（ObjectTypeList）
   {ut::msg::MSG_W_OTL_DERIVED_REDEFINED,
    "Derived type redefined.",
    "派生类型被重复定义。",
    "同一基类型的派生类型名被多次定义",
    "检查类型定义中的重复派生名"},
   // 控制台重定向文件打开失败（WsfConsole）
   {ut::msg::MSG_W_CONSOLE_OPEN_FAIL,
    "Unable to open file.",
    "无法打开文件。",
    "控制台输出重定向的目标文件无法打开",
    "检查文件路径与写权限"},
   // PLATFORM_KILLED 事件已废弃（WsfEventOutputBase）
   {ut::msg::MSG_W_EVENTOUT_DEPRECATED,
    "The PLATFORM_KILLED event has been deprecated and will be removed in a future release. Use PLATFORM_BROKEN instead.",
    "PLATFORM_KILLED 事件已废弃，将在未来版本中移除。请改用 PLATFORM_BROKEN。",
    "旧事件名已被新事件名取代",
    "将 event_output 中 PLATFORM_KILLED 改为 PLATFORM_BROKEN"},
   // 删除空平台指针（WsfSimulation）
   {ut::msg::MSG_W_SIM_DELETE_NULL_PLATFORM,
    "WsfSimulation: Attempting to delete a null platform pointer.",
    "WsfSimulation：试图删除空平台指针。",
    "调用方传入空指针",
    "检查平台指针来源"},
   // 脚本定义已变更可能降性能（WsfSimulation）
   {ut::msg::MSG_W_SIM_SCRIPT_DEFS_CHANGED,
    "The simulation has changed script definitions which may degrade performance.",
    "仿真变更了脚本定义，可能降低性能。",
    "运行中修改脚本 scope 定义导致额外拷贝",
    "避免运行期变更脚本定义"},
   // 非期望状态无法完成仿真（WsfSimulation，原文如此）
   {ut::msg::MSG_W_SIM_COMPLETE_BAD_STATE,
    "WsfSimulation: Simulation cannot be completed. It is in an expected state.",
    "WsfSimulation：无法完成仿真，当前状态不符合要求。",
    "仿真状态非 ACTIVE/PENDING_COMPLETE",
    "核对仿真状态流转"},
   // 非期望状态无法重置仿真（WsfSimulation）
   {ut::msg::MSG_W_SIM_RESET_BAD_STATE,
    "WsfSimulation: Simulation cannot be reset. It is in an unexpected state.",
    "WsfSimulation：无法重置仿真，当前状态非预期。",
    "仿真状态非 ACTIVE",
    "核对仿真状态流转"},
   // 非期望状态无法终止仿真（WsfSimulation）
   {ut::msg::MSG_W_SIM_TERMINATE_BAD_STATE,
    "WsfSimulation: Simulation cannot be terminated. It is in an unexpected state.",
    "WsfSimulation：无法终止仿真，当前状态非预期。",
    "仿真状态非 ACTIVE",
    "核对仿真状态流转"},
   // on_platform_injection 签名错误（WsfSimulation）
   {ut::msg::MSG_W_SIM_ONPLATFORM_BAD_SIG,
    "Definition of on_platform_injection has the wrong signature.",
    "on_platform_injection 的定义签名错误。",
    "回调脚本返回值或参数类型不符",
    "按 bool(WsfPlatform) 签名修正脚本"},
   // -log-server-host 参数处理失败（WsfStandardApplication）
   {ut::msg::MSG_W_APP_LOGHOST_BAD,
    "Failed to process -log-server-host, attempting connection to localhost",
    "-log-server-host 参数处理失败，将尝试连接 localhost。",
    "命令行缺主机参数",
    "补充 -log-server-host <host>"},
   // -log-server-port 参数处理失败（WsfStandardApplication）
   {ut::msg::MSG_W_APP_LOGPORT_BAD,
    "Failed to process -log-server-port, attempting connection to 18888.",
    "-log-server-port 参数处理失败，将尝试连接 18888 端口。",
    "端口参数非法或缺失",
    "补充 -log-server-port <port>"},
   // 日志客户端连接服务器失败（WsfStandardApplication）
   {ut::msg::MSG_W_APP_LOGCONN_FAIL,
    "Log Client Failed to Connect to Logging Server",
    "日志客户端连接日志服务器失败。",
    "日志服务器未启动或地址端口错误",
    "检查日志服务器状态与 -log-server-* 参数"},
   // 帧时间超限（WsfFrameStepSimulation）
   {ut::msg::MSG_W_FSS_FRAME_EXCEEDED,
    "Exceeded frame time.",
    "帧时间超限。",
    "单帧实际执行时间超过帧时间设定",
    "增大 frame_time 或精简单帧计算量"},
   // 性能分析选项须一个参数（ProfilingApplicationExtension 2 处共用，参数化）
   {ut::msg::MSG_W_PROFILING_ARG_ONE,
    "Command-line option {option} requires exactly one argument",
    "命令行选项 {option} 需要且仅需要一个参数。",
    "该选项后缺少参数",
    "补充对应参数值"},
   // 跳出传感器更新（WsfMultiThreadManager）
   {ut::msg::MSG_W_MTM_SKIP_SENSOR_UPD,
    "Multi-Thread Manager: Skipping out of sensor updates.",
    "多线程管理器：提前跳出传感器更新。",
    "并行传感器更新超时被中断",
    "核对 BreakUpdate 时间设置"},
   // 多线程管理找不到平台（WsfMultiThreadManager）
   {ut::msg::MSG_W_MTM_PLATFORM_NOT_FOUND,
    "Multi-Thread Manager: Platform could not be found.",
    "多线程管理器：找不到平台。",
    "待更新平台在多线程列表外已被删除",
    "核对平台删除与并行更新时序"},
   // 插件加载失败（WsfPluginManager 3 处共用）
   {ut::msg::MSG_W_PLUGIN_LOAD_FAIL,
    "Failed to load plugin.",
    "插件加载失败。",
    "插件缺入口函数/版本符号或版本编译器不匹配",
    "检查插件与本程序的版本和编译器一致性"},
   // at_time 已废弃（WsfScriptContext）
   {ut::msg::MSG_W_SCRIPT_ATTIME_DEP,
    "'at_time' has been deprecated.",
    "'at_time' 已废弃。",
    "脚本回调注册使用了废弃的时间参数形式",
    "改用新的注册形式"},
   // 平台配置状态无效（WsfScriptPlatformClass）
   {ut::msg::MSG_W_SCRIPTPLAT_BAD_CONFIG,
    "Invalid configuration state.",
    "配置状态无效。",
    "脚本修改平台配置时平台已初始化",
    "在平台初始化前执行脚本配置"},
   // 隐蔽系数无效（WsfScriptPlatformClass 2 处共用）
   {ut::msg::MSG_W_SCRIPTPLAT_BAD_CONCEAL,
    "Invalid concealment factor.",
    "隐蔽系数无效。",
    "脚本设置的隐蔽系数不在 0-1 区间",
    "将系数调整到 0-1 区间"},
   // 找不到区域（WsfScriptPlatformClass）
   {ut::msg::MSG_W_SCRIPTPLAT_NO_ZONE,
    "Could not find zone.",
    "找不到区域。",
    "脚本引用的区域名未定义",
    "核对区域名"},
   // GetWaypointAt 已废弃（WsfScriptRouteClass）
   {ut::msg::MSG_W_SCRIPTROUTE_WP_AT_DEP,
    "WsfRoute.GetWaypointAt() is deprecated.",
    "WsfRoute.GetWaypointAt() 已废弃。",
    "脚本方法被新方法取代",
    "改用 Waypoint()"},
   // PauseTime 已废弃（WsfScriptRouteClass）
   {ut::msg::MSG_W_SCRIPTROUTE_PAUSE_DEP,
    "WsfRoute.PauseTime() is deprecated.",
    "WsfRoute.PauseTime() 已废弃。",
    "脚本方法被新方法取代",
    "改用 GetPauseTimeAt()"},
   // SetPauseTime 已废弃（WsfScriptRouteClass）
   {ut::msg::MSG_W_SCRIPTROUTE_SET_PAUSE_DEP,
    "WsfRoute.SetPauseTime() is deprecated.",
    "WsfRoute.SetPauseTime() 已废弃。",
    "脚本方法被新方法取代",
    "改用 SetPauseTimeAt()"},
   // 区域无法设置点列（WsfScriptZoneClass）
   {ut::msg::MSG_W_SCRIPTZONE_NO_POINTS,
    "Cannot set points on zone.",
    "无法在区域上设置点列。",
    "脚本对非多边形区域设置点列",
    "仅多边形区域支持点列"},
   // 脚本无法向 comm 发送消息（WsfScriptCommClass）
   {ut::msg::MSG_W_SCRIPTCOMM_NULL_GETTER,
    "Unable to send message to comm.",
    "无法向 comm 设备发送消息。",
    "脚本发送消息时平台或 comm 不存在",
    "核对平台名与 comm 名"},

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

   // ---- 平台/运动段（0x0002xxxx）warning 级（第二十批）----
   // precession_nutation_update_interval 已废弃（WsfPlatform）
   {ut::msg::MSG_W_PLAT_PRECESSION_DEP,
    "The precession_nutation_update_interval input keyword has been deprecated and will be removed in a future release. Use nutation_update_interval instead.",
    "precession_nutation_update_interval 输入关键字已废弃，将在未来版本移除。请改用 nutation_update_interval。",
    "旧关键字被新关键字取代",
    "将 precession_nutation_update_interval 改为 nutation_update_interval"},
   // 初始化后改 side 被拒（WsfPlatform）
   {ut::msg::MSG_W_PLAT_SIDE_CHANGE_INIT,
    "Unable to change the side of a platform after it has been initialized.",
    "平台初始化后无法更改其方别。",
    "平台已初始化，SetSide 调用被忽略",
    "在平台初始化前设置 side"},
   // 外部控制平台 SwapMover 失败（WsfPlatform）
   {ut::msg::MSG_W_PLAT_SWAPMOVER_EXT,
    "SwapMover() called on an external platform (DIS). Mover was not swapped.",
    "对外部控制平台（DIS）调用 SwapMover()，未执行运动体交换。",
    "外部控制平台运动体由外部系统管理",
    "移除对该平台 SwapMover 的调用"},
   // 发射机功率为零仍继续初始化（WsfEM_Xmtr）
   {ut::msg::MSG_W_XMTR_POWER_ZERO,
    "Allowing initialization to continue with power of 0.",
    "允许在功率为 0 的情况下继续初始化。",
    "发射机未设置功率",
    "为发射机设置非零 power"},
   // 发射机频率为零仍继续初始化（WsfEM_Xmtr）
   {ut::msg::MSG_W_XMTR_FREQ_ZERO,
    "Allowing initialization to continue with frequency of 0.",
    "允许在频率为 0 的情况下继续初始化。",
    "发射机未设置频率",
    "为发射机设置非零 frequency"},
   // 共享黑板已存在无法创建（WsfAdvancedBehaviorTreeNode，参数化）
   {ut::msg::MSG_W_ABT_CREATE_SHARED_BB,
    "CreateSharedBlackboard: Could not create the shared blackboard '{name}', it already exists!",
    "CreateSharedBlackboard：无法创建共享黑板“{name}”，该黑板已存在！",
    "同名共享黑板已被创建",
    "换用其他黑板名或先删除原黑板"},
   // 黑板变量不存在无法删除（WsfAdvancedBehaviorTreeNode，参数化）
   {ut::msg::MSG_W_ABT_DELETE_BB_VAR,
    "DeleteBlackboardVar: Could not delete the blackboard var '{var}', it doesn't exist!",
    "DeleteBlackboardVar：无法删除黑板变量“{var}”，该变量不存在！",
    "指定黑板变量未定义",
    "核对变量名"},
   // 共享黑板变量不存在无法删除（WsfAdvancedBehaviorTreeNode，参数化）
   {ut::msg::MSG_W_ABT_DEL_SBB_VAR_FROM,
    "DeleteSharedBlackboardVar: Could not delete var '{var}' from shared blackboard '{board}', it doesn't exist!",
    "DeleteSharedBlackboardVar：无法从共享黑板“{board}”删除变量“{var}”，该变量不存在！",
    "共享黑板上指定变量未定义",
    "核对变量名与黑板名"},
   // 共享黑板不存在无法删除变量（WsfAdvancedBehaviorTreeNode，参数化）
   {ut::msg::MSG_W_ABT_DEL_SBB_VAR_NO_SBB,
    "DeleteSharedBlackboardVar: Could not delete var '{var}', the shared blackboard '{board}' doesn't exist!",
    "DeleteSharedBlackboardVar：无法删除变量“{var}”，共享黑板“{board}”不存在！",
    "指定的共享黑板未创建",
    "核对黑板名"},
   // 黑板变量不存在（WsfAdvancedBehaviorTreeNode，参数化）
   {ut::msg::MSG_W_ABT_GET_BB_VAR,
    "GetBlackboardVar: The blackboard var '{var}' doesn't exist!",
    "GetBlackboardVar：黑板变量“{var}”不存在！",
    "指定黑板变量未定义",
    "核对变量名"},
   // 共享黑板上找不到变量（WsfAdvancedBehaviorTreeNode，参数化）
   {ut::msg::MSG_W_ABT_GET_SBB_VAR_ON,
    "GetSharedBlackboardVar: Could not find the var '{var}' on the shared blackboard '{board}'.",
    "GetSharedBlackboardVar：在共享黑板“{board}”上找不到变量“{var}”。",
    "共享黑板上指定变量未定义",
    "核对变量名与黑板名"},
   // 共享黑板不存在找不到变量（WsfAdvancedBehaviorTreeNode，参数化）
   {ut::msg::MSG_W_ABT_GET_SBB_VAR_NO_SBB,
    "GetSharedBlackboardVar: Could not find the var '{var}', the shared blackboard '{board}' doesn't exist!",
    "GetSharedBlackboardVar：找不到变量“{var}”，共享黑板“{board}”不存在！",
    "指定的共享黑板未创建",
    "核对黑板名"},
   // 共享黑板不存在正创建（SetVar，WsfAdvancedBehaviorTreeNode，参数化）
   {ut::msg::MSG_W_ABT_SET_SBB_VAR_CREATE,
    "SetSharedBlackboardVar: Shared blackboard '{board}' doesn't exist, creating it now...",
    "SetSharedBlackboardVar：共享黑板“{board}”不存在，正在创建……",
    "向未创建的共享黑板写变量，框架自动创建",
    "无需处理，或提前显式创建黑板"},
   // 共享黑板不存在正创建（SetTrackVar，WsfAdvancedBehaviorTreeNode，参数化）
   {ut::msg::MSG_W_ABT_SET_SBB_TRACKVAR_CREATE,
    "SetSharedBlackboardTrackVar: Shared blackboard '{board}' doesn't exist, creating it now...",
    "SetSharedBlackboardTrackVar：共享黑板“{board}”不存在，正在创建……",
    "向未创建的共享黑板写变量，框架自动创建",
    "无需处理，或提前显式创建黑板"},
   // 共享黑板不存在正创建（SetPlatformVar，WsfAdvancedBehaviorTreeNode，参数化）
   {ut::msg::MSG_W_ABT_SET_SBB_PLATVAR_CREATE,
    "SetSharedBlackboardPlatformVar: Shared blackboard '{board}' doesn't exist, creating it now...",
    "SetSharedBlackboardPlatformVar：共享黑板“{board}”不存在，正在创建……",
    "向未创建的共享黑板写变量，框架自动创建",
    "无需处理，或提前显式创建黑板"},
   // 黑板已存在无法共享（WsfAdvancedBehaviorTreeNode 2 处共用，参数化）
   {ut::msg::MSG_W_ABT_SHARE_BB_DUP,
    "ShareBlackboard: Could not share the blackboard '{name}', it already exists!",
    "ShareBlackboard：无法共享黑板“{name}”，该黑板已存在！",
    "目标平台已有同名黑板",
    "换用其他黑板名"},
   // 无法解析外部链路（WsfExternalLinks 5 处共用）
   {ut::msg::MSG_W_EXTLINK_RESOLVE,
    "Unable to resolve external link.",
    "无法解析外部链路。",
    "外部链路的接收方地址/名称无法解析",
    "核对 Notes 中地址、平台名与 comm 名"},
   // 无法解析外部链路（对等接收者，WsfExternalLinks，原文无句点）
   {ut::msg::MSG_W_EXTLINK_RESOLVE_PEER,
    "Unable to resolve external link",
    "无法解析外部链路",
    "外部链路的对等接收方无法解析",
    "核对 Notes 中平台名与 comm 名"},
   // 找不到外部链路（WsfExternalLinks）
   {ut::msg::MSG_W_EXTLINK_FIND,
    "Unable to find external link.",
    "找不到外部链路。",
    "指定分组名未定义",
    "核对 Notes 中分组名"},
   // 无法添加外部链路（WsfExternalLinks 4 处共用）
   {ut::msg::MSG_W_EXTLINK_ADD,
    "Could not add external link.",
    "无法添加外部链路。",
    "同一目标重复指定，框架忽略重复项",
    "移除重复的指定项"},
   // 无法添加组外部链路（WsfExternalLinks）
   {ut::msg::MSG_W_EXTLINK_ADD_GROUP,
    "Could not add group external link.",
    "无法添加组外部链路。",
    "同一目标重复指定，框架忽略重复项",
    "移除重复的指定项"},
   // 外部链路消息发送中止（WsfExternalLinks 2 处共用）
   {ut::msg::MSG_W_EXTLINK_SEND_ABORT,
    "External link message send aborted.",
    "外部链路消息发送中止。",
    "发送方与接收方为同一地址",
    "核对接收方地址设置"},
   // 平台部件无法链接（WsfInternalLinks）
   {ut::msg::MSG_W_INTLINK_PART_UNABLE,
    "Platform part unable to link.",
    "平台部件无法建立链接。",
    "接收方部件不存在",
    "核对接收方部件定义"},
   // 无效地图配置命令（WsfTerrain）
   {ut::msg::MSG_W_TERRAIN_INVALID_MAP_CMD,
    "Invalid map configuration command.",
    "无效的地图配置命令。",
    "map 命令行首字符非 '#' 且非已知命令",
    "按文档核对 map 配置命令"},
   // 地形接口不认识的查询类型（WsfTerrain）
   {ut::msg::MSG_W_TERRAIN_QUERY_UNKNOWN,
    "Query type not recognized by terrain interface.",
    "地形接口无法识别该查询类型。",
    "内部查询枚举扩展后未同步处理",
    "联系开发方核对查询类型"},
   // 移交轨迹初始化失败（WsfTrackManager）
   {ut::msg::MSG_W_TM_HANDOFF_INIT_FAIL,
    "Could not initialize the hand-off track.",
    "无法初始化移交轨迹。",
    "源轨迹为空或移交对象构造失败",
    "核对源轨迹状态"},
   // show_transitions 已废弃（WsfTrackStateController）
   {ut::msg::MSG_W_TSC_SHOW_TRANSITIONS_DEP,
    "'show_transitions' has been deprecated. Use 'show_state_transitions'.",
    "'show_transitions' 已废弃。请改用 'show_state_transitions'。",
    "旧命令名被新命令名取代",
    "改用 show_state_transitions"},
   // attribute 容器 attribute 命令已废弃（WsfAttributeContainer）
   {ut::msg::MSG_W_ATTR_ATTRIBUTE_DEP,
    "WsfAttributeContainer: Input command 'attribute' is deprecated.",
    "WsfAttributeContainer：输入命令 'attribute' 已废弃。",
    "attribute 命令为兼容保留，现仅需类型名",
    "移除 attribute 关键字，保留类型名"},
   // default 状态不存在（WsfMaskingPattern）
   {ut::msg::MSG_W_MASK_DEFAULT_STATE_MISSING,
    "The state 'default' does not exist.",
    "状态 'default' 不存在。",
    "遮蔽模式未定义 default 状态",
    "在定义中补充 default 状态"},
   // 按用户输入禁用接口（WsfNetworkInterface）
   {ut::msg::MSG_W_NETIF_DISABLED,
    "Disabling interface due to user input.",
    "按用户输入禁用接口。",
    "接口定义中包含 disable 类命令",
    "按需移除 disable 命令"},
   // default 类型 subtype 定义被忽略（WsfMessageTable）
   {ut::msg::MSG_W_MSGTABLE_SUBTYPE_IGNORED,
    "'subtype' definitions are not applicable for the 'default' type and will be ignored.",
    "'subtype' 定义不适用于 'default' 类型，将被忽略。",
    "default 类型不支持 subtype 限定",
    "移除 default 类型上的 subtype 定义"},
   // 排除区未定义（WsfPlatformAvailability）
   {ut::msg::MSG_W_PAVAIL_EXCL_ZONE_UNDEF,
    "Undefined platform_availability exclusion_zone.",
    "platform_availability 的排除区未定义。",
    "引用的区域名未在想定中定义",
    "核对 Notes 中区域名"},
   // 包含区未定义（WsfPlatformAvailability）
   {ut::msg::MSG_W_PAVAIL_INCL_ZONE_UNDEF,
    "Undefined platform_availability inclusion_zone.",
    "platform_availability 的包含区未定义。",
    "引用的区域名未在想定中定义",
    "核对 Notes 中区域名"},
   // LOS 管理器状态数据不一致（WsfLOS_Manager）
   {ut::msg::MSG_W_LOSM_INCONSISTENT_STATE,
    "LOS Manager: Inconsistent state data found for ID pair. Skipping.",
    "LOS 管理器：发现 ID 对的状态数据不一致，跳过处理。",
    "内部状态表与请求 ID 不匹配",
    "如频繁出现请联系开发方"},
   // 雷达特征未定义用默认（WsfRadarSignature 2 处共用）
   {ut::msg::MSG_W_RADARSIG_UNDEF,
    "Undefined radar signature.",
    "雷达特征未定义。",
    "平台类型未定义雷达特征，使用默认值",
    "为平台类型补充 radar_signature 定义"},
   // 特征状态未定义（WsfSignatureList，参数化）
   {ut::msg::MSG_W_SIGLIST_STATE_UNDEF,
    "Undefined {className} state.",
    "{className} 状态未定义。",
    "特征列表未定义指定状态，使用默认值",
    "为特征定义补充对应状态"},
   // 铰接部件无法提供当前指向（WsfEquatorialFieldOfView）
   {ut::msg::MSG_W_EQFOV_NO_CUE,
    "WsfArticulatedPart not able to give current cue for articulated part using equatorial field of view.",
    "WsfArticulatedPart 无法为使用赤道视场的铰接部件提供当前指向。",
    "铰接部件不支持赤道视场的指向查询",
    "核对铰接部件的视场配置"},
   // SharedBlackboardVarExists 黑板不存在（WsfAdvancedBehaviorTreeNode，参数化）
   {ut::msg::MSG_W_ABT_SBBVAR_EXISTS_NO_SBB,
    "SharedBlackboardVarExists: Could not find shared blackboard var '{var}', the shared blackboard '{board}' doesn't exist!",
    "SharedBlackboardVarExists：无法查找共享黑板变量“{var}”，共享黑板“{board}”不存在！",
    "指定的共享黑板未创建",
    "核对黑板名"},
   // 初始油量大于最大油量被约束（WsfFuel）
   {ut::msg::MSG_W_FUEL_GT_MAX,
    "Initial fuel quantity was greater than maximum fuel quantity. Fuel was constrained to the maximum.",
    "初始油量大于最大油量，已按最大油量约束。",
    "quantity 定义值超过 maximum_quantity",
    "调整初始油量不超上限"},
   // 初始油量为零或负（WsfFuel）
   {ut::msg::MSG_W_FUEL_ZERO_INIT,
    "Initial fuel quantity is zero or less.",
    "初始油量为零或负值。",
    "quantity 定义值不大于 0",
    "补充正的初始油量"},
   // 暂停状态无法执行航线（WsfWaypointMover）
   {ut::msg::MSG_W_WPMOVER_PAUSED,
    "Platform cannot execute route while movement is paused.",
    "平台在运动暂停期间无法执行航线。",
    "暂停状态调用 GoToRoute",
    "先 Resume 再执行航线"},
   // tether 附着类型仍在开发（WsfOffsetMover）
   {ut::msg::MSG_W_OFFSETMOVER_TETHER,
    "'attachment_type tether' is still in development.",
    "'attachment_type tether' 仍在开发中。",
    "tether 附着类型尚未成熟",
    "谨慎使用该功能"},
   // 航线未加入路径网络（WsfRouteNetwork，参数化）
   {ut::msg::MSG_W_ROUTENET_NOT_ADDED,
    "Route not added to route_network {name}",
    "航线未加入路径网络 {name}",
    "航线定义不完整（可能少于两个航点）",
    "核对航线航点数量"},
   // 无法求解最短路径（WsfRouteNetwork）
   {ut::msg::MSG_W_ROUTENET_NO_SHORTPATH,
    "Could not solve shortest path.",
    "无法求解最短路径。",
    "起终点间不存在连通路径",
    "核对路径网络的连通性"},
   // 航点经纬度超出范围（WsfRouteMover）
   {ut::msg::MSG_W_ROUTEMOVER_BAD_LATLON,
    "Attempting to add an invalid lat/lon to the route of a platform.",
    "试图向平台航线添加无效的经纬度。",
    "航点纬度/经度超出合法范围",
    "修正航点坐标"},
   // 导航时间历史文件打开失败（WsfNavigationErrors）
   {ut::msg::MSG_W_NAVERR_FILE_OPEN,
    "Unable to open navigation time history file for platform.",
    "无法为平台打开导航时间历史文件。",
    "输出文件路径无法创建或无写权限",
    "核对文件路径与权限"},
   // 加油供给配置歧义（WsfTankedFuel）
   {ut::msg::MSG_W_TANKEDFUEL_AMBIGUOUS,
    "Refueling Supply configuration ambiguous.",
    "加油供给配置存在歧义。",
    "供给量或优先级等定义不明确",
    "按文档明确供给配置"},
   // transform_route 已废弃（WsfRoute）
   {ut::msg::MSG_W_ROUTE_TRANSFORM_DEP,
    "transform_route has been deprecated and will be removed, use insert_route instead.",
    "transform_route 已废弃并将移除，请改用 insert_route。",
    "旧命令被新命令取代",
    "改用 insert_route"},
   // switch_on_passing 未实现被忽略（WsfMath3D_Mover）
   {ut::msg::MSG_W_MATH3D_SWITCH_ON_PASSING,
    "The 'switch_on_passing' keyword is not implemented by WSF_KINEMATIC_MOVER. Ignored.",
    "WSF_KINEMATIC_MOVER 未实现 'switch_on_passing' 关键字，已忽略。",
    "该运动体类型不支持此关键字",
    "移除该关键字或换用支持的运动体"},
   // altitude_offset 未实现被忽略（WsfMath3D_Mover）
   {ut::msg::MSG_W_MATH3D_ALT_OFFSET,
    "The 'altitude_offset' keyword is not implemented by WSF_KINEMATIC_MOVER. Ignored.",
    "WSF_KINEMATIC_MOVER 未实现 'altitude_offset' 关键字，已忽略。",
    "该运动体类型不支持此关键字",
    "移除该关键字或换用支持的运动体"},
   // on_turn_failure 未实现被忽略（WsfMath3D_Mover，参数化）
   {ut::msg::MSG_W_MATH3D_TURN_FAILURE,
    "The 'on_turn_failure' and sub-keyword {option} are not implemented by WSF_KINEMATIC_MOVER. Ignored.",
    "WSF_KINEMATIC_MOVER 未实现 'on_turn_failure' 及子关键字 {option}，已忽略。",
    "该运动体类型不支持此关键字",
    "移除该关键字或换用支持的运动体"},
   // 轨迹请求的传感器模式无效（WsfTaskManager）
   {ut::msg::MSG_W_TASKMGR_BAD_SENSOR_MODE,
    "Invalid sensor mode requested for track.",
    "为轨迹请求的传感器模式无效。",
    "任务指定的模式名不在传感器模式列表中",
    "核对传感器模式名"},
   // StartTracking 失败（WsfTaskManager）
   {ut::msg::MSG_W_TASKMGR_STARTTRACK_FAIL,
    "Sensor failed StartTracking for track.",
    "传感器对轨迹执行 StartTracking 失败。",
    "传感器未能开始跟踪指定轨迹",
    "核对传感器能力与目标状态"},
   // StopTracking 失败（WsfTaskManager）
   {ut::msg::MSG_W_TASKMGR_STOPTRACK_FAIL,
    "Sensor failed StopTracking for track.",
    "传感器对轨迹执行 StopTracking 失败。",
    "传感器未能停止跟踪指定轨迹",
    "核对传感器当前跟踪状态"},
   // 上报任务完成状态（WsfTaskManager）
   {ut::msg::MSG_W_TASKMGR_REPORT_COMPLETE,
    "Reporting completion status for task.",
    "正在上报任务完成状态。",
    "完成状态对应的任务不在活动列表中",
    "核对任务列表状态"},
   // 上报进度状态找不到受派者（WsfTaskManager）
   {ut::msg::MSG_W_TASKMGR_REPORT_PROGRESS,
    "Reporting progress status. Could not find Track's Assignee Platform or Resource",
    "上报进度状态：找不到轨迹的受派平台或资源",
    "任务受派方已不存在",
    "核对任务分配链路"},
   // 消息超时（WsfTaskManager）
   {ut::msg::MSG_W_TASKMGR_MSG_TIMEOUT,
    "Message timed out.",
    "消息已超时。",
    "在超时时限内未收到响应",
    "核对通信链路与接收方状态"},
   // 找不到任务（WsfTaskManager）
   {ut::msg::MSG_W_TASKMGR_NO_TASK,
    "Could not find task.",
    "找不到任务。",
    "指定任务已不在任务列表中",
    "核对任务列表"},
   // WSF_TRACK_MANAGER 已废弃（WsfProcessorTypes 2 处共用）
   {ut::msg::MSG_W_PROCTYPES_TM_DEP,
    "WSF_TRACK_MANAGER is deprecated and will be removed in a future release. Use WSF_TRACK_PROCESSOR instead.",
    "WSF_TRACK_MANAGER 已废弃并将移除。请改用 WSF_TRACK_PROCESSOR。",
    "旧类型名被新类型名取代",
    "将 WSF_TRACK_MANAGER 改为 WSF_TRACK_PROCESSOR"},
   // 传感器处理器找不到要挂接的传感器（WsfSensorProcessor）
   {ut::msg::MSG_W_SENPROC_ATTACH_FAIL,
    "Sensor processor could not find sensor to attach.",
    "传感器处理器找不到要挂接的传感器。",
    "指定平台或传感器不存在",
    "核对平台名与传感器名"},
   // 传感器处理器找不到要移除的传感器（WsfSensorProcessor）
   {ut::msg::MSG_W_SENPROC_REMOVE_FAIL,
    "Sensor processor could not find sensor to remove.",
    "传感器处理器找不到要移除的传感器。",
    "指定平台或传感器不存在",
    "核对平台名与传感器名"},
   // 无外部通信链路不上报轨迹（WsfTrackProcessor）
   {ut::msg::MSG_W_TRACKPROC_NO_EXTLINKS,
    "Track processor will not report tracks as it has no external communication links.",
    "轨迹处理器没有外部通信链路，不会上报轨迹。",
    "设置了上报间隔但未定义外部链路",
    "补充 external_links 定义或去掉上报设置"},
   // ---- 平台/签名/mover/processor 段 warning 级（第二十批子批 C wsf_mil）----
   // 武器平台扩展爆炸效果无法初始化交战（WsfExtDetonation）
   {ut::msg::MSG_W_EXTDETON_INIT_FAIL,
    "WsfExtDetonation unable to initialize weapon engagement.",
    "WsfExtDetonation 无法初始化武器交战。",
    "武器交战对象创建失败，回退为简单爆炸",
    "核对武器与交战相关定义的完整性"},
   // 未定义声学签名用默认（WsfAcousticSignature::GetNoisePressure）
   {ut::msg::MSG_W_ACOUSTIC_NOT_DEF,
    "Acoustic_signature not defined. Using default.",
    "未定义声学签名，使用默认值。",
    "平台未定义声学签名",
    "在平台定义中补充 acoustic_signature 或确认默认值可用"},
   // 未定义声学签名用默认（带引号变体，WsfAcousticSignature::GetValue）
   {ut::msg::MSG_W_ACOUSTIC_NOT_DEF2,
    "'acoustic_signature' not defined. Using default.",
    "未定义 'acoustic_signature'，使用默认值。",
    "平台未定义声学签名",
    "在平台定义中补充 acoustic_signature 或确认默认值可用"},
   // 平台声学签名未定义用默认（参数化，WsfAcousticSignature::GetValue 流式拼接）
   {ut::msg::MSG_W_ACOUSTIC_PLAT_VAR,
    "{plat}.{type}: acoustic_signature not defined; using default.",
    "{plat}.{type}：未定义 acoustic_signature，使用默认值。",
    "平台未定义声学签名",
    "在平台定义中补充 acoustic_signature 或确认默认值可用"},
   // 红外签名由组合光学签名模型提供（WsfCompositeOpticalSignature）
   {ut::msg::MSG_W_COMPOPT_IR_BY_OPT,
    "The infrared signature will be provided by the optical signature model and not the Infrared Signature.",
    "红外签名将由光学签名模型提供，而非红外签名。",
    "使用组合光学签名模型时红外签名输入被其接管",
    "如需独立控制红外签名请改用标准红外签名模型"},
   // 双基地状态缺 default 极化（WsfBistaticSig::Initialize）
   {ut::msg::MSG_W_BISTATIC_NO_DEF_POL,
    "The 'default' polarization does not exist for bistatic state.",
    "双基地签名状态缺少 'default' 极化。",
    "双基地签名状态未定义默认极化",
    "在签名状态定义中补充 default 极化数据"},
   // 双基地状态方位/俯仰集不足（WsfBistaticSig::Initialize）
   {ut::msg::MSG_W_BISTATIC_MIN_AZEL,
    "Bistatic signature state does not have the required minimum of at least two azimuth/elevation sets defined.",
    "双基地签名状态未定义至少两组方位/俯仰数据，不满足最低要求。",
    "方位/俯仰插值表维度不足",
    "补充方位/俯仰数据至至少两维"},
   // 天线高度低于浪高（WsfEM_SurfaceClutter）
   {ut::msg::MSG_W_SURFCLUT_LOW_ALT,
    "surface_clutter: Antenna altitude is too low compared to wave height, and may produce undesired results.",
    "surface_clutter：天线高度低于浪高，可能产生非预期结果。",
    "海况对应的浪高高于天线安装高度",
    "调整天线高度或降低海况等级"},
   // 红外签名未定义用默认（WsfInfraredSignature）
   {ut::msg::MSG_W_IR_NOT_DEF,
    "Platform's Infrared Signature not defined. Using default.",
    "平台未定义红外签名，使用默认值。",
    "平台未定义红外签名",
    "在平台定义中补充 infrared_signature 或确认默认值可用"},
   // 雷达签名缺单基地定义（WsfMilRadarSignature::Initialize）
   {ut::msg::MSG_W_MILRADAR_NO_MONO,
    "Radar Signature's 'mono-static signature' not defined, bistatic signature will be used for mono-static cases.",
    "雷达签名未定义 'mono-static signature'，单基地场景将使用双基地签名。",
    "只定义了双基地签名而未定义单基地签名",
    "补充 mono-static signature 定义或确认可接受回退行为"},
   // 雷达签名缺 default 状态（WsfMilRadarSignature::Initialize）
   {ut::msg::MSG_W_MILRADAR_NO_DEF_STATE,
    "The Radar Signature state 'default' does not exist.",
    "雷达签名状态 'default' 不存在。",
    "雷达签名未定义 default 状态",
    "在签名定义中补充 default 状态"},
   // 雷达签名状态缺 default 极化（WsfMilRadarSignature::Initialize）
   {ut::msg::MSG_W_MILRADAR_NO_DEF_POL,
    "The 'default' polarization does not exist for state.",
    "签名状态缺少 'default' 极化。",
    "雷达签名状态未定义默认极化",
    "在状态定义中补充 default 极化数据"},
   // 固有对比度未定义用默认（WsfInherentContrast）
   {ut::msg::MSG_W_INHCON_NOT_DEF,
    "Platform's Inherent Contrast not defined. Using Default.",
    "平台未定义固有对比度，使用默认值。",
    "平台未定义固有对比度",
    "在平台定义中补充 inherent_contrast 或确认默认值可用"},
   // 光学签名未定义用默认（WsfOpticalSignature）
   {ut::msg::MSG_W_OPTSIG_NOT_DEF,
    "Optical signature not defined. Using default.",
    "未定义光学签名，使用默认值。",
    "平台未定义光学签名",
    "在平台定义中补充 optical_signature 或确认默认值可用"},
   // 光学反射率未定义用默认（WsfOpticalReflectivity，忠实保留原文错拼 no）
   {ut::msg::MSG_W_OPTREFL_NOT_DEF,
    "Optical reflectivity no defined. Using default.",
    "未定义光学反射率，使用默认值。",
    "平台未定义光学反射率",
    "在平台定义中补充 optical_reflectivity 或确认默认值可用"},
   // 无法打开时间历史文件（WsfGuidedMoverBase）
   {ut::msg::MSG_W_GMB_NO_HISTFILE,
    "Unable to open time history file.",
    "无法打开时间历史文件。",
    "输出文件路径不存在或无写入权限",
    "核对 time_history 输出路径与磁盘权限"},
   // 级间分段消息（WsfGuidedMoverBase 2 处共用：分离失败/点火失败）
   {ut::msg::MSG_W_GMB_STAGING_MSG,
    "Staging Message:",
    "级间分段消息：",
    "多级飞行器级间分离或点火失败",
    "查看附注中的具体失败原因并核对分段定义"},
   // RK 积分精度受限（WsfGuidedMover::Initialize）
   {ut::msg::MSG_W_GM_RK_ACCURACY,
    "WSF_GUIDED_MOVER Runge-Kutta integration without selecting 'compute_all_forces_each_substep' is less accurate.",
    "WSF_GUIDED_MOVER 未选择 'compute_all_forces_each_substep' 时 Runge-Kutta 积分精度较低。",
    "RK 积分方法未启用每子步受力计算",
    "按文档建议设置 compute_all_forces_each_substep true"},
   // 燃烧率可能计算有误（WsfGuidedMover）
   {ut::msg::MSG_W_GM_BAD_BURNRATE,
    "Potentially erroneous 'burn_rate' calculated.",
    "计算的 'burn_rate' 可能有误。",
    "推力/比冲参数不完整导致燃烧率回退计算",
    "核对 thrust/specific_impulse 等输入定义"},
   // 无法计算拦截点（WsfWeaponTrackProcessor）
   {ut::msg::MSG_W_WTP_NO_INTERCEPT,
    "Unable to calculate intercept location. Target track may not be a valid 3-D track.",
    "无法计算拦截点。目标轨迹可能不是有效的三维轨迹。",
    "目标轨迹缺少三维位置数据",
    "核对目标轨迹质量与传感器探测维度"},
   // 威胁轨迹无有效位置（WsfWeaponThreatProcessor::CreateThreatList）
   {ut::msg::MSG_W_WTP_NO_VALID_LOC,
    "CreateThreatList: Track has no 'valid' location. Skipping.",
    "CreateThreatList：轨迹无有效位置，跳过。",
    "威胁轨迹位置未标记为有效",
    "核对轨迹生成源的位置质量"},
   // 威胁轨迹无有效速度（WsfWeaponThreatProcessor::CreateThreatList）
   {ut::msg::MSG_W_WTP_NO_VALID_VEL,
    "CreateThreatList: Track has no 'valid' velocity. Skipping.",
    "CreateThreatList：轨迹无有效速度，跳过。",
    "威胁轨迹速度未标记为有效",
    "核对轨迹生成源的速度质量"},
   // 无法访问分类处理器（WsfWeaponThreatProcessor::CreateThreatList）
   {ut::msg::MSG_W_WTP_NO_CLASSIFIER,
    "CreateThreatList: No access to ClassifierProcessor.",
    "CreateThreatList：无法访问分类处理器。",
    "指定的轨迹分类处理器不在所属平台上",
    "核对 track_classifier 指定的处理器名与平台定义"},
   // 粗更新间隔须大于精更新间隔（WsfWeaponFuse::Initialize）
   {ut::msg::MSG_W_FUSE_GROSS_LT_FINE,
    "The 'gross_update_interval' must be greater than 'fine_update_interval'.",
    "'gross_update_interval' 必须大于 'fine_update_interval'。",
    "粗更新间隔配置小于精更新间隔",
    "调整间隔取值或按附注以精更新间隔为准"},
   // 引信命令已废弃（WsfWeaponFuse::ProcessInput）
   {ut::msg::MSG_W_FUSE_DISABLE_DEP,
    "'disable_launch_proximity_detonation' has been deprecated.",
    "'disable_launch_proximity_detonation' 已废弃。",
    "引信输入命令被新机制取代",
    "参考文档改用现行命令"},
   // 图像处理器不处理该消息（WsfImageProcessor::TurnOn）
   {ut::msg::MSG_W_IMGPROC_NO_PROCESS,
    "The WSF_IMAGE_PROCESSOR will not process.",
    "WSF_IMAGE_PROCESSOR 将不处理。",
    "收到的消息类型不在处理器订阅范围内",
    "核对消息类型与处理器的订阅配置"},
   // 齐射失败（WsfWeaponTaskManager）
   {ut::msg::MSG_W_WTM_FIRESALVO_FAIL,
    "Failed FireSalvo for track.",
    "对轨迹齐射失败。",
    "齐射任务执行失败（武器不可用或分配失败）",
    "核对武器状态与任务分配"},
   // 按频段停扰失败（WsfWeaponTaskManager）
   {ut::msg::MSG_W_WTM_STOPJAM_FREQ_FAIL,
    "Failed StopJamming for frequency.",
    "按频段停止干扰失败。",
    "停扰资源分配失败（频段/波束不匹配）",
    "核对干扰资源频段配置"},
   // 按轨迹停扰失败（WsfWeaponTaskManager）
   {ut::msg::MSG_W_WTM_STOPJAM_TRACK_FAIL,
    "Failed StopJamming on Track Id.",
    "按轨迹停止干扰失败。",
    "停扰任务对应的轨迹不存在或已失效",
    "核对轨迹编号与任务状态"},
   // 运动器类型错误制导关闭（WsfGuidanceComputer::Initialize）
   {ut::msg::MSG_W_GUIDANCE_BAD_MOVER,
    "Mover is of the incorrect type. Turning guidance off.",
    "运动器类型不正确，关闭制导。",
    "平台运动器不是 WSF_GUIDED_MOVER 派生类型",
    "为平台配置 WSF_GUIDED_MOVER 类型运动器"},

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

   // ---- 传感器段（0x0003xxxx）warning 级（第二十批）----
   // 扇区定义数据不足（WsfSectorScanSensorScheduler）
   {ut::msg::MSG_W_SECTORSCAN_INSUFFICIENT,
    "Insufficient data in sector definition.",
    "扇区定义数据不足。",
    "扇区扫描调度器所需参数缺失",
    "补全扇区扫描模式定义"},
   // 传感器开机时不允许改从属状态（WsfSensor）
   {ut::msg::MSG_W_SENSOR_SLAVE_WHEN_ON,
    "Changing sensor slave status is not currently allowed while the sensor is on.",
    "传感器开机时不允许更改从属状态。",
    "开机状态调用 SetSlaveRequested",
    "先关机再设置从属状态"},
   // 传感器模式不上报任何内容（WsfSensorMode）
   {ut::msg::MSG_W_SENSORMODE_NOT_REPORT,
    "Sensor does not report anything.",
    "传感器不会上报任何内容。",
    "模式未定义任何可见性输出",
    "按需为模式补充可见性定义"},
   // 传感器模式无法上报全部请求字段（WsfSensorMode）
   {ut::msg::MSG_W_SENSORMODE_MISSING_FIELDS,
    "Platform's Sensor in Mode is unable to report all requested fields.",
    "平台的传感器在该模式下无法上报全部请求字段。",
    "mode_definition 请求的字段超出模式能力",
    "核对请求字段与模式定义"},
   // 初始方位角与随机起始冲突（WsfSpinSensorScheduler，参数化）
   {ut::msg::MSG_W_SPIN_RANDOM_AZ_IGNORED,
    "Initial azimuth angle set to {angle} while scheduler set to use randomized starting azimuth on sensor: {sensor} for platform: {platform}. Possible loss of user data.",
    "已将初始方位角设置为 {angle}，但调度器被设置为使用随机起始方位。传感器：{sensor}，平台：{platform}。可能丢失用户数据。",
    "start_azimuth 与 randomized_start_azimuth 同时定义",
    "去掉其中一个定义"},
   // 随机起始方位覆盖了初始角设置（WsfSpinSensorScheduler，参数化）
   {ut::msg::MSG_W_SPIN_RANDOM_AZ_CONFLICT,
    "Initial azimuth angle set to randomized starting azimuth while scheduler was set to an initial angle of {angle} while scheduler set to use randomized starting azimuth on sensor: {sensor} for platform: {platform}. Possible loss of user data.",
    "初始方位角已设置为随机起始方位，而调度器原先设置的初始角为 {angle}（调度器同时被设置为在传感器上使用随机起始方位：传感器 {sensor}，平台 {platform}）。可能丢失用户数据。",
    "randomized_start_azimuth 覆盖了 start_azimuth 设置",
    "去掉其中一个定义"},
   // signal_processor 与显式探测参数冲突（WsfRadarSensor）
   {ut::msg::MSG_W_RADAR_SIGPROC_CONFLICT,
    "'signal_processor' should not be used with 'one_m2_detect_range', 'range_product', or 'loop_gain'",
    "'signal_processor' 不应与 'one_m2_detect_range'、'range_product' 或 'loop_gain' 同时使用。",
    "signal_processor 模式与显式探测距离参数互斥",
    "移除其中一方的定义"},
   // ---- 传感器段 warning 级（第二十批子批 C wsf_mil）----
   // SAR 分辨率低于近似最小值（WsfSAR_Sensor）
   {ut::msg::MSG_W_SAR_MIN_RES,
    "Requested/computed resolution is less than approximate minimum.",
    "请求/计算的分辨率低于近似最小值。",
    "波束宽度决定的分辨率下限高于设定值",
    "增大分辨率取值或调整天线波束宽度"},
   // SAR angular_resolution 已废弃（WsfSAR_Sensor::ProcessInput）
   {ut::msg::MSG_W_SAR_ANGRES_DEP,
    "WSF_SAR_SENSOR: angular_resolution has been deprecated.",
    "WSF_SAR_SENSOR：angular_resolution 已废弃。",
    "命令被 resolution/dwell_time 取代",
    "改用 'resolution' 或 'dwell_time' 命令"},
   // 双基地误差模型忽略输入距离误差（WsfBistaticErrorModel::Initialize）
   {ut::msg::MSG_W_BISTERR_RANGE_IGNORE,
    "BistaticErrorModel: Range error will be computed by the model; the provided value will be ignored.",
    "BistaticErrorModel：距离误差将由模型计算，提供的值将被忽略。",
    "配置了显式距离误差标准差但模型会自行计算",
    "去掉 range_error 输入或确认模型计算结果可接受"},
   // OTH 电离层特征无效（WsfOTH_RadarSensor::Initialize）
   {ut::msg::MSG_W_OTH_BAD_IONO,
    "Invalid ionospheric characterization in WsfOTH_RadarSensor.",
    "WsfOTH_RadarSensor 中的电离层特征无效。",
    "电离层特征参数不完整或非法",
    "核对电离层特征定义参数"},
   // OTH 发射频率低于最低可用频率（WsfOTH_RadarSensor）
   {ut::msg::MSG_W_OTH_FREQ_LT_MUF,
    "Current Transmitter Frequency is LESS than OTH Minimum Usable Frequency for the chosen Ionospheric "
    "Conditions, transmit Signal will not propagate.",
    "当前发射频率低于所选电离层条件下的 OTH 最低可用频率，发射信号将无法传播。",
    "发射频率低于最低可用频率（MUF）",
    "提高发射频率或调整电离层特征参数"},
   // OTH signal_processor 与显式参数冲突（WsfOTH_RadarSensor）
   {ut::msg::MSG_W_OTH_SIGPROC_CONFLICT,
    "Signal_processor should not be used with one_m2_detect_range, range_product or loop_gain.",
    "Signal_processor 不应与 one_m2_detect_range、range_product 或 loop_gain 同时使用。",
    "显式检测参数与信号处理器模式互斥",
    "移除其中一方的定义"},
   // OTH 电离层距离计算覆盖距离限制（WsfOTH_RadarSensor）
   {ut::msg::MSG_W_OTH_RANGE_OVERRIDE,
    "OTH Ionospheric Constrained Range Computation will override defined min/max Range Limits.",
    "OTH 电离层距离约束计算将覆盖已定义的最小/最大距离限制。",
    "电离层约束模式下用户定义的距离限制被重算值取代",
    "确认覆盖行为可接受或调整距离限制定义"},
   // OTH 误差因子命令已废弃（参数化，WsfOTH_RadarSensor::ProcessInput 4 处共用）
   {ut::msg::MSG_W_OTH_CMD_DEP,
    "'{command}' has been deprecated.",
    "'{command}' 已废弃。",
    "误差因子命令被新机制取代",
    "参考文档改用现行命令"},

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

   // ---- 交战段（0x0004xxxx）warning 级（第二十批）----
   // ExtInterface 无法克隆平台
   {ut::msg::MSG_W_EXT_CLONE_FAIL,
    "Unable to clone platform.",
    "无法克隆平台。",
    "EXT 想定克隆平台对象失败",
    "核对源平台的定义完整性"},
   // 预分配实体 ID 已被占用
   {ut::msg::MSG_W_EXT_ENTITYID_INUSE,
    "Preassigned entity ID already in use.",
    "预分配的实体 ID 已被占用。",
    "EXT 指定的实体 ID 与现有实体冲突",
    "更换实体 ID 或移除冲突实体"},
   // ---- 电子战段 warning 级（第二十批子批 C wsf_mil ew）----
   // 选择技术失败：按索引应用时系统类型不兼容（WsfEW_EA/WsfEW_EP 2 处共用）
   {ut::msg::MSG_W_EW_SEL_APPLY_IDX,
    "Select Technique Failed. Could not apply the technique at index due to an incompatible system type.",
    "选择技术失败：因系统类型不兼容，无法应用该索引处的技术。",
    "按索引选择技术时该技术的适用系统类型与请求不符",
    "核对技术定义中的 system 类型与请求的系统类型"},
   // 选择技术失败：索引处无有效技术（WsfEW_EA/WsfEW_EP 2 处共用）
   {ut::msg::MSG_W_EW_SEL_NO_VALID_IDX,
    "Select Technique Failed. Could not find a valid technique at index for system type.",
    "选择技术失败：该系统类型在索引处无有效技术。",
    "指定索引的技术未定义或不适用于该系统类型",
    "核对技术列表定义与索引取值"},
   // 选择技术失败：索引超限（WsfEW_EA/WsfEW_EP 2 处共用）
   {ut::msg::MSG_W_EW_SEL_IDX_TOO_LARGE,
    "Select Technique Failed. Index too large.",
    "选择技术失败：索引过大。",
    "请求的技术索引超出技术列表上限",
    "核对索引取值是否在技术列表范围内"},
   // 选择技术失败：系统类型不兼容（WsfEW_EP，忠实保留原文无 an 变体）
   {ut::msg::MSG_W_EW_SEL_APPLY_NOAN,
    "Select Technique Failed. Could not apply technique due to incompatible system type.",
    "选择技术失败：因系统类型不兼容，无法应用技术。",
    "按 Id 选择技术时该技术的适用系统类型与请求不符",
    "核对技术定义中的 system 类型与请求的系统类型"},
   // 选择技术失败：找不到该系统类型技术（WsfEW_EA/WsfEW_EP 2 处共用）
   {ut::msg::MSG_W_EW_SEL_NOT_FOUND_ST,
    "Select Technique Failed. Could not find technique for system type.",
    "选择技术失败：找不到该系统类型的技术。",
    "指定 Id 的技术不存在",
    "核对技术 Id 是否已在技术列表中定义"},
   // 取消技术失败：按索引应用时系统类型不兼容（WsfEW_EA/WsfEW_EP 2 处共用）
   {ut::msg::MSG_W_EW_DESEL_APPLY_IDX,
    "Deselect Technique Failed. Could not apply the technique at index due to an incompatible system type.",
    "取消技术失败：因系统类型不兼容，无法应用该索引处的技术。",
    "按索引取消技术时该技术的适用系统类型与请求不符",
    "核对技术定义中的 system 类型与请求的系统类型"},
   // 取消技术失败：索引处无有效技术（WsfEW_EP/WsfEW_EA_EP 2 处共用，短版）
   {ut::msg::MSG_W_EW_DESEL_NO_VALID_IDX,
    "Deselect Technique Failed. Could not find a valid technique at index.",
    "取消技术失败：索引处无有效技术。",
    "指定索引的技术未定义或已失效",
    "核对技术列表定义与索引取值"},
   // 取消技术失败：索引超限（WsfEW_EA/WsfEW_EP/WsfEW_EA_EP 3 处共用）
   {ut::msg::MSG_W_EW_DESEL_IDX_TOO_LARGE,
    "Deselect Technique Failed. Index too large.",
    "取消技术失败：索引过大。",
    "请求的技术索引超出技术列表上限",
    "核对索引取值是否在技术列表范围内"},
   // 取消技术失败：系统类型不兼容（WsfEW_EA/WsfEW_EP 2 处共用）
   {ut::msg::MSG_W_EW_DESEL_APPLY_FAIL,
    "Deselect Technique Failed. Could not apply technique due to an incompatible system type.",
    "取消技术失败：因系统类型不兼容，无法应用技术。",
    "按 Id 取消技术时该技术的适用系统类型与请求不符",
    "核对技术定义中的 system 类型与请求的系统类型"},
   // 取消技术失败：找不到技术（WsfEW_EA/WsfEW_EA_EP 2 处共用）
   {ut::msg::MSG_W_EW_DESEL_NOT_FOUND,
    "Deselect Technique Failed. Could not find technique.",
    "取消技术失败：找不到技术。",
    "指定 Id 的技术不存在",
    "核对技术 Id 是否已在技术列表中定义"},
   // 取消技术失败：找不到该系统类型技术（WsfEW_EP）
   {ut::msg::MSG_W_EW_DESEL_NOT_FOUND_ST,
    "Deselect Technique Failed. Could not find technique for system type.",
    "取消技术失败：找不到该系统类型的技术。",
    "指定 Id 的技术不存在",
    "核对技术 Id 是否已在技术列表中定义"},
   // 选择技术失败：索引处无有效技术（WsfEW_EA_EP，忠实保留原文冒号变体）
   {ut::msg::MSG_W_EAEP_SEL_NO_VALID,
    "Select Technique Failed: Could not find a valid technique at index.",
    "选择技术失败：索引处无有效技术。",
    "指定索引的技术未定义或已失效",
    "核对技术列表定义与索引取值"},
   // 选择技术失败：索引超限（WsfEW_EA_EP，忠实保留原文冒号变体）
   {ut::msg::MSG_W_EAEP_SEL_IDX_LARGE,
    "Select Technique Failed: Index too large.",
    "选择技术失败：索引过大。",
    "请求的技术索引超出技术列表上限",
    "核对索引取值是否在技术列表范围内"},
   // 选择技术失败：找不到技术（WsfEW_EA_EP）
   {ut::msg::MSG_W_EAEP_SEL_NOT_FOUND,
    "Select Technique Failed. Could not find technique.",
    "选择技术失败：找不到技术。",
    "指定 Id 的技术不存在",
    "核对技术 Id 是否已在技术列表中定义"},
   // 无法设置增量增益调制技术（WsfEW_EA::SetDeltaGainTechnique）
   {ut::msg::MSG_W_EA_SETDELTAGAIN_FAIL,
    "WsfEW_EA::SetDeltaGainTechnique. Unable to set modulation technique.",
    "WsfEW_EA::SetDeltaGainTechnique：无法设置调制技术。",
    "增量增益效果对象未定义或未初始化",
    "核对 delta_gain 效果定义"},
   // 扫描间隔值无效（WsfFalseTarget::SetScanInterval）
   {ut::msg::MSG_W_FT_BAD_SCAN_INT,
    "WsfFalseTarget:SetScanInterval: Bad scan interval value.",
    "WsfFalseTarget:SetScanInterval：扫描间隔值无效。",
    "扫描间隔不在有效范围内",
    "核对 scan_interval 取值"},
   // 持续性值无效（WsfFalseTarget::SetPersistence）
   {ut::msg::MSG_W_FT_BAD_PERSIST,
    "WsfFalseTarget:SetPersistence: Bad persistence value.",
    "WsfFalseTarget:SetPersistence：持续性值无效。",
    "持续性系数不在 [0,1] 范围内",
    "核对 persistence 取值"},
   // 假目标初始数量重复定义（WsfFalseTarget::SetInitialNumberOfFTs）
   {ut::msg::MSG_W_FT_INIT_NFT_REDEF,
    "WsfFalseTarget::SetInitialNumberOfFTs: Already had an initial number of false targets explicitly defined.",
    "WsfFalseTarget::SetInitialNumberOfFTs：初始假目标数量已被显式定义。",
    "初始数量已显式定义后又传入不同取值",
    "核对多处初始化调用，按附注重置为最大值"},
   // 筛选器扫描模式已废弃（参数化，WsfFalseTargetScreener 2 处共用）
   {ut::msg::MSG_W_SCREEN_SCAN_DEP,
    "WSF_FALSE_TARGET_SCREENER: '{cmd}' has been deprecated, use 'initial_reject' or 'reject' depending on your needs.",
    "WSF_FALSE_TARGET_SCREENER：'{cmd}' 已废弃，按需改用 'initial_reject' 或 'reject'。",
    "扫描间筛选命令被新命令机制取代",
    "参考 WSF 文档改用现行命令"},
   // 未定义的数值类型字符串（WsfEW_Effects::EffectValue）
   {ut::msg::MSG_W_EFFECTVAL_UNDEF_TYPE,
    "EffectValue: Undefined value type string.",
    "EffectValue：未定义的数值类型字符串。",
    "效果定义中的数值类型名不在支持列表",
    "核对效果数值类型定义"},
   // 未定义的相干检查类型字符串（WsfEW_Effects::EffectValue）
   {ut::msg::MSG_W_EFFECTVAL_UNDEF_COH,
    "EffectValue: Undefined coherency check type string.",
    "EffectValue：未定义的相干检查类型字符串。",
    "效果定义中的相干检查类型名不在支持列表",
    "核对相干检查类型定义"},
   // EA 相干检查类型未定义（WsfEW_Effects::EA_CoherencyCheck）
   {ut::msg::MSG_W_EACOH_UNDEF,
    "EA_CoherencyCheck: Undefined coherency check type string.",
    "EA_CoherencyCheck：未定义的相干检查类型字符串。",
    "相干检查类型名不在支持列表",
    "核对相干检查类型定义"},
   // 无法设置增量增益效果（WsfEW_Technique::SetDeltaGainEffect）
   {ut::msg::MSG_W_TECH_SETDELTAGAIN_FAIL,
    "WsfEW_Technique::SetDeltaGainEffect: Unable to set delta gain technique.",
    "WsfEW_Technique::SetDeltaGainEffect：无法设置增量增益技术。",
    "效果对象创建或参数不匹配",
    "核对 delta_gain 效果定义"},
   // 雷达未指定脉宽（WsfEW_SimpleFT_Effect）
   {ut::msg::MSG_W_SFT_NO_PULSEWIDTH,
    "WSF_SIMPLE_FT_EFFECT: pulsewidth not specified on radar.",
    "WSF_SIMPLE_FT_EFFECT：雷达未指定脉宽。",
    "受扰雷达未定义 pulsewidth",
    "核对雷达发射机 pulsewidth 定义"},
   // 雷达未指定脉宽和/或 PRI（WsfEW_RPJ_Effect）
   {ut::msg::MSG_W_RPJ_NO_PW_PRI,
    "WSF_RPJ_EFFECT- pulsewidth and/or PRI not specified on radar.",
    "WSF_RPJ_EFFECT：雷达未指定脉宽和/或 PRI。",
    "受扰雷达未定义 pulsewidth/PRI",
    "核对雷达发射机 pulsewidth 与 PRI 定义"},
   // 雷达未指定脉宽（WsfEW_FalseTargetEffect）
   {ut::msg::MSG_W_FT_NO_PULSEWIDTH,
    "WSF_FT_EFFECT: Pulsewidth not specified on radar.",
    "WSF_FT_EFFECT：雷达未指定脉宽。",
    "受扰雷达未定义 pulsewidth",
    "核对雷达发射机 pulsewidth 定义"},
   // 假目标检测收到未检测回波（WsfEW_SensorComponent）
   {ut::msg::MSG_W_DST_UNDETECTED_BLIP,
    "DefaultSensorTracker::FalseTargetDetected has gotten an undetected blip.",
    "DefaultSensorTracker::FalseTargetDetected 收到未检测状态的假目标回波。",
    "筛选器的可跟踪/屏蔽回波列表与回波状态不一致",
    "核对 WsfFalseTargetScreener 的回波列表配置"},
   // 未定义的相干类型字符串（WsfEW_Effect::ConvertCoherencyIdToMask）
   {ut::msg::MSG_W_EFF_UNDEF_COH_STR,
    "WsfEW_Effect::ConvertCoherencyIdToMask: Undefined coherency type string.",
    "WsfEW_Effect::ConvertCoherencyIdToMask：未定义的相干类型字符串。",
    "效果定义中的相干类型名不在支持列表",
    "核对相干类型定义"},
   // ---- 武器段 warning 级（第二十批子批 C wsf_mil weapon）----
   // 装填 inhibit 命令已废弃（参数化，WsfWeapon 2 处共用）
   {ut::msg::MSG_W_WEAP_INHIBIT_DEP,
    "'{cmd}' has been deprecated.",
    "'{cmd}' 已废弃。",
    "装填期间禁射命令被 inhibit_while_reloading 取代",
    "按附注改用 inhibit_while_reloading 命令"},
   // 命令与该武器类型不兼容（参数化，WsfRF_Jammer/WsfLaserWeapon 6 处共用）
   {ut::msg::MSG_W_WEAP_CMD_INCOMPAT,
    "The {command} command is incompatible with {weapontype} (ignored).",
    "{command} 命令与 {weapontype} 不兼容（已忽略）。",
    "武器子类型不支持该通用武器命令",
    "去掉该命令或改用支持该命令的武器类型"},
   // 表最大半径超过显式最大半径（WsfGraduatedLethality::Initialize）
   {ut::msg::MSG_W_GRADLETH_RADIUS_EXCEED,
    "In weapon_effects - maximum radius from tables exceeds explicit maximum_radius.",
    "weapon_effects：表计算的最大半径超过显式 maximum_radius。",
    "杀伤概率表的半径上限大于显式定义值",
    "核对表格取值或提高 maximum_radius"},
   // 未定义的武器效果（WsfWeaponEngagement）
   {ut::msg::MSG_W_WEAPENG_UNDEF_EFFECT,
    "Undefined weapon_effect.",
    "未定义的武器效果。",
    "引用的 weapon_effects 类型未在想定中定义",
    "核对 weapon_effects 类型定义，将使用默认效果"},
   // 无法初始化武器效果（WsfWeaponEngagement）
   {ut::msg::MSG_W_WEAPENG_INIT_EFFECT_FAIL,
    "Unable to initialize weapon_effect.",
    "无法初始化武器效果。",
    "武器效果对象初始化失败",
    "核对 weapon_effects 定义参数"},
   // 原始轨迹合并失败（WsfRF_Repeater）
   {ut::msg::MSG_W_RPT_RAW_TRACK_MERGE,
    "FAILED Raw Track merge.",
    "原始轨迹合并失败。",
    "转发器模式下原始轨迹合并异常",
    "将导致轨迹错误丢弃，核对转发器工作模式"},
   // 发射时找不到 LAUNCHER_COMMANDER 命令链（WsfExplicitWeapon）
   {ut::msg::MSG_W_EXPL_LAUNCHER_NO_CHAIN,
    "Could not locate command chain for LAUNCHER_COMMANDER during weapon launch.",
    "武器发射时找不到 LAUNCHER_COMMANDER 的命令链。",
    "平台缺少 LAUNCHER_COMMANDER 对应的命令链组件",
    "核对平台 command_chain 定义"},
   // 无法向发射武器添加命令链（WsfExplicitWeapon）
   {ut::msg::MSG_W_EXPL_ADD_CHAIN_FAIL,
    "Failed to add command chain to launched weapon.",
    "无法向发射的武器添加命令链。",
    "命令链组件名与已有组件冲突",
    "核对 command_chain 名与 commander 定义"},
   // 自动插入默认概率表（WsfMobilityAndFirepowerLethality，忠实保留原文错拼 probablity）
   {ut::msg::MSG_W_MOBILITY_DEFAULT_TABLE,
    "Weapon is automatically inserting a 'default' probablity table.",
    "武器自动插入 'default' 概率表。",
    "未定义 default 目标类型的概率表",
    "按需补充显式概率表定义"},
   // 命令与 WSF_HEL_LETHALITY 不兼容（参数化，WsfHEL_Lethality 5 处共用）
   {ut::msg::MSG_W_HEL_CMD_INCOMPAT,
    "The {command} command is incompatible with a WSF_HEL_LETHALITY object.",
    "{command} 命令与 WSF_HEL_LETHALITY 对象不兼容。",
    "HEL 杀伤效果不支持该杀伤概率命令",
    "去掉该命令或改用支持的效果类型"},
   // 目标未指定 damage_radius（WsfHEL_Lethality）
   {ut::msg::MSG_W_HEL_NO_DAMAGE_RADIUS,
    "hel_lethality: No damage_radius was specified for target.",
    "hel_lethality：目标未指定 damage_radius。",
    "目标平台缺少 damage_radius 定义",
    "在目标平台定义中补充 damage_radius"},
   // 附带损伤允许命令已废弃（参数化，WsfWeaponEffects 2 处共用）
   {ut::msg::MSG_W_WFX_INCDMG_DEP,
    "'{cmd}' has been deprecated.",
    "'{cmd}' 已废弃。",
    "附带损伤命令被 incidental_damage_allowed 取代",
    "按附注改用 incidental_damage_allowed 命令"},
   // 目标距离超出射程限制（WsfBallisticMissileLaunchComputer）
   {ut::msg::MSG_W_BMLC_RANGE_LIMITS,
    "Target range is not within the range limits.",
    "目标距离不在射程限制范围内。",
    "目标距离超出 [最小, 最大] 射程区间",
    "核对发射位置与目标距离或调整射程限制"},
   // 无法确定发射窗口结束（WsfBallisticMissileLaunchComputer）
   {ut::msg::MSG_W_BMLC_NO_LAUNCH_WINDOW,
    "Could not determine end of launch window.",
    "无法确定发射窗口的结束点。",
    "发射表数据不足以推算窗口结束时间",
    "核对发射表数据完整性，将退化为仅用窗口起点"},
   // 发射表高度由降转升异常（WsfBallisticMissileLaunchComputer）
   {ut::msg::MSG_W_BMLC_TABLE_ALT_TRANS,
    "Unexpected transition from decreasing to increasing altitudes in launch table.",
    "发射表中出现高度由降转升的意外转变。",
    "发射表数据高度序列异常",
    "核对发射表高度数据的单调性"},
   // 选择技术失败：系统类型不兼容（WsfEW_EA，忠实保留原文有 an 变体）
   {ut::msg::MSG_W_EW_SEL_APPLY_AN,
    "Select Technique Failed. Could not apply technique due to an incompatible system type.",
    "选择技术失败：因系统类型不兼容，无法应用技术。",
    "按 Id 选择技术时该技术的适用系统类型与请求不符",
    "核对技术定义中的 system 类型与请求的系统类型"},
   // 取消技术失败：索引处无有效技术（WsfEW_EA，长版变体）
   {ut::msg::MSG_W_EW_DESEL_NO_VALID_ST,
    "Deselect Technique Failed. Could not find a valid technique at index for system type.",
    "取消技术失败：该系统类型在索引处无有效技术。",
    "按索引取消技术时指定索引的技术未定义或不适配",
    "核对技术列表定义与索引取值"},

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

   // ---- 通信段（0x0005xxxx）comm warning 级（第二十批，按真实侦察重排）----
   // comm 网关无效（WsfComm）
   {ut::msg::MSG_W_COMM_INVALID_GATEWAY,
    "Invalid gateway was set for comm.",
    "为 comm 设置了无效的网关。",
    "网关名或地址与网络管理器中的接口不匹配",
    "核对网关名称与地址"},
   // 无法更改网关地址（WsfComm）
   {ut::msg::MSG_W_COMM_CANNOT_CHANGE_GATEWAY,
    "Cannot change the gateway address.",
    "无法更改网关地址。",
    "新网关不是该 comm 的有效接口",
    "核对网关名称与地址"},
   // 地址字段非数字（WsfCommAddress）
   {ut::msg::MSG_W_COMMADDR_NON_NUMERIC_FIELD,
    "Address contains a field that is not a number.",
    "地址包含非数字字段。",
    "地址字符串中存在无法转换为数字的分段",
    "修正地址格式"},
   // 地址字段值超范围（WsfCommAddress）
   {ut::msg::MSG_W_COMMADDR_BAD_FIELD_VALUE,
    "Invalid address field value.",
    "地址字段值无效。",
    "IPv4 地址分段数值不在 0-255 区间",
    "修正地址字段取值"},
   // 此作用域不能设置介质命令（WsfCommMediumContainer，参数化；忠实保留原文无空格拼接）
   {ut::msg::MSG_W_MEDIUMCMD_WRONG_SCOPE,
    "Cannot set this command {command}at this scope.",
    "无法在此作用域设置命令 {command}。",
    "该作用域下已存在介质实例",
    "直接修改已有介质或移除待添加的介质"},
   // 中止条件致消息投递失败（WsfCommMediumGuided）
   {ut::msg::MSG_W_MEDIUM_DELIVERY_ABORT,
    "Message delivery failure due to abort condition.",
    "因中止条件导致消息投递失败。",
    "导引介质判定本次投递应中止",
    "核对收发双方与介质配置"},
   // 添加介质将替换已有介质（WsfCommMediumTypes）
   {ut::msg::MSG_W_MEDIUM_REPLACE_BY_ADD,
    "Medium is being replaced by adding a medium.",
    "添加介质时将替换已有介质。",
    "同一时刻仅允许一个介质，新介质覆盖旧介质",
    "确认替换是否符合预期"},
   // 介质共享编辑全局生效（WsfCommMediumTypes）
   {ut::msg::MSG_W_MEDIUM_SHARED_EDITS,
    "Medium is shared, and edits will apply changes across all mediums of this type.",
    "介质为共享实例，编辑将应用到此类型的所有介质。",
    "在平台层编辑了全局共享的介质类型",
    "确认编辑影响范围"},
   // 网络成员地址未找到（WsfCommNetwork）
   {ut::msg::MSG_W_COMMNET_ADDR_NOT_FOUND,
    "Address not found for network member.",
    "找不到网络成员的地址。",
    "按名称加入网络的成员未解析出地址",
    "核对成员的平台名与 comm 名"},
   // 网络成员地址未分配被跳过（WsfCommNetwork）
   {ut::msg::MSG_W_COMMNET_UNASSIGNED_ADDR,
    "Unassigned address used as network member. Skipping address.",
    "网络成员使用了未分配的地址，跳过该地址。",
    "地址列表中的地址不属于任何已知成员",
    "核对地址与成员定义"},
   // 命名链路成员未找到（WsfCommNetwork）
   {ut::msg::MSG_W_COMMNET_NAMED_LINK_MISSING,
    "Named Link Members could not be found.",
    "找不到命名的链路成员。",
    "链路一端或两端的平台/comm 不存在",
    "核对链路两端的平台名与 comm 名"},
   // 地址链路成员未找到（WsfCommNetwork）
   {ut::msg::MSG_W_COMMNET_ADDR_LINK_MISSING,
    "Address Link Members could not be found.",
    "找不到地址链路成员。",
    "链路地址未关联任何已知成员",
    "核对链路地址定义"},
   // comm 网络指派冲突（WsfCommNetworkManager）
   {ut::msg::MSG_W_NETMGR_CONFLICTING_NET,
    "Comm has conflicting network assignment.",
    "comm 的网络指派相互冲突。",
    "comm 的指派网络与网络成员表不一致",
    "核对 comm 的网络指派"},
   // 添加连接失败（WsfCommNetworkManager 4 处共用）
   {ut::msg::MSG_W_NETMGR_ADDCONN_FAIL,
    "Failed to add connection to comm network manager.",
    "向 comm 网络管理器添加连接失败。",
    "链路两端 comm 不存在或不属于同一网络管理器",
    "核对链路两端的 comm 定义"},
   // 空 comm 引用无法管理（WsfCommNetworkManager）
   {ut::msg::MSG_W_NETMGR_NULL_COMM,
    "A null referenced comm object cannot be managed by the network manager.",
    "空引用的 comm 对象无法由网络管理器管理。",
    "调用方传入了空 comm 指针",
    "核查调用链的 comm 指针有效性"},
   // comm 已被网络管理器管理（WsfCommNetworkManager）
   {ut::msg::MSG_W_NETMGR_ALREADY_MANAGED,
    "Comm is already managed by the network manager.",
    "该 comm 已由网络管理器管理。",
    "重复向网络管理器注册同一 comm",
    "无需重复注册"},
   // comm 入指定网络被拒（WsfCommNetworkManager）
   {ut::msg::MSG_W_NETMGR_REJECT_ADD,
    "Comm cannot be managed by the network manager due to rejection of comm object addition to the specified network.",
    "由于向指定网络添加 comm 对象被拒，该 comm 无法由网络管理器管理。",
    "目标网络拒绝接收该 comm 成员",
    "核对网络成员约束条件"},
   // 星型网络无中心节点（WsfCommNetworkStar）
   {ut::msg::MSG_W_NETSTAR_NO_HUB,
    "Star network does not have a hub.",
    "星型网络没有中心节点。",
    "星型网络定义缺少 hub 地址",
    "补充 hub 定义"},
   // 每 comm 仅允许一个 IGMP（WsfCommProtocolIGMP）
   {ut::msg::MSG_W_IGMP_ONLY_ONE,
    "Only a single IGMP protocol allowed per comm.",
    "每个 comm 仅允许一个 IGMP 协议。",
    "同一 comm 上添加了多个 IGMP 协议",
    "移除多余的 IGMP 协议"},
   // OSPF 需二级多播一致性（WsfCommProtocolOSPF 2 处共用）
   {ut::msg::MSG_W_OSPF_NEED_L2_MULTICAST,
    "OSPF usage requires level 2 multicast conformance.",
    "OSPF 需要二级多播一致性。",
    "comm 的多播一致性级别低于 OSPF 所需的二级",
    "开启 level 2 多播一致性"},
   // 路由器无多播协议（WsfCommProtocolOSPF）
   {ut::msg::MSG_W_OSPF_NO_MULTICAST_PROTO,
    "Multicast protocol is not present on router.",
    "路由器上不存在多播协议。",
    "使用 OSPF 的路由器未挂接多播协议",
    "为路由器补充多播协议"},
   // OSPF 无法选出指定路由器（WsfCommProtocolOSPF）
   {ut::msg::MSG_W_OSPF_NO_DESIGNATED_RTR,
    "A designated router could not be elected for OSPF.",
    "OSPF 无法选出指定路由器。",
    "该区域中无可胜任指定路由器的设备",
    "核对区域内路由器配置"},
   // 路由器缺少网络管理器（WsfCommRouter）
   {ut::msg::MSG_W_ROUTER_NO_NETMGR,
    "Missing network manager.",
    "缺少网络管理器。",
    "初始化时仿真未提供 comm 网络管理器",
    "如频繁出现请联系开发方"},
   // 平台网关设置无效（WsfCommRouter）
   {ut::msg::MSG_W_ROUTER_BAD_GATEWAY,
    "Invalid gateway set for platform.",
    "为平台设置的网关无效。",
    "网关名称或地址不正确，或网关不是该路由器的接口",
    "核对网关名称与地址"},
   // 路由器对象不支持该命令（WsfCommRouter，参数化）
   {ut::msg::MSG_W_ROUTER_CMD_NOT_SUPPORTED,
    "The {command} command is not supported for router objects. All routers are required to be operational by default to support legacy AFSIM behavior. Disabling/turning off the router is still supported via script method usage after instantiation in the simulation.",
    "路由器对象不支持 {command} 命令。为兼容旧版 AFSIM 行为，所有路由器默认必须保持运行状态。仍可在仿真实例化之后通过脚本方法禁用/关闭路由器。",
    "router 块中使用了改变运行状态的命令",
    "移除该命令，改用脚本方法控制"},
   // 失去连通性路由器丢失网关（WsfCommRouter）
   {ut::msg::MSG_W_ROUTER_LOST_GATEWAY,
    "Router no longer has a gateway due to loss of connectivity with the gateway.",
    "因与网关失去连通性，路由器不再拥有网关。",
    "到网关的路径已断开",
    "核对网关连通性"},
   // 路由器无法更改网关（WsfCommRouter）
   {ut::msg::MSG_W_ROUTER_CANNOT_CHANGE_GW,
    "Router cannot change gateway.",
    "路由器无法更改网关。",
    "新网关不是该路由器的已分配接口",
    "核对网关地址"},
   // 消息超 TTL 被丢弃（WsfCommRouter）
   {ut::msg::MSG_W_ROUTER_TTL_EXCEEDED,
    "A message exceeded TTL and has been dropped.",
    "消息超过 TTL 跳数限制，已被丢弃。",
    "消息在网络中转发的跳数超过生存时间",
    "核对网络拓扑与 TTL 设置"},
   // event_pipe 事件类型无效（WsfEventPipeOptions）
   {ut::msg::MSG_W_EVENTPIPE_BAD_EVENT_TYPE,
    "Invalid \"event_pipe\" event type",
    "无效的 \"event_pipe\" 事件类型。",
    "event_pipe 命令引用了未定义的事件类型",
    "核对事件类型名"},
   // Record 带 sim_time 参数已废弃（WsfEventPipeInterface）
   {ut::msg::MSG_W_EVENTPIPE_RECORD_DEP,
    "WsfEventPipe.Record with the sim_time argument has been deprecated and will be removed in a future release.  Use WsfEventPipe.Record without the sim_time argument instead.This issue may occur multiple places in the scenario.",
    "带 sim_time 参数的 WsfEventPipe.Record 已废弃，将在未来版本移除。请改用不带 sim_time 参数的 WsfEventPipe.Record。此问题可能在想定中的多处出现。",
    "脚本调用了旧签名的事件记录方法",
    "移除 Record 调用中的 sim_time 参数"},
   // XIO 收到版本无效的 PDU（WsfXIO_UDP_Header）
   {ut::msg::MSG_W_XIO_PDU_BAD_VERSION,
    "xio_interface: Received PDU with invalid version.",
    "xio_interface：收到版本无效的 PDU。",
    "对端与本端的 XIO 版本不一致",
    "核对两端程序版本"},
   // 弃轨消息的目标索引已不存在（WsfXIO_AutoDisMapping）
   {ut::msg::MSG_W_XIOSIM_DROP_TRACK_NO_TARGET,
    "WSF_DROP_TRACK_MESSAGE target index no longer exists.",
    "WSF_DROP_TRACK_MESSAGE 的目标索引已不存在。",
    "弃轨消息引用的 DIS 目标索引已被回收",
    "核对弃轨消息的目标索引"},
   // ---- DIS 接口段（0x0005xxxx）dis warning 级（第二十批，按真实侦察重排）----
   // 非标准 DIS 命令已废弃（WsfDisInput 6 处共用，参数化）
   {ut::msg::MSG_W_DISIN_NONSTD_DEPRECATED,
    "{command} and the use of non-standard DIS has been deprecated and will be removed in a future release.",
    "{command} 及非标准 DIS 的使用已废弃，将在未来版本中移除。",
    "使用了非标准 DIS 的废弃命令",
    "改用标准 DIS 命令"},
   // age_out_time 已废弃（WsfDisInput）
   {ut::msg::MSG_W_DISIN_AGEOUT_DEPRECATED,
    "WsfDisInput: 'age_out_time' has been deprecated and will be removed in a future release.",
    "WsfDisInput：'age_out_time' 已废弃，将在未来版本中移除。",
    "该命令被新的超时机制取代",
    "移除 age_out_time 定义"},
   // 影子平台添加失败（WsfDisInterface 2 处共用）
   {ut::msg::MSG_W_DISIF_SHADOW_ADD_FAIL,
    "Unable to add shadow platform to simulation.",
    "无法向仿真添加影子平台。",
    "平台类型未在想定中定义或平台添加被拒",
    "核对平台类型定义"},
   // 距离过滤平台不存在（WsfDisInterface）
   {ut::msg::MSG_W_DISIF_RANGE_FILTER_NO_PLATFORM,
    "Platform specified for range filtering, does not (or, does no longer) exist.",
    "为距离过滤指定的平台不存在或已不存在。",
    "距离过滤列表引用的平台已被删除",
    "核对 range_filter 平台名"},
   // 实体状态 PDU 无实体 ID（WsfDisInterface）
   {ut::msg::MSG_W_DISIF_NULL_ENTITY_ID,
    "Entity State PDU received with no entity ID. Ignored.",
    "收到的实体状态 PDU 无实体 ID，已忽略。",
    "对端发送了实体 ID 为空的 PDU",
    "核对对端实体配置"},
   // DataR 收到未知数据回复（WsfDisDataR）
   {ut::msg::MSG_W_DATAR_UNKNOWN_REPLY,
    "WsfDisDataR received unknown data reply.",
    "WsfDisDataR 收到未知的数据回复。",
    "回复中的数据项 ID 无法被识别",
    "核对请求方期待的数据项"},
   // DataR 收到未解析变量（WsfDisDataR）
   {ut::msg::MSG_W_DATAR_UNPARSED_VARIABLE,
    "WsfDisDataR: Received unparsed variable.",
    "WsfDisDataR：收到未解析的变量。",
    "可变数据项内容无法解析",
    "核对数据项格式"},
   // DataR 无法解释全部返回值（WsfDisDataR）
   {ut::msg::MSG_W_DATAR_CANT_INTERPRET,
    "WsfDisDataR: Cannot interpret all returned values.",
    "WsfDisDataR：无法解释全部返回值。",
    "部分返回值无对应处理器",
    "核对返回数据项定义"},
   // DataR 不识别发起 PDU 类型（WsfDisDataR）
   {ut::msg::MSG_W_DATAR_UNKNOWN_INITIATOR,
    "WsfDisDataR: Does not know what type of initiating PDU was received.",
    "WsfDisDataR：无法识别收到的发起 PDU 的类型。",
    "发起 PDU 的类型未在本端注册",
    "核对请求/应答流程"},
   // DataR 未处理时间推进上报（WsfDisDataR）
   {ut::msg::MSG_W_DATAR_TIMEADV_REPORT,
    "WsfDisDataR: Unhandled method ProcessTimeAdvanceReport().",
    "WsfDisDataR：未处理 ProcessTimeAdvanceReport() 方法。",
    "该接收器不处理时间推进上报",
    "无需处理"},
   // TimedOut 虚函数未重载（dis 系列接收器 10 处共用，参数化）
   {ut::msg::MSG_W_TIMEDOUT_UNHANDLED,
    "{cls}: Unhandled virtual TimedOut() call.",
    "{cls}：未重载的虚函数 TimedOut() 被调用。",
    "接收器类未实现超时处理逻辑",
    "如频繁出现请联系开发方"},
   // 退出演习请求未获充分响应（WsfDisActionRequestR）
   {ut::msg::MSG_W_ARR_RESIGN_NO_RESPONSE,
    "WsfDisActionRequestR: Resign Exercise Request is not given an adequate response yet.",
    "WsfDisActionRequestR：退出演习请求尚未获得充分响应。",
    "本端尚不支持对退出演习请求的应答",
    "如影响使用请联系开发方"},
   // 动作请求无法被 WSF 解释（WsfDisActionRequestR）
   {ut::msg::MSG_W_ARR_REQ_NOT_INTERPRETED,
    "WsfDisActionRequestR: Action Request received that cannot be interpreted by WSF.",
    "WsfDisActionRequestR：收到的动作请求无法被 WSF 解释。",
    "请求的动作 ID 不在本端支持范围",
    "核对请求方发送的动作内容"},
   // 忽略时间推进请求（WsfDisActionRequestR）
   {ut::msg::MSG_W_ARR_TIMEADV_IGNORED,
    "WsfDisActionRequestR: ProcessTimeAdvanceRequest ignored.",
    "WsfDisActionRequestR：已忽略时间推进请求。",
    "本端不处理该时间推进方案",
    "核对时间管理方案配置"},
   // 遇到未知固定数据项（WsfDisActionRequestR）
   {ut::msg::MSG_W_ARR_UNKNOWN_FIXED_DATUM,
    "WsfDisActionRequestR: Unknown fixed datum encountered.",
    "WsfDisActionRequestR：遇到未知的固定数据项。",
    "固定数据项 ID 无法被识别",
    "核对数据项定义"},
   // SetDataR 无法完整响应查询（WsfDisSetDataR）
   {ut::msg::MSG_W_SETDATAR_CANT_FULLY_RESPOND,
    "WsfDisSetDataR: Could not fully respond to data query.",
    "WsfDisSetDataR：无法完整响应数据查询。",
    "部分查询数据项无法提供",
    "核对查询内容与本端数据能力"},
   // SetDataR 固定数据项不支持（WsfDisSetDataR）
   {ut::msg::MSG_W_SETDATAR_BAD_FIXED_DATUM,
    "WsfDisSetDataR: Unsupported fixed datum.",
    "WsfDisSetDataR：不支持的固定数据项。",
    "固定数据项 ID 不在本端支持范围",
    "核对数据项定义"},
   // SetDataR 可变数据项不支持（WsfDisSetDataR）
   {ut::msg::MSG_W_SETDATAR_BAD_VARIABLE_DATUM,
    "WsfDisSetDataR: Unsupported variable datum.",
    "WsfDisSetDataR：不支持的可变数据项。",
    "可变数据项 ID 不在本端支持范围",
    "核对数据项定义"},
   // DataQueryR 收到空查询（WsfDisDataQueryR）
   {ut::msg::MSG_W_DATARQ_NULL_QUERY,
    "WsfDisDataQueryR: Received a null query (no datums requested).",
    "WsfDisDataQueryR：收到空查询（未请求数据项）。",
    "对端发出的查询未包含任何数据项",
    "核对对端查询内容"},
   // DataQueryR 无法响应全部查询（WsfDisDataQueryR）
   {ut::msg::MSG_W_DATARQ_CANT_RESPOND_ALL,
    "WsfDisDataQueryR: Could not respond to all queries.",
    "WsfDisDataQueryR：无法响应全部查询。",
    "部分查询无法在本端完成",
    "核对查询内容与本端数据能力"},
   // SetRecordR 未实现 PopulateReply（WsfDisSetRecordR）
   {ut::msg::MSG_W_SETR_POPULATE_NOT_IMPL,
    "WsfDisSetRecordR has not yet implemented PopulateReply() function.",
    "WsfDisSetRecordR 尚未实现 PopulateReply() 函数。",
    "该接收器功能尚未实现",
    "如影响使用请联系开发方"},
   // SetRecordR 尚不处理数据（WsfDisSetRecordR）
   {ut::msg::MSG_W_SETR_NOT_PROCESSING,
    "WsfDisSetRecordR does not yet process the data in any way.",
    "WsfDisSetRecordR 尚不会以任何方式处理数据。",
    "该接收器仅回执不处理内容",
    "如影响使用请联系开发方"},
   // CreateEntityR 无法执行请求（WsfDisCreateEntityR）
   {ut::msg::MSG_W_CER_CANT_COMPLY,
    "WsfDisCreateEntityR: Cannot yet comply with ProcessCreateEntityRequest().",
    "WsfDisCreateEntityR：尚无法执行 ProcessCreateEntityRequest()。",
    "本端尚不支持远程创建实体请求",
    "如影响使用请联系开发方"},
   // RemoveEntityR 无法执行请求（WsfDisRemoveEntityR）
   {ut::msg::MSG_W_RER_CANT_COMPLY,
    "WsfDisRemoveEntityR: Cannot yet comply with the Remove request.",
    "WsfDisRemoveEntityR：尚无法执行 Remove 请求。",
    "本端尚不支持远程移除实体请求",
    "如影响使用请联系开发方"},
   // Exchange 无 entity_type 映射（WsfDisExchange）
   {ut::msg::MSG_W_EXCH_NO_TYPE_MAPPING,
    "WsfDisExchange: ExchangeQueried(): DIS Interface entity_type mapping not found.",
    "WsfDisExchange：ExchangeQueried()：找不到 DIS 接口的 entity_type 映射。",
    "交换项引用的实体类型未在 DIS 映射表中",
    "核对交换项的实体类型定义"},
   // sEVENT_SUPPLIED 发往未知参与方（WsfDisExchange）
   {ut::msg::MSG_W_EXCH_SUPPLIED_UNKNOWN,
    "WsfDisExchange: ExchangeQueried(): sEVENT_SUPPLIED to unknown player.",
    "WsfDisExchange：ExchangeQueried()：sEVENT_SUPPLIED 发往未知参与方。",
    "交换事件的接收方不存在",
    "核对交换事件的参与方定义"},
   // RecordQueryR 无数据记录响应（WsfDisRecordQueryR）
   {ut::msg::MSG_W_RQR_NO_DATA_RECORDS,
    "No data records were supplied in response to a RecordQueryR PDU.",
    "响应 RecordQueryR PDU 时未提供任何数据记录。",
    "本端无对应记录可返回",
    "核对查询的记录类型"},
   // 不对加入演习请求采取行动（WsfDisActionRequest）
   {ut::msg::MSG_W_AR_NO_JOIN_ACTION,
    "WsfDisActionRequest: Does not yet act upon a Join Exercise Request.",
    "WsfDisActionRequest：尚不对加入演习请求采取行动。",
    "本端不处理加入演习请求",
    "无需处理"},
   // 未启用响应传入查询（WsfDisDataQuery）
   {ut::msg::MSG_W_DQ_NOT_ENABLED,
    "WsfDisDataQuery: Not enabled to act on incoming queries.",
    "WsfDisDataQuery：未启用对传入查询的响应。",
    "未开启 data_query 处理开关",
    "按需开启响应能力"},
   // 实体状态 PDU 数据可疑（WsfDisEntityState）
   {ut::msg::MSG_W_ENTITYSTATE_SUSPICIOUS,
    "WsfDisEntityState: Suspicious data in Entity State PDU.",
    "WsfDisEntityState：实体状态 PDU 中存在可疑数据。",
    "位置/速度/加速度数值异常（如 NaN 或超限）",
    "核对对端实体状态数据"},
   // 超过最大波束数量（WsfDisPlatform）
   {ut::msg::MSG_W_DISPLAT_MAX_BEAMS,
    "Max beam count exceeded.",
    "超过最大波束数量。",
    "电磁发射的波束数超过上限",
    "核对发射系统波束定义"},
   // 影子平台添加失败（WsfDisInterface，忠实保留原文无句点）
   {ut::msg::MSG_W_DISIF_SHADOW_ADD_FAIL2,
    "Unable to add shadow platform to simulation",
    "无法向仿真添加影子平台。",
    "影子平台添加仿真被拒",
    "核对平台类型定义"},
   // ---- 通信/DIS 段 warning 级（第二十批子批 C wsf_mil）----
   // DIS 武器事件未指定实体类型（WsfDisWeaponEvents）
   {ut::msg::MSG_W_DISWEAP_NO_ENT_TYPE,
    "Weapon Entity type not specified.",
    "未指定武器实体类型。",
    "武器 PDU 处理时实体类型字段缺失",
    "核对 DIS 实体类型映射配置"},
   // 定向能射击 PDU 记录类型不支持（WsfDisDirectedEnergyFire）
   {ut::msg::MSG_W_DEFIRE_BAD_RECORD,
    "DisDirectedEnergyFire PDU Record Type not supported (ignoring).",
    "DisDirectedEnergyFire PDU 记录类型不受支持（忽略）。",
    "PDU 中的记录类型不在支持列表",
    "核对对端 DIS 实现的记录类型"},
   // 定向能射击无交战对象（WsfDisDirectedEnergyFire）
   {ut::msg::MSG_W_DEFIRE_NO_ENGAGE,
    "No engagement object.",
    "无交战对象。",
    "找不到该射击事件对应的交战对象",
    "核对交战对象创建流程"},
   // DIS 射击交战对象初始化失败（WsfDisFire）
   {ut::msg::MSG_W_DISFIRE_INIT_FAIL,
    "WsfDisFire failed to initialize engagement object.",
    "WsfDisFire 初始化交战对象失败。",
    "射击事件无法创建或初始化交战对象",
    "核对武器交战相关定义"},
   // 激光发射机 PRF 由调制类型等输入决定（WsfLASER_CommXmtrComponent）
   {ut::msg::MSG_W_LASER_PRF_OVERRIDE,
    "Laser transmitter 'pulse_repetition_frequency' will be set based on modulation type and either 'pulse_width' or "
    "'slot_duration' inputs (not the provided 'pulse_repetition_frequency' input).",
    "激光发射机 'pulse_repetition_frequency' 将基于调制类型及 'pulse_width' 或 'slot_duration' 输入计算（不采用所提供的 "
    "'pulse_repetition_frequency' 输入）。",
    "激光发射机模式下 PRF 由占空比/槽位时长推算",
    "去掉 pulse_repetition_frequency 输入或确认自动计算结果"},
   // JTIDS 槽位组重复加入网络（WsfJTIDS_SlotGroup）
   {ut::msg::MSG_W_JTIDS_ADD_TWICE,
    "Slot group is trying to be added to the network twice.",
    "槽位组试图重复加入网络。",
    "同名槽位组已存在于网络中",
    "核对槽位组名称定义，避免重复添加"},
   // JTIDS 槽位组已合并（WsfJTIDS_SlotGroup）
   {ut::msg::MSG_W_JTIDS_MERGED,
    "Slot group merged.",
    "槽位组已合并。",
    "发送队列中已存在相同序列号的消息",
    "确认合并行为符合预期"},
   // 潜艇通信发射机未指定功率（WsfCommComponentHW_Subsurface）
   {ut::msg::MSG_W_HWSUB_NO_POWER,
    "No power assigned to transmitter.",
    "发射机未指定功率。",
    "通信设备定义中缺少发射功率",
    "在 comm 定义中补充发射功率设置"},

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

std::string FormatWEnTagged(uint32_t aId, const Args& aArgs)
{
   // 二次开发（i18n）：拼接 "[W:ID] " 前缀 —— 与 EmitWarning 的落盘格式保持一致，
   // 供 warning 级需要保留 AddNote() 流式日志结构的调用点使用（第二十批新增）
   char prefix[16];
   std::snprintf(prefix, sizeof(prefix), "[W:%08X] ", aId);
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
