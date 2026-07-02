#!/bin/bash

# Test GET requests

HOST="${1:-127.0.0.1}"
PORT="${2:-8080}"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

PASS=0
FAIL=0

print_test() {
    echo -e "${YELLOW}[TEST]${NC} $1"
}

print_pass() {
    echo -e "${GREEN}[PASS]${NC} $1"
    ((PASS++))
}

print_fail() {
    echo -e "${RED}[FAIL]${NC} $1"
    echo -e "${RED}       Expected: $2, Got: $3${NC}"
    ((FAIL++))
}

send_get_request() {
    local uri="$1"
    local request="GET $uri HTTP/1.1\r\nHost: localhost\r\n\r\n"
    local response=$(echo -ne "$request" | nc -w 2 $HOST $PORT 2>/dev/null)
    local status=$(echo "$response" | head -n 1 | cut -d' ' -f2)
    echo "$status"
}

check_status() {
    local test_name="$1"
    local expected="$2"
    local actual="$3"
    
    if [ "$actual" = "$expected" ]; then
        print_pass "$test_name"
    else
        print_fail "$test_name" "$expected" "$actual"
    fi
}

# Test 1: GET valid file
print_test "GET /www/index.html (valid file)"
STATUS=$(send_get_request "/www/index.html")
check_status "Should return 200" "200" "$STATUS"
sleep 0.5

# Test 2: GET non-existent file
print_test "GET /www/nonexistent.html (non-existent)"
STATUS=$(send_get_request "/www/nonexistent.html")
check_status "Should return 404" "404" "$STATUS"
sleep 0.5

# Test 3: GET directory with index.html
print_test "GET / (directory with autoindex)"
STATUS=$(send_get_request "/")
check_status "Should return 200" "200" "$STATUS"
sleep 0.5

# Test 4: GET directory without trailing slash
print_test "GET /www (directory)"
STATUS=$(send_get_request "/www")
check_status "Should handle directory request" "200" "$STATUS"
sleep 0.5

# Test 5: GET from redirect location
print_test "GET /redir (redirect location)"
STATUS=$(send_get_request "/redir")
check_status "Should return 301 (redirect)" "301" "$STATUS"
sleep 0.5

# Test 6: GET with path traversal attempt
print_test "GET /../etc/passwd (path traversal attempt)"
STATUS=$(send_get_request "/../etc/passwd")
check_status "Should block path traversal" "403" "$STATUS"
sleep 0.5

# Test 7: GET with disallowed method
print_test "GET /cba (allowed GET location)"
STATUS=$(send_get_request "/cba")
check_status "Should return 200" "200" "$STATUS"
sleep 0.5

# Test 8: GET text file with correct Content-Type
print_test "GET /www/text.html (text file)"
STATUS=$(send_get_request "/www/text.html")
check_status "Should return 200" "200" "$STATUS"
sleep 0.5

echo ""
echo -e "Results: ${GREEN}$PASS passed${NC}, ${RED}$FAIL failed${NC}"
exit $FAIL