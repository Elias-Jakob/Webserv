#!/usr/bin/env python3
"""
webserv_tester.py -- test suite for a C++98 "webserv" project (42-style).

Covers: the subject's non-blocking / single-poll() requirements, HTTP/1.0 and
HTTP/1.1 basics, CGI/1.1 (RFC 3875), and the specific config below. No
external dependencies -- Python 3 standard library only.

Written against this config:

    server {
        listen = 127.0.0.1:8080;
        listen = 127.0.0.1:8081;
        listen = 127.0.0.1:8082;
        listen = [::1]:8084;
        server_name = meinewebsite.com;
        error_page 404 = /errors/404.html;
        error_page 500 = /errors/500.html;
        client_max_body_size = 2K;
        root = /www;

        location / { accepted_methods = GET, POST; index = index.html; autoindex = on; }
        location /www { accepted_methods = GET, POST, HEAD; autoindex = on; }
        location /redir { return = 301 https://google.com; }
        location /upload { accepted_methods = POST, DELETE; upload_enable = on;
                            upload_store = /uploads; upload_extensions = .txt, .pdf; }
        location /submit { alias = /submit; accepted_methods = POST; form_output_file = form_file.txt; }
        location /cba { alias = /abc; accepted_methods = GET; autoindex = on; }
        location /cgi-bin { alias = /cgi-bin; accepted_methods = GET, POST; cgi_extension = .php, .py; }
    }

Port 8083 is commented out in the config on purpose -- it must NOT be
reachable, and that is tested explicitly.

ASSUMPTIONS you may need to adjust (search for "ASSUMPTION" below):
  - Uploads: POST body is written verbatim as the file's content, and the
    URI segment after /upload/ is used as the filename. If your server
    instead expects multipart/form-data, adjust `upload_then_delete_roundtrip`
    and the two client_max_body_size tests accordingly.
  - "2K" in client_max_body_size is interpreted as 2048 bytes. If your
    config parser treats K as 1000, change CLIENT_MAX_BODY below.
  - CGI scripts: this suite ships four helper scripts (cgi_echo.py,
    cgi_sleep.py, cgi_status.py, cgi_crash.py) that must be placed in
    whatever directory your /cgi-bin location's alias resolves to on the
    server, and made executable (chmod +x).

Usage:
    python3 webserv_tester.py                    # run everything
    python3 webserv_tester.py --list             # list all test names, don't run
    python3 webserv_tester.py --only cgi         # run tests whose name/tag contains "cgi"
    python3 webserv_tester.py --skip-slow        # skip slow tests (timeouts, stress)
    python3 webserv_tester.py --host 127.0.0.1 --port 8080
"""

import argparse
import random
import socket
import string
import sys
import threading
import time

# ---------------------------------------------------------------------------
# CONFIG -- adjust to match your server's config file and filesystem layout
# ---------------------------------------------------------------------------
HOST = "127.0.0.1"
PORT = 8080                        # primary port used by most tests
ALL_PORTS = [8080, 8081, 8082]      # every `listen` in the config (IPv4 ones)
CLOSED_PORT = 8083                  # commented out in the config -> must refuse
IPV6_HOST = "::1"
IPV6_PORT = 8084

DOC_ROOT_INDEX = "/index.html"      # location / -> index.html under root /www
STATIC_DIR = "/www"                 # location /www
REDIR_PATH = "/redir"               # location /redir -> 301 https://google.com
UPLOAD_PATH = "/upload"             # location /upload
UPLOAD_ALLOWED_EXT = ".txt"
UPLOAD_FORBIDDEN_EXT = ".exe"
ALIAS_PATH = "/cba"                 # location /cba -> alias /abc
CGI_DIR = "/cgi-bin"                # location /cgi-bin, cgi_extension .php, .py
CGI_ECHO = CGI_DIR + "/cgi_echo.py"
CGI_SLEEP = CGI_DIR + "/cgi_sleep.py"
CGI_STATUS = CGI_DIR + "/cgi_status.py"
CGI_CRASH = CGI_DIR + "/cgi_crash.py"

