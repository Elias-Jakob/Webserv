#!/usr/bin/env python3
"""
Einfacher Stress-Tester fuer webserv. Keine Argumente, keine Optionen --
alles was du anpassen willst steht direkt unten als Konstante.

Jeder Thread entscheidet EINMAL bei seinem Start ob er seine Verbindung
per Keep-Alive haelt oder pro Request neu verbindet (Haelfte/Haelfte nach
Thread-Nummer). Pro Request wird dann ausgewuerfelt: ~20% CGI (GET oder
POST auf /cgi-bin/script.py), sonst gleichverteilt GET/POST/DELETE auf
statische bzw. Upload-Pfade. DELETE braucht eine Datei die wirklich
existiert -- dafuer wird kurz vorher hochgeladen (nicht extra gezaehlt/
geprueft, nur Vorbereitung).

Fehlerkriterium bewusst simpel: alles ausser Status 200 ist ein Fail.
STOP_ON_FAILURE bricht dann sofort den gesamten Test ab, damit die
Server-Logs beim Debuggen nicht mit Folgefehlern vollgeschrieben werden.
Zum Beobachten wie sich der Server NACH einem Fehler weiterverhaelt,
einfach auf False stellen.
"""

import random
import socket
import threading

# ---- die zwei angeforderten Konstanten ----
THREAD_COUNT = 100
REQUESTS_PER_THREAD = 10000

STOP_ON_FAILURE = False  # False = nach einem Fehler einfach weiterlaufen lassen
SUCCESS_STATUS = 200  # Achtung: ein erfolgreicher Upload liefert bei vielen
# Implementierungen 201 Created statt 200 -- ggf. anpassen

HOST = "127.0.0.1"
PORT = 8080
HOST_HEADER = "meinewebsite.com"
TIMEOUT = 60

stop_flag = threading.Event()
lock = threading.Lock()
ok_count = 0
fail_count = 0


def rand_name():
    return "".join(random.choices("abcdefghijklmnopqrstuvwxyz", k=8))


def build(method, path, body, keep_alive):
    body = body.encode() if isinstance(body, str) else body
    head = (
        f"{method} {path} HTTP/1.1\r\n"
        f"Host: {HOST_HEADER}\r\n"
        f"Connection: {'keep-alive' if keep_alive else 'close'}\r\n"
        f"Content-Length: {len(body)}\r\n\r\n"
    )
    return head.encode() + body


def read_response(sock):
    """Liest Header bis zur Leerzeile, dann genau Content-Length Bytes Body.
    Reicht fuer diesen Tester (kein HEAD, keine Chunked-Responses erwartet)."""
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
    if length == 0 or len(rest) == 0:
        print("Received response with content-length = 0")
    # stop_flag.set()
    return head + b"\r\n\r\n" + rest[:length]


def status_of(raw):
    if not raw.startswith(b"HTTP/1."):
        return None
    parts = raw.split(b"\r\n", 1)[0].decode(errors="replace").split(" ")
    return int(parts[1]) if len(parts) > 1 and parts[1].isdigit() else None


def build_steps():
    """Ein 'Zug' ist normalerweise ein einzelner Request. Nur bei DELETE
    sind es zwei Schritte (erst hochladen, dann loeschen) -- nur der
    letzte Schritt zaehlt fuer ok/fail."""
    if random.random() < 0.2:  # ~ jeder fuenfte Request ist CGI
        method = random.choice(["GET", "POST"])
        body = f"hello={rand_name()}" if method == "POST" else ""
        return [(method, "/cgi-bin/script.py", body)]

    return [("GET", "/", "")]
    method = random.choice(["GET", "POST", "DELETE"])
    if method == "GET":
        return [("GET", "/", "")]
    if method == "POST":
        return [("POST", f"/upload/{rand_name()}.txt", f"payload {rand_name()}")]

    name = f"/upload/{rand_name()}.txt"
    return [("POST", name, f"seed {rand_name()}"), ("DELETE", name, "")]


def worker(thread_id):
    global ok_count, fail_count
    keep_alive = thread_id % 2 == 0
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
                sock.sendall(build(method, path, body, keep_alive))
                port = sock.getsockname()[1]
                raw = read_response(sock)
                status = status_of(raw)
                if not keep_alive:
                    sock.close()
                    sock = None
        except Exception as e:
            status, raw = None, str(e).encode()
        finally:
            if not keep_alive and sock is not None:
                sock.close()
                sock = None

        with lock:
            if status == SUCCESS_STATUS:
                ok_count += 1
            else:
                fail_count += 1
                print(
                    f"[FAIL] thread={thread_id} port={port} keep_alive={keep_alive} "
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
    f"\nFertig. ok={ok_count} fail={fail_count}"
    + (" (durch STOP_ON_FAILURE vorzeitig abgebrochen)" if stop_flag.is_set() else "")
)
