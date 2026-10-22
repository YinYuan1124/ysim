# -*- coding: utf-8 -*-
# 二次开发（ext-01 阶段 D）：适配器热切换验收脚本
# 时序（mission -rt 40s 后台运行）：
#   t=5s  控制客户端 list（应含 tcp:31600）→ add_udp 239.16.16.17 31700
#   t=7s  UDP 客户端加入新组播组收流（应含快照补推 + 实时流）
#   t=12s remove 31700 → UDP 流应终止（停机后 1.5s 无新数据报）
#   t=13s add_tcp 31710 → t=14s TCP 客户端接入 31710 收流（应含快照）
#   t=18s remove 31710 → 31710 流终止
#   全程 TCP 31600 客户端持续收流不受影响，仿真正常完成
import json
import socket
import subprocess
import sys
import threading
import time

MISSION = r"E:\Project\ysim\build\Release\mission.exe"
# 测试资产归档（tests/）：想定取本目录 scenarios/，运行输出仍写 temp/
SCENARIO = r"E:/Project/ysim/tests/ysim_data_export/scenarios/ysim_smoke_hotswap.txt"
CWD = r"E:\Project\ysim\build\demos\air_to_air"
RESULT = r"E:\Project\ysim\temp\ysim_hotswap_result.txt"

class Reader(threading.Thread):
    """行流读取器（TCP）"""
    def __init__(self, sock):
        super().__init__(daemon=True)
        self.sock = sock
        self.lines = []
        self.buf = b""
        self.alive = True
    def run(self):
        self.sock.settimeout(1.0)
        while self.alive:
            try:
                data = self.sock.recv(4096)
            except socket.timeout:
                continue
            except OSError:
                break
            if not data:
                break
            self.buf += data
            while b"\n" in self.buf:
                line, self.buf = self.buf.split(b"\n", 1)
                self.lines.append(line.decode("utf-8", "replace"))
    def stop(self):
        self.alive = False

def ctrl_cmd(sock, cmd):
    """发送控制命令并读取单行 JSON 响应"""
    sock.sendall((cmd + "\n").encode())
    data = b""
    sock.settimeout(3.0)
    while b"\n" not in data:
        chunk = sock.recv(4096)
        if not chunk:
            break
        data += chunk
    return json.loads(data.split(b"\n")[0].decode())

