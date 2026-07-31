#!/usr/bin/env python3
"""
cgi_crash.py -- place this in the directory your /cgi-bin location's alias
points to, and `chmod +x` it. Exits immediately without producing any
output, simulating a crashing CGI script. The tester verifies your
server detects this (EOF / process exit) and answers with a proper 500
instead of hanging or crashing itself.
"""
import sys

sys.exit(1)
