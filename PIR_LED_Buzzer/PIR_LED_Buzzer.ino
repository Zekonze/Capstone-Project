#define PIR_PIN     2
#define LED_PIN     3
#define BUZZER_PIN  4

void setup() {
  Serial.begin(115200);

  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.println("Tes PIR dengan LED dan Buzzer");
  Serial.println("Tunggu kalibrasi PIR 30-60 detik...");
}

void loop() {
  int gerak = digitalRead(PIR_PIN);

  if (gerak == HIGH) {
    Serial.println("Gerakan terdeteksi!");

    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    Serial.println("Tidak ada gerakan");

    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  }

  delay(300);
}