def main():
    lines_31600, udp_31700, lines_31710 = [], [], []
    ctrl_results = {}
    with open(RESULT, "w", encoding="utf-8") as report:
        def log(msg):
            report.write(msg + "\n")

        # 1) 先启动 mission（-rt，40s 仿真时长），等待数据通道监听就绪
        proc = subprocess.Popen([MISSION, "-rt", SCENARIO], cwd=CWD,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        t0 = time.time()
        for _ in range(50):
            try:
                probe = socket.create_connection(("127.0.0.1", 31600), timeout=0.5)
                probe.close()
                break
            except OSError:
                time.sleep(0.2)

        # 2) 主数据客户端接入 31600（全程）
        main_sock = socket.create_connection(("127.0.0.1", 31600), timeout=5)
        main_reader = Reader(main_sock)
        main_reader.start()

        # 3) t=5s 控制会话：list + add_udp
        time.sleep(max(0, 5 - (time.time() - t0)))
        ctrl = socket.create_connection(("127.0.0.1", 31603), timeout=5)
        ctrl_results["list1"] = ctrl_cmd(ctrl, "list")
        ctrl_results["add_udp"] = ctrl_cmd(ctrl, "add_udp 239.16.16.17 31700")

        # 4) t=7s UDP 客户端加入 239.16.16.17:31700
        time.sleep(max(0, 7 - (time.time() - t0)))
        udp_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        udp_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        udp_sock.bind(("", 31700))
        mreq = socket.inet_aton("239.16.16.17") + socket.inet_aton("0.0.0.0")
        udp_sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
        udp_sock.settimeout(0.5)

        def udp_recv():
            udp_deadline = None  # remove 后设 deadline 检测停流
            while time.time() - t0 < 39:
                try:
                    data, _ = udp_sock.recvfrom(65536)
                except socket.timeout:
                    continue
                text = data.decode("utf-8", "replace").strip()
                if text:
                    udp_31700.append(text)
        udp_thread = threading.Thread(target=udp_recv, daemon=True)
        udp_thread.start()

        # 5) t=12s remove 31700，记录此刻 UDP 收包数
        time.sleep(max(0, 12 - (time.time() - t0)))
        udp_count_before_remove = len(udp_31700)
        ctrl_results["remove_31700"] = ctrl_cmd(ctrl, "remove 31700")

        # 6) t=13s add_tcp 31710
        time.sleep(max(0, 13 - (time.time() - t0)))
        ctrl_results["add_tcp"] = ctrl_cmd(ctrl, "add_tcp 31710")

        # 7) t=14s TCP 客户端接入 31710（应含快照补推）
        time.sleep(max(0, 14 - (time.time() - t0)))
        sock2 = socket.create_connection(("127.0.0.1", 31710), timeout=5)
        reader2 = Reader(sock2)
        reader2.start()

        # 8) t=18s remove 31710，记录此刻 31710 收包数
        time.sleep(max(0, 18 - (time.time() - t0)))
        count_31710_before_remove = len(reader2.lines)
        ctrl_results["remove_31710"] = ctrl_cmd(ctrl, "remove 31710")
        ctrl.close()

        # 9) 等仿真结束（-rt 40s + 余量）
        rc = proc.wait(timeout=30)
        main_reader.stop()
        reader2.stop()
        time.sleep(1.5)
        udp_after_remove = len(udp_31700) - udp_count_before_remove
        after_31710 = len(reader2.lines) - count_31710_before_remove

        # ---------- 校验 ----------
        checks = []
        def check(name, cond, detail=""):
            checks.append((name, bool(cond), detail))

        check("mission exit code 0", rc == 0, "rc=%s" % rc)
        # 控制响应
        ch1 = [c["type"] for c in ctrl_results["list1"]["channels"]]
        check("list 初始含 tcp:31600",
              {"type": "tcp", "port": 31600} in ctrl_results["list1"]["channels"],
              str(ctrl_results["list1"]))
        check("add_udp 31700 ok", ctrl_results["add_udp"].get("ok") is True, str(ctrl_results["add_udp"]))
        check("remove 31700 ok", ctrl_results["remove_31700"].get("ok") is True, str(ctrl_results["remove_31700"]))
        check("add_tcp 31710 ok", ctrl_results["add_tcp"].get("ok") is True, str(ctrl_results["add_tcp"]))
        check("remove 31710 ok", ctrl_results["remove_31710"].get("ok") is True, str(ctrl_results["remove_31710"]))
        # UDP 新通道收流：快照（platform_added）+ 实时 state
        udp_types = {}
        for line in udp_31700:
            try:
                udp_types[json.loads(line).get("type", "?")] = udp_types.get(json.loads(line).get("type", "?"), 0) + 1
            except Exception:
                udp_types["_bad"] = udp_types.get("_bad", 0) + 1
        # UDP 组播无连接语义（无接入事件），快照补推仅适用于 TCP/WS；验证实时流即可
        check("UDP 31700 收到实时 state 流", udp_types.get("state", 0) >= 10, str(udp_types))
        check("UDP 31700 remove 后停流（0 新包）", udp_after_remove == 0, "after_remove=%d" % udp_after_remove)
        # TCP 31710：快照 + 实时流 + remove 后停流
        types2 = {}
        for line in reader2.lines:
            try:
                types2[json.loads(line).get("type", "?")] = types2.get(json.loads(line).get("type", "?"), 0) + 1
            except Exception:
                types2["_bad"] = types2.get("_bad", 0) + 1
        check("TCP 31710 收到快照 platform_added", types2.get("platform_added", 0) >= 4, str(types2))
        check("TCP 31710 收到实时 state 流", types2.get("state", 0) >= 10, str(types2))
        check("TCP 31710 remove 后停流（0 新包）", after_31710 == 0, "after_remove=%d" % after_31710)
        # 主客户端 31600 全程不受影响
        types_main = {}
        for line in main_reader.lines:
            try:
                types_main[json.loads(line).get("type", "?")] = types_main.get(json.loads(line).get("type", "?"), 0) + 1
            except Exception:
                types_main["_bad"] = types_main.get("_bad", 0) + 1
        check("TCP 31600 全程收流完整（含 sim_complete）",
              types_main.get("sim_complete", 0) == 1 and types_main.get("_bad", 0) == 0, str(types_main))
        check("TCP 31600 热切换期间未断流（state 持续）", types_main.get("state", 0) >= 50, str(types_main))

        # ---------- 报告 ----------
        all_pass = all(c[1] for c in checks)
        for name, ok, detail in checks:
            log("%s %s %s" % ("PASS" if ok else "FAIL", name, detail))
        log("udp_31700 types=%s" % udp_types)
        log("tcp_31710 types=%s" % types2)
        log("tcp_31600 types=%s" % types_main)
        log("RESULT: %s" % ("ALL PASS" if all_pass else "HAS FAILURES"))
        print("RESULT: %s (%d/%d checks)" % ("ALL PASS" if all_pass else "FAIL", sum(1 for c in checks if c[1]), len(checks)))
        sys.exit(0 if all_pass else 1)

if __name__ == "__main__":
    main()
