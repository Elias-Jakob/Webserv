#!/bin/bash

# Test POST file extension validation

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

send_multipart_request() {
    local filename="$1"
    local content="$2"
    local boundary="----WebKitFormBoundary"
    local disposition="Content-Disposition: form-data; name=\"file\"; filename=\"$filename\""
    local body="--${boundary}\r\n${disposition}\r\nContent-Type: application/octet-stream\r\n\r\n${content}\r\n--${boundary}--"
    
    local body_bytes=$(echo -ne "$body" | wc -c)

    local request="POST /upload HTTP/1.1\r\nHost: localhost\r\nContent-Type: multipart/form-data; boundary=${boundary}\r\nContent-Length: ${body_bytes}\r\n\r\n${body}"
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

# Test 1: Upload allowed extension (.txt)
print_test "Upload test.txt (allowed extension)"
STATUS=$(send_multipart_request "test.txt" "test content")
check_status "Should return 201 (.txt allowed)" "201" "$STATUS"
sleep 0.5

# Test 2: Upload allowed extension (.pdf)
print_test "Upload test.pdf (allowed extension)"
STATUS=$(send_multipart_request "test.pdf" "pdf content")
check_status "Should return 201 (.pdf allowed)" "201" "$STATUS"
sleep 0.5

# Test 3: Upload disallowed extension (.exe)
print_test "Upload test.exe (disallowed extension)"
STATUS=$(send_multipart_request "test.exe" "executable")
check_status "Should return 415 (.exe not allowed)" "415" "$STATUS"
sleep 0.5

# Test 4: Upload disallowed extension (.sh)
print_test "Upload test.sh (disallowed extension)"
STATUS=$(send_multipart_request "test.sh" "#!/bin/bash")
check_status "Should return 415 (.sh not allowed)" "415" "$STATUS"
sleep 0.5

# Test 5: Upload file with no extension
print_test "Upload file_noext (no extension)"
STATUS=$(send_multipart_request "file_noext" "no extension content")
check_status "Should return 400 (no extension)" "400" "$STATUS"
sleep 0.5

# Test 6: Upload file with multiple dots
print_test "Upload archive.tar.gz (multiple dots)"
STATUS=$(send_multipart_request "archive.tar.gz" "archive content")
check_status "Should return 415 (.gz not in allowed list)" "415" "$STATUS"
sleep 0.5

echo ""
echo -e "Results: ${GREEN}$PASS passed${NC}, ${RED}$FAIL failed${NC}"
exit $FAIL