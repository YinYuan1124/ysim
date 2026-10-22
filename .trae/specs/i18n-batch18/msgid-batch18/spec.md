# AFSIM 消息 ID 化改造 第十八批规格：wsf 库零散目录

## 1. 背景与目标

延续既定的消息 ID 化改造范式（已完成十七批，累计 300 条目录条目、323 处调用点），本批处理 **wsf 库零散目录**的 error 级硬编码报错：

- 范围：dis（8 处）、event_pipe（1 处有效）、ext（1 处）、mover（4 处，第十二批有意跳过项，参照第十七批先例全清）、xio（1 处）、xio_sim（3 处）
- 排除（本批不做）：script（42 处）、sensor（56 处）、traffic（46 处）三大目录，留待后续批次
- 排除项说明：event_pipe/WsfEventPipeFileWriteWorker.cpp:55 为注释掉的代码（`// ut::log::error() << "QUEUE LIMIT EXCEEDED!";`），不纳入

**本批规模：18 处调用点、17 个新 ID、14 个业务文件（其中 11 个需补 include）**

改造后累计：**317 条目录条目、341 处调用点、分十八批**。

## 2. 调用点清单与 ID 分配（决策完备）

### 2.1 0x0002 平台/运动段 mover 域补遗（4 处，续接 0x0002007D 之后）

| # | 文件（src/core/wsf/source/ 下） | 行 | 英文原文（一字不动） | ID 常量 | ID 值 |
|---|---|---|---|---|---|
| 1 | mover/WsfNavigationErrors.cpp | 207 | `WsfNavigationError() Report:` | MSG_NAV_ERROR_INS_REPORT | 0x0002007E |
| 2 | mover/WsfPathFinder.cpp | 1016 | `FindClosestValidPoint() could not find a valid point near target.` | MSG_PF_FIND_CLOSEST_FAIL | 0x0002007F |
| 3 | mover/WsfPathGuidance.cpp | 111 | `WsfPathGuidance has not been attached to a mover.` | MSG_PG_NOT_ATTACHED | 0x00020080 |
| 4 | mover/WsfRouteNetwork.cpp | 1077 | `Failure in WsfRouteNetwork: InsertArea().` | MSG_RN_INSERT_AREA_FAIL | 0x00020081 |

### 2.2 0x0003 传感器段 ext 域（1 处，续接 0x00030031 之后）

| # | 文件 | 行 | 英文原文 | ID 常量 | ID 值 |
|---|---|---|---|---|---|
| 5 | ext/WsfExtEmission.cpp | 473 | `Initialization failed for external sensor.` | MSG_EXT_SENSOR_INIT_FAIL | 0x00030032 |

### 2.3 0x0005 通信段 dis 接口/event_pipe/xio/xio_sim（13 处调用点、12 个 ID，续接 0x00050029 之后）

| # | 文件 | 行 | 英文原文 | ID 常量 | ID 值 |
|---|---|---|---|---|---|
| 6 | dis/WsfDisActionResponse.cpp | 99 | `WsfDisActionRequest: Initializing was not a Join Exercise Request.` | MSG_DIS_ACTION_RESP_NOT_JOIN | 0x0005002A |
| 7 | dis/WsfDisActionResponseR.cpp | 140 | `WsfDisActionRequestR: Initializing was not a Join Exercise Request.` | MSG_DIS_ACTION_RESPR_NOT_JOIN | 0x0005002B |
| 8 | dis/WsfDisInterface.cpp | 410 | `Unable to open dis_interface device.` | MSG_DIS_IFACE_OPEN_DEVICE_FAIL | 0x0005002C |
| 9 | dis/WsfDisInterface.cpp | 1539 | 同上（**两处共用同一 ID**） | 同上 | 同上 |
| 10 | dis/WsfDisInterface.cpp | 2883 | `Unable to set appearance script variable.` | MSG_DIS_IFACE_SET_APPEAR_VAR_FAIL | 0x0005002D |
| 11 | dis/WsfDisInterface.cpp | 2900 | `Unable to execute appearance update.` | MSG_DIS_IFACE_EXEC_APPEAR_FAIL | 0x0005002E |
| 12 | dis/WsfDisPlaybackDevice.cpp | 60 | `Could not open file.` | MSG_DIS_PLAYBACK_OPEN_FAIL | 0x0005002F |
| 13 | dis/WsfDisPlaybackDevice.cpp | 69 | `Too many 'playback' files specified.` | MSG_DIS_PLAYBACK_TOO_MANY | 0x00050030 |
| 14 | event_pipe/WsfEventPipeInterface.cpp | 457 | `Unable to open event_pipe file.` | MSG_EVENTPIPE_OPEN_FAIL | 0x00050031 |
| 15 | xio/WsfXIO_Subscription.cpp | 39 | `No linked connection available.` | MSG_XIO_NO_LINKED_CONN | 0x00050032 |
| 16 | xio_sim/WsfXIO_AutoDisMapping.cpp | 706 | `XIO: Message has no translator.` | MSG_XIO_MSG_NO_TRANSLATOR | 0x00050033 |
| 17 | xio_sim/WsfXIO_DisExtension.cpp | 48 | `auto_dis_mapping: Initialization failed.` | MSG_XIO_AUTO_DIS_MAP_INIT_FAIL | 0x00050034 |
| 18 | xio_sim/WsfXIO_CommComponent.cpp | 107 | `Cannot transmit WsfMessage over XIO.` | MSG_XIO_CANNOT_TX_MSG | 0x00050035 |

