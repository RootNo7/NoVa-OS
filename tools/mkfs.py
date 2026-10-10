#!/usr/bin/env python3
import os, struct, sys

if len(sys.argv) != 3:
    raise SystemExit("usage: mkfs.py <source_dir> <output.bin>")
src, out = sys.argv[1], sys.argv[2]
files=[]
for name in sorted(os.listdir(src)):
    p=os.path.join(src,name)
    if os.path.isfile(p):
        data=open(p,'rb').read()
        files.append((name[:31],data))
if len(files)>11:
    raise SystemExit('max 12 files')
header=bytearray(512)
struct.pack_into('<I',header,0,0x3153564e)
struct.pack_into('<I',header,4,1)
struct.pack_into('<I',header,8,len(files))
base_lba=1025+1
cursor=base_lba
for i,(name,data) in enumerate(files):
    off=16+i*44
    nameb=name.encode('ascii','replace')[:31]+b'\0'
    header[off:off+32]=nameb.ljust(32,b'\0')
    struct.pack_into('<I',header,off+32,len(data))
    struct.pack_into('<I',header,off+36,cursor)
    struct.pack_into('<I',header,off+40,0o644)
    cursor += (len(data)+511)//512
blob=bytearray(header)
for name,data in files:
    blob.extend(data)
    blob.extend(b'\0' * (((len(data)+511)//512)*512-len(data)))
open(out,'wb').write(blob)
