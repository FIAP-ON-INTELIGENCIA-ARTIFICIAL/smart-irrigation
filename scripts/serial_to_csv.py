import os, csv, sys, time, re
import serial
from serial.tools import list_ports

def pick_port(pref=os.environ.get("ESP32_PORT", "")):
    # usa a porta definida se existir
    if pref and os.path.exists(pref):
        return pref
    # tenta achar automaticamente
    cands = [p.device for p in list_ports.comports()]
    mac_like = [p for p in cands if p.startswith("/dev/tty.") or p.startswith("/dev/cu.")]
    for key in ("SLAB_USBtoUART", "usbserial", "usbmodem"):
        for dev in mac_like:
            if key in dev:
                return dev
    return mac_like[0] if mac_like else (cands[0] if cands else None)

PORT = pick_port()
BAUD = int(os.environ.get("ESP32_BAUD", "115200"))
if not PORT:
    print("[ERRO] Nenhuma porta serial encontrada. Conecte o ESP32 e rode novamente.")
    sys.exit(1)

# pastas/arquivos
BASE = os.path.dirname(__file__)
DATA_DIR = os.path.abspath(os.path.join(BASE, "..", "data"))
os.makedirs(DATA_DIR, exist_ok=True)

csv_path = os.path.join(DATA_DIR, time.strftime("leituras_%Y-%m-%d.csv"))
raw_path = os.path.join(DATA_DIR, time.strftime("raw_%Y-%m-%d.log"))

new_csv = not os.path.exists(csv_path)

print(f"[i] Porta: {PORT} @ {BAUD}")
print(f"[i] CSV  : {csv_path}")
print(f"[i] RAW  : {raw_path}")

# abre arquivos
csv_file = open(csv_path, "a", newline="")
writer = csv.writer(csv_file)
if new_csv:
    writer.writerow(["timestamp", "mv", "pct"])

raw_file = open(raw_path, "a", encoding="utf-8")

# abre serial
ser = serial.Serial(PORT, BAUD, timeout=1)
time.sleep(1.5)  # aguarda reset inicial da placa

last_csv_ts = 0

try:
    while True:
        line_bytes = ser.readline()
        if not line_bytes:
            continue
        line = line_bytes.decode(errors="ignore").strip()

        # log bruto (tudo)
        raw_file.write(line + "\n")
        raw_file.flush()

        appended = False

        # 1) formato CSV preferido: CSV,ts,mv,pct
        if line.startswith("CSV,"):
            parts = line.split(",")
            if len(parts) == 4:
                _, ts, mv, pct = parts
                writer.writerow([ts, mv, pct])
                csv_file.flush()
                appended = True

        # 2) fallback: parse da linha humana "mV=####  Umidade=##%  ts=..."
        if not appended:
            m = re.search(r"mV=(\d+)\s+Umidade=(\d+)%\s+ts=([0-9T:\-]+Z)", line)
            if m:
                mv, pct, ts = m.groups()
                writer.writerow([ts, mv, pct])
                csv_file.flush()
                appended = True

        # feedback visual
        now = time.time()
        if appended:
            sys.stdout.write("."); sys.stdout.flush()
            last_csv_ts = now
        elif now - last_csv_ts > 10:
            # a cada 10s sem CSV, avisa
            sys.stderr.write("\n[WARN] 10s sem dados CSV; verifique prefixo/baud/porta.\n")
            last_csv_ts = now

except KeyboardInterrupt:
    print("\n[stop]")
finally:
    try: ser.close()
    except: pass
    try: csv_file.close()
    except: pass
    try: raw_file.close()
    except: pass
