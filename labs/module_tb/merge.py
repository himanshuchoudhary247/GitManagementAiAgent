import json
from pathlib import Path

def pretty_format_json(input_file: str, output_file: str = None) -> None:
    input_path = Path(input_file)
    output_path = Path(output_file) if output_file else input_path

    # Read existing JSON
    with input_path.open("r", encoding="utf-8") as f:
        data = json.load(f)

    # Write back with proper formatting
    with output_path.open("w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False, sort_keys=True)

    print(f"Formatted JSON written to: {output_path}")

# Example usage
if __name__ == "__main__":
    # Replace with your actual input file path
    pretty_format_json("/Users/sudhanshu/code/systemc/module_tb/my_dump.json")

# #!/usr/bin/env python3
# """
# merge_folder_to_json.py
# Read every (text) file under a folder and dump the contents into one JSON file.

# Usage
# -----
# python merge_folder_to_json.py /path/to/folder   # writes merged_files.json
# python merge_folder_to_json.py /path/to/folder -o my_dump.json
# """

# import argparse
# import json
# import os
# from pathlib import Path

# def read_folder(folder: Path, encoding: str = "utf-8") -> dict:
#     """
#     Recursively read all files in *folder* and return a dict
#     {relative_path: file_contents}.
#     Non‑UTF‑8 or binary files are skipped.
#     """
#     data = {}
#     for path in folder.rglob("*"):
#         print(f"Processing: {path}")
#         if path.is_file():
#             rel_key = path.relative_to(folder).as_posix()  # JSON‑friendly key
#             try:
#                 data[rel_key] = path.read_text(encoding=encoding)
#             except UnicodeDecodeError:
#                 # Skip binary / non‑text files
#                 print(f"Skipping non‑text file: {rel_key}")
#     return data

# def main() -> None:
#     parser = argparse.ArgumentParser(
#         description="Dump every text file in a folder tree to a single JSON file.")
#     parser.add_argument("folder", type=Path,
#                         help="Root folder whose files you want to merge")
#     parser.add_argument("-o", "--output", default="merged_files.json",
#                         help="Name (or path) of the JSON file to create")
#     parser.add_argument("--encoding", default="utf-8",
#                         help="Encoding to assume when reading files")
#     args = parser.parse_args()

#     folder = args.folder.expanduser().resolve()
#     if not folder.is_dir():
#         raise SystemExit(f"Error: '{folder}' is not a directory")

#     merged = read_folder(folder, encoding=args.encoding)

#     out_path = Path(args.output).expanduser().resolve()
#     with out_path.open("w", encoding="utf-8") as fp:
#         json.dump(merged, fp, indent=2, ensure_ascii=False)

#     print(f"Wrote {len(merged)} files to {out_path}")

# if __name__ == "__main__":
#     print(1)
#     main()
