#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
const int ledPin = 2;
const char *ssid = "Telia-8FAA41-Greitas";
const char *wifiPass = "9245DDF653";
const char *wifiCredentials[][2] = {
  // TODO: read environment variables
};
String apiEndpoint = "http://192.168.1.172:8080/api/tv";
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

struct View {
  int data[SCREEN_WIDTH * SCREEN_HEIGHT];
  int refreshAfter;
};

void get_next_view(View* view) {
  Serial.println("Fetching data from API");
  HTTPClient http;
  http.begin(apiEndpoint.c_str());
  const char *headerKeys[] = {"X-Refresh-After", "Content-Type", "Content-Length"};
  const size_t headerKeysCount = sizeof(headerKeys) / sizeof(headerKeys[0]);
  http.collectHeaders(headerKeys, headerKeysCount);

  int httpResCode = http.GET();

  if (httpResCode > 0) {
    Serial.print("HTTP response code: ");
    Serial.println(httpResCode);
    String response = http.getString();

    int len = response.length();
    if (len != SCREEN_HEIGHT * SCREEN_WIDTH) {
      Serial.print("Invalid data length: ");
      Serial.println(len);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Invalid data length");
      display.println(len);
      display.display();
      return;
    }
    
    for (int i = 0; i < len; i++) {
      (*view).data[i] = response[i];
    }

    (*view).refreshAfter = http.header("X-Refresh-After").toInt();
    if ((*view).refreshAfter == 0) {
      (*view).refreshAfter = 5000;
    }
  } else {
    Serial.print("Error on HTTP request, code: ");
    Serial.println(httpResCode);
  }

  http.end();
}

void connect_wifi() {
  int numNetworks = sizeof(wifiCredentials) / sizeof(wifiCredentials[0]);
  for (int i = 0; i < numNetworks; i++) {
    WiFi.begin(wifiCredentials[i][0], wifiCredentials[i][1]);
    Serial.print("Connecting to ");
    Serial.println(wifiCredentials[i][0]);

    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Connecting to ");
    display.print(wifiCredentials[i][0]);
    display.display();

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      display.print(".");
      display.display();
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("");
      Serial.println("Connected to WiFi");

      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Connected to WiFi");
      display.display();
      return;
    } else {
      Serial.println("");
      Serial.print("Failed to connect to ");
      Serial.println(wifiCredentials[i][0]);

      display.clearDisplay();
      display.setCursor(0, 0);
      display.print("Failed to connect to ");
      display.print(wifiCredentials[i][0]);
      display.display();
      delay(2000);
    }
  }

  Serial.println("Could not connect to any WiFi network");

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Could not connect to");
  display.println("any WiFi network");
  display.display();
}

void display_stats(int data[]) {
  display.clearDisplay();

  for (int i = 0; i < SCREEN_HEIGHT; i++) {
    for (int j = 0; j < SCREEN_WIDTH; j++) {
      display.drawPixel(j, i, data[i * SCREEN_WIDTH + j]-48); // -48 to convert from ASCII to int
    }
  }
  
  display.display();
}

void setup() {
  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }

  display.clearDisplay();
  delay(500);

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Loading...");
  display.display();

  connect_wifi();

  delay(1000);
}

View* view = new View();
void loop() {
  get_next_view(view);
  display_stats((*view).data);
  delay((*view).refreshAfter);
}
