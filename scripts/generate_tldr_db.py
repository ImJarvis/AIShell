import urllib.request
import zipfile
import os
import json
import shutil

# GitHub repository ZIP URL for tldr-pages
TLDR_ZIP_URL = "https://github.com/tldr-pages/tldr/archive/refs/heads/main.zip"
DOWNLOAD_DEST = "tldr_main.zip"
EXTRACT_DIR = "tldr_extracted"
OUTPUT_JSON = "../tldr_database.json"

def download_and_extract():
    print(f"Downloading tldr pages from {TLDR_ZIP_URL}...")
    urllib.request.urlretrieve(TLDR_ZIP_URL, DOWNLOAD_DEST)
    
    print("Extracting ZIP file...")
    with zipfile.ZipFile(DOWNLOAD_DEST, 'r') as zip_ref:
        zip_ref.extractall(EXTRACT_DIR)

def parse_tldr_pages():
    # The structure after extraction is usually tldr-main/pages/windows
    base_pages_dir = os.path.join(EXTRACT_DIR, "tldr-main", "pages")
    
    # We want both windows specific and common commands (like curl, git, etc.)
    target_dirs = ["windows", "common"]
    
    database = {}
    
    for category in target_dirs:
        dir_path = os.path.join(base_pages_dir, category)
        if not os.path.exists(dir_path):
            print(f"Warning: Directory {dir_path} not found.")
            continue
            
        print(f"Parsing {category} commands...")
        for filename in os.listdir(dir_path):
            if filename.endswith(".md"):
                command_name = filename[:-3] # Remove .md
                filepath = os.path.join(dir_path, filename)
                
                with open(filepath, 'r', encoding='utf-8') as f:
                    content = f.read()
                    # We can store the raw markdown directly, LLMs understand it perfectly
                    database[command_name] = content
                    
    return database

def cleanup():
    print("Cleaning up temporary files...")
    if os.path.exists(DOWNLOAD_DEST):
        os.remove(DOWNLOAD_DEST)
    if os.path.exists(EXTRACT_DIR):
        shutil.rmtree(EXTRACT_DIR)

def main():
    try:
        download_and_extract()
        db = parse_tldr_pages()
        
        # Save to JSON
        output_path = os.path.abspath(OUTPUT_JSON)
        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(db, f, indent=2)
            
        print(f"Success! Generated {output_path} with {len(db)} command documentations.")
    finally:
        cleanup()

if __name__ == "__main__":
    # Ensure this runs relative to the script directory
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    main()
