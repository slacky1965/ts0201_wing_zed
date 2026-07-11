import struct
from zlib import crc32

with open('ts0201_wing_orig512.bin', 'rb') as f:
    flash = f.read()

with open('bin/ts0201_wing_zed_V1.0.01.bin', 'rb') as f:
    our_bin = f.read()

with open('bin/tuya_original/ts0201_wing_orig512.bin', 'rb') as f:
    orig = f.read()

print(f"Flash dump: {len(flash)} bytes")
print(f"Our .bin:   {len(our_bin)} bytes")
print(f"Original:   {len(orig)} bytes")

# 1. Bootloader
print("\n=== BOOTLOADER 0x00000 ===")
bl_cur = flash[0:0x10000]
bl_orig = orig[0:0x10000]
if bl_cur == bl_orig:
    print("IDENTICAL to original")
else:
    diffs = [(i, bl_cur[i], bl_orig[i]) for i in range(0x10000) if bl_cur[i] != bl_orig[i]]
    print(f"{len(diffs)} bytes differ:")
    for addr, cur, was in diffs:
        print(f"  0x{addr:05X}: {cur:02X} (was {was:02X})")

# 2. App FW at 0x10000
print("\n=== APP FW 0x10000 ===")
fw_cur = flash[0x10000:0x10000+len(our_bin)]
fw_orig = orig[0x10000:0x10000+len(our_bin)]
print(f"First 24 bytes: {' '.join(f'{b:02X}' for b in flash[0x10000:0x10018])}")
manuf = flash[0x10012] | (flash[0x10013] << 8)
imgtype = flash[0x10014] | (flash[0x10015] << 8)
fwsize = struct.unpack_from('<I', flash, 0x10018)[0]
print(f"MANUF=0x{manuf:04X} IMG=0x{imgtype:04X} fw_size={fwsize}")
is_orig = fw_cur == fw_orig
is_ours = fw_cur == our_bin
print(f"Same as original Tuya: {is_orig}")
print(f"Same as our .bin: {is_ours}")

# 3. OTA at 0x4D000
print("\n=== OTA 0x4D000 ===")
ota_start = 0x4D000
fw_size_ota = struct.unpack_from('<I', flash, ota_start + 0x18)[0]
print(f"FW size in header: {fw_size_ota} (0x{fw_size_ota:X})")
print(f"First 24 bytes: {' '.join(f'{b:02X}' for b in flash[ota_start:ota_start+24])}")

# Check TLNK
tlnk = flash[ota_start+8:ota_start+12]
print(f"TLNK: {'OK' if tlnk == b'\\x4B\\x4E\\x4C\\x54' else 'MISSING'}")

# Check MANUFACTURER_CODE/IMAGE_TYPE
manuf_ota = flash[ota_start+0x12] | (flash[ota_start+0x13] << 8)
imgtype_ota = flash[ota_start+0x14] | (flash[ota_start+0x15] << 8)
print(f"MANUF=0x{manuf_ota:04X} IMG=0x{imgtype_ota:04X}")

# Compare with our .bin
ota_data = flash[ota_start:ota_start+fw_size_ota]
match = True
first_diff = -1
for i in range(min(len(our_bin), len(ota_data))):
    if ota_data[i] != our_bin[i]:
        first_diff = i
        match = False
        break
if match and len(our_bin) == len(ota_data):
    print("OTA image == our .bin: YES")
elif match:
    print(f"First {min(len(our_bin), len(ota_data))} bytes match, but sizes differ")
else:
    print(f"First diff at offset 0x{first_diff:05X} (abs 0x{ota_start+first_diff:05X})")
    print(f"  flash: {' '.join(f'{b:02X}' for b in ota_data[first_diff:first_diff+16])}")
    print(f"  .bin:  {' '.join(f'{b:02X}' for b in our_bin[first_diff:first_diff+16])}")
    # Count total diffs
    total = sum(1 for i in range(min(len(our_bin), len(ota_data))) if ota_data[i] != our_bin[i])
    print(f"  Total differing bytes: {total}")

# CRC check
if fw_size_ota == len(our_bin) and fw_size_ota > 4:
    crc_stored = struct.unpack_from('<I', ota_data, fw_size_ota-4)[0]
    crc_computed = crc32(ota_data[:fw_size_ota-4]) ^ 0xFFFFFFFF
    print(f"CRC stored:  0x{crc_stored:08X}")
    print(f"CRC computed: 0x{crc_computed:08X}")
    print(f"CRC match: {'YES' if crc_stored == crc_computed else 'NO'}")

# Last 4 bytes of OTA image
print(f"Last 4 bytes of OTA image: {' '.join(f'{b:02X}' for b in ota_data[-4:])}")

# 4. After OTA image
end_of_ota = ota_start + fw_size_ota
print(f"\nBytes after OTA image (0x{end_of_ota:05X}): {' '.join(f'{b:02X}' for b in flash[end_of_ota:end_of_ota+16])}")

# 5. NV area 0x72000+
print(f"\n=== NV AREA (first non-0xFF) ===")
for i in range(0x72000, min(len(flash), 0x80000)):
    if flash[i] != 0xFF:
        print(f"First non-0xFF at 0x{i:05X}")
        print(f"  Data: {' '.join(f'{b:02X}' for b in flash[i:i+32])}")
        break
else:
    print("All 0xFF")
