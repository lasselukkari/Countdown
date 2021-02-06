#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <SSD1306Wire.h>

#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define COUNTDOWN_EPOCH 1669766400 // https://www.epochconverter.com/
#define DISPLAY_GEOMETRY GEOMETRY_64_48
#define ROW_HEIGHT 12
#define FONT ArialMT_Plain_10
#define DISPLAY_WIDTH 64
#define NTP_SERVER "time.facebook.com"
#define UPDATE_INTERVAL 3600000

SSD1306Wire display(0x3c, SDA, SCL, DISPLAY_GEOMETRY);
WiFiUDP udp;
IPAddress timeServerIP;
int ntpPacketSize  = 48;
byte packetBuffer[48];
unsigned long epochDelta;
unsigned long lastUpdate = 0;

void drawRow(int row, const char* title, int value) {
  char buffer [16] {};
  itoa(value, buffer, 10);

  int offset = row * ROW_HEIGHT;

  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, offset, title);
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(DISPLAY_WIDTH, offset, buffer);
}

void sendNTPpacket(IPAddress& address) {
  memset(packetBuffer, 0, ntpPacketSize);

  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  udp.beginPacket(address, 123);
  udp.write(packetBuffer, ntpPacketSize);
  udp.endPacket();
}

void updateTime() {
  WiFi.hostByName(NTP_SERVER, timeServerIP);
  sendNTPpacket(timeServerIP);
  delay(1000);

  if (!udp.parsePacket()) {
    return;
  }

  udp.read(packetBuffer, ntpPacketSize);
  unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
  unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
  unsigned long secsSince1900 = highWord << 16 | lowWord;
  unsigned long seventyYears = 2208988800UL;
  unsigned long now = millis();
  epochDelta = (secsSince1900 - seventyYears)  - (now / 1000);
  lastUpdate = now;
}

void updateDisplay() {
  unsigned long epoch = epochDelta +  (millis() / 1000);
  unsigned long delta = COUNTDOWN_EPOCH - epoch;
  int days = delta / (24 * 3600);
  delta = delta % (24 * 3600);
  int hours = delta / 3600;
  delta %= 3600;
  int minutes = delta / 60 ;
  delta %= 60;
  int seconds = delta;

  display.clear();
  drawRow(0, "Days:", days);
  drawRow(1, "Hours:", hours);
  drawRow(2, "Minutes:", minutes);
  drawRow(3, "Seconds:", seconds);
  display.display();
}

void setup() {
  display.init();
  display.setFont(FONT);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.clear();
  display.drawString(0, 0, "Connecting...");
  display.display();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  udp.begin(2390);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED && (!lastUpdate || (millis() - lastUpdate >= UPDATE_INTERVAL))) {
    updateTime();
  }

  updateDisplay();

  delay(100);
}
