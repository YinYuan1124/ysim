# 外部接入工作包规格（ext-02-protocol-gateway）

> 序号：ext-02 ｜ 功能：协议网关与外部系统接入 ｜ 状态：**阶段 A 已完成（XIO 自定义 Packet 端到端 + 指令注入闭环 + 平台级指令集，2026-10-19）**
> 优先级：P1（第二阶段"标准化对外"） ｜ 依赖：ext-01 的数据通道

## 1. 背景与目标

把外部系统"接进来"：标准协议（DIS/HLA/XIO）、实装数据（数实联动/硬件在环）、以及作为推演引擎被业务系统调用。

- 约束：全部通过扩展点实现（XIO Service / Scenario Extension / 组件 Mover），不改核心
- 共性工程：**时间同步**（外部时钟驱动 vs 仿真时钟外推）与**坐标/时标统一**（roadmap §4）优先定死

## 2. 子模块与决策明细

### A. 标准协议网关（LVC 主线）

**DIS（IEEE 1278）** — 2.9 自带 dis_interface（WsfDisInterface + WsfXIO_DisService），增量空间：

| 增量项 | 说明 |
|---|---|
| 自定义 PDU 编解码 | 私有 PDU 族 |
| Entity Type 七段码映射表 | 完善国产装备枚举映射 |
| 电磁发射/Fire/Detonation/Signal PDU 字段覆盖 | 补齐现覆盖缺口 |
| DIS↔XIO 双向中继 | 一个实例连外部 DIS，全网可见 |

**HLA（IEEE 1516）** — 走 FOM 定制 + 时间管理（保守/乐观）对接 + HLA↔DIS 协议转换网关。

**XIO 自定义 Packet** — 挂自有 Service 扩展，定义私有 Packet ID 与序列化操作符（适合自研 GUI/外部算法/人在回路）。

### B. 数实联动 / 硬件在环（HIL）

- MAVLink / 1553B / ARINC429 解析 → 映射为平台 Mover 更新或 External Control 指令，实现虚实同场
- 雷达/导引头硬件在环：反射内存网（RFM）微秒级共享内存通信
- **风险点**：时间同步与时钟外推（外部时钟驱动 vs 仿真时钟），必须先行设计

### C. 推演引擎后端（仿真即服务）

- AFSIM 封装为无头服务，外部兵棋/指挥信息系统下发决心与指令、回收态势与裁决
- 典型形态：Spring Boot / .NET 后端经 TCP/gRPC 长连接驱动，前端只展示
- 与 ext-01 中间件复用统一消息结构与 Command 通道

## 3. 验证与收尾标准

1. DIS 标准 PDU 与外部 DIS 工具互通（至少 Fire/Detonation/ElectromagneticEmission）
2. XIO 自定义 Packet 端到端（自研客户端 ↔ AFSIM）
3. HIL：MAVLink 模拟器驱动平台位置，误差在容忍域内
4. 推演后端：外部下发指令 → 仿真执行 → 裁决结果回收（gRPC 契约测试）
5. 时间同步策略文档化：lock-step vs 异步 + 掉线外推

### 3.1 demo 直播化改造（2026-10-21，测试资产维护，与 ext-01 §6.1 同批）

用户反馈：dis 两个 demo bat 与 xio verify 直跑时控制台无输出。改造（插件 DLL 零改动）：

1. **dis_listen.py**：首包到达、每实体首次出现（Marking + 阵营 + 七段码 = B1 注入效果直读）、
   2s 心跳（PDU 速率/类型分布）；RESULT JSON 行契约不变（b1_verify 只认 `{"RESULT` 前缀）
2. **dis_capture.py**：Fire/Detonation 逐条直播（武器七段码/弹头/引信/爆点结果），
   Emission 首条+1/40 抽样，2s 心跳；RESULT 行契约不变
3. **两个 bat**：mission 窗口最小化+重定向 → 可见直出（与 ext-01 demo 同样式）
4. **xio_e2e_verify.py**：运行期流式直播——mission 侧仅透传 `ysim_xio_gateway` 行
   （received/executed command 全链路实时可见）+ 5s 心跳；文件句柄边写边读，
   断言逻辑与 RESULT 消费不变（文件读用独立 read_text 复核）
5. **验证**：dis_listen.bat（实体七段码 225/36 直播）、dis_capture.bat（2 Fire/2 Det 实时）、
   xio_e2e_verify.py 全程（两轮指令闭环直播 + 回归 PASS）实跑通过
6. **xio_demo.bat 补齐**（2026-10-21 同日，按用户诉求）：XIO 目录原只有验证脚本无 demo；
   新增三窗联动纯 bat demo（客户端窗 + mission 窗原生直出，规避验证脚本管道缓冲不可见问题）——
   控制窗按 235.7.11.27/3002 组网参数先起 xio_gateway_client（180s）→ 3s 后起 mission
   （xio_rt2 想定，RT 45s）→ 双窗直读指令闭环 → 75s 后定向清理；实跑验证进程起停与清理零残留。
   tests\ demo bat 总数 6→7
