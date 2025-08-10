
from client import send_raw_request
import os
import pathlib

DIR_PATH = pathlib.Path("./upload_tests")
HOST = "127.0.0.1"
PORT = 9999

def run_upload_test(req, res, file, host, port):
    got = send_raw_request(req, host, port)
    if res[0:20] != got[0:20]:
        print("❌")
        print("\t wrong response!")
        print(got)
        return

    uploaded_file = got.split(b'\n')[3].split(b' ')[-1][:-1]
    with open(pathlib.Path("..") / uploaded_file.decode(), 'rb') as got_file:
        if got_file.read() != file.read():
            print("❌")
            print("\t uncorrect file content!")
            return

    print("✅")
    return



def run_upload_tests():
    requests = [i for i in os.listdir(DIR_PATH) if i.startswith("req")]
    success = b''
    with open(str(DIR_PATH / "upload_success"), 'rb') as res:
        success = res.read()

    for req_file in requests:
        with open((DIR_PATH / req_file), 'rb') as req:
            with open(str(DIR_PATH / req_file).replace("req", "file"), 'rb') as file:
                print(f"[upload test] {req_file} ", end="")
                run_upload_test(req.read(), success, file, HOST, PORT)

if __name__ == "__main__":
    run_upload_tests()
