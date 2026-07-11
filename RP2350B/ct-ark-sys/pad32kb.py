#!/usr/bin/env python3
import sys
import os

TARGET_SIZE = 32 * 1024  # 32768 bytes

def pad_to_32k(path):
    size = os.path.getsize(path)
    if size > TARGET_SIZE:
        raise ValueError(f"ファイルサイズが32KBを超えています: {size} bytes")

    if size == TARGET_SIZE:
        print("すでに32KBです。処理は行いません。")
        return

    pad_len = TARGET_SIZE - size
    with open(path, "ab") as f:
        f.write(b"\x00" * pad_len)

    print(f"パディング完了: {path} ({size} -> {TARGET_SIZE} bytes)")

def main():
    if len(sys.argv) != 2:
        print(f"使い方: {sys.argv[0]} <ファイル名>")
        sys.exit(1)

    pad_to_32k(sys.argv[1])

if __name__ == "__main__":
    main()
