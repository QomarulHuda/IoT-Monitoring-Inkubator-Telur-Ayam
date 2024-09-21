#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <UniversalTelegramBot.h>
#include <DHT.h>
#include <ESP8266WiFi.h>

String apiKey = "R0QB6OHVPSIP15RN";
const char* ssid = "AndroidAP";
const char* pass = "mdny1577";
const char* server = "api.thingspeak.com";
const char* BOT_TOKEN = "7444569914:AAHkpqZI9HX6bny4eVjwtD2juSkP4FO9IsE";
const String chat_id = "765070180";  // Ganti dengan chat ID Anda yang benar

#define DHTPIN 13
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
#define KIPAS 0
#define SensorAir A0
#define POMPA 2 
#define PEMBALIK 5  //motor sinkron

const unsigned long BOT_MTBS = 1000; // mean time between scan messages
unsigned long bot_lasttime; // last time messages' scan has been done
WiFiClientSecure client; // Secure WiFi client
UniversalTelegramBot bot(BOT_TOKEN, client);

unsigned long lastTimeBotRan;

// Tambahkan variabel global untuk timer pembalikan otomatis
unsigned long lastTurnTime = 0;
const unsigned long turnInterval = 28800000;  // 8 jam dalam milidetik

void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(KIPAS, OUTPUT);
  pinMode(SensorAir, INPUT);
  pinMode(POMPA, OUTPUT);
  pinMode(PEMBALIK, OUTPUT);

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP Address: " + WiFi.localIP().toString());

  client.setInsecure(); 
}

// thingspeak
void loop() {
  float kelembaban = dht.readHumidity();
  float suhu = dht.readTemperature();
  int air = analogRead(SensorAir);

  if (isnan(kelembaban) || isnan(suhu) || isnan(air)) {
    Serial.println("Failed to read from sensor!");
    return;
  }

  // Logika pembalikan telur otomatis setiap 8 jam
  unsigned long currentMillis = millis();
  if (currentMillis - lastTurnTime >= turnInterval) {
    // Jalankan motor pembalik telur
    digitalWrite(PEMBALIK, HIGH);
    delay(5000);  // Motor aktif selama 5 detik untuk membalik telur
    digitalWrite(PEMBALIK, LOW);

    // Kirim notifikasi ke Telegram
    String message = "Telur sudah dibalik secara otomatis.";
    bot.sendMessage(chat_id, message, "");

    // Perbarui waktu terakhir pembalikan
    lastTurnTime = currentMillis;
  }

  // Logika Thingspeak dan pengontrolan kipas serta pompa
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    if (client.connect(server, 80)) {
      String postStr = apiKey;
      postStr += "&field1=" + String(suhu);
      postStr += "&field2=" + String(kelembaban);
      postStr += "&field3=" + String(air);
      postStr += "\r\n\r\n";

      client.print("POST /update HTTP/1.1\n");
      client.print("Host: api.thingspeak.com\n");
      client.print("Connection: close\n");
      client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
      client.print("Content-Type: application/x-www-form-urlencoded\n");
      client.print("Content-Length: ");
      client.print(postStr.length());
      client.print("\n\n");
      client.print(postStr);

      Serial.print("Suhu: ");
      Serial.print(suhu);
      Serial.print(" degrees Celsius, Kelembaban: ");
      Serial.print(kelembaban);
      Serial.print(" %, Air: ");
      Serial.print(air);
      Serial.println("%. Sent to Thingspeak.");
    }
    client.stop();
  }

  if (suhu > 37) {
    digitalWrite(KIPAS, LOW);
  } else {
    digitalWrite(KIPAS, HIGH);
  }

  if (air <= 100) {
    Serial.println("Volume Air: Habis");
    digitalWrite(POMPA, LOW);
  } else if (air >= 101 && air <= 200) {
    Serial.println("Volume Air: Kurang");
  } else if (air >= 201 && air <= 350) {
    Serial.println("Volume Air: Cukup");
  } else if (air > 400) {
    Serial.println("Volume Air: Penuh");
    digitalWrite(POMPA, HIGH);
  }

  // Bot Telegram
  if (millis() - lastTimeBotRan > BOT_MTBS) { 
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    lastTimeBotRan = millis();
  }
}

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String messageChatId = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;

    if (messageChatId == chat_id) { // Check chat ID
      if (text.equalsIgnoreCase("/start")) {
        String welcome = "Selamat datang di Bot Telegram untuk Monitoring Suhu dan Pembalik Telur Otomatis!\n";
        welcome += "Ketik /suhu untuk melihat suhu saat ini.\n";
        welcome += "Ketik /balik untuk membalikkan telur secara manual.\n";
        welcome += "Ketik /status untuk melihat status semua output dan input.\n";
        welcome += "Ketik /kipas_on untuk menyalakan kipas.\n";
        welcome += "Ketik /kipas_off untuk mematikan kipas.\n";
        welcome += "Ketik /pompa_on untuk menyalakan pompa.\n";
        welcome += "Ketik /pompa_off untuk mematikan pompa.\n";
        welcome += "Ketik /balik_telur untuk membalikkan telur menggunakan motor.\n";
        bot.sendMessage(chat_id, welcome, "");
      }
      if (text.equalsIgnoreCase("/suhu")) {
        float suhu = dht.readTemperature();
        float kelembaban = dht.readHumidity();
        bot.sendMessage(chat_id, "Suhu saat ini: " + String(suhu) + "°C\nKelembaban: " + String(kelembaban) + "%", "");
      }
      if (text.equalsIgnoreCase("/balik")) {
        digitalWrite(PEMBALIK, HIGH); // Aktifkan relay untuk motor
        delay(5000); // Jalankan motor selama 5 detik
        digitalWrite(PEMBALIK, LOW); // Matikan relay setelah 5 detik
        bot.sendMessage(chat_id, "Telur sudah dibalik.", "");
      }
      if (text.equalsIgnoreCase("/status")) {
        float suhu = dht.readTemperature();
        float kelembaban = dht.readHumidity();
        int air = analogRead(SensorAir);
        String kipasStatus = digitalRead(KIPAS) == LOW ? "Menyala" : "Mati";
        String pompaStatus = digitalRead(POMPA) == HIGH ? "Mati" : "Menyala";
        String status = "Suhu: " + String(suhu) + "°C\n";
        status += "Kelembaban: " + String(kelembaban) + "%\n";
        status += "Air: " + String(air) + "\n";
        status += "Kipas: " + kipasStatus + "\n";
        status += "Pompa: " + pompaStatus;
        bot.sendMessage(chat_id, status, "");
      }
      if (text.equalsIgnoreCase("/kipas_on")) {
        digitalWrite(KIPAS, LOW);
        bot.sendMessage(chat_id, "Kipas dinyalakan.", "");
      }
      if (text.equalsIgnoreCase("/kipas_off")) {
        digitalWrite(KIPAS, HIGH);
        bot.sendMessage(chat_id, "Kipas dimatikan.", "");
      }
      if (text.equalsIgnoreCase("/pompa_on")) {
        digitalWrite(POMPA, LOW);
        bot.sendMessage(chat_id, "Pompa dinyalakan.", "");
      }
      if (text.equalsIgnoreCase("/pompa_off")) {
        digitalWrite(POMPA, HIGH);
        bot.sendMessage(chat_id, "Pompa dimatikan.", "");
      }
    }
  }
}
