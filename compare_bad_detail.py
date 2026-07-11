import struct

f1_path = 'ts0201_wing_orig_OTA.bin'
f2_path = 'doc_local/ts0201_wing_512_OTA_from_Oleg.bin'

f1 = open(f1_path,'rb').read()
f2 = open(f2_path,'rb').read()

OUR_DUMP_START = 0x4D000

print('=== Detailed comparison of bad area ===')
print(f'Our dump: 0x{OUR_DUMP_START:06X}-0x{OUR_DUMP_START+len(f1):06X}')
print(f'Oleg dump: 0x00000-0x{len(f2):06X}')
print()

# Show byte-by-byte in the first 16 bytes of each sector
for sector in range(0x66000, 0x7A000, 0x1000):
    off1 = sector - OUR_DUMP_START
    if off1 < 0 or off1 >= len(f1):
        continue
    
    chunk1 = f1[off1:off1+32]
    chunk2 = f2[sector:sector+32]
    
    nonff1 = sum(1 for b in chunk1 if b != 0xFF)
    nonff2 = sum(1 for b in chunk2 if b != 0xFF)
    
    print(f'0x{sector:06X}:')
    print(f'  Ours:  {" ".join(f"{b:02X}" for b in chunk1)}')
    print(f'  Oleg:  {" ".join(f"{b:02X}" for b in chunk2)}')
    
    # Check if this is NV data
    if chunk1[:2] == bytes([0x5A, 0x5A]):
        print(f'  -> Ours: NV record type=0x{chunk1[2]:02X} ver=0x{chunk1[3]:02X}')
    if chunk2[:2] == bytes([0x5A, 0x5A]):
        print(f'  -> Oleg: NV record type=0x{chunk2[2]:02X} ver=0x{chunk2[3]:02X}')
    print()

# Check specific patterns
print('=== Pattern matching ===')
# Look for specific data patterns in our dump
for sector in range(0x66000, 0x7A000, 0x1000):
    off1 = sector - OUR_DUMP_START
    if off1 < 0 or off1 >= len(f1):
        continue
    
    chunk1 = f1[off1:off1+0x1000]
    nonff1 = sum(1 for b in chunk1 if b != 0xFF)
    
    if nonff1 > 0:
        # Find first non-FF byte
        for i, b in enumerate(chunk1):
            if b != 0xFF:
                print(f'0x{sector:06X}: first non-FF at +0x{i:03X}: 0x{b:02X}')
                break
