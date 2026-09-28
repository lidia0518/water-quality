#include <SPI.h>
#include <SD.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include "RTClib.h"

// --- Configurações dos Pinos Livres ---
#define I2C_SDA 1
#define I2C_SCL 2
#define SD_CS   3
#define SD_SCK  4
#define SD_MOSI 5
#define SD_MISO 6
#define ONE_WIRE_BUS 7

// --- Instâncias dos Barramentos e Sensores ---
SPIClass spiSD(FSPI);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Se o RTC for o DS1307, mude a linha abaixo para: RTC_DS1307 rtc;
RTC_DS3231 rtc; 

// --- Variáveis de Controle de Tempo ---
unsigned long tempoAnterior = 0;
const unsigned long intervalo = 900000; // 15 minutos em milissegundos

void setup() {
  Serial.begin(115200);
  delay(1000);

  // 1. Inicializa o barramento I2C e o RTC
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!rtc.begin()) {
    Serial.println("Erro: Modulo RTC nao encontrado. Verifique a fiacao!");
  } else {
    // Se o RTC perdeu a bateria, ajusta para a data/hora em que o código foi compilado
    if (rtc.lostPower()) {
      Serial.println("RTC perdeu energia, ajustando o horario...");
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
  }

  // 2. Inicializa o Sensor de Temperatura
  Serial.println("Iniciando sensor DS18B20...");
  sensors.begin();

  // 3. Inicializa o Cartão SD
  Serial.println("Iniciando montagem do SD Card...");
  spiSD.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS, spiSD)) {
    Serial.println("Falha ao montar o cartão SD.");
    return;
  }
  
  // 4. Cria o cabeçalho do arquivo CSV
  File dataFile = SD.open("/datalog.csv", FILE_APPEND);
  if (dataFile) {
    dataFile.println("Data,Hora,Temperatura_C");
    dataFile.close();
    Serial.println("Sistema ok! Iniciando gravacao a cada 15m...");
  }
}

void loop() {
  unsigned long tempoAtual = millis();

  if (tempoAtual - tempoAnterior >= intervalo) {
    tempoAnterior = tempoAtual; 

    // Lê a temperatura
    sensors.requestTemperatures();
    float tempC = sensors.getTempCByIndex(0);

    // Lê o horário do RTC
    DateTime now = rtc.now();

    if (tempC != DEVICE_DISCONNECTED_C) {
      File dataFile = SD.open("/datalog.csv", FILE_APPEND);
      
      if (dataFile) {
        // Grava Data (DD/MM/YYYY)
        dataFile.print(now.day(), DEC); dataFile.print('/');
        dataFile.print(now.month(), DEC); dataFile.print('/');
        dataFile.print(now.year(), DEC); dataFile.print(",");
        
        // Grava Hora (HH:MM:SS)
        dataFile.print(now.hour(), DEC); dataFile.print(':');
        dataFile.print(now.minute(), DEC); dataFile.print(':');
        dataFile.print(now.second(), DEC); dataFile.print(",");
        
        // Grava Temperatura
        dataFile.println(tempC);
        
        dataFile.close();
        
        // Feedback no monitor serial
        Serial.printf("Salvo: %02d/%02d/%d %02d:%02d:%02d -> %.2f C\n", 
                      now.day(), now.month(), now.year(), 
                      now.hour(), now.minute(), now.second(), tempC);
      } else {
        Serial.println("Erro ao abrir datalog.csv para gravacao.");
      }
    } else {
      Serial.println("Erro: Falha na leitura do sensor de temperatura.");
    }
  }
}