CLIENT_MAX_BODY = 2 * 1024          # "2K" -> 2048 bytes; see ASSUMPTION above
CGI_TIMEOUT_HINT = 8                # seconds; set >= your server's real CGI timeout
IDLE_TIMEOUT_HINT = 12              # seconds; set >= your KEEP_ALIVE_TIMEOUT

CONNECT_TIMEOUT = 3.0
READ_TIMEOUT = 5.0

HOST_HEADER_VALUE = "meinewebsite.com"

# ---------------------------------------------------------------------------
# Minimal raw HTTP client. Deliberately NOT http.client/requests, so we can
# send malformed / partial / chunked / pipelined bytes on purpose.
# ---------------------------------------------------------------------------


class RawResponse:
    def __init__(self, raw: bytes):
        self.raw = raw
        self.status = None
        self.reason = ""
        self.headers = {}
        self.body = b""
        self._parse(raw)

    def _parse(self, raw: bytes):
        if b"\r\n\r\n" in raw:
            head, _, self.body = raw.partition(b"\r\n\r\n")
        else:
            head, self.body = raw, b""
        lines = head.split(b"\r\n")
        if not lines or not lines[0]:
            return
        parts = lines[0].decode(errors="replace").split(" ", 2)
        if len(parts) >= 2 and parts[1].isdigit():
            self.status = int(parts[1])
            self.reason = parts[2] if len(parts) > 2 else ""
        for line in lines[1:]:
            if b":" in line:
                k, _, v = line.partition(b":")
                self.headers[k.decode(errors="replace").strip().lower()] = \
                    v.decode(errors="replace").strip()

    def header(self, name, default=None):
        return self.headers.get(name.lower(), default)

    def __repr__(self):
        return f"<RawResponse {self.status} {self.reason!r} body={len(self.body)}B>"


def read_one_http_response(sock, read_timeout=READ_TIMEOUT, max_bytes=1 << 20, leftover=b""):
    """
    Reads exactly one HTTP response from an already-connected socket: the
    header block up to the blank line, then either exactly Content-Length
    body bytes, or a chunked body up to its terminating 0-chunk, or (if
    neither header is present) reads until the peer closes the connection.

    This matters a lot on a keep-alive connection: a naive "recv until
    timeout" loop would always burn the full timeout, because a correctly
    behaving server has no reason to close the socket after one response.
    Knowing where the response actually ends lets tests return as soon as
    the server has actually answered.

    `leftover` lets a caller feed back bytes it already read past the end
    of a previous response (used for pipelining). Returns (response_bytes,
    unconsumed_leftover_bytes).
    """
    sock.settimeout(read_timeout)
    buf = leftover
    while b"\r\n\r\n" not in buf:
        chunk = sock.recv(4096)
        if not chunk:
            return buf, b""  # connection closed before headers even completed
        buf += chunk
        if len(buf) >= max_bytes:
            return buf, b""

    head, _, rest = buf.partition(b"\r\n\r\n")

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
        while True:
            if data.endswith(b"0\r\n\r\n") or b"\r\n0\r\n\r\n" in data:
                break
            chunk = sock.recv(4096)
            if not chunk:
                break
            data += chunk
            if len(data) >= max_bytes:
                break
        return head + b"\r\n\r\n" + data, b""

    # Neither header present (e.g. some minimal error responses): fall back
    # to reading until the connection closes or we time out.
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


def raw_request(host, port, data: bytes, read_timeout=READ_TIMEOUT,
                 connect_timeout=CONNECT_TIMEOUT, family=socket.AF_INET,
                 close_after=True, sock=None, read_all=True, max_bytes=1 << 20):
    """
    Sends raw bytes to host:port, returns (response_bytes, sock_or_None).
    Pass an existing `sock` to reuse a connection (keep-alive tests). If
    close_after is False, the socket is returned open for reuse.

    With read_all=True (default) this reads exactly one HTTP response and
    stops -- it does not wait out the full timeout on a healthy keep-alive
    connection. Set read_all=False for a single raw recv() with no framing
    awareness (rarely needed).
    """
    if sock is None:
        sock = socket.socket(family, socket.SOCK_STREAM)
        sock.settimeout(connect_timeout)
        sock.connect((host, port))
    sock.settimeout(read_timeout)
    sock.sendall(data)

    if read_all:
        result, _leftover = read_one_http_response(sock, read_timeout=read_timeout, max_bytes=max_bytes)
    else:
        try:
            result = sock.recv(4096)
        except socket.timeout:
            result = b""

    if close_after:
        sock.close()
        sock = None
    return result, sock


