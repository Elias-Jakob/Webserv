#!/usr/bin/env python3
"""
webserv_stress.py -- concurrent stress tester for a C++98 webserv project.

Mixes static-file requests, uploads (POST + cleanup DELETE), and CGI
requests (GET + POST) across many concurrent workers, to surface issues
that only show up under load: partial sends, fd/process exhaustion, or
race conditions in the event loop.

IMPORTANT: this deliberately never pipelines requests (sending request
N+1 before reading response N), since that HTTP/1.1 feature is often not
implemented. Every request either opens a brand new connection (default)
or, with --keep-alive, reuses one connection per worker strictly
*sequentially*: send, wait for the complete response, only then send the
next request.

Requires cgi_echo.py (shipped alongside webserv_tester.py) to be present
and executable wherever your /cgi-bin location's alias points to.

Usage:
  python3 webserv_stress.py --workers 40 --duration 30
  python3 webserv_stress.py --workers 40 --requests 4000
  python3 webserv_stress.py --workers 40 --duration 30 --only cgi
  python3 webserv_stress.py --workers 40 --duration 30 --keep-alive
  python3 webserv_stress.py --host 127.0.0.1 --port 8080 --workers 40 --duration 20
"""

import argparse
import random
import socket
import string
import sys
import threading
import time

# ---------------------------------------------------------------------------
# Config -- matches the assumptions in webserv_tester.py
# ---------------------------------------------------------------------------
HOST = "127.0.0.1"
PORT = 8080
HOST_HEADER_VALUE = "meinewebsite.com"
CGI_ECHO = "/cgi-bin/cgi_echo.py"
UPLOAD_PATH = "/upload"

CONNECT_TIMEOUT = 3.0
READ_TIMEOUT = 10.0  # generous margin under load -- cgi_echo.py itself is not slow

PRINT_INTERVAL = 2.0  # seconds between live progress lines


# ---------------------------------------------------------------------------
# HTTP-aware response reader: reads exactly one response and stops (does
# not wait out a timeout on a healthy connection, because a correctly
# behaving server has no reason to close the socket after one response).
# ---------------------------------------------------------------------------

def read_one_http_response(sock, read_timeout, max_bytes=1 << 20, leftover=b"", request_method=None):
    sock.settimeout(read_timeout)
    buf = leftover
    while b"\r\n\r\n" not in buf:
        chunk = sock.recv(4096)
        if not chunk:
            return buf, b""
        buf += chunk
        if len(buf) >= max_bytes:
            return buf, b""

    head, _, rest = buf.partition(b"\r\n\r\n")
    status = None
    first_line = head.split(b"\r\n", 1)[0].decode(errors="replace").split(" ")
    if len(first_line) >= 2 and first_line[1].isdigit():
        status = int(first_line[1])

    if request_method == b"HEAD" or status in (204, 304):
        return head + b"\r\n\r\n", rest

    content_length = None
    is_chunked = False
    for line in head.split(b"\r\n")[1:]:
        low = line.lower()
        if low.startswith(b"content-length:"):
            try:
                content_length = int(line.split(b":", 1)[1].strip())
            except ValueError:
                content_length = None
        elif low.startswith(b"transfer-encoding:") and b"chunked" in low:
            is_chunked = True

    if content_length is not None:
        while len(rest) < content_length:
            chunk = sock.recv(4096)
            if not chunk:
                break
            rest += chunk
        body, extra = rest[:content_length], rest[content_length:]
        return head + b"\r\n\r\n" + body, extra

    if is_chunked:
        data = rest
        while not (data.endswith(b"0\r\n\r\n") or b"\r\n0\r\n\r\n" in data):
            chunk = sock.recv(4096)
            if not chunk:
                break
            data += chunk
            if len(data) >= max_bytes:
                break
        return head + b"\r\n\r\n" + data, b""

    data = rest
    try:
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            data += chunk
            if len(data) >= max_bytes:
                break
    except socket.timeout:
        pass
    return head + b"\r\n\r\n" + data, b""


