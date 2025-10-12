import os
from dotenv import load_dotenv

load_dotenv()

content = f"""#pragma once

#define WIFI_SSID "{os.getenv('WIFI_SSID')}"
#define WIFI_PASS "{os.getenv('WIFI_PASS')}"

#define API_KEY       "{os.getenv('FIREBASE_API_KEY')}"
#define PROJECT_ID    "{os.getenv('FIREBASE_PROJECT_ID')}"
#define USER_EMAIL    "{os.getenv('FIREBASE_USER_EMAIL')}"
#define USER_PASSWORD "{os.getenv('FIREBASE_USER_PASS')}"
#define FIRESTORE_DB  "{os.getenv('FIREBASE_FIRESTORE_DB')}"

#define OW_API_KEY    "{os.getenv('OW_API_KEY')}"
#define OW_LAT        "{os.getenv('OW_LAT')}"
#define OW_LON        "{os.getenv('OW_LON')}"
#define OW_LANG       "{os.getenv('OW_LANG')}"
#define OW_UNITS      "{os.getenv('OW_UNITS')}"

#define PUMP_ENABLE_DEFAULT "{os.getenv('PUMP_ENABLE_DEFAULT', 'false')}"
"""

os.makedirs("firmware/include", exist_ok=True)
with open("firmware/include/env.h", "w") as f:
    f.write(content)

print("[OK] Gerado: firmware/include/env.h")
