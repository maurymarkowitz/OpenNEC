#!/usr/bin/env python3
import os,re
root='/Volumes/Bigger/Users/maury/Desktop/OpenNEC'
out='/Volumes/Bigger/Users/maury/Desktop/OpenNEC/doc/error_messages_clean.md'
add_re=re.compile(r'add_error\s*\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,\)]+)')
# patterns to find format string
fmt_patterns=[
    re.compile(r'snprintf\s*\(\s*%s\s*,\s*[^,]+\s*,\s*"((?:\\"|[^"])+)"' ),
    re.compile(r'sprintf\s*\(\s*%s\s*,\s*"((?:\\"|[^"])+)"' ),
    re.compile(r'asprintf\s*\(\s*&?%s\s*,\s*"((?:\\"|[^"])+)"' ),
    re.compile(r'vasprintf\s*\(\s*&?%s\s*,\s*"((?:\\"|[^"])+)"' ),
]
# also look for direct assignment of literal: msg = "..."
assign_re=re.compile(r'\b([A-Za-z_][A-Za-z0-9_]*)\b\s*=\s*"((?:\\"|[^"])*)"')
results=[]
for dirpath,dirnames,filenames in os.walk(root):
    for fn in filenames:
        if not fn.endswith(('.c','.h')): continue
        fp=os.path.join(dirpath,fn)
        try:
            txt=open(fp,'r',encoding='utf-8',errors='replace').read()
        except Exception:
            continue
        for m in add_re.finditer(txt):
            start=m.start()
            ln=txt.count('\n',0,start)+1
            msg_expr=m.group(3).strip()
            # literal string directly in call
            lit=re.match(r'"((?:\\"|[^"])*)"', msg_expr)
            if lit:
                results.append((fp,ln,lit.group(1),'LITERAL'))
                continue
            # clean var name
            var=msg_expr
            # remove casts, parentheses
            var=re.sub(r'[()\*]','',var).strip()
            var=var.split()[-1]
            # search within 300 chars before add_error for inline snprintf (multi-line join)
            chunk_start=max(0,start-1000)
            chunk=txt[chunk_start:start]
            found=None
            # simplify chunk by removing newlines between parentheses to catch multiline calls
            # but preserve original for line numbers
            for pat in fmt_patterns:
                try:
                    patt=re.compile(pat.pattern.replace('%s',re.escape(var)), re.DOTALL)
                except re.error:
                    continue
                mm=patt.search(chunk)
                if mm:
                    found=mm.group(1)
                    break
            if not found:
                # search previous 200 lines for a line containing snprintf/asprintf with var as first arg
                lines=txt.splitlines()
                idx=ln-1
                sstart=max(0,idx-200)
                for j in range(idx-1, sstart-1, -1):
                    lj=lines[j]
                    for pat in fmt_patterns:
                        try:
                            patt=re.compile(pat.pattern.replace('%s',re.escape(var)))
                        except re.error:
                            continue
                        mm=patt.search(lj)
                        if mm:
                            found=mm.group(1); break
                    if found: break
                # try simple assignment
                if not found:
                    for j in range(idx-1, sstart-1, -1):
                        lj=lines[j]
                        mm=assign_re.search(lj.replace('%s',var))
                        # the assign_re needs %s, use direct construction
                        if re.search(r'\b'+re.escape(var)+r'\b\s*=\s*"', lj):
                            mm2=re.search(r'\b'+re.escape(var)+r'\b\s*=\s*"((?:\\"|[^"])*)"', lj)
                            if mm2:
                                found=mm2.group(1); break
            if found:
                results.append((fp,ln,found,'FORMAT'))
            else:
                results.append((fp,ln,msg_expr,'MANUAL'))
# write out
with open(out,'w',encoding='utf-8') as fo:
    fo.write('# Clean error message list\n')
    fo.write('# Format: file:line -> "format string" (source)\n\n')
    for fp,ln,msg,kind in results:
        if kind in ('LITERAL','FORMAT'):
            fo.write(f'{fp}:{ln} -> "{msg}"\n')
        else:
            fo.write(f'{fp}:{ln} -> MANUAL: {msg}\n')
print('WROTE',out)
