# 🌱 FarmTech Solutions – Fase 2
Sistema de Irrigação Inteligente com ESP32

## 🎯 Objetivo
Desenvolver um sistema automatizado de irrigação capaz de controlar a bomba d’água com base em sensores de umidade, luminosidade (pH simulado) e nutrientes (NPK), representando um sistema IoT agrícola.

---

## 🧩 Componentes Utilizados
| Componente | Função | Pinos ESP32 |
|-------------|--------|--------------|
| **ESP32** | Microcontrolador principal | — |
| **DHT22** | Sensor de temperatura e umidade do solo | VCC → 3V3, SDA → 15, GND → GND |
| **LDR** | Simulação do pH da terra | AO → 34, VCC → 3V3, GND → GND |
| **Relay (atuador)** | Liga/desliga a bomba de irrigação | VCC → 3V3, IN → 22, GND → GND |
| **LED Verde** | Indica bomba ligada | A → 23, C → GND |
| **Switch N, P, K** | Representam nutrientes Nitrogênio, Fósforo e Potássio | N → 18, P → 19, K → 21 |

---

## ⚙️ Funcionamento
- O **DHT22** lê a umidade do solo.  
- O **LDR** simula o **nível de pH** (quanto menor a luz, menor o pH).  
- As **chaves N, P, K** definem a presença dos nutrientes.  
- A **bomba (Relay)** liga automaticamente quando:
  - Todos os nutrientes estão ativados (NPK = true),
  - O pH está dentro da faixa ideal (6.0 – 6.8),
  - A umidade está abaixo do valor de irrigação (HUM_ON = 35%),
  - E **não há previsão de chuva**.
- O **LED verde** acende enquanto a bomba está ativa.

---

## 📊 Lógica de Controle
| Condição | Ação |
|-----------|------|
| NPK = true, pH entre 6.0–6.8 e Umidade < 35% | Liga bomba |
| Umidade > 45% ou chuva = true | Desliga bomba |

---

## 🧠 Código-Fonte
O código completo está em [`prog1.ino`](./prog1.ino) e foi desenvolvido em **C++ (Arduino)**.  
Ele inclui:
- Controle de debouncing para os switches  
- Função de leitura e média móvel de umidade  
- Interface via **Serial Monitor** (JSON outputs)

---

## 🔬 Simulação
A simulação foi feita na plataforma **[Wokwi](https://wokwi.com)** com o ESP32.  
Os valores de umidade e pH podem ser ajustados nos sliders do DHT22 e LDR.

---

## 🎥 Demonstração
📽️ Link do vídeo (YouTube – Não listado):  
👉 https://youtu.be/xX5vdG0Ofzg

---

## 🧠 Equipe FarmTech Solutions
**Integrantes:**  
- Matheus Meissner  

---

## 🏁 Resultados Esperados
- Sistema automatizado funcional  
- Simulação completa no Wokwi  
- Código comentado e bem estruturado  
- Relatório e README detalhados
