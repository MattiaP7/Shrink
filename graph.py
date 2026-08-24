import json
import pandas as pd
import matplotlib.pyplot as plt

with open('benchmark_results.json', 'r') as f:
    data = json.load(f)

summary = data['summary']
df = pd.DataFrame(data['results'])

# Calculate Throughput and execution times
total_time_sec = summary['total_time_ms'] / 1000.0
throughput = summary['total_files'] / total_time_sec

print(f"Threads used: {summary['threads']}")
print(f"Total execution time: {total_time_sec:.2f} s")
print(f"Throughput: {throughput:.2f} img/sec")

# Visual configuration
plt.style.use('seaborn-v0_8-darkgrid' if 'seaborn-v0_8-darkgrid' in plt.style.available else 'default')
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

# Chart 1: Processing time per image with average line
ax1.bar(range(len(df)), df['time_ms'], color='#3498db', alpha=0.8)
ax1.axhline(df['time_ms'].mean(), color='red', linestyle='--', label=f"Average per thread: {df['time_ms'].mean():.2f} ms")
ax1.set_title(f"Processing Time per File (Wall-clock Total: {total_time_sec:.2f}s @ {summary['threads']} Threads)")
ax1.set_xlabel('Image Index')
ax1.set_ylabel('Time (ms)')
ax1.legend()

# Chart 2: Size reduction distribution (%)
ax2.hist(df['savings_percent'], bins=15, color='#2ecc71', edgecolor='black', alpha=0.7)
ax2.axvline(df['savings_percent'].mean(), color='red', linestyle='--', label=f"Average savings: {df['savings_percent'].mean():.1f}%")
ax2.set_title('Size Reduction Distribution (%)')
ax2.set_xlabel('Savings (%)')
ax2.set_ylabel('Number of Images')
ax2.legend()

plt.tight_layout()
plt.savefig('benchmark_charts.png', dpi=300)