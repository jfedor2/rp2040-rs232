#!/usr/bin/env python3

import argparse


def main():
    parser = argparse.ArgumentParser(
        description="Combine multiple binaries into a single image."
    )
    parser.add_argument(
        "-o",
        "--output",
        required=True,
        help="Output file name for the combined binary.",
    )
    parser.add_argument(
        "-i",
        "--image",
        action="append",
        required=True,
        help="Image to add in format 'filename@0xAddress'",
    )
    args = parser.parse_args()

    image_specs = [
        (filename, int(address_str, 0) - 0x10000000)
        for filename, address_str in [spec.split("@") for spec in args.image]
    ]

    final_binary = bytearray()

    for filename, offset in image_specs:
        with open(filename, "rb") as f:
            app_data = f.read()
        print(f"{filename} ({len(app_data)} bytes) at offset {hex(offset)}")
        if len(final_binary) < offset + len(app_data):
            final_binary += b"\x00" * ((offset + len(app_data)) - len(final_binary))
        final_binary[offset : offset + len(app_data)] = app_data

    with open(args.output, "wb") as f:
        f.write(final_binary)


if __name__ == "__main__":
    main()
