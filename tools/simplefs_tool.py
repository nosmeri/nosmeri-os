#!/usr/bin/env python3
"""
SimpleFS Disk Management Tool for NOSMERI-OS
Supports: ls, put (write), get (read), rm (delete)
"""

import sys
import os
import struct

SECTOR_SIZE = 512
MAGIC = 0x50435231 # "PCR1"

INODE_USED = 0b01
INODE_DIR  = 0b10

SUPERBLOCK_SECTOR = 0
INODE_START_SECTOR = 1
INODE_COUNT = 64
INODES_PER_SECTOR = 4 # 512 / 128
INODE_SECTORS = 16    # 64 / 4
DATA_START_SECTOR = 17
DIRECT_BLOCKS = 30
MAX_FILENAME = 28
ENTRIES_PER_BLOCK = 16 # 512 / 32

class SimpleFS:
    def __init__(self, disk_path):
        self.disk_path = disk_path
        if not os.path.exists(disk_path):
            raise FileNotFoundError(f"Disk image not found: {disk_path}")
        self.f = open(disk_path, "r+b")
        self._read_superblock()

    def close(self):
        if self.f:
            self.f.close()
            self.f = None

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    def _read_sector(self, sector_num):
        self.f.seek(sector_num * SECTOR_SIZE)
        return self.f.read(SECTOR_SIZE)

    def _write_sector(self, sector_num, data):
        if len(data) < SECTOR_SIZE:
            data = data + b'\x00' * (SECTOR_SIZE - len(data))
        elif len(data) > SECTOR_SIZE:
            data = data[:SECTOR_SIZE]
        self.f.seek(sector_num * SECTOR_SIZE)
        self.f.write(data)
        self.f.flush()

    def _read_superblock(self):
        buf = self._read_sector(SUPERBLOCK_SECTOR)
        magic, total_sectors, inode_count, data_block_start = struct.unpack("<IIII", buf[:16])
        if magic != MAGIC:
            raise ValueError(f"Invalid SimpleFS magic: {hex(magic)} (expected {hex(MAGIC)})")
        self.total_sectors = total_sectors
        self.inode_count = inode_count
        self.data_block_start = data_block_start

    def _read_inode(self, inode_idx):
        if inode_idx < 0 or inode_idx >= self.inode_count:
            raise IndexError("Inode index out of bounds")
        sec = INODE_START_SECTOR + (inode_idx // INODES_PER_SECTOR)
        offset = (inode_idx % INODES_PER_SECTOR) * 128
        buf = self._read_sector(sec)
        raw = buf[offset:offset + 128]
        size, flags = struct.unpack("<II", raw[:8])
        blocks = list(struct.unpack(f"<{DIRECT_BLOCKS}I", raw[8:128]))
        return {
            "index": inode_idx,
            "size": size,
            "flags": flags,
            "blocks": blocks
        }

    def _write_inode(self, inode_dict):
        idx = inode_dict["index"]
        sec = INODE_START_SECTOR + (idx // INODES_PER_SECTOR)
        offset = (idx % INODES_PER_SECTOR) * 128
        buf = bytearray(self._read_sector(sec))
        raw = struct.pack(f"<II{DIRECT_BLOCKS}I", inode_dict["size"], inode_dict["flags"], *inode_dict["blocks"])
        buf[offset:offset + 128] = raw
        self._write_sector(sec, buf)

    def _get_all_used_blocks(self):
        used = set()
        for idx in range(self.inode_count):
            inode = self._read_inode(idx)
            if inode["flags"] & INODE_USED:
                for b in inode["blocks"]:
                    if b != 0:
                        used.add(b)
        return used

    def _find_free_inode(self):
        for idx in range(1, self.inode_count): # 0 is root
            inode = self._read_inode(idx)
            if (inode["flags"] & INODE_USED) == 0:
                return idx
        return None

    def _find_free_blocks(self, count):
        used = self._get_all_used_blocks()
        free_blocks = []
        candidate = self.data_block_start
        while len(free_blocks) < count and candidate < self.total_sectors:
            if candidate not in used:
                free_blocks.append(candidate)
            candidate += 1
        if len(free_blocks) < count:
            raise IOError("Disk full: no free data blocks available")
        return free_blocks

    def list_dir(self, dir_inode_idx=0):
        dir_inode = self._read_inode(dir_inode_idx)
        if not (dir_inode["flags"] & INODE_DIR):
            raise ValueError(f"Inode {dir_inode_idx} is not a directory")
        dir_block = dir_inode["blocks"][0]
        buf = self._read_sector(dir_block)
        entries = []
        for i in range(ENTRIES_PER_BLOCK):
            raw = buf[i * 32:(i + 1) * 32]
            inode_idx = struct.unpack("<I", raw[:4])[0]
            name_bytes = raw[4:32].split(b'\x00')[0]
            if name_bytes:
                name = name_bytes.decode('ascii', errors='replace')
                inode = self._read_inode(inode_idx)
                is_dir = bool(inode["flags"] & INODE_DIR)
                entries.append({
                    "entry_idx": i,
                    "inode_idx": inode_idx,
                    "name": name,
                    "size": inode["size"],
                    "is_dir": is_dir,
                    "blocks": [b for b in inode["blocks"] if b != 0]
                })
        return entries

    def put_file(self, host_file_path, target_name=None):
        if not os.path.exists(host_file_path):
            raise FileNotFoundError(f"Host file not found: {host_file_path}")
        with open(host_file_path, "rb") as hf:
            file_data = hf.read()

        if target_name is None:
            target_name = os.path.basename(host_file_path)

        if len(target_name) >= MAX_FILENAME:
            raise ValueError(f"Target name too long (max {MAX_FILENAME - 1} chars)")

        size = len(file_data)
        needed_blocks = (size + SECTOR_SIZE - 1) // SECTOR_SIZE
        if needed_blocks > DIRECT_BLOCKS:
            raise ValueError(f"File too large (max {DIRECT_BLOCKS * SECTOR_SIZE} bytes)")

        # Check if file already exists in root dir
        entries = self.list_dir(0)
        target_entry = next((e for e in entries if e["name"] == target_name), None)

        if target_entry:
            # Overwrite existing file
            inode_idx = target_entry["inode_idx"]
            inode = self._read_inode(inode_idx)
            current_blocks = [b for b in inode["blocks"] if b != 0]
            if len(current_blocks) < needed_blocks:
                add_count = needed_blocks - len(current_blocks)
                new_blocks = self._find_free_blocks(add_count)
                assigned_blocks = current_blocks + new_blocks
            else:
                assigned_blocks = current_blocks[:needed_blocks]
        else:
            # Allocate new Inode
            inode_idx = self._find_free_inode()
            if inode_idx is None:
                raise IOError("No free Inodes available (max 64)")
            assigned_blocks = self._find_free_blocks(needed_blocks)

        # Write data blocks
        for i, block_sec in enumerate(assigned_blocks):
            chunk = file_data[i * SECTOR_SIZE:(i + 1) * SECTOR_SIZE]
            self._write_sector(block_sec, chunk)

        # Update Inode
        blocks_arr = [0] * DIRECT_BLOCKS
        for i, b in enumerate(assigned_blocks):
            blocks_arr[i] = b

        inode_dict = {
            "index": inode_idx,
            "size": size,
            "flags": INODE_USED,
            "blocks": blocks_arr
        }
        self._write_inode(inode_dict)

        # Update root directory entry if new file
        if not target_entry:
            root_inode = self._read_inode(0)
            dir_block = root_inode["blocks"][0]
            buf = bytearray(self._read_sector(dir_block))
            slot_found = False
            for i in range(ENTRIES_PER_BLOCK):
                raw = buf[i * 32:(i + 1) * 32]
                name_bytes = raw[4:32].split(b'\x00')[0]
                if not name_bytes:
                    name_encoded = target_name.encode('ascii')
                    name_padded = name_encoded + b'\x00' * (MAX_FILENAME - len(name_encoded))
                    buf[i * 32:(i + 1) * 32] = struct.pack("<I", inode_idx) + name_padded
                    self._write_sector(dir_block, buf)
                    slot_found = True
                    break
            if not slot_found:
                raise IOError("Root directory is full (max 16 entries)")

        print(f"[OK] Injected '{target_name}' ({size} bytes, {needed_blocks} blocks) at Inode {inode_idx}")

    def rm_file(self, target_name):
        entries = self.list_dir(0)
        target_entry = next((e for e in entries if e["name"] == target_name), None)
        if not target_entry:
            raise FileNotFoundError(f"File '{target_name}' not found in root dir")

        inode_idx = target_entry["inode_idx"]
        inode = self._read_inode(inode_idx)
        if inode["flags"] & INODE_DIR:
            raise ValueError(f"'{target_name}' is a directory (use mkdir/rmdir)")

        # Clear Inode
        empty_inode = {
            "index": inode_idx,
            "size": 0,
            "flags": 0,
            "blocks": [0] * DIRECT_BLOCKS
        }
        self._write_inode(empty_inode)

        # Clear DirEntry
        root_inode = self._read_inode(0)
        dir_block = root_inode["blocks"][0]
        buf = bytearray(self._read_sector(dir_block))
        entry_idx = target_entry["entry_idx"]
        buf[entry_idx * 32:(entry_idx + 1) * 32] = b'\x00' * 32
        self._write_sector(dir_block, buf)
        print(f"[OK] Removed '{target_name}' (Inode {inode_idx} freed)")


def print_usage():
    print("SimpleFS Disk Management Tool")
    print("Usage:")
    print("  python3 tools/simplefs_tool.py ls [disk.img]")
    print("  python3 tools/simplefs_tool.py put <host_file> [target_name] [disk.img]")
    print("  python3 tools/simplefs_tool.py rm <target_name> [disk.img]")

def main():
    if len(sys.argv) < 2:
        print_usage()
        sys.exit(1)

    cmd = sys.argv[1]

    if cmd == "ls":
        disk_path = sys.argv[2] if len(sys.argv) > 2 else "disk.img"
        with SimpleFS(disk_path) as fs:
            print(f"--- Files in {disk_path} (SimpleFS) ---")
            entries = fs.list_dir(0)
            print(f"{'Type':<6} {'Inode':<7} {'Size (Bytes)':<14} {'Blocks':<10} {'Filename'}")
            print("-" * 55)
            for e in entries:
                t = "<DIR>" if e["is_dir"] else "<FILE>"
                blocks_str = ",".join(map(str, e["blocks"]))
                print(f"{t:<6} {e['inode_idx']:<7} {e['size']:<14} {blocks_str:<10} {e['name']}")

    elif cmd == "put":
        if len(sys.argv) < 3:
            print("Usage: python3 tools/simplefs_tool.py put <host_file> [target_name] [disk.img]")
            sys.exit(1)
        host_file = sys.argv[2]
        target_name = None
        disk_path = "disk.img"
        if len(sys.argv) == 4:
            # could be target_name or disk.img
            if sys.argv[3].endswith(".img"):
                disk_path = sys.argv[3]
            else:
                target_name = sys.argv[3]
        elif len(sys.argv) >= 5:
            target_name = sys.argv[3]
            disk_path = sys.argv[4]

        with SimpleFS(disk_path) as fs:
            fs.put_file(host_file, target_name)

    elif cmd == "rm":
        if len(sys.argv) < 3:
            print("Usage: python3 tools/simplefs_tool.py rm <target_name> [disk.img]")
            sys.exit(1)
        target_name = sys.argv[2]
        disk_path = sys.argv[3] if len(sys.argv) > 3 else "disk.img"
        with SimpleFS(disk_path) as fs:
            fs.rm_file(target_name)

    else:
        print(f"Unknown command: {cmd}")
        print_usage()
        sys.exit(1)

if __name__ == "__main__":
    main()
