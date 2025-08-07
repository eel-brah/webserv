import requests

requests_count = 10000
url = "http://localhost:9999"

for i in range(requests_count):
    response = requests.get(url)
    print(f"Request #{i+1} sent, Status Code: {response.status_code}")
