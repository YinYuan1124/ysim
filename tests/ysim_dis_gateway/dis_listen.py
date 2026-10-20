# -*- coding: utf-8 -*-
# 二次开发（ext-02 阶段 B 批次 B1 验证资产）：最小 DIS 监听客户端
#
# 职责：加入 DIS 多播组接收 PDU，解析 PDU 头与 EntityState（type=1）体的
#       Entity ID + Entity Type 七段码，按实体聚合并输出 RESULT JSON 行
#       （供 dis_b1_verify.py 断言消费）。
# 依据：DIS PDU 大端定长头 12B（version/exercise/type/family/timestamp/
#       length/pad）；EntityState 体偏移 12 起：EntityID 6B（site/app/entity
#       各 u16）+ ForceId 1B + pad 1B + EntityType 8B（kind u8/domain u8/
#       country u16/category u8/subcategory u8/specific u8/extra u8）。
# 退出：收到首个包后 IDLE_TIMEOUT 秒无新包（mission 退出后包流停止），
#       或绝对超时 MAX_WAIT 秒兜底。

import json
import socket
import struct
import sys
import time

GROUP = "224.2.25.54"
PORT = 3225
IFACE = "192.168.5.3"
IDLE_TIMEOUT = 3.0
MAX_WAIT = 60.0

PDU_TYPE_NAMES = {
    1: "EntityState", 2: "Fire", 3: "Detonation", 4: "Collision",
    13: "StartResume", 14: "StopFreeze", 15: "Acknowledge",
    23: "Emission", 25: "Transmitter", 26: "Signal", 27: "Receiver",
}


def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("", PORT))
    mreq = struct.pack("=4s4s", socket.inet_aton(GROUP), socket.inet_aton(IFACE))
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
    sock.settimeout(0.5)

    entities = {}   # "site:app:ent" -> {"type": [7段], "seen": n, "marking": str}
    pdu_counts = {}  # type -> count
    total = 0
    start = time.time()
    last_pkt = time.time()

    while True:
        now = time.time()
        if total > 0 and (now - last_pkt) > IDLE_TIMEOUT:
            break
        if (now - start) > MAX_WAIT:
            break
        try:
            data, addr = sock.recvfrom(4096)
        except socket.timeout:
            continue
        last_pkt = time.time()
        total += 1
        if len(data) < 12:
            continue
        ver, exercise, pdu_type, family = data[0], data[1], data[2], data[3]
        (length,) = struct.unpack_from(">H", data, 8)
        pdu_counts[pdu_type] = pdu_counts.get(pdu_type, 0) + 1

        if pdu_type == 1 and len(data) >= 20 + 8:  # EntityState
            site, app, ent = struct.unpack_from(">HHH", data, 12)
            force_id = data[18]  # ForceId（1=友方 2=敌方……人读信息）
            # EntityType 起始偏移 20：EntityID 6B + ForceId 1B + pad 1B
            kind, domain = data[20], data[21]
            (country,) = struct.unpack_from(">H", data, 22)
            cat, sub, spec, extra = data[24], data[25], data[26], data[27]
            key = "%d:%d:%d" % (site, app, ent)
            rec = entities.setdefault(
                key, {"type": [kind, domain, country, cat, sub, spec, extra],
                      "seen": 0, "marking": "", "force": force_id})
            rec["seen"] += 1
            # Marking 尝试解析（偏移 128：charset 1B + 11B 字符；仅人读日志，
            # 不参与断言——失败即跳过）
            if not rec["marking"] and len(data) >= 128 + 12:
                try:
                    raw = data[129:140]
                    rec["marking"] = raw.decode("ascii", errors="ignore").rstrip("\x00").strip()
                except Exception:
                    pass

    country_counts = {}
    for rec in entities.values():
        c = str(rec["type"][2])
        country_counts[c] = country_counts.get(c, 0) + 1

    result = {
        "RESULT": {
            "duration_s": round(time.time() - start, 2),
            "total_pdus": total,
            "pdu_type_counts": {str(k): v for k, v in pdu_counts.items()},
            "pdu_type_names": {str(k): PDU_TYPE_NAMES.get(k, "?") for k in pdu_counts},
            "entity_count": len(entities),
            "entities": entities,
            "country_counts": country_counts,
        }
    }
    print(json.dumps(result, ensure_ascii=False))
    sock.close()


if __name__ == "__main__":
    sys.exit(main())
