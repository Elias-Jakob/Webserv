#!/usr/bin/env python3
import socket
import concurrent.futures
import random
import time
import sys, os

# --- KONFIGURATION ---
HOST = "127.0.0.1"
PORT = 8080
CGI_SCRIPT = "/cgi-bin/test.py"
TOTAL_REQUESTS = 400
CONCURRENT_THREADS = 50
# ---------------------

def send_cgi_request(req_id):
    """Baut eine isolierte TCP-Verbindung auf und sendet EINEN Request."""
    req_type = random.choice(["GET_SIMPLE", "GET_QUERY", "POST"])
    
    if req_type == "GET_SIMPLE":
        req = f"GET {CGI_SCRIPT} HTTP/1.1\r\nHost: {HOST}\r\nConnection: close\r\n\r\n"
    elif req_type == "GET_QUERY":
        req = f"GET {CGI_SCRIPT}?user=stress{req_id}&id={req_id} HTTP/1.1\r\nHost: {HOST}\r\nConnection: close\r\n\r\n"
    else: # POST
        body = f"stress_data=payload_{req_id}_" * 5
        req = (f"POST {CGI_SCRIPT} HTTP/1.1\r\nHost: {HOST}\r\n"
               f"Content-Length: {len(body)}\r\n"
               f"Content-Type: application/x-www-form-urlencoded\r\n"
               f"Connection: close\r\n\r\n{body}")
        
    client_port = "Unknown" # Standardwert, falls connect() fehlschlägt
    
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(4.0)
        s.connect((HOST, PORT))
        
        # WICHTIG: Port merken, SOLANGE das Socket noch offen ist!
        # getsockname() gibt ein Tuple zurück: ('127.0.0.1', 54321) -> [1] ist der Port
        client_port = s.getsockname()[1] 
        
        s.sendall(req.encode('utf-8'))
        
        response = b""
        while True:
            chunk = s.recv(4096)
            if not chunk:
                break
            response += chunk
        s.close()
        
        # Prüfe ob der Request erfolgreich war (200 OK)
        if b"200 OK" in response:
            print(f"[Client-Port {client_port}] OK")
            return True, None
        else:
            first_line = response.split(b"\r\n")[0].decode('utf-8', errors='ignore') if response else "Empty Response"
            # os._exit(0);
            return False, f"[Client-Port {client_port}] {first_line}"
            
    except Exception as e:
        # Falls es z.B. einen Timeout gibt, packen wir den Port auch mit in die Fehlermeldung
        return False, f"[Client-Port {client_port}] {str(e)}"
def main():
    print(f"\nStarte CGI Stresstest: {TOTAL_REQUESTS} Requests, max {CONCURRENT_THREADS} Threads parallel...")
    start_time = time.time()
    
    success = 0
    fail = 0
    errors = {}

    # ThreadPool für den Stresstest
    with concurrent.futures.ThreadPoolExecutor(max_workers=CONCURRENT_THREADS) as executor:
        futures = [executor.submit(send_cgi_request, i) for i in range(TOTAL_REQUESTS)]
        
        for future in concurrent.futures.as_completed(futures):
            passed, err_msg = future.result()
            if passed:
                success += 1
            else:
                fail += 1
                if err_msg not in errors:
                    errors[err_msg] = 0
                errors[err_msg] += 1
                
            # Fortschrittsanzeige
            done = success + fail
            if done % 50 == 0:
                print(f"[{done}/{TOTAL_REQUESTS}] abgearbeitet...")

    elapsed = time.time() - start_time
    
    # --- AUSWERTUNG ---
    print("\n" + "="*40)
    print(f"Zeit benötigt:   {elapsed:.2f} Sekunden")
    print(f"Durchsatz:       {TOTAL_REQUESTS/elapsed:.2f} Req/Sek")
    print(f"Erfolgreich:     {success}")
    print(f"Fehlgeschlagen:  {fail}")
    
    if fail > 0:
        print("\nFehler-Zusammenfassung:")
        for msg, count in errors.items():
            print(f"   - {count}x: {msg}")
    print("="*40 + "\n")

if __name__ == "__main__":
    main()
