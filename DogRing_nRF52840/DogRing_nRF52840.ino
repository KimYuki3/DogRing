/*
 * DogRing - 犬用発光リング (Seeed Studio XIAO nRF52840 用)
 *
 * ボタン1つで全操作:
 *   短押し          : 次の発光パターンへ
 *   ダブルクリック   : 明るさ切替 (弱 → 中 → 強)
 *   トリプルクリック : 最初のパターン(虹)に戻る
 *   長押し(1.5秒)   : 電源OFF / ON
 *
 * 電池まわり:
 *   ・USB-C を挿すと電池を充電 (充電中は基板の緑LEDが点灯)
 *   ・電池が減ると赤く3回点滅でお知らせ、さらに減ると自動で電源OFF
 *   ・電源OFF中は System OFF (数µA) で待機。パターンと明るさは覚えている
 *
 * LED: XUNATA WS2812B 5050 LEDテープを首輪の長さに切って使用
 *       ・テープ上の矢印(→)の根元側 DIN に配線。ハサミマークの位置で切る
 *
 * 配線 (電源: 3.6V リチウムイオン電池):
 *   電池+ ──┬── XIAO 裏面 BAT+ パッド
 *           └── LED +5V (赤線)        (WS2812Bは3.5〜4.2Vでも点灯。青・白がやや暗い)
 *   電池- ──┬── XIAO 裏面 BAT- パッド
 *           └── MOSFET ソース
 *   LED GND (白線) ── MOSFET ドレイン
 *   D2 ── 100Ω ── MOSFET ゲート  (+ ゲート〜GND に 100kΩ)
 *        MOSFET: Nch ロジックレベル (AO3400 / IRLML2502 など)
 *        → 電源OFF時にLEDテープの電流を遮断 (WS2812Bは消灯中も1個約1mA消費するため)
 *   D0 ── ボタン ── GND          (内部プルアップ使用、抵抗不要)
 *   D1 ── 330Ω ── LED DIN (緑線)
 *   LED +5V と GND の間に 100〜470µF のコンデンサ
 *
 *   ※ D4184 MOSFETモジュールを使う場合は ../配線図_nRF52840_D4184.md を参照
 *     (抵抗はモジュールに内蔵なので不要。TRIG→D2, 信号GND→GND)
 *   ※ MOSFETを使わない場合は USE_LED_POWER_SWITCH を 0 にし、LED GND を直接 BAT- へ
 *   ※ 電池は保護回路付きのものを推奨
 *
 * ボード: Seeed nRF52 Boards → "Seeed XIAO nRF52840"  (mbed版ではない方)
 * 必要ライブラリ: Adafruit NeoPixel
 */

#include <Adafruit_NeoPixel.h>
#include <nrf_gpio.h>

enum ButtonEvent { EV_NONE, EV_CLICK, EV_DOUBLE, EV_TRIPLE, EV_LONG };

// ===== 設定 =====
const uint8_t  PIN_BUTTON    = D0;
const uint8_t  PIN_STRIP     = D1;
const uint8_t  PIN_STRIP_PWR = D2;   // LED電源用MOSFETのゲート
#define USE_LED_POWER_SWITCH 1       // MOSFETを付けない場合は 0

// 首輪に巻くLEDの数。60LED/m のテープなら 首周り(cm) × 0.6
//   例: 首周り 35cm → 21個 / 30LED/m のテープなら × 0.3
const uint16_t NUM_LEDS     = 29;
const uint8_t  BRIGHTNESS[] = {20, 60, 150};   // 弱/中/強 (最大255。電池と犬の目のため控えめに)

const uint16_t DEBOUNCE_MS     = 30;
const uint16_t LONG_PRESS_MS   = 1500;
const uint16_t MULTI_CLICK_MS  = 350;
const uint16_t FRAME_MS        = 20;           // 約50fps

