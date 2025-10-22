
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <BH1750.h>

// Definir pines y objetos para DHT11
#define DHT_PIN 4
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

// Crear objeto BMP180
Adafruit_BMP085 bmp;

// Crear objeto BH1750 (GY-30)
BH1750 lightMeter;

// Pines de los 3 relés para ESP32 - MODIFICADO
#define RELAY_BOMBA 5      // GPIO5 para bomba de agua
#define RELAY_VENTILADOR 19 // GPIO18 para ventilador
#define RELAY_LUZ 2        // GPIO19 para luz

// Variables para estado de sensores
bool dhtAvailable = false;
bool bmpAvailable = false;
bool gy30Available = false;
uint8_t gy30Address = 0;

// Variables para timing
unsigned long previousMillis = 0;
const long interval = 2000;  // Intervalo de 2 segundos

// Variables para control de actuadores
bool bombaState = false;
bool ventiladorState = false;
bool luzState = false;

// Umbrales para activación
float humidityThreshold = 4000.0;    // % humedad para activar bomba
float temperatureThreshold = 3.0; // °C para activar ventilador
float lightThreshold = 10000.0;     // lx para activar LED

void setup() {
  // Iniciar comunicación serial
  Serial.begin(115200);
  Serial.println("Iniciando sistema de sensores con 3 relés en ESP32...");
  
  // Iniciar I2C
  Wire.begin();
  
  // Configurar pines de los relés - MODIFICADO para ESP32
  pinMode(RELAY_BOMBA, OUTPUT);
  pinMode(RELAY_VENTILADOR, OUTPUT);
  pinMode(RELAY_LUZ, OUTPUT);
  
  // Iniciar todos los relés apagados
  digitalWrite(RELAY_BOMBA, LOW);
  digitalWrite(RELAY_VENTILADOR, LOW);
  digitalWrite(RELAY_LUZ, LOW);
  
  // Verificar sensores disponibles
  verificarSensores();
  
  Serial.println("✅ Sistema con 3 relés configurado en ESP32:");
  Serial.println("   - Bomba: GPIO " + String(RELAY_BOMBA));
  Serial.println("   - Ventilador: GPIO " + String(RELAY_VENTILADOR));
  Serial.println("   - Luz: GPIO " + String(RELAY_LUZ));
  
  // Mostrar menú de selección
  mostrarMenuActuadores();
  
  Serial.println("Sistema listo para leer sensores");
  Serial.println("==================================");
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Leer comandos seriales
  if (Serial.available()) {
    procesarComando(Serial.read());
  }
  
  // Leer sensores cada 2 segundos
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    
    // Leer solo los sensores disponibles
    if (dhtAvailable) {
      leerDHT11();
    } else {
      Serial.println("=== SENSOR DHT11 ===");
      Serial.println("Sensor no disponible");
    }
    
    if (bmpAvailable) {
      leerBMP180();
    } else {
      Serial.println("=== SENSOR BMP180 ===");
      Serial.println("Sensor no disponible");
    }
    
    if (gy30Available) {
      leerGY30();
    } else {
      Serial.println("=== SENSOR GY-30 (BH1750) ===");
      Serial.println("Sensor no disponible");
    }
    
    // Controlar todos los actuadores automáticamente
    controlarActuadores();
    
    // Mostrar estado actual
    mostrarEstadoActual();
    
    Serial.println("==================================");
  }
}

void verificarSensores() {
  // Verificar sensor DHT11
  Serial.print("Verificando sensor DHT11... ");
  dht.begin();
  delay(100);
  
  float testHumidity = dht.readHumidity();
  float testTemperature = dht.readTemperature();
  
  if (isnan(testHumidity) || isnan(testTemperature)) {
    Serial.println("NO DISPONIBLE");
    dhtAvailable = false;
  } else {
    Serial.println("OK");
    dhtAvailable = true;
  }
  
  // Verificar sensor BMP180
  Serial.print("Verificando sensor BMP180... ");
  if (!bmp.begin()) {
    Serial.println("NO DISPONIBLE");
    bmpAvailable = false;
  } else {
    Serial.println("OK");
    bmpAvailable = true;
  }
  
  // Verificar sensor GY-30 (BH1750)
  Serial.print("Verificando sensor GY-30 (BH1750)... ");
  gy30Available = verificarGY30();
  
  // Mostrar resumen de sensores disponibles
  Serial.println("\n=== RESUMEN DE SENSORES ===");
  Serial.print("DHT11: ");
  Serial.println(dhtAvailable ? "DISPONIBLE" : "NO DISPONIBLE");
  Serial.print("BMP180: ");
  Serial.println(bmpAvailable ? "DISPONIBLE" : "NO DISPONIBLE");
  Serial.print("GY-30: ");
  Serial.println(gy30Available ? "DISPONIBLE" : "NO DISPONIBLE");
}

