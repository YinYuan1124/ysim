# -*- coding: utf-8 -*-
# 二次开发（ext-01 阶段 C）：结构化落盘冒烟校验脚本
# 校验内容：
#   1. run_* 目录三 CSV 存在且表头正确
#   2. events.csv 含全生命周期事件（sim_start/4x platform_added/4x platform_removed/sim_complete）
#   3. platform_states.csv 行数与 SQLite states 表一致；数值列可解析
#   4. SQLite 四表行数；platforms 快照表最终状态非空
#   5. metrics：CSV 与 SQLite 行数一致，平台级 5 指标 x 4 平台 + 运行级指标齐全
#   6. 指标正确性抽查：survival_time 与 events 时间差一致
import csv
import glob
import io
import json
import os
import sqlite3
import sys

ROOT = r"E:\afsim\ysim\temp\ysim_record_out"

def main():
    runs = sorted(glob.glob(os.path.join(ROOT, "run_*")))
    assert runs, "no run_* directory under " + ROOT
    run = runs[-1]
    print("run dir:", run)

    # 1) CSV 文件与表头
    states_path = os.path.join(run, "platform_states.csv")
    events_path = os.path.join(run, "events.csv")
    metrics_path = os.path.join(run, "metrics.csv")
    db_path = os.path.join(run, "ysim_export.db")
    for p in (states_path, events_path, metrics_path, db_path):
        assert os.path.isfile(p), "missing file: " + p

    def read_csv(path):
        # UTF-8 with BOM
        with io.open(path, "r", encoding="utf-8-sig", newline="") as f:
            return list(csv.DictReader(f))

    states = read_csv(states_path)
    events = read_csv(events_path)
    metrics = read_csv(metrics_path)
    print("csv rows: states=%d events=%d metrics=%d" % (len(states), len(events), len(metrics)))
    assert len(states) > 0 and len(events) > 0 and len(metrics) > 0

    # 表头校验
    assert list(states[0].keys()) == ["time","platform_idx","name","type","side","lat","lon","alt_m","speed_mps","heading_deg","pitch_deg","roll_deg"], states[0].keys()
    assert list(events[0].keys()) == ["time","event","platform_idx","name"], events[0].keys()
    assert list(metrics[0].keys()) == ["metric","platform_idx","name","side","value","unit"], metrics[0].keys()

    # 2) 事件完整性
    ev_types = [e["event"] for e in events]
    assert ev_types.count("sim_start") == 1, ev_types
    assert ev_types.count("platform_added") == 4, ev_types
    assert ev_types.count("platform_removed") == 4, ev_types
    assert ev_types.count("sim_complete") == 1, ev_types

    # 3) 数值列可解析 + id 集合
    state_ids = set()
    for s in states:
        for col in ("time","lat","lon","alt_m","speed_mps","heading_deg","pitch_deg","roll_deg"):
            float(s[col])
        state_ids.add(int(s["platform_idx"]))
    added_ids = {int(e["platform_idx"]) for e in events if e["event"] == "platform_added"}
    assert state_ids == added_ids, (state_ids, added_ids)

    # 4) SQLite 四表
    con = sqlite3.connect(db_path)
    cur = con.cursor()
    tables = {r[0] for r in cur.execute("SELECT name FROM sqlite_master WHERE type='table'")}
    assert tables == {"platforms","states","events","metrics"}, tables
    db_states = cur.execute("SELECT COUNT(*) FROM states").fetchone()[0]
    db_events = cur.execute("SELECT COUNT(*) FROM events").fetchone()[0]
    db_metrics = cur.execute("SELECT COUNT(*) FROM metrics").fetchone()[0]
    db_platforms = cur.execute("SELECT COUNT(*) FROM platforms").fetchone()[0]
    print("db rows: states=%d events=%d metrics=%d platforms=%d" % (db_states, db_events, db_metrics, db_platforms))
    assert db_states == len(states), (db_states, len(states))
    assert db_events == len(events), (db_events, len(events))
    assert db_metrics == len(metrics), (db_metrics, len(metrics))
    assert db_platforms == 4, db_platforms

    # platforms 快照表：最终状态非空（哨兵 -1e30 之外）
    for row in cur.execute("SELECT platform_idx,name,type,side,lat,lon,alt_m,removed_time FROM platforms"):
        idx,name,typ,side,lat,lon,alt,removed = row
        assert name and typ and side, row
        assert abs(lat) < 1e29 and abs(lon) < 1e29 and abs(alt) < 1e29, row
        assert removed is not None and removed >= 0, row  # 2v2 想定 4 平台全部被移除

    # 5) metrics 齐全性：平台级 5 指标 x 4 平台 + 运行级
    plat_metrics = {}
    for m in metrics:
        if int(m["platform_idx"]) != 0:
            plat_metrics.setdefault(int(m["platform_idx"]), set()).add(m["metric"])
    assert len(plat_metrics) == 4, plat_metrics
    for idx, mset in plat_metrics.items():
        assert mset == {"survival_time","state_samples","avg_speed","max_speed","distance_m"}, (idx, mset)
    run_metrics = {m["metric"] for m in metrics if int(m["platform_idx"]) == 0}
    assert {"platform_count","sim_duration","total_state_samples"} <= run_metrics, run_metrics
    for key in run_metrics:
        assert key.startswith("side_count_") or key in ("platform_count","sim_duration","total_state_samples"), key

    # 6) 指标正确性抽查：survival_time = removed - added（events 时间）
    added_t = {int(e["platform_idx"]): float(e["time"]) for e in events if e["event"] == "platform_added"}
    removed_t = {int(e["platform_idx"]): float(e["time"]) for e in events if e["event"] == "platform_removed"}
    for m in metrics:
        if m["metric"] == "survival_time":
            idx = int(m["platform_idx"])
            expect = removed_t[idx] - added_t[idx]
            assert abs(float(m["value"]) - expect) < 1e-3, (idx, m["value"], expect)
    # state_samples 与 CSV 行数一致
    csv_samples = {}
    for s in states:
        csv_samples[int(s["platform_idx"])] = csv_samples.get(int(s["platform_idx"]), 0) + 1
    for m in metrics:
        if m["metric"] == "state_samples":
            idx = int(m["platform_idx"])
            assert float(m["value"]) == csv_samples[idx], (idx, m["value"], csv_samples[idx])
    # total_state_samples = states 总行数
    for m in metrics:
        if m["metric"] == "total_state_samples":
            assert float(m["value"]) == len(states), (m["value"], len(states))
    con.close()

    print("ALL CHECKS PASSED")

if __name__ == "__main__":
    main()
