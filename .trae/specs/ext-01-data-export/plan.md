# 数据输出实施计划（ext-01-data-export）

> 依赖 spec.md 的决策明细；按"骨架 → 通道 → 规模化"推进。
> 2026-10-19：阶段 A 规格已按源码侦察定稿（spec.md §7），转入实施。
> 2026-10-19：**阶段 A 已完成并验证通过**（闭环冒烟 333 行 JSON 全有效 + --ysim-port 命令行 + 无客户端场景）。
> 2026-10-19：**阶段 B 已完成并验证通过**（spec §8：适配器抽象 + TCP/WebSocket/UDP 三通道 + 快照补推，三客户端并发冒烟全 PASS）。

## 阶段 A：Observer 插件骨架（第一里程碑）✅ 2026-10-19
- [x] 新建 `src/wsf_plugins/ysim_data_export/`（wsf_module + wsf_cmake_extension.cmake + CMakeLists + source/YsimDataExport.cpp）
- [x] 扩展链：ApplicationExtension（命令行 --ysim-port）→ ScenarioExtension（ysim_data_export 配置块）→ SimulationExtension（Initialize/Start/Complete）
- [x] 订阅 AdvanceTime/MoverUpdated/PlatformAdded/PlatformDeleted，JSON 行序列化（sim_start/platform_added/platform_removed/state/heartbeat/sim_complete）
- [x] 独立 TCP 发送线程 + 队列（回调零阻塞，死区 + 降采样）
- [x] 编译目标 ysim_data_export → DLL 落 mission_plugins；mission.exe 无头冒烟 + temp/ysim_tcp_client.py 收流验证

## 阶段 B：统一消息中间件 ✅ 2026-10-19（遗留项见 spec §8.8）
- [x] 统一消息结构（五类型映射表，线格式向后兼容）
- [x] 适配器抽象接口（Channel 基类）
- [x] TCP 适配器（重构 + 快照补推）
- [x] WebSocket 适配器（最小 RFC 6455 服务端，零第三方依赖）
- [x] UDP 组播适配器（组播/单播自适应）
- [x] 增量推送：死区阈值 + 降采样 + 帧率保护（阶段 A 沿用）
- [x] 多客户端并发与快照补推（新客户端接入即收全量态势）
- [ ] 掉线外推（与 ext-02 时间语义一并定死）

## 阶段 C：结构化落盘 ✅ 2026-10-19
- [x] Parquet/CSV 时序写入（选型 CSV：无 Arrow/Parquet 依赖；三文件 + UTF-8 BOM + RFC 4180 转义）
- [x] SQLite 快照（静态链入 sqlite-3.32.3；platforms 元信息+最终状态，断点恢复数据源）
- [x] 评估指标表（平台级聚合 5 指标 + 运行级汇总；命中率/发现时间/杀伤链转 Track/Engagement 批次）

## 阶段 D：验收 ✅ 2026-10-19（ext-01 收官）
- [x] 官方示例场景全流程跑通（40s -rt 长跑：全生命周期流 + 落盘一致 + 指标齐全）
- [x] 帧率下降 < 5%（-rt 实时口径 +0.41% PASS，含口径归因与 Complete 尾延迟修复）
- [x] 适配器热切换（ControlChannel 控制通道 + 读写锁并发模型，13/13 断言 PASS）
- [x] 记账归档（spec/plan/tasks/README/修改日志/project_memory 六件套）
