#!/usr/bin/env python3
"""
Simple dt extractor - converts log to C++ array
"""

import re
import sys


def extract_dt_values(log_file):
    """Extract all DT values from log"""
    dt_values = []

    with open(log_file, "r") as f:
        for line in f:
            # Look for lines like: DT,0.018456
            match = re.search(r"DT,([\d.]+)", line)
            if match:
                dt_values.append(float(match.group(1)))

    return dt_values


def generate_cpp_header(dt_values, output_file):
    """Generate C++ header with dt array"""

    with open(output_file, "w") as f:
        f.write("// Auto-generated test data\n")
        f.write(f"// Total impulses: {len(dt_values)}\n\n")
        f.write("#pragma once\n\n")

        f.write("const double test_dt_values[] = {\n")

        # Write 10 values per line for readability
        for i in range(0, len(dt_values), 10):
            chunk = dt_values[i : i + 10]
            line = "    " + ", ".join(f"{dt:.6f}" for dt in chunk) + ","
            f.write(line + "\n")

        f.write("};\n\n")
        f.write(f"const size_t test_dt_count = {len(dt_values)};\n")


def main():
    if len(sys.argv) < 2:
        print("Usage: python extract_dt.py <logfile.log>")
        sys.exit(1)

    log_file = sys.argv[1]

    print(f"Extracting dt values from {log_file}...")
    dt_values = extract_dt_values(log_file)

    if not dt_values:
        print("ERROR: No DT values found in log!")
        print("Make sure you're using the data capture firmware.")
        sys.exit(1)

    print(f"Found {len(dt_values)} impulses")

    # Calculate some stats
    avg_dt = sum(dt_values) / len(dt_values)
    min_dt = min(dt_values)
    max_dt = max(dt_values)
    total_time = sum(dt_values)

    print(f"\nStatistics:")
    print(f"  Average dt: {avg_dt:.6f}s")
    print(f"  Min dt:     {min_dt:.6f}s")
    print(f"  Max dt:     {max_dt:.6f}s")
    print(f"  Total time: {total_time:.2f}s")

    # Generate output
    output_file = log_file.replace(".log", "_test_data.h")
    print(f"\nGenerating {output_file}...")
    generate_cpp_header(dt_values, output_file)

    print("✓ Done!")


if __name__ == "__main__":
    main()
