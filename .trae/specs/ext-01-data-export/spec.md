# 数据输出工作包规格（ext-01-data-export）

> 序号：ext-01 ｜ 功能：仿真数据出口（Observer/中间件/落盘/热切换） ｜ 状态：**阶段 A/B/C/D 全部完成并验收（2026-10-19，ext-01 收官）**
> 优先级：P0（第一阶段"打通数据"） ｜ 依据：roadmap §2 扩展点分层

## 1. 背景与目标

把 AFSIM 仿真运行数据（平台状态、航迹、探测、交战、毁伤、命令）零侵入地送出去，供外部系统消费。

- 约束：**只读采集，不改核心**，能挂 `WsfObserver::*` 就不碰 WSF 源码
- 入口：`WsfPluginVersion` + `WsfPluginSetup` 导出，DLL 入 `wsf_plugins`，/MD，编译器版本一致
- 验证基线：不影响仿真正确性、可随时卸载

## 2. 工作包拆分（3 个子模块）

| 子模块 | 内容 | 依赖 |
|---|---|---|
| A. Observer 插件骨架 | 订阅事件 → 序列化外发的最小闭环 | 无 |
| B. 统一消息中间件 | 内部统一消息结构 + 多通道适配器插拔 | A |
| C. 结构化落盘 | Parquet/CSV/SQLite + 评估指标表 | A（或独立） |

## 3. 决策明细（A：Observer 骨架）

订阅事件清单（WsfObserver 命名空间）：

| 事件 | 数据内容 | 序列化目标 |
|---|---|---|
| AdvanceTime | 仿真时钟同步 | Log/心跳 |
| MoverUpdated | 平台位置姿态（WGS84 + 姿态角） | PlatformState |
| TrackCreated/TrackUpdated/TrackDestroyed | 航迹生命周期与属性 | Track |
| Detection 类 | 探测/被探测 | Detection |
| WeaponLaunch / Engagement 类 | 发射、交战事件 | Engagement |
| Damage / Kill | 毁伤结果 | Kill |

序列化选型：**Protobuf 优先**（跨语言、紧凑）；自有后端可用 JSON。

回调中只做"取数 + 打包 + 入队"，网络/落盘由独立发送线程消费队列，避免阻塞仿真线程。

## 4. 决策明细（B：统一消息中间件）

内部消息类型固定为：`PlatformState / Track / Engagement / Command / Log`。

外部通道按适配器插拔，不为一协议写一插件：

| 通道 | 适用场景 | 备注 |
|---|---|---|
| TCP 行协议 / Protobuf | 自有后端、Java/Go 服务 | 参考 wsf_external_control 31000 端口模式 |
| UDP 组播 | 低延迟态势、多客户端 | Warlock 同屏多席位 |
| WebSocket | Web 前端（Cesium/Vue/React） | 最易对接浏览器 |
| ZeroMQ / DDS | 多节点实时、数实联动 | DDS 支持 QoS |
| Kafka / MQTT | 数据落湖、异步解耦 | 事后分析与训练数据 |
| gRPC / REST | 上层业务系统 | "仿真即服务" |
| 共享内存 / mmap | 同机超低延迟 | 微秒级替代 TCP |

**性能红线**：AdvanceTime 内增量推送 + 阈值死区 + 降采样，禁止每帧全量遍历。

## 5. 决策明细（C：结构化落盘）

- Parquet/CSV 时序落库（事后分析）
- SQLite 快照（断点状态）
- 评估指标表：命中率、发现时间、杀伤链时长等聚合结果（减轻 Mystic 后处理压力）

## 6. 验证与收尾标准

1. Observer 骨架在官方示例场景跑通，外部客户端可见平台状态流
2. 中间件切换适配器（TCP↔WebSocket）不重启仿真
3. 高频场景下仿真帧率下降 < 5%（增量推送生效）
4. 时间语义：外部时钟驱动 vs 仿真时钟外推策略先定死（roadmap §4.2）
5. 坐标系统一 WGS84（roadmap §4.3），UTC 与仿真时标双向带转换

## 7. 阶段 A 实施规格（2026-10-19，基于源码侦察定稿）