## 3. 编辑明细

### 3.1 UtMsgCatalog.hpp（e:\Project\ysim\src\tools\util\source\）

三处插入（均按既有分段注释风格）：

1. **0x0002 段**：在第 201 行 `MSG_TBM_MASS_NOT_CONSISTENT = 0x0002007D` 之后、`// ---- 传感器段` 注释之前，新增注释块 `// ---- 平台/运动段（0x0002xxxx）mover 域散点补遗（第十八批）----` 及 4 个常量
2. **0x0003 段**：在第 252 行 `MSG_OTH_MTI_DUP_TABLE = 0x00030031` 之后，新增注释块 `// ---- 传感器段（0x0003xxxx）ext 域（第十八批）----` 及 1 个常量
3. **0x0005 段**：在第 385 行 `MSG_DIS_FACTORY_FIRST_PDU = 0x00050029` 之后，新增注释块 `// ---- 通信段（0x0005xxxx）wsf dis 接口/event_pipe/xio（第十八批）----` 及 12 个常量

### 3.2 UtMsgCatalog.cpp 目录表（17 条，ID 升序插入）

插入锚点（已侦察确认）：

1. **0x0002 段 4 条**：在 MSG_TBM_MASS_NOT_CONSISTENT 条目（cpp 第 804-808 行）之后、`// ---- 传感器段（0x0003xxxx）----`（第 810 行）之前
2. **0x0003 段 1 条**：在 MSG_OTH_MTI_DUP_TABLE 条目（cpp 第 1101-1105 行）之后、`// ---- 交战/武器段`（第 1107 行）之前
3. **0x0005 段 12 条**：在 MSG_DIS_FACTORY_FIRST_PDU 条目（cpp 第 1833-1837 行）之后、`// ---- 工具/UI 段`（第 1839 行）之前

中文译文（Entry{英文原文, 中文译文, 原因, 修复建议}）：

| ID 常量 | 中文译文 | 原因 | 修复建议 |
|---|---|---|---|
| MSG_NAV_ERROR_INS_REPORT | 导航误差报告： | 平台销毁时 GPS 处于非激活状态，输出最终 INS 位置误差报告 | 状态报告，无需修复 |
| MSG_PF_FIND_CLOSEST_FAIL | FindClosestValidPoint() 未能在目标点附近找到有效点。 | 目标点周边网格节点均不可用（受限区域） | 检查目标点是否落在受限区域或导航网格之外 |
| MSG_PG_NOT_ATTACHED | WsfPathGuidance 尚未挂接到运动体。 | 初始化时运动体指针为空 | 检查 mover 的路径制导挂接流程 |
| MSG_RN_INSERT_AREA_FAIL | WsfRouteNetwork 路线段插入失败（InsertArea）。 | 路线段插入空间索引树失败 | 检查路线段几何是否退化（零长度线段） |
| MSG_EXT_SENSOR_INIT_FAIL | 外部传感器初始化失败。 | 外部辐射源对应传感器初始化失败 | 检查传感器类型定义与初始化输入 |
| MSG_DIS_ACTION_RESP_NOT_JOIN | WsfDisActionRequest：初始化请求并非加入演习请求。 | 收到非 JOIN_EXERCISE 类型的动作响应 | 无需修复，该响应已被忽略 |
| MSG_DIS_ACTION_RESPR_NOT_JOIN | WsfDisActionRequestR：初始化请求并非加入演习请求。 | 同上 | 同上 |
| MSG_DIS_IFACE_OPEN_DEVICE_FAIL | 无法打开 dis_interface 设备。 | DIS 网络设备创建或初始化抛出异常（两处调用点共用） | 检查设备配置与网络权限 |
| MSG_DIS_IFACE_SET_APPEAR_VAR_FAIL | 无法设置外观脚本变量。 | 关节参数对应的脚本变量不存在 | 检查 appearance 脚本变量定义 |
| MSG_DIS_IFACE_EXEC_APPEAR_FAIL | 无法执行外观更新脚本。 | 外观更新脚本执行失败 | 检查脚本内容与签名 |
| MSG_DIS_PLAYBACK_OPEN_FAIL | 无法打开回放文件。 | DIS 回放文件不存在或不可读 | 检查 playback 文件路径 |
| MSG_DIS_PLAYBACK_TOO_MANY | 指定的 'playback' 回放文件过多。 | 回放站点编号超出上限 | 减少回放文件数量 |
| MSG_EVENTPIPE_OPEN_FAIL | 无法打开 event_pipe 事件管道文件。 | 输出文件路径无效或不可写 | 检查 event_pipe 输出文件路径 |
| MSG_XIO_NO_LINKED_CONN | 无可用的关联连接。 | 不可靠订阅请求时关联数据连接缺失 | 检查 XIO 连接配置 |
| MSG_XIO_MSG_NO_TRANSLATOR | XIO：消息没有对应的翻译器。 | 消息类型未注册翻译器 | 检查 XIO 消息翻译器注册 |
| MSG_XIO_AUTO_DIS_MAP_INIT_FAIL | auto_dis_mapping：自动 DIS 映射初始化失败。 | 自动 DIS 映射对象初始化失败 | 检查 auto_dis_mapping 配置 |
| MSG_XIO_CANNOT_TX_MSG | 无法通过 XIO 发送 WsfMessage 消息。 | 消息类型不在 XIO 可翻译列表中 | 注册对应消息的翻译器 |

