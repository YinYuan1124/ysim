# 现代装备数据库工作包规格（ext-05-equipmentdb）

> 序号：ext-05 ｜ 功能：装备数据库封装 / 脚本查询 ｜ 状态：**实现已完成（2026-10-21 验收通过；同日 F 阶段补强 37/37）**
> 优先级：P2（业务价值主体，与 ext-03 模型库同方向） ｜ 依赖：无（独立插件，可选依赖 SQLite）
> 侦察完成：2026-10-21（数据库 schema 实测 + 插件/脚本机制源码确认）

## 1. 背景与目标

`build/db/DB3K_modern.db3`（51,564,544 字节，SQLite format 3）为现代装备数据库（DB3000 系 schema），
涵盖飞机/舰船/潜艇/地面单位/卫星/设施/传感器/武器/弹药/挂载等 13 类共 71,166 条装备记录，含中英文名称。

目标：在插件中封装 `EquipmentDB` 类（继承 `WsfObject`），注册到 WSF 脚本系统，
使想定脚本与 C++ 侧可按 AFSIM 习惯方式查询该数据库；零侵入 WSF 核心源码。

非目标（本批不做）：装备记录到 WSF 平台类型（platform type）的自动转换、数据库写入、
DBInfo.dat 私有格式解析（见 §2.4）。

## 2. 数据库侦察结论（2026-10-21 实测，决策依据）

### 2.1 汇总表 DataEquipmentSummary —— 主查询入口

- 行数 71,166；主键 (TableName, ID)；**已建 4 个索引**：`Name`、`EquipmentType`、
  `CountryName_CN`、`TypeName_CN`（sqlite_master 实测）
- 字段：TableName / ID / Name / **Name_CN** / TypeID / TypeName / TypeName_CN /
  CategoryID / CategoryName / CategoryName_CN / RoleID / RoleName / RoleName_CN /
  CountryID / CountryName / CountryName_CN / ServiceID / ServiceName / ServiceName_CN /
  YearCommissioned / YearDecommissioned / Length / WeightMax / DisplacementFull /
  Hypothetical / Deprecated
- 分布（TableName → 行数）：DataLoadout 32335 ｜ DataSensor 7034 ｜ DataAircraft 6933 ｜
  DataShip 4571 ｜ DataWeapon 4219 ｜ DataFacility 4054 ｜ DataPropulsion 3880 ｜
  DataMount 3863 ｜ DataMagazine 1656 ｜ DataWarhead 1307 ｜ DataSubmarine 728 ｜
  DataGroundUnit 437 ｜ DataSatellite 149

### 2.2 明细表与代码表

- 明细表 = `Data<类名>` 主表（如 DataAircraft，约 40~80 字段：尺寸/重量/乘员/传感器参数/装甲等）
  + 关联表（DataAircraftSensors / DataAircraftMounts / DataAircraftLoadouts /
  DataShipMagazines / DataWeaponWarheads / DataWeaponWRA 等，外键 ID → 主表）
- 代码表 = 70 个 `Enum<域>表`（如 EnumAircraftType 36 行、EnumSensorRole 299 行、
  EnumOperatorCountry 179 行），结构统一为 `ID INTEGER, Description TEXT`
- 样例（实测）：`DataEquipmentSummary(TableName='DataAircraft', ID=122)` →
  Name="F-16A Falcon [Netz]"，Name_CN="F-16 战隼"；
  `EnumAircraftType(2002)` → "Multirole (Fighter/Attack)"

### 2.3 SQL 安全要点

- 建表语句用单引号包裹表名（`CREATE TABLE 'DataAircraft'`）——SQLite 兼容，查询无需特殊处理
- 表名/列名来自脚本调用方 → 必须按标识符校验（仅 `[A-Za-z_][A-Za-z0-9_]*`）+ `"` 转义，防注入
- 数据值一律经 `sqlite3_bind_*` 参数化传递，禁止字符串拼接进 SQL

### 2.4 附属文件 DBInfo.dat

- 同目录 59,592 字节，base64 私有编码（头 64 字节全部为 base64 字符集），
  为 DB3K 编辑器配套文件；**SQLite 库自包含，只读查询不需要它** → 本批不解析、不依赖