### 7.1 用户原始方案 vs AFSIM 2.9 真实机制（侦察修正）

| 用户方案 | AFSIM 2.9 实际 | 落地方式 |
|---|---|---|
| `Mission.exe xx.txt --param speed:500` | mission 命令行仅 `-rt/-fs/-es/-fio/-sm/-mi/-list-variables/-help` + 输入文件；**无 --param**；但扩展可经 `WsfApplicationExtension::ProcessCommandLine` 注册自有参数 | 插件提供 `--ysim-port <n>`；想定参数化用原生预处理变量 `$define`+`$<var:默认值>$`+驱动文件 include |
| SDL 中 `require_plugin` 挂载插件 | 全源码无 `require_plugin` 命令；`WsfApplication` 构造期自动扫描 `WSF_PLUGIN_PATH` 环境变量 + `../<app>_plugins` + `../wsf_plugins` 加载 DLL（导出 `WsfPluginVersion`/`WsfPluginSetup`） | DLL 放 `mission_plugins/`（`swdev_plugin_install` 自动部署），mission.exe 启动即挂载 |
| 继承 `WsfProcessor` 在 `FrameComplete` 推流 | `WsfProcessor` 必须挂到具体平台才被 Update，全局态势导出需改想定中每个平台（违背零侵入）；`WsfSimulationExtension` 无逐帧回调 | 采用 `WsfApplicationExtension → WsfScenarioExtension → WsfSimulationExtension` 扩展链（ApplicationExtension 在用户允许清单内），逐帧回调用 `WsfObserver::AdvanceTime`（每帧触发，等价 FrameComplete），零侵入想定 |

### 7.2 已确认的真实 API 签名

- 插件入口：`extern "C" UT_PLUGIN_EXPORT void WsfPluginVersion(UtPluginVersion&)` + `WsfPluginSetup(WsfApplication&)`（版本宏 `WSF_PLUGIN_API_MAJOR_VERSION/MINOR_VERSION/COMPILER_STRING`）
- 扩展注册：`WsfApplicationExtension::ScenarioCreated(WsfScenario&)` 中 `aScenario.RegisterExtension(name, ut::make_unique<...>())`；`WsfScenarioExtension::SimulationCreated(WsfSimulation&)` 中 `WsfPrivate::RegisterSimulationExtension(...)` 或 `RegisterExtension`
- 配置块：`WsfScenarioExtension::ProcessInput(UtInput&)` 处理想定级命令（`UtInputBlock` 读块）
- Observer（均带 `const WsfSimulation*` 取回调列表）：
  - `WsfObserver::AdvanceTime(sim)` → `void(double aSimTime)`（WsfSimulationObserver.hpp）
  - `WsfObserver::MoverUpdated(sim)` → `void(double aSimTime, WsfMover*)`（WsfMoverObserver.hpp；平台经 `aMoverPtr->GetPlatform()`）
  - `WsfObserver::PlatformAdded/PlatformDeleted(sim)` → `void(double, WsfPlatform*)`
- 仿真扩展生命周期：`Initialize() / PlatformsInitialized() / Start() / Complete(double)`

### 7.3 插件定稿设计（ysim_data_export）

- **位置**：`src/wsf_plugins/ysim_data_export/`（`wsf_module` 标记 + `wsf_cmake_extension.cmake`（WSF_EXT_TYPE=plugin）+ `CMakeLists.txt` + `source/YsimDataExport.cpp` 单文件最小闭环）
- **构建**：模仿 wsf_alternate_locations 手写模式（`add_library` + `target_link_libraries(${PROJECT_NAME} ${WSF_LIBS} ws2_32)` + `generate_export_header` + `swdev_plugin_install(${PROJECT_NAME} mission_plugins)`）；re-configure 后 `BUILD_WITH_ysim_data_export` 默认启用
- **想定级配置块**（ScenarioExtension::ProcessInput）：

  ```
  ysim_data_export
     port              31600      # TCP 监听端口（默认 31600）
     heartbeat_interval 5 s       # AdvanceTime 心跳周期
     deadband           2 m       # 位置死区：位移小于此值跳过推送
     min_update_interval 0.2 s    # 同一平台两次推送最小间隔（降采样）
  end_ysim_data_export
  ```

