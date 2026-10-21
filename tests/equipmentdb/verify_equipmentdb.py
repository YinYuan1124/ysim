# -*- coding: utf-8 -*-
# 二次开发（ext-05）：EquipmentDB 端到端交叉验证
#
# 读取想定脚本写出的 eqdb_result.txt（key=value），
# 与 sqlite3 直查 DB3K_modern.db3 的实时结果逐项断言（计数/查询/枚举/列名/负面用例）。
# 用法：python verify_equipmentdb.py [结果文件路径] （默认同目录 eqdb_result.txt）
import sqlite3
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
DB = HERE.parent.parent / "build" / "db" / "DB3K_modern.db3"
RESULT = Path(sys.argv[1]) if len(sys.argv) > 1 else HERE / "eqdb_result.txt"

failures = []
total = 0


def check(name, actual, expected):
    global total
    total += 1
    ok = str(actual) == str(expected)
    print(f'[{"PASS" if ok else "FAIL"}] {name}: got={actual!r} expected={expected!r}')
    if not ok:
        failures.append(name)


# ---- 解析结果文件 ----
if not RESULT.exists():
    print(f"[FAIL] result file missing: {RESULT}")
    print("       mission may have failed before the script block ran (plugin load error?)")
    sys.exit(1)

values = {}
info_line = ""
for line in RESULT.read_text(encoding="utf-8", errors="replace").splitlines():
    if "=" in line and not line.startswith("EquipmentDB:"):
        key, _, val = line.partition("=")
        values.setdefault(key.strip(), val)
    elif line.startswith("EquipmentDB:"):
        info_line = line

if not values:
    print(f"[FAIL] result file has no key=value lines: {RESULT}")
    sys.exit(1)

# ---- SQLite 直查基准 ----
con = sqlite3.connect(f"file:{DB.as_posix()}?mode=ro", uri=True)
cur = con.cursor()

cur.execute("SELECT COUNT(*) FROM DataEquipmentSummary WHERE TableName = 'DataAircraft'")
exp_air = cur.fetchone()[0]
cur.execute("SELECT COUNT(*) FROM DataEquipmentSummary")
exp_total = cur.fetchone()[0]
cur.execute("SELECT COUNT(*) FROM DataEquipmentSummary WHERE TableName = 'DataSensor'")
exp_sensor = cur.fetchone()[0]

# FindByName("F-16", "DataAircraft")：LIKE %F-16% on Name/Name_CN, ORDER BY ID LIMIT 100
cur.execute(
    "SELECT ID FROM DataEquipmentSummary "
    "WHERE (Name LIKE '%F-16%' OR Name_CN LIKE '%F-16%') AND TableName = 'DataAircraft' "
    "ORDER BY ID LIMIT 100")
exp_f16 = [r[0] for r in cur.fetchall()]

cur.execute("SELECT Name, Name_CN FROM DataEquipmentSummary WHERE TableName='DataAircraft' AND ID=122")
row = cur.fetchone()
exp_name, exp_name_cn = (row if row else ("", ""))

cur.execute("SELECT ID FROM DataWeapon WHERE ID=1")
row = cur.fetchone()
exp_wid = str(row[0]) if row else ""

cur.execute("SELECT ID, Name FROM DataAircraft WHERE ID=122")
row = cur.fetchone()
exp_rec_id, exp_rec_name = (str(row[0]), row[1]) if row else ("", "")

cur.execute("PRAGMA table_info(DataWeapon)")
exp_cols = len(cur.fetchall())

cur.execute("SELECT Description FROM EnumAircraftType WHERE ID=2002")
row = cur.fetchone()
exp_enum = row[0] if row else ""

# ---- F 阶段：链接导航与组合过滤的镜像查询（与插件语义逐字对齐） ----

def linked(link_table, parent_id, cap=100):
    """GetLinkedIDs 镜像：SELECT ComponentID WHERE ID=? ORDER BY ComponentID LIMIT cap"""
    cur.execute(f'SELECT ComponentID FROM "{link_table}" WHERE ID = ? ORDER BY ComponentID LIMIT ?',
                (parent_id, cap))
    return [r[0] for r in cur.fetchall()]

def filtered(table, name="", country="", category="", typ="", yf=0, yt=0, cap=100):
    """FindFiltered 镜像：五路可选过滤 + 年代闭区间 + ID 升序 + LIMIT cap（空串通配/0 不设限）"""
    sql = "SELECT ID FROM DataEquipmentSummary WHERE 1=1"
    args = []
    if table and table != "*":
        sql += " AND TableName = ?"
        args.append(table)
    for pattern, en, cn in ((name, "Name", "Name_CN"),
                            (country, "CountryName", "CountryName_CN"),
                            (category, "CategoryName", "CategoryName_CN"),
                            (typ, "TypeName", "TypeName_CN")):
        if pattern:
            like = f"%{pattern}%"
            sql += f" AND ({en} LIKE ? ESCAPE '\\' OR {cn} LIKE ? ESCAPE '\\')"
            args += [like, like]
    if yf > 0:
        sql += " AND YearCommissioned >= ?"
        args.append(yf)
    if yt > 0:
        sql += " AND YearCommissioned <= ?"
        args.append(yt)
    sql += " ORDER BY ID LIMIT ?"
    args.append(cap)
    return [r[0] for r in cur.execute(sql, args)]

