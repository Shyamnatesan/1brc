
"""
Generate expected 1BRC output for a measurements file.

Usage:
    python3 generate_expected.py measurements.txt > expected.txt
"""

import sys


def main():
    if len(sys.argv) != 2:
        print("Usage: generate_expected.py <measurements.txt>", file=sys.stderr)
        sys.exit(1)

    path = sys.argv[1]
    # station -> [min, sum, count, max]
    stats = {}

    with open(path, "r") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            station, temp_str = line.split(";", 1)
            temp = float(temp_str)

            if station not in stats:
                stats[station] = [temp, temp, 1, temp]  # min, sum, count, max
            else:
                s = stats[station]
                s[0] = min(s[0], temp)
                s[1] += temp
                s[2] += 1
                s[3] = max(s[3], temp)

    # Build the required output format, stations sorted
    parts = []
    for station in sorted(stats.keys()):
        min_t, sum_t, count, max_t = stats[station]
        mean = sum_t / count
        # Format to 1 decimal place
        parts.append(f"{station}={min_t:.1f}/{mean:.1f}/{max_t:.1f}")

    print("{" + ", ".join(parts) + "}")

if __name__ == "__main__":
    main()
