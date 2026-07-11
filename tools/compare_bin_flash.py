with open('ts0201_wing_orig512.bin', 'rb') as f:
    flash = f.read()

with open('bin/ts0201_wing_zed_V1.0.01.bin', 'rb') as f:
    our_bin = f.read()

OTA = 0x4D000
flash_img = flash[OTA:OTA+len(our_bin)]

print(f".bin size: {len(our_bin)} (0x{len(our_bin):X})")
print(f"Flash image size: {len(flash_img)}")

# Byte-by-byte comparison
diffs = []
for i in range(len(our_bin)):
    if flash_img[i] != our_bin[i]:
        diffs.append(i)

print(f"Differing bytes: {len(diffs)} / {len(our_bin)}")

if diffs:
    # Show first diff
    i = diffs[0]
    print(f"\nFirst diff at offset 0x{i:05X} (abs 0x{OTA+i:05X}):")
    print(f"  flash: {' '.join(f'{b:02X}' for b in flash_img[i:i+32])}")
    print(f"  .bin:  {' '.join(f'{b:02X}' for b in our_bin[i:i+32])}")

    # Show all diff regions
    regions = []
    start = diffs[0]
    prev = diffs[0]
    for d in diffs[1:]:
        if d == prev + 1:
            prev = d
        else:
            regions.append((start, prev))
            start = d
            prev = d
    regions.append((start, prev))

    print(f"\nDiff regions:")
    for s, e in regions[:20]:
        fl = ' '.join(f'{b:02X}' for b in flash_img[s:s+16])
        bi = ' '.join(f'{b:02X}' for b in our_bin[s:s+16])
        print(f"  0x{s:05X}-0x{e:05X} ({e-s+1}B)")
        print(f"    flash: {fl}")
        print(f"    .bin:  {bi}")
else:
    print("\n*** PERFECT MATCH ***")

# Check CRC area specifically
print(f"\n=== CRC AREA ===")
crc_off = len(our_bin) - 4
print(f"CRC offset: 0x{crc_off:05X} (abs 0x{OTA+crc_off:05X})")
print(f"  flash: {' '.join(f'{b:02X}' for b in flash_img[crc_off:crc_off+4])}")
print(f"  .bin:  {' '.join(f'{b:02X}' for b in our_bin[crc_off:crc_off+4])}")

# Check first 0x19000 bytes (clean area)
clean = sum(1 for i in range(0x19000) if i < len(our_bin) and flash_img[i] == our_bin[i])
print(f"\nFirst 0x19000 bytes: {clean}/{0x19000} match")

# Check last bytes
print(f"\nLast 16 bytes of .bin: {' '.join(f'{b:02X}' for b in our_bin[-16:])}")
print(f"Last 16 bytes of flash img: {' '.join(f'{b:02X}' for b in flash_img[-16:])}")
