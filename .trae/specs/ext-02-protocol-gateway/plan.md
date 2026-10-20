# 外部接入实施计划（ext-02-protocol-gateway）

> 依赖 spec.md §2；按"XIO 起步 → DIS 增强 → 实装/HIL → 服务化"推进。

## 阶段 A：XIO 自定义 Packet（基础，低风险）✅ 2026-10-19 完成
- [x] 侦察 WsfXIO_Interface 扩展机制与 Packet 注册路径
- [x] 定义私有 Packet ID 与序列化操作符
- [x] 自研最小客户端端到端打通（25 断言全 PASS，spec §4.3）
- [x] 指令注入闭环（阶段 A 延伸）：XIO 线程→仿真线程指令队列 + AckPkt(353) 回执 + pause/resume/get_status/ping 执行（RT 模式 35 断言全 PASS，spec §4.4）
- [x] 平台级指令集（阶段 A 延伸）：set_speed/set_route/add_platform/delete_platform（态势注入+运动控制，RT 模式 47 断言全 PASS，spec §4.5）

## 阶段 B：DIS 增强（复用自带 dis_interface，规格见 spec §6）
- [x] 批次 B1：七段码映射表（国产装备枚举）——新插件 ysim_dis_gateway 运行期注入 GetObjectToEntityTypeMap（20 条 country=36 映射 + 想定显式优先语义，两轮 22 断言+降级回归 ALL PASS，spec §6.3，2026-10-19）
- [ ] 批次 B2：电磁发射/Fire/Detonation/Signal PDU 字段覆盖补齐（先抓包比对定缺口）
- [ ] 批次 B3：自定义 PDU 编解码（WsfDisPduFactory::SetPduCreateFunction）
- [ ] 批次 B4：DIS↔XIO 双向中继

## 阶段 C：HLA 联邦对接
- [ ] RTI 基础设施评估与 FOM 定制
- [ ] 时间管理（保守/乐观）对接
- [ ] HLA↔DIS 转换网关

## 阶段 D：数实联动 / HIL
- [ ] MAVLink / 1553B / ARINC429 解析器
- [ ] 映射到 Mover 更新 / External Control
- [ ] RFM 反射内存（若选型）
- [ ] 时间同步与掉线外推验证

## 阶段 E：推演引擎后端
- [ ] 无头服务化封装
- [ ] gRPC/REST 契约（下发指令/回收态势/裁决）
- [ ] 与 ext-01 中间件复用 Command/Log 通道

## 验收
- [ ] 各协议互通性测试（外部真实工具/模拟器）
- [ ] 时间/坐标契约实现并验证
- [ ] 记账三件套归档
