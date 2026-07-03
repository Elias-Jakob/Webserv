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
# sleep 1

# ============================================
# TEST 1: DELETE file1.txt
# ============================================
print_test "Test 1: Valid DELTE of file.txt"
echo "test1 creating test1.txt" > ./www/uploads/test1.txt
REQUEST="DELETE /www/uploads/test1.txt HTTP/1.1\r\nHost: localhost\r\n\r\n"
STATUS=$(send_request "$REQUEST")
check_status "Valid DELETE request should return 204" "204" "$STATUS"
# sleep 1

# ============================================
# TEST 2: DELETE file1.txt not there
# ============================================
print_test "Test 1: Valid DELTE of file.txt"
# echo "test1 creating test1.txt" > ./www/uploads/test1.txt
REQUEST="DELETE /www/uploads/test1.txt HTTP/1.1\r\nHost: localhost\r\n\r\n"
STATUS=$(send_request "$REQUEST")
check_status "Inalid DELETE request should return 404" "404" "$STATUS"
# sleep 1

# ============================================
# TEST 3: DELETE file1.txt not there
# ============================================
print_test "Test 1: Valid DELTE of file.txt"
# echo "test1 creating test1.txt" > ./www/uploads/test1.txt
REQUEST="DELETE /www/test_uploads/test42.txt HTTP/1.1\r\nHost: localhost\r\n\r\n"
STATUS=$(send_request "$REQUEST")
check_status "Inalid DELETE request should return 405" "405" "$STATUS"
# sleep 1

