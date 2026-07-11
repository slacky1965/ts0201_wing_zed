import struct

f1_path = 'ts0201_wing_orig_OTA.bin'
f2_path = 'doc_local/ts0201_wing_512_OTA_from_Oleg.bin'

f1 = open(f1_path,'rb').read()
f2 = open(f2_path,'rb').read()

# Our dump starts at 0x4D000 (OTA area), not 0x00000
OUR_DUMP_START = 0x4D000
OUR_DUMP_END = OUR_DUMP_START + len(f1)

print(f'Our dump: {len(f1)} bytes, covers 0x{OUR_DUMP_START:06X}-0x{OUR_DUMP_END:06X}')
print(f'Oleg dump: {len(f2)} bytes, covers 0x00000-0x{len(f2):06X}')
print()

# Compare in the overlapping region
overlap_start = OUR_DUMP_START
overlap_end = min(len(f1) + OUR_DUMP_START, len(f2))

print(f'=== Comparing 0x{overlap_start:06X}-0x{overlap_end:06X} ===')
print()

# Sector-by-sector comparison in the bad area
for sector in range(0x66000, 0x7A000, 0x1000):
    if sector < OUR_DUMP_START or sector >= len(f1) + OUR_DUMP_START:
        continue
    
    off1 = sector - OUR_DUMP_START
    off2 = sector
    
    chunk1 = f1[off1:off1+0x1000]
    chunk2 = f2[off2:off2+0x1000]
    
    if len(chunk1) == 0 or len(chunk2) == 0:
        continue
    
    nonff1 = sum(1 for b in chunk1 if b != 0xFF)
    nonff2 = sum(1 for b in chunk2 if b != 0xFF)
    
    # Compare byte-by-byte
    diffs = 0
    for i in range(min(len(chunk1), len(chunk2))):
        if chunk1[i] != chunk2[i]:
            diffs += 1
    
    print(f'0x{sector:06X}:')
    print(f'  Our dump:  {nonff1:4d}/4096 non-FF')
    print(f'  Oleg dump: {nonff2:4d}/4096 non-FF')
    print(f'  Diffs:     {diffs:4d}/4096 bytes differ')
    
    if diffs == 0 and nonff1 == nonff2:
        print(f'  Status: IDENTICAL')
    elif nonff1 < 100 and nonff2 > 100:
        print(f'  Status: Our dump has OLD DATA (wasnt erased)')
    elif nonff2 < 100 and nonff1 > 100:
        print(f'  Status: Oleg has old data, ours has new')
    else:
        print(f'  Status: DIFFERENT')
    print()

# Check NV headers
print('=== NV header check in bad area ===')
nv_pattern = bytes([0x5A, 0x5A, 0x04, 0x00])
for sector in [0x66000, 0x6E000, 0x76000]:
    if sector >= OUR_DUMP_START and sector < len(f1) + OUR_DUMP_START:
        off1 = sector - OUR_DUMP_START
        chunk1 = f1[off1:off1+16]
        match1 = chunk1[:4] == nv_pattern
        
        off2 = sector
        chunk2 = f2[off2:off2+16]
        match2 = chunk2[:4] == nv_pattern
        
        print(f'0x{sector:06X}:')
        print(f'  Our dump:  {" ".join(f"{b:02X}" for b in chunk1)} NV={match1}')
        print(f'  Oleg dump: {" ".join(f"{b:02X}" for b in chunk2)} NV={match2}')
        print()
