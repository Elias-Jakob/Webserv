#!/usr/bin/python3
import time, os

print("<p>")
print(os.environ)
print("</p>")
for i in range(5):
    print(
        f"<h1>Blog post no.: {i}</h1><br>This is just some random unimportant text..."
    )
    time.sleep(3)