7. **xio_demo.bat 控制窗详细日志**（2026-10-21 同日，按用户诉求）：控制窗升级为分步日志——
   环境检查（client/mission/插件 DLL/组网块/想定 5 项逐条 FOUND/MISSING，缺项中止）、分步启动说明、
   5s 一跳双进程存活监测（mission 自退即提前收尾，上限 75s）、清理结果可见输出（taskkill SUCCESS +
   tasklist 双 [OK] 复核）、结束汇总。坑：`timeout.exe` 在 stdin 重定向环境报
   "Input redirection is not supported" 直接跳过等待 → 首跳在子进程未起完时误判 exited 提前清理；
   改用 stdin 无关的 `ping -n N 127.0.0.1` 计时 + 先等后查（tick 标签与真实墙钟一致）

## 4. 阶段 A 实施规格：XIO 自定义 Packet 端到端（2026-10-19）

### 4.1 源码侦察结论

| 要点 | 事实 | 出处 |
|---|---|---|
| XIO 底层库 | packetio（PakProcessor/PakPacket/PakTCP_*），独立库目标，构建产物 build\Release\lib\packetio.lib | src/tools/packetio/source/ |
| XIO 主接口 | `WsfXIO_Interface : public WsfXIO_InputData, public PakProcessor`（WSF_EXPORT，可外部链接）；Send/SendToAllTCP/Connect 模板订阅 | src/core/wsf/source/xio/WsfXIO_Interface.hpp |
| 插件取接口 | `WsfXIO_Extension::Find(const WsfSimulation&)`（mission.cpp 已 WSF_REGISTER_EXTENSION(app, xio_interface)） | xio_sim/WsfXIO_Extension.hpp |
| Packet ID 预留 | 115-199 unused；**应用 2 段 200-349**，350+ unused——本插件取 350-352 | xio/WsfXIO_Packet.hpp L31-36 |
| Packet 定义宏 | `XIO_DEFINE_PACKET(Name, Base, Id)` + `using namespace PakSerialization; aBuff& f1& f2` | WsfXIO_PacketRegistry.hpp HeartbeatPkt 范本 |
| 注册/订阅 | `RegisterPacket<Pkt>(id, "name", new Pkt{})`；`Connect<Pkt>(&Cls::Handle, this)` | PakProcessor.hpp；LogServerThread.cpp 范本 |
| 组网模型 | UDP 多播心跳自动发现 + TCP 可靠连接（Accept 后 InitializePkt 三段握手）；想定样例 demos/iads/xio_interface.txt（multicast 235.7.11.27 3002 + port + connect_to_simulations） | WsfXIO_Interface.cpp:297-324 |
| 客户端形态 | 编程式填 WsfXIO_InputData（mUDP_Targets/AddConnectType/mTCP_Port）构造 WsfXIO_Interface，独立驱动循环 AdvanceTime | WsfXIO_InputData.hpp 成员公开 |

### 4.2 设计

- **插件 `ysim_xio_gateway`**（src/wsf_plugins/，三件套同 ext-01 范式）：
  - `SimulationExtension`：Initialize 时 `WsfXIO_Extension::Find(sim)` 取接口（null = 想定未启用 xio_interface，告警并闲置，不阻断仿真）；向 PakProcessor 注册三个私有 Packet 并 Connect 订阅上行
  - 私有 Packet 族（app2 段）：`YsimGwEventPkt(350)`（event 枚举+平台元信息，0=sim_start/1=platform_added/2=platform_removed/3=sim_complete）、`YsimGwStatePkt(351)`（位置姿态时序）、`YsimGwCommandPkt(352)`（上行指令：command+arg 字符串）
  - 数据源：复用 ext-01 的 WsfObserver 零侵入订阅（PlatformAdded/Deleted/MoverUpdated/Start/Complete）；死区+降采样节流沿用 ext-01 参数
  - 发送：`SendToAllTCP(pkt)`；上行 CommandPkt 回调仅登记+日志（指令注入仿真属后续批次，需线程安全投递设计）
- **外部客户端 `xio_gateway_client`**（temp/ 下独立 CMake 工程，链 build\Release\lib 的 wsf/packetio/util 等）：编程式 InputData（多播心跳与 mission 同组）+ 注册同 ID Packet + 打印收流 + 定时上行 CommandPkt
- 验证：mission（2v2 想定 + xio_interface 块 + 插件）与客户端并发——客户端收全事件+状态流（计数断言）、上行命令被 mission 侧登记（日志验证）

### 4.3 验证结论（2026-10-19，全部 PASS）

