# AFSIM 二次开发总体规划（Roadmap）

> 状态：**计划态（未实施）** ｜ 依据：用户蓝图《往外连 / 往里长》双主线
> 所有工作包均为规划文档，落地时按 speckit 流程（先 spec/plan，后实现，实现后补充归档）

## 1. 双主线与四大方向

| 主线 | 方向 | 工作包 | 优先级 |
|---|---|---|---|
| 往外连（数据进出） | 数据输出 | ext-01-data-export（Observer/中间件/落盘） | P0（先打通数据） |
| 往外连（数据进出） | 外部接入 | ext-02-protocol-gateway（DIS/HLA/XIO/数实联动/推演后端） | P1 |
| 往里长（模型与工具链） | 功能增强 | ext-03-model-toolchain（模型库/行为智能/想定工具） | P2（业务价值主体） |
| 往里长（模型与工具链） | 智能与生态 | ext-04-eval-platform（评估/VV&A/国产化/性能） | P3（长期） |

## 2. 扩展点分层（二开对号入座，禁止直接改核心源码）

| 层级 | 基类 | 用途 | 典型回调 |
|---|---|---|---|
| Application | WsfApplicationExtension | 全局类型注册、脚本类注册 | AddedToApplication / ScenarioCreated |
| Scenario | WsfScenarioExtension | 新增场景脚本指令、组件类型注册 | AddedToScenario / ProcessInput / Complete |
| Simulation | WsfSimulationExtension | 运行时逻辑、数据收发、事件监听 | Initialize / PlatformsInitialized / Start / AdvanceTime / Complete |
| 组件 | WsfSensor / WsfProcessor / WsfMover / WsfWeapon | 新模型 | Initialize / Update / AttemptToDetect |
| 事件 | WsfEvent | 外部指令注入 | Execute() |
| 观察者 | WsfObserver::* | 只读数据采集（不改核心） | 订阅各类回调 |

**入口规范**：导出 `WsfPluginVersion` 与 `WsfPluginSetup`，DLL 放入 `wsf_plugins`，编译器版本一致，使用 `/MD`。

## 3. 实施路线（四阶段）

| 阶段 | 内容 | 对应工作包 |
|---|---|---|
| 一（打通数据） | Observer 插件骨架 + 一条外发通道（TCP/WebSocket）+ 最小外部客户端 | ext-01 |
| 二（标准化对外） | 统一消息中间件与多适配器、DIS 映射与自定义 PDU、标准态势流 | ext-01 / ext-02 |
| 三（模型与想定） | 装备模型库、行为模型、图形化想定工具 | ext-03 |
| 四（智能与生态） | RL/LLM 智能体、VV&A、国产化、SDK 分发 | ext-04 |

## 4. 全周期避坑清单（所有工作包强制遵守）

1. **插件 vs 核心**：能挂 Observer/Extension 就不改 WSF 源码，否则官方补丁无法合并
2. **时间语义先行定死**：实时/超实时、lock-step vs 异步、外部掉线外推策略，后期补必引发重构
3. **坐标系与时间基准统一**：WGS84、经纬度/地心、UTC vs 仿真时标——跨系统对接 80% 偏差源于此
4. **保留上游分支**：本地化与二开各自独立分支，定期 rebase，CI 跑官方示例场景回归
5. **性能红线**：AdvanceTime 增量推送 + 死区/降采样，禁止每帧全量遍历

## 5. 工作包索引

- [ext-01-data-export](ext-01-data-export/spec.md)：Observer 数据出口 / 统一消息中间件 / 结构化落盘
- [ext-02-protocol-gateway](ext-02-protocol-gateway/spec.md)：DIS/HLA/XIO 网关 / 数实联动 / 推演引擎后端
- [ext-03-model-toolchain](ext-03-model-toolchain/spec.md)：参数化模型库 / 行为与智能 / 想定工具链 / 批量实验
- [ext-04-eval-platform](ext-04-eval-platform/spec.md)：指标体系与报告 / VV&A / 分布式与国产化 / 性能剖析
