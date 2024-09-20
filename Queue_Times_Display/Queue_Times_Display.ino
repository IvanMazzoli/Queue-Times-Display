#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans24pt7b.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiClientSecureBearSSL.h>
#include <ESP8266HTTPClient.h>

// Variabili WiFi
int maxRetries = 60;
WiFiClientSecure client;
String ssid = "WiFi_SSID";
String password = "WiFiPass";

// Variabili OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
char message[] = "STOP        ";
int x, minX;

// Variabili tempi d'attesa
int lastWaitingTime = -1;

void setup() {

  // Avvio la seriale
  Serial.begin(115200);
  delay(1000);
  Serial.println("[INFO] Queue Times Display Starting...");

  // Configuro l'aggiornamento OTA
  ArduinoOTA.setHostname("Queue Times Display OTA");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }
    Serial.println("[DEBUG] [OTA] Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("[DEBUG] [OTA] Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[ERROR] [OTA] Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("[ERROR] [OTA] Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("[ERROR] [OTA] Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("[ERROR] [OTA] Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("[ERROR] [OTA] Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("[ERROR] [OTA] End Failed");
    }
  });
  ArduinoOTA.begin();

  // Inizializzo il display (add. 0x3C)
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("[ERROR] [OLDED] SSD1306 allocation failed");
    return;
  }
  display.setRotation(2);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE, 0);

  // :)
  printDebugDisplay("Queue Times Display");
  display.println("IvanMazzoli - 2024");
  display.println();
  display.println("Powered By");
  display.println("Queue-Times.com");
  display.println();
  display.println("Github: @IvanMazzoli");
  display.display();
  delay(5000);

  // Provo a connettermi alla rete WiFi
  Serial.print("[INFO] [WIFI] Connecting to WiFi network " + ssid);
  printDebugDisplay("Connessione a WiFi...");
  display.println();
  display.println("Nome Rete:");
  display.println(ssid);
  display.println();
  display.print("Tempo rimanente: ");
  display.display();
  WiFi.begin(ssid, password);
  int retries = 0;
  int curX = display.getCursorX();
  int curY = display.getCursorY();
  while (WiFi.status() != WL_CONNECTED && retries < maxRetries) {
    Serial.print(".");
    display.setCursor(curX, curY);
    display.print(String(maxRetries - retries) + "s   ");
    display.display();
    retries++;
    delay(1000);
  }
  Serial.println(".");

  // Se ho raggiunto i max tentativi (secondi) torno in AP
  if (retries == maxRetries) {
    Serial.println("[ERROR] [WIFI] Connection to the WiFi network failed");

    // La rete esiste veramente?
    Serial.println();
    Serial.println("[DEBUG] [WIFI] Checking if the WiFi network is visible");
    bool ssidFound = false;
    int numSsid = WiFi.scanNetworks();
    for (int thisNet = 0; thisNet < numSsid; thisNet++) {
      String thisSSID = WiFi.SSID(thisNet);
      Serial.println(thisSSID);
      if (thisSSID.equals(ssid)) {
        ssidFound = true;
        break;
      }
    }
    if (ssidFound) {  // TODO: Pagina web per scegliere la rete e connettersi
      Serial.println("[DEBUG] [WIFI] WiFi network found, password wrong?");
      printDebugDisplay("Connessione fallita!");
      display.println();
      display.println("Controlla la password");
      display.println("del WiFi e riprova");
    } else {
      Serial.println("[DEBUG] [WIFI] WiFi network not found. Is the router ON?");
      printDebugDisplay("Connessione fallita!");
      display.println();
      display.println("Controlla nome della");
      display.println("rete WiFi e riprova");
      display.display();
    }
    while (true)
      yield();
  }

  // Connesso, stampo l'indirizzo IP
  Serial.print("[INFO] [WIFI] Connected to the WiFi with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println("[INFO] Startup completed!");
  client.setInsecure();

  // Completo il setup del display
  printDebugDisplay("Avvio completato!");
  display.setFont(&FreeSans24pt7b);
  x = display.width();
  minX = -12 * strlen(message);
}

void loop() {

  // Aggiorno i tempi d'attesa
  updateQueueTimes();

  // Handle degli aggiornamenti OTA
  ArduinoOTA.handle();
}