def build_request(method, path, headers=None, body=b"", http_version="1.1", host_header=True):
    headers = dict(headers or {})
    lower_keys = {k.lower() for k in headers}
    lines = [f"{method} {path} HTTP/{http_version}"]
    if host_header and "host" not in lower_keys:
        headers["Host"] = HOST_HEADER_VALUE
    if isinstance(body, str):
        body = body.encode()
    if body and "content-length" not in lower_keys and "transfer-encoding" not in lower_keys:
        headers["Content-Length"] = str(len(body))
    for k, v in headers.items():
        lines.append(f"{k}: {v}")
    head = ("\r\n".join(lines) + "\r\n\r\n").encode()
    return head + body


def chunk_encode(body: bytes, chunk_size=16):
    out = b""
    for i in range(0, len(body), chunk_size):
        piece = body[i:i + chunk_size]
        out += f"{len(piece):x}\r\n".encode() + piece + b"\r\n"
    out += b"0\r\n\r\n"
    return out


def rand_name(ext):
    return "/" + "".join(random.choices(string.ascii_lowercase, k=8)) + ext


# ---------------------------------------------------------------------------
# Tiny test framework
# ---------------------------------------------------------------------------

TESTS = []  # list of (name, func, tags)


def test(*tags):
    def deco(fn):
        TESTS.append((fn.__name__, fn, set(tags)))
        return fn
    return deco


class Fail(AssertionError):
    pass


def expect(cond, msg):
    if not cond:
        raise Fail(msg)


# ---------------------------------------------------------------------------
# A. Ports / config
# ---------------------------------------------------------------------------

@test("ports", "config")
def ports_all_serve_same_content():
    bodies = []
    for p in ALL_PORTS:
        resp, _ = raw_request(HOST, p, build_request("GET", "/"))
        r = RawResponse(resp)
        expect(r.status == 200, f"port {p}: expected 200, got {r.status}")
        bodies.append(r.body)
    expect(len(set(bodies)) == 1, "not all configured ports served identical content for /")


@test("ports", "config")
def closed_port_refuses_connection():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(CONNECT_TIMEOUT)
    try:
        s.connect((HOST, CLOSED_PORT))
    except (ConnectionRefusedError, socket.timeout, OSError):
        return  # expected: nobody is listening here
    finally:
        s.close()
    raise Fail(f"port {CLOSED_PORT} accepted a connection, but is commented out in the config")


@test("ports", "config", "ipv6")
def ipv6_listener_reachable():
    resp, _ = raw_request(IPV6_HOST, IPV6_PORT, build_request("GET", "/"), family=socket.AF_INET6)
    r = RawResponse(resp)
    expect(r.status == 200, f"IPv6 listener [::1]:{IPV6_PORT}: expected 200, got {r.status}")


# ---------------------------------------------------------------------------
# B. Static files
# ---------------------------------------------------------------------------

@test("static")
def index_resolves_for_directory():
    resp, _ = raw_request(HOST, PORT, build_request("GET", "/"))
    r = RawResponse(resp)
    expect(r.status == 200, f"GET / expected 200, got {r.status}")
    expect(len(r.body) > 0, "GET / returned an empty body")


@test("static")
def not_found_uses_configured_error_page():
    resp, _ = raw_request(HOST, PORT, build_request("GET", "/this-does-not-exist-xyz"))
    r = RawResponse(resp)
    expect(r.status == 404, f"expected 404, got {r.status}")
    expect(len(r.body) > 0, "404 response had an empty body -- error_page not served")