**实现落地**：
- 插件 `ysim_xio_gateway`（src/wsf_plugins/，单文件 YsimXioGateway.cpp ~430 行）：XIO 接口发现（未启用则告警闲置不阻断）+ 三 Packet 注册（350 事件/351 状态/352 上行）+ WsfObserver 订阅（PlatformAdded/Deleted/MoverUpdated/AdvanceTime）+ 死区 2m/降采样 0.2s 节流 + sim_start 连接后补发 + 既有平台快照补推（对齐 ext-01 快照机制）+ Complete 末包保送达（500ms flush）
- 外部客户端 `xio_gateway_client`（temp/xio_gateway_client/，独立 CMake 工程链构建树 wsf/packetio/genio/util）：编程式 InputData 多播心跳发现 + cRESUME 放行 + 收流统计 + 周期上行命令 + 断连兜底退出，RESULT JSON 行输出
- 组网配置：xio_interface 块（multicast 235.7.11.27 192.168. 3002 + heartbeat 1s + **pause_for_connection true** + send_aux_data_updates false）

**端到端验证（temp/xio_e2e_verify.py，两轮 + 降级回归，25 断言全 PASS）**：
| 断言 | 结果 |
|---|---|
| mission/client 退出码 0×2 轮 | PASS |
| XIO 连接建立（多播发现+TCP 握手） | PASS |
| sim_start 恰 1 次（连接后补发） | PASS |
| sim_complete 恰 1 次（末包保送达） | PASS |
| 平台事件增删平衡且含初始快照（14=14，4 初始平台全可见） | PASS |
| 状态流规模 ~2 万帧（2v2 全速 3600s） | PASS |
| 上行命令 mission 侧全量登记（3=3） | PASS |
| 未知包告警清零（关闭原生 AuxData 推送后） | PASS |
| 降级回归：无 xio_interface 块闲置告警+无 FATAL | PASS |

**关键机制结论**：
1. `pause_for_connection true` + 客户端 cRESUME 是全量事件可见的必要条件——否则全速 mission 在 XIO 握手完成前已推进数百秒，早期事件（含 4 初始平台 t=0 添加）全部丢失
2. t=0 初始平台添加发生在扩展 Start 之前的想定加载阶段，快照补推（连接后遍历在场平台补发 added）不可省
3. sim_complete 末帧包发送后需短等 flush，否则进程退出竞态吞包
4. XIO 应用过滤 `mApplicationTypeFilter.insert(cAPP_SIMULATION)` 使客户端主动连 mission；原生 AuxData 推送（ID 73）对未注册客户端是噪声，验证配置应关闭

### 4.4 指令注入闭环（阶段 A 延伸批次，2026-10-19 完成）

**规格**：上行 `YsimGwCommandPkt(352)` 从"登记+日志"升级为真实注入执行，新增下行 `YsimGwAckPkt(353)` 回执——每条上行指令必有一条 ack（含执行状态/执行时刻仿真时间/在场平台数）。

**线程模型**（XIO 网络线程与仿真线程的边界纪律）：
```
XIO 回调线程（HandleCommand）          仿真线程（OnAdvanceTime 帧回调）
  ├─ mutex 入队 mPendingCommands  →     ├─ mutex swap 出队批量
  └─ 日志登记                            └─ 逐条 ExecuteCommand：
                                            pause/resume → WsfSimulation::Pause()/Resume()
                                            get_status   → 查询回组（sim_time/平台数/状态）
                                            ping         → pong 回显
                                            未知          → UNKNOWN 回执（负向用例）
                                         └─ SendToAllTCP(ack)
```
关键依据：暂停态时钟冻结但 mission 主循环 `while(IsActive()){WaitForAdvanceTime();AdvanceTime();}` 仍空转、AdvanceTime 观察者仍每轮触发——**pause 期间 resume 指令照常出队执行，不死锁**。

**验证结论（temp/xio_e2e_verify.py，RT 短程想定 25s 两轮 + 降级回归，35 断言全 PASS）**：
| 断言 | 结果 |
|---|---|
| 5/5 指令 mission 侧全量执行（XIO 线程→仿真线程队列无损） | PASS |
| 正向指令 4 条全 OK 回执 + 未知指令 1 条 UNKNOWN 回执（负向） | PASS |
| pause 真实冻结实时时钟（pause/resume 回执同一冻结时刻 3.50s） | PASS |
| pause 期间状态流停更（6s 保持窗增量=0） | PASS |
| resume 后时钟恢复推进（get_status 回执 6.75s） | PASS |
| get_status 回报在场平台 4/事件流/状态流/未知包清零/降级回归 | PASS |

**关键机制结论**：
1. **`WsfSimulation::Pause()` 在事件驱动全速模式下是空操作**——时钟源 StopClock 不冻结事件步进，仿真照常跑完；pause/resume 的有效域是 `-rt` 实时模式，闭环验证须配短程想定（end_time 25s，复用 include 覆写）
2. 客户端指令脚本门控须以 **cRESUME 实际发出**为计时零点——mission 主动反连会在客户端 Initialize 期间提前触发 OnConnected（连接列表未就绪），以 mConnected 门控会把指令全发进空连接
3. 暂停冻结判定须浮点容差（相邻帧冻结时钟返回值存在亚毫秒差，精确 == 假阴性）
4. 仿真结束时平台统一清扫删除（PlatformDeleted 对全部在场平台触发），事件增删平衡断言应含结束清扫语义

