# DogRing

[日本語](README.md) | English

A light-up LED ring (collar) for dogs, built with a Seeed XIAO. It uses a single button.

## Hardware
- Microcontroller: Seeed XIAO nRF52840
- LEDs: WS2812B 5050 LED strip (29 LEDs)
- Power: 3.6V Li-ion battery (connected to BAT+/BAT− on the back of the XIAO, charged over USB-C)
- LED power switch: D4184 MOSFET module (switched on/off by D2)

## Button controls
| Action | Function |
|---|---|
| Single press | Next pattern (24 patterns) |
| Double press | Change brightness (3 levels) |
| Triple press | Back to the first pattern |
| Long press (1.5 s) | Power on / off |

## Files
| Folder / file | Contents |
|---|---|
| `DogRing_nRF52840/` | Main sketch for XIAO nRF52840 |
| `DogRing_WiringGuide_en.html` | Wiring guide with diagrams (English, open in a browser) |
| `DogRing配線ガイド.html` | Wiring guide with diagrams (Japanese) |

## Required library
- Adafruit NeoPixel

## Safety
- Default brightness is low to be gentle on the dog's eyes.
- Waterproof the electronics and mount them where the dog cannot chew them.
