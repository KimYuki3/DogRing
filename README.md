# DogRing

XIAO で作る犬用の発光リング（首輪）です。ボタンは1つだけです。

## 構成
- マイコン: Seeed XIAO nRF52840（Seeeduino XIAO / SAMD21 版もあります）
- LED: WS2812B 5050 LEDテープ（29個）
- 電源: 3.6V リチウムイオン電池（XIAO 裏面の BAT+/BAT− につなぎ、USB-C で充電）
- LED の電源スイッチ: D4184 MOSFET モジュール（D2 で ON/OFF）

## ボタン操作
| 操作 | 動作 |
|---|---|
| 1回押す | 次のパターン（全24種類） |
| 2回押す | 明るさを切り替える（3段階） |
| 3回押す | 最初のパターンに戻る |
| 長押し（1.5秒） | 電源 ON / OFF |

## フォルダ
| フォルダ / ファイル | 内容 |
|---|---|
| `DogRing_nRF52840/` | 本番スケッチ（XIAO nRF52840 用） |
| `DogRing/` | Seeeduino XIAO（SAMD21）用 |
| `DogRing_Test/` | LEDテープの動作確認と電池電圧の表示 |
| `DogRing_PinTest/` | D1 ピンの確認 |
| `DogRing_FastLEDTest/` | FastLED ライブラリでのテスト |
| `配線図_nRF52840_D4184.md` | 配線図と配線手順 |
| `DogRing配線ガイド.html` | 図入りの配線ガイド（ブラウザで開く） |

## 必要なライブラリ
- Adafruit NeoPixel
- （SAMD21 版のみ）Arduino Low Power
- （FastLED テストのみ）FastLED
