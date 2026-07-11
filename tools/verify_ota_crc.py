import struct, sys
sys.path.insert(0, 'tools')
from zlib import crc32

# Read current flash
with open('ts0201_wing_orig512.bin', 'rb') as f:
    flash = f.read()

# Our image at 0x4D000
OTA_ADDR = 0x4D000
image = flash[OTA_ADDR:OTA_ADDR+0x20000]  # up to 128KB

# Read TLNK header
tlnk_off = 8  # TLNK is at offset +0x08 from image start
if image[tlnk_off:tlnk_off+4] != b'TLNK':
    print("ERROR: No TLNK at expected offset!")
    sys.exit(1)

print("=== OTA IMAGE HEADER ===")
print(f"Bytes 0x00-0x1F: {' '.join(f'{b:02X}' for b in image[:32])}")

# Parse header
reset_vec = image[0] | (image[1] << 8)
file_ver = struct.unpack_from('<I', image, 0x02)[0]
app_build = image[0x06] | (image[0x07] << 8)
tlnk = struct.unpack_from('>I', image, 0x08)[0]  # big-endian "TLNK"
ramcode_size = struct.unpack_from('<I', image, 0x0C)[0]
irq_vec = image[0x10] | (image[0x11] << 8)
manuf = image[0x12] | (image[0x13] << 8)
imgtype = image[0x14] | (image[0x15] << 8)
fw_size = struct.unpack_from('<I', image, 0x18)[0]

print(f"reset_vec: 0x{reset_vec:04X}")
print(f"file_ver: 0x{file_ver:08X}")
print(f"app_build: 0x{app_build:04X}")
print(f"TLNK: 0x{tlnk:08X}")
print(f"ramcode_size: 0x{ramcode_size:08X}")
print(f"irq_vec: 0x{irq_vec:04X}")
print(f"MANUFACTURER_CODE: 0x{manuf:04X}")
print(f"IMAGE_TYPE: 0x{imgtype:04X}")
print(f"fw_size: {fw_size} (0x{fw_size:X})")

# Verify CRC
# CRC is last 4 bytes of fw_size, computed over first (fw_size-4) bytes
if fw_size > 4 and fw_size <= len(image):
    data_for_crc = image[:fw_size-4]
    crc_stored = struct.unpack_from('<I', image, fw_size-4)[0]
    crc_computed = crc32(data_for_crc) & 0xFFFFFFFF

    print(f"\n=== CRC CHECK ===")
    print(f"Data for CRC: first {fw_size-4} bytes")
    print(f"CRC stored in flash: 0x{crc_stored:08X}")
    print(f"CRC computed: 0x{crc_computed:08X}")
    print(f"CRC match: {'YES' if crc_stored == crc_computed else 'NO'}")
else:
    print(f"\nfw_size={fw_size} is invalid (image max=0x{len(image):X})")

# Also verify what our .bin file looks like
print(f"\n=== COMPARING WITH .bin FILE ===")
with open('out/ts0201_wing_zed.bin', 'rb') as f:
    our_bin = f.read()

print(f".bin file size: {len(our_bin)}")
print(f"OTA image at 0x4D000 size (non-0xFF): ", end="")
ota_non_ff = sum(1 for b in image if b != 0xFF)
print(f"{ota_non_ff}")

# Check if .bin matches what's in flash
bin_match = True
for i in range(len(our_bin)):
    if i < len(image) and image[i] != our_bin[i]:
        print(f"Mismatch at offset 0x{i:05X}: flash=0x{image[i]:02X} bin=0x{our_bin[i]:02X}")
        bin_match = False
        if i > 20:
            print("  ... (showing first 20 mismatches)")
            break
if bin_match:
    print(".bin matches flash image: YES")
else:
    print(".bin matches flash image: NO")

# Also check: does the bootloader copy the OTA image to 0x10000?
# If so, the current 0x10000 should be the OLD firmware (not ours)
print(f"\n=== CHECK IF 0x10000 WAS UPDATED ===")
fw_10k = flash[0x10000:0x10020]
print(f"FW at 0x10000: {' '.join(f'{b:02X}' for b in fw_10k)}")
manuf_10k = flash[0x10012] | (flash[0x10013] << 8)
imgtype_10k = flash[0x10014] | (flash[0x10015] << 8)
fwsize_10k = struct.unpack_from('<I', flash, 0x10018)[0]
print(f"MANUF at 0x10000: 0x{manuf_10k:04X}")
print(f"IMG_TYPE at 0x10000: 0x{imgtype_10k:04X}")
print(f"fw_size at 0x10000: {fwsize_10k} (0x{fwsize_10k:X})")

# Compare with original Tuya dump
with open('bin/tuya_original/ts0201_wing_orig512.bin', 'rb') as f:
    orig = f.read()

fw_orig = orig[0x10000:0x10020]
print(f"Original FW at 0x10000: {' '.join(f'{b:02X}' for b in fw_orig)}")
if flash[0x10000:0x10000+0x1000] == orig[0x10000:0x10000+0x1000]:
    print("0x10000 first 4KB: IDENTICAL to original")
else:
    diffs = sum(1 for i in range(0x1000) if flash[0x10000+i] != orig[0x10000+i])
    print(f"0x10000 first 4KB: {diffs} bytes differ from original")
