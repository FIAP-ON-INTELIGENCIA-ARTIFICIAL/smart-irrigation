import os, csv, sys, time, re, json
import serial
from serial.tools import list_ports

# ---------- config ----------
DEVICE_ID = os.environ.get("ESP32_DEV", "esp32_01")

def pick_port(pref=os.environ.get("ESP32_PORT", "")):
    if pref and os.path.exists(pref):
        return pref
    cands = [p.device for p in list_ports.comports()]
    if not cands:
        return None
    pref_keys = ("SLAB_USBtoUART", "usbserial", "usbmodem", "wchusbserial", "ttyUSB", "ttyACM")
    for dev in cands:
        if any(k in dev for k in pref_keys):
            return dev
    return cands[0]

PORT = pick_port()
BAUD = int(os.environ.get("ESP32_BAUD", "115200"))
if not PORT:
    print("[ERRO] Nenhuma porta serial encontrada. Conecte o ESP32 e rode novamente.")
    sys.exit(1)

# ---------- paths ----------
BASE = os.path.dirname(__file__)
DATA_DIR = os.path.abspath(os.path.join(BASE, "..", "data"))
os.makedirs(DATA_DIR, exist_ok=True)

def make_paths():
    date = time.strftime("%Y-%m-%d")
    csv_path = os.path.join(DATA_DIR, f"leituras_{date}.csv")
    raw_path = os.path.join(DATA_DIR, f"raw_{date}.log")
    jsonl_path = os.path.join(DATA_DIR, f"leituras_{date}.ndjson")
    return csv_path, raw_path, jsonl_path

csv_path, raw_path, jsonl_path = make_paths()
new_csv = not os.path.exists(csv_path)

print(f"[i] Porta: {PORT} @ {BAUD}")
print(f"[i] CSV  : {csv_path}")
print(f"[i] RAW  : {raw_path}")
print(f"[i] JSON : {jsonl_path}")

# ---------- open files ----------
csv_file = open(csv_path, "a", newline="", encoding="utf-8")
writer = csv.writer(csv_file)
CSV_COLS = [
    "ts","dev","soil_mv","soil_pct","ldr_mv","ph",
    "npk_n","npk_p","npk_k",
    "pump_on","pump_allowed",
    "rain_hold","rain_hold_until",
    "wx_code","wx_wet",
    "mv","pct"
]
if new_csv:
    writer.writerow(CSV_COLS)

raw_file = open(raw_path, "a", encoding="utf-8")
jsonl_file = open(jsonl_path, "a", encoding="utf-8")

# ---------- serial ----------
ser = serial.Serial(PORT, BAUD, timeout=1)
time.sleep(1.5)

last_csv_ts = 0

# ---------- helpers ----------
def rotate_if_needed():
    global csv_path, raw_path, jsonl_path, csv_file, raw_file, jsonl_file, writer
    expected_csv, expected_raw, expected_json = make_paths()
    if expected_csv != csv_path:
        for f in (csv_file, raw_file, jsonl_file):
            try: f.close()
            except: pass
        csv_path, raw_path, jsonl_path = expected_csv, expected_raw, expected_json
        new_csv_local = not os.path.exists(csv_path)
        csv_file = open(csv_path, "a", newline="", encoding="utf-8")
        writer = csv.writer(csv_file)
        if new_csv_local:
            writer.writerow(CSV_COLS)
        raw_file = open(raw_path, "a", encoding="utf-8")
        jsonl_file = open(jsonl_path, "a", encoding="utf-8")
        print(f"\n[i] Novo dia → arquivos:")
        print(f"[i] CSV  : {csv_path}")
        print(f"[i] RAW  : {raw_path}")
        print(f"[i] JSON : {jsonl_path}")

def to_bool(x):
    if isinstance(x, bool): return x
    s = str(x).strip().lower()
    return s in ("1","true","t","yes","y","on")

def to_int(x, default=0):
    try: return int(x)
    except: return default

def to_float(x, default=0.0):
    try: return float(x)
    except: return default

def emit(record: dict):
    row = [record.get(k, "") for k in CSV_COLS]
    writer.writerow(row); csv_file.flush()
    jsonl_file.write(json.dumps(record, ensure_ascii=False) + "\n"); jsonl_file.flush()

# ---------- parsers ----------
# CSVX curto: CSVX,ts,soil_mv,soil_pct,ldr_mv,ph,npk_n,npk_p,npk_k,pump_on
def parse_csvx_short(parts):
    if len(parts) != 10:
        return None
    _, ts, soil_mv, soil_pct, ldr_mv, ph, npk_n, npk_p, npk_k, pump_on = parts
    return {
        "ts": ts,
        "dev": DEVICE_ID,
        "soil_mv": to_int(soil_mv),
        "soil_pct": to_int(soil_pct),
        "ldr_mv": to_int(ldr_mv),
        "ph": to_float(ph),
        "npk_n": to_int(npk_n),
        "npk_p": to_int(npk_p),
        "npk_k": to_int(npk_k),
        "pump_on": to_bool(pump_on),
        # campos não enviados → defaults
        "pump_allowed": True,
        "rain_hold": False,
        "rain_hold_until": "",
        "wx_code": "",
        "wx_wet": False,
        # duplicatas convenientes
        "mv": to_int(soil_mv),
        "pct": to_int(soil_pct),
    }

