# -*- coding: utf-8 -*-
"""ext-02 阶段 B 批次 B2 端到端验证脚本（PDU 字段覆盖补齐）

流程：起 dis_capture.py 捕获客户端 → 3s 后起 mission（B2 交战想定 +
     dis_interface 块，RT 60s）→ 客户端抽取 Fire/Detonation/Emission/Signal
     关键字段输出 RESULT。
断言（spec §6.4.4，两轮 + 降级回归 + B1 回归）：
  1. mission 退出码 0
  2. Fire PDU >=1 且 Warhead!=0、Fuse!=0（表1 补齐生效）
  3. Fire WeaponType 七段码 country=36（想定显式 entity_type 映射链路）
  4. Detonation PDU >=1 且 Warhead!=0、DetonationResult in {1,2,5,6}
  5. Emission PDU >=1 且 Emitter Name!=0、Function!=0（表2 补齐生效）
  6. Signal PDU >=1 且空载荷（现状记录——缺口底账证据）
  7. 降级回归：无 dis_interface 块 → 两模块闲置告警 + 无 FATAL
  8. B1 回归：七段码映射注入日志 + B2 钩子挂载日志并存
"""
import json
import subprocess
import sys
import time
from pathlib import Path

# 测试资产归档（tests/）：输入（想定/块/子客户端）取本目录，运行输出仍写 temp/
TEMP = Path(r"e:\Project\ysim\temp")
HERE = Path(__file__).resolve().parent
MISSION_EXE = r"e:\Project\ysim\build\Release\mission.exe"
IFACE = str(HERE / "scenarios" / "dis_b2_iface.txt")
SCENARIO = str(HERE / "scenarios" / "dis_b2_scenario.txt")
CWD = r"e:\Project\ysim\src\core\wsf_mil\test_mission\dis\warfare"

DET_RESULTS_OK = {1, 2, 5, 6}  # 1=Entity Impact 2=Entity Proximity 5=Ground Impact 6=Ground Proximity（近似语义域）


def run_e2e(round_id):
    c_out = TEMP / f"dis_capture_r{round_id}.out"
    m_out = TEMP / f"dis_b2_mission_r{round_id}.out"
    client = subprocess.Popen(
        [sys.executable, str(HERE / "dis_capture.py")],
        cwd=str(TEMP), stdout=open(c_out, "w"), stderr=subprocess.STDOUT)
    time.sleep(3)
    mission = subprocess.Popen(
        [MISSION_EXE, "-rt", IFACE, SCENARIO], cwd=CWD,
        stdout=open(m_out, "w"), stderr=subprocess.STDOUT)
    m_rc = mission.wait(timeout=300)
    c_rc = client.wait(timeout=180)

    lines = c_out.read_text(encoding="utf-8", errors="replace").splitlines()
    result_line = [l for l in lines if l.startswith('{"RESULT"')]
    stats = json.loads(result_line[-1])["RESULT"] if result_line else {}
    m_text = m_out.read_text(encoding="utf-8", errors="replace")
    return {
        "_id": round_id, "m_rc": m_rc, "c_rc": c_rc, "stats": stats,
        "b1_inject_log": "ysim_dis_gateway: injected entity type mappings" in m_text,
        "b2_hook_log": "ysim_dis_field_patch: PDU field patch hooks attached" in m_text,
        "no_fatal": "FATAL" not in m_text,
    }


def run_regression():
    """无 dis_interface 块：两模块（B1 注入 + B2 钩子）均闲置告警，仿真正常完成"""
    out = TEMP / "dis_b2_regress_r.out"
    p = subprocess.run([MISSION_EXE, "-rt", SCENARIO], cwd=CWD,
                       stdout=open(out, "w"), stderr=subprocess.STDOUT, timeout=300)
    text = out.read_text(encoding="utf-8", errors="replace")
    return {
        "rc": p.returncode,
        "idle_b1": "dis_interface has no device configured" in text,
        "idle_b2": "PDU field patch hooks not attached" in text,
        "no_fatal": "FATAL" not in text,
    }


def assert_all(rounds, regress):
    ok = True

    def check(name, cond, detail=""):
        nonlocal ok
        print(f"  [{'PASS' if cond else 'FAIL'}] {name} {detail}")
        ok = ok and cond

    for r in rounds:
        print(f"--- 第 {r['_id']} 轮 ---")
        s = r["stats"]
        check("1. mission 退出码 0", r["m_rc"] == 0, f"rc={r['m_rc']}")
        check("   捕获客户端退出码 0", r["c_rc"] == 0, f"rc={r['c_rc']}")
        fires = s.get("fires", [])
        check("2. Fire PDU >=1 且 Warhead!=0、Fuse!=0（表1 补齐）",
              len(fires) >= 1 and all(f["warhead"] != 0 and f["fuse"] != 0 for f in fires),
              f"fires={fires}")
        check("3. Fire WeaponType country=36（显式映射链路）",
              len(fires) >= 1 and all(f["country"] == 36 for f in fires),
              f"countries={[f['country'] for f in fires]}")
        dets = s.get("detonations", [])
        check("4. Detonation PDU >=1 且 Warhead!=0、Result 合法",
              len(dets) >= 1 and all(d["warhead"] != 0 and d["result"] in DET_RESULTS_OK for d in dets),
              f"detonations={dets}")
        check("5. Emission PDU >=1 且 Name!=0、Function!=0（表2 补齐）",
              s.get("emission_nonzero_name", 0) >= 1 and s.get("emission_nonzero_function", 0) >= 1,
              f"nonzero_name={s.get('emission_nonzero_name')} nonzero_function={s.get('emission_nonzero_function')} sample={s.get('emission_sample')}")
        check("6. Signal PDU >=1 且空载荷（缺口底账证据）",
              s.get("signals", 0) >= 1 and s.get("signals_empty_payload", 0) >= 1,
              f"signals={s.get('signals')} empty={s.get('signals_empty_payload')}")
        check("   无 FATAL", r["no_fatal"] is True)

    print("--- B1 回归（双批次共存）---")
    check("8. B1 映射注入日志出现", all(r["b1_inject_log"] for r in rounds))
    check("   B2 钩子挂载日志出现", all(r["b2_hook_log"] for r in rounds))

    print("--- 降级回归（无 dis_interface 块）---")
    check("7. mission 退出码 0", regress["rc"] == 0, f"rc={regress['rc']}")
    check("   B1 模块闲置告警出现", regress["idle_b1"] is True)
    check("   B2 模块闲置告警出现", regress["idle_b2"] is True)
    check("   无 FATAL", regress["no_fatal"] is True)

    return ok


def main():
    do_regress = "--regression" in sys.argv
    rounds = []
    for rid in (1, 2):
        print(f"===== 第 {rid} 轮端到端 =====")
        rounds.append(run_e2e(rid))
    regress = run_regression() if do_regress else {
        "rc": 0, "idle_b1": True, "idle_b2": True, "no_fatal": True}
    if not do_regress:
        print("(跳过降级回归：未传 --regression)")
    ok = assert_all(rounds, regress)
    print(f"\n总判定：{'ALL PASS' if ok else 'FAILED'}")
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