@test("static", "security")
def path_traversal_is_blocked():
    candidates = ["/../../../../etc/passwd", "/%2e%2e/%2e%2e/%2e%2e/etc/passwd",
                  "/..%2f..%2fetc/passwd", "/www/../../../etc/passwd"]
    for p in candidates:
        resp, _ = raw_request(HOST, PORT, build_request("GET", p))
        r = RawResponse(resp)
        expect(r.status in (400, 403, 404),
               f"path traversal '{p}' returned {r.status} instead of 400/403/404")
        expect(b"root:" not in r.body, f"path traversal '{p}' leaked /etc/passwd contents!")


@test("static")
def content_length_matches_body():
    resp, _ = raw_request(HOST, PORT, build_request("GET", DOC_ROOT_INDEX))
    r = RawResponse(resp)
    expect(r.status == 200, f"expected 200, got {r.status}")
    cl = r.header("content-length")
    expect(cl is not None, "no Content-Length header on static response")
    expect(int(cl) == len(r.body), f"Content-Length={cl} but body is {len(r.body)} bytes")


@test("static")
def response_has_date_and_server_headers():
    resp, _ = raw_request(HOST, PORT, build_request("GET", "/"))
    r = RawResponse(resp)
    expect(r.header("date") is not None, "missing Date header")
    expect(r.header("server") is not None, "missing Server header")


# ---------------------------------------------------------------------------
# C. Location-specific behaviour
# ---------------------------------------------------------------------------

@test("locations")
def head_matches_get_without_body():
    get_resp, _ = raw_request(HOST, PORT, build_request("GET", STATIC_DIR + "/"))
    head_resp, _ = raw_request(HOST, PORT, build_request("HEAD", STATIC_DIR + "/"))
    g, h = RawResponse(get_resp), RawResponse(head_resp)
    expect(h.status == g.status, f"HEAD status {h.status} != GET status {g.status}")
    expect(len(h.body) == 0, "HEAD response had a non-empty body")
    expect(h.header("content-length") == g.header("content-length"),
           "HEAD Content-Length doesn't match GET Content-Length")


@test("locations")
def method_not_in_accepted_methods_gives_405():
    resp, _ = raw_request(HOST, PORT, build_request("DELETE", "/"))
    r = RawResponse(resp)
    expect(r.status == 405, f"DELETE on / (only GET,POST configured): expected 405, got {r.status}")
    expect(r.header("allow") is not None, "405 response is missing the Allow header")


@test("locations")
def redirect_location_returns_301():
    resp, _ = raw_request(HOST, PORT, build_request("GET", REDIR_PATH))
    r = RawResponse(resp)
    expect(r.status == 301, f"expected 301, got {r.status}")
    loc = r.header("location")
    expect(loc == "https://google.com", f"expected Location: https://google.com, got {loc!r}")


@test("locations", "upload")
def upload_then_delete_roundtrip():
    # ASSUMPTION: raw POST body becomes the file's content; adjust if your
    # server expects multipart/form-data instead.
    fname = rand_name(UPLOAD_ALLOWED_EXT)
    body = b"hello from the tester\n"
    resp, _ = raw_request(HOST, PORT, build_request(
        "POST", UPLOAD_PATH + fname, {"Content-Type": "text/plain"}, body))
    r = RawResponse(resp)
    expect(r.status in (200, 201), f"upload POST: expected 200/201, got {r.status}")

    resp2, _ = raw_request(HOST, PORT, build_request("GET", UPLOAD_PATH + fname))
    r2 = RawResponse(resp2)
    expect(r2.status in (200, 404, 405),
           f"GET on uploaded file gave unexpected {r2.status} "
           "(405 is fine if GET isn't in accepted_methods for /upload)")

    resp3, _ = raw_request(HOST, PORT, build_request("DELETE", UPLOAD_PATH + fname))
    r3 = RawResponse(resp3)
    expect(r3.status in (200, 204), f"DELETE on uploaded file: expected 200/204, got {r3.status}")


