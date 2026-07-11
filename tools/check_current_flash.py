import struct, os

# Read CURRENT flash dump (top-level file = latest dump from device)
with open('ts0201_wing_orig512.bin', 'rb') as f:
    flash = f.read()

# Also read original copy for comparison
with open('bin/tuya_original/ts0201_wing_orig512.bin', 'rb') as f:
    orig = f.read()

print(f"Current flash: {len(flash)} bytes")
print(f"Original flash: {len(orig)} bytes")

# Compare byte-by-byte
diffs = []
for i in range(min(len(flash), len(orig))):
    if flash[i] != orig[i]:
        diffs.append(i)

print(f"\nDifferences: {len(diffs)} bytes changed")
if diffs:
    # Group into ranges
    ranges = []
    start = diffs[0]
    end = diffs[0]
    for d in diffs[1:]:
        if d == end + 1:
            end = d
        else:
            ranges.append((start, end))
            start = d
            end = d
    ranges.append((start, end))

    print("Changed regions:")
    for s, e in ranges:
        region_start = s & 0xFFFFF0  # align to 16
        region_end = min(e + 16, len(flash))
        print(f"\n  0x{s:05X}-0x{e:05X} ({e-s+1} bytes)")
        # Show context
        for off in range(region_start, min(region_start + 64, len(flash)), 16):
            cur = ' '.join(f'{b:02X}' for b in flash[off:off+16])
            orig_b = ' '.join(f'{b:02X}' for b in orig[off:off+16])
            marker = ' <<<' if any(off <= d <= off+15 for d in range(s, e+1)) else ''
            if flash[off:off+16] != orig[off:off+16]:
                print(f"    0x{off:05X}: {cur}")
                print(f"    orig : {orig_b}{marker}")

# Specifically check bootloader region
print("\n=== BOOTLOADER REGION 0x00000-0x0FFFF ===")
bl_cur = flash[0:0x10000]
bl_orig = orig[0:0x10000]
if bl_cur == bl_orig:
    print("Identical to original")
else:
    bl_diffs = sum(1 for a, b in zip(bl_cur, bl_orig) if a != b)
    print(f"{bl_diffs} bytes differ")
    for i in range(len(bl_cur)):
        if bl_cur[i] != bl_orig[i]:
            print(f"  0x{i:05X}: {bl_cur[i]:02X} (was {bl_orig[i]:02X})")
            if sum(1 for j in range(max(0,i-4), min(len(bl_cur),i+5)) if bl_cur[j] != bl_orig[j]) > 20:
                print("  ... (many differences, skipping)")
                break

# Check OTA area
print("\n=== OTA AREA 0x4D000-0x69FFF ===")
ota_cur = flash[0x4D000:0x6A000]
ota_orig = orig[0x4D000:0x6A000]
non_ff_cur = sum(1 for b in ota_cur if b != 0xFF)
non_ff_orig = sum(1 for b in ota_orig if b != 0xFF)
print(f"Non-0xFF: current={non_ff_cur}, original={non_ff_orig}")
if ota_cur != ota_orig:
    print("OTA area differs from original!")
    for i in range(len(ota_cur)):
        if ota_cur[i] != ota_orig[i]:
            print(f"  First diff at +0x{i:05X} (abs 0x{0x4D000+i:05X}): {ota_cur[i]:02X} (was {ota_orig[i]:02X})")
            break

# Check for TLNK in OTA
for i in range(0, len(ota_cur)-4):
    if ota_cur[i] == 0x4B and ota_cur[i+1] == 0x4E and ota_cur[i+2] == 0x4C and ota_cur[i+3] == 0x54:
        abs_off = 0x4D000 + i
        print(f"TLNK found at 0x{abs_off:05X}")
        manuf = flash[abs_off+0x12] | (flash[abs_off+0x13] << 8)
        imgtype = flash[abs_off+0x14] | (flash[abs_off+0x15] << 8)
        fwsize = flash[abs_off+0x18] | (flash[abs_off+0x19] << 8) | (flash[abs_off+0x1A] << 16) | (flash[abs_off+0x1B] << 24)
        print(f"  MANUF: 0x{manuf:04X}, IMG_TYPE: 0x{imgtype:04X}, fw_size: 0x{fwsize:X}")
        break
else:
    print("No TLNK in OTA area")

# App FW header comparison
print("\n=== APP FW HEADER AT 0x10000 ===")
print("Current: ", end="")
print(' '.join(f'{b:02X}' for b in flash[0x10000:0x10020]))
print("Original:", end="")
print(' '.join(f'{b:02X}' for b in orig[0x10000:0x10020]))

# BL header
print("\n=== BL HEADER AT 0x00000 ===")
print("Current: ", end="")
print(' '.join(f'{b:02X}' for b in flash[0x00000:0x00020]))
print("Original:", end="")
print(' '.join(f'{b:02X}' for b in orig[0x00000:0x00020]))
