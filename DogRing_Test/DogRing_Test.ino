/*
 * DogRing 動作確認用テスト (XIAO nRF52840)
 *
 * ・基板のLEDが 赤→緑→青 と1秒ごとに変わる  … プログラムが動いている証拠
 * ・D2 を常に HIGH (D4184 を ON のまま)
 * ・LEDテープの先頭5個を 赤→緑→青 と変える
 * ・シリアルモニタ (115200bps) に電池電圧と状態を表示
 * ・信号を常に送り続けるので、テスターで D1〜GND を測ると 0.3〜1.5V 程度になる
 *   (0V のままなら信号が出ていない)
 */
#include <Adafruit_NeoPixel.h>

const uint8_t PIN_STRIP     = D1;
const uint8_t PIN_STRIP_PWR = D2;
const uint16_t NUM_TEST     = 5;

Adafruit_NeoPixel strip(NUM_TEST, PIN_STRIP, NEO_GRB + NEO_KHZ800);

float readBattery() {
  pinMode(VBAT_ENABLE, OUTPUT);
  digitalWrite(VBAT_ENABLE, LOW);
  analogReference(AR_INTERNAL_2_4);
  analogReadResolution(12);
  uint32_t sum = 0;
  for (int i = 0; i < 8; i++) sum += analogRead(PIN_VBAT);
  analogReference(AR_DEFAULT);
  return (sum / 8.0f) / 4095.0f * 2.4f * (1510.0f / 510.0f);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(PIN_STRIP_PWR, OUTPUT);
  digitalWrite(PIN_STRIP_PWR, HIGH);   // D4184 ON
  delay(10);
  strip.begin();
  strip.setBrightness(80);
}

void loop() {
  static uint8_t step = 0;
  static uint32_t last = 0;
  if (millis() - last < 1000) {   // 1秒たつまでは同じ色を送り続ける
    strip.show();
    delayMicroseconds(300);
    return;
  }
  last = millis();
  const char* names[] = {"赤", "緑", "青"};
  uint32_t colors[] = {strip.Color(255, 0, 0), strip.Color(0, 255, 0), strip.Color(0, 0, 255)};

  // 基板LED (LOWで点灯)
  digitalWrite(LED_RED,   step == 0 ? LOW : HIGH);
  digitalWrite(LED_GREEN, step == 1 ? LOW : HIGH);
  digitalWrite(LED_BLUE,  step == 2 ? LOW : HIGH);

  strip.fill(colors[step]);
  strip.show();

  Serial.print("LEDテープ: ");
  Serial.print(names[step]);
  Serial.print("  / D2: ");
  Serial.print(digitalRead(PIN_STRIP_PWR) ? "HIGH" : "LOW");
  Serial.print("  / 電池電圧: ");
  Serial.print(readBattery(), 2);
  Serial.println(" V  (2.5V未満なら電池が未接続か空)");

  step = (step + 1) % 3;
}
