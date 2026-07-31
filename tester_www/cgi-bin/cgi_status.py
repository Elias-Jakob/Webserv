#!/usr/bin/env python3
"""
cgi_status.py -- place this in the directory your /cgi-bin location's alias
points to, and `chmod +x` it. Emits a CGI "Status:" header (RFC 3875
6.3.3) so the tester can verify your server actually reads and forwards
it, instead of hardcoding "200 OK" for every CGI response.

Usage: /cgi-bin/cgi_status.py?code=404
"""
import os
import sys
from urllib.parse import parse_qs

qs = parse_qs(os.environ.get("QUERY_STRING", ""))
code = qs.get("code", ["404"])[0]
reasons = {"200": "OK", "301": "Moved Permanently", "404": "Not Found",
           "500": "Internal Server Error"}
reason = reasons.get(code, "Status")

out = f"CGI reported status {code}\n".encode()

sys.stdout.buffer.write(f"Status: {code} {reason}\r\n".encode())
sys.stdout.buffer.write(b"Content-Type: text/plain\r\n")
sys.stdout.buffer.write(f"Content-Length: {len(out)}\r\n\r\n".encode())
sys.stdout.buffer.write(out)
sys.stdout.buffer.flush()