@test("locations", "upload")
def upload_rejects_forbidden_extension():
    fname = "/malicious" + UPLOAD_FORBIDDEN_EXT
    resp, _ = raw_request(HOST, PORT, build_request(
        "POST", UPLOAD_PATH + fname, {"Content-Type": "application/octet-stream"}, b"MZ\x90\x00"))
    r = RawResponse(resp)
    expect(r.status in (400, 403, 415),
           f"upload with forbidden extension '{UPLOAD_FORBIDDEN_EXT}': expected 400/403/415, got {r.status}")


@test("locations")
def delete_on_missing_file_is_404():
    resp, _ = raw_request(HOST, PORT, build_request("DELETE", UPLOAD_PATH + "/does-not-exist-xyz.txt"))
    r = RawResponse(resp)
    expect(r.status == 404, f"DELETE on missing file: expected 404, got {r.status}")


@test("locations")
def alias_directory_is_reachable():
    resp, _ = raw_request(HOST, PORT, build_request("GET", ALIAS_PATH + "/"))
    r = RawResponse(resp)
    expect(r.status in (200, 404),
           f"GET {ALIAS_PATH}/: expected 200 (autoindex/index) or 404 (empty dir), got {r.status}")
    # NOTE: to truly prove /cba maps to /abc (not root+/cba == /www/cba), place
    # a marker file directly in /abc and assert its content shows up here.


# ---------------------------------------------------------------------------
# D. Methods
# ---------------------------------------------------------------------------

@test("methods")
def unknown_method_gives_501():
    resp, _ = raw_request(HOST, PORT, build_request("FROBNICATE", "/"))
    r = RawResponse(resp)
    expect(r.status == 501, f"unknown method: expected 501, got {r.status}")


@test("methods")
def malformed_request_line_gives_400():
    resp, _ = raw_request(HOST, PORT, b"NOT EVEN CLOSE TO HTTP\r\n\r\n")
    r = RawResponse(resp)
    expect(r.status == 400, f"garbage request line: expected 400, got {r.status}")


# ---------------------------------------------------------------------------
# E. Body-size limits (client_max_body_size = 2K)
# ---------------------------------------------------------------------------

@test("limits")
def body_within_limit_is_accepted():
    body = b"a" * (CLIENT_MAX_BODY - 16)
    resp, _ = raw_request(HOST, PORT, build_request("POST", UPLOAD_PATH + "/within_limit.txt", {}, body))
    r = RawResponse(resp)
    expect(r.status in (200, 201), f"body under client_max_body_size: expected 200/201, got {r.status}")


@test("limits")
def body_over_limit_is_rejected():
    body = b"a" * (CLIENT_MAX_BODY + 4096)
    resp, _ = raw_request(HOST, PORT, build_request("POST", UPLOAD_PATH + "/over_limit.txt", {}, body))
    r = RawResponse(resp)
    expect(r.status == 413, f"body over client_max_body_size: expected 413, got {r.status}")


# ---------------------------------------------------------------------------
# F. HTTP/1.0 vs HTTP/1.1 connection handling
# ---------------------------------------------------------------------------

@test("protocol")
def http10_defaults_to_connection_close():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(CONNECT_TIMEOUT)
    sock.connect((HOST, PORT))
    resp, sock = raw_request(HOST, PORT, build_request("GET", "/", http_version="1.0"),
                              sock=sock, close_after=False)
    r = RawResponse(resp)
    expect(r.status == 200, f"HTTP/1.0 GET /: expected 200, got {r.status}")
    try:
        sock.settimeout(1.5)
        sock.sendall(build_request("GET", "/", http_version="1.0"))
        more = sock.recv(4096)
        expect(more == b"", "HTTP/1.0 without Connection: keep-alive should close, but server kept it open")
    except (BrokenPipeError, ConnectionResetError, socket.timeout, OSError):
        pass  # expected: connection was already closed
    finally:
        sock.close()


@test("protocol")
def http11_defaults_to_keep_alive():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(CONNECT_TIMEOUT)
    sock.connect((HOST, PORT))
    resp1, sock = raw_request(HOST, PORT, build_request("GET", "/", http_version="1.1"),
                               sock=sock, close_after=False)
    r1 = RawResponse(resp1)
    expect(r1.status == 200, f"first HTTP/1.1 request: expected 200, got {r1.status}")
    resp2, sock = raw_request(HOST, PORT, build_request("GET", "/", http_version="1.1"),
                               sock=sock, close_after=True)
    r2 = RawResponse(resp2)
    expect(r2.status == 200, f"second request on the same keep-alive connection failed "
                              f"(got {r2.status}) -- is keep-alive actually working?")