// 電池
const bool     FAST_CHARGE     = true;   // true: 100mA充電 / false: 50mA (電池容量200mAh未満なら false)
const float    BATT_LOW_V      = 3.40;   // これ以下で赤点滅のお知らせ
const float    BATT_CUTOFF_V   = 3.20;   // これ以下で自動電源OFF (電池保護)
const uint32_t BATT_CHECK_MS   = 30000;  // 電池チェック間隔

// XIAO nRF52840 の基板内部配線 (nRFのポート番号)
const uint8_t  NRF_VBAT_ENABLE = 14;     // P0.14: LOW で電池電圧の分圧回路ON
const uint8_t  NRF_CHG_CURRENT = 13;     // P0.13: LOW で100mA充電

Adafruit_NeoPixel strip(NUM_LEDS, PIN_STRIP, NEO_GRB + NEO_KHZ800);
uint8_t  brightIdx = 1;
bool     powerOn   = true;
uint32_t frame     = 0;    // フレームカウンタ (各パターンのアニメ用)

// ===== 小道具 =====
uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) { return strip.Color(r, g, b); }
uint32_t hsv(uint16_t h, uint8_t s = 255, uint8_t v = 255) {
  return strip.gamma32(strip.ColorHSV(h, s, v));
}
void fillAll(uint32_t c) { for (uint16_t i = 0; i < NUM_LEDS; i++) strip.setPixelColor(i, c); }

// 色を暗くする (残像・フェード用)
uint32_t dim(uint32_t c, uint8_t scale) {
  uint8_t r = (uint8_t)(c >> 16), g = (uint8_t)(c >> 8), b = (uint8_t)c;
  return rgb(r * scale / 256, g * scale / 256, b * scale / 256);
}
void fadeAll(uint8_t scale) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) strip.setPixelColor(i, dim(strip.getPixelColor(i), scale));
}
// 三角波 0→255→0 (period フレームで1往復)
uint8_t tri(uint32_t t, uint16_t period) {
  uint32_t p = t % period;
  uint32_t half = period / 2;
  return p < half ? p * 255 / half : (period - p) * 255 / half;
}

// ===== 発光パターン =====
// 各関数は1フレーム分の描画を行う (show() は呼び出し側)

// 1. 虹が回る
void pRainbow() {
  for (uint16_t i = 0; i < NUM_LEDS; i++)
    strip.setPixelColor(i, hsv(frame * 256 + i * 65536UL / NUM_LEDS));
}

// 2. 全体が同じ色で虹色にゆっくり変化
void pColorCycle() { fillAll(hsv(frame * 128)); }

// 3〜7. 単色点灯
void pRed()    { fillAll(rgb(255, 0, 0)); }
void pGreen()  { fillAll(rgb(0, 255, 0)); }
void pBlue()   { fillAll(rgb(0, 0, 255)); }
void pWhite()  { fillAll(rgb(255, 255, 255)); }
void pPink()   { fillAll(rgb(255, 40, 120)); }

// 8. 白の点滅 (夜の散歩で目立つ)
void pBlink() { fillAll(((frame / 15) % 2) ? rgb(255, 255, 255) : 0); }

// 9. ダブルストロボ (ピカッピカッ…休み)
void pStrobe() {
  uint8_t p = frame % 50;
  bool on = (p < 2) || (p >= 6 && p < 8);
  fillAll(on ? rgb(255, 255, 255) : 0);
}

// 10. パトカー風 (半分ずつ赤青が交互)
void pPolice() {
  bool phase = (frame / 12) % 2;
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    bool firstHalf = i < NUM_LEDS / 2;
    strip.setPixelColor(i, (firstHalf == phase) ? rgb(255, 0, 0) : rgb(0, 0, 255));
  }
}

// 11. 光が一周する (尾を引くシアン)
void pChase() {
  fadeAll(150);
  strip.setPixelColor((frame / 2) % NUM_LEDS, rgb(0, 255, 255));
}

