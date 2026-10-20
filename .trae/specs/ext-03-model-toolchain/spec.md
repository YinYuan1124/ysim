# 模型与想定工具链工作包规格（ext-03-model-toolchain）

> 序号：ext-03 ｜ 功能：装备模型库 / 行为与智能 / 想定与工具链 ｜ 状态：**实施中（批次 A1 导弹域参数化模型库，2026-10-20 启动）**
> 优先级：P2（第三阶段"模型与想定"，业务价值主体） ｜ 依赖：ext-01 数据通道（RL 训练回采）

## 1. 背景与目标

往里长：扩充模型、增强行为智能、武装想定工具链，让战术人员而非程序员能编场景。

- 约束：新模型一律走组件扩展点（WsfSensor/WsfProcessor/WsfMover/WsfWeapon），不改核心
- 代码风格：遵循项目规范（中文注释、二次开发前缀、UTF-8 BOM 单层）

## 2. 子模块与决策明细

### A. 装备模型库

- **国产装备参数化模型库**：传感器、干扰机、导弹、数据链，做成"指标→参数"配置化工具，避免硬编码
- **薄弱域补齐**：电子战（干扰/抗干扰、频谱管理）、通信链路（组网、带宽、时延、丢包）、网络空间、认知域
- **多分辨率切换**：同一实体在高保真物理模型与聚合行为模型间动态切换（支撑大规模兵力）

### B. 行为与智能层

- Processor / 行为树增强：任务分配、编队协同、动态重规划
- **RL 环境封装**：Gym 接口（reset() 加载想定、step() 推进返回 observation/reward）
- **LLM 接入**：高层意图下达与裁决解释
- 红/蓝方 AI 代理框架：无人集群、有人-无人协同试验

### C. 想定与工具链

- 图形化想定编辑器：拖拽装配、参数面板、语法校验
- Wizard 汉化延伸（承接 i18n-batch18）：命令提示补全、F1 帮助跳转中文文档、SDL 语法高亮（VSCode/LSP）、想定校验器
- 批量实验设计：正交试验、拉丁超立方抽样、灵敏度分析，对接 Mystic

## 3. 验证与收尾标准

1. 每个新模型组件走扩展点实现，官方升级可合并
2. 参数化模型库：同一指标集可生成多型号装备参数（配置化，非硬编码）
3. RL 环境：reset/step 契约可用，observation/reward 稳定可收敛（冒烟训练）
4. 想定编辑器产出合法 .txt 想定（Mystic 可加载）
5. 批量实验对接 Mystic 跑通正交/拉丁超立方
6. 记账三件套归档（doc 日志/SKILL.md/project_memory.md）

## 4. 阶段 A 批次 A1 实施规格：参数化模型库原型·导弹域（2026-10-20 定稿）

### 4.1 源码侦察结论（2026-10-20）

1. **官方导弹武器链完整范式**（`build/demos/base_types/weapons/aam/simple_medium_range_missile.txt`）：
   - `platform_type` 挂四组件：`mover WSF_GUIDED_MOVER`（total_mass / fuel_mass / specific_impulse / thrust / thrust_duration / update_interval / aero）+ `processor autopilot WSF_GUIDANCE_COMPUTER`（proportional_navigation_gain / velocity_pursuit_gain / g_bias / maximum_commanded_g）+ `processor seeker WSF_PERFECT_TRACKER`（理想导引头）+ `processor fuse WSF_AIR_TARGET_FUSE`（time_of_flight_to_arm / max_time_of_flight_to_detonate / detonate_on_mach_decreasing_to / detonate_below_height_AGL）
   - `weapon <名> WSF_EXPLICIT_WEAPON`：launched_platform_type + weapon_effects + quantity / firing_delay / salvo_interval / slew_limits
   - 配套：`aero` = WSF_AERO 的 mach_and_cd 表（0-4 Ma）+ reference_area + cl_max / aspect_ratio；`weapon_effects` = WSF_GRADUATED_LETHALITY radius_and_pk（launch computer 生成版需 Pk 1.0 副本）
2. **验证载体现成**：SA-10C 地空交战链（`src/core/wsf_mil/test_mission/dis/warfare/types.txt`）经 ext-02 B2 批次验证单进程全触发（Fire×2 / Detonation×2 / Emission×14）——A1 端到端断言直接改造该骨架
3. **EW 链对比**（`src/core/wsf_mil/source/ew/WsfEW_Effect.cpp` ProcessInput）：effect_name / system_type_data / 目标保护 / 允许拒绝集等配置面宽、频谱参数深，"指标→参数"首批性价比低于导弹
4. **选型结论**：首批垂直域 = **导弹（AAM 空空 / SAM 地空）**。零 C++ / CMake 改动，纯外部生成工具 + 想定类型产物；换算链有显式物理公式（比冲/火箭动力学、比例导引经验增益）

### 4.2 设计

#### 4.2.1 工具落位

`src/ysim_tools/model_gen/`（纯 Python 3 标准库零依赖，不进 CMake 构建树）：

