#include <MQ2_LPG.h>

#define MQ2_PIN     0
#define PIR_PIN     2
#define LED_PIN     3
#define BUZZER_PIN  4

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

void setup() {
  Serial.begin(115200);

  mq2.begin();
  mq2.setCalibration(
    RL_Value, Ro_Value, Volt_Value, ADC_Value,
    x_Value, x1_Value, x2_Value,
    y_Value, y1_Value, y2_Value
  );

  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.println("MQ2 + PIR Monitoring System");
  Serial.println("LED dan Buzzer ON hanya jika Gas Danger + Gerakan Terdeteksi");
}

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

  if (alarmAktif) {
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    Serial.println("⚠️ ALARM AKTIF! Gas Danger + Gerakan Terdeteksi!");
  } else {
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println("Alarm OFF");
  }

  Serial.println("---------------------------");
  delay(1000);
}