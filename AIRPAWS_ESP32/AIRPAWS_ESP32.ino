/*===========================================================================================//
  Projto: AIR PAWS
  Autor Cód.: Chagas Junior
  Data: 17/07/2024
  Versão: 1.2
//==========================================================================================*/

#pragma region ESCOPO
//==================================== Bibliotecas =========================================//
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <UrlEncode.h>
#include <ESPmDNS.h>
//==========================================================================================//

//=================================== Definições LCD ========================================//
#define address 0x27
#define coluns 16
#define rows 2

LiquidCrystal_I2C lcd(address, coluns, rows);
//==========================================================================================//

//=================================== Definições DHT ========================================//
#define DHTPIN 4
#define DHTTYPE DHT11
float temp = 0;
float umid = 0;

DHT dht(DHTPIN, DHTTYPE);
//===========================================================================================//

//=================================== Configuração WiFi =====================================//
#define WiFIMode WIFI_STA
#define SSID "AIR PAWS"
bool connected = false;

WiFiManager wifi;
//===========================================================================================//

//================================= Definições Web Page =====================================//
#define portWeb 80

WebServer server(portWeb);
//===========================================================================================//

//========================== Definições Caracteres Especiais ================================//
byte grau[8] = {
  0b00000,
  0b01110,
  0b01010,
  0b01110,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};
byte gota[8] = {
  0b00100,
  0b00100,
  0b01010,
  0b01010,
  0b10001,
  0b10001,
  0b01110,
  0b00000
};
byte termometro[8] = {
  0b00100,
  0b01010,
  0b01010,
  0b01010,
  0b10001,
  0b10001,
  0b11111,
  0b01110
};
byte heart[8] = {
  0b00000,
  0b01010,
  0b11111,
  0b11111,
  0b01110,
  0b00100,
  0b00000,
  0b00000
};
//===========================================================================================//

//================================ Variáveis Auxiliares =====================================//
#define ledRed 13
#define ledGreen 12
#define pot A0
#define buzz 14

int ledRedState = 0;
int buzzerState = 0;
int ledGreenState = 0;
int potValue = 0;
int batimentos = 0;
bool alarmB = 1;

// Simulated GPS coordinates
int indexprevious = 0;
int ind = 0;
float latitude;
float longitude;
String airport;

// Matriz de coordenadas dos aeroportos
float airports[3][2] = {
  { -23.4356, -46.4731 },  // Aeroporto de São Paulo (GRU)
  { -22.9109, -43.1633 },  // Aeroporto do Rio de Janeiro (GIG)
  { -3.7763, -38.5326 }    // Aeroporto de Fortaleza (FOR)
};
String airportsNames[3] = {
  "Aeroporto de Guarulios - SP",
  "Aeroporto Santos Dumont - RJ",
  "Aeroporto Pinto Martins - FOR"
};
unsigned long lastUpdate = 0;

String phoneNumber = "+558585288335";
String apiKey = "8151819";
//==========================================================================================//

#pragma endregion

#pragma region Metodos
//================================== Métodos Principais ====================================//
void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  lcd.home();
  lcd.createChar(0, grau);
  lcd.createChar(1, termometro);
  lcd.createChar(2, gota);
  lcd.createChar(3, heart);

  dht.begin();

  pinMode(ledGreen, OUTPUT);
  pinMode(ledRed, OUTPUT);

  Starting();
}
void loop() {
  if ((WiFi.status() != WL_CONNECTED)) Reconnection();
  if (!SensoresRead()) return;
  SensoresScreem();
  Alarms();
  server.handleClient();
  //if (ledGreenState ) updateCoordinates();
  // if (millis() - lastUpdate >= 10000) {  // Atualiza a cada 10 segundos
  //   updateCoordinates();
  //   lastUpdate = millis();

  // }
}
//==========================================================================================//

