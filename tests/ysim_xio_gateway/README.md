# ysim_xio_gateway 测试（ext-02 阶段 A：XIO 网关）

> 插件：`build\Release\mission_plugins\ysim_xio_gateway.dll`（私有 Packet 350-353：事件/平台状态 JSON 下行 + 指令上行注入）。客户端程序：`build\Release\xio_gateway_client.exe`（源码 temp\xio_gateway_client\，协议明细见其源文件头）。批次详情见 `.trae\specs\ext-02-protocol-gateway\spec.md` §3-§5。

## 脚本清单

> **demo**：双击 [xio_demo.bat](./xio_demo.bat)（三窗联动，纯 bat 无 python：控制窗分步详细日志——环境检查/启动/5s 进程监测/清理复核，mission 自退即提前收尾；客户端窗 10 条指令脚本+回执直出；mission 窗插件 received/executed 日志直出。tests\ 全局策略见 [../BAT_USAGE.md](../BAT_USAGE.md)）。

| 脚本 | 用途 | 运行方式 |
|---|---|---|
| xio_demo.bat | **demo**：三窗联动观摩 XIO 指令闭环——控制窗分步详细日志（环境检查 5 项 → 起客户端 → 3s → 起 mission → 5s 一跳监测双进程 → mission 自退即提前收尾 → 清理输出可见 + 双 [OK] 复核）→ 客户端窗 10 条指令（ping/add_platform/set_route/set_speed/pause/resume/get_status/delete_platform×2/负向 UNKNOWN）回执实时直出 → mission 窗插件 received/executed 日志直出 | 双击即跑，无参 |
| xio_e2e_verify.py | 端到端全链路：起 xio_gateway_client（180s 超时）→ 3s 后起 mission（air_movers+xio 块，pause_for_connection）→ 10 条指令脚本（ping/add_platform/set_route/set_speed/pause/resume/get_status/delete_platform×2/负向 UNKNOWN）→ 断言增删/航路/调速/回执闭环；`--regression` 无块降级回归 | **验证**：`python xio_e2e_verify.py`（加 `--regression` ＝仅跑降级回归，跳过两轮端到端） |

## scenarios/ 想定与组网块

| 文件 | 用途 |
|---|---|
| xio_iface_block.txt | XIO 组网块：multicast 235.7.11.27 <本机网卡段前缀> 3002 + heartbeat 1s + pause_for_connection + send_aux_data_updates false |
| xio_rt_scenario.txt | RT 冒烟想定（20s 基础事件流） |
| xio_rt2_scenario.txt | 平台级指令集验证想定（45s，RouteMover 族 + XIO_DRONE 靶标，set_route/set_speed 载体） |

## 对接要点

- 客户端连多播 235.7.11.27:3002，JSON 行协议：下行 Packet 350（事件）/351（状态）/352（回执）/353（心跳），上行指令 pause/resume/get_status/ping/set_speed/set_route/add_platform/delete_platform
- `pause_for_connection true` 时 mission 等客户端 XIO 连接建立并收到 cRESUME 才开跑（无人值守场景勿开）
- xio_interface 扩展自动注册 dis_interface（硬依赖）；`send_aux_data_updates false` 避免未注册 ID 73 包的未知包告警
- 组网块多播地址第二参数按本机网卡地址段填写（如 192.168.）
