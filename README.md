# FIAP - Faculdade de Informática e Administração Paulista 

![fiap-logo](https://github.com/henriquehsilva/template-projeto-fiap/blob/main/assets/logo-fiap.png, width="200")

<br>

# Smart-Irrigation 

<p align="center">
  <img src="https://github.com/FIAP-ON-INTELIGENCIA-ARTIFICIAL/agrovision-pwa/blob/main/logo-agrovision.png" alt="AgroVision" width="600">
</p>

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

## Professores

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
## Planejamento e Modelagem da Solução

O projeto foi planejado utilizando o **HSD Framework** (Hypothesize → Shape → Deliver) para transformar ideias em hipóteses testáveis, definir escopos enxutos e orquestrar entregas contínuas, garantindo que as necessidades e expectativas dos usuários finais sejam atendidas.

Para mais detalhes sobre o HSD Framework, consulte o [HSD Consultoria — da hipótese ao impacto.](https://henriquesilva.substack.com/)

Board do projeto [Miro](https://miro.com/app/board/uXjVJC8Oa2s=/?share_link_id=719781750699).

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
Execute:
```bash
   ESP32_PORT=/dev/cu.usbserial-5A7B0701171 ESP32_BAUD=115200 python scripts/serial_to_csv.py
```
**Obs.:** A porta provavelmente será outra acima (usbserial-5A7B0701171) está como exemplo.

### 3) Análise (R)
1. Abra o RStudio.
2. Carregue `src/analytics/analysis.R`.
3. Ajuste o caminho do CSV (linha 10).
4. Execute o script para gerar gráficos e relatórios.

---
### 4) Stories entregues — Versão 1.0

1 - COMO estudante, QUERO inserir dados via Serial no simulador, PARA validar rapidamente cenários sem depender de conectividade externa.

COMO desenvolvedor, QUERO transferir dados manualmente do Python para C/C++, PARA testar o impacto da integração em ambientes de baixo custo.

3 - COMO usuário, QUERO medir a umidade com o sensor DHT22, PARA que a irrigação ocorra automaticamente conforme o limite configurado.

4 - COMO agricultor pesquisador, QUERO simular níveis de NPK através de botões, PARA testar diferentes condições de nutrientes sem precisar de sensores caros.

5 - COMO estudante, QUERO simular o pH usando um sensor LDR, PARA compreender o impacto da acidez/alkalinidade no acionamento da irrigação.

6 - COMO mentor acadêmico, QUERO que as regras sejam documentadas com base em KPIs, PARA garantir que os resultados possam ser comparados e replicados.

7 - COMO pesquisador, QUERO analisar os dados dos sensores em R, PARA encontrar padrões estatísticos que melhorem a robustez das regras de irrigação.

8 - COMO agricultor, QUERO que o sistema se integre ao OpenWeather, PARA suspender a irrigação quando houver previsão de chuva e evitar desperdício de água.

9 - COMO agricultor, QUERO que um relé acione a bomba d’água automaticamente, PARA manter a irrigação alinhada às regras da cultura agrícola escolhida.

10 - COMO usuário avançado, QUERO configurar regras de irrigação baseadas em nutrientes, pH e umidade, PARA personalizar o controle conforme o tipo de cultura.

11 - COMO pesquisador, QUERO visualizar no dashboard a previsão do tempo ao lado dos sensores, PARA correlacionar clima e decisões de irrigação.

---
### 5) proximos passos (backlog)

12 - COMO agricultor, QUERO uma PWA (Progressive Web App) para acessar o sistema de irrigação de qualquer lugar, PARA facilitar o gerenciamento e monitoramento.

---
## 📚 Referências
- [Documentação Firebase ESP32](https://firebase-esp32.readthedocs.io/en/latest/)
- [Documentação ArduinoJson](https://arduinojson.org/)
- [Documentação PySerial](https://pyserial.readthedocs.io/en/latest/)
- [Documentação OpenWeather API](https://openweathermap.org/api)  
- [Documentação R](https://www.r-project.org/)

---
## 📝 Licença

Este projeto está licenciado sob a Licença Creative Commons Atribuição 4.0 Internacional. Para mais detalhes, consulte o arquivo [LICENSE](LICENSE).
