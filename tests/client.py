
import socket


def send_raw_request(request, host, port):
    response = b""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((host, port))
        s.sendall(request)
        s.settimeout(1)
        try:
            while True:
                data = s.recv(4096)
                if not data:
                    break
                response += data
        except socket.timeout:
            pass
    return response


if __name__ == "__main__":
    with open('./test_cases/req1', 'rb') as req:
        print(send_raw_request(req.read(), '127.0.0.1', 9999))
