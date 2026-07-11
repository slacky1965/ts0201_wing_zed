import struct
from zlib import crc32

path = 'bin/ts0201_wing_zed_V1.0.01.bin'
with open(path, 'rb') as f:
    data = f.read()

print(f"File size: {len(data)} (0x{len(data):X})")
print(f"First 24 bytes: {' '.join(f'{b:02X}' for b in data[:24])}")

manuf = data[0x12] | (data[0x13] << 8)
imgtype = data[0x14] | (data[0x15] << 8)
fw_size = struct.unpack_from('<I', data, 0x18)[0]
app_build = data[0x06] | (data[0x07] << 8)

print(f"app_build (+0x06): 0x{app_build:04X}")
print(f"MANUFACTURER_CODE: 0x{manuf:04X}")
print(f"IMAGE_TYPE: 0x{imgtype:04X}")
print(f"fw_size (+0x18): {fw_size} (0x{fw_size:X})")
print(f"Magic at +0x06: {'0x025D OK' if app_build == 0x025D else 'NOT 0x025D'}")

print(f"\nFile size vs fw_size:")
print(f"  file={len(data)}, fw_size={fw_size}, diff={len(data)-fw_size}")

# Try interpretation 1: fw_size includes CRC (fw_size == file size)
if fw_size == len(data):
    print("\n[fw_size == file size] CRC is last 4 bytes of file")
    data_for_crc = data[:fw_size-4]
    crc_stored = struct.unpack_from('<I', data, fw_size-4)[0]
    crc_computed = crc32(data_for_crc) & 0xFFFFFFFF
    print(f"  CRC stored:  0x{crc_stored:08X}")
    print(f"  CRC computed: 0x{crc_computed:08X}")
    print(f"  CRC match: {'YES' if crc_stored == crc_computed else 'NO'}")

# Try interpretation 2: fw_size excludes CRC (file = fw_size + 4)
if fw_size + 4 == len(data):
    print("\n[fw_size excludes CRC] CRC is at offset fw_size")
    data_for_crc = data[:fw_size]
    crc_stored = struct.unpack_from('<I', data, fw_size)[0]
    crc_computed = crc32(data_for_crc) & 0xFFFFFFFF
    print(f"  CRC stored:  0x{crc_stored:08X}")
    print(f"  CRC computed: 0x{crc_computed:08X}")
    print(f"  CRC match: {'YES' if crc_stored == crc_computed else 'NO'}")

# Try interpretation 3: tl_check_fw.py style - CRC of entire file XOR 0xFFFFFFFF
print("\n[tl_check_fw.py style] crc32(file) ^ 0xFFFFFFFF:")
crc_xor = crc32(data) ^ 0xFFFFFFFF
print(f"  crc32(file) ^ 0xFFFFFFFF = 0x{crc_xor:08X}")

# Try: compute CRC without magic/XOR tricks
print(f"\nLast 8 bytes of file: {' '.join(f'{b:02X}' for b in data[-8:])}")

# Also check the other .bin files
for name in ['out/ts0201_wing_zed.bin', 'bin/ts0201_wing_zed_last_version.bin']:
    try:
        with open(name, 'rb') as f:
            d = f.read()
        fs = struct.unpack_from('<I', d, 0x18)[0]
        print(f"\n{name}:")
        print(f"  size={len(d)}, fw_size={fs}")
        if fs == len(d):
            crc_s = struct.unpack_from('<I', d, fs-4)[0]
            crc_c = crc32(d[:fs-4]) & 0xFFFFFFFF
            print(f"  CRC stored=0x{crc_s:08X} computed=0x{crc_c:08X} match={crc_s==crc_c}")
        elif fs+4 == len(d):
            crc_s = struct.unpack_from('<I', d, fs)[0]
            crc_c = crc32(d[:fs]) & 0xFFFFFFFF
            print(f"  CRC stored=0x{crc_s:08X} computed=0x{crc_c:08X} match={crc_s==crc_c}")
    except:
        pass
