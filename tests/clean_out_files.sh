#!/usr/bin/env bash
# Remove generated .onec and .out files under test directory and subfolders

find test -type f \( -name '*.onec' -o -name '*.out' \) -print -delete
