#!/usr/bin/env python3
"""
login.py -- place this in the directory your /cgi-bin location's alias
points to, and `chmod +x` it. Demonstrates the webserver's generic
session-data mechanism:
  - IN:  SESSION_ID / SESSION_<KEY> environment variables, set by the
         server from whatever was previously stored for this session.
  - OUT: "X-Session-Set: key=value" response headers, read and stored by
         the server (ClientConnection::applyCgiSessionHeaders()), then
         stripped before the response reaches the browser.

The server has no notion of "login" -- this script alone decides what
"isLoggedIn"/"name" mean and when to set them. Any other CGI script can
read the same SESSION_ISLOGGEDIN / SESSION_NAME variables to gate access.

Usage:
  GET  /cgi-bin/login.py             -> reports current login state
  POST /cgi-bin/login.py             body: username=alice&password=wonderland
  GET|POST /cgi-bin/login.py?action=logout
"""
import os
import sys
from urllib.parse import parse_qs

USERS = {
    "alice": "wonderland",
    "bob": "builder",
}


def read_body():
    try:
        length = int(os.environ.get("CONTENT_LENGTH") or "0")
    except ValueError:
        length = 0
    return sys.stdin.buffer.read(length) if length > 0 else b""


def send(body_text, extra_headers=None, status=None):
    out = body_text.encode()
    if status:
        sys.stdout.buffer.write(f"Status: {status}\r\n".encode())
    sys.stdout.buffer.write(b"Content-Type: text/plain\r\n")
    for header in (extra_headers or []):
        sys.stdout.buffer.write((header + "\r\n").encode())
    sys.stdout.buffer.write(f"Content-Length: {len(out)}\r\n\r\n".encode())
    sys.stdout.buffer.write(out)
    sys.stdout.buffer.flush()


query = parse_qs(os.environ.get("QUERY_STRING", ""))
# already_logged_in = os.environ.get("SESSION_ISLOGGEDIN", "false") == "true"
current_name = os.environ.get("SESSION_NAME", "")
logging_out = os.environ.get("SESSION_ISLOGGEDIN", "true") == "false"

username = current_name

if username in USERS:
    # Redirect to form with success message
    sys.stdout.buffer.write(b"Status: 303 See Other\r\n")
    sys.stdout.buffer.write(b"Location: /form.html?status=logout_success\r\n")
    sys.stdout.buffer.write(b"X-Session-Set: isLoggedIn=false\r\n")
    sys.stdout.buffer.write(f"X-Session-Set: name={username}\r\n".encode())
    sys.stdout.buffer.write(b"Content-Length: 0\r\n\r\n")
    sys.stdout.buffer.flush()
else:
    send("login failed\n", status="401 Unauthorized")