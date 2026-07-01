#!/bin/bash

# Master Test Suite - Runs all webserv tests
# Usage: ./test_all.sh

HOST="127.0.0.1"
PORT="8080"
WEBSERV_CONF="${WEBSERV_CONF:-../confFiles/webserv.conf}"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

TOTAL_PASS=0
TOTAL_FAIL=0

print_header() {
    echo -e "\n${BLUE}========================================${NC}"
    echo -e "${BLUE}  $1${NC}"
    echo -e "${BLUE}========================================${NC}\n"
}

check_server() {
    if ! nc -z $HOST $PORT 2>/dev/null; then
        echo -e "${RED}[ERROR]${NC} Server not running on $HOST:$PORT"
        echo "Start the server with: ./webserv $WEBSERV_CONF"
        exit 1
    fi
}

run_test_suite() {
    local test_name="$1"
    local script="$2"
    
    print_header "$test_name"
    if [ -f "$script" ]; then
        bash "$script" "$HOST" "$PORT"
        local exit_code=$?
        if [ $exit_code -ne 0 ]; then
            echo -e "${RED}[SUITE FAILED]${NC} $test_name exited with code $exit_code"
            ((TOTAL_FAIL += exit_code))
        fi
    else
        echo -e "${RED}[ERROR]${NC} Test script not found: $script"
        ((TOTAL_FAIL++))
    fi
}

main() {
    echo -e "${GREEN}Starting Webserv Test Suite${NC}"
    check_server
    
    run_test_suite "GET Request Tests" "test_get.sh"
    run_test_suite "POST Content-Type Validation Tests" "test_post_content_type.sh"
    run_test_suite "POST File Extension Validation Tests" "test_post_extensions.sh"
    run_test_suite "POST Form Submission Tests" "test_post_submit.sh"
    run_test_suite "DELETE Request Tests" "test_delete.sh"
    
    print_header "All Tests Complete"
    
    if [ $TOTAL_FAIL -eq 0 ]; then
        echo -e "${GREEN}✓ All test suites passed!${NC}"
        exit 0
    else
        echo -e "${RED}✗ $TOTAL_FAIL test(s) failed${NC}"
        exit 1
    fi
}

main "$@"