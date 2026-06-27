#!/bin/bash

# HTTP Test Suite for Webserv
# Tests all error codes and validation

HOST="127.0.0.1"
PORT="8080"
PASS=0
FAIL=0

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

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

# Function to send HTTP request and get status code
send_request() {
    local request="$1"
    local response=$(echo -ne "$request" | nc -w 2 $HOST $PORT 2>/dev/null)
    local status=$(echo "$response" | head -n 1 | cut -d' ' -f2)
    echo "$status"
}

# Function to check status code
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

echo "======================================"
echo "  HTTP Webserv Test Suite"
echo "======================================"
echo ""

# Check if server is running
print_test "Checking if server is running on $HOST:$PORT..."
if ! nc -z $HOST $PORT 2>/dev/null; then
    echo -e "${RED}[ERROR]${NC} Server is not running on $HOST:$PORT"
    echo "Please start the server first: ./webserv"
    exit 1
fi
echo -e "${GREEN}[OK]${NC} Server is running"
echo ""
sleep 1
# ============================================
# TEST 1: Valid GET Request (200 OK)
# ============================================
print_test "Test 1: Valid GET request"
REQUEST="GET /www/index.html HTTP/1.1\r\nHost: localhost\r\n\r\n"
STATUS=$(send_request "$REQUEST")
check_status "Valid GET request should return 200" "200" "$STATUS"
sleep 1

# ============================================
# TEST 2: Missing Host Header (400 Bad Request)
# ============================================
print_test "Test 2: Missing Host header"
REQUEST="GET /www/index.html HTTP/1.1\r\n\r\n"
STATUS=$(send_request "$REQUEST")
check_status "Request without Host header should return 400" "400" "$STATUS"
sleep 1

# ============================================
# TEST 3: Malformed Request Line (400 Bad Request)
# ============================================
print_test "Test 3: Malformed request line (no spaces)"
REQUEST="GETINDEX.HTML\r\nHost: localhost\r\n\r\n"
STATUS=$(send_request "$REQUEST")
check_status "Malformed request line should return 400" "400" "$STATUS"
sleep 1

# ============================================
# TEST 4: Malformed Request Line (missing version)
# ============================================
print_test "Test 4: Malformed request line (missing HTTP version)"
REQUEST="GET /www/index.html\r\nHost: localhost\r\n\r\n"
STATUS=$(send_request "$REQUEST")
check_status "Request line without HTTP version should return 400" "400" "$STATUS"
sleep 1

# ============================================
# TEST 5: Malformed Header (no colon)
# ============================================
print_test "Test 5: Malformed header (no colon)"
REQUEST="GET /www/index.html HTTP/1.1\r\nHost localhost\r\n\r\n"
STATUS=$(send_request "$REQUEST")
check_status "Header without colon should return 400" "400" "$STATUS"
sleep 1

# ============================================
# TEST 6: POST without Content-Length (411 Length Required)
# ============================================
print_test "Test 6: POST without Content-Length"
REQUEST="POST /submit HTTP/1.1\r\nHost: localhost\r\n\r\n"
STATUS=$(send_request "$REQUEST")
check_status "POST without Content-Length should return 411" "411" "$STATUS"
sleep 1

# ============================================
# TEST 7: PUT without Content-Length (411 Length Required)
# ============================================
#print_test "Test 7: PUT without Content-Length"
#REQUEST="PUT /file.txt HTTP/1.1\r\nHost: localhost\r\n\r\n"
#STATUS=$(send_request "$REQUEST")
#check_status "PUT without Content-Length should return 411" "411" "$STATUS"
#sleep 1

# ============================================
# TEST 8: Payload Too Large (413)
# ============================================
print_test "Test 8: Payload too large (exceeds MAX_BODY_SIZE)"
# MAX_BODY_SIZE is 4096, so send 5000
LARGE_BODY=$(printf 'A%.0s' {1..5000})
REQUEST="POST /submit HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5000\r\n\r\n$LARGE_BODY"
STATUS=$(send_request "$REQUEST")
check_status "Body exceeding MAX_BODY_SIZE should return 413" "413" "$STATUS"
sleep 1

