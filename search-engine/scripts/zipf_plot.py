#!/usr/bin/env python3
import sys
import csv
import math
import matplotlib.pyplot as plt

def main(path):
    ranks = []
    freqs = []
    with open(path, newline='', encoding='utf-8') as f:
        rd = csv.DictReader(f)
        for row in rd:
            ranks.append(int(row["rank"]))
            freqs.append(int(row["tf"]))

    if not ranks:
        print("Empty CSV")
        return

    C = freqs[0]
    zipf = [C / r for r in ranks]

    # log-log plot
    plt.figure()
    plt.xscale('log')
    plt.yscale('log')
    plt.plot(ranks, freqs, marker='.', linestyle='none', label='Corpus')
    plt.plot(ranks, zipf, linestyle='-', label='Zipf (C/r)')
    plt.xlabel('Rank (log)')
    plt.ylabel('Frequency (log)')
    plt.title('Zipf Law')
    plt.legend()
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: zipf_plot.py data/zipf.csv")
        sys.exit(1)
    main(sys.argv[1])
