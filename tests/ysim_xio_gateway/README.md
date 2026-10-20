# ysim_xio_gateway 测试（ext-02 阶段 A：XIO 网关）

> 插件：`build\Release\mission_plugins\ysim_xio_gateway.dll`（私有 Packet 350-353：事件/平台状态 JSON 下行 + 指令上行注入）。客户端程序：`build\Release\xio_gateway_client.exe`（源码 temp\xio_gateway_client\，协议明细见其源文件头）。批次详情见 `.trae\specs\ext-02-protocol-gateway\spec.md` §3-§5。

## 脚本清单

| 脚本 | 用途 | 运行方式 |
|---|---|---|
| xio_e2e_verify.py | 端到端全链路：起 xio_gateway_client（180s 超时）→ 3s 后起 mission（air_movers+xio 块，pause_for_connection）→ 10 条指令脚本（ping/add_platform/set_route/set_speed/pause/resume/get_status/delete_platform×2/负向 UNKNOWN）→ 断言增删/航路/调速/回执闭环；`--regression` 无块降级回归 | `python xio_e2e_verify.py --regression` |

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
