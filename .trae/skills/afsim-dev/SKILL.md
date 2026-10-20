---
name: afsim-dev
description: AFSIM C++/OSG/osgEarth/Qt secondary development guide — source map, extension patterns, build and debug workflow. Use when modifying AFSIM source, adding plugins sensors or movers, or troubleshooting build DLL and debugger issues.
---

# AFSIM 二次开发指南

## 项目描述

AFSIM (Advanced Framework for Simulation, Integration, and Modeling) v2.9.0，Boeing 开发的分布式作战仿真框架，CUI 限制。本仓库 `e:\afsim\ysim` 用于对 AFSIM 源码做二次开发。

- **语言/标准**: C++11（MSVC v144 工具集下按 C++14 编译）
- **构建**: CMake + Visual Studio 2022 x64 生成器，源码根 `src/`，输出 `build/`
- **技术栈**: C++ / OSG 3.6.3 / osgEarth 2.10.1 / Qt 5.12.11 / OpenGL / GDAL 3.3.2 / GEOS 3.5.1 / PROJ 8.1.1 / ZMQ / XIO
- **形态**: 200+ CMake 子模块，运行时 DLL 插件化加载（`wsf_plugins/`、`mission_plugins/`）
- **GUI**: wizard.exe（场景编辑器，Qt + OSG/osgEarth）、warlock/mystic（仿真执行与分析）

## 源码地图（改代码先看这里）

| 路径 | 内容 | 二次开发相关性 |
|------|------|---------------|
| `src/core/wsf/` | 仿真核心引擎：Scenario、Platform、事件调度、脚本 | 最高 — 所有扩展的基座 |
| `src/core/wsf_mil/` | 军事域：雷达传感器、武器、mover、跟踪 | 高 — 常改这里 |
| `src/core/wsf_space/` | 空间域：轨道、星历 | 按需 |
| `src/core/sensor_plot_lib/` | 传感器覆盖图计算与绘制 | 高 — 战场可视化核心 |
| `src/core/wkf/`、`wkf_common` | 通用 GUI 框架（Qt 基础组件、地图窗口） | 写 GUI 插件必读 |
| `src/core/util/` | 基础库 Ut*（UtInput 脚本解析、UtEntity 地理） | 全项目地基 |
| `src/wizard/` | 场景编辑器 GUI（lib=wizard_core + main=wizard.exe） | 改编辑器 |
| `src/wsf_plugins/` | 运行时插件（six_dof、p6dof、oms_uci 等） | **加新插件的标准位置** |
| `src/mission/`、`src/engage/`、`src/mystic/` | 仿真执行、交战、结果分析工具链 | 按需 |
| `src/core/dis/`、`src/core/xio/` | DIS 分布式仿真、分布式 IO | 联邦集成 |
| `src/vespatk/` | CMake 构建辅助与通用工具库 | 改构建时读 |
| `*/test/`（各模块下） | GTest 单元测试，编译后为 `build/<Config>/*_test.exe`（core_test、mil_test、six_dof_test 等 25 个） | 改动后的回归验证 |

## 核心架构概念（30 秒版）

1. **WsfScenario** — 全局单例上下文，持有所有类型注册表
2. **WsfPlatform** — 仿真实体（飞机/舰船/卫星），组件容器
3. **事件驱动** — 离散事件调度器推进仿真时间（非实时循环）
4. **工厂 + Types 注册表** — 传感器/处理器/mover 等按名字符串注册，如 `WsfSensorTypes::Get(aScenario)`
5. **UtInput 脚本** — 想定文件是命令式语言，由 UtInput 逐行解析
6. **运行时插件** — DLL 放 `build/<Config>/wsf_plugins/` 自动加载
7. **PrefObject/PrefWidget** — wkf 偏好体系：`wkf::PrefObjectT<Data>`（QSettings 持久化）+ `wkf::PrefWidgetT<PrefObject>`（Preferences 对话框页面），新页面在 `wizard::PreferencesDialog`（`src/wizard/lib/source/gui/PreferencesDialog.hpp`）注册到 `mStandardWidgets`

## 二次开发标准流程

新增一个功能（传感器/处理器/插件）：

