#!/usr/bin/env python3
import re,os
occ_file='/tmp/add_error_results.txt'
out='/tmp/add_error_expanded.txt'
if not os.path.exists(occ_file):
    print('occurrence file missing',occ_file); raise SystemExit(1)
lines=open(occ_file,'r',encoding='utf-8',errors='replace').read().splitlines()
occ=[]
for L in lines:
    if not L.strip(): continue
    m=re.match(r'(.+?):(\d+):\s*(.+)',L)
    if m:
        occ.append((m.group(1),int(m.group(2)),m.group(3)))
# filter unique
occ2=[]
seen=set()
for f,ln,code in occ:
    key=(f,ln)
    if key in seen: continue
    seen.add(key)
    # skip literal messages
    if 'add_error' in code and ('"' in code or "'" in code):
        continue
    occ2.append((f,ln,code))

with open(out,'w',encoding='utf-8') as fo:
    for f,ln,code in occ2:
        fo.write(f'FILE: {f}\nLINE: {ln}\nCALL: {code}\n')
        try:
            with open(f,'r',encoding='utf-8',errors='replace') as fh:
                file_lines=fh.read().splitlines()
        except Exception as e:
            fo.write('FAILED TO READ FILE: '+str(e)+'\n\n')
            continue
        start=max(0,ln-60-1); end=min(len(file_lines),ln+5)
        fo.write(f'--- Context (lines {start+1}-{end}) ---\n')
        for i in range(start,end):
            prefix='>' if i==(ln-1) else ' '
            fo.write(f"{prefix} {i+1:5d}: {file_lines[i]}\n")
        fo.write('\n-- Message variable name candidates --\n')
        # extract message expression inside add_error(..., <msg>, ...)
        m=re.search(r'add_error\s*\(.*?,.*?,\s*([^,\)]+)', code)
        var=m.group(1).strip() if m else 'msg'
        varname=re.sub(r"\[.*\]","",var).split()[-1]
        fo.write(f'VAR_EXPR: {var}  VAR_NAME: {varname}\n')
        fo.write('-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --\n')
        sstart=max(0,ln-200-1)
        found=False
        for j in range(sstart,ln-1):
            lj=file_lines[j]
            if varname in lj and any(k in lj for k in ('sprintf','snprintf','asprintf','vasprintf','strcpy','strncpy','strcat','strncat','= ')):
                fo.write(f'L{j+1}: {lj}\n')
                found=True
        if not found:
            fo.write('NONE FOUND - manual review needed\n')
        fo.write('\n'+'='*72+'\n\n')
print('WROTE',out)
