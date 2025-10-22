CODIGO MI WIFI

#include <DHT.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <BH1750.h>
#include <WiFi.h>
#include <FirebaseESP32.h>

// Provide the token generation process info.
#include <addons/TokenHelper.h>

// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

// ========== CONFIGURACIÓN WIFI Y FIREBASE ==========
#define WIFI_SSID "Aaaaa"
#define WIFI_PASSWORD "cacahuates"
#define API_KEY "AIzaSyC7Jr6cEzPTggaQjQvIEhIHwQUi962IIwM"
#define DATABASE_URL "https://mcei-5810e-default-rtdb.firebaseio.com"
#define USER_EMAIL "a22310388@ceti.mx"
#define USER_PASSWORD "uni56431"

// ========== CONFIGURACIÓN SENSORES ==========
#define DHT_PIN 4
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

// Crear objetos para sensores
Adafruit_BMP085 bmp;
BH1750 lightMeter;

// ========== CONFIGURACIÓN ACTUADORES ==========
#define RELAY_BOMBA 5
#define RELAY_VENTILADOR 19
#define RELAY_LUZ 2

// ========== VARIABLES SENSORES ==========
bool dhtAvailable = false;
bool bmpAvailable = false;
bool gy30Available = false;
uint8_t gy30Address = 0;

// ========== VARIABLES TIMING ==========
unsigned long previousMillis = 0;
const long interval = 2000;  // Intervalo lectura sensores (2 segundos)
unsigned long firebasePreviousMillis = 0;
const long firebaseInterval = 5000;  // Intervalo envío Firebase (5 segundos)

// ========== VARIABLES CONTROL ACTUADORES ==========
bool bombaState = false;
bool ventiladorState = false;
bool luzState = false;

// ========== UMBRALES AUTOMÁTICOS ==========
float humidityThreshold = 40.0;
float temperatureThreshold = 30.0;
float lightThreshold = 1000.0;

// ========== VARIABLES FIREBASE ==========
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ========== ESTRUCTURA PARA DATOS DE SENSORES ==========
struct SensorData {
  float temperatura;
  float humedad;
  float sensacionTermica;
  float presion;
  float altitud;
  float luz;
  bool bomba;
  bool ventilador;
  bool luzEstado;
  String timestamp;
};

SensorData currentData;

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando sistema de monitoreo con Firebase...");
  
  // Iniciar I2C
  Wire.begin();
  
  // Configurar pines de los relés
  pinMode(RELAY_BOMBA, OUTPUT);
  pinMode(RELAY_VENTILADOR, OUTPUT);
  pinMode(RELAY_LUZ, OUTPUT);
  
  // Iniciar todos los relés apagados
  digitalWrite(RELAY_BOMBA, LOW);
  digitalWrite(RELAY_VENTILADOR, LOW);
  digitalWrite(RELAY_LUZ, LOW);
  
  // Conectar WiFi
  conectarWiFi();
  
  // Configurar Firebase
  configurarFirebase();
  
  // Verificar sensores disponibles
  verificarSensores();
  
  Serial.println("✅ Sistema configurado correctamente");
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
    leerSensores();
    controlarActuadores();
    mostrarEstadoActual();
    Serial.println("==================================");
  }
  
  // Enviar datos a Firebase cada 5 segundos
  if (Firebase.ready() && (currentMillis - firebasePreviousMillis >= firebaseInterval)) {
    firebasePreviousMillis = currentMillis;
    enviarDatosFirebase();
  }
}

