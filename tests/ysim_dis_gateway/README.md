# ysim_dis_gateway 测试（ext-02 阶段 B：DIS 网关）

> 插件：`build\Release\mission_plugins\ysim_dis_gateway.dll`（同 DLL 双模块：B1 七段码映射注入 20 条 + B2 PDU 字段补齐钩子）。批次详情见 `.trae\specs\ext-02-protocol-gateway\spec.md` §6。

## 脚本清单

| 脚本 | 用途 | 运行方式 |
|---|---|---|
| dis_listen.py | DIS 多播监听客户端：join 224.2.25.54:3225 收 PDU，聚合各平台 EntityState 七段码输出 RESULT（B1 断言载体） | 被编排脚本拉起，也可独立运行（IFACE 常量须改本机出接口 IP） |
| dis_capture.py | DIS 捕获客户端（增强版）：解析 Fire/Detonation/Emission/Signal 关键字段（Warhead/Fuse/EmitterName 等偏移取值）输出 RESULT（B2 断言载体） | 被 dis_b2_verify.py 拉起 |
| dis_b1_verify.py | B1 端到端：起监听→起 mission（air_movers+dis 块 RT 20s）→ 断言想定显式 entity_type 优先（BLUE country=225）+ 插件注入生效（RED country=36）；`--regression` 无块降级回归 | `python dis_b1_verify.py --regression` |
| dis_b2_verify.py | B2 端到端：SA-10C 交战想定 RT 60s，断言 Fire/Detonation Warhead+Fuse 补齐、Emission EmitterName+Function 补齐、Signal 空载荷现状记录 + B1 回归共存 | `python dis_b2_verify.py` |

## scenarios/ 想定与组网块

| 文件 | 用途 |
|---|---|
| dis_iface_block.txt | B1 组网块：multicast 224.2.25.54 <本机出接口IP> 3225 + exercise/site/application |
| dis_b1_scenario.txt | B1 想定：air_movers 2v2 + 显式 entity_type 对照 |
| dis_b1_nodevice.txt | B1 降级回归：无 dis_interface 块（插件应闲置告警） |
| dis_b2_iface.txt | B2 组网块（对齐 B1 实测参数 + SA-10C 显式七段码 2:1:36:1:2） |
| dis_b2_scenario.txt | B2 交战想定：include 源码树 warfare/types.txt SA-10C 武器链 + F-18 航线 + 20km 自动交战（CWD 须为源码树 dis\warfare 目录） |

## 对接要点

- 多播第二参数 = 出接口网卡 IP（客户端 join 须同接口）；`autostart` 为无外部网关自跑开关
- DIS PDU 大端定长，Python struct 直解；EntityState EntityType 偏移 20（勿按 18 解析国家码）
- 想定 `dis_interface` 块内显式 `entity_type <类型名> <七段码>` 优先于插件注入表（仅插不存在的键）
