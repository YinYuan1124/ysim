# AFSIM 二次开发工程（ysim）

## 工程介绍

本工程基于 **AFSIM v2.9.0**（The Advanced Framework for Simulation, Integration, and Modeling）源码开展**持续性二次开发**。本地化（i18n）是当前已系统性落地的第一个功能板块，后续将围绕**仿真模型扩展、插件开发、工具链定制、想定工程支持**等方向持续开展二开工作。

**本地化板块目标**：在保持官方技术支持可用（日志落盘英文 + 消息 ID）的前提下，实现界面与诊断信息的中文本地化。

- **构建链**：CMake + Visual Studio 2022（MSVC v144）+ Qt 5.12.11，Release/Debug 双配置
- **产物**：`build/Release/` 下 mission.exe（想定运行）、wizard.exe（想定编辑器）及各领域 DLL
- **编译说明**：见 [doc/如何编译.md](doc/如何编译.md)；每日改动记录见 [doc/](doc/) 目录（"日期+修改日志"每日一档）
- **项目规范**：`.trae/PROMPT.md`（项目级强制规范）；深入开发指南：`.trae/skills/afsim-dev/SKILL.md`

### 二开强制约定

1. 新增/修改代码必须带中文注释，修改处标注 `// 二次开发：` 前缀及原因
2. 含中文的源文件必须保存为 **UTF-8 with BOM**（MSVC 无 BOM 会按 CP936 解析报语法错）
3. 含中文的 QString 字面量必须使用 `QStringLiteral`（MSVC 窄字符串默认 GBK 编码）
4. 报错消息走消息目录 ID 化，禁止业务代码硬编码中文；想定语法关键字/枚举值/事件 schema 一律不翻译
5. 保留原版 CUI 版权声明头，源码不外传

## 项目目录

```
ysim/
├── README.md               # 本文件
├── .gitignore              # 忽略 build/、dependencies/、.trae/ 及二进制产物
├── .vscode/                # wizard 调试/智能感知配置（cppvsdbg 须用官方 VS Code / VS2022）
├── doc/                    # 如何编译.txt + 每日修改日志（日期+修改日志.md）
│
├── src/                    # AFSIM v2.9.0 源码树（CMake 源目录，二开改造在此进行）
│   ├── CMakeLists.txt      # 顶层 CMake 入口
│   ├── core/               # 仿真内核与领域库
│   │   ├── wsf/            # 核心仿真库：平台/传感器/事件/想定解析/通信
│   │   ├── wsf_mil/        # 军事扩展库：武器/杀伤模型/发射计算机/交战（本工程改造重点）
│   │   ├── wsf_util/       # wsf 辅助库
│   │   ├── wsf_parser/     # 想定解析器
│   │   ├── wsf_cyber/      # 网络空间域
│   │   ├── wsf_space/      # 空间域（轨道/卫星）
│   │   ├── wsf_mtt/        # 多目标跟踪
│   │   ├── wsf_nx/ wsf_l16/ wsf_mil_parser/ wsf_simdis/ wsf_ripr/ wsf_grammar_check/
│   │   ├── wsf_weapon_server/  # 武器服务器
│   │   └── sensor_plot_lib/ # 传感器绘图库
│   ├── tools/
│   │   ├── util/           # 基础工具库：日志/输入/数学（含 UtMsgCatalog 消息目录 i18n 基础设施）
│   │   ├── wkf/            # Wizard 通用框架库（Qt）
│   │   ├── utilqt/ utilosg/ # Qt/OSG 封装库
│   │   ├── dis/            # DIS 分布式仿真协议
│   │   ├── geodata/ genio/ scene_gen/ tracking_filters/ packetio/ profiling/
│   │   ├── artificer/      # 想定验证工具库
│   │   └── 3rd_party-cmake/ # 三方库 CMake 脚本
│   ├── wizard/             # 想定编辑器（Qt，中英切换与界面翻译在此）
│   ├── mission/            # 想定运行主程序（mission.exe）
│   ├── warlock/            # 交互式脚本前端（warlock.exe）
│   ├── engage/             # 交战分析工具
│   ├── mystic/             # 批量想定运行工具（mystic.exe）
│   ├── evt_reader/         # 事件文件读取工具
│   ├── mover_creator/ post_processor/ sensor_plot/ weapon_tools/  # 配套工具
│   ├── wsf_plugins/        # 仿真插件（air_combat/p6dof/simdis 等）
│   └── cmake/ doc/         # CMake 模块 / 源内文档
│
├── build/                  # CMake 构建输出（git 忽略）
│   ├── CMakeCache.txt      # 配置缓存（生成器 VS17 2022，源目录 E:/afsim/ysim/src）
│   ├── 3rd_party/          # 预编译三方依赖（Qt 5.12.11、geos 等，含 lupdate/lrelease 工具链）
│   ├── Release/ Debug/     # 产物目录：mission.exe、wizard.exe、warlock.exe 及各领域 DLL
│   ├── build_wsf/ build_wsf_mil/ build_wizard/ build_mission/ ...  # 各目标子构建目录（含 .vcxproj）
│   ├── wsf_install/        # INSTALL 目标安装产物（bin/ 下拷回 Release/）
│   ├── resources/ demos/ documentation/  # wizard 运行资源
│   └── x64/ lib/ include/ doxygen/ ...   # 中间产物与生成头文件
│
└── dependencies/           # 第三方依赖包（git 忽略）
```

