# ysim_data_export 测试（ext-01 数据出口）

> 插件：`build\Release\mission_plugins\ysim_data_export.dll`（Observer 订阅 → CSV/SQLite 落盘 + TCP/WS/UDP 推流 + 适配器热切换）。批次详情见 `.trae\specs\ext-01-data-export\spec.md`。

## 脚本清单

> 全部脚本均配同名 `.bat` **一键运行**（双击自动「起 mission + 想定 → 跑测试 → 清理」；通用机制与排错 FAQ 见 [../BAT_USAGE.md](../BAT_USAGE.md)）。

| 脚本 | 用途 | 一键运行（bat，推荐） |
|---|---|---|
| ysim_tcp_client.py | TCP 收流客户端（最小对接示例：连 31600 收全量行流，结果写 temp\ysim_client_result.txt） | 双击 `ysim_tcp_client.bat`：自动起 ysim_smoke_2v2（RT 20s）→ 收流至 sim_complete → 清理；先 `set YSIM_PORT=32700` 再双击可整链换端口。手工：起 mission 后 `python ysim_tcp_client.py` |
| ysim_multi_client.py | 多通道并发客户端：TCP 31600 + WS 31601 + UDP 组播 239.16.16.16:31602 三通道同时收流断言 | 双击 `ysim_multi_client.bat`：自动起 ysim_smoke_full（RT 20s）→ 三通道并发断言 → 清理。手工：mission 起三通道想定后 `python ysim_multi_client.py` |
| ysim_hotswap_verify.py | 适配器热切换验收：运行期 list/add_udp/remove/add_tcp/remove 指令时序（t=5/7/12/13/14/18s），全程主通道不受影响 | 双击 `ysim_hotswap_verify.bat`（自起 mission -rt 40s，想定 scenarios\ysim_smoke_hotswap.txt） |
| ysim_record_verify.py | 结构化落盘校验：run_* 目录三 CSV 表头/事件全生命周期/SQLite 四表行数/指标一致性抽查 | 双击 `ysim_record_verify.bat`：自动跑落盘想定（无 -rt 尽快完成）→ 校验 temp\ysim_record_out\run_* |
| ysim_bench_client.py | 吞吐基准客户端（连端口收流统计至截止时间，参数：端口 秒数） | 双击 `ysim_bench_client.bat [端口] [秒数]`（默认 31600 300）：自动起 ysim_bench_full → 计行 → 清理；指定端口时 mission 同步覆盖 |

## scenarios/ 想定

| 文件 | 用途 |
|---|---|
| ysim_smoke_2v2.txt | 冒烟：2v2 空战 + TCP 推流块 |
| ysim_smoke_full.txt | 全通道：TCP+WS+UDP 三推流块 |
| ysim_smoke_hotswap.txt | 热切换时序验证想定 |
| ysim_smoke_record.txt / _off | CSV+SQLite 落盘开启/关闭对照 |
| ysim_bench_base / _base60 / _full / _full60 | 吞吐基准（基础/全通道 × 20s/60s） |

## 对接要点

- 想定内配置块：`ysim_data_export ... port 31600 ... end_ysim_data_export`（块命令清单见插件源码头注释）
- 命令行覆盖：`mission.exe --ysim-port 32700 想定.txt`（**选项必须在文件名之前**，框架约定）
- 输出落盘根目录由想定块指定，本测试历史运行产物在 temp\ysim_record_out\
