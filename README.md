# zmk-config-redoxft

## PL

Konfiguracja ZMK dla **Redox FT** - rozszerzonej klawiatury ergonomicznej FalbaTech.

## Hardware

- Shield: `redox` (oficjalny shield upstream ZMK)
- Kontrolery: 2x nice!nano v2
- Wyświetlacz: nice!view, Sharp Memory LCD
- RGB: per-key WS2812, 35 LED na każdej połówce
- 70 klawiszy, 5 rzędów x 7 kolumn + 5 thumb na stronę

## Warstwy

| # | Nazwa | Funkcja |
|---|---|---|
| 0 | `DEF` | QWERTY base |
| 1 | `NAV` | Strzałki, nawigacja |
| 2 | `SYM` | Symbole, brackets |
| 3 | `ADJ` | F-keys, system, BT controls |
| 4 | `EXTRA` | Mouse emulation + RGB controls |

## ZMK Studio

ZMK Studio jest aktywne.

Procedura odblokowania jest taka sama we wszystkich klawiaturach FalbaTech FT:

> Trzymaj oba thumby aktywujące warstwy systemowe i wciśnij skrajny lewy górny klawisz.

Po odblokowaniu klawiatura jest edytowalna z poziomu przeglądarki:

https://zmk.studio

## Bluetooth - obsługa 5 urządzeń

Klawiatura obsługuje 5 niezależnych profili Bluetooth. Sterowanie odbywa się w warstwie systemowej `ADJ`.

| Klawisz | Funkcja |
|---|---|
| `Z` | Profil BT 0 |
| `X` | Profil BT 1 |
| `C` | Profil BT 2 |
| `V` | Profil BT 3 |
| `B` | Profil BT 4 |
| `N` | Wyczyść aktywny profil |
| `M` | Wyczyść wszystkie profile |
| `,` | Tryb USB |
| `.` | Tryb Bluetooth |

### Parowanie nowego urządzenia

1. Wejdź do warstwy `ADJ`.
2. Wciśnij odpowiedni klawisz Bluetooth `Z-B`.
3. Znajdź „Redox FT” w ustawieniach Bluetooth komputera lub telefonu.
4. Sparuj urządzenie.

## RGB controls

Sterowanie RGB znajduje się w warstwie `EXTRA`.

| Klawisz | Funkcja |
|---|---|
| `Q` | RGB on/off |
| `W` | Zmiana efektu |
| `E/R` | Hue +/- |
| `S/D` | Brightness +/- |
| `F/G` | Saturation +/- |
| `X/C` | Speed +/- |

## Build

GitHub Actions buduje 3 pliki firmware:

- `redox_left-nice_nano-zmk.uf2`
- `redox_right-nice_nano-zmk.uf2`
- `settings_reset-nice_nano-zmk.uf2`

## Flashowanie

1. Podłącz lewą połówkę przez USB.
2. Naciśnij RESET dwa razy szybko.
3. Przeciągnij `redox_left-...uf2` na dysk `NICENANO`.
4. Podłącz prawą połówkę przez USB.
5. Naciśnij RESET dwa razy szybko.
6. Przeciągnij `redox_right-...uf2`.
7. Połącz obie połówki przewodem TRRS.
8. Sparuj klawiaturę jako "Redox FT" przez Bluetooth.

## Wsparcie

FalbaTech  
https://falbatech.click

---

## EN

ZMK configuration for **Redox FT** - extended ergonomic FalbaTech keyboard.

## Hardware

- Shield: `redox` (official upstream ZMK shield)
- Controllers: 2x nice!nano v2
- Display: nice!view, Sharp Memory LCD
- RGB: per-key WS2812, 35 LEDs on each half
- 70 keys, 5 rows x 7 columns + 5 thumb keys per side

## Layers

| # | Name | Function |
|---|---|---|
| 0 | `DEF` | QWERTY base |
| 1 | `NAV` | Arrows, navigation |
| 2 | `SYM` | Symbols, brackets |
| 3 | `ADJ` | F-keys, system, BT controls |
| 4 | `EXTRA` | Mouse emulation + RGB controls |

## ZMK Studio

ZMK Studio is enabled.

The unlock procedure is the same across all FalbaTech FT keyboards:

> Hold both thumb keys activating system layers and press the top left key.

After unlocking, the keyboard can be configured from your browser:

https://zmk.studio

## Bluetooth - 5 device support

The keyboard supports 5 independent Bluetooth profiles. Control is handled in the system layer `ADJ`.

| Key | Function |
|---|---|
| `Z` | BT Profile 0 |
| `X` | BT Profile 1 |
| `C` | BT Profile 2 |
| `V` | BT Profile 3 |
| `B` | BT Profile 4 |
| `N` | Clear active profile |
| `M` | Clear all profiles |
| `,` | USB mode |
| `.` | Bluetooth mode |

### Pairing a new device

1. Enter the `ADJ` layer.
2. Press the selected Bluetooth key `Z-B`.
3. Find “Redox FT” in your computer or phone Bluetooth settings.
4. Pair the device.

## RGB controls

RGB controls are located in the `EXTRA` layer.

| Key | Function |
|---|---|
| `Q` | RGB on/off |
| `W` | Change effect |
| `E/R` | Hue +/- |
| `S/D` | Brightness +/- |
| `F/G` | Saturation +/- |
| `X/C` | Speed +/- |

## Build

GitHub Actions builds 3 firmware files:

- `redox_left-nice_nano-zmk.uf2`
- `redox_right-nice_nano-zmk.uf2`
- `settings_reset-nice_nano-zmk.uf2`

## Flashing

1. Connect the left half via USB.
2. Press RESET twice quickly.
3. Drag `redox_left-...uf2` onto the `NICENANO` drive.
4. Connect the right half via USB.
5. Press RESET twice quickly.
6. Drag `redox_right-...uf2`.
7. Connect both halves using a TRRS cable.
8. Pair the keyboard as "Redox FT" over Bluetooth.

## Support

FalbaTech  
https://falbatech.click
