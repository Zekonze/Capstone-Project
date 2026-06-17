#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

#include <MQ2_LPG.h>
#include <ESP32Servo.h>

// ================= WIFI & TELEGRAM =================
const char* ssid = "rzl0806";
const char* password = "06082004";

#define BOT_TOKEN "8972787724:AAF15CH9uqvFt7eVdXIhN_r10SVuZcI6uko"
#define CHAT_ID   "5046912323"

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// ================= PIN =================
#define MQ2_PIN       0
#define PIR_PIN       2

#define LED_HIJAU     3
#define LED_KUNING    4
#define LED_MERAH     5

#define BUZZER_PIN    6

#define SERVO1_PIN    7
#define SERVO2_PIN    8

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

MQ2Sensor mq2(MQ2_PIN);

Servo servo1;
Servo servo2;

bool servoSudahGerak = false;
bool telegramSudahKirim = false;

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("Menghubungkan WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi terhubung");

  client.setInsecure();

  bot.sendMessage(CHAT_ID, "Sistem deteksi gas aktif.", "");

  mq2.begin();
  mq2.setCalibration(
    RL_Value, Ro_Value, Volt_Value, ADC_Value,
    x_Value, x1_Value, x2_Value,
    y_Value, y1_Value, y2_Value
  );

  pinMode(PIR_PIN, INPUT);

  pinMode(LED_HIJAU, OUTPUT);
  pinMode(LED_KUNING, OUTPUT);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);

  digitalWrite(LED_HIJAU, LOW);
  digitalWrite(LED_KUNING, LOW);
  digitalWrite(LED_MERAH, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  servo1.write(0);
  servo2.write(0);

  Serial.println("MQ2 + PIR + LED + Buzzer + Servo + Telegram System");
}

// ================= LOOP =================
void loop() {
  float ppm = mq2.readGas();
  int gerak = digitalRead(PIR_PIN);

  bool gasDanger = (ppm >= 3000.0);
  bool motionDetected = (gerak == HIGH);
  bool alarmAktif = (gasDanger && motionDetected);

  Serial.print("LPG Level: ");
  Serial.print(ppm);
  Serial.println(" PPM");

  Serial.print("Status MQ2: ");
  Serial.println(gasDanger ? "DANGER" : "SAFE");

  Serial.print("Status PIR: ");
  Serial.println(motionDetected ? "Gerakan Terdeteksi" : "Tidak Ada Gerakan");

  // ================= KONDISI AMAN =================
  if (!gasDanger) {
    digitalWrite(LED_HIJAU, HIGH);
    digitalWrite(LED_KUNING, LOW);
    digitalWrite(LED_MERAH, LOW);
    digitalWrite(BUZZER_PIN, LOW);

    Serial.println("Status: AMAN");
  }

  // ================= GAS DANGER SAJA =================
  else if (gasDanger && !motionDetected) {
    digitalWrite(LED_HIJAU, LOW);
    digitalWrite(LED_KUNING, HIGH);
    digitalWrite(LED_MERAH, LOW);
    digitalWrite(BUZZER_PIN, LOW);

    Serial.println("Status: GAS DANGER, tapi belum ada gerakan");
  }

  // ================= GAS + GERAKAN =================
  else if (alarmAktif) {
    digitalWrite(LED_HIJAU, LOW);
    digitalWrite(LED_KUNING, LOW);
    digitalWrite(LED_MERAH, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);

    if (!servoSudahGerak) {
      servo1.write(180);
      servo2.write(90);

      servoSudahGerak = true;

      Serial.println("Servo bergerak sekali untuk menutup valve gas");
    }

    if (!telegramSudahKirim) {
      String pesan = "⚠️ ALARM GAS AKTIF!\n";
      pesan += "Gas LPG terdeteksi DANGER.\n";
      pesan += "Gerakan juga terdeteksi oleh PIR.\n";
      pesan += "Level LPG: ";
      pesan += String(ppm);
      pesan += " PPM\n";
      pesan += "Servo sudah menutup valve gas.";

      bot.sendMessage(CHAT_ID, pesan, "");

      telegramSudahKirim = true;
      Serial.println("Notifikasi Telegram terkirim");
    }

    Serial.println("⚠️ ALARM AKTIF! Gas Danger + Gerakan Terdeteksi!");
  }

  Serial.println("---------------------------");
  delay(1000);
}