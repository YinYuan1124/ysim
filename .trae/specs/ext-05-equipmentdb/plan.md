# ext-05-equipmentdb 推进计划

> 状态：计划态（未实施）。实现按本计划分四阶段推进，每阶段可独立验证。

## 阶段 A：插件骨架 + 数据库打通（先冒烟）

1. 建目录四件套（CMakeLists.txt / wsf_module / wsf_cmake_extension.cmake / source/）
   - CMake 照抄 ext-01 三态 SQLite 模式（变量名 YSIM_EQDB_SQLITE_SUPPORT）
2. YsimEquipmentDB.cpp：Register_ysim_equipment_db + Application/Scenario 扩展
   - ScenarioExtension：默认配置（database/max_rows/preload）+ ProcessInput 输入块
3. EquipmentDB.hpp/.cpp：WsfObject 子类骨架
   - 只读打开（文件头校验）+ Close/IsOpen + 值转换辅助函数
4. 脚本类最小集：Open/IsOpen/GetEquipmentCount/GetInfo
5. 重新 configure + 编译 + DLL 落 mission_plugins 确认

## 阶段 B：查询方法全集

1. FindByName（LIKE + Name/Name_CN + max_rows + ID 升序）
2. GetSummary / GetRecord（PRAGMA table_info 列序 + 参数化 WHERE ID=?）
3. GetColumnNames / GetEnumDescription
4. 标识符白名单校验 + 注入防护实装

## 阶段 C：端到端测试三件套

1. tests\equipmentdb\test_equipmentdb.txt：输入块 + 脚本块逐项查询并 print
2. verify_equipmentdb.py：跑 mission.exe 截获输出，与 sqlite3 直查交叉断言 + 负面用例
3. run_equipmentdb.bat：一键化（标题 ysim-bat-equipmentdb、注释纯 ASCII、定向清理）

## 阶段 D：收尾归档

1. 无 SQLite 降级路径编译验证（可选，条件允许时）
2. spec 状态改"实现已完成" + 验证结论归档；tasks 全勾
3. .trae/specs/README.md 目录树 + roadmap.md 工作包索引更新

## 后续批次注意事项

- 装备 → 平台类型生成器（ext-03 主线）应复用本插件的 EquipmentDB 查询层，勿另起炉灶
- 新增源文件后 MSBuild 陈旧缓存坑：touch 或重新 configure（SKILL.md 已知坑表）
- 想定关键字（ysim_equipment_db/database/max_rows/preload）保持英文原样
