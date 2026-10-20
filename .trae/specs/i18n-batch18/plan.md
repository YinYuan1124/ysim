# 界面本地化实施计划（i18n-batch18）

> 依赖 spec.md 的通道体系与范式；按"基础 → 主干 → 深水区 → 扫尾"顺序推进。
> **实现已全部完成**，本计划为执行过程复盘归档，供后续新批次沿用。

## 阶段一：基础设施（已完成）

- [x] 消息 ID 化双通道 API：`FormatEnTagged`/`EmitWarning`（默认参数化 `= {}`），L1 消费端 `TranslateTaggedToZh` 按 `[E:ID]/[W:ID]` 前缀反提取参数
- [x] 日志面板中文显示：`wkf::log::SetChineseDisplay(true)`（mystic 正确引入 WkfLogModel.hpp）
- [x] 四应用翻译加载注入：QTranslator + wizard_zh_CN.qm + qtbase_zh_CN.qm 部署 build/Release/i18n/
- [x] 消息 ID 化 20 批：error 19 批 + warning 第 20 批全域清零（719 条在册，catalog 341+ 处调用）

## 阶段二：主干界面（已完成）

- [x] wizard 八大主菜单 + 子元素 + 工具栏（FindOrCreateMenu 双通道，objectName 查找键）
- [x] 编辑器交互链：标签页右键/项目树右键/命令链/错误列表标题/Open Recent/打开项目对话框
- [x] 四对话框（Preferences/PluginManager/About/Documentation）补 Q_OBJECT
- [x] 三维视图右键菜单族：BuildEntityContextMenu 34 实现点 + BuildAttachmentContextMenu 3 处

## 阶段三：深水区（已完成）

- [x] 首选项对话框：36 个 PrefWidget 分类标签（Category 伪上下文）+ 运行时控件标签 8 类通道清零 + 标准按钮 + 跳转定位（data 键比较）
- [x] 地图定义页：8 个内置地图名 + Mil-Std 树（MapName 伪上下文，键/显示分离）
- [x] 插件管理器：PluginName/PluginDescription 伪上下文 231 条 + autostart 键分离（GetPluginKey 反查）
- [x] Designer/Mystic Rv* 系列、wizard 目录 151 上下文全量翻译

## 阶段四：扫尾与残留（已完成）

- [x] DemoBrowser：工具箱标签（uic+运行时双上下文登记）+ 字段标签 + Matched Files
- [x] 地形工具链：TerrainToolsPlugin 菜单/状态栏 + LOS_Ruler 7 处（VaAttachmentQt 显式 translate）+ WkfViewer 橡皮筋后缀 + PlatformMovement 提示
- [x] lupdate 缺口过滤脚本（Unescape 最后解码 &amp; + wkf::Plugin 兜底），全域缺口清零
- [x] ts 重复条目清零（同上下文 source 去重）
- [x] 多层 BOM 全库扫描修复（循环剥离只留单层 + 字节级复验）

## 后续批次注意事项（计划指引）

1. 每个新功能模块：先 grep 全库同族函数（菜单/右键/对话框常有多实现点），再统一改造
2. 改造前必须确认字符串是否同时被 strcmp/currentText/find/setItemText 等键逻辑使用
3. 改完先编译暴露 C3861/C2660（自由函数 tr/API 签名），再 lrelease 验证上下文命中
4. 含中文文件补 BOM 后编辑再复核；批次收尾跑全库 BOM 扫描
5. 验收：lrelease 0 警告 + 四应用编译 exit 0 + 冒烟存活 + 记账三件套