### 2.5 补充侦察结论（2026-10-21 F 阶段，关联表导航与过滤字段设计依据）

- **链接表约定统一**：46 张 `Data{Parent}{Child}` 链接表全部为 `ID → ComponentID`
  （如 DataAircraftSensors(ID, ComponentNumber, ComponentID, ...)、DataLoadoutWeapons、
  DataMountSensors/DataMountWeapons 等），另有少量 `*Codes` 表为 `ID → CodeID`
  （能力代码映射，本阶段不导航）—— 故 GetLinkedIDs 固定 ID→ComponentID 约定，无需传列名
- **组件链示例**：DataAircraftSensors(ID=1305) → 14 个传感器（首 ID 490）；
  DataAircraftLoadouts(ID=1305) → 13 个挂载方案；飞机 5257 有 170 个挂载方案
  （DataAircraftLoadouts，用于 max_rows 截断实测；单挂载方案武器数 ≤ 9，
  真正的截断夹具是"平台→挂载方案"层级）；F-16（ID=122）为无组件空壳记录（变体占位）
- **RoleName 全库 NULL**（71,166 行全部为空）—— 军用角色语义实际在
  TypeName/TypeName_CN（如 F-16 → "Multirole (Fighter/Attack)"，经 EnumAircraftType 翻译）；
  CategoryName 是平台形态（固定翼/直升机/舰载固定翼等）
- **稀疏字段**：CountryName 仅约 1.7 万行有值（'United States'/'美国' 2656 行居首），
  YearCommissioned NULL/0 占多数 —— 过滤谓词对 NULL 自然排除（LIKE/比较对 NULL 恒假），
  属预期语义：启用某过滤即隐含"仅统计该字段有值的记录"

## 3. 架构决策

### 3.1 插件形态（决策 1：独立插件，复用 ext-01 骨架）

新建 `src/wsf_plugins/ysim_equipment_db/`，四件套 + 源码：

```
src/wsf_plugins/ysim_equipment_db/
├── CMakeLists.txt            # 见 §3.5
├── wsf_module                # 目录标记（内容固定一行注释）
├── wsf_cmake_extension.cmake # WSF_EXT_NAME=ysim_equipment_db（仿 ext-01 16 行模板）
└── source/
    ├── EquipmentDB.hpp/.cpp           # 领域类（继承 WsfObject）
    ├── EquipmentDBScriptClass.hpp/.cpp # 脚本包装（继承 WsfScriptObjectClass）
    └── YsimEquipmentDB.cpp            # 插件入口（扩展注册 + 想定输入块）
```

挂接机制（源码已确认）：`src/CMakeLists.txt` L140 对 `src/*/wsf_module` 做 glob 自动发现，
新目录**无需改任何既有 CMake 文件**，但需执行一次 cmake 重新 configure（已知坑清单同款）。

### 3.2 EquipmentDB 类设计（决策 2：继承 WsfObject，不可克隆）

- `class EquipmentDB : public WsfObject`（满足用户明确要求；WsfObject 见
  src/core/wsf/source/WsfObject.hpp L39，含 name/type/ProcessInput 体系）
- `Clone()` 返回 nullptr（仿 WsfUncloneableObject 语义，数据库句柄不可复制）
- `GetScriptClassName()` 返回 `"EquipmentDB"`
- 持有：`sqlite3* mDB`（C API 句柄——SQLite 本身即 C ABI，符合跨边界契约偏好）、
  路径、max_rows、打开状态
- 打开方式：`sqlite3_open_v2(path, &db, SQLITE_OPEN_READONLY, nullptr)`
  —— **只读模式，从机制上杜绝写库/损坏**
- `ProcessInput` 子命令（C++ 侧对象配置）：`database <path>` / `max_rows <n>`
- 查询方法与脚本方法一一对应（§3.3 表），全部返回值/容错语义固定

### 3.3 脚本接口（决策 3：脚本类 "EquipmentDB"，仿 UtScriptFileIO 实例模式）

注册方式（wsf_oms_uci 先例，WsfUCI.cpp L116）：ScenarioExtension::AddedToScenario 中
`GetScenario().GetScriptTypes()->Register(ut::make_unique<script::EquipmentDBScriptClass>("EquipmentDB", ...))`。

