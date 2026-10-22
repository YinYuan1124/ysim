# -*- coding: utf-8 -*-
"""ext-02 阶段 B 批次 B1 端到端验证脚本（DIS Entity Type 七段码映射注入）

流程：起 dis_listen.py 监听客户端 → 3s 后起 mission（air_movers 想定 +
     dis_interface 块，RT 20s）→ 客户端聚合 EntityState 七段码输出 RESULT。
断言：想定显式 entity_type（BLUE_FIGHTER_AIR, country=225）优先不被插件覆盖；
     插件注入映射（RED_FIGHTER_AIR, country=36）经 EntityState PDU 生效。
可选 --regression：无 dis_interface 块时插件闲置降级回归。
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
DIS_BLOCK = str(HERE / "scenarios" / "dis_iface_block.txt")
SCENARIO = str(HERE / "scenarios" / "dis_b1_scenario.txt")
SCENARIO_NODEVICE = str(HERE / "scenarios" / "dis_b1_nodevice.txt")
CWD_DEMO = r"e:\Project\ysim\build\demos\air_to_air"

# 插件表值（YsimDisGateway.cpp cTYPE_MAPPINGS）：RED_FIGHTER_AIR 国产映射
EXPECT_CN = [1, 2, 36, 2, 1, 2, 0]
# 想定显式映射（dis_iface_block.txt）：BLUE_FIGHTER_AIR 美制（优先级验证）
EXPECT_US = [1, 2, 225, 2, 1, 1, 0]


def run_e2e(round_id):
    c_out = TEMP / f"dis_listen_r{round_id}.out"
    m_out = TEMP / f"dis_mission_r{round_id}.out"
    client = subprocess.Popen(
        [sys.executable, str(HERE / "dis_listen.py")],
        cwd=str(TEMP),
        stdout=open(c_out, "w"), stderr=subprocess.STDOUT)
    time.sleep(3)
    mission = subprocess.Popen(
        [MISSION_EXE, "-rt", DIS_BLOCK, SCENARIO], cwd=CWD_DEMO,
        stdout=open(m_out, "w"), stderr=subprocess.STDOUT)
    m_rc = mission.wait(timeout=300)
    c_rc = client.wait(timeout=60)

    result_line = [l for l in c_out.read_text(encoding="utf-8", errors="replace").splitlines()
                   if l.startswith('{"RESULT"')]
    stats = json.loads(result_line[-1])["RESULT"] if result_line else {}
    m_text = m_out.read_text(encoding="utf-8", errors="replace")
    return {"m_rc": m_rc, "c_rc": c_rc, "stats": stats,
            "plugin_active": "ysim_dis_gateway: injected entity type mappings" in m_text,
            "no_fatal": "FATAL" not in m_text}


def run_regression():
    """无 dis_interface 块：插件闲置告警 + 仿真正常完成"""
    out = TEMP / "dis_regress_r.out"
    p = subprocess.run([MISSION_EXE, "-rt", SCENARIO_NODEVICE], cwd=CWD_DEMO,
                       stdout=open(out, "w"), stderr=subprocess.STDOUT, timeout=300)
    text = out.read_text(encoding="utf-8", errors="replace")
    return {"rc": p.returncode,
            "idle_warn": "dis_interface has no device configured" in text,
            "no_fatal": "FATAL" not in text}


def assert_all(rounds, regress):
    ok = True
    def check(name, cond, detail=""):
        nonlocal ok
        print(f"  [{'PASS' if cond else 'FAIL'}] {name} {detail}")
        ok = ok and cond

    for r in rounds:
        print(f"--- 第 {r['_id']} 轮 ---")
        check("mission 退出码 0", r["m_rc"] == 0, f"rc={r['m_rc']}")
        check("监听客户端退出码 0", r["c_rc"] == 0, f"rc={r['c_rc']}")
        s = r["stats"]
        check("收到 DIS PDU 流", s.get("total_pdus", 0) > 0,
              f"total_pdus={s.get('total_pdus')}")
        check("EntityState PDU 存在（type=1）", s.get("pdu_type_counts", {}).get("1", 0) > 0,
              f"counts={s.get('pdu_type_counts')}")
        check("实体数 >= 4（2v2 全平台）", s.get("entity_count", 0) >= 4,
              f"entity_count={s.get('entity_count')}")
        cc = s.get("country_counts", {})
        check("想定显式映射实体恰 2 个（country=225 蓝/蓝）", cc.get("225") == 2,
              f"country_counts={cc}")
        check("插件注入映射实体恰 2 个（country=36 红/红）", cc.get("36") == 2,
              f"country_counts={cc}")
        ents = s.get("entities", {})
        cn_types = [tuple(v["type"]) for v in ents.values() if v["type"][2] == 36]
        us_types = [tuple(v["type"]) for v in ents.values() if v["type"][2] == 225]
        check("插件注入七段码与表值一致（RED_FIGHTER_AIR）",
              len(cn_types) == 2 and all(t == tuple(EXPECT_CN) for t in cn_types),
              f"cn_types={cn_types}")
        check("想定显式七段码未被插件覆盖（BLUE_FIGHTER_AIR）",
              len(us_types) == 2 and all(t == tuple(EXPECT_US) for t in us_types),
              f"us_types={us_types}")
        check("插件注入日志出现", r["plugin_active"] is True)
        check("无 FATAL", r["no_fatal"] is True)
        # 人读信息：实体明细（marking 为试解析，不参与断言）
        for k, v in sorted(ents.items()):
            print(f"    entity {k}: type={v['type']} marking={v['marking']!r} seen={v['seen']}")

    print("--- 降级回归（无 dis_interface 块）---")
    check("mission 退出码 0", regress["rc"] == 0, f"rc={regress['rc']}")
    check("插件闲置告警出现", regress["idle_warn"] is True)
    check("无 FATAL", regress["no_fatal"] is True)

    return ok


def main():
    do_regress = "--regression" in sys.argv
    rounds = []
    for rid in (1, 2):
        print(f"===== 第 {rid} 轮端到端 =====")
        r = run_e2e(rid)
        r["_id"] = rid
        rounds.append(r)
    regress = run_regression() if do_regress else {"rc": 0, "idle_warn": True, "no_fatal": True}
    if not do_regress:
        print("(跳过降级回归：未传 --regression)")
    ok = assert_all(rounds, regress)
    print(f"\n总判定：{'ALL PASS' if ok else 'FAILED'}")
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
