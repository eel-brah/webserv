#!/usr/bin/env python3
# session/profile.py
import os
import sys
import sqlite3
import html

def print_http_header(header):
    print(header, end="\r\n")

# Helper functions
def get_db():
    return sqlite3.connect('users.db')

def get_cookie(name):
    if 'HTTP_COOKIE' in os.environ:
        cookies = os.environ['HTTP_COOKIE'].split('; ')
        for cookie in cookies:
            if cookie.startswith(name + '='):
                return cookie.split('=')[1][:-2]
    return None

def html_response(title, body):
    print_http_header("Content-Type: text/html")
    print("\r\n", end="")
    print(f"""<!DOCTYPE html>
<html>
<head>
    <title>{title}</title>
    <style>/* Same styles as register.py */</style>
</head>
<body>
    <h1>{title}</h1>
    {body}
</body></html>""")

# Profile handler
def main():
    session_id = get_cookie(' session_id')
    user = None
    logout = False
    
    # Check for logout action
    if os.environ.get('QUERY_STRING') == 'action=logout':
        if session_id:
            conn = get_db()
            cursor = conn.cursor()
            cursor.execute("UPDATE users SET session_id=NULL WHERE session_id=?", 
                         (session_id,))
            conn.commit()
        # Clear cookie and redirect
        print_http_header("Set-Cookie: session_id=; expires=Thu, 01 Jan 1970 00:00:00 GMT; Path=/")
        print_http_header("Status: 302 Found")
        print_http_header("Location: login.py")
        print("\r\n", end="")
        return
    
    # Fetch user if session exists
    if session_id:
        conn = get_db()
        cursor = conn.cursor()
        cursor.execute("SELECT username FROM users WHERE session_id=?", (session_id,))
        user = cursor.fetchone()
    
    if user:
        body = f"<p>Welcome, {html.escape(user[0])}!</p>"
        body += '<p><a href="profile.py?action=logout">Logout</a></p>'
    else:
        body = f'<p class="error">Please login to view your profile</p>'
        body += '<p><a href="login.py">Login</a></p>'
    
    html_response("Your Profile", body)

if __name__ == '__main__':
    main()
