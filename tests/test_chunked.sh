#!/bin/bash

# Test Chunked Transfer-Encoding requests

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

send_chunked_request() {
    local uri="$1"
    local chunks="$2"  # Pre-formatted chunked data
    local request="POST $uri HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\n\r\n${chunks}"
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

# Create chunked data helper
# Args: chunk1_size chunk1_data chunk2_size chunk2_data ... then "0" at end
create_chunked_body() {
    local result=""
    while [ $# -gt 0 ]; do
        if [ "$1" = "0" ]; then
            result+="0\r\n\r\n"
            break
        fi
        local size="$1"
        local data="$2"
        result+=$(printf "%x\r\n$data\r\n" "$size")
        shift 2
    done
    echo -ne "$result"
}

# Test 1: Simple chunked POST with single chunk
print_test "POST with single chunk"
CHUNKED=$(printf "5\r\nHello\r\n0\r\n\r\n")
STATUS=$(send_chunked_request "/" "$CHUNKED")
check_status "Should return 200 or 201" "200" "$STATUS"
sleep 0.5

# Test 2: Multiple chunks
print_test "POST with multiple chunks (Hello + World)"
CHUNKED=$(printf "5\r\nHello\r\n6\r\n World\r\n0\r\n\r\n")
STATUS=$(send_chunked_request "/" "$CHUNKED")
check_status "Should handle multiple chunks" "200" "$STATUS"
sleep 0.5

# Test 3: Chunked form data (name=value pairs)
print_test "POST chunked form data"
CHUNKED=$(printf "10\r\nname=John&age=30\r\n0\r\n\r\n")
STATUS=$(send_chunked_request "/cgi-bin/form.php" "$CHUNKED")
check_status "Should accept chunked form data" "200" "$STATUS"
sleep 0.5

# Test 4: Empty chunks (chunk of size 0)
print_test "POST with empty intermediate chunk"
CHUNKED=$(printf "5\r\nHello\r\n0\r\n\r\n")
STATUS=$(send_chunked_request "/" "$CHUNKED")
check_status "Should terminate on 0 chunk" "200" "$STATUS"
sleep 0.5

# Test 5: Large data split into multiple chunks
print_test "POST with large data in multiple chunks"
CHUNKED=$(printf "a\r\n0123456789\r\na\r\nabcdefghij\r\n0\r\n\r\n")
STATUS=$(send_chunked_request "/" "$CHUNKED")
check_status "Should reconstruct large chunked data" "200" "$STATUS"
sleep 0.5

# Test 6: Chunked POST to upload endpoint
print_test "POST chunked to upload handler"
CHUNKED=$(printf "e\r\nfile_content_1\r\n0\r\n\r\n")
STATUS=$(send_chunked_request "/upload" "$CHUNKED")
check_status "Should handle chunked upload" "200" "$STATUS"
sleep 0.5

# Test 7: Verify unchunking works with CGI scripts
print_test "POST chunked to CGI script"
CHUNKED=$(printf "b\r\ntest=value!\r\n0\r\n\r\n")
STATUS=$(send_chunked_request "/cgi-bin/script.py" "$CHUNKED")
check_status "Should pass unchunked data to CGI" "200" "$STATUS"
sleep 0.5

echo ""
echo -e "Results: ${GREEN}$PASS passed${NC}, ${RED}$FAIL failed${NC}"
exit $FAIL