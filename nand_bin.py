#!/usr/bin/env python3
import argparse

def main():
    parser = argparse.ArgumentParser(prog='nand_bin',
                                     description='Combine stage1 & 2 to NAND image')
    parser.add_argument('-p', '--page_size', type=int, default=4096, help="page size")
    parser.add_argument('-s', '--oob_size', type=int, default=218, help="oob size")
    parser.add_argument('stage1_file', help="stage1.bin file")
    parser.add_argument('stage2_file', help="stage2.bin file")
    parser.add_argument('out_file', help="output file")
    args = parser.parse_args()

    page_size = args.page_size
    oob_size  = args.oob_size
    block_size = page_size + oob_size
    block_size = (block_size + 3) // 4 * 4
    written_size = 0
    with open(args.out_file, "wb") as f_out:
        for f in [args.stage1_file, args.stage2_file]:
            with open(f, "rb") as f_in:
                while True:
                    block_data = f_in.read(page_size)
                    if not block_data:
                        break
                    block_data += bytes([0xff] * (block_size - len(block_data)))
                    f_out.write(block_data)
                    written_size += len(block_data)
            # Padding to 8 KiB after stage1
            stage1_padding = (8192 // page_size) * block_size
            if written_size < stage1_padding:
                block_data = bytes([0xff] * (stage1_padding - written_size))
                f_out.write(block_data)
                written_size += len(block_data)

if __name__ == '__main__':
    main()
