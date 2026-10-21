# tests 一键运行 bat 使用手册

> 适用范围：`tests\` 下全部 13 个 `.bat`。本文档为**详细使用手册**（运行前提 / 通用机制 / 各 bat 用法 / 排错 FAQ）；速查索引见 [tests/README.md](./README.md)「bat 一键运行」一节。

## 1. 概述

所有 bat 的定位均为**一键快速验证**：双击自动完成「起 mission.exe（含对应想定/组网块）→ 跑客户端/断言 → 结束清理」，无需手工准备仿真，用于二次开发（ext-01 数据出口 / ext-02 DIS·XIO 网关 / e2e 端到端联调）的日常联调与验收。

按执行编排方式分两类：

| 模式 | bat | 说明 |
|---|---|---|
| bat 编排型 | ysim_tcp_client / ysim_multi_client / ysim_record_verify / ysim_bench_client / dis_listen / dis_capture | bat 自己拉起并按窗口标题清理 mission，Python 只做客户端 |
| Python 编排型 | start_demo / e2e_control_verify / e2e_ws_verify / ysim_hotswap_verify / dis_b1_verify / dis_b2_verify / xio_e2e_verify | 由 Python 脚本自编排（自起 mission / 两轮端到端 + 回归），bat 仅作启动入口 |

## 2. 运行前提

1. **构建树就绪**：`build\Release\mission.exe` 与 `mission_plugins\`（ysim_data_export / ysim_dis_gateway / ysim_xio_gateway 三个 DLL 已部署）
2. **Python 3**：标准库零依赖，无需安装第三方包
3. **端口空闲**：31600 / 31601 / 31602（TCP·WS·UDP 三通道）、18080（e2e 控制桥接）、多播 224.2.25.54:3225（DIS）
4. **同一时间只跑一个 bat**：端口与多播组为全局资源，多开必冲突；上一轮窗口 `pause` 未关时，bat 开跑前会自动清同名残留
5. **DIS 类前提**：出接口 IP 三处一致——组网块 `interface_address`、dis_listen.py `IFACE`、dis_capture.py `LOCAL_IF`，均须为本机实际网卡 IP（当前配置 192.168.5.3）

## 3. 通用机制（所有 bat 一致）

- **编码**：注释与提示为纯 ASCII 英文（中文 Windows 的 cmd 按系统 GBK 代码页解码 UTF-8 中文会显示乱码）；`chcp 65001` + `PYTHONUTF8=1` 仅服务于 Python 脚本的 UTF-8 输出与产物编码
- **目录自适应**：`cd /d %~dp0` 切到 bat 所在目录，双击或任意路径调用均可
- **参数透传**：`%*` 原样传给脚本（如 `dis_b1_verify.bat --regression`）
- **仿真窗口管理**（bat 编排型）：mission.exe 在独立**最小化窗口**运行，窗口标题固定为 `ysim-bat-*`；开跑前先清同名残留，客户端结束后 `taskkill /T` **按标题定向清理**——只杀自己这一例，不误伤其他仿真
- **产物统一落 temp\**（中间产物红线）：mission 控制台输出 `temp\bat_*_mission.out`，客户端/校验产物见各 bat 说明
- **结束行为**：`pause` 留窗查看结果，按任意键退出

## 4. 各 bat 使用说明

### 4.1 e2e_visual_demo\

| bat | 用法与说明 |
|---|---|
| [start_demo.bat](./e2e_visual_demo/start_demo.bat) | 观摩演示：后台起仿真（起步暂停，浏览器控制推进）+ 打开 WebSocket 实时态势页。参数透传 run_demo.py：`--http-port 端口` / `--no-browser`（不自动开浏览器）/ `--scenario 路径`（换想定）。窗口内 Ctrl+C 退出并结束仿真 |
| [e2e_control_verify.bat](./e2e_visual_demo/e2e_control_verify.bat) | 无头控制链路验收：起步冻结 / resume / pause / 4× 加速 / reset 重启 / 树杀清理。无参（桥接器用 18080）。**开跑前全局 taskkill mission.exe**，会杀掉机器上所有 mission，勿与其他仿真并行 |
| [e2e_ws_verify.bat](./e2e_visual_demo/e2e_ws_verify.bat) | 无头 WS 推流验收：握手 / 快照 / 状态流 / 心跳 / 字段断言，全部满足即提前终止 mission。无参 |

### 4.2 ysim_data_export\

| bat | 用法与说明 |
|---|---|
| [ysim_tcp_client.bat](./ysim_data_export/ysim_tcp_client.bat) | TCP 收流最小示例：自动起 ysim_smoke_2v2（RT 20s）→ 连 31600 收 JSON 行流至 sim_complete → 清理。先 `set YSIM_PORT=32700` 再双击可整链换端口（mission 经 `--ysim-port` 与客户端同步覆盖）。产物 temp\ysim_client_result.txt |
| [ysim_multi_client.bat](./ysim_data_export/ysim_multi_client.bat) | 三通道并发断言：自动起 ysim_smoke_full（RT 20s）→ TCP 31600 + WS 31601 + UDP 239.16.16.16:31602 同时收流 → 窗口打印 tcp/ws/udp 三项 PASS/FAIL → 清理。产物 temp\ysim_multi_result.txt |
| [ysim_hotswap_verify.bat](./ysim_data_export/ysim_hotswap_verify.bat) | 适配器热切换验收（Python 编排，自起 mission 约 40s）：list / add_udp / remove / add_tcp 时序断言。无参；产物 temp\ysim_hotswap_result.txt |
| [ysim_record_verify.bat](./ysim_data_export/ysim_record_verify.bat) | 落盘一键校验：同步跑 ysim_smoke_record（无 -rt 尽快完成）→ 校验 temp\ysim_record_out\ 最新 run_*（CSV 表头 / 事件全生命周期 / SQLite 四表 / 指标一致性）。无参 |
| [ysim_bench_client.bat](./ysim_data_export/ysim_bench_client.bat) | 吞吐基准：自动起 ysim_bench_full（RT，end_time 600s，全通道）→ 连 TCP 计行 → 客户端到时或仿真断连后清理。用法：`ysim_bench_client.bat [端口] [秒数]`，默认 31600 300；指定端口时 mission 同步覆盖。快速验证：`ysim_bench_client.bat 31600 60` |

### 4.3 ysim_dis_gateway\

| bat | 用法与说明 |
|---|---|
| [dis_listen.bat](./ysim_dis_gateway/dis_listen.bat) | B1 七段码监听：自动起 dis_iface_block + dis_b1_scenario（RT 20s）→ 加入多播 224.2.25.54:3225 → 聚合 EntityState 七段码 → 输出 RESULT JSON → 清理。无参；**正式断言验收请用 dis_b1_verify.bat** |
| [dis_capture.bat](./ysim_dis_gateway/dis_capture.bat) | B2 增强捕获：自动起 dis_b2_iface + dis_b2_scenario（RT 60s，CWD 自动切源码树 dis\warfare）→ 捕获 Fire/Detonation/Emission/Signal 关键字段 → 输出 RESULT JSON → 清理。无参 |
| [dis_b1_verify.bat](./ysim_dis_gateway/dis_b1_verify.bat) | B1 端到端验收（Python 编排，两轮 RT 20s）；加 `--regression` 追加降级回归 |
| [dis_b2_verify.bat](./ysim_dis_gateway/dis_b2_verify.bat) | B2 端到端验收（Python 编排，两轮 RT 60s）；加 `--regression` 追加降级回归 |

### 4.4 ysim_xio_gateway\

| bat | 用法与说明 |
|---|---|
| [xio_e2e_verify.bat](./ysim_xio_gateway/xio_e2e_verify.bat) | XIO 平台级指令集端到端（Python 编排，两轮 RT 45s + 回归）；加 `--regression`＝**仅跑降级回归**（跳过两轮端到端） |

### 4.5 equipmentdb\

| bat | 用法与说明 |
|---|---|
| [run_equipmentdb.bat](./equipmentdb/run_equipmentdb.bat) | ext-05 装备数据库端到端：自动跑 test_equipmentdb.txt（脚本侧 EquipmentDB 13 方法全量查询）→ verify_equipmentdb.py 与 SQLite 直查 DB3K_modern.db3 交叉断言 37 项（各类计数 / F-16 检索 / 汇总与明细整行 / 中文名回读 / 枚举翻译 / 列名 / 关联表导航含 max_rows 截断 / 组合过滤国别·类别·年代·名称·型别 / 四类负面用例不崩溃）→ 清理。无参；窗口标题 `ysim-bat-equipmentdb`；结果文件 `eqdb_result.txt` 生成于 bat 所在目录，PASS 自动删除、FAIL 保留。前置：`ysim_equipment_db.dll` 已部署 mission_plugins\（编译目标 ysim_equipment_db） |

## 5. 常见问题排查

**Q1：提示端口被占用**
bat 编排型开跑前已自动清理同名残留窗口；仍占用时检查是否有其他 bat 窗口未退出，或手动清理：

```bat
taskkill /F /T /FI "WINDOWTITLE eq ysim-bat-tcp"
```

（按实际标题替换：ysim-bat-tcp / ysim-bat-multi / ysim-bat-bench / ysim-bat-dis1 / ysim-bat-dis2）
确认无任何仿真在跑时，也可全局清理：`taskkill /F /IM mission.exe`。

**Q2：记事本打开 start_demo.bat 显示乱码**
该文件为历史文件、UTF-8 编码含中文注释，记事本按 GBK 解码显示乱码，**不影响运行**；其余 12 个 bat 已纯 ASCII 化，任何查看器零乱码。

**Q3：DIS 类监听不到数据**
依次确认：① 组网块 `interface_address` 与 dis_listen.py `IFACE` / dis_capture.py `LOCAL_IF` 是否为本机实际网卡 IP；② `temp\bat_dis_b1_mission.out` / `bat_dis_b2_mission.out` 中 mission 是否正常启动；③ 防火墙是否放行 UDP 3225 多播。

**Q4：bench 客户端连接失败**
基准想定数据出口就绪较慢，bat 已内置 3s 等待（bench 客户端无重试逻辑）；仍失败时查看 `temp\bat_bench_mission.out`。快速自检可用 `ysim_bench_client.bat 31600 60`。

**Q5：找不到结果文件**
所有产物统一落 `e:\afsim\ysim\temp\`：`bat_*_mission.out`＝bat 拉起的 mission 控制台输出；`ysim_*_result.txt`＝客户端/校验断言明细；`ysim_record_out\run_*`＝落盘校验对象。

**Q6：能否并行跑多个测试**
不能：31600/31601/31602 与 DIS 多播组为全局资源。需要批量验证时按顺序执行，等上一个窗口出现 `pause` 并按键退出后再双击下一个；确需并行须先改想定中的端口/多播地址。