bool verificarGY30() {
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23)) {
    gy30Address = 0x23;
    Serial.println("OK (Dirección 0x23)");
    return true;
  }
  
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x5C)) {
    gy30Address = 0x5C;
    Serial.println("OK (Dirección 0x5C)");
    return true;
  }
  
  Serial.println("NO DISPONIBLE");
  return false;
}

void mostrarMenuActuadores() {
  Serial.println("\n=== CONTROL DE 3 RELÉS ===");
  Serial.println("Comandos manuales:");
  Serial.println("b - Activar/Desactivar BOMBA manualmente");
  Serial.println("v - Activar/Desactivar VENTILADOR manualmente");
  Serial.println("l - Activar/Desactivar LUZ manualmente");
  Serial.println("a - Activar TODOS los relés");
  Serial.println("x - Apagar TODOS los relés");
  Serial.println("t - Cambiar umbrales automáticos");
  Serial.println("s - Mostrar estado actual");
  Serial.println("m - Mostrar este menú");
  Serial.println("==============================");
}

void procesarComando(char comando) {
  switch(comando) {
    case 'b':
    case 'B':
      bombaState = !bombaState;
      digitalWrite(RELAY_BOMBA, bombaState ? HIGH : LOW);
      Serial.println("🎛️  BOMBA " + String(bombaState ? "ACTIVADA" : "DESACTIVADA") + " manualmente");
      break;
    case 'v':
    case 'V':
      ventiladorState = !ventiladorState;
      digitalWrite(RELAY_VENTILADOR, ventiladorState ? HIGH : LOW);
      Serial.println("🎛️  VENTILADOR " + String(ventiladorState ? "ACTIVADO" : "DESACTIVADO") + " manualmente");
      break;
    case 'l':
    case 'L':
      luzState = !luzState;
      digitalWrite(RELAY_LUZ, luzState ? HIGH : LOW);
      Serial.println("🎛️  LUZ " + String(luzState ? "ACTIVADA" : "DESACTIVADA") + " manualmente");
      break;
    case 'a':
    case 'A':
      // Activar todos los relés
      bombaState = true;
      ventiladorState = true;
      luzState = true;
      digitalWrite(RELAY_BOMBA, HIGH);
      digitalWrite(RELAY_VENTILADOR, HIGH);
      digitalWrite(RELAY_LUZ, HIGH);
      Serial.println("🎛️  TODOS los relés ACTIVADOS");
      break;
    case 'x':
    case 'X':
      // Apagar todos los relés
      bombaState = false;
      ventiladorState = false;
      luzState = false;
      digitalWrite(RELAY_BOMBA, LOW);
      digitalWrite(RELAY_VENTILADOR, LOW);
      digitalWrite(RELAY_LUZ, LOW);
      Serial.println("🎛️  TODOS los relés APAGADOS");
      break;
    case 't':
    case 'T':
      cambiarUmbrales();
      break;
    case 's':
    case 'S':
      mostrarEstadoActual();
      break;
    case 'm':
    case 'M':
      mostrarMenuActuadores();
      break;
    default:
      Serial.println("Comando no reconocido. Presiona 'm' para ver el menú.");
  }
}

void cambiarUmbrales() {
  Serial.println("\n=== CAMBIAR UMBRALES AUTOMÁTICOS ===");
  
  if (Serial.available()) Serial.read(); // Limpiar buffer
  
  Serial.print("Umbral humedad actual ("); Serial.print(humidityThreshold); Serial.print("%) - Nuevo: ");
  while(!Serial.available()) delay(100);
  humidityThreshold = Serial.parseFloat();
  Serial.println(humidityThreshold);
  
  Serial.print("Umbral temperatura actual ("); Serial.print(temperatureThreshold); Serial.print("°C) - Nuevo: ");
  while(!Serial.available()) delay(100);
  temperatureThreshold = Serial.parseFloat();
  Serial.println(temperatureThreshold);
  
  Serial.print("Umbral luz actual ("); Serial.print(lightThreshold); Serial.print(" lx) - Nuevo: ");
  while(!Serial.available()) delay(100);
  lightThreshold = Serial.parseFloat();
  Serial.println(lightThreshold);
  
  Serial.println("Umbrales actualizados correctamente");
}

