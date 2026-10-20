# 外部接入任务清单（ext-02-protocol-gateway）

> 依赖 spec.md §2；阶段 A 已完成（2026-10-19），其余待实施。

## 任务 1：XIO 自定义 Packet ✅ 2026-10-19 完成
- [x] 侦察 WsfXIO_Interface 扩展机制
- [x] 私有 Packet ID + 序列化操作符（350-352，app2 段）
- [x] 最小客户端端到端（25 断言全 PASS：事件/状态下行 + 命令上行登记 + 降级回归）
- [x] 指令注入闭环：AckPkt(353) + XIO 线程→仿真线程队列 + pause/resume/get_status/ping 执行（RT 模式 35 断言全 PASS，spec §4.4）
- [x] 平台级指令集：set_speed/set_route/add_platform/delete_platform（动态增删 Clone/AddPlatform/DeletePlatform + 航路弧度转换/双航点防瞬移，RT 模式 47 断言全 PASS，spec §4.5）

## 任务 2：DIS 增强（阶段 B，规格 spec §6；侦察结论 temp/recon_dis.md）
- [x] B1：七段码映射表插件 ysim_dis_gateway（运行期注入 + Python DIS 监听客户端断言 + 降级回归，两轮 22 断言 ALL PASS，spec §6.3，2026-10-19）
- [x] B2：PDU 字段覆盖补齐（规格 spec §6.4，两轮 8 断言 + 降级回归 + B1 回归 ALL PASS，2026-10-19）
  - [x] 侦察缺口底账落盘（Fire/Detonation Warhead+Fuse 恒0；Emission EmitterName/Function 恒0；Signal 空载荷；Transmitter RadioEntityType/ModulationType 缺）——spec §6.4.1
  - [x] 扩展插件：YsimDisFieldPatch.cpp（表1 七段码→Warhead/Fuse + 表2 类型名→EmitterName/Function，钩子 ProcessFirePDU/ProcessDetonationPDU/OverrideTypeAndFunction，补缺不覆盖）
  - [x] 想定 temp/dis_b2_scenario.txt（SA-10C 单进程交战复刻）+ 捕获 temp/dis_capture.py
  - [x] 验证两轮 8 断言 + 降级回归 + B1 回归（spec §6.4.5，ALL PASS）
  - [x] 归档六件套（spec 验证结论/tasks/修改日志第十章/project_memory/README）
- [ ] B3：自定义 PDU 编解码
- [ ] B4：DIS↔XIO 双向中继

## 任务 3：HLA
- [ ] RTI 评估 + FOM 定制
- [ ] 时间管理对接（保守/乐观）
- [ ] HLA↔DIS 转换网关

## 任务 4：数实联动 / HIL
- [ ] MAVLink/1553B/ARINC429 解析器
- [ ] Mover 更新 / External Control 映射
- [ ] RFM 反射内存（选型后）
- [ ] 时间同步 + 掉线外推

## 任务 5：推演引擎后端
- [ ] 无头服务化封装
- [ ] gRPC/REST 契约（指令下发/态势回收/裁决返回）
- [ ] 与 ext-01 Command/Log 通道复用

## 验收标准
- DIS 与外部工具互通；XIO Packet 端到端；HIL 位置误差在容忍域；gRPC 契约测试通过
- 时间同步策略文档化并实现；坐标系/时标统一（roadmap §4）
