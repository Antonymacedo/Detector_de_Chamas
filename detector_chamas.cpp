#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

// =====================================================
//  CONFIGURE AQUI
// =====================================================
const char* WIFI_SSID     = "SEU_WIFI";
const char* WIFI_PASSWORD = "SUA_SENHA";

// Tópico que você criou no app ntfy (deve ser único)
const char* NTFY_TOPICO = "detector-chamas-seutopico";
// =====================================================

// Pinos
const int sensorPin   = 0;
const int ledVerde    = 8;
const int ledVermelho = 9;
const int buzzer      = 10;

const int LIMIAR = 100;

const unsigned long COOLDOWN_LEMBRETE = 30000; // 30 s entre lembretes

// ---- Estado interno ----
bool chamaAtiva = false;
unsigned long ultimaNotificacao = 0;

// -------------------------------------------------------

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

// Envia notificação push via ntfy.sh
// titulo    : título da notificação
// mensagem  : corpo da notificação
// prioridade: "default" | "high" | "urgent" | "low"
// forcar    : ignora cooldown quando true
void enviarNtfy(const String& titulo,
                const String& mensagem,
                const String& prioridade = "default",
                bool forcar = false) {

  unsigned long agora = millis();
  if (!forcar && (agora - ultimaNotificacao < COOLDOWN_LEMBRETE)) return;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado. Reconectando...");
    conectarWiFi();
  }

  String url = "https://ntfy.sh/";
  url += NTFY_TOPICO;

  HTTPClient http;
  http.begin(url);
  http.addHeader("Title",    titulo);
  http.addHeader("Priority", prioridade);
  http.addHeader("Tags",     "fire");        // ícone de fogo na notificação

  Serial.print("Enviando notificacao... ");
  int httpCode = http.POST(mensagem);

  if (httpCode == HTTP_CODE_OK || httpCode == 200) {
    Serial.println("OK");
    ultimaNotificacao = agora;
  } else {
    Serial.printf("FALHOU (HTTP %d)\n", httpCode);
  }

  http.end();
}

// -------------------------------------------------------

void setup() {
  Serial.begin(115200);

  pinMode(ledVerde,    OUTPUT);
  pinMode(ledVermelho, OUTPUT);
  pinMode(buzzer,      OUTPUT);

  digitalWrite(ledVerde,    HIGH);
  digitalWrite(ledVermelho, LOW);
  digitalWrite(buzzer,      LOW);

  conectarWiFi();

  enviarNtfy("Detector de Chamas", "Sistema iniciado e online.", "low", true);
  Serial.println("Detector de Chamas Iniciado");
}

void loop() {
  int leitura = analogRead(sensorPin);

  Serial.print("Leitura: ");
  Serial.println(leitura);

  // ---------- CHAMA DETECTADA ----------
  if (leitura > LIMIAR) {

    digitalWrite(ledVerde,    LOW);
    digitalWrite(ledVermelho, HIGH);
    digitalWrite(buzzer,      HIGH);
    Serial.println("CHAMA DETECTADA!");

    if (!chamaAtiva) {
      chamaAtiva = true;
      // Primeiro alerta: prioridade máxima, imediato
      enviarNtfy("ALERTA DE INCENDIO",
                 "Chama detectada! Verifique imediatamente.",
                 "urgent",
                 true);
    } else {
      // Lembrete enquanto chama continua ativa
      enviarNtfy("Chama ainda ativa",
                 "O sensor continua detectando chama.",
                 "high");
    }

  // ---------- SISTEMA NORMAL ----------
  } else {

    if (chamaAtiva) {
      chamaAtiva = false;
      enviarNtfy("Chama extinguida",
                 "Sistema normalizado.",
                 "default",
                 true);
    }

    digitalWrite(ledVerde,    HIGH);
    digitalWrite(ledVermelho, LOW);
    digitalWrite(buzzer,      LOW);
  }

  delay(50);
}
