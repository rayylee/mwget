#!/usr/bin/env bash

commands=("gnuplot" "mwget" "wget" "curl")

for cmd in "${commands[@]}"; do
    if ! command -v "$cmd" &> /dev/null; then
        echo "Error: command '$cmd' not found"
        exit 1
    fi
done

./benchmark.sh -s -b http

