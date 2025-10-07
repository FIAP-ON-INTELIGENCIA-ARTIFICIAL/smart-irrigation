import pandas  # Baixar a biblioteca pandas
from datetime import datetime  # Afim de trabalharmos com datas

# Ler o arquivo CSV
df = pandas.read_csv('data/sensor_data.csv')

print("===== ANÁLISE DE UMIDADE DO SOLO =====")
print(f"\nTotal de leituras: {len(df)}")
print(f"Primeira leitura: {df['timestamp'].iloc[0]}")
print(f"Última leitura: {df['timestamp'].iloc[-1]}")

# Calcular estatísticas
ultima_leitura = df['humidity_percentage'].iloc[-1]
media_total = df['humidity_percentage'].mean()
minima = df['humidity_percentage'].min()
maxima = df['humidity_percentage'].max()

# Média das últimas 5 leituras
ultimas_5_media = df['humidity_percentage'].tail(5).mean() 

# Calcular tendência (comparando primeira e última das 5 leituras)
if len(df) >= 5:
    primeira_das_5 = df['humidity_percentage'].iloc[-5]
    ultima_das_5 = df['humidity_percentage'].iloc[-1]
    diferenca = ultima_das_5 - primeira_das_5
    
    if diferenca > 2:
        tendencia = "Umedecendo"
    elif diferenca < -2:
        tendencia = "Secando"
    else:
        tendencia = "Estável"
else:
    tendencia = "Dados insuficientes"

# Mostrar resultados
print("\nESTATÍSTICAS:")
print(f"Última leitura: {ultima_leitura:.1f}%")
print(f"Média total: {media_total:.1f}%")
print(f"Média (últimas 5 leituras): {ultimas_5_media:.1f}%")
print(f"Umidade mínima: {minima:.1f}%")
print(f"Umidade máxima: {maxima:.1f}%")
print(f"Tendência: {tendencia}")
print(f"\nVALOR RECOMENDADO PARA C/C++: {ultimas_5_media:.1f}")