void conectarWiFi() {
  Serial.print("Conectando a WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 20) {
    delay(500);
    Serial.print(".");
    intentos++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("✅ Conectado a WiFi - IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("❌ Error conectando a WiFi");
  }
}

void configurarFirebase() {
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;
  
  Firebase.reconnectNetwork(true);
  fbdo.setBSSLBufferSize(4096, 1024);
  
  Firebase.begin(&config, &auth);
  Firebase.setDoubleDigits(5);
  
  Serial.println("Configurando Firebase...");
  delay(1000);
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

void leerSensores() {
  // Leer DHT11
  if (dhtAvailable) {
    currentData.temperatura = dht.readTemperature();
    currentData.humedad = dht.readHumidity();
    
    if (!isnan(currentData.temperatura) && !isnan(currentData.humedad)) {
      currentData.sensacionTermica = dht.computeHeatIndex(currentData.temperatura, currentData.humedad, false);
      
      Serial.println("=== SENSOR DHT11 ===");
      Serial.print("Temperatura: "); Serial.print(currentData.temperatura); Serial.println(" °C");
      Serial.print("Humedad: "); Serial.print(currentData.humedad); Serial.println(" %");
      Serial.print("Sensación térmica: "); Serial.print(currentData.sensacionTermica); Serial.println(" °C");
    } else {
      Serial.println("=== SENSOR DHT11 ===");
      Serial.println("Error en lectura");
    }
  }
  
  // Leer BMP180
  if (bmpAvailable) {
    currentData.presion = bmp.readPressure() / 100.0;
    currentData.altitud = bmp.readAltitude();
    
    if (!isnan(currentData.presion)) {
      Serial.println("=== SENSOR BMP180 ===");
      Serial.print("Presión: "); Serial.print(currentData.presion); Serial.println(" hPa");
      Serial.print("Altitud: "); Serial.print(currentData.altitud); Serial.println(" m");
    } else {
      Serial.println("=== SENSOR BMP180 ===");
      Serial.println("Error en lectura");
    }
  }
  
  // Leer GY-30
  if (gy30Available) {
    currentData.luz = lightMeter.readLightLevel();
    
    if (!isnan(currentData.luz) && currentData.luz >= 0) {
      Serial.println("=== SENSOR GY-30 ===");
      Serial.print("Luz: "); Serial.print(currentData.luz); Serial.println(" lx");
    } else {
      Serial.println("=== SENSOR GY-30 ===");
      Serial.println("Error en lectura");
    }
  }
}

void controlarActuadores() {
  // Controlar BOMBA (se activa cuando humedad baja)
  if (dhtAvailable && !isnan(currentData.humedad)) {
    bool nuevoEstadoBomba = (currentData.humedad < humidityThreshold);
    if (nuevoEstadoBomba != bombaState) {
      bombaState = nuevoEstadoBomba;
      digitalWrite(RELAY_BOMBA, bombaState ? HIGH : LOW);
      Serial.println("💧 BOMBA " + String(bombaState ? "ACTIVADA" : "DESACTIVADA"));
    }
    currentData.bomba = bombaState;
  }
  
  // Controlar VENTILADOR (se activa cuando temperatura alta)
  if (dhtAvailable && !isnan(currentData.temperatura)) {
    bool nuevoEstadoVentilador = (currentData.temperatura > temperatureThreshold);
    if (nuevoEstadoVentilador != ventiladorState) {
      ventiladorState = nuevoEstadoVentilador;
      digitalWrite(RELAY_VENTILADOR, ventiladorState ? HIGH : LOW);
      Serial.println("🌬️  VENTILADOR " + String(ventiladorState ? "ACTIVADO" : "DESACTIVADO"));
    }
    currentData.ventilador = ventiladorState;
  }
  
  // Controlar LUZ (se activa cuando luz baja)
  if (gy30Available && !isnan(currentData.luz) && currentData.luz >= 0) {
    bool nuevoEstadoLuz = (currentData.luz < lightThreshold);
    if (nuevoEstadoLuz != luzState) {
      luzState = nuevoEstadoLuz;
      digitalWrite(RELAY_LUZ, luzState ? HIGH : LOW);
      Serial.println("💡 LUZ " + String(luzState ? "ACTIVADA" : "DESACTIVADA"));
    }
    currentData.luzEstado = luzState;
  }
}

void enviarDatosFirebase() {
  Serial.println("📡 Enviando datos a Firebase...");
  
  // Obtener timestamp
  currentData.timestamp = String(millis());
  
  // Crear objeto JSON con los datos
  FirebaseJson json;
  
  // Agregar datos de sensores
  if (dhtAvailable) {
    json.set("sensores/temperatura", currentData.temperatura);
    json.set("sensores/humedad", currentData.humedad);
    json.set("sensores/sensacion_termica", currentData.sensacionTermica);
  }
  
  if (bmpAvailable) {
    json.set("sensores/presion", currentData.presion);
    json.set("sensores/altitud", currentData.altitud);
  }
  
  if (gy30Available) {
    json.set("sensores/luz", currentData.luz);
  }
  
  // Agregar estado de actuadores
  json.set("actuadores/bomba", currentData.bomba);
  json.set("actuadores/ventilador", currentData.ventilador);
  json.set("actuadores/luz", currentData.luzEstado);
  
  // Agregar umbrales
  json.set("umbrales/humedad", humidityThreshold);
  json.set("umbrales/temperatura", temperatureThreshold);
  json.set("umbrales/luz", lightThreshold);
  
  // Agregar timestamp
  json.set("timestamp", currentData.timestamp);
  json.set("timestamp_millis", millis());
  
  // Enviar datos a Firebase
  String path = "/invernadero/datos";
  if (Firebase.setJSON(fbdo, path, json)) {
    Serial.println("✅ Datos enviados correctamente a Firebase");
  } else {
    Serial.println("❌ Error enviando datos: " + fbdo.errorReason());
  }
  
  // También enviar datos individuales para facilitar el acceso
  enviarDatosIndividuales();
}

void enviarDatosIndividuales() {
  String basePath = "/invernadero/ultima_lectura";
  
  if (dhtAvailable) {
    Firebase.setFloat(fbdo, basePath + "/temperatura", currentData.temperatura);
    Firebase.setFloat(fbdo, basePath + "/humedad", currentData.humedad);
  }
  
  if (gy30Available) {
    Firebase.setFloat(fbdo, basePath + "/luz", currentData.luz);
  }
  
  Firebase.setBool(fbdo, basePath + "/bomba_activa", currentData.bomba);
  Firebase.setBool(fbdo, basePath + "/ventilador_activo", currentData.ventilador);
  Firebase.setBool(fbdo, basePath + "/luz_activa", currentData.luzEstado);
}

// ========== FUNCIONES EXISTENTES (sin cambios) ==========
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
  Serial.println("f - Forzar envío a Firebase");
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
    case 'f':
    case 'F':
      enviarDatosFirebase();
      break;
    default:
      Serial.println("Comando no reconocido. Presiona 'm' para ver el menú.");
  }
}

void cambiarUmbrales() {
  Serial.println("\n=== CAMBIAR UMBRALES AUTOMÁTICOS ===");
  
  if (Serial.available()) Serial.read();
  
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

void mostrarEstadoActual() {
  Serial.println("\n=== ESTADO ACTUAL ===");
  Serial.println("💧 BOMBA:    " + String(bombaState ? "ON" : "OFF"));
  Serial.println("🌬️  VENTILADOR: " + String(ventiladorState ? "ON" : "OFF"));
  Serial.println("💡 LUZ:     " + String(luzState ? "ON" : "OFF"));
  Serial.println("====================");
}