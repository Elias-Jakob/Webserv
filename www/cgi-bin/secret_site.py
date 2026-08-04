#!/usr/bin/env python3
"""
secret_site.py -- only accessible if logged in
"""
import os
import sys

def send(body_text, extra_headers=None, status=None):
    out = body_text.encode()
    if status:
        sys.stdout.buffer.write(f"Status: {status}\r\n".encode())
    sys.stdout.buffer.write(b"Content-Type: text/html\r\n")
    for header in (extra_headers or []):
        sys.stdout.buffer.write((header + "\r\n").encode())
    sys.stdout.buffer.write(f"Content-Length: {len(out)}\r\n\r\n".encode())
    sys.stdout.buffer.write(out)
    sys.stdout.buffer.flush()

# Check if logged in
is_logged_in = os.environ.get("SESSION_ISLOGGEDIN", "false") == "true"
username = os.environ.get("SESSION_NAME", "")

if not is_logged_in:
    # Not logged in - redirect to login
    sys.stdout.buffer.write(b"Status: 302 Found\r\n")
    sys.stdout.buffer.write(b"Location: /form.html\r\n")
    sys.stdout.buffer.write(b"Content-Length: 0\r\n\r\n")
    sys.stdout.buffer.flush()
    sys.exit(0)

# User IS logged in - serve the secret page
secret_content = f"""
<html>
<head><title>Secret Page</title></head>
<body>
<h1>Welcome, {username}!</h1>
<p>This is a secret page only for logged-in users.</p>
<a href="/cgi-bin/logout.py?action=logout">Logout</a>
</body>
</html>
"""

send(secret_content)