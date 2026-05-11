import re
from pathlib import Path

# Read files, normalize CRLF to LF for comparison
p2 = Path('tests/Patch2.out').read_bytes().replace(b'\r\n', b'\n').replace(b'\r', b'\n').decode('latin-1').splitlines()
p1 = Path('tests/Patch1nc.fortran.crlf.out').read_bytes().replace(b'\r\n', b'\n').replace(b'\r', b'\n').decode('latin-1').splitlines()

print(f"Patch2.out lines: {len(p2)}")
print(f"Patch1nc.fortran.crlf.out lines: {len(p1)}")

# A line is "numeric-only different" if after replacing all numbers with X, the lines match
def normalize_numbers(s):
    # Replace floating point / scientific notation numbers with placeholder
    # Using 'N' as in the prompt
    return re.sub(r'[-+]?\d+\.?\d*(?:[eE][-+]?\d+)?', 'N', s)

# Compare line by line
diffs = []
max_lines = max(len(p2), len(p1))
for i in range(max_lines):
    l2 = p2[i] if i < len(p2) else '<MISSING>'
    l1 = p1[i] if i < len(p1) else '<MISSING>'
    if l2 == l1:
        continue
    # Check if it's purely numeric difference
    n2 = normalize_numbers(l2)
    n1 = normalize_numbers(l1)
    if n2 == n1:
        # purely numeric, skip
        continue
    diffs.append((i+1, l2, l1))

print(f"\nTotal non-numeric differences: {len(diffs)}")
print("\n--- Differences (line#: Patch2 | Patch1nc.fortran.crlf) ---")
for lineno, l2, l1 in diffs[:200]:
    print(f"Line {lineno}:")
    print(f"  P2 : {repr(l2)}")
    print(f"  P1c: {repr(l1)}")
