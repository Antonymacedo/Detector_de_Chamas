

# Detector de Chamas com ESP32 e Notificação no Celular 🔥
 
Sistema de detecção de chamas usando um fototransistor conectado ao **ESP32 Mini C3**, com alertas visuais (LEDs), sonoros (buzzer) e **notificações push no celular** via [ntfy.sh](https://ntfy.sh) — sem depender de nenhum aplicativo de mensagens.
 
Quando uma chama é detectada, o sistema aciona o LED vermelho e o buzzer, e envia uma notificação instantânea ao celular. Quando a chama é extinta, uma confirmação de normalização também é enviada. Este é um projeto base que pode ser expandido para automação residencial e sistemas de segurança IoT.
 
---
 
## Demonstração 📸
 
> **Foto do circuito montado**

> <img width="400" height="500" alt="WhatsApp Image 2026-06-06 at 17 41 27" src="https://github.com/user-attachments/assets/4d9e6e5a-645e-4081-8f9e-b35fa8f9b38b" />

 
---
 
> **Vídeo do funcionamento**

>https://github.com/user-attachments/assets/f4a06781-566e-475a-8e73-563353c6143e

---
 
## Funcionalidades 🤖
 
- Leitura analógica contínua do fototransistor
- LED verde indica sistema em repouso; LED vermelho indica chama detectada
- Buzzer acionado ao detectar chama
- Notificação push imediata no celular (via ntfy.sh) na primeira detecção
- Lembrete a cada 30 segundos enquanto a chama continuar ativa
- Notificação de confirmação quando a chama é extinta
- Reconexão automática ao WiFi em caso de queda
---
 
## Materiais ⚙️
 
| Componente | Quantidade |
|---|---|
| ESP32 Mini C3 | 1 |
| Fototransistor (sensor de chama infravermelho) | 1 |
| LED Verde | 1 |
| LED Vermelho | 1 |
| Buzzer passivo ou ativo | 1 |
| Resistor 220Ω | 2 |
| Protoboard | 1 |
| Fios de conexão | — |
 
> O ESP32 Mini C3 já possui WiFi integrado — nenhum módulo extra é necessário para as notificações.
 
---
 
## Conexões ⚡️
 
| Componente | Pino no ESP32 Mini C3 | Descrição |
|---|---|---|
| Fototransistor (OUT) | GPIO 0 (ADC) | Leitura analógica do sensor |
| Fototransistor (VCC) | 3.3V | Alimentação do sensor |
| Fototransistor (GND) | GND | Terra do circuito |
| LED Verde | GPIO 8 | Indica sistema em repouso (via resistor 220Ω) |
| LED Vermelho | GPIO 9 | Indica chama detectada (via resistor 220Ω) |
| Buzzer | GPIO 10 | Alerta sonoro |
 
---
 
## Dependências e Ambiente 🛠️
 
O projeto usa **PlatformIO** com o framework Arduino para ESP32.
 
As bibliotecas `WiFi.h` e `HTTPClient.h` já fazem parte do core do ESP32 — nenhuma instalação adicional é necessária.
 
**`platformio.ini`**
```ini
[env:esp32-c3-devkitm-1]
platform  = espressif32
board     = esp32-c3-devkitm-1
framework = arduino
monitor_speed = 115200
```
 
---
 
## Configuração do ntfy.sh 🔔
 
O [ntfy.sh](https://ntfy.sh) é um serviço gratuito de notificações push que funciona com uma simples requisição HTTP — sem cadastro, sem API key.
 
1. Instale o app **ntfy** no seu celular ([Android](https://play.google.com/store/apps/details?id=io.hntr.ntfy) / [iOS](https://apps.apple.com/app/ntfy/id1625396347))
2. Dentro do app, toque em **"+"** e inscreva-se em um tópico único, por exemplo: `detector-chamas-abc123`
3. No código, preencha esse mesmo nome no campo `NTFY_TOPICO`
4. Pronto — qualquer mensagem enviada para esse tópico aparecerá como notificação
> **Atenção:** O ntfy.sh é público por padrão. Use um nome de tópico difícil de adivinhar para evitar que outras pessoas recebam suas notificações.
 
---
 
## Como Funciona 🔍
 
O ESP32 faz leituras analógicas do fototransistor a cada 50ms. O fototransistor é sensível à radiação infravermelha emitida por chamas, e seu valor aumenta conforme a intensidade da luz detectada.
 
Quando o valor ultrapassa o `LIMIAR` definido no código (padrão: 100), o sistema entra em estado de alerta: aciona o LED vermelho, o buzzer, e envia uma notificação com prioridade máxima ao celular. Enquanto a chama permanecer ativa, lembretes são enviados a cada 30 segundos. Ao se extinguir, uma notificação de normalização é enviada.
 
O sistema também envia uma mensagem de "online" ao inicializar, útil para confirmar conectividade após uma queda de energia ou reinicialização.
 
---
 
## Código 💻
 
```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
 
// =====================================================
//  CONFIGURE AQUI
// =====================================================
const char* WIFI_SSID     = "SEU_WIFI";
const char* WIFI_PASSWORD = "SUA_SENHA";
const char* NTFY_TOPICO   = "detector-chamas-seutopico";
// =====================================================
 
const int sensorPin   = 0;
const int ledVerde    = 8;
const int ledVermelho = 9;
const int buzzer      = 10;
 
const int LIMIAR = 100;
const unsigned long COOLDOWN_LEMBRETE = 30000;
 
bool chamaAtiva = false;
unsigned long ultimaNotificacao = 0;
 
String urlEncode(const String& texto) {
  String encoded = "";
  for (size_t i = 0; i < texto.length(); i++) {
    unsigned char c = texto[i];
    if (isAlphaNumeric(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += (char)c;
    } else {
      char buf[4];
      sprintf(buf, "%%%02X", c);
      encoded += buf;
    }
  }
  return encoded;
}
 
void conectarWiFi() {
  Serial.print("Conectando ao WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi conectado! IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nFalha ao conectar. Reiniciando...");
    ESP.restart();
  }
}
 
void enviarNtfy(const String& titulo, const String& mensagem,
                const String& prioridade = "default", bool forcar = false) {
  unsigned long agora = millis();
  if (!forcar && (agora - ultimaNotificacao < COOLDOWN_LEMBRETE)) return;
  if (WiFi.status() != WL_CONNECTED) conectarWiFi();
 
  String url = "https://ntfy.sh/";
  url += NTFY_TOPICO;
 
  HTTPClient http;
  http.begin(url);
  http.addHeader("Title",    titulo);
  http.addHeader("Priority", prioridade);
  http.addHeader("Tags",     "fire");
 
  int httpCode = http.POST(mensagem);
  if (httpCode == 200) ultimaNotificacao = agora;
  http.end();
}
 
void setup() {
  Serial.begin(115200);
  pinMode(ledVerde,    OUTPUT);
  pinMode(ledVermelho, OUTPUT);
  pinMode(buzzer,      OUTPUT);
  digitalWrite(ledVerde, HIGH);
  digitalWrite(ledVermelho, LOW);
  digitalWrite(buzzer, LOW);
  conectarWiFi();
  enviarNtfy("Detector de Chamas", "Sistema iniciado e online.", "low", true);
}
 
void loop() {
  int leitura = analogRead(sensorPin);
  Serial.print("Leitura: ");
  Serial.println(leitura);
 
  if (leitura > LIMIAR) {
    digitalWrite(ledVerde,    LOW);
    digitalWrite(ledVermelho, HIGH);
    digitalWrite(buzzer,      HIGH);
    if (!chamaAtiva) {
      chamaAtiva = true;
      enviarNtfy("ALERTA DE INCENDIO", "Chama detectada! Verifique imediatamente.", "urgent", true);
    } else {
      enviarNtfy("Chama ainda ativa", "O sensor continua detectando chama.", "high");
    }
  } else {
    if (chamaAtiva) {
      chamaAtiva = false;
      enviarNtfy("Chama extinguida", "Sistema normalizado.", "default", true);
    }
    digitalWrite(ledVerde,    HIGH);
    digitalWrite(ledVermelho, LOW);
    digitalWrite(buzzer,      LOW);
  }
  delay(50);
}
```
 
---
 
## Sobre o Projeto 📋
 
Desenvolvido como projeto técnico no curso de **Eletrotécnica do CEFET-MG**, integrando conceitos de eletrônica analógica, programação embarcada e IoT.
 
**Autores:** Antony Macedo, Isaac  
**Ambiente:** PlatformIO + ESP32 Arduino Core  
**Licença:** MIT