### 3.3 调用点替换范式（与十七批一致）

```cpp
// 替换前
ut::log::error() << "硬编码英文";
// 替换后（上方加注释）
// 二次开发（i18n）：错误消息 ID 化 —— 无运行时参数
ut::log::error() << ut::msg::FormatEnTagged(ut::msg::MSG_XXX, {});
```

- 原文为 `auto out = ut::log::error() << "..."` 且后续有 `out.AddNote()` 的（#1/2/5/6/7/10/11/12/13/14/16/18 共 12 处），**保留 auto out = 声明**，AddNote 附注保持英文原样
- 无参数调用点统一传空 Args `{}`

### 3.4 include 补齐（11 个文件）

统一前缀注释 + 字母序插入：

```cpp
// 二次开发（i18n）：报错消息走消息目录（UtMsgCatalog）
#include "UtMsgCatalog.hpp"
```

| 文件 | 插入锚点（前一行 → 后一行） |
|---|---|
| mover/WsfNavigationErrors.cpp | `#include "UtMemory.hpp"` → `#include "UtStringUtil.hpp"` |
| mover/WsfPathGuidance.cpp | `#include "UtMath.hpp"` → `#include "UtSphericalEarth.hpp"` |
| ext/WsfExtEmission.cpp | `#include "UtMath.hpp"` → `#include "WsfArticulatedPart.hpp"` |
| dis/WsfDisActionResponseR.cpp | `#include "UtLog.hpp"` → `#include "WsfSimulation.hpp"` |
| dis/WsfDisActionResponse.cpp | `#include "UtLog.hpp"` → `#include "WsfSimulation.hpp"` |
| dis/WsfDisInterface.cpp | `#include "UtMath.hpp"` → `#include "UtSleep.hpp"` |
| dis/WsfDisPlaybackDevice.cpp | `#include "UtLog.hpp"` → `#include "WsfComm.hpp"` |
| xio/WsfXIO_Subscription.cpp | `#include "UtLog.hpp"` → `#include "xio/WsfXIO_Connection.hpp"` |
| xio_sim/WsfXIO_AutoDisMapping.cpp | `#include "UtLog.hpp"` → `#include "UtStringUtil.hpp"` |
| xio_sim/WsfXIO_DisExtension.cpp | `#include "UtLog.hpp"` → `#include "xio/WsfXIO_Interface.hpp"` |
| xio_sim/WsfXIO_CommComponent.cpp | `#include "UtLog.hpp"` → `#include "UtStringIdLiteral.hpp"` |

**已含 include 无需添加（3 个）**：mover/WsfPathFinder.cpp、mover/WsfRouteNetwork.cpp（第十二批改造）、event_pipe/WsfEventPipeInterface.cpp（第 17-18 行已有）。

## 4. 验证与收尾流程

1. **BOM 处理**：所有修改文件执行循环剥离脚本（剥离所有前导 BOM 只留单层 UTF-8 BOM）——多重 BOM 叠加会导致 MSVC `sal.h C2144` + `(1,1) C4430`
2. **编译验证**：`cmake --build "e:\Project\ysim\build" --config Release --target wsf mission`（wsf 库变更级联 wsf_mil → mission）；要求 exit 0 无 error
3. **记账三件套**：
   - `doc/2026-10-15修改日志.md` 顶部插第十八批章节（注意 Edit 锚点需保留后一章节标题行）
   - `.trae/skills/afsim-dev/SKILL.md` L2 消息层累计口径更新为 **317 条/341 处，分十八批**，追加⑱明细
   - `project_memory.md` 累计口径同步

## 5. 约束与风险

- ID 一经分配永不复用；英文原文一字不动（含标点/大小写）
- 含中文的源文件必须保存为 UTF-8 with BOM（单层）
- 本批 4 处 mover 域为第十二批跳过的"内部防御/调试报告"性质报错，参照第十七批先例纳入全清
- wsf 库被 wsf_mil/mission/wizard 全链路依赖，编译目标选 wsf + mission 即可覆盖验证
