#!/usr/bin/env php
<?php
$method = $_SERVER['REQUEST_METHOD'] ?? 'GET';
$query_string = $_SERVER['QUERY_STRING'] ?? '';

$body = '';
if ($method === 'POST') {
    $content_length = (int)($_SERVER['CONTENT_LENGTH'] ?? 0);
    if ($content_length > 0) {
        $body = file_get_contents('php://input');
    }
}

header("Content-Type: text/html");
header("Status: 200 OK");
echo "<html>\n";
echo "<body>\n";
echo "<h1>Hello, CGI!</h1>\n";
echo "<p>Method: " . htmlspecialchars($method) . "</p>\n";
echo "<p>Query String: " . htmlspecialchars($query_string) . "</p>\n";
if ($body !== '') {
    echo "<p>POST Body: " . htmlspecialchars($body) . "</p>\n";
}
echo "</body>\n";
echo "</html>\n";

exit(0);
?>