void controlarActuadores() {
  // Controlar BOMBA (se activa cuando humedad baja)
  if (dhtAvailable) {
    float humidity = dht.readHumidity();
    if (!isnan(humidity)) {
      bool nuevoEstadoBomba = (humidity < humidityThreshold);
      if (nuevoEstadoBomba != bombaState) {
        bombaState = nuevoEstadoBomba;
        digitalWrite(RELAY_BOMBA, bombaState ? HIGH : LOW);
        Serial.println("💧 BOMBA " + String(bombaState ? "ACTIVADA" : "DESACTIVADA") + " (Humedad: " + String(humidity) + "%)");
      }
    }
  }
  
  // Controlar VENTILADOR (se activa cuando temperatura alta)
  if (dhtAvailable) {
    float temperature = dht.readTemperature();
    if (!isnan(temperature)) {
      bool nuevoEstadoVentilador = (temperature > temperatureThreshold);
      if (nuevoEstadoVentilador != ventiladorState) {
        ventiladorState = nuevoEstadoVentilador;
        digitalWrite(RELAY_VENTILADOR, ventiladorState ? HIGH : LOW);
        Serial.println("🌬️  VENTILADOR " + String(ventiladorState ? "ACTIVADO" : "DESACTIVADO") + " (Temp: " + String(temperature) + "°C)");
      }
    }
  }
  
  // Controlar LUZ (se activa cuando luz baja)
  if (gy30Available) {
    float lux = lightMeter.readLightLevel();
    if (!isnan(lux) && lux >= 0) {
      bool nuevoEstadoLuz = (lux < lightThreshold);
      if (nuevoEstadoLuz != luzState) {
        luzState = nuevoEstadoLuz;
        digitalWrite(RELAY_LUZ, luzState ? HIGH : LOW);
        Serial.println("💡 LUZ " + String(luzState ? "ACTIVADA" : "DESACTIVADA") + " (Luz: " + String(lux) + " lx)");
      }
    }
  }
}

void mostrarEstadoActual() {
  Serial.println("\n=== ESTADO ACTUAL DE RELÉS ===");
  Serial.println("💧 BOMBA:    " + String(bombaState ? "ON" : "OFF") + " (Humedad < " + String(humidityThreshold) + "%)");
  Serial.println("🌬️  VENTILADOR: " + String(ventiladorState ? "ON" : "OFF") + " (Temp > " + String(temperatureThreshold) + "°C)");
  Serial.println("💡 LUZ:     " + String(luzState ? "ON" : "OFF") + " (Luz < " + String(lightThreshold) + " lx)");
  Serial.println("==============================");
}

void leerDHT11() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("=== SENSOR DHT11 ===");
    Serial.println("Error en lectura del sensor");
    return;
  }
  
  float heatIndex = dht.computeHeatIndex(temperature, humidity, false);
  
  Serial.println("=== SENSOR DHT11 (Temperatura/Humedad) ===");
  Serial.print("Humedad: "); Serial.print(humidity); Serial.println(" %");
  Serial.print("Temperatura: "); Serial.print(temperature); Serial.println(" °C");
  Serial.print("Sensación térmica: "); Serial.print(heatIndex); Serial.println(" °C");
  
  // Mostrar estado respecto a umbrales
  Serial.print("🔍 Humedad "); Serial.print(humidity < humidityThreshold ? "BAJA" : "OK");
  Serial.print(" (Umbral: "); Serial.print(humidityThreshold); Serial.println("%)");
  
  Serial.print("🔍 Temperatura "); Serial.print(temperature > temperatureThreshold ? "ALTA" : "OK");
  Serial.print(" (Umbral: "); Serial.print(temperatureThreshold); Serial.println("°C)");
}

void leerBMP180() {
  float temperatureBMP = bmp.readTemperature();
  float pressure = bmp.readPressure();
  float altitude = bmp.readAltitude();
  
  if (isnan(temperatureBMP) || isnan(pressure)) {
    Serial.println("=== SENSOR BMP180 ===");
    Serial.println("Error en lectura del sensor");
    return;
  }
  
  float pressureHPa = pressure / 100.0;
  
  Serial.println("=== SENSOR BMP180 (Presión/Altitud) ===");
  Serial.print("Presión: "); Serial.print(pressureHPa); Serial.println(" hPa");
  Serial.print("Altitud aproximada: "); Serial.print(altitude); Serial.println(" m");
}

void leerGY30() {
  float lux = lightMeter.readLightLevel();
  
  if (isnan(lux) || lux < 0) {
    Serial.println("=== SENSOR GY-30 (BH1750) ===");
    Serial.println("Error en lectura del sensor");
    return;
  }
  
  Serial.println("=== SENSOR GY-30 (BH1750 - Luz) ===");
  Serial.print("Intensidad luminosa: "); Serial.print(lux); Serial.println(" lx");
  
  // Mostrar estado respecto a umbral
  Serial.print("🔍 Luz "); Serial.print(lux < lightThreshold ? "BAJA" : "OK");
  Serial.print(" (Umbral: "); Serial.print(lightThreshold); Serial.println(" lx)");
}