- **命令行**：`--ysim-port <n>`（ApplicationExtension::ProcessCommandLine，命令行优先于想定配置）
- **消息协议**（阶段 A：JSON 行协议，UTF-8，`\n` 分隔；Protobuf 留阶段 B）：
  `sim_start / platform_added / platform_removed / state / heartbeat / sim_complete`；`state` 字段：time/id/name/type/side/lat/lon/alt_m/speed_mps/heading_deg（WGS84，经纬度弧度→度，航向弧度→度）
- **线程模型**（性能红线）：回调线程只做"取数→拼 JSON→入队"（mutex+deque，上限 10000 条丢最旧）；独立 TCP 线程 accept 多客户端 + select 可写检查 + 广播发送，断线即剔除，best-effort 不阻塞仿真
- **代码红线**：中文注释 + `// 二次开发：` 前缀；源文件 UTF-8 单层 BOM；不碰 WSF 核心源码

### 7.4 验证方案（冒烟闭环）

1. 编译目标 `ysim_data_export` → DLL 落 `build/Release/mission_plugins/`
2. 演示想定：复用官方 demo 场景追加 `ysim_data_export` 配置块（temp 下制作用于冒烟，正式样例随插件归档）
3. `build/Release/mission.exe <想定>` 无头运行；`temp/ysim_tcp_client.py` 连 31600 端口收流
4. 通过标准：依次收到 sim_start → platform_added → state（经纬度随时间变化）→ sim_complete；mission 正常退出无崩溃；无客户端连接时仿真照常运行（插件不拖垮引擎）

### 7.5 验证结论（2026-10-19 实测归档）

| 验证项 | 结果 |
|---|---|
| 编译 + 部署 | ✅ DLL 自动落 mission_plugins/，mission.exe 启动日志确认加载 |
| 闭环冒烟（官方 2v2 想定 + -rt） | ✅ 333 行 JSON 全有效：state 324（4 平台 81 帧，经纬度随时间变化）/ heartbeat 4 / platform_removed 4 / sim_complete 1，解析零失败 |
| --ysim-port 32700 命令行 | ✅ 覆盖想定配置，客户端在 32700 收到全量数据流（**选项必须置于文件名之前**，框架约定） |
| 无客户端场景 | ✅ 退出码 0，帧利用率 3.8%，插件不拖垮引擎 |

实施踩坑记录（后续批次必读）：
1. **Edit 工具会剥 BOM**——含中文源文件每次编辑后必须字节级复查并补单层 BOM（本轮 2 次中招，无 BOM 时 MSVC 报行号漂移的 C2447/C1004）
2. **JSON 字符串拼接链是重灾区**——字段值闭引号 `\"\","` 在多行 `+` 拼接中极易丢失（本轮 3 处），拼装后必须用真实客户端 json.loads 全量验证，禁止只看编译通过
3. **demo 想定非自包含**——air_to_air/scenarios/2v2_air_movers.txt 依赖外层 2v2.txt 定义 script_variables（log_print 等extern），冒烟须 include 官方 master 想定；且相对路径以 CWD 解析，运行目录须为 demos/air_to_air
4. **WsfStandardApplication::InvalidCommandLineArgument 未导出**——插件侧命令行异常须本地派生 WsfApplication::Exception（内联类可链接）

## 8. 阶段 B 实施规格：统一消息中间件（2026-10-19）

### 8.1 适配器抽象（Channel 基类）

- 单文件内抽象基类 `Channel`：统一"入队（仿真线程）→ 独立发送线程 → 广播"骨架；派生类只实现 `Run()` 网络线程主体
- 共性红线沿用阶段 A：回调零阻塞（队列上限 10000 丢最旧）、SO_SNDTIMEO 200ms、断线剔除、best-effort、通道启动失败仅告警不拖垮仿真
- 消息层与通道层解耦：消息统一为 JSON 行（UTF-8 + '\n'），通道层不感知消息语义（UDP 每行一个数据报；WebSocket 每行一个文本帧，payload 剥离尾 '\n'）

