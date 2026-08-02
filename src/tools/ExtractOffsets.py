#!/usr/bin/env python3
"""
Extracts field offsets for named classes directly from dump.cs.

Usage:
    python ExtractOffsets.py <path_to_dump.cs> <class_name_1> <class_name_2> ...
    
Example:
    python ExtractOffsets.py ../../Dump/dump.cs HeroInfoData vh vo ze
"""
import sys
import re

def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)

    dump_path = sys.argv[1]
    target_classes = [c.lower() for c in sys.argv[2:]]

    try:
        with open(dump_path, "r", encoding="utf-8") as f:
            lines = f.readlines()
    except Exception as e:
        print(f"Error reading {dump_path}: {e}")
        return

    current_class = None
    results = {}

    # Matches: public class HeroInfoData : vo // TypeDefIndex: 1560
    class_def_re = re.compile(r"^\s*(?:public|private|protected|internal)?\s*(?:sealed|abstract)?\s*class\s+([A-Za-z0-9_]+)")
    
    # Matches: public int HeroKey; // 0x30
    field_re = re.compile(r"^\s*(?:public|private|protected|internal|static|readonly)*\s+([A-Za-z0-9_<>\[\], ]+)\s+([A-Za-z0-9_]+)\s*;\s*//\s*(0x[0-9A-Fa-f]+)")

    for line in lines:
        c_match = class_def_re.search(line)
        if c_match:
            cls_name = c_match.group(1)
            # Exact match (case insensitive)
            if cls_name.lower() in target_classes:
                current_class = cls_name
                results[current_class] = []
            else:
                current_class = None
            continue
        
        if current_class:
            f_match = field_re.search(line)
            if f_match:
                ftype, fname, offset = f_match.groups()
                results[current_class].append((fname, ftype.strip(), offset))

    if not results:
        print(f"No classes matched {sys.argv[2:]}.")
        return

    for cls, fields in results.items():
        print(f"\n== {cls} ==")
        for fname, ftype, offset in fields:
            print(f"  {fname:<24} type={ftype:<20} offset={offset}")

if __name__ == "__main__":
    main()