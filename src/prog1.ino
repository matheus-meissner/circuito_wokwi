/* FarmTech Fase 2 – ESP32 (NPK com SLIDE SWITCH)
 * Pinagem:
 *  DHT22 ............ GPIO15
 *  LDR (AO invertido) ADC34
 *  Switches N,P,K ... GPIO18, 19, 21  (SPDT: COM->GPIO, lado A->3V3, lado B->GND)
 *  RELÉ (bomba) ..... GPIO22
 *  LED indicador .... GPIO23
 */

#include <Arduino.h>
#include <DHT.h>

#define PIN_DHT      15
#define DHTTYPE      DHT22
#define PIN_LDR_AO   34

#define PIN_SW_N     18
#define PIN_SW_P     19
#define PIN_SW_K     21

#define PIN_RELAY    22
#define PIN_LED      23

const bool RELAY_ACTIVE_HIGH = true;

// Alvos (ex.: tomate)
const float PH_MIN  = 6.0;
const float PH_MAX  = 6.8;
const float HUM_ON  = 35.0;  // liga
const float HUM_OFF = 45.0;  // desliga (histerese)

// Ritmos
unsigned long LOOP_MS  = 600;
unsigned long PRINT_MS = 1500;

// Média móvel da umidade
const uint8_t HUM_WINDOW = 5;

DHT dht(PIN_DHT, DHTTYPE);

// Estado
bool bombaLigada = false;
bool previsaoChuva = false;

// Console
bool paused  = false;
bool stepOne = false;
bool verbose = false;

// Média
float humBuf[HUM_WINDOW];
uint8_t humIdx = 0, humCount = 0;

// Últimos valores impressos (print-on-change)
float lastHum = NAN, lastPh = NAN;
bool  lastPump=false, lastRain=false, lastN=false, lastP=false, lastK=false;

static inline float mapf(float x,float in_min,float in_max,float out_min,float out_max){
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float addHumiditySample(float h){
  if (isnan(h)) return NAN;
  humBuf[humIdx] = h;
  humIdx = (humIdx + 1) % HUM_WINDOW;
  if (humCount < HUM_WINDOW) humCount++;
  float sum = 0;
  for (uint8_t i=0;i<humCount;i++) sum += humBuf[i];
  return sum / humCount;
}

void setRelay(bool on){
  if (RELAY_ACTIVE_HIGH) digitalWrite(PIN_RELAY, on ? HIGH : LOW);
  else                   digitalWrite(PIN_RELAY, on ? LOW  : HIGH);
  digitalWrite(PIN_LED, on ? HIGH : LOW);
  bombaLigada = on;
}

void printJSON(float hum, float ph, bool npkN, bool npkP, bool npkK){
  Serial.print("{\"umid\":");
  if (isnan(hum)) Serial.print("null"); else Serial.print(hum,1);
  Serial.print(",\"ph\":");    Serial.print(ph,2);
  Serial.print(",\"N\":");     Serial.print(npkN ? "true":"false");
  Serial.print(",\"P\":");     Serial.print(npkP ? "true":"false");
  Serial.print(",\"K\":");     Serial.print(npkK ? "true":"false");
  Serial.print(",\"chuva\":"); Serial.print(previsaoChuva ? "true":"false");
  Serial.print(",\"bomba\":"); Serial.print(bombaLigada ? "true":"false");
  Serial.println("}");
}

void handleSerial(){
  while (Serial.available()){
    char c = Serial.read();
    if (c=='P'||c=='p'){ paused = true;  Serial.println("{\"mode\":\"PAUSE\"}"); }
    else if (c=='C'||c=='c'){ paused = false; Serial.println("{\"mode\":\"RUN\"}"); }
    else if (c=='S'||c=='s'){ stepOne = true; Serial.println("{\"mode\":\"STEP\"}"); }
    else if (c=='V'||c=='v'){ verbose = !verbose; Serial.print("{\"verbose\":"); Serial.print(verbose?"true":"false"); Serial.println("}"); }
    else if (c=='F'||c=='f'){ lastHum = NAN; lastPh = NAN; } // força imprimir
    else if (c=='R'){ while (!Serial.available()) {} char v = Serial.read(); previsaoChuva = (v=='1'); Serial.print("{\"cmd\":\"previsaoChuva\",\"value\":"); Serial.print(previsaoChuva?"true":"false"); Serial.println("}"); }
  }
}

void setup(){
  Serial.begin(115200);
  dht.begin();

  // Slide switches SPDT: COM -> GPIO, lado A->3V3, lado B->GND
  pinMode(PIN_SW_N, INPUT);   // sem pullup/pulldown, pois chave fornece 3V3/0V
  pinMode(PIN_SW_P, INPUT);
  pinMode(PIN_SW_K, INPUT);

  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  setRelay(false);

  analogReadResolution(12);

  Serial.println("{\"status\":\"start\",\"hint\":\"P=pause C=continue S=step V=verbose F=force R1/R0=chuva\"}");
}

void loop(){
  handleSerial();

  if (paused && !stepOne) return;

  static unsigned long lastLoop = 0;
  unsigned long now = millis();
  if (now - lastLoop < LOOP_MS) return;
  lastLoop = now;
  stepOne = false;

  // --- Leitura NPK: HIGH quando a chave está para o lado do 3V3; LOW quando para o GND
  bool npkN = (digitalRead(PIN_SW_N) == HIGH);
  bool npkP = (digitalRead(PIN_SW_P) == HIGH);
  bool npkK = (digitalRead(PIN_SW_K) == HIGH);

  // --- Sensores
  float h    = dht.readHumidity();
  float hAvg = addHumiditySample(h);
  int   raw  = analogRead(PIN_LDR_AO);

  // Seu módulo AO: escuro->raw alto->pH alto (mapeamento invertido)
  float ph   = mapf(raw, 0, 4095, 14.0, 0.0);
  if (ph < 0) ph = 0; if (ph > 14) ph = 14;

  // --- Regras
  bool humOk = (!isnan(hAvg) && (hAvg < HUM_ON));
  bool phOk  = (ph >= PH_MIN && ph <= PH_MAX);
  bool npkOk = (npkN && npkP && npkK);

  if (!bombaLigada){
    if (humOk && phOk && npkOk && !previsaoChuva) setRelay(true);
  }else{
    if (previsaoChuva || !npkOk || !phOk || isnan(hAvg) || (hAvg > HUM_OFF)) setRelay(false);
  }

  // --- Impressão controlada
  bool changed =
      (isnan(lastHum) != isnan(hAvg)) ||
      (!isnan(hAvg) && !isnan(lastHum) && fabs(hAvg - lastHum) > 0.5) ||
      (fabs(ph - lastPh) > 0.05) ||
      (npkN != lastN) || (npkP != lastP) || (npkK != lastK) ||
      (bombaLigada != lastPump) || (previsaoChuva != lastRain);

  if (verbose){
    static unsigned long lastPrint = 0;
    if (now - lastPrint >= PRINT_MS){
      printJSON(hAvg, ph, npkN, npkP, npkK);
      lastPrint = now;
    }
  }else{
    if (changed){
      printJSON(hAvg, ph, npkN, npkP, npkK);
      lastHum = hAvg; lastPh = ph; lastN = npkN; lastP = npkP; lastK = npkK; lastPump = bombaLigada; lastRain = previsaoChuva;
    }
  }
}