### 8.2 三个首适配器

| 适配器 | 端口/参数 | 机制 | 快照补推 |
|---|---|---|---|
| TcpChannel | tcp_port 31600 | 阶段 A TcpServer 原逻辑重构 | ✅ accept 后先发快照再入广播流 |
| WebSocketChannel | websocket_port 31601 | **最小 RFC 6455 服务端**（自实现，零第三方依赖）：HTTP Upgrade 握手（Sec-WebSocket-Accept = Base64(SHA1(key+GUID))，SHA-1/Base64 内置实现）+ 文本帧封装（FIN+opcode1，7/16/64 位长度编码）+ ping→pong 应答 + close 帧断开；只做单向推送，客户端上行数据一律忽略 | ✅ 握手成功即补推 |
| UdpChannel | udp_address 239.16.16.16 + udp_port 31602 + udp_ttl 4 | 组播/单播自适应（224-239 首段判组播：设 TTL + 本机环回），每条消息一个数据报；连接式语义无快照 | ❌（无连接语义） |

### 8.3 配置与命令行扩展

- 想定块新关键字：`tcp_port`（`port` 保留为别名）/ `websocket_port` / `udp_address` / `udp_port` / `udp_ttl`；端口 0 = 禁用该通道，默认三通道全开（31600/31601/239.16.16.16:31602）
- 命令行新增：`--ysim-ws-port <n>` / `--ysim-udp-port <n>`（与 `--ysim-port` 同语义：优先于想定、必须置于文件名之前）

### 8.4 新客户端快照补推（断线重连的第一步）

- SimulationExtension 维护双缓存：`元信息帧`（platform_added）+ `最近状态帧`（state）per 平台，互斥锁保护（仿真线程写/网络线程读）
- 新 TCP/WS 客户端接入时先补推全量快照（各平台 platform_added + 最近 state），再进入实时广播流——中途接入的客户端不再"等下一帧才有画面"
- 掉线外推（预测推演）不在本批：留待外部时钟对接时一并定死时间语义

### 8.5 统一消息结构（五类型映射表）

线格式（wire format）保持阶段 A 的 JSON 行协议不变（向后兼容），消息目录与五概念类型映射如下；Track/Engagement 数据源订阅属后续数据面扩展批次：

| 线类型 type | 概念类型 | 状态 |
|---|---|---|
| state | PlatformState | ✅ 已实现 |
| platform_added / platform_removed | PlatformState（生命周期） | ✅ 已实现 |
| sim_start / sim_complete / heartbeat | Log | ✅ 已实现 |
| Track* / Engagement* / Command | Track / Engagement / Command | 待后续批次（新增 Observer 订阅） |

### 8.6 验证方案（阶段 B 冒烟）

1. 编译通过 + DLL 部署 mission_plugins
2. 想定三通道全开（tcp 31600 / ws 31601 / udp 239.16.16.16:31602），mission.exe -rt 无头运行
3. 三客户端并发收流：TCP（**延迟 4s 接入**，验证快照补推：接入即收 4×platform_added + 4×state）/ WebSocket（裸 socket 握手 + 文本帧解析）/ UDP（加入组播组收数据报）
4. 通过标准：三通道均收到 JSON 全有效流（解析零失败）且含 sim_complete；TCP 通道在接入后 2s 内收到全部 4 平台快照

### 8.7 验证结论（2026-10-19 实测归档）

三客户端并发（官方 2v2 空战想定，-rt 实时 20s）：

| 通道 | 结果 | 明细 |
|---|---|---|
| TCP（延迟 4s 接入） | ✅ PASS | 293 行全有效；接入 2s 内收到快照 4×platform_added（t=0 的事件只能来自快照缓存）+ 4×state 快照 + 实时流；心跳/移除/结束帧齐全 |
| WebSocket | ✅ PASS | 341 行全有效（含接入前的 sim_start），RFC 6455 握手 + 文本帧解析零失败 |
| UDP 组播 | ✅ PASS | 343 行全有效，数据报收流完整（含 sim_start） |

仿真本体：正常退出（退出码 0），帧利用率 3.19%（多通道并发开销可忽略）。

