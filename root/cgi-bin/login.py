#!/usr/bin/env python3
# session/login.py
import os
import sys
import sqlite3
import hashlib
import html
import uuid

def print_http_header(header):
    print(header, end="\r\n")

# Helper functions (same as register.py plus session management)
def get_db():
    return sqlite3.connect('users.db')

def hash_password(password):
    return hashlib.sha256(password.encode()).hexdigest()

def parse_form_data():
    data = {}
    if 'CONTENT_LENGTH' in os.environ:
        length = int(os.environ['CONTENT_LENGTH'])
        raw_data = sys.stdin.read(length)
        pairs = raw_data.split('&')
        for pair in pairs:
            key, value = pair.split('=')
            data[key] = html.escape(value)
        data["raw_data"] = raw_data
    return data

def set_cookie(name, value):
    print_http_header(f"Set-Cookie: {name}={value}; Path=/")


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

# Login handler
def main():
    error = ""
    if os.environ.get('REQUEST_METHOD') == 'POST':
        form = parse_form_data()
        username = form.get('username', '')
        password = form.get('password', '')
        
        if not username or not password:
            error = f"Both fields are required {username}, {password}, {form.get('raw_data', '')}"
        else:
            conn = get_db()
            cursor = conn.cursor()
            cursor.execute("SELECT id FROM users WHERE username=? AND password=?", 
                         (username, hash_password(password)))
            user = cursor.fetchone()
            
            if user:
                session_id = str(uuid.uuid4())
                cursor.execute("UPDATE users SET session_id=? WHERE id=?", 
                             (session_id, user[0]))
                conn.commit()
                set_cookie('session_id', session_id)
                # Redirect to profile
                print_http_header("Status: 302 Found")
                print_http_header("Location: profile.py")
                print("\r\n", end="")
                return
            else:
                error = f"Invalid username or password {username}, {password}, {form.get('raw_data', '')}"
    
    # Show login form
    body = f'<p class="error">{error}</p>' if error else ''
    body += """<form method="POST">
        <div class="form-group">
            <label>Username:</label>
            <input type="text" name="username" required>
        </div>
        <div class="form-group">
            <label>Password:</label>
            <input type="password" name="password" required>
        </div>
        <input type="submit" value="Login">
    </form>
    <p>Don't have an account? <a href="register.py">Register here</a></p>"""
    html_response("Login", body)

if __name__ == '__main__':
    main()