// Metodo per aggiornare i tempi d'attesa mostrati
void updateQueueTimes() {

  // Creo una richiesta all'endpoint
  // 12 = Gardaland
  HTTPClient http;
  http.begin(client, "https://queue-times.com/parks/12/queue_times.json");
  int httpCode = http.GET();

  // Se ho un codice minore o uguale a zero la connessione all'API è fallita
  if (httpCode <= 0) {
    http.end();
    Serial.print("[ERROR] [HTTP] HTTP GET request failed");
    delay(5000);
    updateQueueTimes();
    return;
  }

  // Controllo il codice HTTP ricevuto
  if (httpCode != HTTP_CODE_OK) {
    http.end();
    Serial.print("[ERROR] [HTTP] HTTP request failed with error code: ");
    Serial.println(httpCode);
    delay(5000);
    updateQueueTimes();
    return;
  }

  // Deserializzo il payload (json) ricevuto da QueueTimes
  String payload = http.getString();
  Serial.print("[DEBUG] [JSON] Received paylod from API: ");
  Serial.println(payload);
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    http.end();
    Serial.print("[ERROR] [JSON] Deserialization failed: ");
    Serial.println(error.c_str());
    delay(5000);
    updateQueueTimes();
    return;
  }

  // Ottengo i dati che mi servono
  // Lands = 0 (Adrenaline)
  // Rides = 3 (Raptor)
  JsonObject path = doc["lands"][0]["rides"][3];
  const char* rideName = path["name"];
  bool isOpen = path["is_open"];
  int waitTime = path["wait_time"];
  Serial.print("[DEBUG] [API] Ride Name: ");
  Serial.println(rideName);
  Serial.print("[DEBUG] [API] Is ride open? ");
  Serial.println(isOpen ? "Yes" : "No");
  Serial.print("[DEBUG] [API] Wait time: ");
  Serial.println(waitTime);

  // Chiudo la connessione
  http.end();

  // Mostro i dati scaricati
  int nextFetch = 1000 * 60;
  if (isOpen)
    displayWaitTime(waitTime, nextFetch);
  else
    displayStopMessage(nextFetch);
}

// Metodo per stampare a schermo il tempo d'attesa corrente
void displayWaitTime(int currentWait, int nextFetch) {

  // Se l'attesa è uguale a quella vecchia mi fermo
  if (currentWait == lastWaitingTime) {
    delay(nextFetch);
    return;
  }

  // Se l'attesa è 0, mostro 1 minuto
  if (currentWait == 0)
    currentWait = 1;

  // Calcola le dimensioni del testo
  display.clearDisplay();
  int16_t x1, y1;
  uint16_t w, h;
  char buffer[4];
  itoa(currentWait, buffer, 10);
  display.getTextBounds(buffer, 0, 0, &x1, &y1, &w, &h);

  // Imposta il cursore per centrare il testo
  int16_t x_c = (SCREEN_WIDTH - w) / 2;
  int16_t y = (SCREEN_HEIGHT + h) / 2;
  display.setCursor(x_c, y);
  display.print(buffer);
  display.display();

  // Aggiorno l'ultimo tempo mostrato
  lastWaitingTime = currentWait;
  delay(nextFetch);
}

// Metodo per mostrare "STOP" a schermo
void displayStopMessage(int nextFetch) {

  // Preparo le dimensioni
  display.clearDisplay();
  display.setFont(&FreeSans24pt7b);
  display.setTextSize(1);
  int16_t textWidth = display.width();
  int16_t textHeight = 24;

  // Scrollo il testo
  int time_delay = 0;
  while (time_delay < nextFetch / 5) {
    display.clearDisplay();
    display.setCursor(x, (SCREEN_HEIGHT + textHeight) / 2);
    display.print(message);
    display.display();
    x = x - 5;
    if (x < minX) {
      x = display.width();
      delay(1000);
      time_delay += 1000;
    }
    time_delay++;
    ArduinoOTA.handle();
  }

  // Resetto la posizione di X
  x = display.width();
}

// Metodo per pulire lo schermo e scrivere debug su display
void printDebugDisplay(String debugMsg) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(debugMsg);
  display.display();
}