// 12. 虹色の彗星
void pComet() {
  fadeAll(180);
  strip.setPixelColor((frame / 2) % NUM_LEDS, hsv(frame * 300));
}

// 13. 2つの光が逆回り
void pDualSpin() {
  fadeAll(160);
  uint16_t a = (frame / 2) % NUM_LEDS;
  uint16_t b = (NUM_LEDS - 1) - a;
  strip.setPixelColor(a, rgb(255, 0, 180));
  strip.setPixelColor(b, rgb(0, 200, 255));
}

// 14. スキャナー (赤い光が往復)
void pScanner() {
  fadeAll(140);
  uint16_t span = (NUM_LEDS - 1) * 2;
  uint16_t p = (frame / 2) % span;
  if (p >= NUM_LEDS) p = span - p;
  strip.setPixelColor(p, rgb(255, 0, 0));
}

// 15. シアター (3つおきの光が流れる)
void pTheater() {
  uint8_t offset = (frame / 6) % 3;
  for (uint16_t i = 0; i < NUM_LEDS; i++)
    strip.setPixelColor(i, (i % 3 == offset) ? hsv(frame * 200 + i * 2000) : 0);
}

// 16. カラーワイプ (色が順に塗り替わる)
void pWipe() {
  uint16_t pos   = (frame / 3) % NUM_LEDS;
  uint16_t round = (frame / 3) / NUM_LEDS;
  uint32_t c = hsv(round * 12000);
  strip.setPixelColor(pos, c);
}

// 17. 呼吸 (オレンジがゆっくり明滅)
void pBreath() {
  uint8_t v = tri(frame, 200);
  v = (uint16_t)v * v / 255;        // 人の目に自然なカーブ
  fillAll(rgb(v, v / 3, 0));
}

// 18. ハートビート (ドクン、ドクン)
void pHeartbeat() {
  uint8_t p = frame % 60;
  uint8_t v = 0;
  if (p < 5)                 v = 255 - p * 40;
  else if (p >= 10 && p < 15) v = 200 - (p - 10) * 35;
  fillAll(rgb(v, 0, v / 8));
}

// 19. キラキラ (ランダムに白く瞬く)
void pSparkle() {
  fadeAll(200);
  if (random(3) == 0) strip.setPixelColor(random(NUM_LEDS), rgb(255, 255, 255));
}

// 20. カラフル紙吹雪
void pConfetti() {
  fadeAll(220);
  if (random(2) == 0) strip.setPixelColor(random(NUM_LEDS), hsv(random(65536)));
}

// 21. 炎 (ゆらゆら)
void pFire() {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    uint8_t flick = random(80, 256);
    strip.setPixelColor(i, rgb(flick, flick * random(20, 45) / 100, 0));
  }
}

// 22. 海 (青〜水色のうねり)
void pOcean() {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    uint8_t w = tri(frame * 2 + i * 256 / NUM_LEDS * 2, 256);
    strip.setPixelColor(i, rgb(0, w / 2, 128 + w / 2));
  }
}

// 23. 半分ずつ2色が回る
void pSplit() {
  uint16_t shift = (frame / 3) % NUM_LEDS;
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    bool half = ((i + shift) % NUM_LEDS) < NUM_LEDS / 2;
    strip.setPixelColor(i, half ? rgb(255, 140, 0) : rgb(0, 80, 255));
  }
}

// 24. 信号機 (赤 → 黄 → 緑)
void pTraffic() {
  uint8_t s = (frame / 50) % 3;
  const uint32_t cols[3] = {rgb(255, 0, 0), rgb(255, 160, 0), rgb(0, 255, 0)};
  fillAll(cols[s]);
}

