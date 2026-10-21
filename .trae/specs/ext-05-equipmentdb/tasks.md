# ext-05-equipmentdb 任务清单

> 验收口径见 spec.md §4；勾选 = 该项验证通过而非仅编译通过。
> **2026-10-21 验收结论：全部完成（17/17 端到端断言 PASS，含 OFF 降级编译验证）**

## 阶段 A：骨架 + 打通

- [x] A1 建目录四件套（CMake 三态 SQLite / wsf_module / wsf_cmake_extension.cmake）
- [x] A2 插件入口 + Application/Scenario 扩展 + `ysim_equipment_db` 输入块
- [x] A3 EquipmentDB 类（继承 WsfObject，只读打开 + 文件头校验）
- [x] A4 脚本类注册 "EquipmentDB"（11 个方法，mConstructible + 显式构造）
- [x] A5 重新 configure → Release 编译 → DLL 落 build/Release/mission_plugins/ 确认

## 阶段 B：查询全集

- [x] B1 FindByName（Name/Name_CN LIKE + ESCAPE 转义、max_rows、ID 升序）
- [x] B2 GetSummary / GetRecord（Map<string,string> 整行，含中文名回读）
- [x] B3 GetColumnNames / GetEnumDescription
- [x] B4 标识符白名单 + 参数化绑定（注入防护，负面用例实测拦截）

## 阶段 C：端到端验证

- [x] C1 测试想定（输入块 + on_initialize 脚本查询写结果文件）
- [x] C2 verify_equipmentdb.py 交叉断言 17 项（与 sqlite3 直查逐项比对）
- [x] C3 run_equipmentdb.bat 一键运行（标题 ysim-bat-equipmentdb、ASCII 注释、PASS 删/FAIL 留；
      2026-10-21 按 tests\ 全局 demo 定位策略随纯验证类 bat 一并移除，入口改 python 直跑——见文件尾验收命令口径）

## 阶段 D：收尾

- [x] D1 YSIM_EQDB_SQLITE_SUPPORT=OFF 降级编译验证（通过后恢复 AUTO 重建复验）
- [x] D2 spec/tasks/README/BAT_USAGE/roadmap 归档更新（状态：实现已完成）

## 阶段 F：插件补强（2026-10-21 同日，spec §2.5/§3.3/§4.1）

- [x] F1 侦察关联表结构（46 链接表统一 ID→ComponentID）与过滤字段语义（RoleName 全 NULL，角色在 TypeName）
- [x] F2 spec 增补：§2.5 侦察结论 + §3.3 方法表两行 + §4.1 验收节 + §5 遗留项收敛
- [x] F3 实现 GetLinkedIDs + FindFiltered（EscapeLike 提取复用；五路过滤全参数化绑定）
- [x] F4 BOM 重套 + Release 编译 + DLL 部署
- [x] F5 测试扩展：场景 +13 处查询输出，verify 交叉断言 17 → **37 项全部 PASS**
      （截断夹具修正：飞机 5257 的 170 挂载方案，非单挂载方案武器数）
- [x] F6 归档更新 + 清理侦察脚本

## 阶段 G：单元测试（2026-10-21 同日，spec §4.2）

- [x] G1 EquipmentDB_Test.cpp：自建夹具库 + 31 项断言（重点 GetLinkedIDs/FindFiltered 各 5/10 项）
- [x] G2 test/CMakeLists.txt（GTest + SQLite 前置守卫；领域类源码直编入 exe）+ 插件挂 add_subdirectory(test)
- [x] G3 configure + 编译（修复：target_link_libraries 签名混用、source/ include 路径）
- [x] G4 31/31 全部 PASS（修复两处测试自身缺陷：GTest 1.8 须 SetUpTestCase；count 语义断言）
- [x] G5 归档：spec §4.2 + tasks 阶段 G + tests/equipmentdb README 提示

## 验收命令口径

```
# 编译
cmake --build E:/afsim/ysim/build --config Release --target ysim_equipment_db -- /m
# 部署确认
dir E:\afsim\ysim\build\Release\mission_plugins\ysim_equipment_db.dll
# 端到端（37 项断言 + RESULT JSON 行）——bat 已按 tests\ demo 定位策略移除（2026-10-21），改两步直跑：
cd E:\afsim\ysim\tests\equipmentdb
E:\afsim\ysim\build\Release\mission.exe scenarios\test_equipmentdb.txt
python verify_equipmentdb.py
```
