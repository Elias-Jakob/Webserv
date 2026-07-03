#!/usr/bin/python3
import time, os

time.sleep(3)

print(os.environ)

for i in range(10):
    print(
        f"<h1>Blog post no.: {i}</h1><br>This is just some random unimportant text..."
    )