1. 参考 `src/core/wsf/source/Wsf*.hpp` 中同类型组件的接口（如 `WsfSensor`）
2. 在对应 Types 工厂中注册（`WsfSensorTypes::Add` 模式）
3. 新 CMake 模块用 `wsf_module` 惯例（参考 `src/wsf_plugins/` 任一插件的 CMakeLists.txt）
4. 编译: `cmake --build E:/afsim/ysim/build --config Release --target <目标> -- /m`
5. DLL 落到 `build/Release/`；若为运行时插件，确认被拷到 `wsf_plugins/` 子目录

## 构建

```powershell
# 配置（已配置过，无需重复）
cmake -G "Visual Studio 17 2022" -A x64 -B E:/afsim/ysim/build  # 在 src/ 下执行

# 增量编译单目标（日常）
cmake --build E:/afsim/ysim/build --config Release --target wizard -- /m
cmake --build E:/afsim/ysim/build --config Debug   --target wizard -- /m

# 全量
cmake --build E:/afsim/ysim/build --config Release --target ALL_BUILD -- /m
```

- Debug 产物带 `_d` 后缀（`wsf_d.dll`、`wizard_core_d.dll`），PDB 同目录
- 第三方依赖由 `dependencies/3rd_party/*.tar.gz` 自动解压到 `build/3rd_party/`
- 改动验证：编译对应模块的 `*_test` 目标跑单元测试，GUI 改动启动 wizard 肉眼验证

## 调试

- **TRAE 内 F5 不可用** — `cppvsdbg` 有微软 license 校验，拒绝非官方 VS Code
- **官方 VS Code**: `code e:\afsim\ysim` 后 F5，`.vscode/launch.json` 双配置现成（Debug 构建 → 断点单步；Release 构建 → 直接运行）
- **VS2022**: 打开 `build/afsim.sln`，wizard 设启动项目，Debug+x64，F5
- 调试环境布局: `build/Debug/` 与 `build/Release/` 各自独立完整（qt.conf + qt_plugins + 全部 DLL），**cwd 必须是对应配置目录**

## 已知坑（已踩过，勿重复）

| 坑 | 症状 | 修法 |
|----|------|------|
| 旧代码缺 `<chrono>` 头 | `WsfThreadPool.hpp` 报 `system_clock 不是 std::chrono 成员` | 补 `#include <chrono>`（VS2022 比 VS2017 严格） |
| `resetiosflags` 缺 `std::` | `HorizontalMapFunction.cpp` C3861/C2593 | 改 `std::resetiosflags(...)` |
| 间接依赖 DLL 缺失 | exe 秒退，exit code `0xC0000139` (DLL_NOT_FOUND) | 用 dumpbin 递归扫描依赖树；`geos.dll`/`tiffd.dll` 在 `build/3rd_party/<pkg>/(debug|release)/bin` |
| Qt 插件加载失败 | GUI 秒退无输出 | `qt_plugins/`（含 `platforms/qwindows.dll`）+ `qt.conf`（`Prefix=.`）必须在 exe 同目录 |
| cmake --install 中断 | 找不到 `engage.exe` 等未编译目标 | 先 `--target <缺失目标>` 编译再 install |
| GEOS debug/release 同名 | Debug 里误放 release 版 geos.dll | Debug 目录必须用 `3rd_party/geos/debug/bin/geos.dll` |
| 新增 .ui/.cpp 后编译报 C1083 找不到 ui_*.h | wizard lib 用 `file(GLOB)` 收集源文件，配置期扫描 | 增删文件后必须重新 configure：`cmake -B E:/afsim/ysim/build -S E:/afsim/ysim/src` |
| PowerShell 处理 .ts 翻译 XML | 空 `<translation>` 元素被 XML 适配器折叠成字符串属性，InnerText 赋值静默失败 | 逐行文本处理或正则替换，不要用 `[xml]` + 属性赋值 |
| **含中文注释的源文件无 UTF-8 BOM** | MSVC 按代码页 936(GBK) 解析 UTF-8 注释，双字节序列吞掉换行/代码字符 → C2447/C2065 等离奇语法错误，行号对不上实际内容 | **任何含非 ASCII 的 .cpp/.hpp 必须存为 UTF-8 with BOM**（`[IO.File]::WriteAllText($f, $text, [Text.UTF8Encoding]::new($true))`）；只有纯 ASCII 文件才能无 BOM |
| **`fromUtf8("中文")` 窄字面量乱码** | 源文件带 BOM 也无用：MSVC 窄字符串字面量的**执行编码**默认是系统代码页(GBK)，obj 里存 GBK 字节，`fromUtf8` 按 UTF-8 解析 GBK → 运行时乱码 | 含中文的 QString 字面量一律用 `QStringLiteral("中文")`（展开为 UTF-16 宽字面量，全程 Unicode 不经代码页转换）；需运行时拼接的用 `QString::fromWCharArray(L"中文")` |
| 缺 Q_OBJECT 的类里用 tr() | 翻译永不命中（lupdate 警告 lacks Q_OBJECT；运行时 tr() 用基类 context，与 ts 中该类 context 不匹配） | 给类补 `Q_OBJECT` 宏（AUTOMOC 自动处理），重跑 lupdate |
| MSBuild 用陈旧源文件编译 | 改完文件编译报的行号/内容与磁盘不符（增量构建时间戳判断失效） | touch 改动文件（`(Get-Item $f).LastWriteTime = Get-Date`）后重编 |

