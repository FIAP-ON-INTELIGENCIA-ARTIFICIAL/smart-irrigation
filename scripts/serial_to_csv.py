import os, csv, sys, time, re, json
import serial
from serial.tools import list_ports

def pick_port(pref=os.environ.get("ESP32_PORT", "")):
    if pref and os.path.exists(pref):
        return pref
    cands = [p.device for p in list_ports.comports()]
    if not cands:
        return None
    # prioriza nomes comuns em mac/linux
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

# pastas/arquivos
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

# abre arquivos
csv_file = open(csv_path, "a", newline="", encoding="utf-8")
writer = csv.writer(csv_file)
if new_csv:
    writer.writerow([
        "ts","dev","soil_mv","soil_pct","ldr_mv","ph",
        "npk_n","npk_p","npk_k",
        "pump_on","pump_allowed",
        "rain_hold","rain_hold_until",
        "wx_code","wx_wet",
        "mv","pct"
    ])

raw_file = open(raw_path, "a", encoding="utf-8")
jsonl_file = open(jsonl_path, "a", encoding="utf-8")

# abre serial
ser = serial.Serial(PORT, BAUD, timeout=1)
time.sleep(1.5)  # aguarda reset inicial da placa

last_csv_ts = 0

# helpers -----------------------------------------------------
def rotate_if_needed():
    global csv_path, raw_path, jsonl_path, csv_file, raw_file, jsonl_file, writer
    # roda se mudou a data (novo dia)
    expected_csv, expected_raw, expected_json = make_paths()
    if expected_csv != csv_path:
        # fecha atuais
        for f in (csv_file, raw_file, jsonl_file):
            try: f.close()
            except: pass
        # reabre novos
        csv_path, raw_path, jsonl_path = expected_csv, expected_raw, expected_json
        new_csv = not os.path.exists(csv_path)
        csv_file = open(csv_path, "a", newline="", encoding="utf-8")
        writer = csv.writer(csv_file)
        if new_csv:
            writer.writerow([
                "ts","dev","soil_mv","soil_pct","ldr_mv","ph",
                "npk_n","npk_p","npk_k",
                "pump_on","pump_allowed",
                "rain_hold","rain_hold_until",
                "wx_code","wx_wet",
                "mv","pct"
            ])
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

def emit(record: dict):
    """Emite no CSV e no NDJSON com as colunas/campos padronizados."""
    # ordem do CSV
    cols = [
        "ts","dev","soil_mv","soil_pct","ldr_mv","ph",
        "npk_n","npk_p","npk_k",
        "pump_on","pump_allowed",
        "rain_hold","rain_hold_until",
        "wx_code","wx_wet",
        "mv","pct"
    ]
    row = [record.get(k, "") for k in cols]
    writer.writerow(row); csv_file.flush()
    jsonl_file.write(json.dumps(record, ensure_ascii=False) + "\n"); jsonl_file.flush()

# parsing -----------------------------------------------------
# CSVX: CSVX,ts,dev,soil_mv,soil_pct,ldr_mv,ph,npk_n,npk_p,npk_k,pump_on,pump_allowed,rain_hold,rain_hold_until,wx_code,wx_wet,mv,pct
def parse_csvx(parts):
    # garante tamanho mínimo
    need = 1 + 17  # prefixo + 17 campos
    if len(parts) < need:
        return None
    _, ts, dev, soil_mv, soil_pct, ldr_mv, ph, npk_n, npk_p, npk_k, pump_on, pump_allowed, rain_hold, rain_hold_until, wx_code, wx_wet, mv, pct = parts[:18]
    rec = {
        "ts": ts,
        "dev": dev,
        "soil_mv": int(soil_mv),
        "soil_pct": int(soil_pct),
        "ldr_mv": int(ldr_mv),
        "ph": float(ph),
        "npk_n": int(npk_n),
        "npk_p": int(npk_p),
        "npk_k": int(npk_k),
        "pump_on": to_bool(pump_on),
        "pump_allowed": to_bool(pump_allowed),
        "rain_hold": to_bool(rain_hold),
        "rain_hold_until": rain_hold_until,
        "wx_code": int(wx_code),
        "wx_wet": to_bool(wx_wet),
        "mv": int(mv),
        "pct": int(pct),
    }
    return rec

# CSV antigo: CSV,ts,mv,pct  → mapeia para campos básicos (dev default "esp32_01")
def parse_legacy_csv(parts):
    if len(parts) != 4:
        return None
    _, ts, mv, pct = parts
    return {
        "ts": ts,
        "dev": "esp32_01",
        "mv": int(mv),
        "pct": int(pct),
        # não temos os demais, ficam vazios
    }

# JSON: J,{ ...obj... }  → aceita chave a chave (se vierem com nomes corretos)
def parse_json_line(payload):
    try:
        obj = json.loads(payload)
        # normaliza booleans possíveis strings
        for k in ("pump_on","pump_allowed","rain_hold","wx_wet"):
            if k in obj:
                obj[k] = to_bool(obj[k])
        return obj
    except Exception:
        return None

# fallback humano: "SOLO mV=2662 Umid= 58% | LDR mV= 769 pH=3.06 | ... | 2025-10-11T19:51:04Z"
def parse_human(line):
    try:
        ts = re.search(r"(\d{4}-\d{2}-\d{2}T[^ ]+Z)", line)
        mv = re.search(r"SOLO mV=\s*(\d+)", line)
        pct = re.search(r"Umid=\s*(\d+)%", line)
        ldr = re.search(r"LDR mV=\s*(\d+)", line)
        ph  = re.search(r"pH=([0-9.]+)", line)
        pump= re.search(r"PUMP=(\d+)", line)
        rec = {"ts":"", "dev":"esp32_01"}
        if ts:   rec["ts"] = ts.group(1)
        if mv:   rec["soil_mv"] = int(mv.group(1)); rec["mv"] = rec["soil_mv"]
        if pct:  rec["soil_pct"] = int(pct.group(1)); rec["pct"] = rec["soil_pct"]
        if ldr:  rec["ldr_mv"] = int(ldr.group(1))
        if ph:   rec["ph"] = float(ph.group(1))
        if pump: rec["pump_on"] = (pump.group(1) == "1")
        return rec if "ts" in rec and rec["ts"] else None
    except Exception:
        return None

# loop --------------------------------------------------------
try:
    while True:
        rotate_if_needed()

        line_bytes = ser.readline()
        if not line_bytes:
            continue
        line = line_bytes.decode(errors="ignore").strip()

        # log bruto (tudo)
        raw_file.write(line + "\n")
        raw_file.flush()

        appended = False

        # 1) CSVX completo
        if line.startswith("CSVX,"):
            parts = line.split(",")
            rec = parse_csvx(parts)
            if rec:
                emit(rec); appended = True

        # 2) JSON embutido
        elif line.startswith("J,"):
            rec = parse_json_line(line[2:].strip())
            if rec:
                emit(rec); appended = True

        # 3) CSV antigo
        elif line.startswith("CSV,"):
            parts = line.split(",")
            rec = parse_legacy_csv(parts)
            if rec:
                emit(rec); appended = True

        # 4) fallback humano
        if not appended:
            rec = parse_human(line)
            if rec:
                emit(rec); appended = True

        # feedback visual
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