阶段 B 实施踩坑：
1. **C++ 访问控制陷阱**：通道公共选项函数初版写成 TcpChannel 私有静态成员，WebSocketChannel 调用报 C2248——跨类公共工具一律抽匿名命名空间自由函数
2. **容器重构遗漏**：TcpServer(vector)→WebSocketChannel(map) 重构时 Broadcast 仍按 `mClients[i].first` 索引访问，编译期暴露——重构容器类型时必须全量复查该容器的全部使用点
3. **帧构造器全面改用 snprintf 单模板**后，阶段 A 的拼接丢引号问题类别被根除（本批零 JSON 拼装错误）

### 8.8 阶段 B 遗留项（转入后续批次）

- Track/Engagement/Command 数据源订阅（新增 Observer：TrackCreated/WeaponLaunch/Damage 等）
- 掉线外推（预测推演）与外部时钟时间语义：待 ext-02 对接时一并定死
- 适配器热切换（运行期增删通道不重启仿真）：阶段 D 验收项

## 9. 阶段 C 实施规格：结构化落盘（2026-10-19）

### 9.1 选型结论（源码侦察）

| spec 目标 | 侦察结论 | 落地方式 |
|---|---|---|
| Parquet/CSV 时序落库 | 依赖树无 Arrow/Parquet（仅 zmq/zlib/tinyxml2/sqlite/sdl/qt/pybind11 等） | **CSV**（UTF-8 with BOM，Excel/pandas 双友好），Parquet 留待外部工具链引入后再议 |
| SQLite 快照 | 3rd_party 已带 sqlite-3.32.3-x64-vs2017 **静态库**（1.5MB lib，无 DLL 部署问题） | 静态链入插件；`swdev_package_*` 宏的 sqlite 变量仅 wizard 子树作用域可见（宏会静默跳过），插件 CMakeLists 按 `${CMAKE_BINARY_DIR}/3rd_party/sqlite-3.*` 解压约定直接定位 |
| 评估指标表 | 命中率/发现时间/杀伤链依赖 Track/Engagement 数据源（§8.8 遗留） | 本批先落地**平台级聚合指标**（数据源已有），metrics 采用行式 key-value schema，后续交战类指标直接插行扩展 |

### 9.2 Recorder 架构（与 Channel 平行的持久化 Sink）

- `Recorder` 独立类（非 Channel 派生）：持久化语义与网络广播不同（无客户端/快照概念）
- 线程模型沿用性能红线：仿真回调只填 POD 记录入队（独立队列+锁，上限 10000 丢最旧）→ 独立落盘线程批量消费
- 记录结构：平铺超集 `Record`（kind + 15 标量 + 3 字符串），单队列保序；kind ∈ {state, event, metric}
- CSV 转义：含逗号/引号字段按 RFC 4180 引号包裹；每批 fflush 防崩溃丢数据
- SQLite：`journal_mode=OFF; synchronous=OFF`（分析导出非业务库，吞吐优先），每批事务提交，states/events/metrics 三条 prepared insert
- 失败降级：目录创建失败/文件打开失败/SQLite open 失败仅告警并禁用对应 Sink，仿真照常

### 9.3 输出文件布局（每轮运行独立子目录，蒙特卡洛多轮天然隔离）

```
<record_directory>/run_YYYYMMDD_HHMMSS_mmm/
   platform_states.csv   # 时序：time,platform_idx,name,type,side,lat,lon,alt_m,speed_mps,heading_deg,pitch_deg,roll_deg
   events.csv            # 事件：time,event,platform_idx,name（sim_start/platform_added/platform_removed/sim_complete）
   metrics.csv           # 指标：metric,platform_idx,name,side,value,unit
   ysim_export.db        # SQLite：platforms(元信息+最终状态快照=断点恢复)/states/events/metrics 四表
```

### 9.4 评估指标（本批平台级聚合；仿真回调线程内联累计，无锁）

| 指标 | 含义 |
|---|---|
| survival_time | 平台生存时长（removed-added；未移除=仿真结束时间-added） |
| state_samples | 状态样本数（与推送流同源同节流） |
| avg_speed / max_speed | 平均/最大速度 |
| distance_m | 累计航程（相邻样本位移和，节流下为下近似） |
| 运行级 | platform_count / per_side_count / sim_duration / total_state_samples |

