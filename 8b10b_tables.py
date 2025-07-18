with open('8b10b_codes.txt', 'rb') as f:
    lines = f.readlines()
    for idx, line in enumerate(lines):
        if idx < 4:
            continue
        line = line.strip()
        if not line:
            continue
        code, data, neg, pos = line.split()
        data = int(data,2)
        neg = int(neg, 2)
        pos = int(pos, 2)
        #{0x274, 0x00}, {0x18B, 0x00},  // D0.0: 1001110100 RD-, 0110001011 RD+
        print("{{{2:#05x}, {1:#04x}}}, {{{3:#05x}, {1:#04x}}}, // {0}: {2:#012b} RD-, {3:#012b} RD+".format(str(code), data, neg, pos))