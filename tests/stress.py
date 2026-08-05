import random
import socket
import threading

THREAD_COUNT = 100
REQUESTS_PER_THREAD = 100

STOP_ON_FAILURE = True
SUCCESS_STATUS = 200

HOST = "127.0.0.1"
PORT = 8080
HOST_HEADER = "meinewebsite.com"
TIMEOUT = 120

stop_flag = threading.Event()
lock = threading.Lock()
ok_count = 0
fail_count = 0


def rand_name():
    return "".join(random.choices("abcdefghijklmnopqrstuvwxyz", k=8))


def build(method, path, body):
    body = body.encode() if isinstance(body, str) else body
    head = (
        f"{method} {path} HTTP/1.1\r\n"
        f"Host: {HOST_HEADER}\r\n"
        f"Connection: close\r\n"
        f"Content-Length: {len(body)}\r\n\r\n"
    )
    return head.encode() + body


def read_response(sock):
    buf = b""
    while b"\r\n\r\n" not in buf:
        chunk = sock.recv(4096)
        if not chunk:
            return buf
        buf += chunk
    head, _, rest = buf.partition(b"\r\n\r\n")
    length = 0
    for line in head.split(b"\r\n")[1:]:
        if line.lower().startswith(b"content-length:"):
            length = int(line.split(b":")[1].strip())
    while len(rest) < length:
        chunk = sock.recv(4096)
        if not chunk:
            break
        rest += chunk
    return head + b"\r\n\r\n" + rest[:length]


def status_of(raw):
    if not raw.startswith(b"HTTP/1."):
        return None
    parts = raw.split(b"\r\n", 1)[0].decode(errors="replace").split(" ")
    return int(parts[1]) if len(parts) > 1 and parts[1].isdigit() else None


def build_steps():
    if random.random() < 0.2:
        method = random.choice(["GET", "POST"])
        body = f"hello={rand_name()}" if method == "POST" else ""
        return [(method, "/cgi-bin/script.py", body)]

    method = random.choice(["GET", "POST"])  # , "DELETE"])
    if method == "GET":
        return [("GET", "/", "")]
    if method == "POST":
        return [("POST", "/submit", f"payload={rand_name()}")]

    name = f"/upload/{rand_name()}.txt"
    return [("POST", name, f"seed {rand_name()}"), ("DELETE", name, "")]


def worker(thread_id):
    global ok_count, fail_count
    sock = None
    port = "no port available"

    def ensure_connection():
        nonlocal sock
        if sock is None:
            sock = socket.socket()
            sock.settimeout(TIMEOUT)
            sock.connect((HOST, PORT))

    for _ in range(REQUESTS_PER_THREAD):
        if stop_flag.is_set():
            break

        status, raw, method, path = None, b"", None, None
        try:
            for method, path, body in build_steps():
                ensure_connection()
                sock.sendall(build(method, path, body))
                port = sock.getsockname()[1]
                raw = read_response(sock)
                status = status_of(raw)
                sock.close()
                sock = None
        except Exception as e:
            status, raw = None, str(e).encode()
            if sock is not None:
                try:
                    sock.close()
                except OSError:
                    pass
                sock = None
        finally:
            if sock is not None:
                sock.close()
                sock = None

        with lock:
            if status == 200 or method == "POST" and status == 201:
                ok_count += 1
            else:
                fail_count += 1
                print(
                    f"[FAIL] thread={thread_id} port={port} "
                    f"{method} {path} -> status={status}\n       raw: {raw[:200]!r}"
                )
                if STOP_ON_FAILURE:
                    stop_flag.set()

    if sock is not None:
        sock.close()


threads = [threading.Thread(target=worker, args=(i,)) for i in range(THREAD_COUNT)]
for t in threads:
    t.start()
for t in threads:
    t.join()

print(
    f"\nDone. ok={ok_count} fail={fail_count}"
    + (" (STOP_ON_FAILURE)" if stop_flag.is_set() else "")
)
