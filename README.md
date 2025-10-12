# FIAP - Faculdade de Informática e Administração Paulista 

<p align="center">
<a href="https://www.fiap.com.br/">
  <img src="https://github.com/henriquehsilva/template-projeto-fiap/blob/main/assets/logo-fiap.png" alt="FIAP - Faculdade de Informática e Admnistração Paulista" border="0" width="40%" height="40%">
</a>
</p>

<br>

# Smart-Irrigation (AgroVision TEAM)

## 👨‍🎓 Integrantes

<table>
  <tr>
    <td width="110" align="center" valign="top">
      <a href="https://github.com/henriquehsilva">
        <img src="https://github.com/henriquehsilva.png" width="100" height="100" alt="Henrique Silva" style="border-radius:50%; object-fit:cover;" />
      </a>
      <br/><strong>Henrique Silva</strong><br/>
      <a href="https://github.com/henriquehsilva">@henriquehsilva</a>
    </td>
    <td width="110" align="center" valign="top">
      <a href="https://github.com/manoellaweiser-gif">
        <img src="https://github.com/manoellaweiser-gif.png" width="100" height="100" alt="Manoella Weiser" style="border-radius:50%; object-fit:cover;" />
      </a>
      <br/><strong>Manoella Weiser</strong><br/>
      <a href="https://github.com/manoellaweiser-gif">@manoellaweiser-gif</a>
    </td>
    <td width="110" align="center" valign="top">
      <a href="https://github.com/JoaoMDPaiva">
        <img src="https://github.com/JoaoMDPaiva.png" width="100" height="100" alt="João Paiva" style="border-radius:50%; object-fit:cover;" />
      </a>
      <br/><strong>João Paiva</strong><br/>
      <a href="https://github.com/JoaoMDPaiva">@JoaoMDPaiva</a>
    </td>
    <td width="110" align="center" valign="top">
      <a href="https://github.com/adrylan">
        <img src="https://github.com/adrylan.png" width="100" height="100" alt="Luis Adrylan" style="border-radius:50%; object-fit:cover;" />
      </a>
      <br/><strong>Luis Adrylan</strong><br/>
      <a href="https://github.com/adrylan">@adrylan</a>
    </td>
    <td width="110" align="center" valign="top">
      <a href="https://github.com/Luiz-Frederico">
        <img src="https://github.com/Luiz-Frederico.png" width="100" height="100" alt="Luiz Campelo" style="border-radius:50%; object-fit:cover;" />
      </a>
      <br/><strong>Luiz Campelo</strong><br/>
      <a href="https://github.com/Luiz-Frederico">@luizCampelo</a>
    </td>
  </tr>
</table>

## 👩‍🏫 Tutor(a)

<table>
  <tr>
    <td width="110" align="center" valign="top">
      <a href="https://github.com/SabrinaOtoni">
        <img src="https://github.com/SabrinaOtoni.png" width="100" height="100" alt="Sabrina Otoni (Tutor)" style="border-radius:50%; object-fit:cover;" />
      </a>
      <br/><strong>Sabrina Otoni</strong><br/>
      <span>Tutor(a)</span><br/>
      <a href="https://github.com/SabrinaOtoni">@SabrinaOtoni</a>
    </td>
  </tr>
</table>

## 🧭 Coordenador(a)

<table>
  <tr>
    <td width="110" align="center" valign="top">
      <a href="#">
        <img src="https://github.com/agodoi.png" width="100" height="100" alt="Coordenador(a)" style="border-radius:50%; object-fit:cover; filter: grayscale(20%);" />
      </a>
      <br/><strong>André Godoi</strong><br/>
      <span>Coordenador(a)</span><br/>
      <a href="https://github.com/agodoi">@agodoi</a>
    </td>
  </tr>
</table>

---

## 📜 Descrição

O **Smart-Irrigation** (dentro do escopo do projeto **AgroVision**) é um sistema experimental e de baixo custo para **automação de irrigação** com foco em ensino e pesquisa. O protótipo integra:

- **Firmware ESP32 (Arduino/C++)** para leitura de sensores/simulações (umidade do solo, LDR→pH didático, níveis N-P-K por botões), aplicação de **regras de histerese e segurança** e acionamento da bomba via relé.
- **Coleta de dados em Python** pela porta serial, com geração de **CSV** e **NDJSON**, rotação diária de arquivos e logs brutos para auditoria.
- **Análises em R** para consolidação dos dados e criação de relatórios/gráficos diários (ex.: evolução da umidade).
- **Integração climática** via API pública (OpenWeather), permitindo políticas como **suspender irrigação se houver previsão de chuva**.

Além da implementação, o projeto reforça boas práticas de desenvolvimento (Git/GitHub), documentação e reflexão sobre **impactos sociais, éticos e ambientais** da tecnologia no agronegócio.

**Apresentação do “Seu Jorge do Agro”** – Cliente representativo:
[![Apresentação do Seu Jorge do Agro](https://img.youtube.com/vi/cSJFwvnrj1w/hqdefault.jpg)](https://www.youtube.com/watch?v=cSJFwvnrj1w)


**[FIAP | Fase 2] Smart-Irrigation
”** – Funcionamento Completo do Projeto:
[![Smart-Irrigation - uncionamento Completo do Projeto](https://img.youtube.com/vi/m1FP4ee3Ig4/hqdefault.jpg)](https://youtu.be/m1FP4ee3Ig4?si=tEUa6hlrO7wI_4af)


LINK-VIDEO: [https://youtu.be/m1FP4ee3Ig4?si=tEUa6hlrO7wI_4af](https://youtu.be/m1FP4ee3Ig4?si=tEUa6hlrO7wI_4af)
---

## 📁 Estrutura de pastas

- **src/**: código-fonte do projeto ao longo das fases:
  - `firmware/` – código do ESP32 (Arduino/C++).
  - `scripts/` – (coleta serial → CSV/NDJSON) e utilitários.
  - `analytics/` – scripts em R (`analysis.R`) para gráficos/relatórios.
- **README.md**: guia geral do projeto.

---

## 🔧 Como executar o código

### Pré-requisitos
- **ESP32 + Arduino IDE** (ou PlatformIO)  
  - Placa ESP32 instalada no Arduino IDE  
  - Bibliotecas: `Firebase_ESP_Client`, `ArduinoJson`, `WiFi`, etc.
- **Python 3.10+**  
  - `pip install pyserial`
- **R 4.x**  
  - Pacotes base (para `read.csv` e `png`; instale extras se necessário)

### 1) Firmware (ESP32)
1. Build a aplicação
```bash
docker compose up --build -d
```
2. Crie `env.h` com suas 
credenciais (Wi-Fi, Firebase, OpenWeather, etc.).
```bash
mv .env.example .env /
docker compose run --rm envgen
```
3. Compile 
```bash
docker compose run --rm pio
```
4. Abra `src/firmware/`
5. Compile e faça o upload para o ESP32.
```bash
pio run -e esp32dev -t upload --upload-port /dev/cu.usbserial-5A7B0701171
```
6. Abra o **Serial Monitor** (115200 baud) e valide os logs de leitura e “CSVX”.
```bash
pio device monitor --port /dev/cu.usbserial-5A7B0701171 -b 115200 --rts 0 --dtr 0
```

### 2) Coleta (Python)`.
2. Execute:
   ```bash
   ESP32_PORT=/dev/cu.usbserial-5A7B0701171 ESP32_BAUD=115200 python scripts/serial_to_csv.py

**Obs.:** A porta provavelmente será outra acima (usbserial-5A7B0701171) está como exemplo.
