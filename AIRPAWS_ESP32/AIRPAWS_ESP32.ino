/*===========================================================================================//
  Projto: AIR PAWS
  Autor Cód.: Chagas Junior
  Data: 17/07/2024
  Versão: 0.1
//==========================================================================================*/
//==================================== Bibliotecas =========================================//
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
//==========================================================================================//

//=================================== Definiçõe LCD ========================================//
#define address 0x27
#define coluns 16
#define rows 2

LiquidCrystal_I2C lcd(address, coluns, rows);
//==========================================================================================//

//=================================== Definiçõe DHT ========================================//
#define DHTPIN 5
#define DHTTYPE DHT11
float temp = 0;
float umid = 0;

DHT dht(DHTPIN, DHTTYPE);
//==========================================================================================//

//========================== Definiçõe Caracteres Especiais ================================//
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
byte heartVoid[8] = {
  0b00000,
  0b01010,
  0b10101,
  0b10001,
  0b01010,
  0b00100,
  0b00000,
  0b00000
};
byte heartFull[8] = {
  0b00000,
  0b01010,
  0b11111,
  0b11111,
  0b01110,
  0b00100,
  0b00000,
  0b00000
};
//==========================================================================================//

//================================ Variáveis Auxiliares ====================================//
#define ledRed   2
#define ledGreen 4
#define pot A0

int ledRedState = 0;
int ledGreenState = 0;
int potValue = 0;
int batimentos = 0;
//==========================================================================================//

//================================== Métodos Principais ====================================//
void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  lcd.home();
  lcd.createChar(0, grau);
  lcd.createChar(1, termometro);
  lcd.createChar(2, gota);
  lcd.createChar(3, heartVoid);
  lcd.createChar(4, heartFull);

  dht.begin();

  pinMode(ledGreen, OUTPUT);
  pinMode(ledRed, OUTPUT);

  Starting();
}

void loop() {
  if (!SensoresRead()) return;
  LCD();
}
//==========================================================================================//

//================================== Métodos Auxiliares ====================================//
void Starting()
{
  lcd.clear();
  lcd.print("Iniciado AIRPAWS");
  lcd.setCursor(0,1);

  for(int i = 0; i < 16; i++)
  {
    lcd.write(255);
    delay(300);
  }

  lcd.clear();
  lcd.setCursor(2,0);
  lcd.print("Bem-vindo ao");
  lcd.setCursor(4,1);
  lcd.print("AIR PAWS");
  delay(2000);
}
bool SensoresRead() {
  umid = dht.readHumidity();
  temp = dht.readTemperature();
  potValue = analogRead(pot);
  batimentos = map(potValue, 0, 4095, 50, 130);

  if (isnan(umid) || isnan(temp)) {
    Serial.println("Falha no sensor!!!");
    lcd.setCursor(0, 0);
    lcd.print("Falha no sensor!!!");
    delay(100);
    return false;
  }

  return true;
}
void LCD()
{
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
  lcd.write(4);
  lcd.print(batimentos);
  lcd.print("bpm");
  delay(1000);
}
//==========================================================================================//