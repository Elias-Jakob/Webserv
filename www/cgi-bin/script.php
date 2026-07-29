#!/usr/bin/env php-cgi
<?php

header("Content-Type: text/html");

echo "PHP-CGI funktioniert!\n\r";
echo "PHP Version: " . PHP_VERSION . "\n\r";
echo "SAPI: " . PHP_SAPI . "\n\r";
echo "REQUEST_METHOD: " . ($_SERVER['REQUEST_METHOD'] ?? '-') . "\n\r";
echo "QUERY_STRING: " . ($_SERVER['QUERY_STRING'] ?? '-') . "\n\r";
echo "SCRIPT_NAME: " . ($_SERVER['SCRIPT_NAME'] ?? '-') . "\n\r";
echo "SCRIPT_FILENAME: " . ($_SERVER['SCRIPT_FILENAME'] ?? '-') . "\n\r";
