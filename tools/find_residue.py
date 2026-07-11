with open('ts0201_wing_orig512.bin','rb') as f: flash=f.read()
ota=flash[0x4D000:0x74000]
regions=[]
start=None
for i,b in enumerate(ota):
    if b!=0xFF:
        if start is None: start=i
    else:
        if start is not None:
            regions.append((start,i-1))
            start=None
if start is not None: regions.append((start,len(ota)-1))
print(f'Non-0xFF regions in OTA (0x4D000-0x74000):')
for s,e in regions:
    data=ota[s:e+1]
    print(f'  0x{0x4D000+s:05X}-0x{0x4D000+e:05X} ({e-s+1} bytes):')
    for row in range(0, len(data), 16):
        chunk=data[row:row+16]
        hex_str=' '.join(f'{b:02X}' for b in chunk)
        print(f'    +0x{s+row:05X}: {hex_str}')
