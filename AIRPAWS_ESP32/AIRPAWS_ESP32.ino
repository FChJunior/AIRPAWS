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
bool alarmL = 1;

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

String phoneNumber = "";
String apiKey = "";
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
    alarmL = 1;
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

    if (alarmL) {
      alarmL = 0;
      String message = "";
      message += "*Informações do seu Pet Atualizadas: (Localização)*\n\n";
      message += "*Localização perdida*";
      sendMessage(message);
    }
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

  server.on("/", handleRoot);  // Define o manipulador para a URL raiz
  server.on("/gps", handleGPS);
  server.on("/loc", updateCoordinates);
  server.on("/phone", HTTP_POST, phoneUpdate);
  server.on("/key", HTTP_POST, keyUpdate);
  server.begin();  // Inicia o servidor web
  Serial.println("Servidor HTTP iniciado.");

  while (phoneNumber == "" && apiKey == "") {
    lcd.clear();
    lcd.print("  Insira o tel.");
    lcd.setCursor(5, 1);
    lcd.print("do Tutor");
    server.handleClient();
    delay(500);
  }

  delay(500);
  String m = "Para acomanhar via *navegador* use o seguinte link: http:// " + nameSever + ".local";
  sendMessage(m);

  SensoresRead();
  updateCoordinates();
}
String WebPage() {
    String webPage = "<!DOCTYPE html><html lang='pt-BR'>";
    webPage += "<head><title>Informações da Caixa de Transporte e do Pet</title>";
    webPage += "<meta charset='UTF-8' http-equiv='refresh' content='3'>";
    webPage += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    webPage += "<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0-beta3/css/all.min.css'>";
    webPage += "<style>";
    webPage += "body {font-family: Arial, sans-serif; margin: 0; padding: 0; display: flex; justify-content: center; align-items: flex-start; min-height: 100vh; background: linear-gradient(135deg, #000000, #007bff); overflow-y: auto; padding-top: 60px;}";
    webPage += ".container {background-color: rgba(1, 1, 1, 0.9); padding: 30px; border-radius: 10px; box-shadow: 0 0 10px rgba(0, 0, 0, 0.3); width: 90%; max-width: 500px; text-align: center; margin-top: 20px;}";
    webPage += ".card {background-color: #007bff; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0, 0, 0, 0.2); margin-bottom: 30px; color: #fff; text-align: left; width: 100%; box-sizing: border-box;}";
    webPage += "h1 {font-size: 36px; margin-bottom: 20px; color: #007bff;}";
    webPage += "h2 {font-size: 22px; margin-bottom: 15px; color: #ffffff;}";
    webPage += "p {font-size: 16px; margin: 5px 0; color: #fff;}";
    webPage += ".icon {margin-right: 5px;}";
    webPage += ".form-group {margin-bottom: 15px; display: flex; justify-content: space-between; align-items: center;}";
    webPage += ".form-group label {flex: 1; text-align: right; padding-right: 10px; font-weight: bold;}";
    webPage += ".form-group input {flex: 2; padding: 10px; border: 1px solid #007bff; border-radius: 5px; background-color: #fff; color: #333; font-size: 16px;}";
    webPage += ".form-group input[readonly] {background-color: #e9ecef; border: 1px solid #007bff;}";
    webPage += "@media screen and (max-width: 600px) { .container { padding: 20px; } .form-group { flex-direction: column; align-items: flex-start; } .form-group label { text-align: left; padding-right: 0; margin-bottom: 5px; }}";
    webPage += "</style></head><body>";
    webPage += "<div class='container'>";
    webPage += "<h1>Air Paws</h1>";
    webPage += "<h2>Informações do Animal</h2>";
    webPage += "<div class='card'><form>";
    webPage += "<div class='form-group'><label for='animalName'><i class='fas fa-paw icon'></i>Nome do Animal:</label><input type='text' id='animalName' value='Rex' readonly></div>";
    webPage += "<div class='form-group'><label for='animalSpecies'><i class='fas fa-dog icon'></i>Espécie:</label><input type='text' id='animalSpecies' value='Cachorro' readonly></div>";
    webPage += "<div class='form-group'><label for='animalBreed'><i class='fas fa-paw icon'></i>Raça:</label><input type='text' id='animalBreed' value='Labrador' readonly></div>";
    webPage += "<div class='form-group'><label for='animalAge'><i class='fas fa-birthday-cake icon'></i>Idade:</label><input type='text' id='animalAge' value='5 anos' readonly></div>";
    webPage += "<div class='form-group'><label for='animalWeight'><i class='fas fa-weight icon'></i>Peso:</label><input type='text' id='animalWeight' value='20 kg' readonly></div>";
    webPage += "</form></div>";
    webPage += "<h2>Dados da Viagem</h2>";
    webPage += "<div class='card'><form>";
    webPage += "<div class='form-group'><label for='temperature'><i class='fas fa-thermometer-half icon'></i>Temperatura:</label><input type='text' id='temperature' value='" + String(temp) + " °C' readonly></div>";
    webPage += "<div class='form-group'><label for='humidity'><i class='fas fa-tint icon'></i>Umidade:</label><input type='text' id='humidity' value='" + String(umid) + " %' readonly></div>";
    webPage += "<div class='form-group'><label for='heartbeat'><i class='fas fa-heart icon'></i>Batimentos:</label><input type='text' id='heartbeat' value='" + String(batimentos) + " bpm' readonly></div>";
    webPage += "</form></div>";
    webPage += "<h2>Localização</h2>";
    webPage += "<div class='card'><form>";
    webPage += "<div class='form-group'><label for='latitude'><i class='fas fa-map-marker-alt icon'></i>Latitude:</label><input type='text' id='latitude' value='" + String(latitude, 6) + "°' readonly></div>";
    webPage += "<div class='form-group'><label for='longitude'><i class='fas fa-map-marker-alt icon'></i>Longitude:</label><input type='text' id='longitude' value='" + String(longitude, 6) + "°' readonly></div>";
    webPage += "<div class='form-group'><label for='location'><i class='fas fa-map-marker-alt icon'></i>Localização:</label><input type='text' id='location' value='" + String(airport) + "' readonly></div>";
    webPage += "</form></div></div></body></html>";
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
void phoneUpdate() {
  phoneNumber = server.arg(0);
  server.send(200, "text/plain", "Message received: " + phoneNumber);
}
void keyUpdate() {
  apiKey = server.arg(0);
  server.send(200, "text/plain", "Message received: " + apiKey);
}
//==========================================================================================//
#pragma endregion
