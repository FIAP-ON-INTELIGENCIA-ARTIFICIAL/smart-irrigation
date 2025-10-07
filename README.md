# 🌾 Projeto AgroVision - Aplicação para Apoio ao Agronegócio

## 📌 Resumo do Projeto
O projeto tem como objetivo desenvolver **uma aplicação em Python** para auxiliar agricultores no **cálculo de áreas de plantio, aplicação de insumos e previsão de produção agrícola**.  
Além da implementação prática, o projeto integra **análises estatísticas em R**, permitindo explorar dados coletados de forma mais aprofundada, apoiando a **tomada de decisão no agronegócio**.  

O escopo também inclui:
- **Integração com APIs meteorológicas públicas**, para coleta e análise de dados climáticos.  
- **Uso de GitHub** como plataforma de versionamento e colaboração, simulando um ambiente real de desenvolvimento de software.  
- **Reflexão crítica** sobre o impacto social, ético e ambiental das tecnologias aplicadas ao agronegócio, por meio de atividades de leitura e resumo de artigos acadêmicos.  

---

## Apresentação do Seu Jorge do Agro

Conheça o Seu Jorge, agricultor do interior de Goiás e cliente representativo do Projeto AgroVision.  
Neste vídeo, ele compartilha sua experiência no campo e como a tecnologia pode transformar a produção agrícola.(click na imagem)

[![Apresentação do Seu Jorge do Agro](https://img.youtube.com/vi/cSJFwvnrj1w/hqdefault.jpg)](https://www.youtube.com/watch?v=cSJFwvnrj1w)

---

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

```
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

---

## 👥 Time

<table>
  <tr>
    <td width="110" align="center" valign="top">
      <a href="https://github.com/SabrinaOtoni">
        <img src="https://github.com/SabrinaOtoni.png" width="100" height="100" alt="Avatar de Sabrina Otoni" style="border-radius:50%; object-fit:cover;" />
      </a>
    </td>
    <td valign="middle">
      <strong style="font-size:1.05rem;">Sabrina Otoni</strong><br/>
      <a href="https://github.com/SabrinaOtoni">@SabrinaOtoni</a><br/>
      <img alt="Papel: Tutor" src="https://img.shields.io/badge/papel-Tutor-2ea44f?style=flat-square" />
    </td>
    <td width="110" align="center" valign="top">
      <a href="https://github.com/henriquehsilva">
        <img src="https://github.com/henriquehsilva.png" width="100" height="100" alt="Avatar de Henrique Silva" style="border-radius:50%; object-fit:cover;" />
      </a>
    </td>
    <td valign="middle">
      <strong style="font-size:1.05rem;">Henrique Silva</strong><br/>
      <a href="https://github.com/henriquehsilva">@henriquehsilva</a><br/>
      <img alt="Papel" src="https://img.shields.io/badge/papel-Desenvolvedor-36a2eb?style=flat-square" />
    </td>
    <td width="110" align="center" valign="top">
      <a href="https://github.com/manoellaweiser-gif">
        <img src="https://github.com/manoellaweiser-gif.png" width="100" height="100" alt="Avatar de Manoella Weiser" style="border-radius:50%; object-fit:cover;" />
      </a>
    </td>
    <td valign="middle">
      <strong style="font-size:1.05rem;">Manoella Weiser</strong><br/>
      <a href="https://github.com/manoellaweiser-gif">@manoellaweiser-gif</a><br/>
      <img alt="Papel" src="https://img.shields.io/badge/papel-Desenvolvedor-36a2eb?style=flat-square" />
    </td>
  </tr>
  <tr>
    <td width="110" align="center" valign="top">
      <a href="https://github.com/JoaoMDPaiva">
        <img src="https://github.com/JoaoMDPaiva.png" width="100" height="100" alt="Avatar de João Paiva" style="border-radius:50%; object-fit:cover;" />
      </a>
    </td>
    <td valign="middle">
      <strong style="font-size:1.05rem;">João Paiva</strong><br/>
      <a href="https://github.com/JoaoMDPaiva">@JoaoMDPaiva</a><br/>
      <img alt="Papel" src="https://img.shields.io/badge/papel-Desenvolvedor-36a2eb?style=flat-square" />
    </td>
    <td width="110" align="center" valign="top">
      <a href="https://github.com/adrylan">
        <img src="https://github.com/adrylan.png" width="100" height="100" alt="Avatar de Luis Adrylan" style="border-radius:50%; object-fit:cover;" />
      </a>
    </td>
    <td valign="middle">
      <strong style="font-size:1.05rem;">Luis Adrylan</strong><br/>
      <a href="https://github.com/adrylan">@adrylan</a><br/>
      <img alt="Papel" src="https://img.shields.io/badge/papel-Desenvolvedor-36a2eb?style=flat-square" />
    </td>
    <td width="110" align="center" valign="top">
      <a href="https://github.com/Luiz-Frederico">
        <img src="https://github.com/Luiz-Frederico.png" width="100" height="100" alt="Avatar de Luiz Ampelo" style="border-radius:50%; object-fit:cover;" />
      </a>
    </td>
    <td valign="middle">
      <strong style="font-size:1.05rem;">Luiz Campelo</strong><br/>
      <a href="https://github.com/Luiz-Frederico">@luizCampelo</a><br/>
      <img alt="Papel" src="https://img.shields.io/badge/papel-Desenvolvedor-36a2eb?style=flat-square" />
    </td>
  </tr>
</table>
