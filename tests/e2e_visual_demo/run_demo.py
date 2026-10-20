# -*- coding: utf-8 -*-
# ****************************************************************************
# 二次开发（e2e 控制面板）：一键演示桥接器
# 职责：拉起/重启 mission.exe 仿真进程；HTTP 服务态势页；WebSocket /ws 端点
#       将浏览器控制指令（pause/resume/set_rate/status/reset）转发到仿真
#       控制通道（TCP 31610 行协议），reset 通过重启仿真进程实现。
# 用法：python run_demo.py [--http-port 8080] [--no-browser] [--scenario 路径]
# ****************************************************************************
import argparse
import base64
import functools
import hashlib
import json
import os
import select
import socket
import struct
import subprocess
import sys
import threading
import time
import webbrowser
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

# 控制台乱码修复：管道/终端模式下 Python 默认按系统 locale(GBK) 编码输出中文，
# 而 cmd/现代终端按 UTF-8 解码（或相反组合），强制 UTF-8 覆盖两种场景。
# （_WindowsConsoleIO 本就期望 UTF-8 字节，reconfigure 对真控制台无副作用）
if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    sys.stderr.reconfigure(encoding="utf-8", errors="replace")

HERE = Path(__file__).resolve().parent
MISSION_EXE = r"e:\afsim\ysim\build\Release\mission.exe"
CWD_DEMO = r"e:\afsim\ysim\build\demos\air_to_air"
DEFAULT_SCENARIO = str(HERE / "scenarios" / "e2e_control_scenario.txt")
TEMP = Path(r"e:\afsim\ysim\temp")
MISSION_OUT = TEMP / "e2e_demo_mission.out"

CTRL_HOST, CTRL_PORT = "127.0.0.1", 31610
DATA_HOST, DATA_PORT = "127.0.0.1", 31601
WS_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"


