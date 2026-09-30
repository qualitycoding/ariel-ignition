"""Spike for C-020: CRC-16/CCITT-FALSE reference values."""
def crc(b):
    c = 0xFFFF
    for x in b:
        c ^= x << 8
        for _ in range(8):
            c = ((c << 1) ^ 0x1021) & 0xFFFF if c & 0x8000 else (c << 1) & 0xFFFF
    return c
for s in (b"123456789", b"", b"A"):
    print(repr(s), hex(crc(s)))
