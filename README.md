# DogRing

[English](README.en.md) | 日本語

XIAO で作る犬用の発光リング（首輪）です。ボタンは1つだけです。

## 構成
- マイコン: Seeed XIAO nRF52840
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
| `DogRing配線ガイド.html` | 図入りの配線ガイド（ブラウザで開く） |
| `DogRing_WiringGuide_en.html` | 配線ガイド（英語版） |

## 必要なライブラリ
- Adafruit NeoPixel