### 9.5 配置与命令行扩展

- 想定块新关键字：`record`（bool，默认 **false** 显式启用）/ `record_directory`（默认 `ysim_export`）/ `record_csv`（默认 true）/ `record_sqlite`（默认 true）
- 命令行：`--ysim-record <dir>`（启用落盘并指定输出根目录，优先于想定，须置于文件名之前）
- 落盘默认关闭的理由：写文件是有副作用动作（区别于无客户端零成本的通道），显式启用符合工程习惯

### 9.6 验证方案（阶段 C 冒烟）

1. 编译部署：DLL 含静态 SQLite 链接成功
2. 2v2 想定 + record 配置全开运行：三 CSV 行数合理（states ≈ TCP 流 state 数）、events 含全生命周期帧、metrics 4 平台×5 指标+运行级；python sqlite3 验证 .db 四表结构/行数/platforms 最终状态非空/metrics 与 CSV 一致
3. 落盘与推流并发互不干扰（TCP 客户端照常收流）
4. 默认关闭回归：无 record 配置不生成任何文件

### 9.7 验证结论（2026-10-19 实测归档）

| 验证项 | 结果 | 明细 |
|---|---|---|
| 编译部署 | ✅ | DLL 1.12MB（静态 SQLite 链入），mission_plugins 同步部署 |
| 纯落盘冒烟（2v2 想定，通道全关） | ✅ | states=164 / events=10（sim_start+4×added+4×removed+sim_complete）/ metrics=25（4 平台×5 指标+运行级 5）；SQLite 四表行数与 CSV 完全一致；platforms 快照表 4 行最终状态非空；survival_time 与 events 时间差、state_samples 与 CSV 行数、total_state_samples 全部对得上 |
| 三通道+落盘并发（-rt 20s） | ✅ | 三通道收流 293/341/343 零解析失败（与阶段 B 同水平）；**落盘 states=328 与 WS/UDP 流 state 数完全一致**（同源同节流实证）；落盘校验全 PASS |
| 默认关闭回归 | ✅ | 无 record 配置跑完整想定，CWD 无 ysim_export 目录生成 |
| 校验工具 | — | temp/ysim_record_verify.py（六类断言）；冒烟想定 temp/ysim_smoke_record.txt、ysim_smoke_full.txt |

验证期发现并修复的问题：
1. **C++14 折叠表达式不可用**（WSF 插件 CMake 标准）：BindAll 变参模板改手写逐参 sqlite3_bind_* 序列
2. 首次运行失败为 CWD 问题（event_output 相对路径），非插件缺陷：运行目录须为 demos/air_to_air（既有约定）

### 9.8 阶段 C 遗留项（转入后续批次）

- 命中率/发现时间/杀伤链时长等交战类指标（依赖 Track/Engagement Observer 订阅批次）
- Parquet 列式输出（待外部工具链/依赖引入决策）
- 落盘与推流独立节流（当前同源，保证落盘数据与实时流一致）

## 10. 阶段 D 实施规格：验收 + 适配器热切换（2026-10-19）

### 10.1 适配器热切换（运行期增删通道不重启仿真）

- **ControlChannel 控制通道**（独立类，不进广播列表）：TCP 行协议监听 `control_port`（想定配置，默认 0=禁用；命令行 `--ysim-control-port <n>`）；单客户端串行命令循环（select 20ms 节拍与数据通道一致）
- **命令集**（空格分隔 token，避免引入手写 JSON 解析器；响应仍为 JSON 行与数据流格式统一）：

| 命令 | 语义 |
|---|---|
| `add_tcp <port>` | 运行期新增 TCP 数据通道 |
| `add_ws <port>` | 运行期新增 WebSocket 数据通道 |
| `add_udp <address> <port> [ttl]` | 运行期新增 UDP 通道（组播/单播自适应） |
| `remove <port>` | 按端口移除通道（排空后停线程；控制端口自身不可移除） |
| `list` | 返回当前通道清单 |
| `help` | 命令帮助 |