### 4.5 平台级指令集（阶段 A 延伸批次，2026-10-19 完成）

**规格**：指令集从仿真级（pause/resume/get_status/ping）扩展到平台级控制——态势注入（实体增删）与运动控制（调速/航路重规划）。线程模型沿用 §4.4（XIO 线程 mutex 入队 → 仿真线程帧回调出队执行 + AckPkt 回执）；原 §5 规划写"走 WsfEvent::Add 事件机制投递"，实现为帧回调直接执行——AdvanceTime 观察者先于当帧事件派发触发，add/delete 同帧生效，与事件机制同帧等价且少一层间接。

**指令集**（参数空格分隔，arg[0] 恒为目标平台名）：

| 指令 | 用法 | API 路径 |
|---|---|---|
| set_speed | `<平台名> <速度 m/s>` | `WsfRouteMover::GoToSpeed(t, v, 5.0, keepRoute=true)` |
| set_route | `<平台名> <纬度°> <经度°> <高度 m> [速度 m/s]` | 双航点 `WsfRoute` + `WsfRouteMover::SetRoute` |
| add_platform | `<平台名> <平台类型>` | `WsfPlatformTypes::Clone(类型名)` + `SetName` + `WsfSimulation::AddPlatform`（全托管：Initialize→Initialize2→PlatformAdded 观察者） |
| delete_platform | `<平台名>` | `WsfSimulation::DeletePlatform(t, p, true)`（调度当帧一次性事件→PlatformDeleted→内存回收） |

**验证结论（temp/xio_e2e_verify.py，air_movers 版 RT 想定 45s 两轮 + 降级回归，47 断言全 PASS）**：
| 断言 | 结果 |
|---|---|
| 10/10 指令 mission 侧全量执行；8 OK / 1 ERROR（幽灵平台删除）/ 1 UNKNOWN（负向） | PASS |
| add_platform 动态注入：平台增事件 5（4 初始+1 注入）、get_status 回报 5、靶标 added/removed 事件齐 | PASS |
| set_route 弧度修复+双航点防瞬移：靶标平滑飞行位移 131/153m（缺陷版瞬移 233km），无瞬移 | PASS |
| set_speed 调速生效：靶标最大速度 31.2/33.8 m/s（加速能力 5 m/s² × 机动窗口 8s 量级吻合） | PASS |
| pause 冻结时钟 + 状态流停更 delta=0（平台级批次全量回归，含 §4.4 全部断言） | PASS |
| 平台事件增删平衡（含结束清扫）：added=removed=5 | PASS |
| 降级回归：无 xio_interface 块闲置告警 + 无 FATAL | PASS |

**关键机制结论**：
1. **`WsfPlatformTypes` 无 Create 方法**——运行期创建平台的标准路径是 `Clone(WsfStringId)`（WsfObjectTypeList.hpp:118，返回裸指针归 AddPlatform 托管）；动态类型须自包含（裸 `platform_type XIO_DRONE` 无基类会报 "Could not find platform_type mover"，挂 `AIR_MOVER_FIGHTER` 基类后裸克隆即可安全运行）
2. **WsfWaypoint 纬经度是弧度制**——外部协议约定度（与状态流一致），转换须 `* UtMath::cRAD_PER_DEG`（UtMath 无 cRAD 常量，只有 cRAD_PER_DEG=PI/180）；缺陷版 0.02° 被解释为 0.02rad=1.15° 实测位移 233km
3. **单航点航路会瞬移**——RouteMover 以航路首点为起始锚，单航点即直接跳到目标点；set_route 须组双航点（首点=当前位置 `GetLocationLLA`，末点=目标），首点速度用 `WsfPath::cUSE_PREVIOUS`
4. **set_route/set_speed 的类型域是 RouteMover 族**（WsfAirMover→WsfWaypointMover→WsfRouteMover）——brawler 想定（BLUE_FIGHTER 用 WSF_BRAWLER_MOVER）不可用，验证想定须切 air_movers 版（`2v2.txt` 的 SCNRIO 路径变量=2v2_air_movers，且须保留全局 script_variables——FileIO 日志对象被平台脚本运行期引用）
5. 机动窗口口径：pause 占 5s 的 45s 想定中，靶标实际机动窗口仅 8s（t=4~8 加速 + t=13~17），位移断言区间须按此物理口径设定（百米级），而非目标距离（公里级）

## 5. 阶段 B+ 规划（待实施）

阶段 A 已打通 XIO 事件/状态下行 + 指令注入闭环（仿真级 pause/resume/get_status/ping + 平台级 set_speed/set_route/add_platform/delete_platform，均带 AckPkt 回执）。后续批次按 plan.md 推进：
- ~~指令集扩展：平台级控制（航路重规划/参数注入/实体增删）~~ **已完成（§4.5，2026-10-19）**
- **DIS 增强**：实施规格见 §6；HLA / 数实联动 / 推演后端：见 §2 子模块决策明细

## 6. 阶段 B 实施规格：DIS 增强（2026-10-19 启动）

