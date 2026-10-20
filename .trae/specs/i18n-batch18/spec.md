# 界面本地化（i18n 汉化）功能规格 — i18n-batch18

> 序号：batch18 ｜ 功能：界面本地化（GUI 汉化） ｜ 状态：**实现已完成，本文为补充归档**
> 应用域：wizard / warlock / mystic / mover_creator 四应用共享 `wizard_zh_CN.qm`

## 1. 背景与目标

AFSIM 界面全部为英文硬编码。本地化目标是在**不破坏协议键、持久化键、查找键**的前提下，让四应用界面显示中文。

已实现成果（截至本文归档）：

| 资产 | 数量 |
|---|---|
| `wizard_zh_CN.ts` 翻译条目 | 3952 finished / 0 unfinished / 0 警告 |
| 翻译上下文 | 140+（含 4 个伪上下文） |
| 日志消息 ID 化 | 719 条（error 19 批 + warning 第 20 批，全域清零） |
| 界面 tr()/translate 化 | 菜单/工具栏/对话框/右键/首选项/插件管理器等全量 |

## 2. 翻译通道体系（四通道）

| 通道 | 适用 | 上下文归属 | 备注 |
|---|---|---|---|
| `tr()` | QObject 子类成员函数 | 类全限定名（含命名空间） | 类缺 Q_OBJECT 时静默落父类/QObject，条目永不命中 → 死条目 |
| `QCoreApplication::translate(ctx, str)` | 非 QObject 类 / 自由函数 | 显式指定 ctx | 上下文要与成员 tr() 实际落点一致 |
| `.ui` 文件静态文本 | uic 生成代码 | uic class 名（`.ui` 的 class 属性） | lupdate 需扫 .ui 才能提取 |
| 伪上下文 translate | 数据键显示分离 | 自定义名（Category/MapName/PluginName/PluginDescription） | 键保持英文，未登记译名原样显示 |

Qt 基础控件（QDialogButtonBox 的 OK/取消等）走 `qtbase_zh_CN.qm`（Qt 自带），需部署到 `build/Release/i18n/`。

## 3. 关键范式（历史踩坑沉淀）

### 3.1 键/显示分离（最高优先级红线）

凡字符串被 **持久化键 / 查找键 / 协议键** 使用，翻译后必须双通道分离：

- 菜单标题 "Tools"：GetMenuByName 按 title 匹配 → 改 objectName 查找键，显示走翻译
- 地图名（QSettings activeMap/ProfileMap、std::map 键）：组合框 addItem 显示中文、itemData 存英文键，currentData()/findData() 读写
- 插件名（autostart 配置键）：列表显示 translate("PluginName")，键通道 GetPluginKey() 从 uid 反查英文原名
- 想定文件 token、脚本变量名、`<Choose Platform>` 等 currentText() 被比较的占位符 → **一律不译**
- 文本比较路由（`currentText()=="..."`、`sender->text()=="..."`）遇翻译必失配 → 改 objectName/currentIndex()/data 通道

### 3.2 死条目与上下文

- 类无 Q_OBJECT → tr() 落基类上下文（如 wkf::Plugin），ts 按子类名登记的条目永不命中
- 修复二选一：补 Q_OBJECT（QDialog 子类必须）或改 `QCoreApplication::translate("完整类名", ...)`
- uic 静态与运行时覆盖（setItemText/setText 运行时改写）必须**两个上下文都登记**（DemoBrowser 教训）
- 自由函数调 tr() 报 C3861 → 必须显式 translate

### 3.3 伪上下文范式

- `Category`（首选项分类标签）：建树点动态 translate，键存 QTreeWidgetItem data(UserRole+2)
- `MapName`（地图名）：文件级 MapDisplayName(key) 伪上下文
- `PluginName` / `PluginDescription`（插件管理器）：列表名与描述显示点统一 translate，键通道走 uid 反查
- lupdate 不提取伪上下文，条目须手工维护

### 3.4 文件编码与工具链（硬约束）

- 含中文（非 ASCII）的 .cpp/.hpp 必须 **UTF-8 with BOM（单层）**——多层 BOM 叠加 → MSVC C4819/C2144/C4430
- **BOM 修复必须"循环剥离所有前导 BOM 只留一层"**，不能盲目追加；每批次收尾全库字节级扫描复验
- 编辑器/工具保存可能再次叠加 BOM → 每次编辑后复查
- lrelease 生成 qm 部署 `build/Release/i18n/`；lrelease 告警源=同上下文重复 source（须先 grep 去重）
- ts 批量写回**严禁用 [System.Xml] 序列化**（双 XML 声明 + DOCTYPE 变形 + 全文重排），必须字符串操作

## 4. 模块覆盖清单（已实现）

- 主窗口八大菜单 + 工具栏 + 启动对话框（wizard）
- 编辑器交互链（标签页右键/项目树/命令链/错误列表/Open Recent）
- 首选项对话框（分类标签/标准按钮/运行时控件标签/跳转定位）
- 三维视图右键菜单（BuildEntityContextMenu 34 实现点 + BuildAttachmentContextMenu）
- 插件管理器（143 插件名 + 描述 + autostart 键分离）
- 演示浏览器（工具箱标签/字段标签/匹配文件）
- 地形工具链（TerrainToolsPlugin 菜单与状态栏 / LOS_Ruler 右键与对话框 / WkfViewer 橡皮筋后缀 / PlatformMovement 提示）
- 地图定义页（地图名 + Mil-Std 树）
- Mystic 日志面板（wkf::log::SetChineseDisplay + 消息 ID 双通道）
- 消息 ID 化目录：20 批 error/warning 全域清零

## 5. 不译清单（协议/数据边界）

- 想定语法关键字、事件 token、脚本变量名、XIO/DIS 协议串
- QSettings 持久化键、std::map 键、autostart 配置键
- 查找/比较占位符、演示卡片数据内容（Unclassified 等）
- 专名插件（CommVis/SPLAT/Engage/Astrolabe 等）保留原文，translate 未命中原样显示

## 6. 验证与收尾标准

1. lrelease：0 警告 / 0 unfinished
2. qm 部署到 build/Release/i18n/（四应用共享）
3. 编译四应用 exit 0，无 C4819/C2144（BOM 全库扫描 0 多层）
4. 冒烟：wizard/warlock/mystic 启动存活
5. 记账三件套：doc 修改日志 / SKILL.md 口径 / project_memory.md
