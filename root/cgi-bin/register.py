#!/usr/bin/env python3
# session/register.py
import os
import sys
import sqlite3
import hashlib
import html

# Database setup
DB_PATH = 'users.db'
conn = sqlite3.connect(DB_PATH)
conn.execute('''CREATE TABLE IF NOT EXISTS users (
                id INTEGER PRIMARY KEY,
                username TEXT UNIQUE,
                password TEXT,
                session_id TEXT)''')
conn.commit()
conn.close()

def print_http_header(header):
    print(header, end="\r\n")

# Helper functions
def get_db():
    return sqlite3.connect(DB_PATH)

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
            data[key] = html.escape(value)  # Basic XSS protection
    return data

def html_response(title, body):
    print_http_header("content-type: text/html")
    print_http_header("status: 200 OK")
    print("\r\n", end="")

    print(f"""<!DOCTYPE html>
<html>
<head>
    <title>{title}</title>
    <style>
        body {{ font-family: Arial, sans-serif; max-width: 500px; margin: 0 auto; padding: 20px; }}
        .form-group {{ margin-bottom: 15px; }}
        label {{ display: block; margin-bottom: 5px; }}
        input[type="text"], input[type="password"] {{ width: 100%; padding: 8px; }}
        .error {{ color: red; }}
    </style>
</head>
<body>
    <h1>{title}</h1>
    {body}
</body></html>""")

# Registration handler
def main():
    error = ""
    if os.environ.get('REQUEST_METHOD') == 'POST':
        form = parse_form_data()
        username = form.get('username', '')
        password = form.get('password', '')
        
        if not username or not password:
            error = "Both fields are required"
        else:
            try:
                conn = get_db()
                cursor = conn.cursor()
                cursor.execute("INSERT INTO users (username, password) VALUES (?, ?)", 
                              (username, hash_password(password)))
                conn.commit()
                # Redirect to login
                print_http_header("Status: 302 Found")
                print_http_header("Location: login.py")
                print("\r\n", end="")
                return
            except sqlite3.IntegrityError:
                error = "Username already exists"
    
    # Show registration form
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
        <input type="submit" value="Register">
    </form>
    <p>Already have an account? <a href="login.py">Login here</a></p>"""
    html_response("Register", body)

if __name__ == '__main__':
    main()
    sys.stdout.flush()