### 6.1 源码侦察结论（完整事实底账见 temp/recon_dis.md）

| 要点 | 事实 | 出处 |
|---|---|---|
| 源码树 | core：src/core/wsf/source/dis/；武器：src/core/wsf_mil/source/dis/；映射表真身在 ext/ | — |
| 扩展注册名 | `dis_interface`（想定含非平凡 dis_interface 块激活） | WsfDisExtension.cpp:76 |
| 接口发现 | `WsfDisInterface::Find(const WsfSimulation&)`（与 XIO 同范式，null=未启用） | dis/WsfDisInterface.hpp:91 |
| 七段码映射存储 | `WsfExtData::mObjectTypeToEntityType` 双向表（entity_type 命令写入） | ext/WsfExtInterface.cpp:311 |
| 映射查询回退 | 沿平台类型继承链 most→least 查表——**基类映射可被派生类继承**；未命中→icon→全 0 | WsfExtInterface.cpp:434-453,567 |
| 运行期注入点 | `WsfExtInterface::GetObjectToEntityTypeMap()` **返回可写 map 引用（WSF_EXPORT）** | ext/WsfExtInterface.hpp:230 |
| 查表触发 | WsfDisPlatform 构造本地实体时定 EntityState 七段码 | dis/WsfDisPlatform.cpp:159-167 |
| 自定义 PDU 点 | `WsfDisPduFactory::SetPduCreateFunction(pduType, fn)` + `UserCreate` override；发送 `PutPdu(simTime, unique_ptr<DisPdu>)` | dis/WsfDisPduFactory.hpp:30-50 |
| XIO↔DIS 既有桥 | xio_interface 硬依赖 dis_interface；WsfXIO_DisService 推 DisPlatformInfoPkt（信息级，非 PDU 级中继） | xio_sim/WsfXIO_Extension.cpp:168-177 |
| 验证资源 | test_mission/dis/emission 系列想定；DIS PDU 大端定长，Python struct 直解（免链构建树） | src/core/wsf/test_mission/dis/ |

### 6.2 批次切分与优先级

| 批次 | 任务 | 扩展点 | 风险 | 状态 |
|---|---|---|---|---|
| B1 | 七段码映射表（国产装备枚举） | 新插件运行期注入 GetObjectToEntityTypeMap | 低（数据驱动） | **已完成（§6.3，2026-10-19）** |
| B2 | 电磁发射/Fire/Detonation/Signal PDU 字段覆盖 | 观察者+PutPdu；先验证现有覆盖再定缺口 | 中 | 待 B1 后 |
| B3 | 自定义 PDU 编解码 | WsfDisPduFactory::SetPduCreateFunction | 中 | 排队 |
| B4 | DIS↔XIO 双向中继 | B1 插件 + 阶段 A XIO 网关复用 | 高 | 排队 |

### 6.3 批次 B1 规格：Entity Type 七段码映射表（国产装备枚举）

**插件 `ysim_dis_gateway`**（src/wsf_plugins/，三件套同 ext-01/ext-02A 范式，单文件 YsimDisGateway.cpp 起步）：

- **ApplicationExtension**：AddedToApplication 中 `ExtensionDepends("ysim_dis_gateway", "dis_interface", true)` 声明硬依赖（保证本插件扩展初始化晚于 dis_interface，注入时序安全）
- **SimulationExtension::Initialize**：
  1. `WsfDisInterface::Find(sim)` → null（想定未启用 DIS）则告警闲置、不阻断（同 XIO 网关范式）
  2. 经 `WsfDisUtil::GetExtInterface(interfacePtr)` 取 `WsfExtInterface*` → `GetObjectToEntityTypeMap()` 批量注入国产装备映射（键=AFSIM 平台类型名，值=WsfExtEntityType 七段码）
  3. 注入策略：仅插入不存在键（想定显式 entity_type 命令优先，插件补缺不覆盖）；日志打印注入条数
- **映射数据**：源码内 const 数组（国家码统一 SISO 36=中国），首批覆盖：
  - 想定常用基类/类型：FIGHTER/ATTACKER/BOMBER（歼击/攻击/轰炸类）、AEW_C2（预警指挥）、EW_RADAR/RADAR（雷达）、SAM_LAUNCHER/SAM_SITE（地空导弹）、MISSILE（导弹）、SURFACE_SHIP/CARRIER（舰船）、UAV（无人机）等 12-20 条
  - 语义段取值按 SISO REF-010 合理落位（kind/domain/category/subcategory/specific），非标自拟段须中文注释标注"自拟段"
- **时序风险兜底**：若扩展 Initialize 晚于平台创建（t=0 平台在想定加载阶段添加），注入失效——验证断言直接暴露；回退方案=注入提前至 AddedToSimulation
- **不改核心**：零侵入（GetObjectToEntityTypeMap 是官方导出接口），无 `// 二次开发：` 前缀改动

