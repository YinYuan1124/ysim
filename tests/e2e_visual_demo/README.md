# 端到端联调示例（e2e_visual_demo）

> 演示目标：**mission.exe 后台无界面仿真 → 浏览器实时态势页 + 仿真控制台**（开始/暂停/重置/仿真频率/原始数据流）。零第三方依赖（纯标准库 Python + 原生 HTML/JS）。

## 一键启动（推荐）

双击 `start_demo.bat`（或命令行运行），自动完成：拉起 mission 后台仿真（**起步暂停**）→ 启动 Web 服务 → 打开浏览器 `http://127.0.0.1:8080/`。

页面操作：
- **开始仿真**：解除起步暂停，仿真推进（数据流/态势随之运动）
- **暂停 / 继续**：运行中冻结/恢复时钟（时间严格不跳变）
- **重置**：重启仿真进程，回到起步暂停（态势清空、快照重推）
- **频率选择**：0.5×–8× 时钟倍率，运行中即时生效
- **状态徽标/控件联动**：就绪/运行中/已暂停/仿真结束/重启中——控件随状态启用禁用，每次交互回执后刷新
- **右侧面板**：**实时数据流表格**（每条仿真数据追加一行，红/蓝平台整行着色，数据增多自动出滚动条——上滑查看历史、贴底自动跟随最新，上限 2000 行）+ 事件与命令日志 / 原始数据流（页签切换）

退出：桥接器窗口内 **Ctrl+C**（同时结束仿真进程）。

无头验证（不开浏览器，自起自清 mission，`python` 直跑——bat 仅保留 demo 定位，全局策略见 [../BAT_USAGE.md](../BAT_USAGE.md)）：`python e2e_control_verify.py`（控制链路 13 断言：起步冻结/推进/暂停/4×加速/重置/树杀清理）、`python e2e_ws_verify.py`（WS 推流 6 断言：握手/快照/状态流/心跳/字段）。

## 文件清单

| 文件 | 用途 |
|---|---|
| start_demo.bat | 一键启动入口（demo） |
| run_demo.py | 桥接器：mission 进程管理（拉起/重置/清理）+ HTTP 服务 + /ws 控制端点（控制指令转发到 TCP 31610） |
| situation.html | 浏览器控制与态势页（自包含单文件）：控制栏 + 状态机 + 俯视态势图 + 平台表格 + 事件/命令日志 + 原始数据流 |
| e2e_control_verify.py | 无头全链路验证：起步冻结/推进/暂停冻结/4×加速/重置重启/树杀清理，13 断言 |
| e2e_ws_verify.py | 无头推流链路验证（自动跑场景）：握手/快照/状态流/心跳/字段，6 断言 |
| scenarios/e2e_control_scenario.txt | 控制演示想定：2v2 + 三通道 + control_port 31610 + start_paused + 1s 心跳 |
| scenarios/e2e_visual_scenario.txt | 观摩演示想定：2v2 + 三通道推流（自动运行，600s） |

## 控制协议

**浏览器 → 桥接器（WS `/ws`，JSON 文本帧）**：`{"action":"pause"|"resume"|"status"|"reset"|"set_rate","value":<倍率>}` → 回执 `{"type":"control_result"/"sim_status",...,"action":...}`。

**桥接器 → 仿真（TCP 31610 行协议）**：`pause` / `resume` / `set_rate <0.1-100>` / `status`（另有 add_tcp/add_ws/add_udp/remove/list 热切换命令）——status 返回 `{"type":"sim_status","state","paused","clock_rate","sim_time","platform_count"}`。reset 由桥接器重启进程实现（引擎不支持进程内重置想定）。

**数据流路由（/data 同源转发）**：页面数据流默认经桥接器同源端口转发（ws://127.0.0.1:8080/data → 插件 WS 31601），单端口依赖——页面能打开数据就能通，免受跨端口环境差异影响；`?ws=host:port` 显式指定时直连插件推流端口（高级用法）。远程机器访问页面时同理走桥接。

**插件配置块新增项**（ysim_data_export）：`control_port <n>`（控制通道端口）、`start_paused true`（仿真首帧即暂停，等 resume 指令）。

## 手工运行（不经桥接器）

```powershell
cd e:\afsim\ysim\build\demos\air_to_air
e:\afsim\ysim\build\Release\mission.exe -rt e:\afsim\ysim\tests\e2e_visual_demo\scenarios\e2e_control_scenario.txt
# 浏览器打开 situation.html（自动连 ws://127.0.0.1:31601 收流；控制指令可发 TCP 31610）
```

## WS 推流协议（RFC 6455 文本帧，每帧一行 JSON）

| type | 字段 | 说明 |
|---|---|---|
| platform_added | id/name/platform_type/side | 平台加入（连接后快照补推 + 运行期动态） |
| platform_removed | id/name | 平台移除 |
| state | id/name/side/lat/lon/alt_m/speed_mps/heading_deg/pitch_deg/roll_deg/time | 平台状态（deadband 2m / 最低 0.2s 间隔） |
| heartbeat | time | 心跳（想定可配，控制演示用 1s） |
| sim_start / sim_complete | - | 仿真起止 |

## 已知修复记录

- 2026-10-20：修复插件 WS 握手 `Sec-WebSocket-Accept` 计算错误（SHA1 输出循环移位窗口错位 j=31/23/15/7→24/16/8/0），此前浏览器/标准客户端严格校验 Accept 全部拒绝握手，自研 Python 客户端因未校验 Accept 而漏检。详见 doc 修改日志第十三批。
- 2026-10-20：新增仿真控制能力（pause/resume/set_rate/status 控制命令 + start_paused 配置项 + 控制面板演示链路），详见 doc 修改日志第十六批。
- 2026-10-20：修复用户侧数据流不显示（系统代理/安全软件拦截 31601 非常规端口——新增 /data 桥接转发端点 + 页面直连失败自动降级）与控制台中文乱码（Python 管道 GBK × 终端 UTF-8 解码错配——强制 UTF-8 输出 + chcp 65001）。详见 doc 修改日志第十七批。
- 2026-10-20：修复"依旧无数据"——真实根因是演示进程死亡/多实例冲突（残留 mission 占 316xx 端口 + 8080 冲突致桥接器崩溃）：启动前自动清理残留实例（仅杀 python/mission）；数据流默认改走同源 /data 转发（单端口依赖）；代理拦截理论证伪（ProxyOverride bypass 127.*）。详见 doc 修改日志第十八批。
- 2026-10-20：修复态势画布渲染（数据在流、帧数在涨但画布空白）——draw() 中 X/Y 投影函数声明在 if 块内 + 严格模式块级作用域致块外 ReferenceError，平台标记/航迹/表格全部画不出；改为块外 var + 块内函数表达式赋值。详见 doc 修改日志第二十一批。
- 2026-10-20：修复"没有表格"真根因——flex 布局 min-width:auto 陷阱：canvas width 属性反馈锁死把 #side 整体挤出视口外（表格一直在 DOM 里但用户看不见）；`#cv{min-width:0}` 一行根治 + no-store 响应头 + 高可见度滚动条 + 画布标签贴边翻转。截图对比验收（1400×900 与 1000×700 双尺寸：8 列表头 + 4 行红蓝数据全部可见）。详见 doc 修改日志第二十五批。