def parse_status(raw: bytes):
    if not raw:
        return None
    line = raw.split(b"\r\n", 1)[0]
    if not (line.startswith(b"HTTP/1.0") or line.startswith(b"HTTP/1.1")):
        return None  # malformed / garbled response -- see classify_response()
    parts = line.decode(errors="replace").split(" ")
    if len(parts) >= 2 and parts[1].isdigit():
        return int(parts[1])
    return None


def build_request(method, path, headers=None, body=b"", keep_alive=False):
    headers = dict(headers or {})
    lower = {k.lower() for k in headers}
    if "host" not in lower:
        headers["Host"] = HOST_HEADER_VALUE
    if isinstance(body, str):
        body = body.encode()
    if body and "content-length" not in lower:
        headers["Content-Length"] = str(len(body))
    if "connection" not in lower:
        headers["Connection"] = "keep-alive" if keep_alive else "close"
    lines = [f"{method} {path} HTTP/1.1"]
    for k, v in headers.items():
        lines.append(f"{k}: {v}")
    return ("\r\n".join(lines) + "\r\n\r\n").encode() + body


def rand_token(n=8):
    return "".join(random.choices(string.ascii_lowercase, k=n))


def classify_error(exc: Exception) -> str:
    errno = getattr(exc, "errno", None)
    known = {104: "ECONNRESET", 32: "EPIPE", 111: "ECONNREFUSED", 110: "ETIMEDOUT"}
    if errno in known:
        return known[errno]
    if isinstance(exc, (socket.timeout, TimeoutError)):
        return "TIMEOUT"
    if errno is not None:
        return f"OSError(errno={errno})"
    return type(exc).__name__


# ---------------------------------------------------------------------------
# Request catalogue. Each "simple" generator returns one (method, path,
# headers, body). Uploads are a paired POST+DELETE so a long run doesn't
# leave garbage files behind.
# ---------------------------------------------------------------------------

def req_static_index():
    return "GET", "/", {}, b""


def req_static_www():
    return "GET", "/www/", {}, b""


def req_static_head():
    return "HEAD", "/www/", {}, b""


def req_redirect():
    return "GET", "/redir", {}, b""


def req_cgi_get():
    q = f"n={rand_token()}&t={int(time.time() * 1000)}"
    return "GET", f"{CGI_ECHO}?{q}", {}, b""


def req_cgi_post():
    body = f"payload={rand_token(32)}".encode()
    return "POST", CGI_ECHO, {"Content-Type": "application/x-www-form-urlencoded"}, body


STATIC_REQUESTS = [req_static_index, req_static_www, req_static_head, req_redirect]
CGI_REQUESTS = [req_cgi_get, req_cgi_post]


# ---------------------------------------------------------------------------
# Stats
# ---------------------------------------------------------------------------

class Stats:
    def __init__(self):
        self.lock = threading.Lock()
        self.records = []  # (category, ok, status_or_None, error_label_or_None, latency)
        self.malformed_samples = []  # keep a few raw samples of garbled responses for inspection

    def add(self, category, ok, status, error_label, latency, raw_sample=None):
        with self.lock:
            self.records.append((category, ok, status, error_label, latency))
            if raw_sample is not None and len(self.malformed_samples) < 5:
                self.malformed_samples.append((category, raw_sample))

    def snapshot_count(self):
        with self.lock:
            return len(self.records)


def percentile(sorted_vals, p):
    if not sorted_vals:
        return 0.0
    k = int(round((len(sorted_vals) - 1) * p))
    return sorted_vals[k]