- 响应帧：`{"type":"control_result","ok":true|false,"detail":"..."}`；`list` 另回 `{"type":"channel_list","channels":[...]}`

### 10.2 并发模型（通道列表热替换）

- `mChannels` 由 `std::shared_timed_mutex`（C++14）保护：仿真线程 Publish 持 shared_lock 只读遍历；控制线程增删持 unique_lock
- **停机在锁外**：remove 先锁内摘链、锁外 FlushAndStop（内含 150ms 排空等待，持锁会阻塞仿真帧）
- 新增通道注入同一快照 provider（中途新增通道的新客户端同样享受快照补推）
- 端口冲突：add 端口已被占用（含配置期通道）返回失败；端口占用检测在 Start 内 bind 失败自然暴露

### 10.3 验收方案

1. **官方示例全流程数据可见**：2v2 空战想定（含交战/毁伤——4 平台被击落即 platform_removed）全开运行，TCP/WS/UDP 收流含完整生命周期帧（sim_start → added×4 → state 流 → removed×4 → sim_complete）+ 落盘四表 + 指标齐全（复用 ysim_record_verify.py 六类断言）
2. **热切换**：-rt 长跑中途控制客户端执行 add_udp 新增通道 → 新 UDP 客户端入组收实时流（组播无连接语义，快照补推仅适用 TCP/WS）→ remove 移除后该通道停流而仿真不中断；add_tcp 热切换通道的新客户端收快照补推 + 实时流
3. **帧率下降 < 5%**：同一想定（60s 截断、事件驱动非实时）跑两遍对比 Elapsed Processor Time——基线（mission_plugins 中 DLL 改名禁用）vs 全开（三通道+落盘+控制通道+客户端收流）；判据 (P_full − P_base)/P_base < 5%（总 CPU 时间含网络线程，是最严口径）
4. 记账三件套归档（spec/plan/tasks/README/修改日志/project_memory）

### 10.4 验证结论（2026-10-19 实测归档）

| 验收项 | 结果 | 明细 |
|---|---|---|
| 官方示例全流程数据可见 | ✅ | 2v2 空战想定 40s -rt 长跑：TCP 31600 全程完整生命周期流（sim_start→added×4→state 流→removed×4→sim_complete，解析零失败）；落盘 states=648 与推流完全一致、四表齐全、指标 25 条（4 平台×5+运行级 5） |
| 适配器热切换 | ✅ | 13/13 断言 PASS：list/add_udp/remove/add_tcp/remove 全链路；add_tcp 热通道新客户端收快照补推（4×platform_added）+实时流；UDP 热通道入组收实时流（组播无连接语义，无快照属预期）；remove 后双通道均停流；31600 全程不断流；仿真正常退出 |
| 帧率下降 < 5% | ✅ | **-rt 实时模式（红线口径）：基线 60.261s vs 全开 60.508s = +0.41%**；事件驱动全速模式归因：600s 想定 +11%（空帧回调 0.44μs × 60 万帧放大 + 100ms 固定尾部）、60s 想定 +80%（基线仅 0.28s 墙钟，属采集成本非帧率损失） |
| 归档 | ✅ | spec/plan/tasks/README/修改日志/project_memory 六件套 |

验证期发现并修复的问题：
1. **Complete 串行停机尾延迟**：原逐通道 FlushAndStop（各含 150ms sleep）→ 600s 全速 +24% 墙钟；改为单次 100ms 排空 + 并行 Stop，收敛至 +11%（全速口径）/+0.41%（实时口径）
2. 帧率基准的口径修正：事件驱动全速模式的墙钟增量是"采集成本"（数据没白采），帧率红线的工程度量应为 -rt 实时模式仿真是否跟上实时节拍

### 10.5 阶段 D 遗留项（ext-01 收官后转后续批次）

- Track/Engagement/Command 数据源订阅与交战类指标（命中率/发现时间/杀伤链）——ext-01 之外首个候选批次
- 掉线外推与外部时钟时间语义（待 ext-02 对接定死）
- 控制通道认证（当前无鉴权，仅限可信内网使用）
- Parquet 列式输出（待依赖决策）
