/*
 * D1 ピン確認テスト (XIAO nRF52840)
 * D1 を 2秒ごとに HIGH / LOW 切り替える。
 * テスターで D1〜GND を測ると 約3.3V ⇔ 0V と交互に変われば D1 は正常。
 * 基板LED: 緑 = HIGH中 / 赤 = LOW中
 */
void setup() {
  pinMode(D1, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
}

void loop() {
  digitalWrite(D1, HIGH);
  digitalWrite(LED_GREEN, LOW);  digitalWrite(LED_RED, HIGH);
  delay(2000);
  digitalWrite(D1, LOW);
  digitalWrite(LED_GREEN, HIGH); digitalWrite(LED_RED, LOW);
  delay(2000);
}
