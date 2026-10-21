# tests 测试资产总览

> 二次开发新增功能的测试脚本与配套想定归档区。按功能一文件夹一插件，编排脚本在本层、想定/组网块在 `scenarios/` 子层；运行时中间输出（*.out / RESULT 文本）仍写 `temp\`（中间产物红线）。
>
> **bat 定位（2026-10-21 起）**：tests\ 下 bat 仅保留** demo 快速启动器**——双击起实时仿真 + 直播展示对应插件的作用。纯验证类编排脚本（*_verify.py 等）不配 bat，由 `python` 直跑（见各目录 README）。

## 目录结构

| 文件夹 | 对应插件（DLL） | 功能 | 入口 |
|---|---|---|---|
| [ysim_data_export/](./ysim_data_export/README.md) | ysim_data_export.dll | 数据出口：CSV/SQLite 落盘 + TCP/WS/UDP 推流 + 适配器热切换 | demo：`ysim_tcp_client.bat` 等；验证：`python ysim_hotswap_verify.py` 等 |
| [ysim_dis_gateway/](./ysim_dis_gateway/README.md) | ysim_dis_gateway.dll | DIS 网关：七段码映射注入（B1）+ PDU 字段补齐（B2） | demo：`dis_listen.bat` / `dis_capture.bat`；验证：`python dis_b1_verify.py --regression` 等 |
| [ysim_xio_gateway/](./ysim_xio_gateway/README.md) | ysim_xio_gateway.dll | XIO 网关：Packet 350-353 事件/状态下行 + 指令上行注入 | demo：`xio_demo.bat`（三窗联动）；验证：`python xio_e2e_verify.py` |
| [e2e_visual_demo/](./e2e_visual_demo/README.md) | ysim_data_export.dll（WS 通道） | 端到端联调示例：mission 后台 + 浏览器 WebSocket 实时态势页 | demo：`start_demo.bat`；验证：`python e2e_ws_verify.py` 等 |
| [equipmentdb/](./equipmentdb/README.md) | ysim_equipment_db.dll | 装备数据库：EquipmentDB 脚本类只读查询 DB3K_modern.db3（计数/检索/整行/枚举） | 验证：`python verify_equipmentdb.py`（无 demo bat） |

## demo bat 速查（共 7 个）

> 详细使用手册（运行前提 / 通用机制 / 各 demo 用法详解 / 排错 FAQ）：[BAT_USAGE.md](./BAT_USAGE.md)。

| bat | 一键流程（起仿真 + 直播展示插件作用） | 参数 / 产物 |
|---|---|---|
| [start_demo.bat](./e2e_visual_demo/start_demo.bat) | 王牌演示：自动起仿真（起步暂停）+ 浏览器控制态势页 | 透传 run_demo.py：`--http-port` / `--no-browser` / `--scenario 路径`；窗口内 Ctrl+C 退出并结束仿真 |
| [ysim_tcp_client.bat](./ysim_data_export/ysim_tcp_client.bat) | TCP 收流演示：自动起 smoke_2v2（RT 20s）→ 连 31600 实时收 JSON 流至 sim_complete → 清理 | `set YSIM_PORT=` 可同时覆盖 mission 与客户端端口；产物 temp\ysim_client_result.txt |
| [ysim_multi_client.bat](./ysim_data_export/ysim_multi_client.bat) | 三通道并发演示：自动起 smoke_full（RT 20s）→ TCP+WS+UDP 同时实时收流 → 清理 | 产物 temp\ysim_multi_result.txt |
| [ysim_bench_client.bat](./ysim_data_export/ysim_bench_client.bat) | 吞吐演示：自动起 bench_full（RT 600s 全通道）→ 连 31600 实时计行 → 到时/断连后清理 | 参数：`端口 秒数`（默认 31600 300；指定端口时 mission 同步覆盖） |
| [dis_listen.bat](./ysim_dis_gateway/dis_listen.bat) | DIS 实时监听演示：自动起 B1 组网+想定（RT 20s）→ 加入多播实时聚合 EntityState 七段码 | join 224.2.25.54:3225；出接口 IP 变化须同步改组网块与 .py `IFACE` |
| [dis_capture.bat](./ysim_dis_gateway/dis_capture.bat) | DIS 增强捕获演示：自动起 B2 组网+交战想定（RT 60s，CWD 自动切 dis\warfare）→ 实时捕获四类 PDU 字段 | 同上（.py `LOCAL_IF`） |
| [xio_demo.bat](./ysim_xio_gateway/xio_demo.bat) | XIO 三窗联动演示：控制窗分步日志（环境检查/启动/5s 进程监测/清理复核）+ 客户端窗（10 条指令脚本+回执直出）+ mission 窗（插件 received/executed 日志直出） | 无参；mission 自退即提前收尾（上限 75s）；纯 bat 无 python |

- **统一样式**：bat 注释/提示纯 ASCII 英文（防 GBK 乱码）；`chcp 65001` + `PYTHONUTF8=1` 供 Python 脚本 UTF-8 输出；`cd /d %~dp0` 双击/任意路径可用；结尾 `pause` 留窗查看
- **定向清理**：bat 拉起的 mission 在独立最小化窗口（标题 `ysim-bat-*`），结束只清理自己这一例，开跑前也先清同名残留
- **勿同时双击多个 demo bat**：316xx 端口与 224.x / 235.x 组播会互相冲突（smoke 系列与 bench_full 均用 31600/31601/31602）
- **换机运行**：DIS 组网块 IP 与 dis_listen / dis_capture 内 `IFACE` / `LOCAL_IF` 常量须同步改本机出接口 IP

## 验证脚本直跑（不配 bat）

按用户定位调整（2026-10-21）：纯验证编排不占 bat 位，命令行直跑 `python <脚本>` 即可，各目录 README 有完整清单与参数说明：

| 脚本 | 用途 | 参数 |
|---|---|---|
| ysim_data_export\ysim_hotswap_verify.py | 适配器热切换验收（自起 mission RT 40s，list/add/remove 时序断言） | 无参 |
| ysim_data_export\ysim_record_verify.py | 结构化落盘校验（跑 smoke_record → 校验 CSV/SQLite 四表/指标） | 无参 |
| ysim_dis_gateway\dis_b1_verify.py | B1 七段码端到端（两轮 RT 20s + 降级回归） | `--regression`＝追加降级回归 |
| ysim_dis_gateway\dis_b2_verify.py | B2 PDU 字段补齐端到端（两轮 RT 60s + 降级回归） | `--regression`＝追加降级回归 |
| ysim_xio_gateway\xio_e2e_verify.py | XIO 平台级指令集端到端（两轮 RT 45s + 回归） | `--regression`＝仅跑降级回归 |
| e2e_visual_demo\e2e_ws_verify.py | 无头 WS 推流验证（握手/快照/状态流/心跳/字段断言） | 无参 |
| e2e_visual_demo\e2e_control_verify.py | 无头控制链路验证（起步冻结/推进/暂停/4×加速/重置/树杀清理） | 无参（**开跑前全局 taskkill mission.exe**，勿与其他仿真并行） |
| equipmentdb\verify_equipmentdb.py | 装备数据库端到端（需先跑 test_equipmentdb.txt 生成 eqdb_result.txt） | `[结果文件路径]` |

## 通用约定

1. **运行环境**：Python 3（标准库零依赖）+ 构建树 `build\Release\`（mission.exe / 插件已部署 mission_plugins\）
2. **RESULT 行**：全部编排脚本结束打印 `RESULT: {...}` JSON 行，供 CI/外部核对（含 PASS/FAIL 计数与明细键）
3. **组网参数**：DIS/XIO 多播块内 IP 按本机网卡实际地址段修改（`scenarios\` 内注释有标注；dis 组固定 224.2.25.54:3225 + 出接口 IP）
4. **回归模式**：`--regression` 参数 = 无对应 interface 块时插件应闲置告警且仿真正常完成（降级设计断言）
5. **两轮断言**：带 `_verify.py` 后缀的编排脚本按批次验证规范执行两轮 + 回归，历史结论见 `.trae\specs\` 对应工作包 spec.md
