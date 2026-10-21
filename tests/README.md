# tests 测试资产总览

> 二次开发新增功能的测试脚本与配套想定归档区。按功能一文件夹一插件，编排脚本在本层、想定/组网块在 `scenarios/` 子层；运行时中间输出（*.out / RESULT 文本）仍写 `temp\`（中间产物红线）。

## 目录结构

| 文件夹 | 对应插件（DLL） | 功能 | 验证入口 |
|---|---|---|---|
| [ysim_data_export/](./ysim_data_export/README.md) | ysim_data_export.dll | 数据出口：CSV/SQLite 落盘 + TCP/WS/UDP 推流 + 适配器热切换 | 双击 `ysim_hotswap_verify.bat` 等（清单见下节） |
| [ysim_dis_gateway/](./ysim_dis_gateway/README.md) | ysim_dis_gateway.dll | DIS 网关：七段码映射注入（B1）+ PDU 字段补齐（B2） | `dis_b1_verify.bat --regression` / `dis_b2_verify.bat` |
| [ysim_xio_gateway/](./ysim_xio_gateway/README.md) | ysim_xio_gateway.dll | XIO 网关：Packet 350-353 事件/状态下行 + 指令上行注入 | `xio_e2e_verify.bat --regression` |
| [e2e_visual_demo/](./e2e_visual_demo/README.md) | ysim_data_export.dll（WS 通道） | 端到端联调示例：mission 后台 + 浏览器 WebSocket 实时态势页 | `start_demo.bat`（演示）/ `e2e_ws_verify.bat` |
| [equipmentdb/](./equipmentdb/README.md) | ysim_equipment_db.dll | 装备数据库：EquipmentDB 脚本类只读查询 DB3K_modern.db3（计数/检索/整行/枚举） | `run_equipmentdb.bat` |

## bat 一键运行

> 详细使用手册（运行前提 / 通用机制 / 各 bat 用法详解 / 排错 FAQ）：[BAT_USAGE.md](./BAT_USAGE.md)。本节为速查索引。

所有示例脚本均配同名 `.bat`，全部为**一键快速验证**：双击自动完成「起 mission.exe（含对应想定/组网块）→ 跑客户端/断言 → 结束清理」，无需手工准备仿真。统一样式：bat 内注释/提示为**纯 ASCII 英文**（cmd 按系统 GBK 代码页解码 UTF-8 中文会显示乱码，纯 ASCII 在任何代码页/记事本/IDE 下零乱码），`chcp 65001` + `PYTHONUTF8=1` 保留供 Python 脚本 UTF-8 输出与产物编码使用，`cd /d %~dp0` 自动切到 bat 所在目录（双击或任意路径调用均可），结尾 `pause` 保留窗口查看输出；命令行调用时参数经 `%*` 原样透传（如 `dis_b1_verify.bat --regression`）。

bat 拉起的 mission 在独立最小化窗口运行（按窗口标题 `ysim-bat-*` 定向清理，控制台输出落 temp\bat_*.out）；hotswap / dis_b1 / dis_b2 / xio / e2e 等 verify 类由 Python 自行编排两轮 + 回归，bat 仅作启动入口。

| bat | 一键流程 | 参数 / 产物 |
|---|---|---|
| [start_demo.bat](./e2e_visual_demo/start_demo.bat) | 观摩演示：自动起仿真（起步暂停）+ 浏览器控制态势页 | 透传 run_demo.py：`--http-port` / `--no-browser` / `--scenario 路径`；窗口内 Ctrl+C 退出并结束仿真 |
| [e2e_control_verify.bat](./e2e_visual_demo/e2e_control_verify.bat) | 无头控制链路验证：起步冻结 / resume / pause / 4× 加速 / reset 重启 / 树杀清理 | 无参（桥接器用 18080 端口）；**开跑前全局 taskkill mission.exe**，勿与其他仿真并行 |
| [e2e_ws_verify.bat](./e2e_visual_demo/e2e_ws_verify.bat) | 无头 WS 推流验证：自动起演示想定，断言满足即提前终止 | 无参；校验握手 / 快照 / 状态流 / 心跳 / 字段 |
| [ysim_tcp_client.bat](./ysim_data_export/ysim_tcp_client.bat) | TCP 收流示例：自动起 smoke_2v2（RT 20s）→ 连 31600 收流至 sim_complete → 清理 | `set YSIM_PORT=` 可同时覆盖 mission 与客户端端口；产物 temp\ysim_client_result.txt |
| [ysim_multi_client.bat](./ysim_data_export/ysim_multi_client.bat) | 三通道并发断言：自动起 smoke_full（RT 20s）→ TCP+WS+UDP 同时收流 → 清理 | 产物 temp\ysim_multi_result.txt |
| [ysim_hotswap_verify.bat](./ysim_data_export/ysim_hotswap_verify.bat) | 适配器热切换验收：自起 hotswap 想定（RT 40s），list / add_udp / remove / add_tcp 时序断言 | 无参；产物 temp\ysim_hotswap_result.txt |
| [ysim_record_verify.bat](./ysim_data_export/ysim_record_verify.bat) | 落盘校验：自动跑 smoke_record（无 -rt 尽快完成）→ 校验 CSV/SQLite 四表/指标一致性 | 无参；校验 temp\ysim_record_out\ 最新 run_* |
| [ysim_bench_client.bat](./ysim_data_export/ysim_bench_client.bat) | 吞吐基准：自动起 bench_full（RT 600s 全通道）→ 连 31600 计行 → 到时/断连后清理 | 参数：`端口 秒数`（默认 31600 300；指定端口时 mission 同步覆盖） |
| [dis_listen.bat](./ysim_dis_gateway/dis_listen.bat) | DIS 监听：自动起 B1 组网+想定（RT 20s）→ 聚合 EntityState 七段码输出 RESULT | join 224.2.25.54:3225；出接口 IP 变化须同步改组网块与 .py `IFACE` |
| [dis_capture.bat](./ysim_dis_gateway/dis_capture.bat) | DIS 增强捕获：自动起 B2 组网+交战想定（RT 60s，CWD 自动切 dis\warfare）→ 四类 PDU 字段输出 RESULT | 同上（.py `LOCAL_IF`） |
| [dis_b1_verify.bat](./ysim_dis_gateway/dis_b1_verify.bat) | B1 七段码映射注入端到端（两轮 RT 20s） | `--regression`＝追加降级回归 |
| [dis_b2_verify.bat](./ysim_dis_gateway/dis_b2_verify.bat) | B2 PDU 字段补齐端到端（两轮 RT 60s） | `--regression`＝追加降级回归 |
| [xio_e2e_verify.bat](./ysim_xio_gateway/xio_e2e_verify.bat) | XIO 平台级指令集端到端（两轮 RT 45s + 回归） | `--regression`＝仅跑降级回归（跳过两轮端到端） |
| [run_equipmentdb.bat](./equipmentdb/run_equipmentdb.bat) | 装备数据库端到端：跑 EquipmentDB 脚本查询想定 → 与 SQLite 直查交叉断言（37 项，含关联表导航与组合过滤）→ 清理 | 无参；窗口标题 ysim-bat-equipmentdb；结果文件本目录 eqdb_result.txt（PASS 删 / FAIL 留） |

- **定向清理**：bat 拉起的 mission 按窗口标题（`ysim-bat-tcp` 等）只清理自己这一例，开跑前也先清同名残留；e2e_control_verify.bat 例外——开跑前**全局 taskkill mission.exe**
- **勿同时双击多个 bat**：316xx 端口与 224.x / 235.x 组播会互相冲突（smoke 系列与 bench_full 均用 31600/31601/31602）
- **换机运行**：DIS 组网块 IP 与 dis_listen / dis_capture 内 `IFACE` / `LOCAL_IF` 常量须同步改本机出接口 IP，详见「通用约定」第 3 条
- **产物统一落 temp\**（中间产物红线）：客户端/校验产物照旧，bat 拉起的 mission 控制台输出新增落 temp\bat_*.out，窗口内逐项打印 PASS/FAIL 与 RESULT 总结行

## 通用约定

1. **运行环境**：Python 3（标准库零依赖）+ 构建树 `build\Release\`（mission.exe / 插件已部署 mission_plugins\）
2. **RESULT 行**：全部编排脚本结束打印 `RESULT: {...}` JSON 行，供 CI/外部核对（含 PASS/FAIL 计数与明细键）
3. **组网参数**：DIS/XIO 多播块内 IP 按本机网卡实际地址段修改（`scenarios\` 内注释有标注；dis 组固定 224.2.25.54:3225 + 出接口 IP）
4. **回归模式**：`--regression` 参数 = 无对应 interface 块时插件应闲置告警且仿真正常完成（降级设计断言）
5. **两轮断言**：带 `_verify.py` 后缀的编排脚本按批次验证规范执行两轮 + 回归，历史结论见 `.trae\specs\` 对应工作包 spec.md
