import os, sys

# Read original flash dump
with open('bin/tuya_original/ts0201_wing_orig512.bin', 'rb') as f:
    orig = f.read()

# Read our firmware
with open('out/ts0201_wing_zed.bin', 'rb') as f:
    our = f.read()

# Read original bootloader dump (standalone)
bl_path = 'bin/tuya_original/ts0201_wing_orig_bootloader.bin'
if os.path.exists(bl_path):
    with open(bl_path, 'rb') as f:
        bl_standalone = f.read()
    print(f"Standalone BL file size: {len(bl_standalone)} (0x{len(bl_standalone):X})")
else:
    bl_standalone = None

print("\n=== BOOTLOADER AT 0x00000 ===")
bl = orig[0x00000:0x10000]
print(f"First 64 bytes:")
for row in range(0, 64, 16):
    hex_str = ' '.join(f'{b:02X}' for b in bl[row:row+16])
    print(f"  0x{row:04X}: {hex_str}")

# Check for TLNK in bootloader
for off in range(0, len(bl)-4):
    if bl[off] == 0x4B and bl[off+1] == 0x4E and bl[off+2] == 0x4C and bl[off+3] == 0x54:
        print(f"  TLNK found at offset 0x{off:04X}")

print("\n=== APP FIRMWARE AT 0x10000 ===")
# Scan for TLNK in first 0x50000 of firmware area
fw_area = orig[0x10000:0x50000]
for off in range(0, min(len(fw_area)-4, 0x100)):
    if fw_area[off] == 0x4B and fw_area[off+1] == 0x4E and fw_area[off+2] == 0x4C and fw_area[off+3] == 0x54:
        abs_off = 0x10000 + off
        manuf = orig[abs_off+0x12] | (orig[abs_off+0x13] << 8)
        imgtype = orig[abs_off+0x14] | (orig[abs_off+0x15] << 8)
        fwsize = orig[abs_off+0x18] | (orig[abs_off+0x19] << 8) | (orig[abs_off+0x1A] << 16) | (orig[abs_off+0x1B] << 24)
        filever = orig[abs_off+0x02] | (orig[abs_off+0x03] << 8) | (orig[abs_off+0x04] << 16) | (orig[abs_off+0x05] << 24)
        app_build = orig[abs_off+0x06] | (orig[abs_off+0x07] << 8)
        print(f"  TLNK at 0x{abs_off:05X}")
        print(f"  Full header (24B): {' '.join(f'{b:02X}' for b in orig[abs_off:abs_off+24])}")
        print(f"  FILE_VERSION: 0x{filever:08X}")
        print(f"  APP_BUILD_CODE: 0x{app_build:04X}")
        print(f"  MANUFACTURER_CODE: 0x{manuf:04X}")
        print(f"  IMAGE_TYPE: 0x{imgtype:04X}")
        print(f"  fw_size: {fwsize} (0x{fwsize:X})")
        break

print("\n=== ORIGINAL FIRMWARE AT 0x10000 (first 128 bytes) ===")
for row in range(0, 128, 16):
    hex_str = ' '.join(f'{b:02X}' for b in orig[0x10000+row:0x10000+row+16])
    print(f"  0x{0x10000+row:05X}: {hex_str}")

