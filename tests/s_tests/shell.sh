#!/bin/bash

REQUESTS=10000

for ((i=1; i<=REQUESTS; i++))
do
  curl -s http://localhost:9999 > /dev/null
  echo "Request #$i sent"
done