# ============================================
# TEST 9: URI Too Long (414)
# ============================================
print_test "Test 9: URI too long (exceeds MAX_URI_LENGTH)"
# MAX_URI_LENGTH is 1024, so create a URI of 1100 chars
LONG_URI=$(printf 'A%.0s' {1..1100})
REQUEST="GET /$LONG_URI HTTP/1.1\r\nHost: localhost\r\n\r\n"
STATUS=$(send_request "$REQUEST")
check_status "URI exceeding MAX_URI_LENGTH should return 414" "414" "$STATUS"
sleep 1

# ============================================
# TEST 10: Unsupported Media Type (415)
# ============================================
print_test "Test 10: Unsupported Content-Type"
REQUEST="POST /submit HTTP/1.1\r\nHost: localhost\r\nContent-Type: unsupported/type\r\nContent-Length: 5\r\n\r\nhello"
STATUS=$(send_request "$REQUEST")
check_status "Unsupported Content-Type should return 415" "415" "$STATUS"
sleep 1

# ============================================
# TEST 11: Unknown Method (501 Not Implemented)
# ============================================
print_test "Test 11: Unknown HTTP method"
REQUEST="PATCH /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n"
STATUS=$(send_request "$REQUEST")
check_status "Unknown HTTP method should return 501" "501" "$STATUS"
sleep 1

# ============================================
# TEST 12: Unknown Method (OPTIONS)
# ============================================
print_test "Test 12: OPTIONS method (not implemented)"
REQUEST="OPTIONS /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n"
STATUS=$(send_request "$REQUEST")
check_status "OPTIONS method should return 501" "501" "$STATUS"
sleep 1

# ============================================
# TEST 13: Unsupported HTTP Version (505)
# ============================================
print_test "Test 13: Unsupported HTTP version (HTTP/2.0)"
REQUEST="GET /index.html HTTP/2.0\r\nHost: localhost\r\n\r\n"
STATUS=$(send_request "$REQUEST")
check_status "HTTP/2.0 should return 505" "505" "$STATUS"
sleep 1

# ============================================
# TEST 14: Unsupported HTTP Version (HTTP/1.0)
# ============================================
print_test "Test 14: Unsupported HTTP version (HTTP/1.0)"
REQUEST="GET /index.html HTTP/1.0\r\nHost: localhost\r\n\r\n"
STATUS=$(send_request "$REQUEST")
check_status "HTTP/1.0 should return 505" "505" "$STATUS"
sleep 1

# ============================================
# TEST 15: Valid POST with Content-Length
# ============================================
print_test "Test 15: Valid POST request with Content-Length"
REQUEST="POST /submit HTTP/1.1\r\nHost: localhost\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 13\r\n\r\nname=testuser"
STATUS=$(send_request "$REQUEST")
check_status "Valid POST with Content-Length should return 200" "200" "$STATUS"
sleep 1

# ============================================
# TEST 16: Valid DELETE Request
# ============================================
print_test "Test 16: Valid DELETE request"
REQUEST="DELETE /file.txt HTTP/1.1\r\nHost: localhost\r\n\r\n"
STATUS=$(send_request "$REQUEST")
# DELETE might return 200, 204, or 404 depending on if file exists
# We just check it's not an error code we're testing for
if [[ "$STATUS" =~ ^(200|204|404)$ ]]; then
    print_pass "Valid DELETE request processed correctly"
else
    print_fail "Valid DELETE request" "200/204/404" "$STATUS"
fi
sleep 1

# ============================================
# TEST 8: Payload Too Large (413)
# ============================================
print_test "Test 8: Payload too large (exceeds MAX_BODY_SIZE)"
# MAX_BODY_SIZE is 4096, so send 5000
LARGE_BODY=$(printf 'A%.0s' {1..5000})
REQUEST="POST /submit HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5000\r\n\r\n$LARGE_BODY"
STATUS=$(send_request "$REQUEST")
check_status "Body exceeding MAX_BODY_SIZE should return 413" "413" "$STATUS"
sleep 1

# ============================================
# SUMMARY
# ============================================
echo ""
echo "======================================"
echo "  Test Summary"
echo "======================================"
echo -e "${GREEN}Passed: $PASS${NC}"
echo -e "${RED}Failed: $FAIL${NC}"
echo "Total:  $((PASS + FAIL))"
echo ""

if [ $FAIL -eq 0 ]; then
    echo -e "${GREEN}✓ All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}✗ Some tests failed${NC}"
    exit 1
fi
