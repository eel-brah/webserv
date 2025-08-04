#!/usr/bin/env python3
import os
import sys

method = os.environ.get("REQUEST_METHOD", "GET")
query_string = os.environ.get("QUERY_STRING", "")
content_length = int(os.environ.get("CONTENT_LENGTH", 0))

body = ""
if method == "POST" and content_length > 0:
    body = sys.stdin.read(content_length)

sys.stdout.write("Content-Type: text/html\r\n")
sys.stdout.write("Status: 200 OK\r\n")
sys.stdout.write("\r\n")
sys.stdout.write("<html>\n")
sys.stdout.write("<body>\n")
sys.stdout.write("<h1>Hello, CGI!</h1>\n")
sys.stdout.write(f"<p>Method: {method}</p>\n")
sys.stdout.write(f"<p>Query String: {query_string}</p>\n")
if body:
    sys.stdout.write(f"<p>POST Body: {body}</p>\n")
sys.stdout.write("</body>\n")
sys.stdout.write("</html>\n")
sys.stdout.flush()
sys.exit(0)
