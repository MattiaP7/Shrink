import csv
import os
import urllib.request

tsv_file = "photos.tsv000"
output_dir = "./unsplash_test_dataset"
os.makedirs(output_dir, exist_ok=True)

print(f"Estrazione degli url da {tsv_file}")
urls = []
with open(tsv_file, 'r', encoding='utf-8') as f:
    reader = csv.reader(f, delimiter='\t')
    header = next(reader)

    try:
        url_idx = header.index("photo_image_url")
    except ValueError:
        url_idx = 2

    for row in reader:
        if len(row) > url_idx and row[url_idx].startswith("http"):
            urls.append(row[url_idx])
            # scarico le prime 100
            if len(urls) >= 100:
                break

print(f"Scaricamento di {len(urls)} immagini in corso...")
for idx, url in enumerate(urls, start=1):
    # Appende parametri per la qualità originale
    full_res_url = f"{url}?fm=jpg&q=85"
    file_path = os.path.join(output_dir, f"img_{idx:03d}.jpg")
    try:
        urllib.request.urlretrieve(full_res_url, file_path)
        print(f"[{idx}/{len(urls)}] Scaricata: {file_path}")
    except Exception as e:
        print(f"Errore sul download {url}: {e}")

print("Dataset pronto in ./unsplash_test_dataset")