typedef void (*PatternFn)();
const PatternFn PATTERNS[] = {
  pRainbow, pColorCycle, pRed, pGreen, pBlue, pWhite, pPink,
  pBlink, pStrobe, pPolice, pChase, pComet, pDualSpin, pScanner,
  pTheater, pWipe, pBreath, pHeartbeat, pSparkle, pConfetti,
  pFire, pOcean, pSplit, pTraffic,
};
const uint8_t PATTERN_COUNT = sizeof(PATTERNS) / sizeof(PATTERNS[0]);
uint8_t mode = 0;

void render() {
  static uint32_t lastFrame = 0;
  uint32_t now = millis();
  if (now - lastFrame < FRAME_MS) return;
  lastFrame = now;
  frame++;
  PATTERNS[mode]();
  strip.show();
}

void changeMode(uint8_t m) {
  mode = m;
  frame = 0;
  strip.clear();   // 残像系パターンの前の絵を消す
}

// ===== ボタン処理 =====

ButtonEvent readButton() {
  static bool     lastRaw     = HIGH;
  static bool     stable      = HIGH;
  static uint32_t lastChange  = 0;
  static uint32_t pressStart  = 0;
  static uint32_t lastRelease = 0;
  static uint8_t  clickCount  = 0;
  static bool     longvFiredFired   = false;

  uint32_t now = millis();
  bool raw = digitalRead(PIN_BUTTON);
  if (raw != lastRaw) { lastChange = now; lastRaw = raw; }

  if (now - lastChange > DEBOUNCE_MS && raw != stable) {
    stable = raw;
    if (stable == LOW) {               // 押された
      pressStart = now;
      longFired = false;
    } else if (!longFired) {           // 離された
      clickCount++;
      lastRelease = now;
    }
  }

  if (stable == LOW && !longFired && now - pressStart >= LONG_PRESS_MS) {
    longFired = true;
    clickCount = 0;
    return EV_LONG;
  }

  if (clickCount > 0 && stable == HIGH && now - lastRelease > MULTI_CLICK_MS) {
    uint8_t c = clickCount;
    clickCount = 0;
    if (c >= 3) return EV_TRIPLE;
    if (c == 2) return EV_DOUBLE;
    return EV_CLICK;
  }
  return EV_NONE;
}

// 操作フィードバック用の短い点滅
void flash(uint32_t color, uint8_t times) {
  for (uint8_t n = 0; n < times; n++) {
    fillAll(color); strip.show(); delay(80);
    strip.clear();  strip.show(); delay(80);
  }
}

// ===== 電池 =====
float readBatteryVoltage() {
  // XIAO nRF52840: 電池 → 1MΩ/510kΩ 分圧 → PIN_VBAT(P0.31)
  analogReference(AR_INTERNAL_2_4);
  analogReadResolution(12);
  uint32_t sum = 0;
  for (uint8_t i = 0; i < 8; i++) sum += analogRead(PIN_VBAT);
  analogReference(AR_DEFAULT);
  float adc = sum / 8.0f;
  return adc / 4095.0f * 2.4f * (1510.0f / 510.0f);
}

void setupBatteryPins() {
  // 分圧回路は常にON (P0.14をHIGHにすると充電中にP0.31へ高電圧がかかる恐れがあるため)
  nrf_gpio_cfg_output(NRF_VBAT_ENABLE);
  nrf_gpio_pin_clear(NRF_VBAT_ENABLE);
  if (FAST_CHARGE) {
    nrf_gpio_cfg_output(NRF_CHG_CURRENT);
    nrf_gpio_pin_clear(NRF_CHG_CURRENT);
  }
}

// ===== 状態の保存 (System OFF をまたいで保持されるレジスタ) =====
// bit7: 有効フラグ / bit5-6: 明るさ / bit0-4: パターン番号
void saveState() {
  NRF_POWER->GPREGRET2 = 0x80 | ((brightIdx & 0x03) << 5) | (mode & 0x1F);
}

