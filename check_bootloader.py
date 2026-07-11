import struct

f2_path = 'doc_local/ts0201_wing_512_OTA_from_Oleg.bin'
f2 = open(f2_path,'rb').read()

print('=== Bootloader area (0x000000-0x010000) ===')
print()

# Check for encryption patterns
# Common encryption: AES, DES, XOR patterns
# Bootloader usually has strings like "encrypt", "decrypt", "key", "AES"

# Look for ASCII strings
print('=== ASCII strings in bootloader ===')
for i in range(0, 0x10000):
    # Find printable ASCII sequences (3+ chars)
    s = ''
    j = i
    while j < len(f2) and j < i + 100:
        b = f2[j]
        if 0x20 <= b < 0x7F:
            s += chr(b)
        else:
            break
        j += 1
    if len(s) >= 4:
        print(f'0x{i:06X}: {s}')
    i = j

print()
print('=== Bootloader header analysis ===')
# Header at 0x000000
header = f2[:64]
print(f'Header: {" ".join(f"{b:02X}" for b in header)}')

# Check for common patterns
print()
print('=== Potential encryption indicators ===')

# Look for repeated patterns (could be keys)
for i in range(0, 0x10000, 4):
    chunk = f2[i:i+4]
    if chunk == chunk[::-1] and chunk[0] != 0xFF:  # Palindrome
        print(f'0x{i:06X}: Palindrome: {" ".join(f"{b:02X}" for b in chunk)}')

# Look for high entropy (encrypted) areas
print()
print('=== Entropy analysis (4KB blocks) ===')
for block in range(0, 0x10000, 0x1000):
    chunk = f2[block:block+0x1000]
    # Count unique bytes
    unique = len(set(chunk))
    nonff = sum(1 for b in chunk if b != 0xFF)
    print(f'0x{block:06X}: {unique:3d} unique bytes, {nonff:4d}/4096 non-FF')
