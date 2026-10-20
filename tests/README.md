# tests 测试资产总览

> 二次开发新增功能的测试脚本与配套想定归档区。按功能一文件夹一插件，编排脚本在本层、想定/组网块在 `scenarios/` 子层；运行时中间输出（*.out / RESULT 文本）仍写 `temp\`（中间产物红线）。

## 目录结构

| 文件夹 | 对应插件（DLL） | 功能 | 验证入口 |
|---|---|---|---|
| [ysim_data_export/](./ysim_data_export/README.md) | ysim_data_export.dll | 数据出口：CSV/SQLite 落盘 + TCP/WS/UDP 推流 + 适配器热切换 | `python ysim_hotswap_verify.py` 等 |
| [ysim_dis_gateway/](./ysim_dis_gateway/README.md) | ysim_dis_gateway.dll | DIS 网关：七段码映射注入（B1）+ PDU 字段补齐（B2） | `python dis_b1_verify.py --regression` / `dis_b2_verify.py` |
| [ysim_xio_gateway/](./ysim_xio_gateway/README.md) | ysim_xio_gateway.dll | XIO 网关：Packet 350-353 事件/状态下行 + 指令上行注入 | `python xio_e2e_verify.py --regression` |
| [e2e_visual_demo/](./e2e_visual_demo/README.md) | ysim_data_export.dll（WS 通道） | 端到端联调示例：mission 后台 + 浏览器 WebSocket 实时态势页 | `python e2e_ws_verify.py` |

## 通用约定

1. **运行环境**：Python 3（标准库零依赖）+ 构建树 `build\Release\`（mission.exe / 插件已部署 mission_plugins\）
2. **RESULT 行**：全部编排脚本结束打印 `RESULT: {...}` JSON 行，供 CI/外部核对（含 PASS/FAIL 计数与明细键）
3. **组网参数**：DIS/XIO 多播块内 IP 按本机网卡实际地址段修改（`scenarios\` 内注释有标注；dis 组固定 224.2.25.54:3225 + 出接口 IP）
4. **回归模式**：`--regression` 参数 = 无对应 interface 块时插件应闲置告警且仿真正常完成（降级设计断言）
5. **两轮断言**：带 `_verify.py` 后缀的编排脚本按批次验证规范执行两轮 + 回归，历史结论见 `.trae\specs\` 对应工作包 spec.md