脚本用法（WSF 脚本语法）：

```
EquipmentDB equipdb;                              # 声明（Create 构造，默认未打开）
bool ok = equipdb.Open("");                       # "" = 想定块配置的默认库；相对路径相对想定文件目录解析
int n = equipdb.GetEquipmentCount("DataAircraft"); # 6933；-1 = 出错
Array<int> ids = equipdb.FindByName("F-16", "");  # 名称 LIKE '%F-16%'，匹配 Name/Name_CN，上限 max_rows
Array<int> sids = equipdb.GetLinkedIDs("DataAircraftSensors", 1305);  # 1305 的 14 个传感器 ID
Array<int> usa = equipdb.FindFiltered("DataAircraft", "", "United States", "", "", 1980, 2000); # 美/1980-2000 机型
Map<string,string> s = equipdb.GetSummary("DataAircraft", 122); # DataEquipmentSummary 整行
Map<string,string> r = equipdb.GetRecord("DataWeapon", 336);    # Data<类名> 明细表整行
Array<string> cols = equipdb.GetColumnNames("DataSensor");      # PRAGMA table_info 列名
string t = equipdb.GetEnumDescription("EnumAircraftType", 2002); # "Multirole (Fighter/Attack)"
string info = equipdb.GetInfo();                   # 路径/版本/各类计数 诊断串
```

方法清单（返回类型均为脚本既有类型，Map 返回有先例 WsfScriptTrackClass::Types）：

| 方法 | 签名 | 返回/错误语义 |
|---|---|---|
| Open | `bool Open(string aPath)` | 成功 true；失败 false + 告警日志；"" = 默认库；未配置默认库且路径空 → false |
| Close | `void Close()` | 幂等 |
| IsOpen | `bool IsOpen()` | — |
| GetEquipmentCount | `int GetEquipmentCount(string aTableName)` | DataEquipmentSummary 按 TableName 计数；`""`/`"*"` = 总数；出错 -1 |
| FindByName | `Array<int> FindByName(string aPattern, string aTableName)` | LIKE %pattern%，匹配 Name 或 Name_CN；表名空 = 全部类；按 ID 升序；上限 max_rows |
| GetLinkedIDs | `Array<int> GetLinkedIDs(string aLinkTable, int aParentID)` | 关联表导航（F 阶段）：固定约定 `SELECT ComponentID WHERE ID=?`；按 ComponentID 升序；上限 max_rows；非法表名/未打开/无行 → 空 Array |
| FindFiltered | `Array<int> FindFiltered(string aTableName, string aName, string aCountry, string aCategory, string aType, int aYearFrom, int aYearTo)` | 组合过滤（F 阶段）：字符串空 = 通配，年代 0 = 不设限；Name/CountryName/CategoryName/TypeName 各含 *_CN 双语 LIKE（ESCAPE 转义）+ YearCommissioned 闭区间；按 ID 升序；上限 max_rows；NULL 字段自然排除（§2.5） |
| GetSummary | `Map<string,string> GetSummary(string aTableName, int aID)` | 汇总表整行；查无 → 空 Map |
| GetRecord | `Map<string,string> GetRecord(string aTableName, int aID)` | 明细/汇总表整行（PRAGMA 取列序）；查无 → 空 Map |
| GetColumnNames | `Array<string> GetColumnNames(string aTableName)` | — |
| GetEnumDescription | `string GetEnumDescription(string aEnumTable, int aCode)` | 查无 → "" |
| GetInfo | `string GetInfo()` | 未打开 → "EquipmentDB: not open" |

值转换规则：INTEGER → 十进制串；REAL → `%g`；TEXT 原样；NULL → ""；BLOB → `<BLOB n>`。

### 3.4 想定输入块与默认配置（决策 4：ScenarioExtension 持有默认值，脚本类回指针取用）

想定文件可配置（想定关键字英文，不翻译）：

```
ysim_equipment_db
   database ../build/db/DB3K_modern.db3   # 默认库路径（相对想定文件目录）
   max_rows 100                            # FindByName 结果上限
   preload true                            # 想定加载期预打开验证（默认 true）
end_ysim_equipment_db
```