# CSVX longo legado: CSVX,ts,dev,soil_mv,soil_pct,ldr_mv,ph,npk_n,npk_p,npk_k,pump_on,pump_allowed,rain_hold,rain_hold_until,wx_code,wx_wet,mv,pct
def parse_csvx_long(parts):
    if len(parts) < 18:
        return None
    (_, ts, dev, soil_mv, soil_pct, ldr_mv, ph,
     npk_n, npk_p, npk_k,
     pump_on, pump_allowed,
     rain_hold, rain_hold_until,
     wx_code, wx_wet, mv, pct) = parts[:18]
    return {
        "ts": ts,
        "dev": dev or DEVICE_ID,
        "soil_mv": to_int(soil_mv),
        "soil_pct": to_int(soil_pct),
        "ldr_mv": to_int(ldr_mv),
        "ph": to_float(ph),
        "npk_n": to_int(npk_n),
        "npk_p": to_int(npk_p),
        "npk_k": to_int(npk_k),
        "pump_on": to_bool(pump_on),
        "pump_allowed": to_bool(pump_allowed),
        "rain_hold": to_bool(rain_hold),
        "rain_hold_until": rain_hold_until,
        "wx_code": to_int(wx_code, default=""),
        "wx_wet": to_bool(wx_wet),
        "mv": to_int(mv),
        "pct": to_int(pct),
    }

def parse_csvx(parts):
    # detecta automaticamente a variante
    if len(parts) == 10:
        return parse_csvx_short(parts)
    elif len(parts) >= 18:
        return parse_csvx_long(parts)
    else:
        return None

# CSV antigo: CSV,ts,mv,pct
def parse_legacy_csv(parts):
    if len(parts) != 4:
        return None
    _, ts, mv, pct = parts
    return {
        "ts": ts,
        "dev": DEVICE_ID,
        "mv": to_int(mv),
        "pct": to_int(pct),
        "soil_mv": to_int(mv),
        "soil_pct": to_int(pct),
    }

# JSON: J,{ ... }
def parse_json_line(payload):
    try:
        obj = json.loads(payload)
        for k in ("pump_on","pump_allowed","rain_hold","wx_wet"):
            if k in obj:
                obj[k] = to_bool(obj[k])
        if "dev" not in obj: obj["dev"] = DEVICE_ID
        return obj
    except Exception:
        return None

# fallback humano
def parse_human(line):
    try:
        ts = re.search(r"(\d{4}-\d{2}-\d{2}T[^ ]+Z)", line)
        mv = re.search(r"SOLO mV=\s*(\d+)", line)
        pct = re.search(r"Umid=\s*(\d+)%", line)
        ldr = re.search(r"LDR mV=\s*(\d+)", line)
        ph  = re.search(r"pH=([0-9.]+)", line)
        pump= re.search(r"PUMP=(\d+)", line)
        if not ts: return None
        rec = {"ts": ts.group(1), "dev": DEVICE_ID}
        if mv:   rec["soil_mv"] = to_int(mv.group(1)); rec["mv"] = rec["soil_mv"]
        if pct:  rec["soil_pct"] = to_int(pct.group(1)); rec["pct"] = rec["soil_pct"]
        if ldr:  rec["ldr_mv"] = to_int(ldr.group(1))
        if ph:   rec["ph"] = to_float(ph.group(1))
        if pump: rec["pump_on"] = (pump.group(1) == "1")
        return rec
    except Exception:
        return None

# ---------- loop ----------
try:
    while True:
        rotate_if_needed()

        line_bytes = ser.readline()
        if not line_bytes:
            continue
        line = line_bytes.decode(errors="ignore").strip()

        raw_file.write(line + "\n")
        raw_file.flush()

        appended = False

        if line.startswith("CSVX,"):
            parts = line.split(",")
            rec = parse_csvx(parts)
            if rec:
                emit(rec); appended = True

        elif line.startswith("J,"):
            rec = parse_json_line(line[2:].strip())
            if rec:
                emit(rec); appended = True

        elif line.startswith("CSV,"):
            parts = line.split(",")
            rec = parse_legacy_csv(parts)
            if rec:
                emit(rec); appended = True

        if not appended:
            rec = parse_human(line)
            if rec:
                emit(rec); appended = True

        now = time.time()
        if appended:
            sys.stdout.write("."); sys.stdout.flush()
            last_csv_ts = now
        elif now - last_csv_ts > 10:
            sys.stderr.write("\n[WARN] 10s sem dados CSV; verifique prefixo/baud/porta.\n")
            last_csv_ts = now

except KeyboardInterrupt:
    print("\n[stop]")
finally:
    for f in (csv_file, raw_file, jsonl_file):
        try: f.close()
        except: pass
    try: ser.close()
    except: pass
