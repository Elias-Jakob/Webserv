#!/bin/bash

# Test POST Content-Type validation

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

send_request() {
    local request="$1"
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

# Test 1: POST without Content-Type header
print_test "POST /upload without Content-Type header"
REQUEST="POST /upload HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5\r\n\r\ntest="
STATUS=$(send_request "$REQUEST")
check_status "Should return 400 (no Content-Type)" "400" "$STATUS"
sleep 0.5

# Test 2: POST with invalid Content-Type
print_test "POST /upload with Content-Type: text/plain"
REQUEST="POST /upload HTTP/1.1\r\nHost: localhost\r\nContent-Type: text/plain\r\nContent-Length: 5\r\n\r\ntest="
STATUS=$(send_request "$REQUEST")
check_status "Should return 400 (invalid Content-Type)" "400" "$STATUS"
sleep 0.5

# Test 3: POST with Content-Type: application/xml
print_test "POST /upload with Content-Type: application/xml"
REQUEST="POST /upload HTTP/1.1\r\nHost: localhost\r\nContent-Type: application/xml\r\nContent-Length: 5\r\n\r\ntest="
STATUS=$(send_request "$REQUEST")
check_status "Should return 400 (not form-urlencoded)" "400" "$STATUS"
sleep 0.5

# Test 4: POST with Content-Type: multipart/mixed (wrong subtype)
print_test "POST /upload with Content-Type: multipart/mixed"
REQUEST="POST /upload HTTP/1.1\r\nHost: localhost\r\nContent-Type: multipart/mixed; boundary=test\r\nContent-Length: 5\r\n\r\ntest="
STATUS=$(send_request "$REQUEST")
check_status "Should return 400 (wrong multipart subtype)" "400" "$STATUS"
sleep 0.5

# Test 5: POST with valid Content-Type: application/x-www-form-urlencoded
print_test "POST /submit with Content-Type: application/x-www-form-urlencoded"
REQUEST="POST /submit HTTP/1.1\r\nHost: localhost\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 10\r\n\r\nname=Kevin"
STATUS=$(send_request "$REQUEST")
check_status "Should return 201 (valid form submission)" "201" "$STATUS"
sleep 0.5

# Test 6: POST with valid Content-Type: multipart/form-data
print_test "POST /upload with Content-Type: multipart/form-data"
BOUNDARY="----WebKitFormBoundary"
BODY="--$BOUNDARY\r\nContent-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\nContent-Type: text/plain\r\n\r\ntest content\r\n--$BOUNDARY--"
REQUEST="POST /upload HTTP/1.1\r\nHost: localhost\r\nContent-Type: multipart/form-data; boundary=$BOUNDARY\r\nContent-Length: ${#BODY}\r\n\r\n$BODY"
STATUS=$(send_request "$REQUEST")
# May return 201 or 413 depending on body size, but not 400
if [ "$STATUS" != "400" ]; then
    print_pass "Should not return 400 (valid multipart)"
else
    print_fail "Should not return 400" "201/413" "$STATUS"
fi
sleep 0.5

echo ""
echo -e "Results: ${GREEN}$PASS passed${NC}, ${RED}$FAIL failed${NC}"
exit $FAIL