@test("protocol")
def connection_close_header_is_honoured():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(CONNECT_TIMEOUT)
    sock.connect((HOST, PORT))
    resp, sock = raw_request(HOST, PORT, build_request(
        "GET", "/", {"Connection": "close"}, http_version="1.1"), sock=sock, close_after=False)
    r = RawResponse(resp)
    expect(r.status == 200, f"expected 200, got {r.status}")
    try:
        sock.settimeout(1.5)
        more = sock.recv(4096)
        expect(more == b"", "server kept the connection open despite Connection: close")
    except (ConnectionResetError, socket.timeout, OSError):
        pass
    finally:
        sock.close()


@test("protocol")
def missing_host_header_on_http11_is_400():
    resp, _ = raw_request(HOST, PORT, build_request("GET", "/", http_version="1.1", host_header=False))
    r = RawResponse(resp)
    expect(r.status == 400,
           f"HTTP/1.1 request without Host: RFC 7230 requires 400, got {r.status} "
           "(some implementations are lenient here -- check your grading rubric)")


@test("protocol")
def pipelined_requests_answered_in_order():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(CONNECT_TIMEOUT)
    sock.connect((HOST, PORT))
    two_requests = (build_request("GET", "/", http_version="1.1") +
                    build_request("GET", DOC_ROOT_INDEX, http_version="1.1"))
    sock.settimeout(READ_TIMEOUT)
    sock.sendall(two_requests)
    time.sleep(0.3)
    data = b""
    try:
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            data += chunk
    except socket.timeout:
        pass
    sock.close()
    count = data.count(b"HTTP/1.1 200")
    expect(count == 2, f"pipelined two GETs: expected 2 responses, got {count} "
                        f"(raw byte count: {len(data)})")


# ---------------------------------------------------------------------------
# G. Chunked Transfer-Encoding
# ---------------------------------------------------------------------------

@test("chunked")
def chunked_request_body_is_decoded():
    payload = b"chunked upload works if you can read this back" * 4
    req = build_request("POST", UPLOAD_PATH + "/chunked_test.txt", {"Transfer-Encoding": "chunked"}, b"")
    head = req.split(b"\r\n\r\n")[0] + b"\r\n\r\n"
    full = head + chunk_encode(payload)
    resp, _ = raw_request(HOST, PORT, full)
    r = RawResponse(resp)
    expect(r.status in (200, 201), f"chunked upload: expected 200/201, got {r.status}")


@test("chunked")
def malformed_chunk_size_gives_400():
    head = (f"POST {UPLOAD_PATH}/bad_chunk.txt HTTP/1.1\r\n"
            f"Host: {HOST_HEADER_VALUE}\r\nTransfer-Encoding: chunked\r\n\r\n").encode()
    body = b"ZZZZ\r\nnotarealchunk\r\n0\r\n\r\n"  # "ZZZZ" is not a valid hex size
    resp, _ = raw_request(HOST, PORT, head + body)
    r = RawResponse(resp)
    expect(r.status == 400, f"malformed chunk size: expected 400, got {r.status}")


# ---------------------------------------------------------------------------
# H. CGI -- place cgi_echo.py / cgi_sleep.py / cgi_status.py / cgi_crash.py
#          (shipped alongside this script) wherever /cgi-bin's alias points,
#          and `chmod +x` them.
# ---------------------------------------------------------------------------

@test("cgi")
def cgi_get_with_query_string():
    resp, _ = raw_request(HOST, PORT, build_request("GET", CGI_ECHO + "?name=webserv&x=1"))
    r = RawResponse(resp)
    expect(r.status == 200, f"CGI GET: expected 200, got {r.status}")
    expect(b"QUERY_STRING=name=webserv&x=1" in r.body,
           "QUERY_STRING was not passed to the CGI script correctly")
    expect(b"REQUEST_METHOD=GET" in r.body, "REQUEST_METHOD missing/incorrect in CGI env")


