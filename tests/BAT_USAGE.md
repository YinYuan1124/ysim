# tests demo bat 使用手册

> 适用范围：`tests\` 下全部 7 个 demo `.bat`。本文档为**详细使用手册**（运行前提 / 通用机制 / 各 demo 用法 / 排错 FAQ）；速查索引见 [tests/README.md](./README.md)。
>
> **bat 定位（2026-10-21 起）**：tests\ 下 bat 仅保留 **demo 快速启动器**——双击起实时仿真 + 直播展示对应插件的作用，用于快速观摩与联调。纯验证类编排脚本（`*_verify.py` / `verify_*.py`）**不配 bat**，命令行 `python` 直跑，参数与用法见各目录 README 及 [tests/README.md](./README.md)「验证脚本直跑」一节。

## 1. 概述

所有 demo bat 的行为均为**一键起仿真 + 直播展示**：双击自动拉起 mission.exe（含对应想定/组网块），客户端实时展示插件输出（JSON 流 / 多播包 / 浏览器态势页），结束后定向清理，无需手工准备仿真。

| demo bat | 展示的插件作用 |
|---|---|
| start_demo.bat | e2e 王牌演示：浏览器控制 + WebSocket 实时态势页 |
| ysim_tcp_client.bat | ext-01 TCP JSON 行流实时推送 |
| ysim_multi_client.bat | ext-01 TCP/WS/UDP 三通道并发推流 |
| ysim_bench_client.bat | ext-01 推流吞吐能力 |
| dis_listen.bat | ext-02 DIS 多播 EntityState 七段码实时监听 |
| dis_capture.bat | ext-02 DIS Fire/Detonation/Emission/Signal 四类 PDU 实时捕获 |
| xio_demo.bat | ext-02 XIO 三窗联动：10 条指令脚本闭环（发令/执行/回执全程直读） |

## 2. 运行前提

1. **构建树就绪**：`build\Release\mission.exe` 与 `mission_plugins\`（ysim_data_export / ysim_dis_gateway / ysim_xio_gateway 等 DLL 已部署）
2. **Python 3**：标准库零依赖，无需安装第三方包
3. **端口空闲**：31600 / 31601 / 31602（TCP·WS·UDP 三通道）、8080（e2e HTTP）、多播 224.2.25.54:3225（DIS）
4. **同一时间只跑一个 demo**：端口与多播组为全局资源，多开必冲突；上一轮窗口 `pause` 未关时，bat 开跑前会自动清同名残留
5. **DIS 类前提**：出接口 IP 三处一致——组网块 `interface_address`、dis_listen.py `IFACE`、dis_capture.py `LOCAL_IF`，均须为本机实际网卡 IP（当前配置 192.168.5.3）

## 3. 通用机制（所有 demo bat 一致）

- **编码**：注释与提示为纯 ASCII 英文（中文 Windows 的 cmd 按系统 GBK 代码页解码 UTF-8 中文会显示乱码）；`chcp 65001` + `PYTHONUTF8=1` 仅服务于 Python 脚本的 UTF-8 输出与产物编码
- **目录自适应**：`cd /d %~dp0` 切到 bat 所在目录，双击或任意路径调用均可
- **参数透传**：`%*` 原样传给脚本（如 `ysim_tcp_client.bat` 透传 ysim_tcp_client.py 参数）
- **仿真窗口管理**：mission.exe 在独立**最小化窗口**运行，窗口标题固定为 `ysim-bat-*`；开跑前先清同名残留，结束后 `taskkill /T` **按标题定向清理**——只杀自己这一例，不误伤其他仿真
- **产物统一落 temp\**（中间产物红线）：mission 控制台输出 `temp\bat_*_mission.out`，客户端产物见各 demo 说明
- **结束行为**：`pause` 留窗查看结果，按任意键退出

## 4. 各 demo 使用说明

### 4.1 e2e_visual_demo\

| bat | 用法与说明 |
|---|---|
| [start_demo.bat](./e2e_visual_demo/start_demo.bat) | 观摩演示：后台起仿真（起步暂停，浏览器控制推进）+ 打开 WebSocket 实时态势页。参数透传 run_demo.py：`--http-port 端口` / `--no-browser`（不自动开浏览器）/ `--scenario 路径`（换想定）。窗口内 Ctrl+C 退出并结束仿真 |

### 4.2 ysim_data_export\

| bat | 用法与说明 |
|---|---|
| [ysim_tcp_client.bat](./ysim_data_export/ysim_tcp_client.bat) | TCP 收流演示：自动起 ysim_smoke_2v2（RT 20s）→ 连 31600 实时收 JSON 行流至 sim_complete → 清理。先 `set YSIM_PORT=32700` 再双击可整链换端口（mission 经 `--ysim-port` 与客户端同步覆盖）。产物 temp\ysim_client_result.txt |
| [ysim_multi_client.bat](./ysim_data_export/ysim_multi_client.bat) | 三通道并发演示：自动起 ysim_smoke_full（RT 20s）→ TCP 31600 + WS 31601 + UDP 239.16.16.16:31602 同时实时收流 → 窗口打印 tcp/ws/udp 三项 PASS/FAIL → 清理。产物 temp\ysim_multi_result.txt |
| [ysim_bench_client.bat](./ysim_data_export/ysim_bench_client.bat) | 吞吐演示：自动起 ysim_bench_full（RT，end_time 600s，全通道）→ 连 TCP 实时计行 → 客户端到时或仿真断连后清理。用法：`ysim_bench_client.bat [端口] [秒数]`，默认 31600 300；指定端口时 mission 同步覆盖。快速观摩：`ysim_bench_client.bat 31600 60` |

### 4.3 ysim_dis_gateway\

| bat | 用法与说明 |
|---|---|
| [dis_listen.bat](./ysim_dis_gateway/dis_listen.bat) | B1 七段码实时监听：自动起 dis_iface_block + dis_b1_scenario（RT 20s）→ 加入多播 224.2.25.54:3225 → 实时聚合 EntityState 七段码 → 输出 RESULT JSON → 清理。无参；**正式断言验收请直跑 `python dis_b1_verify.py`** |
| [dis_capture.bat](./ysim_dis_gateway/dis_capture.bat) | B2 增强捕获演示：自动起 dis_b2_iface + dis_b2_scenario（RT 60s，CWD 自动切源码树 dis\warfare）→ 实时捕获 Fire/Detonation/Emission/Signal 关键字段 → 输出 RESULT JSON → 清理。无参 |

### 4.4 ysim_xio_gateway\

| bat | 用法与说明 |
|---|---|
| [xio_demo.bat](./ysim_xio_gateway/xio_demo.bat) | XIO 三窗联动演示（纯 bat，无 python）：控制窗分步详细日志——环境检查（client/mission/插件 DLL/组网块/想定 5 项 FOUND）→ 起 XIO 客户端窗（title `ysim-bat-xio-client`，10 条指令脚本+回执实时直出）→ 3s 后起 mission 窗（title `ysim-bat-xio`，RT 45s，pause_for_connection；插件 received/executed 日志直出）→ 5s 一跳监测双进程存活（上限 75s，mission 自退即提前收尾）→ 清理结果可见输出（taskkill SUCCESS + 双 [OK] 复核）。无参；正式断言验收 `python xio_e2e_verify.py` |

## 5. 常见问题排查

**Q1：提示端口被占用**
demo bat 开跑前已自动清理同名残留窗口；仍占用时检查是否有其他 bat 窗口未退出，或手动清理：

```bat
taskkill /F /T /FI "WINDOWTITLE eq ysim-bat-tcp"
```

（按实际标题替换：ysim-bat-tcp / ysim-bat-multi / ysim-bat-bench / ysim-bat-dis1 / ysim-bat-dis2 / ysim-bat-xio / ysim-bat-xio-client）
确认无任何仿真在跑时，也可全局清理：`taskkill /F /IM mission.exe`。

**Q2：记事本打开 start_demo.bat 显示乱码**
该文件为历史文件、UTF-8 编码含中文注释，记事本按 GBK 解码显示乱码，**不影响运行**；其余 6 个 demo bat 已纯 ASCII 化，任何查看器零乱码。

**Q3：DIS 类监听不到数据**
依次确认：① 组网块 `interface_address` 与 dis_listen.py `IFACE` / dis_capture.py `LOCAL_IF` 是否为本机实际网卡 IP；② `temp\bat_dis_b1_mission.out` / `bat_dis_b2_mission.out` 中 mission 是否正常启动；③ 防火墙是否放行 UDP 3225 多播。

**Q4：bench 客户端连接失败**
基准想定数据出口就绪较慢，bat 已内置 3s 等待（bench 客户端无重试逻辑）；仍失败时查看 `temp\bat_bench_mission.out`。快速自检可用 `ysim_bench_client.bat 31600 60`。

**Q5：找不到结果文件**
所有产物统一落 `e:\Project\ysim\temp\`：`bat_*_mission.out`＝bat 拉起的 mission 控制台输出；`ysim_*_result.txt`＝客户端明细；`ysim_record_out\run_*`＝落盘校验对象（verify 脚本直跑时生成）。

**Q6：能否并行跑多个 demo**
不能：31600/31601/31602 与 DIS 多播组为全局资源。需要连续观摩时按顺序执行，等上一个窗口出现 `pause` 并按键退出后再双击下一个；确需并行须先改想定中的端口/多播地址。

**Q7：验证类脚本怎么跑**
demo bat 不承担验证职能。验证脚本在各自目录内 `python` 直跑（多为两轮端到端 + 降级回归，结尾打印 `RESULT: {...}` JSON 行），完整清单见 [tests/README.md](./README.md)「验证脚本直跑」一节。
