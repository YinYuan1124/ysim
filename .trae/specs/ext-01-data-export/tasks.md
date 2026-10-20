# 数据输出任务清单（ext-01-data-export）

> 依赖 spec.md §3/§4/§5/§7/§8；**阶段 A/B 已完成（2026-10-19），阶段 C/D 待启动**。

## 任务 1：Observer 插件骨架（ysim_data_export）✅ 2026-10-19
- [x] 工程骨架：wsf_module + wsf_cmake_extension.cmake + CMakeLists.txt（swdev_plugin_install → mission_plugins）+ WsfPluginVersion/WsfPluginSetup 导出
- [x] ApplicationExtension：注册扩展 + 命令行 `--ysim-port <n>`（选项必须置于文件名之前，框架约定）
- [x] ScenarioExtension：`ysim_data_export { port/heartbeat_interval/deadband/min_update_interval } end_ysim_data_export` 配置块
- [x] SimulationExtension：Initialize 启 TCP 线程、Start 发 sim_start、Complete 发 sim_complete 并收线程
- [x] 事件订阅：AdvanceTime（心跳+帧驱动）/ MoverUpdated（state，死区+降采样）/ PlatformAdded / PlatformDeleted
- [x] JSON 行序列化（UTF-8，\n 分隔；弧度→度换算在 IO 边界）
- [x] 发送线程 + 队列（回调零阻塞，队列上限 10000 丢最旧，select 可写检查，断线剔除）
- [x] 源文件 UTF-8 单层 BOM + 中文注释 + 二次开发前缀

## 任务 2：验证（阶段 A 冒烟）✅ 2026-10-19
- [x] cmake re-configure → BUILD_WITH_ysim_data_export 启用 → 编译目标通过
- [x] DLL 落 build/Release/mission_plugins/，mission.exe 启动日志确认插件加载（Plugins Loaded 列表第 7 位）
- [x] demo 想定（官方 2v2 空战 + 配置块）无头运行，temp/ysim_tcp_client.py 收流：333 行全有效（state 324 / heartbeat 4 / platform_removed 4 / sim_complete 1，JSON 解析零失败）
- [x] 无客户端连接时仿真照常运行不崩溃（退出码 0，帧利用率 3.8% 正常）

## 任务 3：统一消息中间件（阶段 B）✅ 2026-10-19（遗留项见 §8.8）
- [x] 统一消息结构定义（五类型映射表，线格式向后兼容；spec §8.5）
- [x] 适配器抽象接口（Channel 基类：入队/发送线程/广播/快照骨架）
- [x] TCP 适配器（阶段 A 重构 + 快照补推；tcp_port/port 关键字）
- [x] WebSocket 适配器（最小 RFC 6455 服务端：SHA-1/Base64 内置实现 + 握手 + 文本帧 + ping/pong/close）
- [x] UDP 组播适配器（组播/单播自适应，TTL/环回可配，每消息一数据报）
- [x] 增量推送（死区 + 降采样 + 帧率保护，阶段 A 已落地沿用）
- [x] 多客户端并发与快照补推（新 TCP/WS 客户端接入即收全量态势；temp/ysim_multi_client.py 三通道并发验证 PASS）
- [x] 命令行扩展：--ysim-ws-port / --ysim-udp-port（与 --ysim-port 同语义）
- [ ] 掉线外推（待 ext-02 外部时钟对接时定死时间语义）
- [ ] Track/Engagement/Command 数据源订阅（新增 Observer 批次）

## 任务 4：结构化落盘（阶段 C）✅ 2026-10-19
- [x] Parquet/CSV 时序落库（选型 CSV：依赖树无 Arrow/Parquet；UTF-8 with BOM 三文件 platform_states/events/metrics）
- [x] SQLite 快照（静态链入 3rd_party sqlite-3.32.3，无 DLL 部署；platforms 元信息+最终状态四表库）
- [x] 评估指标表生成（平台级 5 指标×N + 运行级汇总；交战类指标转 Track/Engagement 批次）
- [x] 配置/命令行：record/record_directory/record_csv/record_sqlite + --ysim-record <dir>（默认关闭显式启用）
- [x] 验证：纯落盘 + 三通道并发落盘（states=328 与推流完全一致，同源同节流）+ 默认关闭回归 全 PASS（spec §9.7）

## 任务 5：验收（阶段 D）✅ 2026-10-19（ext-01 收官）
- [x] 适配器热切换：ControlChannel 控制通道（control_port/--ysim-control-port，add_tcp/add_ws/add_udp/remove/list 命令，读写锁并发模型，锁外停机）
- [x] 官方示例场景全流程数据可见：40s -rt 长跑全生命周期流 + 落盘 648=648 一致 + 指标 25 条
- [x] 热切换验证：13/13 断言 PASS（add/remove/list 全链路、TCP 热通道快照补推、remove 后停流、主通道全程不断流）
- [x] 帧率下降 < 5%：-rt 实时口径 +0.41% PASS（含口径归因：全速模式 +11%/600s 为采集成本非帧率损失；Complete 串行尾延迟修复 24%→11%）
- [x] 记账归档六件套（spec/plan/tasks/README/修改日志/project_memory）

## 验收标准
- 外部客户端实时看到平台/航迹/交战流；仿真正确性不受影响；插件可卸载
- 时间与坐标系契约（roadmap §4）文档化并实现
