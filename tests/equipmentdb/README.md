# equipmentdb —— ext-05 现代装备数据库插件验证

对应插件：`ysim_equipment_db.dll`（部署于 `build\Release\mission_plugins\`）

功能：封装 `EquipmentDB` 类（继承 `WsfObject`）注册到 WSF 脚本系统，
以只读模式查询 `build\db\DB3K_modern.db3`（71,166 条现代装备记录，含中英文名称）。

## 一键运行

双击 [run_equipmentdb.bat](./run_equipmentdb.bat)：自动跑 mission 想定 →
脚本查询结果与 SQLite 直查交叉断言（37 项）→ 清理。

- 无参数；窗口标题 `ysim-bat-equipmentdb`
- 断言覆盖：各类计数 / F-16 名称检索 / 汇总表与明细表整行读取 / 中英文名回读 /
  Enum 代码翻译 / 列名 / 关联表导航（传感器/挂架/挂载方案计数与首 ID、max_rows 截断）/
  组合过滤（国别/类别+年代/名称/型别）/ 负面用例（坏路径、非法表名、不存在 ID 不崩溃）
- 结果文件 `eqdb_result.txt` 生成于本目录，PASS 时自动删除，FAIL 保留供排查

## 脚本用法（WSF 脚本）

```
ysim_equipment_db
   database ../../build/db/DB3K_modern.db3   # 相对本想定文件目录解析
   max_rows 100                               # FindByName 结果上限
   preload true
end_ysim_equipment_db

on_initialize
   EquipmentDB equipdb = EquipmentDB();
   bool ok = equipdb.Open("");                          # "" = 上面的默认库
   int n = equipdb.GetEquipmentCount("DataAircraft");   # 6933
   Map<string, string> s = equipdb.GetSummary("DataAircraft", 122); # F-16A
   Array<int> ids = equipdb.FindByName("F-16", "");     # 名称检索
   Array<int> sids = equipdb.GetLinkedIDs("DataAircraftSensors", 1305); # 关联表导航
   Array<int> usa = equipdb.FindFiltered("DataAircraft", "", "United States", "", "", 1980, 2000); # 组合过滤
end_on_initialize
```

完整方法契约（13 个方法）见 `.trae\specs\ext-05-equipmentdb\spec.md` §3.3：
GetLinkedIDs 按库内统一约定 `ID → ComponentID` 导航任意链接表；
FindFiltered 为名称/国别/类别/型别（均含中文列）+ 年代区间的组合检索，空串通配。

## 单元测试（另一层）

本目录为端到端层；领域类另有 GTest 单元测试 `src\wsf_plugins\ysim_equipment_db\test\`
（自建微型夹具库，31 项断言）：

```powershell
cmake --build E:/afsim/ysim/build --config Release --target ysim_equipment_db_test -- /m
E:\afsim\ysim\build\Release\ysim_equipment_db_test.exe
```