- 解析者：ScenarioExtension::ProcessInput（WsfScenario 对未识别命令轮询扩展，ext-01 同款）
- 顺序保证：脚本运行时想定必已解析完 → EquipmentDBScriptClass 持有 ScenarioExtension
  回指针（注册时 Set），Create() 时取实时默认值 —— 无时序问题
- preload 失败（路径不存在/非 SQLite 文件/表缺失）：**告警不中止**（防崩溃原则，
  脚本仍可 Open 其他路径）；打开前校验文件头 16 字节 = "SQLite format 3\0"
- 路径解析次序：绝对路径原样 → 相对想定输入文件目录 → 相对 CWD

### 3.5 SQLite 链接策略（决策 5：复用 ext-01 三态选项，防干净构建树翻车）

直接照抄 ext-01 CMakeLists 的 `YSIM_SQLITE_SUPPORT` 三态模式（AUTO/ON/OFF，
3rd_party glob → find_package(SQLite3) 回退 → 降级编译），但变量名独立为
`YSIM_EQDB_SQLITE_SUPPORT`，降级宏 `YSIM_EQDB_NO_SQLITE`：无 SQLite 时插件仍编译加载，
脚本方法返回错误态（GetEquipmentCount=-1，Open=false + 告警）。
其余：`project(ysim_equipment_db)` + `include(swdev_project)` + `file(GLOB SRCS source/*.?pp)`
+ `target_link_libraries(... ${WSF_LIBS})` + `swdev_plugin_install(${PROJECT_NAME} mission_plugins)`
+ `swdev_warning_level` + `FOLDER "wsf_plugins"`。

### 3.6 防御性设计（红线汇总）

1. 只读打开（SQLITE_OPEN_READONLY）——任何 SQL 均无法写库
2. 表名/列名标识符白名单校验 + 双引号包裹；数据值全部参数化绑定
3. 行数上限 max_rows（默认 100）防大结果集
4. 全部查询失败路径返回约定错误值 + UtLog 告警，不抛异常不崩溃
5. 打开前文件头校验（"SQLite format 3\0"），给明确错误信息
6. 单线程使用（仿真线程内调用）；不跨线程共享句柄，文档注明
7. 含中文注释源文件一律 UTF-8 with BOM（项目红线，PowerShell 写入法见 SKILL.md）

## 4. 验证与收尾标准（2026-10-21 验收结论：全部通过）

1. 编译：`cmake --build E:/afsim/ysim/build --config Release --target ysim_equipment_db -- /m`
   成功 ✓；`ysim_equipment_db.dll` 已出现在 `build/Release/mission_plugins/` ✓；
   `YSIM_EQDB_SQLITE_SUPPORT=OFF` 降级路径编译通过后已恢复 AUTO 并重建复验 ✓
2. 端到端（tests\equipmentdb\run_equipmentdb.bat）**17/17 断言全部 PASS**：
   - GetEquipmentCount("DataAircraft") == 6933、("*") == 71166、("DataSensor") == 7034（与 sqlite3 实时直查一致）
   - FindByName("F-16","DataAircraft")：命中 max_rows 上限 100，首 ID == 122（LIKE 双字段 + ESCAPE 生效）
   - GetSummary("DataAircraft",122)：Name == "F-16A Falcon [Netz]"，Name_CN == "F-16 战隼"（中文全程无损）
   - GetRecord("DataAircraft",122) 整行回读 ID/Name 一致；GetColumnNames("DataWeapon") == 59 列
   - GetEnumDescription("EnumAircraftType",2002) == "Multirole (Fighter/Attack)"
3. 负面用例不崩溃 ✓：Open("不存在路径")=false；GetEquipmentCount("NotATable!")=-1（白名单拦截）；
   GetRecord("DataWeapon",99999999) → 空 Map（缺失键取值 == ""）
4. bat 一键运行 ✓：窗口标题 ysim-bat-equipmentdb，注释纯 ASCII 英文；结果文件 PASS 即删/FAIL 保留
5. 记账归档 ✓：spec 状态更新、tasks 全勾、tests\README.md / BAT_USAGE.md / 子目录 README 收录
   （结果文件在 bat 所在目录而非 temp\：属脚本断言中间产物，PASS 自动清理，与 temp\ 红线不冲突）

