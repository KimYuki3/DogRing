/*
 * FastLED 版テスト (XIAO nRF52840)
 * Adafruit NeoPixel とは別のライブラリで LEDテープを光らせる。
 * こちらで光れば、原因はライブラリ側。
 *
 * ・基板LED と LEDテープ先頭5個が 赤→緑→青 と1秒ごとに変わる
 * ・D2 は HIGH (D4184 ON)
 */
#include <Adafruit_TinyUSB.h>   // FastLED がシリアル機能を使うため必要
#include <FastLED.h>

#define DATA_PIN  D1
#define NUM_TEST  5

CRGB leds[NUM_TEST];

void setup() {
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(D2, OUTPUT);
  digitalWrite(D2, HIGH);
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_TEST);
  FastLED.setBrightness(80);
}

void loop() {
  static uint8_t step = 0;
  const CRGB colors[3] = {CRGB::Red, CRGB::Green, CRGB::Blue};

  digitalWrite(LED_RED,   step == 0 ? LOW : HIGH);
  digitalWrite(LED_GREEN, step == 1 ? LOW : HIGH);
  digitalWrite(LED_BLUE,  step == 2 ? LOW : HIGH);

  fill_solid(leds, NUM_TEST, colors[step]);
  uint32_t t = millis();
  while (millis() - t < 1000) {   // 1秒間 信号を送り続ける
    FastLED.show();
    delayMicroseconds(300);
  }
  step = (step + 1) % 3;
}