def print_report(stats: Stats, elapsed: float):
    with stats.lock:
        records = list(stats.records)

    total = len(records)
    print(f"\n{'=' * 70}\nSTRESS TEST REPORT -- {total} requests in {elapsed:.1f}s "
          f"({total / elapsed if elapsed > 0 else 0:.1f} req/s)\n{'=' * 70}")

    by_cat = {}
    for cat, ok, status, err, lat in records:
        by_cat.setdefault(cat, {"ok": 0, "errors": {}, "statuses": {}, "latencies": []})
        d = by_cat[cat]
        d["latencies"].append(lat)
        if ok:
            d["ok"] += 1
            d["statuses"][status] = d["statuses"].get(status, 0) + 1
        else:
            d["errors"][err] = d["errors"].get(err, 0) + 1

    for cat in sorted(by_cat):
        d = by_cat[cat]
        n = len(d["latencies"])
        lat_sorted = sorted(d["latencies"])
        print(f"\n[{cat}] {n} requests, {d['ok']} ok, {n - d['ok']} failed")
        if d["statuses"]:
            status_str = ", ".join(f"{code}: {c}" for code, c in sorted(d["statuses"].items()))
            print(f"  status codes:  {status_str}")
        if d["errors"]:
            err_str = ", ".join(f"{label}: {c}" for label, c in sorted(d["errors"].items()))
            print(f"  ERRORS:        {err_str}")
        if lat_sorted:
            print(f"  latency (ms):  min={lat_sorted[0]*1000:.1f}  "
                  f"p50={percentile(lat_sorted, 0.50)*1000:.1f}  "
                  f"p95={percentile(lat_sorted, 0.95)*1000:.1f}  "
                  f"max={lat_sorted[-1]*1000:.1f}")

    total_errors = sum(1 for _, ok, _, _, _ in records if not ok)
    econnreset = sum(1 for _, ok, _, err, _ in records if not ok and err == "ECONNRESET")
    print(f"\n{'-' * 70}")
    print(f"TOTAL: {total} requests, {total - total_errors} ok, {total_errors} failed")
    if econnreset:
        print(f"  -> {econnreset} of those were ECONNRESET specifically")
    if stats.malformed_samples:
        print(f"\n{len(stats.malformed_samples)} malformed/garbled response sample(s) "
              f"(response didn't start with a valid HTTP status line -- possible sign of "
              f"corrupted output, e.g. sending stale/oversized buffer contents):")
        for cat, sample in stats.malformed_samples:
            preview = sample[:120]
            print(f"  [{cat}] {preview!r}{'...' if len(sample) > 120 else ''}")
    print(f"{'=' * 70}")


# ---------------------------------------------------------------------------
# Worker
# ---------------------------------------------------------------------------

def do_single_request(method, path, headers, body, keep_alive, sock=None):
    """
    Sends one request and reads exactly one full response. If `sock` is
    given, reuses it (caller is responsible for the keep-alive contract:
    never send request N+1 before this call for request N has returned).
    Returns (ok, status, error_label, latency, sock_or_None, raw_bytes).
    sock_or_None is the (possibly still-open) socket for reuse, or None if
    it was closed / broke.
    """
    req = build_request(method, path, headers, body, keep_alive=keep_alive)
    t0 = time.time()
    owns_socket = sock is None
    try:
        if sock is None:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(CONNECT_TIMEOUT)
            sock.connect((HOST, PORT))
        sock.settimeout(READ_TIMEOUT)
        sock.sendall(req)
        raw, _leftover = read_one_http_response(
            sock, read_timeout=READ_TIMEOUT, request_method=method.encode())
        latency = time.time() - t0
        status = parse_status(raw)
        if status is None:
            if not keep_alive:
                sock.close()
                sock = None
            return False, None, "MALFORMED_RESPONSE", latency, sock, raw
        if not keep_alive:
            sock.close()
            sock = None
        return True, status, None, latency, sock, raw
    except Exception as e:  # noqa: BLE001 -- deliberately broad: any failure is a stress-test finding
        latency = time.time() - t0
        try:
            if sock is not None:
                sock.close()
        except OSError:
            pass
        return False, None, classify_error(e), latency, None, b""


