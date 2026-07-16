#!/usr/bin/env php-cgi
<?php

header("Content-Type: text/html");

echo "PHP-CGI funktioniert!\n\n";

echo "PHP Version: " . PHP_VERSION . "\n";
echo "SAPI: " . PHP_SAPI . "\n\n";

echo "REQUEST_METHOD: " . ($_SERVER['REQUEST_METHOD'] ?? '-') . "\n";
echo "QUERY_STRING: " . ($_SERVER['QUERY_STRING'] ?? '-') . "\n";
echo "SCRIPT_NAME: " . ($_SERVER['SCRIPT_NAME'] ?? '-') . "\n";
