# 🌱 Smart Irrigation

Sistema experimental de irrigação inteligente de baixo custo, projetado para **testar lógicas de irrigação baseadas em nutrientes (NPK), pH e umidade do solo**, com possibilidade de integração climática (OpenWeather) e análise estatística (R).

---

## Visão Geral

Nesta etapa, o protótipo:
- Monitora **níveis de NPK** via três botões físicos (simulação);
- Simula **pH** por meio de um **sensor LDR**;
- Mede **umidade** com o **DHT22**;
- Aciona automaticamente um **relé** (bomba d’água) conforme regras específicas da **cultura agrícola** selecionada.

A proposta busca oferecer um **ambiente replicável e acessível** para pesquisa e ensino em automação agrícola.

---

## Arquitetura

```mermaid
flowchart LR
A[Sensores (NPK, LDR, DHT22)] --> B[Microcontrolador (ESP32/Arduino)]
B -->|Serial| C[Python Simulator]
C -->|Dados CSV| D[R - Analytics]
C --> E[Relé / Bomba d'Água]
C -->|API| F[OpenWeather]
````

---
## Funcionalidades

### Visão Geral dos Módulos

| Módulo     | Função                                             | Tecnologia               |
|------------|----------------------------------------------------|--------------------------|
| `firmware` | Leitura dos sensores e controle do relé           | C++ / Arduino            |
| `simulator`| Geração e transmissão de dados                     | Python                   |
| `analytics`| Análise estatística e otimização de thresholds     | R                        |
| `docs`     | Documentação técnica e de negócio                  | Markdown / Draw.io       |

---

## Regras de Irrigação (exemplo)

### Parâmetros por Cultura

| Cultura     | Umidade (%) | pH        | NPK (médio)            | Ação           |
|-------------|--------------|-----------|-------------------------|----------------|
| Tomateiro   | < 40         | 6.0–6.8   | N=20, P=10, K=20        | Acionar bomba  |
| Alface      | < 50         | 5.5–6.5   | N=15, P=10, K=15        | Acionar bomba  |
| Feijão      | < 35         | 6.0–7.0   | N=25, P=10, K=20        | Acionar bomba  |

---

## Integração Climática (OpenWeather)

O sistema pode suspender a irrigação automática se houver previsão de chuva nas próximas 12h, consultando a API do OpenWeather.