exp_sensors = linked("DataAircraftSensors", 1305)
exp_mounts = linked("DataAircraftMounts", 1305)
exp_loadouts = linked("DataAircraftLoadouts", 1305)
cur.execute("SELECT COUNT(*) FROM DataAircraftLoadouts WHERE ID = 5257")
exp_l5257_total = cur.fetchone()[0]
exp_f1 = filtered("DataAircraft", country="United States")
exp_f2 = filtered("DataAircraft", category="Fixed Wing", yf=1980, yt=2005)
exp_f3 = filtered("DataAircraft", name="Tu-22")
exp_f4 = filtered("DataAircraft", name="F-16", country="United States", yf=1978, yt=1990)
exp_ftype = filtered("DataAircraft", typ="Fighter")

con.close()

# ---- 断言 ----
check("open", values.get("open"), "true")
check("aircraft_count", values.get("aircraft_count"), exp_air)
check("aircraft_count_star", values.get("aircraft_count_star"), exp_total)
check("sensor_count", values.get("sensor_count"), exp_sensor)
check("f16_count", values.get("f16_count"), len(exp_f16))
if exp_f16:
    check("f16_first", values.get("f16_first"), exp_f16[0])
check("summary_name", values.get("summary_name"), exp_name)
check("summary_name_cn", values.get("summary_name_cn"), exp_name_cn)
check("weapon_id", values.get("weapon_id"), exp_wid)
check("aircraft_rec_id", values.get("aircraft_rec_id"), exp_rec_id)
check("aircraft_rec_name", values.get("aircraft_rec_name"), exp_rec_name)
check("weapon_cols", values.get("weapon_cols"), exp_cols)
check("enum_desc", values.get("enum_desc"), exp_enum)
check("neg_open", values.get("neg_open"), "false")
check("neg_count", values.get("neg_count"), "-1")
check("neg_record", values.get("neg_record"), "true")

# ---- F 阶段断言：关联表导航 ----
check("linked_sensor_count_1305", values.get("linked_sensor_count_1305"), len(exp_sensors))
check("linked_sensor_count_positive", len(exp_sensors) > 0, True)
if exp_sensors:
    check("linked_sensor_first_1305", values.get("linked_sensor_first_1305"), exp_sensors[0])
check("linked_mount_count_1305", values.get("linked_mount_count_1305"), len(exp_mounts))
check("linked_loadout_count_1305", values.get("linked_loadout_count_1305"), len(exp_loadouts))
# 截断实测：飞机 5257 有 170 个挂载方案，max_rows=100 → 两侧均为 100
check("linked_a5257_loadouts", values.get("linked_a5257_loadouts"), min(exp_l5257_total, 100))
check("linked_a5257_cap_active", exp_l5257_total > 100, True)

# ---- F 阶段断言：组合过滤 ----
check("f1_us_aircraft", values.get("f1_us_aircraft"), len(exp_f1))
check("f1_positive", len(exp_f1) > 0, True)
check("f2_fixedwing_1980_2005", values.get("f2_fixedwing_1980_2005"), len(exp_f2))
check("f2_positive", len(exp_f2) > 0, True)
check("f3_tu22_count", values.get("f3_tu22_count"), len(exp_f3))
check("f3_positive", len(exp_f3) > 0, True)
if exp_f3:
    check("f3_tu22_first", values.get("f3_tu22_first"), exp_f3[0])
check("f4_f16_us_1978_1990", values.get("f4_f16_us_1978_1990"), len(exp_f4))
check("f4_positive", len(exp_f4) > 0, True)
check("f_type_fighter", values.get("f_type_fighter"), len(exp_ftype))
check("f_type_positive", len(exp_ftype) > 0, True)

# ---- F 阶段断言：负面用例 ----
check("neg_link_missing", values.get("neg_link_missing"), 0)
check("neg_filter_badtable", values.get("neg_filter_badtable"), 0)

check("info_line_present", "equipment_total=" in info_line, True)

print()
if failures:
    print(f"RESULT: {len(failures)} FAILED -> {', '.join(failures)}")
    print(f'RESULT: {{"pass": {total - len(failures)}, "fail": {len(failures)}, "status": "FAIL"}}')
    sys.exit(1)
print(f'RESULT: {{"pass": {total}, "fail": 0, "status": "PASS"}}')