**验证（批次 B1，断言两轮+降级回归）**：
- 想定：2v2 基线 + `dis_interface` 块（exercise/site/application/multicast 224.2.25.54:3225 样例参数；含一条显式 entity_type 验证"想定优先不覆盖"语义）
- 外部客户端：`temp/dis_listen.py`（Python 最小 DIS 监听，join 多播组、解析 PDU 头 protocol=7/pduType=7/EntityState 体 entityType 七段）
- 断言：① mission 两轮退出码 0 ② 收到目标平台 EntityState 且七段码==插件注册值（country=36+段位匹配）③ 想定显式映射的平台七段码==显式值（未被插件覆盖）④ 未映射平台回退全 0 或继承基类值 ⑤ 降级回归：无 dis_interface 块→插件闲置告警+无 FATAL

**验证结论（2026-10-19，两轮 22 断言 + 降级回归，ALL PASS）**：

**实现落地**：
- 插件 `ysim_dis_gateway`（src/wsf_plugins/，单文件 YsimDisGateway.cpp）：映射表 20 条（kind/domain/category 按 SISO REF-010，country=36 统一，自拟段中文注释）+ AddedToSimulation 注入/Initialize 幂等补注 + 仅插不存在键（想定显式 entity_type 优先）+ `HasDevice()` 闲置判定（未配 DIS 设备告警不阻断）+ 软依赖 dis_interface 保序
- 外部客户端 `temp/dis_listen.py`：join 多播组 224.2.25.54:3225 + 大端 PDU 头/EntityState 解析（EntityType 偏移 20：EntityID 6B+ForceId 1B+pad 1B）+ 实体聚合 RESULT JSON
- 组网块 `temp/dis_iface_block.txt`：multicast 224.2.25.54 192.168.5.3 + port 3225 + autostart + 显式 entity_type BLUE_FIGHTER_AIR 1:2:225:2:1:1:0（美制，优先级验证键）

| 断言 | 结果 |
|---|---|
| mission/客户端退出码 0×2 轮 | PASS |
| DIS PDU 流 449 条（EntityState 48/Emission 21/Transmitter 152/Signal 76/Receiver 152） | PASS |
| 想定显式映射优先：blue_1/blue_2 七段码=[1,2,225,2,1,1,0] 未被插件覆盖 | PASS |
| 插件注入生效：red_1/red_2 七段码=[1,2,36,2,1,2,0]（RED_FIGHTER_AIR 国产值） | PASS |
| 实体数 4 + ForceId 1/1/2/2（友/敌阵营正确） | PASS |
| 降级回归：无 dis_interface 块→闲置告警+退出码 0+无 FATAL | PASS |

**关键机制结论**：
1. **EntityState 体偏移 20**：EntityID 6B 后是 ForceId 1B+pad 1B，再 EntityType 8B——按 18 解析会把 force 当 kind、错位到 country
2. **dis_interface 经 xio_interface 注册链无条件注册**（ExtensionDepends 硬依赖）——未启用 DIS 的判定须 `HasDevice()`，不能 Find()==null
3. **autostart 是外部 DIS 模式自跑开关**：无外部网关发 Start/Resume PDU 时须配置，否则 mission 不推进
4. **多播接口指定**：multicast 第二参数=出接口网卡 IP（非 127.0.0.1），客户端 join 组须同接口
5. **2v2 air_movers 想定原生就在发 Emission/Transmitter/Signal PDU**（449 条中 401 条非 EntityState）——B2 抓包基线现成
6. **映射注入时序无虞**：AddedToSimulation 必然早于想定平台创建（WsfDisPlatform 构造查表），t=0 初始平台即生效（blue/red 全部命中断言佐证）

### 6.4 批次 B2 规格：Fire/Detonation/Emission/Signal PDU 字段覆盖补齐（2026-10-19 定稿）

#### 6.4.1 侦察结论：发包侧字段缺口底账（源码逐字段核实）

| PDU | 已覆盖字段 | 恒缺字段（缺口） | 发包点 |
|---|---|---|---|
| Fire(2) | 三实体ID/Event/WeaponType七段码/Location/Velocity/Range | **Warhead 恒0**（GetWarheadEnum 查 mil_ext_interface warhead 空表默认 0="Other"）；**Fuse 恒0**（从不设置）；FireMissionIndex 恒0；Quantity 恒1/Rate 恒0（单发语义正确） | WsfDisWeaponEvents::WeaponFiredP（wsf_mil/source/dis/WsfDisWeaponEvents.cpp:87-305） |
| Detonation(3) | 三实体/WeaponType/Location/Velocity/ImpactLocation（显式武器+目标在场）/DetonationResult（leth/geom 语义映射） | **Warhead/Fuse 同 Fire** | WsfDisWeaponEvents::WeaponTerminated（同文件:581-815） |
| Emission(23) | EmittingEntityId/EventId/系统相对位置/beam 频率/功率/扫描参数 | **Emitter Name 恒0**（emitter_type 映射默认空）；**Emitter Function 恒0** | WsfDisPlatform::SendEmissionsPDU + SetEmitterTypeAndFunction（wsf/source/dis/WsfDisInterface.cpp:3313） |
| Signal(26) | EntityId/RadioId | **消息载荷空**（源码注释明说 "Send the Signal PDU with an empty message"）；EncodingClass 恒0 | WsfDisPlatform::MessageTransmitted（wsf/source/dis/WsfDisPlatform.cpp:598-678） |
| Transmitter(25) | RadioId/Frequency/Bandwidth/Power/Location/TransmitState | RadioEntityType（radio_entity_type 未配置全0）；ModulationType 恒0 | 同上（伴生 PDU） |

