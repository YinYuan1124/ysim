# 端到端联调示例（e2e_visual_demo）

> 演示目标：**mission.exe 后台无界面仿真 → ysim_data_export WebSocket 推流 → 浏览器实时态势页**。零第三方依赖（纯标准库 Python + 原生 HTML/JS）。

## 文件清单

| 文件 | 用途 |
|---|---|
| situation.html | 浏览器实时态势页（自包含单文件）：俯视图自动缩放、蓝红标记+航向线+尾迹、平台表格、事件日志、仿真时间/帧计数状态栏；右上输入框可改 WS 地址 |
| e2e_ws_verify.py | 无头端到端验证脚本：起 mission → 裸 RFC 6455 客户端收流 → 6 断言（握手/快照补推≥4/状态流≥20/心跳/JSON 零失败/字段齐全）→ RESULT JSON 行 |
| scenarios/e2e_visual_scenario.txt | 演示想定：air_to_air 2v2 + TCP/WS/UDP 三通道推流块，end_time 600s |

## 运行步骤

```powershell
# 1. 启动后台仿真（注意 CWD 必须在 demo 目录，想定内 include 用相对路径）
cd e:\afsim\ysim\build\demos\air_to_air
e:\afsim\ysim\build\Release\mission.exe -rt e:\afsim\ysim\tests\e2e_visual_demo\scenarios\e2e_visual_scenario.txt

# 2. 浏览器打开态势页（双击 situation.html 或任意静态服务）
#    默认连 ws://127.0.0.1:31601，中途接入自动补推 platform_added 快照

# 3. （可选）无头验证全链路
python e:\afsim\ysim\tests\e2e_visual_demo\e2e_ws_verify.py
```

## WS 推流协议（RFC 6455 文本帧，每帧一行 JSON）

| type | 字段 | 说明 |
|---|---|---|
| platform_added | id/name/platform_type/side | 平台加入（连接后快照补推 + 运行期动态） |
| platform_removed | id/name | 平台移除 |
| state | id/name/side/lat/lon/alt_m/speed_mps/heading_deg/pitch_deg/roll_deg/time | 平台状态（deadband 2m / 最低 0.2s 间隔） |
| heartbeat | time | 心跳（5s 周期） |
| sim_start / sim_complete | - | 仿真起止 |

## 已知修复记录

- 2026-10-20：修复插件 WS 握手 `Sec-WebSocket-Accept` 计算错误（SHA1 输出循环移位窗口错位 j=31/23/15/7→24/16/8/0），此前浏览器/标准客户端严格校验 Accept 全部拒绝握手，自研 Python 客户端因未校验 Accept 而漏检。详见 doc 修改日志第十三批。
