#!/bin/bash

# Number of parallel requests
PARALLEL_REQUESTS=10
TOTAL_REQUESTS=1000

# Run parallel curl requests
seq $TOTAL_REQUESTS | parallel -j $PARALLEL_REQUESTS curl -s http://localhost:9999 > /dev/null

