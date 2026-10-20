# -*- coding: utf-8 -*-
"""ext-02 平台级指令集批次端到端验证脚本（XIO 私有 Packet 网关）

流程：起客户端(180s 超时) → 3s 后起 mission(air_movers 想定+xio块, pause_for_connection)
     → 客户端 10 条指令脚本（ping/add_platform/set_route/set_speed/pause/resume/
     get_status/delete_platform×2/负向 UNKNOWN）→ 断言增删/航路/调速/回执全闭环。
可选 --regression：无 xio_interface 块时插件闲置降级回归。
"""
import json
import re
import subprocess
import sys
import time
from pathlib import Path

# 测试资产归档（tests/）：输入（想定/块）取本目录，运行输出仍写 temp/
TEMP = Path(r"e:\afsim\ysim\temp")
HERE = Path(__file__).resolve().parent
CLIENT_EXE = r"e:\afsim\ysim\build\Release\xio_gateway_client.exe"
MISSION_EXE = r"e:\afsim\ysim\build\Release\mission.exe"
XIO_BLOCK = str(HERE / "scenarios" / "xio_iface_block.txt")
# 平台级指令集验证用 air_movers 版 RT 想定（45s）：
# set_route/set_speed 需 RouteMover 族（BLUE/RED_FIGHTER_AIR），XIO_DRONE 为靶标类型
SCENARIO = str(HERE / "scenarios" / "xio_rt2_scenario.txt")
CWD_DEMO = r"e:\afsim\ysim\build\demos\air_to_air"

def run_e2e(round_id):
    c_out = TEMP / f"xio_client_r{round_id}.out"
    m_out = TEMP / f"xio_mission_r{round_id}.out"
    client = subprocess.Popen(
        [CLIENT_EXE, "235.7.11.27", "192.168.", "3002", "180", "5"],
        cwd=r"e:\afsim\ysim\build\Release",
        stdout=open(c_out, "w"), stderr=subprocess.STDOUT)
    time.sleep(3)
    mission = subprocess.Popen(
        # -rt 实时模式（选项须置于文件名之前）；想定 end_time 45s
        [MISSION_EXE, "-rt", XIO_BLOCK, SCENARIO], cwd=CWD_DEMO,
        stdout=open(m_out, "w"), stderr=subprocess.STDOUT)
    m_rc = mission.wait(timeout=300)
    c_rc = client.wait(timeout=60)

    result_line = [l for l in c_out.read_text(encoding="utf-8", errors="replace").splitlines()
                   if l.startswith("RESULT ")]
    stats = json.loads(result_line[-1][7:]) if result_line else {}
    m_text = m_out.read_text(encoding="utf-8", errors="replace")
    c_text = c_out.read_text(encoding="utf-8", errors="replace")
    # 平台级指令集：mission 侧执行计数（每条上行指令一次 executed 日志）
    mission_cmds = len(re.findall(r"ysim_xio_gateway: executed command", m_text))
    unknown_pkts = len(re.findall(r"Received unknown packet", c_text))
    return {"m_rc": m_rc, "c_rc": c_rc, "stats": stats,
            "mission_cmds": mission_cmds, "unknown_pkts": unknown_pkts}

