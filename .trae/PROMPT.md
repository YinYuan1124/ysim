# PROMPT — AFSIM 二次开发项目约束

> 本文件是本仓库所有 AI 辅助开发（包括 Claude/TRAE/Cursor 等）必须遵守的项目级约束。优先级高于 AI 的默认习惯。

## 项目背景

- **AFSIM v2.9.0**（Advanced Framework for Simulation, Integration and Modeling），分布式作战仿真框架
- **源码已获授权用于二次开发**，可自由修改、扩展，不受原版授权限制
- **技术栈**: C++（MSVC v144 / C++11）、Qt 5.12.11、OSG 3.6.3、osgEarth 2.10.1、OpenGL、CMake + Visual Studio 2022 x64
- 源码根 `src/`，构建树 `build/`，场景编辑器 wizard.exe（Qt + OSG/osgEarth）

## 强制约束（必须遵守）

### 1. 注释规范（最高优先级）

- **所有新增代码必须添加中文注释** — 说明"做什么、为什么"，而非复述代码
- **核心逻辑必须添加中文注释** — 包括算法思路、状态转换、Qt 信号槽链路、OSG 场景图操作、事件回调时序
- **对已有源码的每一处修改**，必须在修改处加 `// 二次开发：` 前缀的中文注释，说明修改目的
- 复杂设计（新增类/模块）在文件头或类声明前加中文块注释，说明设计意图与约束

```cpp
// 二次开发：为 xxx 功能增加 xxx 处理
// 原因：xxx 场景下会发生 xxx，不处理会导致 xxx
void WsfXxx::DoSomething()
{
   // 核心：两阶段提交，先写日志再改状态，保证崩溃后可恢复
   ...
}
```

### 2. 编码规范（防乱码/防编译错误）

- **含中文（任何非 ASCII）的 .cpp/.hpp 文件必须存为 UTF-8 with BOM** — 无 BOM 时 MSVC 按代码页 936 解析，会产生行号错乱的离奇语法错误
- **代码中构造含中文的 QString 一律用 `QStringLiteral("中文")`** — 禁止 `QString::fromUtf8("中文")`（MSVC 窄字面量执行编码是 GBK，运行时乱码）
- 面向用户显示的文本走 `tr()` 翻译通道（i18n），禁止在业务逻辑里硬编码中文界面文本

### 3. 遵循现有代码风格

- C++11 标准；3 空格缩进；成员 `m` 前缀、参数 `a` 前缀、常量 `c` 前缀
- 文件头保留原版 CUI 声明块；类声明顺序：ctor → 公有接口 → 私有槽 → 成员
- 新文件命名与所在目录现有惯例一致（Wsf*、Ut*、wkf* 前缀体系）

### 4. 架构红线（禁止修改的区域）

| 禁止 | 原因 |
|------|------|
| 想定语法关键字/类型名/枚举值/文件路径 | 解析器 token、插件反射、脚本绑定全靠字符串匹配 |
| 事件记录(.aer)/真值数据 schema 字段名 | 后处理工具链与外部脚本依赖 |
| 内核量纲体系（内部永远 SI） | 换算只允许发生在 IO 边界 |
| 运行时插件加载协议、XIO/DIS 协议格式 | 与外部仿真联邦的兼容性契约 |

### 5. 资源存放路径约束（最高优先级）

- **AI 可调用电脑上任何工具**（PowerShell、Git、MSBuild、Qt 工具链、CMake 等），但**生成的脚本、图片等中间资源必须放到 `E:\Project\ysim\temp` 路径下**
- **禁止将任何生成文件放到工作区外**（如 `C:\Users\...\AppData\Local\Temp\`、`C:\Temp\`、桌面、下载目录等）
- 例外：最终交付物（文档、报告、修改日志等）按既有规范放置；临时脚本（如 `.ps1` 批量处理脚本）统一放 `temp/`，用完即删
- 验证：每次生成文件前确认目标路径前缀为 `E:\Project\ysim\temp\`，否则拒绝写入

```powershell
# 正确：临时脚本放 temp 目录
$scriptPath = "E:\Project\ysim\temp\my_script.ps1"

# 错误：临时脚本放工作区外
$scriptPath = "C:\Users\$env:USERNAME\AppData\Local\Temp\my_script.ps1"
```

## 构建与验证

```powershell
# 编译单目标（日常）
cmake --build E:/Project/ysim/build --config Release --target wizard -- /m

# 新增/删除源文件后必须重新 configure（GLOB 是配置期扫描）
cmake -B E:/Project/ysim/build -S E:/Project/ysim/src
```

- 改动验证：编译对应 `*_test` 目标跑单元测试；GUI 改动启动 wizard 验证
- 编译修复必须同时验证 Debug + Release 两个配置
- `build/`、`dependencies/` 不入 git（.gitignore 已配置）

## SpecKit 规范（功能规格库）

- **功能规格**统一存 `.trae/specs/`，文件夹命名 = **序号 + 功能**（如 `i18n-batch18`）
- 每个功能文件夹必须包含三件套：`spec.md`（功能规格，决策完备）、`plan.md`（实施计划）、`tasks.md`（任务清单）
- 实现前先写 spec/plan；实现完成后补充归档（标注"实现已完成，补充归档"）
- 历史批次移入子目录归档不删除（如 `i18n-batch18/msgid-batch18/`）
- 具体规范见 `.trae/specs/README.md`；示例见 `.trae/specs/i18n-batch18/`（界面本地化批次）

## 详细参考

深入指南（源码地图、扩展模式、排障记录、i18n 基础设施）见项目 skill：`skills/afsim-dev/SKILL.md`（与本文件同在 `.trae` 目录下）

修改记录（日期 + 内容）见仓库根：`修改日志.md`