def worker_loop(worker_id, stats: Stats, stop_event, only_category, keep_alive, req_counter, req_target):
    persistent_sock = None
    rng = random.Random(worker_id * 7919 + 1)

    def take_slot():
        if req_target is None:
            return True
        with req_counter["lock"]:
            if req_counter["n"] >= req_target:
                return False
            req_counter["n"] += 1
            return True

    while not stop_event.is_set():
        if not take_slot():
            break

        choices = []
        if only_category in (None, "static"):
            choices += [("static", fn) for fn in STATIC_REQUESTS]
        if only_category in (None, "cgi"):
            choices += [("cgi", fn) for fn in CGI_REQUESTS]
        if only_category in (None, "upload"):
            choices += [("upload", None)]  # handled specially below
        cat, fn = rng.choice(choices)

        if cat == "upload":
            name = f"/stress_{rand_token()}_{worker_id}.txt"
            body = f"stress payload {rand_token(16)}".encode()
            ok, status, err, lat, s, raw = do_single_request(
                "POST", UPLOAD_PATH + name, {"Content-Type": "text/plain"}, body,
                keep_alive, persistent_sock if keep_alive else None)
            stats.add("upload", ok, status, err, lat, raw if err == "MALFORMED_RESPONSE" else None)
            if keep_alive:
                persistent_sock = s
            # best-effort cleanup, always attempted, own try/except so a
            # failed cleanup never kills the worker
            ok2, status2, err2, lat2, s2, raw2 = do_single_request(
                "DELETE", UPLOAD_PATH + name, {}, b"",
                keep_alive, persistent_sock if keep_alive else None)
            stats.add("upload", ok2, status2, err2, lat2, raw2 if err2 == "MALFORMED_RESPONSE" else None)
            if keep_alive:
                persistent_sock = s2
        else:
            method, path, headers, body = fn()
            ok, status, err, lat, s, raw = do_single_request(
                method, path, headers, body, keep_alive, persistent_sock if keep_alive else None)
            stats.add(cat, ok, status, err, lat, raw if err == "MALFORMED_RESPONSE" else None)
            if keep_alive:
                persistent_sock = s

    if persistent_sock is not None:
        try:
            persistent_sock.close()
        except OSError:
            pass


def progress_printer(stats: Stats, stop_event, t_start):
    last_n = 0
    while not stop_event.wait(PRINT_INTERVAL):
        n = stats.snapshot_count()
        elapsed = time.time() - t_start
        rate = (n - last_n) / PRINT_INTERVAL
        print(f"  ... {n} requests so far, {elapsed:.0f}s elapsed, ~{rate:.0f} req/s (last {PRINT_INTERVAL:.0f}s)")
        last_n = n


def main():
    global HOST, PORT
    ap = argparse.ArgumentParser(description=__doc__,
                                  formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--host", default=HOST)
    ap.add_argument("--port", type=int, default=PORT)
    ap.add_argument("--workers", type=int, default=40)
    ap.add_argument("--duration", type=float, default=None,
                     help="run for this many seconds (default 20s if --requests not given)")
    ap.add_argument("--requests", type=int, default=None,
                     help="stop after this many total requests instead of using a duration")
    ap.add_argument("--only", choices=["static", "cgi", "upload"], default=None,
                     help="restrict to one request category")
    ap.add_argument("--keep-alive", action="store_true",
                     help="each worker reuses ONE connection sequentially "
                          "(send, wait for full response, then next -- never pipelined) "
                          "instead of opening a fresh connection per request")
    args = ap.parse_args()

    HOST, PORT = args.host, args.port

    duration = args.duration
    if duration is None and args.requests is None:
        duration = 20.0

    stats = Stats()
    stop_event = threading.Event()
    req_counter = {"lock": threading.Lock(), "n": 0}

    mode = "keep-alive (sequential, non-pipelined)" if args.keep_alive else "fresh connection per request"
    print(f"Stress-testing {HOST}:{PORT} with {args.workers} workers "
          f"[{mode}]"
          + (f", duration={duration:.0f}s" if duration else f", requests={args.requests}")
          + (f", only={args.only}" if args.only else ", mix=static+cgi+upload"))

    t_start = time.time()
    threads = [threading.Thread(target=worker_loop, args=(
        i, stats, stop_event, args.only, args.keep_alive, req_counter, args.requests))
        for i in range(args.workers)]
    printer = threading.Thread(target=progress_printer, args=(stats, stop_event, t_start), daemon=True)

    try:
        for t in threads:
            t.start()
        printer.start()
        if duration is not None:
            stop_event.wait(duration)
            stop_event.set()
        for t in threads:
            t.join()
    except KeyboardInterrupt:
        print("\nInterrupted -- stopping workers and printing what we have so far...")
        stop_event.set()
        for t in threads:
            t.join(timeout=5)

    elapsed = time.time() - t_start
    stop_event.set()
    print_report(stats, elapsed)

    total_errors = sum(1 for _, ok, _, _, _ in stats.records if not ok)
    sys.exit(1 if total_errors else 0)


if __name__ == "__main__":
    main()
