#!/usr/bin/env python3
"""
cgi_echo.py -- place this in the directory your /cgi-bin location's alias
points to, and `chmod +x` it. Echoes the CGI meta-variables (RFC 3875)
plus the request body, so the tester can verify your server sets them
correctly (QUERY_STRING, CONTENT_LENGTH, REQUEST_METHOD, PATH_INFO, ...).
"""
import os
import sys

try:
    length = int(os.environ.get("CONTENT_LENGTH") or "0")
except ValueError:
    length = 0
body = sys.stdin.buffer.read(length) if length > 0 else b""

relevant_prefixes = ("REQUEST_", "QUERY_", "CONTENT_", "SCRIPT_", "PATH_",
                     "SERVER_", "GATEWAY_", "REMOTE_", "AUTH_", "HTTP_")
lines = [f"{key}={os.environ[key]}" for key in sorted(os.environ)
         if key.startswith(relevant_prefixes)]

out = ("\n".join(lines) + "\n\n--- BODY ---\n").encode() + body

sys.stdout.buffer.write(b"Content-Type: text/plain\r\n")
sys.stdout.buffer.write(f"Content-Length: {len(out)}\r\n\r\n".encode())
sys.stdout.buffer.write(out)
sys.stdout.buffer.flush()