void loadState() {
  uint8_t s = NRF_POWER->GPREGRET2;
  if (s & 0x80) {
    uint8_t m = s & 0x1F;
    uint8_t b = (s >> 5) & 0x03;
    if (m < PATTERN_COUNT)      mode = m;
    if (b < sizeof(BRIGHTNESS)) brightIdx = b;
  }
}

// ===== 電源 =====
void ledPower(bool on) {
#if USE_LED_POWER_SWITCH
  if (on) {
    digitalWrite(PIN_STRIP_PWR, HIGH);
    delay(5);                          // LEDテープの電源が安定するまで待つ
  } else {
    strip.clear();
    strip.show();
    digitalWrite(PIN_STRIP, LOW);      // データ線経由でLEDに電流が回り込まないように
    digitalWrite(PIN_STRIP_PWR, LOW);
  }
#else
  if (!on) { strip.clear(); strip.show(); }
#endif
}

bool heldFor(uint16_t ms) {
  uint32_t t = millis();
  while (digitalRead(PIN_BUTTON) == LOW) {
    if (millis() - t >= ms) return true;
    delay(5);
  }
  return false;
}

// System OFF に入る。ボタンを押すとリセットされ setup() から再開する
void sleepNow() {
  saveState();
  ledPower(false);
  while (digitalRead(PIN_BUTTON) == LOW) delay(10);   // 指を離すまで待つ
  delay(50);
  systemOff(PIN_BUTTON, LOW);
}

void powerOff() {
  flash(rgb(255, 0, 0), 2);
  sleepNow();
}

void checkBattery() {
  static uint32_t lastCheck = 0;
  if (millis() - lastCheck < BATT_CHECK_MS) return;
  lastCheck = millis();

  float v = readBatteryVoltage();
  if (v < 2.5f) return;                // 電池未接続 (USB給電のみ) とみなす
  if (v < BATT_CUTOFF_V) {
    flash(rgb(255, 0, 0), 5);
    sleepNow();
  } else if (v < BATT_LOW_V) {
    flash(rgb(255, 0, 0), 3);
    changeMode(mode);
  }
}

// ===== メイン =====
void setup() {
  pinMode(PIN_BUTTON, INPUT_PULLUP);
#if USE_LED_POWER_SWITCH
  pinMode(PIN_STRIP_PWR, OUTPUT);
  digitalWrite(PIN_STRIP_PWR, LOW);
#endif
  setupBatteryPins();
  loadState();

  // ボタンで System OFF から起きた場合: 長押しでなければ再び眠る
  if (readResetReason() & POWER_RESETREAS_OFF_Msk) {
    delay(DEBOUNCE_MS);
    if (!heldFor(LONG_PRESS_MS - DEBOUNCE_MS)) {
      systemOff(PIN_BUTTON, LOW);
    }
  }

  randomSeed(analogRead(A3));
  strip.begin();
  strip.setBrightness(BRIGHTNESS[brightIdx]);
  ledPower(true);
  flash(rgb(0, 255, 0), 1);

  // 起動時に電池が少なければお知らせ
  float v = readBatteryVoltage();
  if (v >= 2.5f && v < BATT_LOW_V) flash(rgb(255, 0, 0), 3);

  while (digitalRead(PIN_BUTTON) == LOW) delay(10);   // 起動の長押しを離すまで待つ
  changeMode(mode);
}

void loop() {
  switch (readButton()) {
    case EV_CLICK:
      changeMode((mode + 1) % PATTERN_COUNT);
      break;
    case EV_DOUBLE:
      brightIdx = (brightIdx + 1) % sizeof(BRIGHTNESS);
      strip.setBrightness(BRIGHTNESS[brightIdx]);
      flash(rgb(255, 255, 255), brightIdx + 1);   // 1〜3回点滅で段階を表示
      changeMode(mode);
      break;
    case EV_TRIPLE:
      flash(hsv(0), 1);
      changeMode(0);
      break;
    case EV_LONG:
      powerOff();
      return;
    default:
      break;
  }

  checkBattery();
  render();
}