//================================ Métodos Inicialização ===================================//
void Starting() {
  lcd.clear();
  lcd.print("Iniciado AIRPAWS");
  lcd.setCursor(0, 1);

  for (int i = 0; i < 16; i++) {
    lcd.write(255);
    delay(250);
  }

  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Bem-vindo ao");
  lcd.setCursor(4, 1);
  lcd.print("AIR PAWS");
  delay(2000);

  ConnectingWifi();
}
void ConnectingWifi() {
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("Rede WIFI:");
  lcd.setCursor(4, 1);
  lcd.print(SSID);

  connected = wifi.autoConnect(SSID);

  if (!connected) ESP.restart();

  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("Rede WIFI:");
  lcd.setCursor(2, 1);
  lcd.print("CONECTADO!!!");
  delay(2000);
  ledGreenState = 1;
  ServerInit();
}
void Reconnection() {
  connected = false;
  ledGreenState = connected ? 1 : 0;
  digitalWrite(ledGreen, ledGreenState);
  lcd.clear();
  lcd.noBlink();
  lcd.setCursor(4, 0);
  lcd.print("REDE WIFI");
  lcd.setCursor(4, 1);
  lcd.print("PERDIDA!!");
  delay(2000);

  lcd.clear();
  lcd.print("Reconectando!");

  WiFi.begin(wifi.getWiFiSSID(), wifi.getWiFiPass());
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {  // Espera a conexão WiFi ser estabelecida
    lcd.setCursor(i, 1);
    lcd.write(255);
    delay(250);
    i++;
    if (i > 15) {
      i = 0;
      lcd.clear();
      lcd.print("Reconectando!");
    }
  }

  connected = true;
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("Rede WIFI:");
  lcd.setCursor(2, 1);
  lcd.print("CONECTADO!!!");
  delay(2000);
  ServerInit();
}
//==========================================================================================//

//============================ Métodos Leitura e Mensagens =================================//
bool SensoresRead() {
  umid = dht.readHumidity();
  temp = dht.readTemperature();
  potValue = analogRead(pot);
  batimentos = map(potValue, 0, 4095, 0, 200);

  if (isnan(umid) || isnan(temp)) {
    lcd.clear();
    Serial.println("Falha no sensor!!!");
    lcd.setCursor(0, 0);
    lcd.print("Falha no sensor!!!");
    delay(100);
    return false;
  }

  return true;
}
void SensoresScreem() {
  lcd.clear();
  lcd.write(2);
  lcd.print(umid);
  lcd.print("%");

  lcd.print(" ");
  lcd.write(1);
  lcd.print(temp);
  lcd.write(0);
  lcd.print("C");

  lcd.setCursor(4, 1);
  lcd.write(3);
  lcd.print(batimentos);
  lcd.print("bpm");
  lcd.setCursor(4, 1);
  lcd.blink();
  delay(500);
}
void Alarms() {
  if (batimentos <= 50 || batimentos >= 160) {
    ledRedState = !ledRedState;
    buzzerState = buzzerState == 500 ? 0 : 500;

    if (alarmB) {
      alarmB = 0;
      String message = "";
      message += "*Informações do seu Pet Atualizadas (Alerta):* \n";
      message += "*Status:* Batimentos alterados!!!\n\n";
      message += "*Batimentos: " + String(batimentos) + " bpm*\n";
      message += "*Temperatura:* " + String(temp) + " °C\n";
      message += "*Umidade:* " + String(umid) + " %\n";
      message += "*Localização:* " + airport;
      sendMessage(message);
    }

  } else {
    ledRedState = LOW;
    buzzerState = 0;
    alarmB = 1;
  }
  digitalWrite(ledRed, ledRedState);
  tone(buzz, buzzerState);

  digitalWrite(ledGreen, ledGreenState);
}
void sendMessage(String message) {

  // Data to send with HTTP POST
  String url = "https://api.callmebot.com/whatsapp.php?phone=" + phoneNumber + "&apikey=" + apiKey + "&text=" + urlEncode(message);
  HTTPClient http;
  http.begin(url);

  // Specify content-type header
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  // Send HTTP POST request
  int httpResponseCode = http.POST(url);
  if (httpResponseCode == 200) {
    Serial.print("Mensagem enviada com sucesso");
  } else {
    Serial.println("Erro no envio da mensagem");
    Serial.print("HTTP response code: ");
    Serial.println(httpResponseCode);
  }

  // Free resources
  http.end();
}
//==========================================================================================//

//============================ Métodos Atualização Coordenadas =============================//
void updateCoordinates() {
  server.send(200, "text/html", "LOC");
  if (ledGreenState) {
    while (ind == indexprevious) {
      ind = random(0, 3);  // Seleciona aleatoriamente um dos três aeroportos
    }
    indexprevious = ind;
    latitude = airports[ind][0];
    longitude = airports[ind][1];
    airport = airportsNames[ind];

    String message = "";
    message += "*Informações do seu Pet Atualizadas: (Localização)*\n\n";
    message += "*Batimentos:* " + String(batimentos) + " bpm\n";
    message += "*Temperatura:* " + String(temp) + " °C\n";
    message += "*Umidade:* " + String(umid) + " %\n";
    message += "*Localização: " + airport + "*";
    sendMessage(message);

  } else {
    latitude = 0;
    longitude = 0;
    airport = "GPS Desligado";
  }
  server.sendHeader("Location", "/");
  server.send(303);
}
//==========================================================================================//

