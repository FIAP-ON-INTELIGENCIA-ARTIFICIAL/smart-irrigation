import pandas 
import subprocess

# 1. LER CSV E CALCULAR
df = pandas.read_csv('data/sensor_data.csv')
media_5_leituras = df['humidity_percentage'].tail(5).mean()

print(f"Python calculou: {media_5_leituras:.1f}%")

# 2. SALVAR NO ARQUIVO
with open('data/valor_ccpp.txt', 'w') as f:
    f.write(str(media_5_leituras))

print("✓ Arquivo salvo!\n")

# 3. CHAMAR O PROGRAMA C
print("Rodando programa C...\n")
subprocess.run('umidade_solo.exe', shell=True)