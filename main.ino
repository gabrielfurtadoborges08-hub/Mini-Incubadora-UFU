#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// ================= DISPLAY LCD I2C =================
// Se o display não funcionar com 0x27, troque para 0x3F
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= DHT22 =================
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// ================= LEDS =================
#define LED_AQUECER 18
#define LED_ESFRIAR 19

// ================= LIMITES =================

// Para incubadora depois, se quiser:
// float tempMin = 36.0;
// float tempMax = 37.5;

// ================= WI-FI DA ESP32 =================
const char* ssid = "Mini_Incubadora_UFU";
const char* password = "12345678";

WebServer server(80);

// ================= VARIAVEIS =================
float temperatura = 0.0;
float umidade = 0.0;
bool erroDHT = false;

String statusSistema = "Iniciando";
String textoStatus = "Sistema iniciando";

unsigned long tempoAnterior = 0;
const unsigned long intervalo = 2000;

// ================= LER DHT22 E CONTROLAR LEDS =================
void lerDHT22() {
  float t = dht.readTemperature();
  float u = dht.readHumidity();

  if (isnan(t) || isnan(u)) {
    erroDHT = true;
    statusSistema = "Erro";
    textoStatus = "Erro no sensor";

    digitalWrite(LED_AQUECER, LOW);
    digitalWrite(LED_ESFRIAR, LOW);

    Serial.println("Erro ao ler DHT22");
    return;
  }

  erroDHT = false;
  temperatura = t;
  umidade = u;

  if (temperatura < tempMin) {
    statusSistema = "Aquecer";
    textoStatus = "Necessario aquecer";

    digitalWrite(LED_AQUECER, HIGH);
    digitalWrite(LED_ESFRIAR, LOW);
  } 
  else if (temperatura > tempMax) {
    statusSistema = "Esfriar";
    textoStatus = "Necessario esfriar";

    digitalWrite(LED_AQUECER, LOW);
    digitalWrite(LED_ESFRIAR, HIGH);
  } 
  else {
    statusSistema = "Ideal";
    textoStatus = "Temperatura ideal";

    digitalWrite(LED_AQUECER, LOW);
    digitalWrite(LED_ESFRIAR, LOW);
  }

  Serial.print("Temperatura: ");
  Serial.print(temperatura);
  Serial.println(" C");

  Serial.print("Umidade: ");
  Serial.print(umidade);
  Serial.println(" %");

  Serial.print("Status: ");
  Serial.println(textoStatus);

  Serial.println("----------------");
}

// ================= ATUALIZAR LCD =================
void atualizarDisplay() {
  lcd.clear();

  if (erroDHT) {
    lcd.setCursor(0, 0);
    lcd.print("Erro DHT22");
    lcd.setCursor(0, 1);
    lcd.print("Verifique fios");
    return;
  }

  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(temperatura, 1);
  lcd.print((char)223);
  lcd.print("C ");

  if (statusSistema == "Aquecer") {
    lcd.print("AQUEC");
  } 
  else if (statusSistema == "Esfriar") {
    lcd.print("ESFR");
  } 
  else {
    lcd.print("OK");
  }

  lcd.setCursor(0, 1);
  lcd.print("U:");
  lcd.print(umidade, 1);
  lcd.print("% IP:4.1");
}