//=================================== Métodos Web Page =====================================//
void ServerInit() {
    String nameSever = "airpaws";
    if (!MDNS.begin(nameSever)) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  MDNS.addService("http", "tcp", 80);
  String m = "Para acomanhar via *navegador* use o seguinte link: http:// " + nameSever + ".local";
  sendMessage(m);

  SensoresRead();
  updateCoordinates();
  
  server.on("/", handleRoot);  // Define o manipulador para a URL raiz
  server.on("/gps", handleGPS);
  server.on("/loc", updateCoordinates);
  server.begin();  // Inicia o servidor web
  Serial.println("Servidor HTTP iniciado.");
}
String WebPage() {
  String webPage = "<!DOCTYPE html><html>";  // Inicia o HTML da página web
  webPage += "<head><title>Informações da Caixa de Transporte e do Pet</title>";
  webPage += "<meta charset='UTF-8' http-equiv='refresh' content='3'>";  // Atualiza a página a cada 3 segundos
  webPage += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  webPage += "<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0-beta3/css/all.min.css'>";
  webPage += "<style>";  // CSS para estilização dos elementos
  webPage += "body {font-family: Arial, sans-serif; margin: 0; padding: 0; display: flex; justify-content: center; align-items: center; height: 100vh; background-color: #f5f5f5;}";
  webPage += ".container {background-color: #fff; padding: 50px; border-radius: 10px; box-shadow: 0 0 10px rgba(0, 0, 0, 0.1); width: 400px; text-align: center;}";
  webPage += ".card {background-color: #007bff; padding: 35px; border-radius: 10px; box-shadow: 0 0 10px rgba(0, 0, 0, 0.1); margin-bottom: 40px; color: #fff; text-align: left;}";
  webPage += "h1 {font-size: 25px; margin-bottom: 10px;}";
  webPage += "p {font-size: 18px; margin: 2px 0;}";
  webPage += ".icon {margin-right: 5px;}";
  webPage += "@media screen and (max-width: 600px) { .container {margin: 10px;}}";
  webPage += "</style></head><body>";
  webPage += "<div class='container'>";
  webPage += "<h1>Informações do Animal</h1>";  // Novo card com informações do animal
  webPage += "<div class='card'>";
  webPage += "<p>Nome do Animal: Rex</p>";  // Exemplo de nome do animal
  webPage += "<p>Espécie: Cachorro</p>";    // Exemplo de espécie
  webPage += "<p>Raça: Labrador</p>";       // Exemplo de raça
  webPage += "<p>Idade: 5 anos</p>";        // Exemplo de idade
  webPage += "<p>Peso: 20 kg</p>";          // Exemplo de peso
  webPage += "</div>";
  webPage += "<h1>Informações de Status</h1>";
  webPage += "<div class='card'>";
  webPage += "<p><i class='fas fa-thermometer-half icon'></i>Temperatura: " + String(temp) + " °C</p>";
  webPage += "<p><i class='fas fa-tint icon'></i>Umidade: " + String(umid) + " %</p>";
  webPage += "<p><i class='fas fa-heart icon'></i>Batimentos: " + String(batimentos) + " bpm</p>";
  webPage += "</div>";
  webPage += "<h1>Localização</h1>";
  webPage += "<div class='card'>";
  webPage += "<p><i class='fas fa-map-marker-alt icon'></i>Lati: " + String(latitude, 6) + "°</p>";
  webPage += "<p><i class='fas fa-map-marker-alt icon'></i>Long: " + String(longitude, 6) + "°</p>";
  webPage += "<p><i class='fas fa-map-marker-alt icon'></i>Localização: " + String(airport) + "</p>";
  webPage += "</div>";
  webPage += "</div></body></html>";

  return webPage;

  return webPage;
}
void handleRoot() {
  if (!SensoresRead()) return;               // Lê os dados do sensor DHT e verifica erros
  server.send(200, "text/html", WebPage());  // Envia a página web ao cliente
  Serial.println("Página web enviada.");
}
void handleGPS() {
  server.send(200, "text/html", "GPS");
  ledGreenState = !ledGreenState;
  server.sendHeader("Location", "/");
  server.send(303);
}
//==========================================================================================//
#pragma endregion
