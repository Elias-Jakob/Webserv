#!/usr/bin/env python3
"""
cgi_sleep.py -- place this in the directory your /cgi-bin location's alias
points to, and `chmod +x` it. Sleeps far longer than any reasonable CGI
timeout, so the tester can verify your server kills the process and
answers with an error instead of hanging forever.
"""
import sys
import time

time.sleep(60)  # long enough to guarantee the server's timeout fires first
sys.stdout.write("Content-Type: text/plain\r\n\r\nThis should never be seen.\n")
