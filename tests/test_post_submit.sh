#!/bin/bash

# Test POST form submission (application/x-www-form-urlencoded)

HOST="${1:-127.0.0.1}"
PORT="${2:-8080}"
SUBMIT_FILE="../www/submit/form_file.txt"

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
    echo -e "${RED}       $2${NC}"
    ((FAIL++))
}

send_form_request() {
    local data="$1"
    local request="POST /submit HTTP/1.1\r\nHost: localhost\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: ${#data}\r\n\r\n${data}"
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
        print_fail "$test_name" "Expected $expected, got $actual"
    fi
}

# Clean up old file
rm -f "$SUBMIT_FILE" 2>/dev/null

# Test 1: Submit simple form
print_test "Submit simple form data"
DATA="username=Kevin&email=kevin@example.com"
STATUS=$(send_form_request "$DATA")
check_status "Should return 201 (form submitted)" "201" "$STATUS"
sleep 0.5

# Test 2: Check if file was created
print_test "Verify form data written to file"
if [ -f "$SUBMIT_FILE" ]; then
    print_pass "Form file created"
else
    print_fail "Form file created" "File not found at $SUBMIT_FILE"
fi
sleep 0.5

# Test 3: Check file contains submitted data
print_test "Verify file contains correct data"
if grep -q "username = Kevin" "$SUBMIT_FILE"; then
    print_pass "File contains username field"
else
    print_fail "File contains username field" "Field not found in file"
fi

if grep -q "email = kevin@example.com" "$SUBMIT_FILE"; then
    print_pass "File contains email field"
else
    print_fail "File contains email field" "Field not found in file"
fi
sleep 0.5

# Test 4: Submit with special characters
print_test "Submit form with special characters"
DATA="message=Hello+World&text=Test%20Data"
STATUS=$(send_form_request "$DATA")
check_status "Should return 201" "201" "$STATUS"
sleep 0.5

# Test 5: Submit form with empty value
print_test "Submit form with empty field value"
DATA="name=&age=25"
STATUS=$(send_form_request "$DATA")
check_status "Should return 201 (empty values allowed)" "201" "$STATUS"
sleep 0.5

echo ""
echo -e "Results: ${GREEN}$PASS passed${NC}, ${RED}$FAIL failed${NC}"
exit $FAIL