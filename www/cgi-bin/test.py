#!/usr/bin/env python3
import os, sys

print("Status: 200 OK")
print("Content-Type: text/html\r\n")
print("<h1>CGI Test Erfolgreich!</h1>")
print(f"<p>Query String: {os.environ.get('QUERY_STRING', '')}</p>")

# Falls POST-Body vorhanden ist, ausgeben
if os.environ.get('REQUEST_METHOD') == 'POST':
    length = int(os.environ.get('CONTENT_LENGTH', 0))
    body = sys.stdin.read(length) if length > 0 else ''
    print(f"<p>POST Body: {body}</p>")
