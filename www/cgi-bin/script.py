#!/usr/bin/python3
import os

if os.getenv("REQUEST_METHOD") == "POST":
    read = input()
    print("<h1>POST BODY = </h1>")
    print(read)
else:
    print("<h1>NOT A POST REQUEST</h1>")
