


from client import send_raw_request
import os
import pathlib

DIR_PATH = pathlib.Path("./simple_tests")
HOST = "127.0.0.1"
PORT = 9999

def run_simple_test(req, res, host, port):
    got = send_raw_request(req, host, port)
    return got != res

def run_simple_tests():
    requests = [i for i in os.listdir(DIR_PATH) if i.startswith("req")]
    for req_file in requests:
        with open((DIR_PATH / req_file), 'rb') as req:
            with open(str(DIR_PATH / req_file).replace("req", "res"), 'rb') as res:
                if run_simple_test(req.read(), res.read(), HOST, PORT):
                    print(f"[simple test] {req_file} ✅")
                else:
                    print(f"[simple test] {req_file} ❌")

if __name__ == "__main__":
    run_simple_tests()