完整构建排障记录见 [references/build-debug.md](references/build-debug.md)。

## i18n 汉化（已建基础设施）

wizard 中英切换功能已实现（P0 骨架，设计文档见对话）：

- **入口**: wizard → Options → Preferences → Application 分类 → Language 页（`LanguagePrefWidget`）
- **存储**: `%APPDATA%\United States Air Force (USAF)\AFSIM-Wizard\Settings.ini` 的 `[LanguageOptions] language`（`en`/`zh_CN`），重启生效
- **代码**: `src/wizard/lib/source/gui/LanguagePref{Object,Widget}.{hpp,cpp}` + `src/wizard/lib/ui/LanguagePrefWidget.ui`；translator 加载在 `Runner::run()`（`src/wizard/lib/source/application/Runner.cpp`）
- **翻译资产**: `src/wizard/lib/i18n/wizard_zh_CN.ts`（入库，**455 条全量翻译**：功能页/Preferences/General/主菜单栏八大菜单及子元素/编辑器右键菜单/项目浏览器右键/命令链右键/错误列表标题/最近文件/打开项目链对话框/启动对话框/About/克隆对话框/开发者计时器/更改历史/宏帮助/查找替换/单位页/阵营可见性/插件管理器等 58 个上下文）；qm 运行时在 `build/Release/i18n/`（wizard_zh_CN.qm + qtbase_zh_CN.qm）
- **P1 进展**: 第一批 tr() 改造已完成 — StartupDialog(含补 Q_OBJECT)、ContextMenuActions(5 个右键动作)、CreatePlatformDialog、ClonePlatformDialog、DeveloperActions(计时器参数化)、AutoCompletePopup、ContextDocumentationControl(含补 Q_OBJECT)、FindResultsControl；动态拼接已按决策 4 参数化为 `tr("... %1").arg(...)`。**L4 翻译补齐已完成：310 条全量翻译（2026-10-15 补齐 218 条 unfinished 清零，.ui 静态文本与低频界面全覆盖，lrelease 310 finished/0 unfinished）**。**主菜单栏全量汉化（2026-10-16）：File/Edit/View/Options/Tools/Project/Run/Help 八大菜单及子元素 90 条新上下文条目（wizard_zh_CN.ts 310→400 条，lrelease 394 finished）** — FindOrCreateMenu/FindOrCreateViewSubMenu 动态菜单标题 translate("WkfMainWindowUi")+objectName 查找键分离（View 子菜单排序改 objectName 比较）；Preferences/PluginManager/About/Documentation 四对话框与 ActionManager/SimulationManager Plugin 补 Q_OBJECT（tr 上下文命中前提）；Tools 菜单 11 插件入口无需逐个补 Q_OBJECT，tr() 统一落基类 `wkf::Plugin` 上下文；Run 菜单 14 动作落 `SimulationManager::Plugin`。**编辑器交互链汉化（2026-10-16 二批）**：编辑器标签页右键 14+6 处、项目树右键、命令链右键、错误列表标题、Open Recent、打开项目链对话框共 55 条（ts 400→455，lrelease 448 finished）—— **死条目教训**：类缺 Q_OBJECT 时 tr() 实际落父类上下文（QObject/QPlainTextEdit），ts 按类名登记的条目永不命中（DeveloperActions 计时器 4 条、WsfEditor 2 条、Editor 1 条均属此类），修复二选一：补 Q_OBJECT（QObject/QDockWidget 直接子类安全）或改 `QCoreApplication::translate("完整类名", ...)` 显式上下文（非 QObject 类如 PasteContext、无宏 widget 类如 WsfEditor/Editor 一律走此路）；**FileOpen 路由陷阱**：`sender->text() == "..."` 文本比较查找遇翻译必失配，改 objectName 查找键（Environment::FileOpen 先例）；UndoContext 撤销描述串也应走翻译通道（PasteContext 先例）；重构拼接变量改名须全局搜引用（PasteContext defaultOption 遗漏首轮编译失败教训）。注意：ts 填充脚本须用 pwsh 7+ 执行（PS 5.1 按 ANSI 读无 BOM UTF-8 会乱码）
- **L2 消息层 MsgCatalog（决策 1 落地）**: `src/tools/util/source/UtMsgCatalog.hpp/.cpp` — 消息 ID 分段常量（0x0001xxxx 想定解析 / 0x0002xxxx 平台运动 / 0x0003 传感器 / 0x0004 交战 / 0x0005 通信 / 0x0006xxxx 工具UI）、编译期内置目录表（Entry{en/zh/cause/fix}）、`{参数}` 占位符引擎、双通道 API（`Emit/EmitWarning` + `FormatEnTagged` 落日志英文+`[E:ID]` 前缀；`FormatZh` 界面中文；`GetCauseZh/GetFixZh` 诊断说明）。**已接入 481 处调用点（目录 437 条）**，分十九批：①内核 14 处 — WsfSimulation(5 平台增删)、WsfPlatform(3 组件初始化)、WsfCallback(1)、WsfStandardApplication(1)、WsfDraw(2)、event_pipe(2 消息ID冲突)；②sensor/comm 域 17 处 — WsfCommNetworkManager(8：路由器不存在/分配网络失败/地址分配失败/外部链路缺平台缺设备缺地址)、WsfCommMedium(2：模式未定义/默认模式名无效)、WsfCommMediumGuided+Unguided(4：模式不存在/重复发送消息，共用 ID)、WsfMTI_TableManager(3：表文件打不开/非单调递增/行解析失败)；③交战/武器域 22 处 — WsfWeapon(1：weapon_effects 未定义)、WsfWeaponPlatformExtension(1：交战初始化失败)、WsfExplicitWeapon(4：发射平台类型未定义/发射解缺失/延迟发射初始化失败/武器入仿真失败)、WsfLaunchComputer(6：空武器指针/三选二约束/负推力或滑行时长/燃尽速度或最小末速缺失)、WsfATA_LaunchComputer(10：5 个分箱数下限/方位角限幅/3 组上下限颠倒/Range 非递增)；④通信段补全 10 处（0x0005 段闭段，comm 目录 error 全覆盖）— WsfCommNetworkManager(4：默认网络满/入网参数过多/缺 router 关联/平台缺 router)、WsfCommMediumTypes(1：介质类型不允许)、WsfCommProtocolOSPF(3：缺 area/区域未连骨干/backbone 不一致，地址需 ostringstream)、WsfCommRouter(2：发送源地址为空/自动链路失败)；⑤mover/processor 域 30 处（0x0002 段续接 0x0002000A-25）— WsfTaskManager(10：track processor 类型/找不到、资源类型未知×2、按地址找 comm、发消息三连败、任务分配内部错、取消任务未找到)、WsfAero2D(7：全部 Initialize 气动配置校验)、WsfRoadMover(6：路线/路径各两处共用 ID+起终点成对+缺 speed)、WsfExchangeProcessor(4)、WsfMessageProcessor(3)；⑥sensor 域核心 25 处（0x0003 段续接 0x00030004-15）— WsfPassiveSensor(19：频带校验+PSOS 归还/添加各按模式分 ID+dwell/revisit 约束两 ID 多点共用)、WsfRadarSensor(4：clutter_model 三连)、WsfSensorMode(2：on_success/on_failure 模式名)。WsfSensorBeam 3 处"please implement"虚函数缺省警告不接入（非想定报错）；⑦发射计算机双雄 19 处（0x0004 段续接 0x00040017-27）— WsfSAM_LaunchComputer(9：四维表数组×元素数/单调性两类校验各独立 ID+TOF 插值失败)、WsfBallisticMissileLaunchComputer(10：缺表/目标过期/输入错误 3 处共用/打开失败/地面距离/TOF 单调性/空弹道/无弹道)；⑧lethality/激光系列 15 处（0x0004 段续接 0x00040028-35）— Tabulated(2：CEP/Pk 缺失+Pk 不随 CEP 递减)、HEL_Lethality(2：条目无效应+表行数)、Graduated(2：半径/Pk 缺失+Pk 不随脱靶量递减)、HighEnergyLaser(3：laser_type+相交测试两处共用)、MobilityAndFirepower(3：MFK 概率和+PkFactor/Table 互斥+表未找到)、Spherical(3：半径/伤害/阈值上下限)；⑨武器域散点收尾 9 处（0x0004 段续接 0x00040036-3E，weapon 目录 error 级清零）— LaunchHandoffData(1：无法确定发射平台)、ATG_LaunchComputer(2：无 LAR+父武器缺 update_interval)、AirToAirLaunchComputer(1：拦截表文件解析失败)、CuedLaserWeapon(1：找不到波束导向器)、OrbitalLaunchComputer(3：文件打开失败读/写分 ID+行读取失败)、ATG_WeaponLAR(1：未定义 use_zone)。注意：武器代码在 `src/core/wsf_mil/source/weapon/`（wsf_mil 库），不在 wsf 库；⑩wsf_mil sensor 域 28 处（0x0003 段续接 0x00030016-31）— BeamDirector(1：模式缺 type)、BistaticErrorModel(3：方位/俯仰标准差超限+domain 无法识别)、LaserDesignator(2：缺 laser_designations 扩展+传感器类型错)、LaserTracker(1：缺扩展)、SurfaceWaveRadarSensorErrorModel(1：仅限表面波雷达)、OTH_RadarSensorErrorModel(1：仅限超视距雷达)、EOIR_Sensor(3：frame_time/角分辨率或像素数/分辨率过小)、SAR_Sensor(3：resolution 与 dwell_time 互斥+误差统计未初始化)、LADAR_Sensor(2：发射机缺 PRF+缺检测阈值)、EOIR_IRST_SensorMode(4：缺频带/NEI/阈值/衰减模型失败)、OTH_RadarSensor(7：clutter_model 克隆/未定义/初始化失败+mti_processor 缺 PRF/最大距离/脉间周期+MTI 表重复)；跳过 CTD 指针一致性 4 处与 Bistatic 运行期防御 8 处（非想定配置错）；⑪processor 域 34 处（0x0002 段续接 0x00020026-46，processor 目录 error 级闭段）— GuidanceComputer(13：无 phase/initial_phase 无效/阶段与程序初始化失败/程序引用缺失/方位偏移方向/guidance_target/比例导引方法/脚本阶段名/切换变量/请求阶段不存在/next_phase 未定义/瞄准点偏移须成对)、GuidanceProgram(3：仅限 WSF_GUIDED_MOVER+飞行路径角未定义两处共用)、PerceptionProcessor(4：资产与威胁评分函数脚本缺失/签名不符)、WeaponFuse(3：空对地模式与 detonate_above_height_agl 冲突×2+交战对象创建失败)、WeaponThreatProcessor(2：classifier 类型错+pkCalculator 缺失)、Quantum 三件套 Allocator/Generator/Evaluator(各 2：脚本缺失+签名无效)、UplinkProcessor(1：uplink_path 引用未知 sensor/comm)、WeaponTrackProcessor(1：传感器不存在)、QuantumTaskerProcessor(1：找不到要拒绝的任务)；跳过 8 处内部防御错误（FusionCenter/ClassificationProcessor 断言、WeaponThreatProcessor ThrowLocal 与运行期候选异常、QuantumTaskerProcessor internal error）；⑫wsf mover 域散点 27 处（0x0002 段续接 0x00020047-5D，wsf mover 目录 error 级闭段）— HybridMover(3：缺 follower_mover/waypoint_mover/current_mover 类型)、Aero(2：初始化未关联平台+参考面积为零)、OffsetMover(1：缺参考平台)、PathFinder(3：找不到有效点两处共用+初始化失败)、RouteNetwork(1：源/目的节点间无关联路线)、TabularRateFuel(4：缺表/当前模式无表+No fuel_table 两处共用)、VariableRateFuel(4：缺表/无高度层/模式无表×2)、TankedFuel(4：缺最大受油率+无法供油三处共用)、TSPI_Mover(1：TSPI 文件打不开)、ShortestPath(4：代价矩阵空/非方阵/不对称/节点索引越界)；跳过 3 处（NavigationErrors INS 误差调试报告、PathGuidance 未挂接防御、RouteNetwork InsertArea 内部防御）；⑬wsf_mil mover 域 33 处（0x0002 段续接 0x0002005E-7D，wsf_mil mover 目录 error 级闭段）— GuidedMover(17：阶段初始化失败与 TBM 共用 ID+divert 双参数+nozzle_exit_area 不一致+比冲计算×2+比值不等+nozzle 须指定+质量/推进三量确定与不一致×5+燃料超耗+aero 类型未识别（核心库首个参数化 {aeroType} 调用点）+aero 须指定)、TBM_Mover(12：阶段共用+cep 冲突+弹道不可达+迭代超限+三段飞行时间+质量与燃料参数校验×7)、StraightLineMover(3：轨迹类型缺失/重复+弹射速度为零)、TowedMover(1：找不到交战对象)；跳过 3 处 abort 前防御（GuidedMoverBase/GuidedMover stage index 越界、TBM rkqc step）；⑭dis 域 5 处（0x0004 段续接 0x0004003F-43，dis 目录 error 级闭段）— WsfDisWeaponEvents 武器转移 5 连：无法克隆武器类型/无法添加/无法初始化（带 Weapon System Type 附注）+本地发射失败/发射后交战指针为空（无附注）；⑮ew 电子战域 15 处（0x0004 段续接 0x00040044-50，ew 目录 error 级闭段）— EW_SensorComponent(2：筛选器+干扰方位探测器初始化失败)、FalseTarget(4：分布行为组合冲突+缺参考质心+纬度/经度输入不完整)、SLC_Effect(4：缺天线方向图与 SLB 共用 ID+cancelation_ratios 两处共用+对消器数目缺表)、FalseTargetEffect(2：假目标初始化失败+定义未找到)、JamStrobeDetector(1：不报告任何量测)、EW_Effect(1：基础效果数据无效)、SLB_Effect(1：缺天线方向图共用)；跳过 7 处内部防御（SensorComponent 4 处 mode/beam 指针 mismatch、EW_EA_EP 2 处基类虚函数 base call error、EW_Technique 1 处 Coding Violation）；⑯DIS 协议库 11 处（0x0005 通信段续接 0x00050016-20，tools/dis 库 error 级闭段）— Dis(6：DIS_VERSION/DIS_EXERCISE/DIS_SITE/DIS_APPLICATION 超范围+DIS_HRT_BEAT_TIMER/DIS_HRT_BEAT_MPLIER 为负，均带 Value 附注)、DisPduFactory(1：PDU 类型读取失败)、DisPdu(1：数据尺寸不匹配)、DisSignal(3：长度非 32 位边界+数据库索引长度须 96+数据长度与头部位数不符)；跳过 9 处（DisRadioId 4 处+DisIO 2 处 ShowError(cIO_ERRORS) 门控 IO 防御、DisPduFactory 2 处诊断转储句、DisSignal 1 处 assert）；⑰DIS 协议库流 IO 收尾 9 处（0x0005 段续接 0x00050021-29，tools/dis 库 error 级清零；第十六批跳过项按用户要求全部纳入）— DisIO(2：DisRadioId 流提取失败/not good)、DisRadioId(4：Pack/UnPack 各 failed+not good)、DisSignal(1：Copy 无效数据指针)、DisPduFactory LogError(2：Last good PDU Header was:（保留 auto out = 供转储追加）+first PDU)；⑱wsf 库零散目录 18 处（0x0002 段续接 0x0002007E-81 / 0x0003 段 0x00030032 / 0x0005 段续接 0x0005002A-35；wsf 库 dis/ext/event_pipe/xio/xio_sim 目录 error 级闭段，第十二批跳过的 mover 4 处一并全清）— mover 补遗(4：NavigationErrors INS 最终误差报告引出句（保留 auto out = 供 5 个附注追加）+PathFinder FindClosestValidPoint 无有效点+PathGuidance 未挂接运动体+RouteNetwork InsertArea 内部防御)、ext(1：ExtEmission 外部传感器初始化失败)、wsf dis 接口(8：ActionResponse/ActionResponseR 非加入演习请求各保留 auto out =+DisInterface 设备打开失败两处共用 ID+外观脚本变量+外观更新执行+DisPlaybackDevice 回放文件打不开/文件过多)、event_pipe(1：EventPipeInterface 输出文件打开失败)、xio(1：Subscription 无关联连接)、xio_sim(3：AutoDisMapping 消息无翻译器+DisExtension 自动 DIS 映射初始化失败+CommComponent 无法经 XIO 发送 WsfMessage)；script(42)/sensor(56)/traffic(46) 大目录已于第十九批闭段。⑲script/sensor/traffic 三大目录 140 处（0x0001 段续接 0x00010004-1D / 0x0002 段续接 0x00020082-AE / 0x0003 段续接 0x00030033-63，wsf 库全部目录 error 级闭段；跳过 SensorBeam 3 处 please implement 虚函数缺省警告与 MTI_TableManager:39 注释代码）— script 42 处：Callback(1)+Context(9)+MessageHandler(1)+ObjectClass(1)+Observer(2)+PlatformClass(12：五类设备查找各 2 处共用)+ProcessorClass(1)+RouteClass(2)+SimulationClass(1)+StateMachine(10)+StateMachineProcessor(1)；sensor 52 处：CompositeSensor(7)+MarcumSwerling(3)+PassiveDetector(9：Pulse 无 d 变体独立 ID)+RadarMTI_AdjSignalProcessor(5)+MTI_TableManager(13)+DefaultSensorTracker(1)+DefaultSensorScheduler(3)+PhysicalScanSensorScheduler(3：mTypeName 参数化)+RadarSensorErrorModel(1)+SectorScanSensorScheduler(3)+SpinSensorScheduler(2：传感器/模式名参数化)+SensorSchedulerTypes(1)+SensorSignalProcessor(1)；traffic 46 处：XWsfAirTraffic(10)+XWsfRouteNetwork(2)+XWsfSeaTraffic(14：港口不存在 2 处共用+fraction 类型名参数化)+XWsfOSM_Traffic(10)+XWsfRoadTraffic(10)。⑳warning 级全域覆盖 340 处/138 文件（wsf 229 + wsf_mil 111，两库 warning 活跃硬编码清零；warning 目录 281 条在册、全目录累计 719 条）— 新增 warning 双通道 API `FormatWEnTagged(id, args = {})/EmitWarning(id, args = {})`（后者无 AddNote 场景直接输出；**新 API 必须一步到位默认参数化**，否则单参调用点批量报 C2660）；warning 常量命名 `MSG_W_*`，ID 段位与 error 共用段空间；分三子批：A=wsf 根目录 92 处/29 文件（ZoneDefinition 28/ExternalLinks 14/AdvancedBehaviorTreeNode 13 等）；B=wsf 十组子目录 137 处/60 文件（script 10/mover 13/processor 12/sensor 7/ext 2/event_pipe 2/xio 2/comm 31/dis 46）+ catalog 0x0005 段虚构条目 56 条逐文件真实 grep 重写为 63+1 条真实常量（**侦察数据必须真实 grep 落盘 temp/recon_*.md，防上下文压缩虚构污染**）；C=wsf_mil 全库 111 处/53 文件（根签名 15/mover+processor 17/sensor+ew 49/weapon+dis+comm 30）+ 目录登记 91 条（0x0002 段 30 条 0x200ED-0x2010A/0x0003 段 8 条 0x3006B-72/0x0004 段 45 条 0x40053-7F/0x0005 段 8 条 0x50076-7D）——**EA/EP 文案变体教训**：WsfEW_EA 与 WsfEW_EP 同义消息文案有细微差异（"due to an incompatible system type" 有 an/无 an、Deselect 长短版、EA_EP 冒号变体），目录条目必须逐字比对分别登记（新增 0x4007E/0x4007F 变体），EAEP 3 处复用 EP 常量；原文错拼忠实保留 2 处（OpticalReflectivity "no defined"、MobilityAndFirepowerLethality "probablity"）；QuantumTaskerProcessor 复用 MSG_W_TASKMGR_NO_TASK 跨库复用先例。**Edit 写回会剥离文件 BOM**，改动含中文文件后必须重跑 BOM 补齐脚本，否则 MSVC 报 C2001。**警惕多重 BOM 叠加**：Edit 剥一层 + 补齐脚本加一层 + 会话间隙工具再加一层会叠成 2-3 层 BOM（症状：`sal.h C2144` + 源文件 `(1,1) C4430`），修复须循环剥离所有前导 BOM 只留一层。**参数字符串化坑**：`WsfCommAddress` 无 ToString 仅支持 `operator<<`，作模板参数须 `std::ostringstream` 转字符串且显式 `#include <sstream>`（原文件常缺此头）。**新报错点接入范式**：hpp 分配 ID 常量 → cpp 目录表登记条目（保持升序，英文原文一字不动，中文模板加 `{参数}` 占位符）→ 业务侧 `ut::log::error() << ut::msg::FormatEnTagged(ID, {{"key", val}})`，AddNote 保持英文原样。util 是 GLOB 收集，**新增文件必须重新 configure**。双通道行为已用独立测试程序验证（英文+ID/中文参数化/未注册回退 `[msg:ID]`/漏参保留占位符）
- **L1 消费端（日志界面中文显示，已落地）**: `UtMsgCatalog::TranslateTaggedToZh(text)` 解析 `[E:ID]/[W:ID]` 前缀 → 英文模板反提取参数（**尾字面量为空串时参数值取到文本末尾，`find("")` 会返回当前位置导致截断**）→ 填充中文模板；失败返回空串回退英文。`wkf::log::SetChineseDisplay(bool)`（WKF_COMMON_EXPORT，include 须带 `logging/` 前缀）控制 `WkfLogModel::MakeItem` 显示通道；wizard `Runner.cpp` zh_CN 时注入 true。落盘/控制台永远英文+ID。round-trip 单测 6 项全过
- **AFSIM 2.9 想定最小模板**（实测可用，mission.exe 验证）：`platform_type <名> WSF_PLATFORM ... end_platform_type`（基类名必须是 **WSF_PLATFORM**，无 `is` 关键字）/ `platform <名字> <类型> ... end_platform`（**先名字后类型**）/ 顶层 `end_time 10 sec`（单位带空格，`5s` 不合法）/ `event_output` 是块命令（子命令 print_xxx 等，无 end_time 子命令）。解析期已防平台重名（"Use 'edit platform'"报错），运行时重名需脚本触发：`WsfPlatform t = WsfSimulation.CreatePlatform("<类型>"); WsfSimulation.AddPlatform(t, "<名字>");`
- **二开代码注释规范**: 所有二次开发代码必须带中文注释，标记 `// 二次开发（i18n）：`；含中文的文件必须 UTF-8 with BOM（见坑表）
- **工具链**: `build/3rd_party/qt-5.12.11-x64-vs2017/bin/{lupdate,lrelease}.exe`；更新流程 = 改代码后 lupdate 重提取 → 翻译新条目 → lrelease 重生成 qm → 拷到 `build/<Config>/i18n/`
- **现状**: 全库 `tr()` 覆盖几乎为零（仅 17 处），未翻译文本自动回退英文显示；扩大汉化面需按模块渐进包裹 `tr()`，禁止在业务代码里硬编码中文
- **红线**: 想定语法关键字/类型名/枚举值/事件记录 schema 一律不翻译（机器读层）；UI 文本/报错提示/报表表头才是合法翻译对象

## 约定

- 源码是 CUI 限制的 Boeing 代码，注释保留原版声明头，不外传
- `build/`、`dependencies/` 已被 `.gitignore` 忽略，产物不进 git
- 编译修复须同步验证 Debug + Release 两个配置