实现备注（与 spec 的偏差）：
- 场景级 max_rows 经脚本类 Create() 注入对象默认值（GetSCENARIO 取场景配置），非 Open 时注入
- 基准目录在 FileLoaded（首个想定文件）时机捕获，非 ProcessInput 时机
- 测试发现：WSF 脚本对象声明须显式构造（`EquipmentDB x = EquipmentDB();`），
  裸声明 `EquipmentDB x;` 为 null 引用（FileIO 同此，test_file_io.txt 先例）

### 4.1 F 阶段补强验收（2026-10-21 同日完成）

1. GetLinkedIDs：固定 ID→ComponentID 约定（§2.5 实测 46 表统一）；上限 max_rows；
   截断路径实测 DataAircraftLoadouts(ID=5257)（飞机 5257 的 170 个挂载方案）→ 返回 100（与 max_rows=100 一致）
2. FindFiltered：五路过滤（名称/国别/类别/型别双语 LIKE + 年代闭区间）全部参数化绑定，
   空串通配、0 年代不设限、NULL 字段自然排除
3. 端到端断言 17 → **37 项全部 PASS**（新增 20 项：组件链计数与首 ID×5 + 截断验证×2、
   组合过滤计数与正值哨兵×11、负面用例×2——不存在父 ID → 空 Array、非法表名 → 空 Array）
4. 测试夹具（数据静态）：Tu-22RDK（ID=1305，14 传感器/3 挂架/13 挂载方案）、
   飞机 5257（170 挂载方案，截断用）、F-16（美/1978-1990 计 12 架次）

### 4.2 G 阶段单元测试（2026-10-21 同日完成，31/31 PASS）

1. 位置与形态：`src\wsf_plugins\ysim_equipment_db\test\`（仿 six_dof_test 模式），
   GTest 直测领域类 EquipmentDB；脚本包装为薄封送，行为由端到端 37 项覆盖
2. 链接方式：插件 DLL 仅导出 extern "C" 入口（无 C++ 导入库），
   故 EquipmentDB.cpp 直接编入测试 exe，不链插件目标
3. 测试夹具：自建微型 SQLite 库（8 行汇总 + 链接表 + 枚举 + 明细，含 NULL 行与 '%' 字面量行），
   确定性断言，不依赖 51MB 运行库；中文值用 UTF-8 字节转义规避 MSVC GBK 执行编码
4. 覆盖（31 项）：生命周期/坏文件/未打开错误语义 4、标识符白名单 1、计数 2、
   FindByName 4（中英双列、表过滤、%转义、截断）、GetLinkedIDs 5（排序/截断/缺父/坏表）、
   FindFiltered 10（国别含中文列、类别+年代、型别、纯年代、表过滤、组合、全通配、截断、坏表、%转义）、
   整行/列名/枚举 3、Open 幂等/重开 2
5. 版本坑（记入已知坑）：本仓库 GTest 锁 1.8.0，套件钩子须用 `SetUpTestCase/TearDownTestCase`
   （`SetUpTestSuite` 为 1.10+ 命名，静默不被调用 → 夹具从未创建，31 项全秒败）；
   另 `GetEquipmentCount("合法但无记录的表名")` 返回 0（TableName 过滤语义），仅白名单拦截才 -1
6. 运行：`cmake --build E:/afsim/ysim/build --config Release --target ysim_equipment_db_test -- /m`
   后在 build\Release 运行 `ysim_equipment_db_test.exe`（亦注册为 ctest 项 ysim_equipment_db）

## 5. 遗留项（转后续批次）

- 装备记录 → WSF 平台/传感器/武器类型（WsfPlatformTypes 等）的参数化生成器（ext-03 模型库主线；
  所需组件链导航已由 GetLinkedIDs 提供：平台→传感器/挂架/挂载方案→武器的完整 ID 链路）
- FindByName/FindFiltered 前缀优先排序（精确 > 前缀 > 子串）与分页（offset 参数）
- *Codes 能力代码表导航（ID → CodeID，如 DataWeaponTargets/DataSensorCapabilities）
- DBInfo.dat 私有格式逆向（如需编辑器级数据）