#### 6.4.2 补齐路径决策：官方发包前钩子改字段（非补发 PutPdu）

三个可用钩子均为官方导出（WSF_EXPORT/WSF_MIL_EXPORT），PutPdu 发出前调用、可写字段：

| 钩子 | 签名 | 补齐字段 |
|---|---|---|
| `WsfDisWeaponEvents::ProcessFirePDU` | UtCallbackListN&lt;void(DisFire&)&gt;（公开成员，WsfDisWeaponEvents.hpp:78） | Warhead/Fuse |
| `WsfDisWeaponEvents::ProcessDetonationPDU` | UtCallbackListN&lt;void(DisDetonation&)&gt;（:80） | Warhead/Fuse |
| `WsfDisEmission::OverrideTypeAndFunction` | 静态 UtCallbackListN&lt;void(WsfPlatformPart\*, DisSystem\*, DisEnum16& type, DisEnum8& func)&gt;（WsfDisEmission.cpp:44，查表后调用） | Emitter Name/Function |

- **Signal/Transmitter 无发包前钩子**（MessageTransmitted 内部构造 PDU 无观察者）→ B2 不补，缺口记录；Signal 载荷补齐归 B3 私有 PDU 域
- 发现链：`WsfDisInterface::Find(sim)` → `WsfMilDisInterface::Find(*disPtr)`（静态，WsfMilDisInterface.hpp:44）→ `GetWeaponEvents()`（:53）；WsfMilDisInterface 由 WsfMil 应用扩展挂到 WsfDisExtension（WsfMil.cpp:1033）——应用加载期构造，插件 SimulationExtension::AddedToSimulation/Initialize 可达

#### 6.4.3 实施（扩展 ysim_dis_gateway，新文件 source/YsimDisFieldPatch.cpp）

- **表1 弹药七段码→(Warhead, Fuse)**：键=(entityKind, domain, country, category, subcategory)（匹配 DisFire/DisDetonation 的 GetWeaponType() 七段码），值按 SISO REF-011 Munition Warhead/Fuse 枚举，国产弹药条目（country=36）约 6 条：空射导弹(2,2,36,1,\*)→warhead=1000(HE)/fuse=2(impact)；地空导弹(2,3,36,1,\*)→warhead=1000/fuse=1(proximity)；自拟段中文注释
- **表2 平台类型名→(EmitterName, EmitterFunction)**：RADAR/EW_RADAR/AEW_C2 等 4-6 条（SISO REF-011 Emitter Name/Function 语义合理值，自拟段中文注释）
- **回调语义：补缺不覆盖**——仅当字段现值==0 时查表写入（想定 mil_ext_interface 显式 warhead / dis_interface 显式 emitter_type 命令优先）
- **连接时序**：SimulationExtension::AddedToSimulation（B1 同款最早阶段）+ Initialize 幂等兜底；发现链任一 null → 告警闲置不阻断（降级锚点）
- **线程**：回调全在仿真线程（观察者链内），无锁；UtCallbackIdN 成员持有生命周期
- **CMakeLists**：链 `wsf_mil`（wsf_simdis 范式）+ include `src/core/wsf_mil/source`
- 零侵入 WSF 核心；新代码中文注释；含中文文件 UTF-8 单层 BOM

#### 6.4.4 验证（temp/dis_b2_scenario.txt + temp/dis_capture.py + temp/dis_b2_verify.py）

- **想定**：复刻 warfare 交战结构单进程版（SA-10C_LAUNCHER red 36:30n/115w + F-18E blue 30000ft 航线穿越，WSF_TASK_PROCESSOR 20km 自动交战，end_time 60s 内 Fire/Detonation 必发）+ B1 组网块（multicast 224.2.25.54 127.0.0.1:3225 autostart）+ `entity_type SA-10C 2:3:36:1:2:0:0`（country=36 对齐插件表，想定显式优先路径）
- **捕获脚本**：dis_listen.py 增强版——解析 PDU type 2(Fire)/3(Detonation)/23(Emission)/26(Signal) 关键字段（DIS 大端 struct 直解）
- **断言（两轮 + 降级回归）**：
  1. mission 退出码 0×2 轮
  2. Fire PDU ≥1 且 Warhead≠0、Fuse≠0（表1 补齐生效）
  3. Fire WeaponType 七段码 country=36（显式映射链路）
  4. Detonation PDU ≥1 且 Warhead≠0、DetonationResult∈{1,2,5,6}
  5. Emission PDU ≥1 且 Emitter Name≠0、Function≠0（表2 补齐生效）
  6. Signal PDU ≥1（现状记录：空载荷——缺口底账证据落盘）
  7. 降级回归：无 dis_interface 块 → 插件闲置告警 + 无 FATAL
  8. B1 回归：七段码映射注入条数日志不变（两批次共存无损）

