import socket
import time

s = socket.create_connection(("127.0.0.1", 8080))

s.sendall(b"POST /cgi-bin/script.py HTTP/1.1\r\n")
time.sleep(1)

s.sendall(b"Host: localhost\r\n")
time.sleep(1)

s.sendall(b"Transfer-Encoding: chunked\r\n\r\n")
time.sleep(1)

s.sendall(b"5\r\nHello\r\n")
time.sleep(2)

s.sendall(b"6\r\n World\r\n")
time.sleep(2)

s.sendall(b"0\r\n\r\n")

print(s.recv(4096))
