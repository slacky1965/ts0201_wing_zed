with open('ts0201_wing_orig512.bin', 'rb') as f:
    flash = f.read()

with open('bin/tuya_original/ts0201_wing_orig512.bin', 'rb') as f:
    orig = f.read()

ota = flash[0x4D000:0x4D000+0x27000]
non_ff = sum(1 for b in ota if b != 0xFF)
print(f"OTA area 0x{0x4D000:05X}-0x{0x4D000+0x27000:05X}: {len(ota)} bytes")
print(f"Non-0xFF bytes: {non_ff}")
print(f"All 0xFF: {non_ff == 0}")
print(f"First 32 bytes: {' '.join(f'{b:02X}' for b in ota[:32])}")

off = 0x19000
if off < len(ota):
    print(f"At offset 0x{off:05X} (abs 0x{0x4D000+off:05X}): {' '.join(f'{b:02X}' for b in ota[off:off+16])}")

end = len(ota) - 16
print(f"Last 16 bytes: {' '.join(f'{b:02X}' for b in ota[end:])}")

bl = flash[0:0x10000]
bl_orig = orig[0:0x10000]
print(f"\nBootloader: {'IDENTICAL' if bl == bl_orig else 'MODIFIED'} to original")

fw = flash[0x10000:0x10000+0x40000]
fw_orig = orig[0x10000:0x10000+0x40000]
fw_diffs = sum(1 for a,b in zip(fw, fw_orig) if a != b)
print(f"App FW 0x10000: {fw_diffs} bytes differ from original")
