import json

try:
    with open('tldr_database.json', 'r', encoding='utf-8') as f:
        db = json.load(f)
    print(f"Loaded {len(db)} entries")

    queries = [
        "get-childitem to find all hidden files in the c drive recursively silently",
        "robocopy to mirror directory c:\\src to d:\\dest",
        "ipconfig to flush the dns resolver cache"
    ]
    
    for q in queries:
        found = False
        lowerQuery = q.lower()
        for key in db.keys():
            tokenBoundaryStart = key + " "
            tokenBoundaryEnd = " " + key
            tokenBoundary = " " + key + " "
            
            if (lowerQuery == key or
                tokenBoundary in lowerQuery or
                lowerQuery.startswith(tokenBoundaryStart) or
                lowerQuery.endswith(tokenBoundaryEnd)):
                print(f"[{key}] matched query: {q}")
                found = True
                break
        if not found:
            print(f"[FAIL] No match for: {q}")
            
except Exception as e:
    print("Error:", e)
