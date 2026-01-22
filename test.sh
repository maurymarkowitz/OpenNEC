#!/bin/bash
cd /Volumes/Bigger/Users/maury/Desktop/OpenNEC
make clean
make
./onec test/example5.deck > test/example5.out 2>&1
tail -10 test/example5.out
