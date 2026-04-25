#!/usr/bin/env python3
import re

ref_file = "6mOnec4.out"
cur_file = "current_output.out"

with open(ref_file) as f:
    ref_lines = f.readlines()
with open(cur_file) as f:
    cur_lines = f.readlines()

print(f"REF lines: {len(ref_lines)}, CUR lines: {len(cur_lines)}, diff: {len(cur_lines) - len(ref_lines)}")

# Find where they start to diverge
min_len = min(len(ref_lines), len(cur_lines))
divergence_point = None
last_match = 0

for i in range(min_len):
    # Check if ref line i matches cur line i
    if ref_lines[i].rstrip('\r\n') == cur_lines[i].rstrip('\r\n'):
        last_match = i
    else:
        if divergence_point is None and i > 100:  # Skip early differences
            divergence_point = i
            break

if divergence_point:
    print(f"\nFirst major divergence at line {divergence_point+1}:")
    print(f"REF[{divergence_point}]: {repr(ref_lines[divergence_point][:80])}")
    print(f"CUR[{divergence_point}]: {repr(cur_lines[divergence_point][:80])}")
    
    # Show context
    print(f"\nContext (±3 lines):")
    for j in range(max(0, divergence_point-3), min(min_len, divergence_point+4)):
        match = "✓" if ref_lines[j].rstrip('\r\n') == cur_lines[j].rstrip('\r\n') else "✗"
        print(f"{match} {j+1:5d} REF: {ref_lines[j][:70].rstrip()}")
        if j < len(cur_lines):
            print(f"       {j+1:5d} CUR: {cur_lines[j][:70].rstrip()}")
else:
    print(f"\nNo major divergence found in common {min_len} lines")
    print(f"Last matching line: {last_match+1}")
    if last_match < min_len - 1:
        print(f"\nFirst difference after line {last_match+1}:")
        print(f"REF[{last_match+1}]: {repr(ref_lines[last_match+1][:80])}")
        print(f"CUR[{last_match+1}]: {repr(cur_lines[last_match+1][:80])}")