#### 6.4.5 验证结论（2026-10-19，两轮 8 断言 + 降级回归 + B1 回归，ALL PASS）

**实现落地**：
- 模块 `YsimDisFieldPatch.cpp`（ysim_dis_gateway DLL 内，独立扩展名 ysim_dis_field_patch，同 DLL 双扩展驻留——WsfPluginSetup 统一注册）：表1 弹药七段码前五段→(Warhead,Fuse) 6 条（Warhead 统一 REF-011 1000=HE，Fuse 1=近炸/2=触发，域按发射平台域）+ 表2 传感器部件类型名→(EmitterName,Function) 5 条（Name 自拟段 36xxx，含 WSF_RADAR_SENSOR 库类型键）+ 三钩子回调补缺不覆盖 + 发现链（Find→MilDis Find→GetWeaponEvents）任一缺失告警闲置（Emission 钩子独立挂载）+ unique_ptr 句柄自动断链
- CMakeLists 链 wsf_mil（wsf_simdis 范式）；YsimDisGateway.cpp 主入口追加 Register_ysim_dis_field_patch
- 捕获脚本 `temp/dis_capture.py`（Fire/Detonation/Emission/Signal 字段抽取，偏移经抓包实测校准）+ 想定 `temp/dis_b2_scenario.txt`（warfare types.txt 武器链 include + 派生类型挂 comm + 周期 SendMessage 触发 Signal）+ 组网块 `temp/dis_b2_iface.txt`（显式 entity_type SA-10C 2:1:36:1:2:0:0）

| 断言 | 结果 |
|---|---|
| mission/捕获客户端退出码 0×2 轮 | PASS |
| Fire PDU 2 条且 Warhead=1000、Fuse=1（表1 补齐生效） | PASS |
| Fire WeaponType 七段码 2:1:36:1:2（想定显式映射链路，country=36） | PASS |
| Detonation PDU 2 条且 Warhead=1000、Result 1/5（合法域） | PASS |
| Emission PDU 14 条且 Name=36001、Function=5 全非零（表2 命中） | PASS |
| Signal PDU 3 条全空载荷（缺口底账证据落盘，移交 B3） | PASS |
| B1 回归：B1 注入日志 + B2 钩子挂载日志并存（双批次共存无损） | PASS |
| 降级回归：无 dis_interface 块 → B1/B2 双模块闲置告警 + 退出码 0 + 无 FATAL | PASS |

**关键机制结论**：
1. **发包前钩子是字段补齐正路**：WsfDisWeaponEvents::ProcessFirePDU/ProcessDetonationPDU（公开 UtCallbackListN，PutPdu 前调用可写）+ WsfDisEmission::OverrideTypeAndFunction（静态回调，引用参数改写落 PDU）——改原生包不补发，零重复
2. **Fire PDU fireMissionIndex 是 DisUint32（4B）**：总长 96B，weaponType@64/warhead@72/fuse@74——非直觉 2B，按 2B 推导偏移全错 2B
3. **Emission 系统记录偏移以抓包实测为准**（name@32/function@34/number@35，源码 Put 链推导差 1B）——PDU 解析偏移须冒烟校准；错位指纹：读值 [0xA1,0x05] 恰为表值 36001 低字节+5 相邻排列
4. **Signal/Transmitter 无发包前钩子**（MessageTransmitted 内部构造 PDU 无观察者）——空载荷缺口只能记录，Signal 载荷补齐归 B3
5. **表2 键是传感器部件类型名**（WsfPlatformPart::GetTypeId，非平台类型名，无继承链）——B1 表（平台名+继承链）与 B2 表（部件名+精确）是两个键域
6. **Signal PDU 最小触发构造**：WSF_COMM_TRANSCEIVER + WSF_COMM_MEDIUM_GUIDED + execute at_time SendMessage 脚本（guided medium 与距离无关）
7. **钩子连接时机 Initialize 足矣**（运行期事件 vs B1 数据注入须赶平台创建前）；同 DLL 双扩展经主入口统一注册（导出符号唯一约束）

#### 6.4.6 归档
spec §6.4 验证结论、tasks.md 勾选、修改日志第十章、project_memory、README。

### 6.5 批次 B3-B4 规格（占位，B2 收官后细化）

- **B3**：私有 PDU 族（SISO 实验段编号 250+），DisPdu 子类 + SetPduCreateFunction 注册，双端（mission 插件 + Python 客户端）编解码对齐；Signal PDU 载荷补齐（§6.4.1 缺口移交）
- **B4**：DIS 外部实体 → 本地 WsfDisPlatform → XIO DisPlatformInfoPkt/自定义 Packet 透传；XIO 上行指令 → DIS PDU 发射。复用阶段 A 指令队列线程模型
