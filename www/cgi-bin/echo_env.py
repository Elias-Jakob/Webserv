#\!/usr/bin/env python3
import os
print("Content-Type: text/plain\r\n\r")
for key in ("SCRIPT_NAME", "PATH_INFO", "PATH_TRANSLATED", "QUERY_STRING", "REQUEST_METHOD"):
    print(key + "=" + os.environ.get(key, "<unset>"))