print("\n=== OTA AREA 0x4D000-0x69FFF ===")
ota = orig[0x4D000:0x6A000]
non_ff = sum(1 for b in ota if b != 0xFF)
print(f"Size: {len(ota)} (0x{len(ota):X})")
print(f"Non-0xFF bytes: {non_ff}")
if non_ff > 0:
    for i, b in enumerate(ota):
        if b != 0xFF:
            print(f"First non-0xFF at 0x{0x4D000+i:05X}: {b:02X}")
            end = min(i+64, len(ota))
            hex_str = ' '.join(f'{x:02X}' for x in ota[i:end])
            print(f"Data: {hex_str}")
            # Check if there is a TLNK header
            if ota[i+8] == 0x4B and ota[i+9] == 0x4E and ota[i+10] == 0x4C and ota[i+11] == 0x54:
                print("  ** TLNK header found in OTA area! **")
                abs_ota = 0x4D000 + i
                manuf = orig[abs_ota+0x12] | (orig[abs_ota+0x13] << 8)
                imgtype = orig[abs_ota+0x14] | (orig[abs_ota+0x15] << 8)
                fwsize = orig[abs_ota+0x18] | (orig[abs_ota+0x19] << 8) | (orig[abs_ota+0x1A] << 16) | (orig[abs_ota+0x1B] << 24)
                print(f"  MANUFACTURER_CODE: 0x{manuf:04X}")
                print(f"  IMAGE_TYPE: 0x{imgtype:04X}")
                print(f"  fw_size: {fwsize} (0x{fwsize:X})")
            break
else:
    print("OTA area is all 0xFF (empty)")

print("\n=== OUR FIRMWARE HEADER ===")
hdr = our[:24]
print(f"Full header: {' '.join(f'{b:02X}' for b in hdr)}")
manuf = our[0x12] | (our[0x13] << 8)
imgtype = our[0x14] | (our[0x15] << 8)
fwsize = our[0x18] | (our[0x19] << 8) | (our[0x1A] << 16) | (our[0x1B] << 24)
print(f"MANUFACTURER_CODE: 0x{manuf:04X}")
print(f"IMAGE_TYPE: 0x{imgtype:04X}")
print(f"fw_size: {fwsize} (0x{fwsize:X})")

print("\n=== BOOTLOADER CROSS-REFERENCES ===")
# Search for strings/patterns in bootloader that might indicate checks
bl_text = bytes(bl)
# Look for check patterns
import struct
# Check if bootloader references 0x4D000 or 0x10000
# In little-endian: 0x4D000 = 00 D0 04 00, 0x10000 = 00 00 01 00
for pattern, desc in [(b'\x00\xD0\x04\x00', '0x4D000'), (b'\x00\x00\x01\x00', '0x10000'),
                       (b'\x00\x39\x00\x00', '0x39000'), (b'\x00\x70\x04\x00', '0x47000'),
                       (b'\x00\x6A\x06\x00', '0x6A000')]:
    idx = bl_text.find(pattern)
    while idx >= 0:
        print(f"  Found {desc} reference at BL+0x{idx:04X}")
        idx = bl_text.find(pattern, idx+1)

# Check for 0x544C4E4B (TLNK as raw value, might be pushed to stack)
for pattern, desc in [(b'\x4B\x4C\x4E\x54', 'TLNK little-endian'), (b'\x4B\x4E\x4C\x54', 'TLNK big-endian')]:
    idx = bl_text.find(pattern)
    while idx >= 0:
        print(f"  Found {desc} at BL+0x{idx:04X}")
        idx = bl_text.find(pattern, idx+1)

# Also look in the original firmware header for additional clues
print("\n=== ORIGINAL FIRMWARE HEADER AT 0x10000 ===")
orig_fw_hdr = orig[0x10000:0x10020]
print(f"{' '.join(f'{b:02X}' for b in orig_fw_hdr)}")

# Check CRC at end of original FW
orig_fw_size = orig[0x10018] | (orig[0x10019] << 8) | (orig[0x1001A] << 16) | (orig[0x1001B] << 24)
print(f"Original FW size from header: {orig_fw_size} (0x{orig_fw_size:X})")
print(f"Original FW CRC (4B at 0x10000+fw_size-4): ", end="")
if orig_fw_size > 4:
    crc_bytes = orig[0x10000 + orig_fw_size - 4 : 0x10000 + orig_fw_size]
    print(' '.join(f'{b:02X}' for b in crc_bytes))

# Check if there's an image CRC field at +0x1C (some Telink FW have this)
print(f"\nBytes at +0x1C-0x1F of original FW header: {' '.join(f'{b:02X}' for b in orig[0x1001C:0x10020])}")
