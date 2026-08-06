#!/usr/bin/env python3
import sys
import re
import os

def main():
    if len(sys.argv) != 3:
        print("Usage: python3 UpdateOffsets.py <path_to_dump.cs> <path_to_Il2CppOffsets.h>")
        sys.exit(1)

    dump_path = sys.argv[1]
    header_path = sys.argv[2]

    if not os.path.exists(dump_path):
        print(f"Error: {dump_path} not found.")
        sys.exit(1)
    if not os.path.exists(header_path):
        print(f"Error: {header_path} not found.")
        sys.exit(1)

    print(f"[*] Reading header file: {header_path}")
    with open(header_path, 'r', encoding='utf-8') as f:
        header_content = f.read()

    # Find all tags like: @[ClassName.FieldName]
    # We want to extract ClassName and FieldName.
    # regex matches: // @[ClassName.FieldName]
    tag_re = re.compile(r"//\s*@\[([A-Za-z0-9_]+)\.([A-Za-z0-9_]+)\]")
    
    needed_offsets = {} # (ClassName, FieldName) -> None (to be filled with hex)
    for match in tag_re.finditer(header_content):
        cls_name = match.group(1)
        fld_name = match.group(2)
        needed_offsets[(cls_name, fld_name)] = None

    if not needed_offsets:
        print("[-] No @[Class.Field] tags found in header file.")
        sys.exit(0)

    print(f"[*] Found {len(needed_offsets)} tags. Scanning dump.cs...")

    with open(dump_path, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    current_class = None
    class_def_re = re.compile(r"^\s*(?:public|private|protected|internal)?\s*(?:sealed|abstract)?\s*class\s+([A-Za-z0-9_]+)")
    
    # Matches: public int HeroKey; // 0x30
    # Also handles JsonProperty and other attributes above fields.
    field_re = re.compile(r"^\s*(?:public|private|protected|internal|static|readonly)*\s+(?:[A-Za-z0-9_<>\[\], ]+)\s+([A-Za-z0-9_]+)\s*;\s*//\s*(0x[0-9A-Fa-f]+)")

    for line in lines:
        c_match = class_def_re.search(line)
        if c_match:
            current_class = c_match.group(1)
            continue
        
        if current_class:
            f_match = field_re.search(line)
            if f_match:
                fname = f_match.group(1)
                offset = f_match.group(2)
                
                if (current_class, fname) in needed_offsets:
                    needed_offsets[(current_class, fname)] = offset

    # Verify if we found all
    missing = False
    for (cls, fld), off in needed_offsets.items():
        if off is None:
            print(f"  [!] Missing offset for {cls}.{fld}")
            missing = True
        else:
            print(f"  [+] {cls}.{fld} -> {off}")

    if missing:
        print("[-] Not all offsets were found. Make sure class and field names match exactly.")
    
    # Now patch the header file
    # We replace: = 0x123;  // @[Class.Field]
    # with:       = NEW_HEX;  // @[Class.Field]
    print("[*] Updating header file...")
    
    def replacer(match):
        hex_val = match.group(1)
        tag_content = match.group(2)
        
        # parse the class and field out of the tag
        m = re.match(r"([A-Za-z0-9_]+)\.([A-Za-z0-9_]+)", tag_content)
        if m:
            cls = m.group(1)
            fld = m.group(2)
            new_hex = needed_offsets.get((cls, fld))
            if new_hex:
                # return the new line with the updated hex, keeping formatting
                return match.group(0).replace(hex_val, new_hex)
        return match.group(0)

    # Regex to capture the hex value before the // @[tag]
    # Matches: = 0xABC; // @[Class.Field]
    full_line_re = re.compile(r"=\s*(0x[0-9A-Fa-f]+)\s*;\s*//\s*@\[([A-Za-z0-9_]+\.[A-Za-z0-9_]+)\]")
    
    new_header_content = full_line_re.sub(replacer, header_content)

    with open(header_path, 'w', encoding='utf-8') as f:
        f.write(new_header_content)

    print("[*] Successfully updated Il2CppOffsets.h!")

if __name__ == "__main__":
    main()