// ================= PAGINA WEB BONITA =================
void paginaPrincipal() {
  String corStatus = "#15803d";
  String fundoStatus = "#dcfce7";
  String bordaStatus = "#86efac";
  String iconeStatus = "OK";

  String tempTexto = String(temperatura, 1);
  String umidTexto = String(umidade, 1);

  if (statusSistema == "Aquecer") {
    corStatus = "#b91c1c";
    fundoStatus = "#fee2e2";
    bordaStatus = "#fecaca";
    iconeStatus = "!";
  } 
  else if (statusSistema == "Esfriar") {
    corStatus = "#1d4ed8";
    fundoStatus = "#dbeafe";
    bordaStatus = "#bfdbfe";
    iconeStatus = "!";
  } 
  else if (statusSistema == "Erro") {
    corStatus = "#991b1b";
    fundoStatus = "#fecaca";
    bordaStatus = "#fca5a5";
    iconeStatus = "X";
    tempTexto = "--";
    umidTexto = "--";
  }

  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <meta http-equiv="refresh" content="3">
  <meta name="color-scheme" content="light">
  <title>Mini Incubadora Portatil</title>

  <style>
    * {
      box-sizing: border-box;
    }

    html {
      color-scheme: light;
      background: #eef5ff;
    }

    body {
      margin: 0;
      font-family: Arial, Helvetica, sans-serif;
      background: linear-gradient(180deg, #eef5ff 0%, #f8fbff 100%);
      color: #0f172a;
    }

    .container {
      max-width: 430px;
      margin: 0 auto;
      padding: 18px 14px;
    }

    .main {
      background: #ffffff;
      border-radius: 26px;
      padding: 22px;
      box-shadow: 0 16px 35px rgba(15, 23, 42, 0.14);
      border: 1px solid #dbeafe;
    }

    .header {
      display: flex;
      gap: 14px;
      align-items: center;
      padding-bottom: 18px;
      border-bottom: 1px solid #e5e7eb;
      margin-bottom: 20px;
    }

    .logo {
      width: 68px;
      height: 68px;
      border-radius: 18px;
      background: #eff6ff;
      border: 2px solid #1e3a8a;
      display: flex;
      align-items: center;
      justify-content: center;
      color: #1e3a8a;
      font-size: 34px;
      font-weight: 900;
      flex-shrink: 0;
    }

    .title h1 {
      margin: 0;
      font-size: 23px;
      line-height: 1.1;
      color: #172554;
    }

    .subtitle {
      margin-top: 7px;
      font-size: 13px;
      line-height: 1.35;
      color: #475569;
    }

    .grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 14px;
      margin-bottom: 18px;
    }

    .sensor {
      background: #ffffff;
      border: 1px solid #e5e7eb;
      border-radius: 22px;
      padding: 16px;
      box-shadow: 0 8px 18px rgba(15, 23, 42, 0.08);
    }

    .sensorTop {
      display: flex;
      align-items: center;
      gap: 9px;
      margin-bottom: 12px;
    }

    .sensorIcon {
      width: 38px;
      height: 38px;
      border-radius: 50%;
      background: #dbeafe;
      display: flex;
      align-items: center;
      justify-content: center;
      color: #1d4ed8;
      font-weight: 900;
      flex-shrink: 0;
    }

    .sensorLabel {
      font-size: 13px;
      font-weight: bold;
      color: #1e3a8a;
    }

    .sensorValue {
      font-size: 34px;
      font-weight: 900;
      color: #0f172a;
      letter-spacing: -1px;
    }

    .unit {
      font-size: 17px;
      font-weight: 600;
      color: #334155;
    }

    .bar {
      height: 9px;
      border-radius: 999px;
      background: #dbeafe;
      margin-top: 14px;
      overflow: hidden;
    }

    .barFill {
      height: 100%;
      width: 72%;
      border-radius: 999px;
      background: #2563eb;
    }

    .status {
      display: flex;
      align-items: center;
      gap: 16px;
      background: {{FUNDO_STATUS}};
      border: 1px solid {{BORDA_STATUS}};
      border-radius: 22px;
      padding: 18px;
      margin-bottom: 18px;
    }

    .statusIcon {
      width: 58px;
      height: 58px;
      border-radius: 50%;
      background: rgba(255,255,255,0.75);
      border: 2px solid {{COR_STATUS}};
      display: flex;
      align-items: center;
      justify-content: center;
      font-size: 30px;
      font-weight: 900;
      color: {{COR_STATUS}};
      flex-shrink: 0;
    }

    .statusSmall {
      font-size: 13px;
      font-weight: bold;
      color: #334155;
      margin-bottom: 5px;
    }

    .statusText {
      font-size: 24px;
      font-weight: 900;
      color: {{COR_STATUS}};
    }

    .range, .footerBox {
      background: #ffffff;
      border: 1px solid #dbeafe;
      border-radius: 20px;
      padding: 16px;
      margin-bottom: 14px;
      box-shadow: 0 7px 16px rgba(15, 23, 42, 0.06);
    }

    .rangeTitle {
      font-size: 13px;
      font-weight: bold;
      color: #1e3a8a;
      margin-bottom: 6px;
    }

    .rangeValue {
      font-size: 21px;
      font-weight: 800;
      color: #0f172a;
    }

    .footerLine {
      font-size: 13px;
      color: #334155;
      margin: 8px 0;
    }

    .ip {
      color: #1d4ed8;
      font-weight: bold;
    }

    .project {
      margin-top: 14px;
      font-size: 11px;
      text-align: center;
      color: #64748b;
    }
  </style>
</head>

<body>
  <div class="container">
    <div class="main">

      <div class="header">
        <div class="logo">+</div>

        <div class="title">
          <h1>Mini Incubadora Portatil</h1>
          <div class="subtitle">
            Engenharia Biomedica<br>
            Universidade Federal de Uberlandia
          </div>
        </div>
      </div>

      <div class="grid">

        <div class="sensor">
          <div class="sensorTop">
            <div class="sensorIcon">T</div>
            <div class="sensorLabel">Temperatura</div>
          </div>

          <div class="sensorValue">{{TEMP}}<span class="unit"> &deg;C</span></div>
          <div class="bar"><div class="barFill"></div></div>
        </div>

        <div class="sensor">
          <div class="sensorTop">
            <div class="sensorIcon">U</div>
            <div class="sensorLabel">Umidade</div>
          </div>

          <div class="sensorValue">{{UMID}}<span class="unit"> %</span></div>
          <div class="bar"><div class="barFill"></div></div>
        </div>

      </div>

      <div class="status">
        <div class="statusIcon">{{ICONE_STATUS}}</div>
        <div>
          <div class="statusSmall">Status do sistema</div>
          <div class="statusText">{{TEXTO_STATUS}}</div>
        </div>
      </div>

      <div class="range">
        <div class="rangeTitle">Faixa de controle</div>
        <div class="rangeValue">{{TEMP_MIN}}&deg;C a {{TEMP_MAX}}&deg;C</div>
      </div>

      <div class="footerBox">
        <div class="footerLine">Monitoramento local via ESP32</div>
        <div class="footerLine">Atualizacao automatica a cada 3 segundos</div>
        <div class="footerLine">Endereco de acesso: <span class="ip">192.168.4.1</span></div>
      </div>

      <div class="project">Projeto academico - ESP32 + DHT22 + LCD I2C + LEDs</div>

    </div>
  </div>
</body>
</html>
)rawliteral";

  html.replace("{{TEMP}}", tempTexto);
  html.replace("{{UMID}}", umidTexto);
  html.replace("{{TEXTO_STATUS}}", textoStatus);
  html.replace("{{ICONE_STATUS}}", iconeStatus);
  html.replace("{{TEMP_MIN}}", String(tempMin, 1));
  html.replace("{{TEMP_MAX}}", String(tempMax, 1));
  html.replace("{{COR_STATUS}}", corStatus);
  html.replace("{{FUNDO_STATUS}}", fundoStatus);
  html.replace("{{BORDA_STATUS}}", bordaStatus);

  server.send(200, "text/html", html);
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);

  lcd.init();
  lcd.backlight();

  dht.begin();

  pinMode(LED_AQUECER, OUTPUT);
  pinMode(LED_ESFRIAR, OUTPUT);

  digitalWrite(LED_AQUECER, LOW);
  digitalWrite(LED_ESFRIAR, LOW);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Mini Incubadora");
  lcd.setCursor(0, 1);
  lcd.print("Iniciando...");
  delay(2000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Criando WiFi");
  lcd.setCursor(0, 1);
  lcd.print("Aguarde...");

  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();

  Serial.println("Wi-Fi criado!");
  Serial.print("Nome da rede: ");
  Serial.println(ssid);
  Serial.print("Senha: ");
  Serial.println(password);
  Serial.print("Acesse: ");
  Serial.println(IP);

  server.on("/", paginaPrincipal);
  server.begin();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi criado");
  lcd.setCursor(0, 1);
  lcd.print("192.168.4.1");
  delay(3000);

  lerDHT22();
  atualizarDisplay();
}

// ================= LOOP =================
void loop() {
  server.handleClient();

  unsigned long agora = millis();

  if (agora - tempoAnterior >= intervalo) {
    tempoAnterior = agora;

    lerDHT22();
    atualizarDisplay();
  }
}
