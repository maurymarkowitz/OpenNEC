#!/usr/bin/env python3
import re,os
expanded='/Volumes/Bigger/Users/maury/Desktop/OpenNEC/doc/error_messages_expanded.md'
out='/Volumes/Bigger/Users/maury/Desktop/OpenNEC/doc/error_messages_clean.md'
src_root='/Volumes/Bigger/Users/maury/Desktop/OpenNEC'
# load expanded blocks
blocks={}
if os.path.exists(expanded):
    data=open(expanded,'r',encoding='utf-8',errors='replace').read()
    parts=data.split('\n================================================================================\n')
    for p in parts:
        m=re.search(r'^FILE: (.+)\nLINE: (\d+)',p,re.M)
        if not m: continue
        key=(m.group(1),int(m.group(2)))
        blocks[key]=p

# helper to extract format from snprintf/asprintf/sprintf
fmt_re=re.compile(r'(?:snprintf|sprintf|asprintf|vasprintf)\s*\(.*?,.*?,\s*"((?:\\"|[^"])*)"')
# find add_error occurrences in source
results=[]
for dirpath,dirnames,filenames in os.walk(src_root):
    for f in filenames:
        if not f.endswith(('.c','.h')): continue
        p=os.path.join(dirpath,f)
        try:
            txt=open(p,'r',encoding='utf-8',errors='replace').read()
        except:
            continue
        for m in re.finditer(r'add_error\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,\)]+)\s*\)', txt):
            # determine line number
            start=m.start()
            ln=txt.count('\n',0,start)+1
            msg_expr=m.group(3).strip()
            # literal?
            lit=re.match(r'"((?:\\"|[^"])*)"', msg_expr)
            if lit:
                results.append((p,ln,lit.group(1), 'LITERAL'))
                continue
            # else try expanded block
            key=(p,ln)
            fmt=None
            if key in blocks:
                block=blocks[key]
                # find Lxxx lines with snprintf etc
                for lm in re.finditer(r'^L(\d+):\s*(.*)$', block, re.M):
                    line_text=lm.group(2)
                    fm=fmt_re.search(line_text)
                    if fm:
                        fmt=fm.group(1)
                        break
                # if not found, search whole block for snprintf pattern
                if not fmt:
                    fm=fmt_re.search(block)
                    if fm: fmt=fm.group(1)
            # fallback: search upward in source file for snprintf assigning to var
            if not fmt:
                # extract var name
                varname=re.sub(r"\[.*\]","",msg_expr).split()[-1]
                # search prior 200 lines
                lines=txt.splitlines()
                sidx=max(0,ln-200-1)
                for j in range(sidx,ln-1):
                    lj=lines[j]
                    fm=fmt_re.search(lj)
                    if fm and varname in lj:
                        fmt=fm.group(1)
                        break
            if fmt:
                results.append((p,ln,fmt,'FORMAT'))
            else:
                results.append((p,ln,msg_expr,'MANUAL'))

# write cleaned output
with open(out,'w',encoding='utf-8') as fo:
    fo.write('# Clean error message list\n')
    fo.write('# Format: file:line -> "format string" (source)\n\n')
    for p,ln,msg,kind in results:
        if kind=='LITERAL' or kind=='FORMAT':
            fo.write(f'{p}:{ln} -> "{msg}"\n')
        else:
            # strip whitespace
            fo.write(f'{p}:{ln} -> MANUAL: {msg}\n')
print('WROTE',out)
