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

#define OM_LAT        "{os.getenv('OM_LAT')}"
#define OM_LON        "{os.getenv('OM_LON')}"
#define OM_LANG       "{os.getenv('OM_LANG')}"
#define OM_UNITS      "{os.getenv('OM_UNITS')}"

#define PUMP_ENABLE_DEFAULT "{os.getenv('PUMP_ENABLE_DEFAULT', 'false')}"
"""

os.makedirs("firmware/include", exist_ok=True)
with open("firmware/include/env.h", "w") as f:
    f.write(content)

print("[OK] Gerado: firmware/include/env.h")
