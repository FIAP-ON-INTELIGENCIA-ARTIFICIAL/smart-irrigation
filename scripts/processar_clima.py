'''
import pandas  # Baixar a biblioteca pandas
import datetime  # O módulo datetime ajuda a entender o formato de timestamp (ex: "2024-10-06 08:00:00") 
import os

# Ler o arquivo CSV
df = pandas.read_csv('data/sensor_data.csv')

print(" ANÁLISE DE UMIDADE DO SOLO")
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

# AQUI é onde você define o valor a ser transferido!
valor_para_ccpp = ultimas_5_media 

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
print(f"\nVALOR RECOMENDADO PARA C/C++: {valor_para_ccpp:.1f}")

try:
    # Abre o arquivo usando o caminho absoluto garantido pelo OS
    with open(caminho_arquivo, 'w') as arquivo_comando:
        arquivo_comando.write(f"{valor_para_ccpp:.1f}")
    
    # Imprime o caminho completo no terminal para confirmação visual
    print(f"\n✅ SUCESSO! Valor {valor_para_ccpp:.1f} transferido para:\n{caminho_arquivo}")

except Exception as e:
    print(f"\n❌ ERRO ao salvar arquivo de comando: {e}")

'''

import pandas 
import datetime 
import os # ESSENCIAL: para garantir o caminho correto

# Ler o arquivo CSV
df = pandas.read_csv('data/sensor_data.csv')

print(" ANÁLISE DE UMIDADE DO SOLO")
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

# AQUI é onde você define o valor a ser transferido!
valor_para_ccpp = ultimas_5_media 

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
print(f"\nVALOR RECOMENDADO PARA C/C++: {valor_para_ccpp:.1f}")

diretorio = r'C:\Users\jvmp2\Documents\FIAP\FASE2\Atividade1\smart-irrigation\data'
arquivo = os.path.join(diretorio, 'valor_ccpp.txt')

# Cria o diretório se não existir
os.makedirs(diretorio, exist_ok=True)

# Escreve o valor no arquivo
with open(arquivo, 'w', encoding='utf-8') as f:
    f.write(str(valor_para_ccpp))

print(f'Valor {valor_para_ccpp} escrito em: {arquivo}')
