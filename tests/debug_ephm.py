ref_file = open('6mOnec4.out').readlines()
cur_file = open('current_output.out').readlines()

# Find the first radiation patterns section for each frequency
def find_first_radiation_data(file_lines):
    for i, line in enumerate(file_lines):
        if 'RADIATION PATTERNS' in line and '- - -' in line:
            # Skip header lines and find first data line
            for j in range(i+5, min(i+20, len(file_lines))):
                if file_lines[j].strip() and '.' in file_lines[j]:
                    return file_lines[j], i
    return None, None

# Get first radiation pattern data for reference
ref_data, ref_header_line = find_first_radiation_data(ref_file)
cur_data, cur_header_line = find_first_radiation_data(cur_file)

if ref_data and cur_data:
    print("First radiation pattern data line:")
    print(f"Reference: {ref_data.rstrip()}")
    print(f"Current:   {cur_data.rstrip()}")
    
    # Parse the values
    ref_parts = ref_data.split()
    cur_parts = cur_data.split()
    
    if len(ref_parts) >= 11 and len(cur_parts) >= 11:
        # ephm is typically the second-to-last value
        print(f"\nRef ephm (2nd-to-last): {ref_parts[-2]}")
        print(f"Cur ephm (2nd-to-last): {cur_parts[-2]}")

# Now find the first radiation patterns section for SECOND frequency
def find_nth_radiation_header(file_lines, n):
    count = 0
    for i, line in enumerate(file_lines):
        if 'RADIATION PATTERNS' in line and '- - -' in line:
            count += 1
            if count == n:
                return i
    return -1

ref_freq2_header = find_nth_radiation_header(ref_file, 2)
cur_freq2_header = find_nth_radiation_header(cur_file, 2)

print(f"\n\nSecond frequency radiation pattern header:")
print(f"Reference at line: {ref_freq2_header + 1}")
print(f"Current at line: {cur_freq2_header + 1}")

# Find first data line after second frequency header
if ref_freq2_header > 0:
    for j in range(ref_freq2_header + 5, min(ref_freq2_header + 20, len(ref_file))):
        if ref_file[j].strip() and '.' in ref_file[j]:
            print(f"\nRef 2nd freq first data: {ref_file[j].rstrip()}")
            ref_parts = ref_file[j].split()
            print(f"  ephm={ref_parts[-2]}")
            break

if cur_freq2_header > 0:
    for j in range(cur_freq2_header + 5, min(cur_freq2_header + 20, len(cur_file))):
        if cur_file[j].strip() and '.' in cur_file[j]:
            print(f"Cur 2nd freq first data: {cur_file[j].rstrip()}")
            cur_parts = cur_file[j].split()
            print(f"  ephm={cur_parts[-2]}")
            break
