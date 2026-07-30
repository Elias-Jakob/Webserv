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

# Test 2: POST without Content-Type header
# print_test "POST /contacts without Content-Type header"
# REQUEST="POST /contacts HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5\r\n\r\nhello"
# STATUS=$(send_request "$REQUEST")
# check_status "Should return 200 | 201 (no Content-Type)" "201" "$STATUS"
# sleep 0.5

# Test: Post binary_file
# print_test "POST/ upload binary file (application/octet-stream)"
# REQUEST="POST /upload HTTP/1.1\r\nHost: localhost\r\nContent-Type: application/octet-stream\r\nContent-Length: 8\r\n\r\n\x89PNG\r\nx1a\n"
# STATUS=$(send_request "$REQUEST")
# check_status "Should return 200" "200" "$STATUS"
# sleep 0.5

# Test: Post multipart/form-data
# print_test "POST /upload multipart/form-data)"
# REQUEST="POST /upload HTTP/1.1\r\nHost: localhost\r\nContent-Type: multipart/form-data; boundary=boundary123\r\nContent-Length: 218\r\n\r\n--boundary123\r\nContent-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\nContent-Type: text/plain\r\n\r\nhello world\r\n--boundary123--\r\n"
# STATUS=$(send_request "$REQUEST")
# check_status "Should return 200 | 201" "201" "$STATUS"
# sleep 10


# # Test: Post multipart/form-data
# print_test "GET /page#section"
# REQUEST="GET /page#section HTTP/1.1\r\nHost: localhost\r\n\r\n"
# STATUS=$(send_request "$REQUEST")
# check_status "Should return 400" "400" "$STATUS"
# sleep 0.5

# # Test: invalid header-name -header-field seperator
# print_test "POST localhost\r\n"
# REQUEST="POST /contacts HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: gzip\r\n\r\nhello"
# STATUS=$(send_request "$REQUEST")
# check_status "Should return 400" "400" "$STATUS"
# sleep 0.5


# print_test "POST localhost\r\n"
# REQUEST="POST /contacts HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: gzip\r\n\r\nhello"
# STATUS=$(send_request "$REQUEST")
# check_status "Should return 400" "400" "$STATUS"
# sleep 0.5

#Test directory listing
REQUEST="GET /files HTTP/1.1\r\nHost: localhost\r\n\r\n"
STATUS=$(send_request "$REQUEST")
check_status "Should return 200" "200" $STATUS
sleep 0.5

# Test call to nonexisten dir
print_test "GET /nonexistent directory"
REQUEST="GET /files/nonexistent_subdir/ HTTP/1.1\r\nHost: localhost\r\n\r\n"
STATUS=$(send_request "$REQUEST")
check_status "Should return 404" "404" $STATUS

# Test redirect 302
print_test "GET /temp redirect 302"
REQUEST="GET /temp HTTP/1.1\r\nHost: localhost\r\n\r\n"
STATUS=$(send_request "$REQUEST")
check_status "Should return 302" "302" $STATUS

# Test redirect 301
print_test "GET /temp redirect 301"
REQUEST="GET /old HTTP/1.1\r\nHost: localhost\r\n\r\n"
STATUS=$(send_request "$REQUEST")
check_status "Should return 301" "301" $STATUS


# Test respone has content-type
print_test "GET /nonexistent (check for content-type)"
REQUEST="GET /nonexistent HTTP/1.1\r\nHost: localhost\r\n\r\n"
STATUS=$(send_request "$REQUEST")
check_status "Should return 404" "404" $STATUS

echo ""
echo -e "Results: ${GREEN}$PASS passed${NC}, ${RED}$FAIL failed${NC}"
exit $FAIL