## 编译步骤

> 环境要求：Visual Studio 2022（MSVC v144）、CMake ≥ 3.20。源码目录为 `src/`，构建目录为 `build/`。

### ① CMake 配置生成工程

```powershell
# 在仓库根目录执行（注意：源码目录是 src/，不是仓库根目录）
cmake -S src -B build -G "Visual Studio 17 2022"
```

- 生成器：Visual Studio 17 2022；配置类型：Release / Debug / MinSizeRel / RelWithDebInfo
- 安装前缀默认 `build/wsf_install`

### ② 全量编译（Release）

```powershell
# 方式一：命令行（推荐）
cmake --build build --config Release

# 方式二：VS2022 打开 build 下的 .sln，编译 ALL_BUILD
```

### ③ 执行 INSTALL 编译并回拷产物

```powershell
cmake --build build --config Release --target INSTALL
# 将 wsf_install/bin 下所有文件拷贝覆盖到 build/Release/
Copy-Item build\wsf_install\bin\* build\Release\ -Force
```

### ④ 配置 wizard 运行环境

1. 将 wizard 设为启动项（VS2022 内右键 wizard 项目 → 设为启动项目；或直接运行 `build/Release/wizard.exe`）
2. 将所需的 `resources/`、`demos/`、`documentation/` 拷贝到 `build/` 目录下（wizard 按 build 根查找这些资源）

### ⑤ 增量编译与单目标编译

```powershell
# 只重编受影响目标（例：改了 wsf_mil 库后验证 mission 链路）
cmake --build build --config Release --target wsf_mil mission

# wizard 链路
cmake --build build --config Release --target wizard
```

依赖链：`util → wsf → wsf_mil → mission/wizard`；改 UtMsgCatalog（util 库）会触发下游全链路重编。

### 编译坑点（实测记录）

| 坑点 | 现象 | 处理 |
|---|---|---|
| 含中文源文件无 BOM | MSVC 按 CP936 解析，行号错乱语法错 | 统一保存 UTF-8 with BOM |
| WsfThreadPool.hpp 头文件缺失 | 编译失败 | 已补 `<chrono>`/`<vector>`（现行源码已含） |
| 间接依赖 DLL 缺失 | wizard 启动秒退 | 从 `build/3rd_party` 对应库的 release/debug 子目录补齐（如 geos.dll）到 `build/Release`、`build/Debug` |
| 两个编译进程并行 | obj 文件 Permission denied | 结束残留 MSBuild 进程后重跑，勿同时启动多个构建 |
| util 新增源文件 | 改动不生效 | util 是 GLOB 收集，新增文件须重新 configure |


