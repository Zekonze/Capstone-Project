#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <MQ2_LPG.h>
#include <ESP32Servo.h>
#include "esp_system.h"

// ================= WIFI & TELEGRAM =================
const char* ssid = "Infinix NOTE 30";
const char* password = "06082004";

#define BOT_TOKEN "8972787724:AAFkIt2rolpB9sJxZ_IlvBz7u-mAw7n8Z3s"
#define CHAT_ID   "5046912323"

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

unsigned long bot_lasttime = 0;
const unsigned long BOT_INTERVAL = 1000;

// ================= PIN =================
#define MQ2_PIN       0
#define PIR_PIN       2

#define LED_HIJAU     3
#define LED_KUNING    4
#define LED_MERAH     5

#define BUZZER_PIN    6
#define SERVO_PIN     7

// ================= MQ2 CONFIG =================
#define RL_Value   2022.0
#define Ro_Value   5.46
#define Volt_Value 5.0
#define ADC_Value  4095.0

#define x1_Value 199.150007852152
#define x2_Value 797.3322752256328
#define y1_Value 1.664988323698715
#define y2_Value 0.8990240080541785
#define x_Value 497.4177875376839
#define y_Value 1.0876679972710004

#define GAS_DANGER_ON   3000.0
#define GAS_SAFE_RESET  2500.0

MQ2Sensor mq2(MQ2_PIN);
Servo servo1;

bool servoSudahGerak = false;
bool telegramSudahKirim = false;
bool gasDanger = false;
bool motionDetected = false;

float ppm = 0;

// ================= TELEGRAM =================
void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String text = bot.messages[i].text;
    String chat_id = bot.messages[i].chat_id;

    if (chat_id != CHAT_ID) {
      bot.sendMessage(chat_id, "Akses ditolak.", "");
      continue;
    }

    if (text == "/start") {
      bot.sendMessage(chat_id,
        "Bot Gas Monitoring aktif.\n\nPerintah:\n/status\n/reset",
        ""
      );
    }

    else if (text == "/status") {
      String msg = "STATUS SISTEM\n";
      msg += "LPG Level: " + String(ppm) + " PPM\n";
      msg += "MQ2: ";
      msg += gasDanger ? "DANGER\n" : "SAFE\n";
      msg += "PIR: ";
      msg += motionDetected ? "Gerakan terdeteksi\n" : "Tidak ada gerakan\n";
      msg += "Servo: ";
      msg += servoSudahGerak ? "Valve tertutup\n" : "Valve terbuka/posisi awal\n";

      bot.sendMessage(chat_id, msg, "");
    }

    else if (text == "/reset") {
      ppm = mq2.readGas();

      if (ppm <= GAS_SAFE_RESET) {
        servo1.write(0);   // reset ke posisi awal
        delay(500);

        servoSudahGerak = false;
        telegramSudahKirim = false;
        gasDanger = false;

        digitalWrite(BUZZER_PIN, LOW);
        digitalWrite(LED_MERAH, LOW);
        digitalWrite(LED_KUNING, LOW);
        digitalWrite(LED_HIJAU, HIGH);

        String msg = "RESET BERHASIL\n";
        msg += "MQ2 sudah SAFE.\n";
        msg += "LPG Level: " + String(ppm) + " PPM\n";
        msg += "Servo kembali ke 0 derajat.";

        bot.sendMessage(chat_id, msg, "");
      } 
      else {
        String msg = "RESET DITOLAK!\n";
        msg += "Gas masih belum aman.\n";
        msg += "LPG Level: " + String(ppm) + " PPM\n";
        msg += "Servo tetap menutup valve.";

        bot.sendMessage(chat_id, msg, "");
      }
    }

    else {
      bot.sendMessage(chat_id, "Perintah tidak dikenal.\nGunakan:\n/status\n/reset", "");
    }
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  Serial.print("Reset reason: ");
  Serial.println(esp_reset_reason());

  pinMode(PIR_PIN, INPUT);
  pinMode(LED_HIJAU, OUTPUT);
  pinMode(LED_KUNING, OUTPUT);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(LED_HIJAU, LOW);
  digitalWrite(LED_KUNING, LOW);
  digitalWrite(LED_MERAH, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  servo1.attach(SERVO_PIN, 500, 2500);
  servo1.write(0);   // posisi awal / valve terbuka

  mq2.begin();
  mq2.setCalibration(
    RL_Value, Ro_Value, Volt_Value, ADC_Value,
    x_Value, x1_Value, x2_Value,
    y_Value, y1_Value, y2_Value
  );

  WiFi.begin(ssid, password);
  Serial.print("Menghubungkan WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi terhubung");

  client.setInsecure();

  Serial.println("Sistem siap.");
}

// ================= LOOP =================
void loop() {
  ppm = mq2.readGas();

  int gerak = digitalRead(PIR_PIN);
  motionDetected = (gerak == HIGH);

  if (ppm >= GAS_DANGER_ON) {
    gasDanger = true;
  } 
  else if (ppm <= GAS_SAFE_RESET) {
    gasDanger = false;
  }

  bool alarmAktif = gasDanger;

  if (millis() - bot_lasttime > BOT_INTERVAL) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    bot_lasttime = millis();
  }

  Serial.print("LPG Level: ");
  Serial.print(ppm);
  Serial.println(" PPM");

  Serial.print("MQ2: ");
  Serial.println(gasDanger ? "DANGER" : "SAFE");

  Serial.print("PIR: ");
  Serial.println(motionDetected ? "Gerakan Terdeteksi" : "Tidak Ada Gerakan");

  if (!gasDanger) {
    if (motionDetected) {
      digitalWrite(LED_HIJAU, HIGH);
      digitalWrite(LED_KUNING, HIGH);
      digitalWrite(LED_MERAH, LOW);
      digitalWrite(BUZZER_PIN, LOW);

      Serial.println("Status: GERAKAN TERDETEKSI");
    } 
    else {
      digitalWrite(LED_HIJAU, HIGH);
      digitalWrite(LED_KUNING, LOW);
      digitalWrite(LED_MERAH, LOW);
      digitalWrite(BUZZER_PIN, LOW);

      Serial.println("Status: AMAN");
    }
  }

  else if (alarmAktif) {
    digitalWrite(LED_HIJAU, LOW);
    digitalWrite(LED_KUNING, HIGH);
    digitalWrite(LED_MERAH, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);

    if (!servoSudahGerak) {
      servo1.write(180);   // danger: servo dari 0 ke 180
      delay(800);

      servoSudahGerak = true;

      Serial.println("Servo bergerak dari 0 ke 180 untuk menutup valve gas");
    }

    if (!telegramSudahKirim) {
      String pesan = "ALARM GAS AKTIF!\n";
      pesan += "Gas LPG terdeteksi DANGER.\n";
      pesan += "Level LPG: " + String(ppm) + " PPM\n";
      pesan += "Servo bergerak dari 0 ke 180 untuk menutup valve gas.\n\n";
      pesan += "Gunakan /status untuk cek sistem.\n";
      pesan += "Gunakan /reset jika gas sudah aman.";

      bot.sendMessage(CHAT_ID, pesan, "");

      telegramSudahKirim = true;
      Serial.println("Telegram alarm terkirim");
    }

    Serial.println("ALARM AKTIF");
  }

  Serial.println("---------------------------");
  delay(1000);
}