# ----------------------------------------------------------------------------
# 残留实例清理：上次演示异常退出（窗口被强关等）会留下 mission.exe 残进程，
# 占着 316xx 端口导致本次 mission 起不来/插件 bind 失败、8080 冲突导致桥接器
# 崩溃（浏览器页面于是无数据）。启动前检测并自动清理（仅杀 python/mission）。
# ----------------------------------------------------------------------------
def cleanup_stale(http_port):
    stale_pids = {}  # pid -> name
    try:
        rows = subprocess.check_output(
            ["netstat", "-ano", "-p", "TCP"], text=True, errors="replace")
    except Exception:
        return
    watch = {http_port, 31600, 31601, 31602, 31610}
    for ln in rows.splitlines():
        parts = ln.split()
        if len(parts) >= 5 and parts[3] == "LISTENING" and parts[-1].isdigit():
            local = parts[1]
            port = int(local.rsplit(":", 1)[1]) if ":" in local else -1
            if port in watch:
                stale_pids[int(parts[-1])] = "?"
    if not stale_pids:
        return
    # 只杀 python/mission（8080 可能被无关程序占用，不能误伤）
    for pid in list(stale_pids):
        try:
            name = subprocess.check_output(
                ["tasklist", "/FI", "PID eq %d" % pid, "/FO", "CSV", "/NH"],
                text=True, errors="replace").strip().split('","')[0].strip('"')
        except Exception:
            name = ""
        if name.lower() in ("python.exe", "pythonw.exe", "mission.exe"):
            stale_pids[pid] = name
        else:
            del stale_pids[pid]
    for pid, name in stale_pids.items():
        print("[demo] 清理残留 %s (PID %d)" % (name, pid))
        subprocess.call(["taskkill", "/F", "/T", "/PID", str(pid)],
                        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    if stale_pids:
        time.sleep(1.5)  # 等端口释放


# ----------------------------------------------------------------------------
# 仿真进程管理：拉起 mission.exe 并维持到控制通道的长连接
# ----------------------------------------------------------------------------
class Mission:
    def __init__(self, scenario):
        self.scenario = scenario
        self.proc = None
        self.ctrl = None
        self.lock = threading.Lock()

    def start(self):
        out = open(MISSION_OUT, "a")
        try:
            self.proc = subprocess.Popen(
                [MISSION_EXE, "-rt", self.scenario], cwd=CWD_DEMO,
                stdout=out, stderr=subprocess.STDOUT)
        finally:
            out.close()
        return self._connect_ctrl()

    def _connect_ctrl(self):
        deadline = time.time() + 30
        while time.time() < deadline and self.proc and self.proc.poll() is None:
            try:
                self.ctrl = socket.create_connection((CTRL_HOST, CTRL_PORT), timeout=1)
                return True
            except OSError:
                time.sleep(0.2)
        return False

    def command(self, line):
        with self.lock:
            if self.ctrl is None and not self._connect_ctrl():
                return {"type": "control_result", "ok": False,
                        "detail": "control channel unavailable"}
            try:
                self.ctrl.sendall((line + "\n").encode())
                buf = b""
                while b"\n" not in buf:
                    chunk = self.ctrl.recv(4096)
                    if not chunk:
                        raise ConnectionError("control channel closed")
                    buf += chunk
                return json.loads(buf.decode().strip())
            except (OSError, ConnectionError) as exc:
                self.ctrl = None
                return {"type": "control_result", "ok": False, "detail": str(exc)}

    def reset(self):
        with self.lock:
            self._stop_locked()
            return self.start()

    def _stop_locked(self):
        if self.ctrl:
            try:
                self.ctrl.close()
            except OSError:
                pass
            self.ctrl = None
        if self.proc:
            self.proc.terminate()
            try:
                self.proc.wait(timeout=10)
            except subprocess.TimeoutExpired:
                self.proc.kill()
            self.proc = None

    def stop(self):
        with self.lock:
            self._stop_locked()


# ----------------------------------------------------------------------------
# WebSocket 帧编解码（RFC 6455 服务端视角：客户端帧带掩码，服务端帧不掩码）
# ----------------------------------------------------------------------------
def recv_exact(sock, n):
    data = b""
    while len(data) < n:
        chunk = sock.recv(n - len(data))
        if not chunk:
            raise ConnectionError("socket closed")
        data += chunk
    return data


def ws_read_frame(sock):
    hdr = recv_exact(sock, 2)
    opcode = hdr[0] & 0x0F
    ln = hdr[1] & 0x7F
    masked = hdr[1] & 0x80
    if ln == 126:
        ln = struct.unpack(">H", recv_exact(sock, 2))[0]
    elif ln == 127:
        ln = struct.unpack(">Q", recv_exact(sock, 8))[0]
    mask = recv_exact(sock, 4) if masked else b""
    data = recv_exact(sock, ln) if ln else b""
    if masked:
        data = bytes(b ^ mask[i % 4] for i, b in enumerate(data))
    return opcode, data


def ws_send_text(sock, text):
    data = text.encode("utf-8")
    n = len(data)
    if n < 126:
        hdr = struct.pack(">BB", 0x81, n)
    elif n < 65536:
        hdr = struct.pack(">BBH", 0x81, 126, n)
    else:
        hdr = struct.pack(">BBQ", 0x81, 127, n)
    sock.sendall(hdr + data)


def ws_handshake(handler):
    key = handler.headers.get("Sec-WebSocket-Key", "")
    accept = base64.b64encode(
        hashlib.sha1((key + WS_GUID).encode()).digest()).decode()
    handler.send_response(101)
    handler.send_header("Upgrade", "websocket")
    handler.send_header("Connection", "Upgrade")
    handler.send_header("Sec-WebSocket-Accept", accept)
    handler.end_headers()


# ----------------------------------------------------------------------------
# /data 数据流转发端点：浏览器 → 桥接器(8080) → 插件 WS 推流(31601)
# 用途：系统代理/安全软件拦截非常规端口 31601 时，页面自动降级走本端点
#       （8080 能开页面即能通数据）；亦支持远程机器访问浏览器页面。
# 转发为帧级字节直通：31601 下行帧本就是"服务端帧"（不掩码），原样透传。
# ----------------------------------------------------------------------------
def ws_connect_upstream(host, port):
    """以客户端身份与上游 WS 服务器握手，返回 socket 与剩余缓冲。"""
    key = base64.b64encode(os.urandom(16)).decode()
    req = ("GET / HTTP/1.1\r\nHost: %s:%d\r\nUpgrade: websocket\r\n"
           "Connection: Upgrade\r\nSec-WebSocket-Key: %s\r\n"
           "Sec-WebSocket-Version: 13\r\n\r\n" % (host, port, key))
    sock = socket.create_connection((host, port), timeout=5)
    sock.sendall(req.encode())
    buf = b""
    while b"\r\n\r\n" not in buf:
        chunk = sock.recv(4096)
        if not chunk:
            raise ConnectionError("upstream closed during handshake")
        buf += chunk
    head, _, rest = buf.partition(b"\r\n\r\n")
    if b"101" not in head.split(b"\r\n")[0]:
        sock.close()
        raise ConnectionError("upstream refused websocket upgrade")
    accept = ""
    for ln in head.decode("latin-1").split("\r\n"):
        if ln.lower().startswith("sec-websocket-accept:"):
            accept = ln.split(":", 1)[1].strip()
    expect = base64.b64encode(
        hashlib.sha1((key + WS_GUID).encode()).digest()).decode()
    if accept != expect:
        sock.close()
        raise ConnectionError("upstream accept key mismatch")
    return sock, rest


# ----------------------------------------------------------------------------
# HTTP/WS 处理器：/ws 为控制端点，其余按静态文件服务（/ 默认态势页）
# ----------------------------------------------------------------------------
def make_handler(mission):
    class DemoHandler(SimpleHTTPRequestHandler):
        protocol_version = "HTTP/1.1"  # WebSocket 升级要求 HTTP/1.1
        def end_headers(self):
            # 禁缓存：确保浏览器每次拿到最新页面（旧标签页 F5 即得新版，杜绝旧 JS 驻留）
            self.send_header("Cache-Control", "no-store, must-revalidate")
            self.send_header("Pragma", "no-cache")
            super().end_headers()

        def do_GET(self):
            path = self.path.split("?")[0]
            if path == "/ws":
                self._ws_session()
            elif path == "/data":
                self._data_session()
            elif self.path in ("/", "/index.html"):
                self.path = "/situation.html"
                super().do_GET()
            else:
                super().do_GET()

        def _data_session(self):
            """数据流转发会话：上游(31601)帧透传浏览器，浏览器 close 即断开。"""
            try:
                up, rest = ws_connect_upstream(DATA_HOST, DATA_PORT)
            except (ConnectionError, OSError):
                self.send_error(502, "upstream data stream unavailable")
                return
            ws_handshake(self)  # 先确认上游可用再向浏览器回 101，避免假成功
            browser = self.connection
            browser.settimeout(None)
            up.settimeout(None)
            try:
                if rest:
                    browser.sendall(rest)  # 握手响应随带的首批数据帧
                while True:
                    rlist, _, _ = select.select([browser, up], [], [])
                    if up in rlist:
                        chunk = up.recv(65536)
                        if not chunk:
                            break  # 上游关闭（仿真结束/重置）→ 结束会话
                        browser.sendall(chunk)
                    if browser in rlist:
                        opcode, _ = ws_read_frame(browser)
                        if opcode == 0x8:  # 浏览器主动断开
                            try:
                                browser.sendall(struct.pack(">BB", 0x88, 0))
                            except OSError:
                                pass
                            break
            except (ConnectionError, OSError):
                pass  # 任一侧断开：结束转发会话
            finally:
                try:
                    up.close()
                except OSError:
                    pass

        def _ws_session(self):
            ws_handshake(self)
            try:
                while True:
                    opcode, data = ws_read_frame(self.connection)
                    if opcode == 0x8:  # close → 回送 close 帧后结束会话
                        try:
                            self.connection.sendall(struct.pack(">BB", 0x88, 0))
                        except OSError:
                            pass
                        break
                    if opcode == 0x9:  # ping → pong
                        self.connection.sendall(
                            struct.pack(">BB", 0x8A, 0))
                        continue
                    if opcode != 0x1:  # 仅文本帧
                        continue
                    resp = self._dispatch(json.loads(data.decode("utf-8")))
                    ws_send_text(self.connection,
                                 json.dumps(resp, ensure_ascii=False))
            except (ConnectionError, OSError, json.JSONDecodeError):
                pass  # 客户端断开或异常报文：结束会话

        def _dispatch(self, msg):
            action = msg.get("action", "")
            if action == "reset":
                ok = mission.reset()
                return {"type": "control_result", "ok": bool(ok),
                        "detail": "simulation restarted", "action": "reset"}
            if action in ("pause", "resume", "status"):
                resp = mission.command(action)
            elif action == "set_rate":
                resp = mission.command("set_rate %s" % msg.get("value", 1))
            else:
                resp = {"type": "control_result", "ok": False,
                        "detail": "unknown action: %s" % action}
            resp["action"] = action
            return resp

        def log_message(self, fmt, *args):
            pass  # 静默静态文件访问日志

    return functools.partial(DemoHandler, directory=str(HERE))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--http-port", type=int, default=8080)
    ap.add_argument("--scenario", default=DEFAULT_SCENARIO)
    ap.add_argument("--no-browser", action="store_true")
    args = ap.parse_args()

    TEMP.mkdir(exist_ok=True)
    cleanup_stale(args.http_port)  # 清理上次演示残留（强关窗口遗留的 mission/桥接器）
    mission = Mission(args.scenario)
    if not mission.start():
        print("[demo] mission 启动失败或控制通道未就绪，退出")
        return 1

    try:
        server = ThreadingHTTPServer(("127.0.0.1", args.http_port),
                                      make_handler(mission))
    except OSError as exc:
        mission.stop()
        print("[demo] 端口 %d 绑定失败（%s）——可能被其他程序占用，"
              "换端口请用 --http-port" % (args.http_port, exc))
        return 1
    url = "http://127.0.0.1:%d/" % args.http_port
    print("[demo] 仿真已启动（起步暂停），控制态势页: %s （Ctrl+C 退出并结束仿真）" % url)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    if not args.no_browser:
        time.sleep(0.5)
        webbrowser.open(url)

    def heartbeat():
        """控制台心跳：每 5s 打印仿真时间/平台数/暂停状态。
        用户窗口自此持续滚动输出——桥接器死活/仿真推进与否一眼可辨，
        消除"控制台没有任何数据"的诊断黑洞。"""
        while True:
            st = mission.command("status")
            if st.get("type") == "sim_status":
                print("[demo] 仿真时间 %.1fs | 平台 %d | %s | 倍率 %s×" % (
                    st.get("sim_time", 0), st.get("platform_count", 0),
                    "已暂停" if st.get("paused") else "运行中",
                    st.get("clock_rate", 1)), flush=True)
            else:
                print("[demo] 控制通道无响应: %s" % st.get("detail", "?"), flush=True)
            time.sleep(5)

    threading.Thread(target=heartbeat, daemon=True).start()
    try:
        while True:
            time.sleep(3600)
    except KeyboardInterrupt:
        print("\n[demo] 正在结束仿真进程…")
    finally:
        server.shutdown()
        mission.stop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