@test("cgi")
def cgi_post_with_body():
    body = b"field=value&other=42"
    resp, _ = raw_request(HOST, PORT, build_request(
        "POST", CGI_ECHO, {"Content-Type": "application/x-www-form-urlencoded"}, body))
    r = RawResponse(resp)
    expect(r.status == 200, f"CGI POST: expected 200, got {r.status}")
    expect(f"CONTENT_LENGTH={len(body)}".encode() in r.body, "CONTENT_LENGTH incorrect/missing")
    expect(body in r.body, "request body was not correctly relayed to the CGI script's stdin")


@test("cgi")
def cgi_status_header_is_respected():
    resp, _ = raw_request(HOST, PORT, build_request("GET", CGI_STATUS + "?code=404"))
    r = RawResponse(resp)
    expect(r.status == 404,
           f"CGI script emitted 'Status: 404', but server responded with {r.status} "
           "(server must not hardcode 200 for every CGI response)")


@test("cgi", "slow")
def cgi_timeout_returns_error_without_hanging():
    start = time.time()
    resp, _ = raw_request(HOST, PORT, build_request("GET", CGI_SLEEP), read_timeout=CGI_TIMEOUT_HINT + 5)
    elapsed = time.time() - start
    r = RawResponse(resp)
    expect(r.status in (500, 502, 503, 504), f"hanging CGI: expected a 5xx error, got {r.status}")
    expect(elapsed < CGI_TIMEOUT_HINT + 5,
           f"server took {elapsed:.1f}s to respond to a hanging CGI -- timeout not enforced?")


@test("cgi")
def cgi_crash_returns_500():
    resp, _ = raw_request(HOST, PORT, build_request("GET", CGI_CRASH))
    r = RawResponse(resp)
    expect(r.status == 500, f"crashing CGI script: expected 500, got {r.status}")


@test("cgi", "locations")
def non_cgi_extension_in_cgi_dir_is_static():
    resp, _ = raw_request(HOST, PORT, build_request("GET", CGI_DIR + "/plain.txt"))
    r = RawResponse(resp)
    expect(r.status in (200, 404),
           f"non-CGI file in /cgi-bin: expected 200 or 404 (not executed), got {r.status}")


@test("cgi", "slow")
def slow_cgi_does_not_block_other_clients():
    """The single most important non-blocking test: while a CGI request that
    sleeps is in flight, a concurrent request for a static file must still
    be answered quickly. If the event loop blocks on the CGI pipe, this
    test times out."""
    result = {}

    def slow_client():
        t0 = time.time()
        resp, _ = raw_request(HOST, PORT, build_request("GET", CGI_SLEEP), read_timeout=CGI_TIMEOUT_HINT + 5)
        result["slow_elapsed"] = time.time() - t0
        result["slow_status"] = RawResponse(resp).status

    thread = threading.Thread(target=slow_client)
    thread.start()
    time.sleep(0.5)  # give the slow request a head start

    t0 = time.time()
    resp, _ = raw_request(HOST, PORT, build_request("GET", "/"), read_timeout=2.0)
    fast_elapsed = time.time() - t0
    r = RawResponse(resp)
    thread.join(timeout=CGI_TIMEOUT_HINT + 10)

    expect(r.status == 200, f"concurrent static GET during a slow CGI: expected 200, got {r.status}")
    expect(fast_elapsed < 2.0,
           f"static GET took {fast_elapsed:.2f}s while a CGI was sleeping -- "
           "the event loop appears to block on the CGI pipe")


# ---------------------------------------------------------------------------
# I. Non-blocking / robustness / stress
# ---------------------------------------------------------------------------

@test("robustness", "slow")
def idle_client_is_eventually_dropped():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(CONNECT_TIMEOUT)
    sock.connect((HOST, PORT))
    sock.settimeout(IDLE_TIMEOUT_HINT + 5)
    start = time.time()
    try:
        data = sock.recv(4096)
        expect(data == b"", "server sent data to a client that never sent a request")
    except socket.timeout:
        raise Fail(f"server did not close an idle connection within "
                    f"{IDLE_TIMEOUT_HINT + 5}s -- check your timeout logic")
    finally:
        elapsed = time.time() - start
        sock.close()
    expect(elapsed < IDLE_TIMEOUT_HINT + 5, "idle timeout took implausibly long")