def run_regression():
    """无 xio_interface 块：插件闲置告警 + 仿真正常完成"""
    out = TEMP / "xio_regress_r.out"
    p = subprocess.run([MISSION_EXE, "2v2.txt"], cwd=CWD_DEMO,
                       stdout=open(out, "w"), stderr=subprocess.STDOUT, timeout=300)
    text = out.read_text(encoding="utf-8", errors="replace")
    return {"rc": p.returncode,
            "idle_warn": "XIO gateway disabled" in text,
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
        check("client 退出码 0", r["c_rc"] == 0, f"rc={r['c_rc']}")
        s = r["stats"]
        check("XIO 连接建立", s.get("connected") is True)
        check("sim_start 恰 1 次（连接后补发）", s.get("sim_start") == 1)
        check("sim_complete 恰 1 次", s.get("sim_complete") == 1)
        # air_movers 想定：4 初始平台 + add_platform 注入 1 靶标 = 5
        check("平台增事件 5 个（4 初始+1 动态注入）", s.get("platform_added") == 5,
              f"added={s.get('platform_added')}")
        check("平台事件增删平衡（含结束清扫）",
              s.get("platform_added") == s.get("platform_removed"),
              f"removed={s.get('platform_removed')}")
        check("状态流规模（>=100 帧，RT 1x）", s.get("state_pkts", 0) >= 100,
              f"state_pkts={s.get('state_pkts')}")
        check("末帧仿真时间达 45s（end_time）", abs(s.get("last_sim_time", 0) - 45.0) < 1.0,
              f"last_sim_time={s.get('last_sim_time')}")
        check("上行命令 mission 侧全量执行", r["mission_cmds"] == s.get("commands_sent"),
              f"client={s.get('commands_sent')} mission={r['mission_cmds']}")
        # ---- 平台级指令集断言（10 条指令：8 OK / 1 ERROR / 1 UNKNOWN）----
        check("正向指令全 OK 回执（8 条）", s.get("acks_ok") == 8,
              f"acks_ok={s.get('acks_ok')}")
        check("delete_platform 幽灵平台 ERROR 回执（1 条）", s.get("acks_error") == 1,
              f"acks_error={s.get('acks_error')}")
        check("未知指令 UNKNOWN 回执（负向 1 条）", s.get("acks_unknown") == 1,
              f"acks_unknown={s.get('acks_unknown')}")
        check("pause 期间仿真时间冻结", s.get("pause_frozen") is True,
              f"pause_frozen={s.get('pause_frozen')}")
        check("pause 期间状态流停更（增量<=2）", 0 <= s.get("state_stall_delta", -1) <= 2,
              f"delta={s.get('state_stall_delta')}")
        # get_status 于 t=15s：4 初始平台 + 靶标在场 = 5
        check("get_status 回报在场平台 5 个", s.get("status_platforms", -1) == 5,
              f"platforms={s.get('status_platforms')}")
        # ---- 靶标生命周期与运动学断言 ----
        check("靶标 add_platform 事件收到", s.get("drone_added") is True)
        check("靶标 delete_platform 事件收到", s.get("drone_removed") is True)
        check("靶标状态流帧数（>=10）", s.get("drone_state_pkts", 0) >= 10,
              f"drone_state_pkts={s.get('drone_state_pkts')}")
        # set_route 60 → set_speed 90（5 m/s² 加速能力，存在期 t=2~17s）
        check("靶标调速生效（最大速度 20~95 m/s）",
              20 <= s.get("drone_max_speed", -1) <= 95,
              f"drone_max_speed={s.get('drone_max_speed')}")
        # 弧度修复+双航点防瞬移：机动窗口仅 8s（t=4~8 加速 + t=13~17，
        # pause 冻结 5s），5 m/s² 加速度下位移百米级；瞬移缺陷版实测 233km
        check("靶标平滑飞行无瞬移（50~5000m）",
              50 <= s.get("drone_moved_m", -1) <= 5000,
              f"drone_moved_m={s.get('drone_moved_m')}")
        check("未知包告警清零", r["unknown_pkts"] == 0, f"count={r['unknown_pkts']}")
    if regress:
        print("--- 降级回归（无 xio_interface 块）---")
        check("mission 退出码 0", regress["rc"] == 0)
        check("插件闲置告警出现", regress["idle_warn"])
        check("无 FATAL", regress["no_fatal"])
    return ok

def main():
    regression_only = "--regression" in sys.argv
    rounds = []
    if not regression_only:
        for i in (1, 2):
            r = run_e2e(i)
            r["_id"] = i
            rounds.append(r)
            print(f"round {i}: {r['stats']} mission_cmds={r['mission_cmds']} unknown={r['unknown_pkts']}")
    regress = run_regression()
    ok = assert_all(rounds, regress)
    print("\n结论：平台级指令集端到端验证 " + ("全部 PASS" if ok else "存在 FAIL"))
    sys.exit(0 if ok else 1)

if __name__ == "__main__":
    main()
