#!/bin/python3

def decode_downlink(hex_str):
	hex_str = hex_str.strip().lower()

	if len(hex_str) not in (4, 6, 8):
		print("Invalid payload length. Must be 4, 6, or 8 hex characters.\n")
		return

	try:
		cmd = int(hex_str[0:2], 16)
		field = int(hex_str[2:4], 16)
	except ValueError:
		print("Invalid hex format.\n")
		return

	# Command 0x30 → Request Reading
	if cmd == 0x30:
		vital_map = {
			0x01: "Blood Pressure",
			0x02: "Temperature",
			0x03: "Heart Rate",
		}

		label = vital_map.get(field)
		if not label:
			print(f"Unknown vital sign ID in reading request: 0x{field:02X}\n")
			return

		print(f"Request Reading → {label}\n")
		return

	# Command 0x20 → Set Threshold
	elif cmd == 0x20:
		field_map = {
			0x01: ("Heart Rate Min", False),
			0x02: ("Heart Rate Max", False),
			0x03: ("Temp Min", True),
			0x04: ("Temp Max", True),
			0x05: ("BP Sys Min", False),
			0x06: ("BP Sys Max", False),
			0x07: ("BP Dia Min", False),
			0x08: ("BP Dia Max", False),
		}

		if field not in field_map:
			print(f"Unknown field ID: 0x{field:02X}\n")
			return

		label, is_scaled = field_map[field]

		try:
			if is_scaled:
				if len(hex_str) != 8:
					print(f"Expected 4-byte payload for temperature threshold, got {len(hex_str)//2} bytes.\n")
					return
				msb = int(hex_str[4:6], 16)
				lsb = int(hex_str[6:8], 16)
				raw_value = (msb << 8) | lsb
				value = raw_value / 10.0
			else:
				if len(hex_str) != 6:
					print(f"Expected 3-byte payload for {label}, got {len(hex_str)//2} bytes.\n")
					return
				value = int(hex_str[4:6], 16)
		except ValueError:
			print("Failed to parse value.\n")
			return

		print(f"Set Threshold → {label} = {value}\n")
		return

	else:
		print(f"Unknown command ID: 0x{cmd:02X}\n")

# 🧪 Prompt loop
while True:
	user_input = input("Paste hex payload (e.g. 200267, 2003016D, 3002), or 'q' to quit: ").strip()
	if user_input.lower() == 'q':
		break
	decode_downlink(user_input)
