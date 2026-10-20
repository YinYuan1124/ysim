# -*- coding: utf-8 -*-
"""ext-02 阶段 B 批次 B2：DIS 增强捕获客户端（dis_listen.py 增强版）

在 B1 监听范式（join 多播组 + 大端解析 + RESULT JSON 行）基础上，增加
Fire(2)/Detonation(3)/Emission(23)/Signal(26) 四类 PDU 的关键字段抽取。

字段偏移依据 Dis 库序列化实现（Put 顺序，IEEE 1278.1 大端）：
  公共头 12B：version(1) exercise(1) pdu_type(1) family(1) timestamp(4) length(2) pad(2)
  Fire(2)      : firing(6) target(6) weapon(6) event(6) fireMission(4B!) location(24)
                 weaponType(8: kind@64 domain@65 country@66-68 category@68 subcat@69)
                 warhead@72(2) fuse@74(2) quantity@76(2) rate@78(2) velocity(12) range(4)
  Detonation(3): firing(6) target(6) weapon(6) event(6) velocity(12) location(24)
                 weaponType(8) warhead@80(2) fuse@82(2) quantity(2) rate(2)
                 impactLocation(12) result@100(1) ...
  Emission(23) : emitting(6) event(6) stateUpdate@24(1) numSystems@25(1) pad(2)
                 系统1: dataLen(2) numBeams(1) pad(2) name@32(2) function@34(1) number@35(1)
                 （偏移经 2026-10-19 抓包实测校准：name@32/function@34/number@35）
  Signal(26)   : entity(6) radio(2) encoding(2) tdl(2) sampleRate(4) dataLen@28(4)
                 sampleCount(2) data...（空载荷时 PDU 总长 34）
"""
import json
import socket
import struct
import sys
import time

MCAST_GROUP = "224.2.25.54"
MCAST_PORT = 3225
LOCAL_IF = "192.168.5.3"  # 与 mission dis_interface multicast 出接口一致（B1 实测参数）
IDLE_EXIT_SEC = 6.0       # 静默退出：mission 结束后 DIS 心跳停止
MAX_RUN_SEC = 180.0


def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("", MCAST_PORT))
    mreq = struct.pack("=4s4s", socket.inet_aton(MCAST_GROUP), socket.inet_aton(LOCAL_IF))
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
    sock.settimeout(1.0)

    stats = {
        "total_pdus": 0,
        "pdu_type_counts": {},
        "fires": [],
        "detonations": [],
        "emission_nonzero_name": 0,
        "emission_nonzero_function": 0,
        "emission_names": [],
        "signals": 0,
        "signals_empty_payload": 0,
    }

    t0 = time.time()
    last_pdu = time.time()
    while True:
        now = time.time()
        if now - t0 > MAX_RUN_SEC:
            break
        if stats["total_pdus"] > 0 and now - last_pdu > IDLE_EXIT_SEC:
            break
        try:
            data, _addr = sock.recvfrom(65535)
        except socket.timeout:
            continue
        if len(data) < 12:
            continue
        last_pdu = time.time()
        stats["total_pdus"] += 1
        _ver, _ex, pdu_type, _fam = data[0], data[1], data[2], data[3]
        pdu_len = struct.unpack_from(">H", data, 10)[0]
        stats["pdu_type_counts"][str(pdu_type)] = stats["pdu_type_counts"].get(str(pdu_type), 0) + 1

        try:
            if pdu_type == 2 and len(data) >= 96:  # Fire（fireMissionIndex 4B → 总长 96B）
                kind, domain = data[64], data[65]
                country = struct.unpack_from(">H", data, 66)[0]
                category, subcat = data[68], data[69]
                warhead = struct.unpack_from(">H", data, 72)[0]
                fuse = struct.unpack_from(">H", data, 74)[0]
                quantity = struct.unpack_from(">H", data, 76)[0]
                stats["fires"].append({
                    "kind": kind, "domain": domain, "country": country,
                    "category": category, "subcategory": subcat,
                    "warhead": warhead, "fuse": fuse, "quantity": quantity,
                })
            elif pdu_type == 3 and len(data) >= 101:  # Detonation
                warhead = struct.unpack_from(">H", data, 80)[0]
                fuse = struct.unpack_from(">H", data, 82)[0]
                result = data[100]
                stats["detonations"].append({"warhead": warhead, "fuse": fuse, "result": result})
            elif pdu_type == 23 and len(data) >= 36:  # Electromagnetic Emission
                num_systems = data[25]
                if num_systems >= 1:
                    name = struct.unpack_from(">H", data, 32)[0]
                    function = data[34]
                    if name != 0:
                        stats["emission_nonzero_name"] += 1
                    if function != 0:
                        stats["emission_nonzero_function"] += 1
                    stats["emission_names"].append({"name": name, "function": function})
            elif pdu_type == 26:  # Signal
                stats["signals"] += 1
                if len(data) >= 32:
                    data_len_bits = struct.unpack_from(">I", data, 28)[0]
                    if data_len_bits == 0 and pdu_len <= 34:
                        stats["signals_empty_payload"] += 1
        except (IndexError, struct.error):
            # 长度不足的畸形包：计数但不中断捕获
            continue

    sock.close()
    # 人读明细限幅（Emission 帧率高）
    stats["emission_sample"] = stats["emission_names"][:5]
    del stats["emission_names"]
    print(json.dumps({"RESULT": stats}, ensure_ascii=False))


if __name__ == "__main__":
    main()
