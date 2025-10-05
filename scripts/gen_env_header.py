import os
from dotenv import load_dotenv

load_dotenv()

content = f"""#pragma once

#define WIFI_SSID "{os.getenv('WIFI_SSID')}"
#define WIFI_PASS "{os.getenv('WIFI_PASS')}"

#define API_KEY "{os.getenv('FIREBASE_API_KEY')}"
#define PROJECT_ID "{os.getenv('FIREBASE_PROJECT_ID')}"
#define USER_EMAIL "{os.getenv('FIREBASE_USER_EMAIL')}"
#define USER_PASSWORD "{os.getenv('FIREBASE_USER_PASS')}"
"""

os.makedirs("firmware/include", exist_ok=True)
with open("firmware/include/env.h", "w") as f:
    f.write(content)

print("[OK] Gerado: firmware/include/env.h")