- `ysim_model_gen.py`——指标 JSON → WSF 类型 .txt 生成器（CLI：`--input <json> --outdir <dir>`）
- `models/`——3 个示例型号指标 JSON（自拟训练代号，非真实装备参数）

生成 .txt 全 ASCII（WSF 想定输入保守 ASCII）；designator 限大写字母数字下划线。Python 源文件中文注释、UTF-8 无 BOM（Python3 安全，C++ BOM 红线不适用）。

#### 4.2.2 导弹域指标 schema v1（JSON 字段表）

| 字段 | 必填 | 默认 | 说明 |
|---|---|---|---|
| designator | 是 | - | 型号代号（ASCII，作类型名） |
| class | 是 | - | "AAM"（空空）/ "SAM"（地空） |
| range_km | 是 | - | 射程 |
| launch_mass_kg | 是 | - | 发射质量 |
| warhead_mass_kg | 是 | - | 战斗部质量 |
| cruise_mach | 是 | - | 巡航马赫 |
| max_mach | 是 | - | 最大马赫 |
| max_g | 是 | - | 机动过载上限 |
| diameter_m | 否 | 0.178 | 弹径 |
| seeker.type | 否 | "perfect" | "perfect"/"radar"/"ir"（radar/ir 首批登记→降级 perfect 并注释占位，A2 批接真导引头模型） |
| seeker.acquisition_range_km / seeker.fov_deg | 否 | - | 登记（perfect 跟踪器不受限，A2 生效） |
| pn_gain | 否 | 30 | 比例导引增益 |
| quantity | 否 | 4 | 挂弹量 |
| fuse_arm_time_s | 否 | 2.0 | 引信解除保险时间 |

#### 4.2.3 换算规则（显式可审计，全部回显进生成文件头注释）

- 平均速度 `V_avg = 0.8 × cruise_mach × 340 m/s`（末段能量衰减折减系数）
- 总飞行时间 `t_tot = range_km × 1000 / V_avg`
- 动力段时长 `thrust_duration = 0.3 × t_tot`
- `fuel_mass = 0.38 × launch_mass_kg`（固体火箭装药比典型区间 0.35-0.45 取中）
- `thrust = fuel_mass × Isp(250 s) × 9.81 / thrust_duration`（N）
- `max_time_of_flight_to_detonate = 1.5 × t_tot`
- `reference_area = π × diameter_m² / 4`
- aero：官方 GUESSED_AERO mach_and_cd 曲线按 max_mach 截断复用（A2 再按气动特性分档）
- `maximum_commanded_g = max_g`；`proportional_navigation_gain = velocity_pursuit_gain = pn_gain`；`g_bias = 1.2`（官方缺省）
- weapon_effects：`radius_m = 10 × √warhead_mass_kg`（杀伤半径经验式）、pk 0.75；另生成 launch computer 用副本（pk 1.0，官方 demo 同款双份模式）
- signature：1 m² / 1 W/sr 常量（同官方 demo 模板）；slew_limits 沿用 demo（方位 ±180°、俯仰 10-70°）

#### 4.2.4 生成产物结构

每型号一个 `<designator>.txt`：注释头（指标回显 + 换算中间量全量审计行）+ 三 signature（类型名带型号前缀）+ aero + weapon_effects（正常版 + pk1.0 副本）+ `platform_type <designator>`（mover/autopilot/seeker/fuse 四组件链）+ `weapon <designator> WSF_EXPLICIT_WEAPON`。

### 4.3 验证方案（两轮断言 + 降级回归）

- **轮 1 结构断言**（Python 自检）：3 型号全部生成成功；关键参数与换算公式预期值逐一断言（thrust / thrust_duration / max_time_of_flight_to_detonate / maximum_commanded_g / reference_area / aero 按 max_mach 截断 / weapon_effects 双份）；非法指标（缺必填字段 / 负值 / class 越界）显式报错且不产出文件
- **轮 2 端到端交战断言**：改造 B2 骨架——红方发射平台（SA-10C_LAUNCHER 派生）换装生成型号 TY_SAM_1，蓝方 F-18 保持 30000ft 500kts 航线；mission.exe 60 s + DIS 抓包（复用 temp/dis_listen.py 范式）断言：Fire≥1、TY_SAM_1 导弹平台出现在 Entity State 流、Detonation≥1（命中或引信 maxToF 触发）
- **降级回归**：temp/dis_b2_scenario.txt 原样重跑冒烟（B2 基线断言不漂移）；生成工具重复运行对已存在输出覆盖幂等
- **验收对照**总规格 §3.2：同一 schema 生成 3 型号（配置化、非硬编码、工具内零型号特判分支）

### 4.4 红线自查

零 C++ / CMake / 核心源码改动；纯新增 `src/ysim_tools/` 目录与生成产物；本批无 C++ 文件故 BOM 红线不触发（.py 中文注释 UTF-8 无 BOM 安全、生成 .txt 全 ASCII）；中间产物（验证脚本/抓包日志）只落 temp/。
