import struct

f1_path = 'ts0201_wing_orig_OTA.bin'
f2_path = 'doc_local/ts0201_wing_512_OTA_from_Oleg.bin'

f1 = open(f1_path,'rb').read()
f2 = open(f2_path,'rb').read()

print(f'File 1 (our dump): {len(f1)} bytes ({len(f1)//1024} KB)')
print(f'File 2 (Oleg dump): {len(f2)} bytes ({len(f2)//1024} KB)')
print()

# Our dump is only ~156KB, covers up to ~0x27000
# Bad area 0x66000+ is beyond our dump
# So we can only compare app FW area

print('=== App FW comparison (0x10000) ===')
a1 = f1[0x10000:0x10000+64]
a2 = f2[0x10000:0x10000+64]
print(f'Our dump:    {" ".join(f"{b:02X}" for b in a1)}')
print(f'Oleg dump:   {" ".join(f"{b:02X}" for b in a2)}')

diffs = 0
for i in range(min(len(a1), len(a2))):
    if a1[i] != a2[i]:
        print(f'  Diff at +0x{i:02X}: ours=0x{a1[i]:02X} oleg=0x{a2[i]:02X}')
        diffs += 1
if diffs == 0:
    print('  Identical!')
print()

# Check TLNK
tlnk1 = struct.unpack_from('<I', f2, 0x10008)[0]
print(f'App FW TLNK at 0x10008: 0x{tlnk1:08X} (0x544C4E4B=valid)')
if len(f2) > 0x4D00C:
    tlnk2 = struct.unpack_from('<I', f2, 0x4D008)[0]
    print(f'OTA TLNK at 0x4D008:    0x{tlnk2:08X} (0x544C4EFF=poisoned)')
print()

# Full sector scan of Oleg dump
print('=== Oleg dump: full sector scan ===')
for a in range(0, min(len(f2), 0x80000), 0x1000):
    chunk = f2[a:a+0x1000]
    nonff = sum(1 for b in chunk if b != 0xFF)
    pct = nonff * 100 // 4096
    if nonff == 0:
        print(f'0x{a:06X}: ALL FF')
    elif nonff < 100:
        sample = ' '.join(f'{b:02X}' for b in chunk[:16])
        print(f'0x{a:06X}: {nonff:4d}/4096 ({pct:3d}%) {sample} ...')
    else:
        print(f'0x{a:06X}: {nonff:4d}/4096 ({pct:3d}%)')
print()

# NV data pattern check
print('=== NV header check (5A 5A 04 00) ===')
nv_pattern = bytes([0x5A, 0x5A, 0x04, 0x00])
for a in range(0x60000, min(len(f2), 0x80000), 0x1000):
    if f2[a:a+4] == nv_pattern:
        print(f'0x{a:06X}: NV header found: {" ".join(f"{f2[a+i]:02X}" for i in range(16))}')
