#!/bin/python3

def decode_downlink(hex_str):
    hex_str = hex_str.strip().lower()

    try:
        cmd = int(hex_str[0:2], 16)
        field = int(hex_str[2:4], 16)
    except (ValueError, IndexError):
        print("Invalid hex format or too short.\n")
        return

    if cmd == 0x30:
        vital_map = {
            0x01: "Blood Pressure",
            0x02: "Temperature",
            0x03: "Heart Rate",
        }
        label = vital_map.get(field, f"Unknown (0x{field:02X})")
        print(f"Request Reading → {label}\n")
        return

    elif cmd == 0x20:
        if field == 0x01 and len(hex_str) == 8:
            # Heart Rate thresholds (1 byte min, 1 byte max)
            hr_min = int(hex_str[4:6], 16)
            hr_max = int(hex_str[6:8], 16)
            print(f"Set Heart Rate Thresholds → Min = {hr_min}, Max = {hr_max}\n")
            return

        elif field == 0x02 and len(hex_str) == 12:
            # Temperature thresholds (2 bytes min, 2 bytes max, scaled by 10)
            tmin = (int(hex_str[4:6], 16) << 8) | int(hex_str[6:8], 16)
            tmax = (int(hex_str[8:10], 16) << 8) | int(hex_str[10:12], 16)
            print(f"Set Temperature Thresholds → Min = {tmin / 10:.1f}°C, Max = {tmax / 10:.1f}°C\n")
            return

        elif field == 0x03 and len(hex_str) == 12:
            # Blood Pressure thresholds (1 byte each: sys min, sys max, dia min, dia max)
            sys_min = int(hex_str[4:6], 16)
            sys_max = int(hex_str[6:8], 16)
            dia_min = int(hex_str[8:10], 16)
            dia_max = int(hex_str[10:12], 16)
            print(f"Set Blood Pressure Thresholds → SYS Min = {sys_min}, SYS Max = {sys_max}, DIA Min = {dia_min}, DIA Max = {dia_max}\n")
            return

        else:
            print("Invalid or unsupported threshold format.\n")
            return

    else:
        print(f"Unknown command ID: 0x{cmd:02X}\n")

# 🧪 Prompt loop
while True:
    user_input = input("Paste hex payload (e.g. 20017364, 2002016D016E, 200373783C46), or 'q' to quit: ").strip()
    if user_input.lower() == 'q':
        break
    decode_downlink(user_input)
