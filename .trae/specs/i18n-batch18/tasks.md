# 界面本地化执行任务清单（i18n-batch18）

> 依赖 spec.md §2/§3 与 plan.md 阶段划分。**全部已完成**，勾选状态为归档记录。

## 任务 1：基础设施
- [x] 消息 ID 化双通道 API（FormatEnTagged/EmitWarning 默认参数化）
- [x] L1 消费端 TranslateTaggedToZh（[E:ID]/[W:ID] 前缀反提取 + 尾字面量空串特判）
- [x] 日志面板中文显示（mystic/warlock 注入 wkf::log::SetChineseDisplay）
- [x] 四应用 QTranslator 加载 + qm 部署 build/Release/i18n/（wizard_zh_CN.qm + qtbase_zh_CN.qm）
- [x] 消息 ID 化 20 批（error 19 批 + warning 第 20 批，719 条在册全域清零）

## 任务 2：主干界面汉化
- [x] wizard 八大主菜单 + 工具栏（双通道、objectName 查找键、View 子菜单排序按 objectName）
- [x] 编辑器交互链 55 条（标签页右键 14+6 / 项目树 / 命令链 / 错误列表 / Open Recent）
- [x] 四对话框 + ActionManager/SimulationManager 补 Q_OBJECT
- [x] 三维视图右键菜单族（34+3 实现点统一 tr() 化）
- [x] Tools 菜单 11 个插件入口（统一落 wkf::Plugin 基类上下文）

## 任务 3：深水区
- [x] 首选项 36 分类（Category 伪上下文 + data 键）+ 运行时控件 8 类通道清零
- [x] 地图定义页 8 地图名 + Mil-Std 树（MapName 键/显示分离）
- [x] 插件管理器 PluginName 108 条 + PluginDescription 123 条 + autostart 键分离（GetPluginKey）
- [x] wizard 目录 151 上下文 / 1395 条全量
- [x] Designer 与 Mystic Rv* 系列 651 条

## 任务 4：扫尾
- [x] DemoBrowser 双上下文登记 + 字段标签 + Matched Files
- [x] TerrainToolsPlugin/LOS_Ruler/WkfViewer/PlatformMovement 视线链 22 条（显式 translate 通道）
- [x] ts 重复条目清零（同上下文 source 去重 4+ 条）
- [x] 多层 BOM 全库扫描修复（7 文件循环剥离 + 字节级复验）
- [x] lrelease 0 警告验证 + qm 部署

## 任务 5：记账
- [x] doc/2026-10-17修改日志.md 当日归档
- [x] SKILL.md / project_memory.md 累计口径同步（lrelease 3952 finished）

## 验收标准
- lrelease：3952 finished / 0 unfinished / 0 警告
- 四应用编译 exit 0，无 C4819/C2144
- wizard/warlock/mystic 冒烟存活
- 伪上下文（Category/MapName/PluginName/PluginDescription）条目在册，键通道保持英文
- 全域 lupdate 缺口与硬编码残留清零（wkf_batch_residual.txt 已确认为假缺口）