## 功能点

### 一、本地化板块（已落地）

#### 1. wizard 中英切换（P0）

- 入口：wizard → Options → Preferences → Application → Language 页（`LanguagePrefWidget`）
- 语言项（`en` / `zh_CN`）持久化到 Settings.ini `[LanguageOptions] language`，重启生效
- translator 加载于 `Runner::run()`：qtbase_zh_CN + wizard_zh_CN，qm 运行时位于 `build/Release/i18n/`

#### 2. 界面 tr() 翻译（P1，已完成）

- 第一批 tr() 改造 8 组件：StartupDialog、ContextMenuActions、CreatePlatformDialog、ClonePlatformDialog、DeveloperActions、AutoCompletePopup、ContextDocumentationControl、FindResultsControl（含补齐缺失的 Q_OBJECT 宏）
- 翻译资产 `src/wizard/lib/i18n/wizard_zh_CN.ts`：**310 条全量中文翻译**（lrelease 310 finished / 0 unfinished），覆盖主菜单、Preferences、对话框、单位页等 39 个上下文

#### 3. 内核报错消息 ID 化（L2 消息层 MsgCatalog，已十五批全域覆盖）

- 基础设施：`src/tools/util/source/UtMsgCatalog.{hpp,cpp}`
  - 消息 ID 分段：0x0001 想定解析 / 0x0002 平台运动 / 0x0003 传感器 / 0x0004 交战武器 / 0x0005 通信 / 0x0006 工具 UI
  - 编译期内置目录表：每条含英文原文 / 中文译文 / 原因说明 / 修复建议（英文原文一字未动）
  - `{参数}` 占位符引擎 + 双通道 API：日志落盘英文带 `[E:ID]` 前缀（可 grep 排错），界面显示中文译文
- 覆盖进度：**十五批累计 280 条目录条目 / 303 处调用点替换**，weapon、comm、sensor、processor、mover、dis、ew 各域 error 级硬编码报错全部闭段
- 接入范式：hpp 分配 ID 常量 → cpp 目录表登记条目 → 业务侧 `ut::log::error() << ut::msg::FormatEnTagged(ID, args)`，AddNote 附注保持英文

### 二、工程化建设（已落地）

#### 4. 编译与调试环境

- CMake（VS2022 + Release）全量编译通过；修复 `WsfThreadPool.hpp` 缺 `<chrono>`/`<vector>` 头文件
- `.vscode/` 提供 wizard Debug/Release 双配置调试（TRAE 编辑器不支持 cppvsdbg，调试须用官方 VS Code 或 VS2022）
- 补齐间接依赖 DLL（geos.dll 等）到 `build/Release`、`build/Debug`，避免启动秒退

#### 5. 工程治理

- `.gitignore` 排除 build/dependencies 产物与大文件
- `doc/` 修改日志按"日期+修改日志"每日一档归档
- `.trae/PROMPT.md` 项目强制规范 + `afsim-dev` 二开技能（源码地图/坑表/接入范式）

### 三、后续二开方向（规划中）

本地化之外，本工程规划持续开展以下二开工作（按需排期）：

- **仿真模型扩展**：传感器、武器、运动体（mover）模型的定制开发与参数调优
- **插件开发**：基于 wsf_plugins 机制开发专用领域插件（air_combat/p6dof 模式）
- **想定工程支持**：想定模板库、常用平台/传感器/武器配置的复用资产建设
- **工具链定制**：wizard 功能扩展、批量仿真脚本、结果后处理流程定制
- **界面功能增强**：wizard 可视化编辑、态势显示等 Qt 界面二次开发