@test("robustness")
def client_disconnect_before_response_does_not_crash_server():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(CONNECT_TIMEOUT)
    sock.connect((HOST, PORT))
    sock.sendall(b"GET / HTTP/1.1\r\nHost: " + HOST_HEADER_VALUE.encode() + b"\r\n\r\n")
    sock.close()  # abrupt close, before we ever read a response
    time.sleep(0.3)
    resp, _ = raw_request(HOST, PORT, build_request("GET", "/"))
    r = RawResponse(resp)
    expect(r.status == 200, "server appears to have died after a client disconnected early "
                             f"(follow-up request got {r.status})")


@test("robustness", "slow")
def slow_trickle_request_is_still_served():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(CONNECT_TIMEOUT)
    sock.connect((HOST, PORT))
    sock.settimeout(READ_TIMEOUT + 5)
    req = build_request("GET", "/")
    for byte in req:
        sock.sendall(bytes([byte]))
        time.sleep(0.01)
    data = b""
    try:
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            data += chunk
    except socket.timeout:
        pass
    sock.close()
    r = RawResponse(data)
    expect(r.status == 200, f"byte-by-byte request: expected 200, got {r.status}")


@test("robustness", "slow", "stress")
def many_concurrent_static_requests_all_succeed():
    n = 60
    results = [None] * n

    def worker(i):
        try:
            resp, _ = raw_request(HOST, PORT, build_request("GET", "/"), read_timeout=10)
            results[i] = RawResponse(resp).status
        except Exception as e:  # noqa: BLE001 -- want to record *any* client-side failure
            results[i] = str(e)

    threads = [threading.Thread(target=worker, args=(i,)) for i in range(n)]
    for t in threads:
        t.start()
    for t in threads:
        t.join(timeout=15)

    ok = sum(1 for x in results if x == 200)
    expect(ok == n, f"{ok}/{n} concurrent requests succeeded -- server may be dropping "
                     "connections under load (subject requires passing a stress test)")


# ---------------------------------------------------------------------------
# Runner
# ---------------------------------------------------------------------------

def main():
    global HOST, PORT
    ap = argparse.ArgumentParser(description=__doc__,
                                  formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--host", default=HOST)
    ap.add_argument("--port", type=int, default=PORT)
    ap.add_argument("--only", default=None, help="only run tests whose name or tag contains this substring")
    ap.add_argument("--skip-slow", action="store_true", help="skip tests tagged 'slow' (timeouts/stress)")
    ap.add_argument("--list", action="store_true", help="list test names and exit")
    args = ap.parse_args()

    HOST, PORT = args.host, args.port

    if args.list:
        for name, _, tags in TESTS:
            print(f"{name:45s} [{', '.join(sorted(tags))}]")
        return

    to_run = []
    for name, fn, tags in TESTS:
        if args.skip_slow and "slow" in tags:
            continue
        if args.only and args.only not in name and not any(args.only in t for t in tags):
            continue
        to_run.append((name, fn, tags))

    passed, failed, errored = 0, 0, 0
    print(f"Running {len(to_run)} test(s) against {HOST}:{PORT}\n")
    for name, fn, tags in to_run:
        try:
            fn()
        except Fail as e:
            print(f"[FAIL]  {name}: {e}")
            failed += 1
        except Exception as e:  # noqa: BLE001 -- surface any unexpected client-side error too
            print(f"[ERROR] {name}: {type(e).__name__}: {e}")
            errored += 1
        else:
            print(f"[ OK ]  {name}")
            passed += 1

    print(f"\n{passed} passed, {failed} failed, {errored} errored "
          f"(of {len(to_run)} run, {len(TESTS)} total)")
    sys.exit(1 if (failed or errored) else 0)


if __name__ == "__main__":
    main()
