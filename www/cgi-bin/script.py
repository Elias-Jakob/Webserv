#!/usr/bin/python3
import os

print("Status: 404 Not Found", end="\r\n");
print("CONTENT-TYPE: text/html", end="\r\n");
print("ANOTHER-HEADER: hello!!!", end="\r\n");
print("\r")

if os.getenv("REQUEST_METHOD") == "POST":
    read = input()
    print("<h1>POST BODY = </h1>")
    print(read)
else:
    print("<h1>NOT A POST REQUEST</h1>")
