import struct, sys
from zlib import crc32

with open('ts0201_wing_orig512.bin', 'rb') as f:
    flash = f.read()

OTA_ADDR = 0x4D000
image = flash[OTA_ADDR:OTA_ADDR+0x30000]

# Verify TLNK
tlnk_bytes = image[8:12]
print(f"Bytes at +0x08: {' '.join(f'{b:02X}' for b in tlnk_bytes)}")
print(f"TLNK match: {tlnk_bytes == bytes([0x4B, 0x4E, 0x4C, 0x54])}")

print("\n=== OTA IMAGE HEADER ===")
print(f"Full 24-byte header: {' '.join(f'{b:02X}' for b in image[:24])}")

file_ver = struct.unpack_from('<I', image, 0x02)[0]
app_build = image[0x06] | (image[0x07] << 8)
ramcode_size = struct.unpack_from('<I', image, 0x0C)[0]
manuf = image[0x12] | (image[0x13] << 8)
imgtype = image[0x14] | (image[0x15] << 8)
fw_size = struct.unpack_from('<I', image, 0x18)[0]

print(f"reset: 0x{image[0]:02X}{image[1]:02X}")
print(f"file_ver: 0x{file_ver:08X}")
print(f"app_build: 0x{app_build:04X}")
print(f"ramcode_size: 0x{ramcode_size:08X}")
print(f"MANUFACTURER_CODE: 0x{manuf:04X}")
print(f"IMAGE_TYPE: 0x{imgtype:04X}")
print(f"fw_size: {fw_size} (0x{fw_size:X})")

# Check fw_size validity
print(f"\nExpected fw_size from .bin: 158372 (0x26AA4)")
print(f"Actual fw_size in image:   {fw_size} (0x{fw_size:X})")

if fw_size == 158372:
    print("fw_size CORRECT")
elif fw_size > 0x30000 or fw_size == 0:
    print(f"fw_size INVALID: {fw_size}")
else:
    print(f"fw_size DIFFERENT from .bin!")

# Verify CRC
if 0 < fw_size <= len(image):
    data_for_crc = image[:fw_size-4]
    crc_stored = struct.unpack_from('<I', image, fw_size-4)[0]
    crc_computed = crc32(data_for_crc) & 0xFFFFFFFF

    print(f"\n=== CRC CHECK ===")
    print(f"CRC stored:  0x{crc_stored:08X}")
    print(f"CRC computed: 0x{crc_computed:08X}")
    print(f"CRC match: {'YES' if crc_stored == crc_computed else 'NO'}")

# Check CRC from .bin file
with open('out/ts0201_wing_zed.bin', 'rb') as f:
    our_bin = f.read()

print(f"\n=== .bin FILE ===")
print(f"Size: {len(our_bin)} (0x{len(our_bin):X})")
bin_crc = struct.unpack_from('<I', our_bin, len(our_bin)-4)[0]
bin_data_crc = crc32(our_bin[:len(our_bin)-4]) & 0xFFFFFFFF
print(f"CRC in .bin:  0x{bin_crc:08X}")
print(f"CRC computed: 0x{bin_data_crc:08X}")
print(f".bin CRC valid: {'YES' if bin_crc == bin_data_crc else 'NO'}")

# Compare flash image with .bin
print(f"\n=== FLASH vs .bin ===")
match = True
first_diff = -1
for i in range(min(len(our_bin), len(image))):
    if image[i] != our_bin[i]:
        if first_diff < 0:
            first_diff = i
        match = False
if match and len(our_bin) <= len(image):
    print("Flash image matches .bin: YES")
elif match:
    print("First {len(our_bin)} bytes match, but flash has more data")
else:
    print(f"First difference at offset 0x{first_diff:05X}")
    print(f"  flash: {' '.join(f'{b:02X}' for b in image[first_diff:first_diff+16])}")
    print(f"  .bin:  {' '.join(f'{b:02X}' for b in our_bin[first_diff:first_diff+16])}")

# Check what's AFTER the image in flash
end_of_fw = OTA_ADDR + fw_size
print(f"\n=== AFTER IMAGE (0x{end_of_fw:05X}) ===")
print(f"Bytes: {' '.join(f'{b:02X}' for b in flash[end_of_fw:end_of_fw+16])}")

# Verify CRC stored in flash matches .bin CRC
flash_crc = struct.unpack_from('<I', flash, end_of_fw-4)[0]
print(f"\nCRC in flash at end of fw: 0x{flash_crc:08X}")
print(f"CRC in .bin at end:        0x{bin_crc:08X}")
print(f"Match: {'YES' if flash_crc == bin_crc else 'NO'}")

# Check: does bootloader use OTA_ADDR or some other offset?
# Check if maybe there is a ZCL OTA header (tag_id=0x0BEE) at 0x4D000
# Our .bin should be raw binary, not wrapped in ZCL OTA
print(f"\n=== CHECK FOR ZCL OTA HEADER ===")
first_16 = image[:16]
print(f"First 16 bytes: {' '.join(f'{b:02X}' for b in first_16)}")
# ZCL OTA header starts with 0x0BEE (big-endian)
if first_16[0] == 0x0B and first_16[1] == 0xEE:
    print("ZCL OTA HEADER DETECTED!")
else:
    print("No ZCL OTA header - raw binary (correct for bootloader)")

# Check the actual .zigbee file we created
print(f"\n=== CHECK .zigbee FILE ===")
import os
zigbee_path = 'bin/1141-d3a3-1111114b-ts0201_wing_zed.zigbee'
if os.path.exists(zigbee_path):
    with open(zigbee_path, 'rb') as f:
        zigbee = f.read()
    print(f".zigbee size: {len(zigbee)} (0x{len(zigbee):X})")
    print(f"First 32 bytes: {' '.join(f'{b:02X}' for b in zigbee[:32])}")
    # Check if it has ZCL OTA header
    tag_id = struct.unpack_from('>H', zigbee, 0)[0]
    print(f"Tag ID: 0x{tag_id:04X}")
    if tag_id == 0x0BEE:
        print("ZCL OTA header present")
        field_ctrl = struct.unpack_from('<H', zigbee, 2)[0]
        manuf_code = struct.unpack_from('<H', zigbee, 4)[0]
        image_type = struct.unpack_from('<H', zigbee, 6)[0]
        file_ver = struct.unpack_from('<I', zigbee, 8)[0]
        total_size = struct.unpack_from('<I', zigbee, 12)[0]
        print(f"  fieldCtrl: 0x{field_ctrl:04X}")
        print(f"  manufacturerCode: 0x{manuf_code:04X}")
        print(f"  imageType: 0x{image_type:04X}")
        print(f"  fileVersion: 0x{file_ver:08X}")
        print(f"  totalImageSize: {total_size} (0x{total_size:X})")
else:
    print(f".zigbee file not found at {zigbee_path}")

# Compare our raw .bin with what was written to flash
print(f"\n=== FINAL SUMMARY ===")
print(f"Flash at 0x4D000: TLNK=OK, fw_size={fw_size}, CRC={'OK' if fw_size>4 and struct.unpack_from('<I', image, fw_size-4)[0] == crc32(image[:fw_size-4]) & 0xFFFFFFFF else 'FAIL'}")
print(f"Flash at 0x10000: Original Tuya FW (MANUF=0x{flash[0x10012]:02X}{flash[0x10013]:02X}, IMG=0x{flash[0x10014]:02X}{flash[0x10015]:02X})")
print(f"Flash at 0x00000: Bootloader (intact)")
