# zmk-config-redoxft

Konfiguracja ZMK dla **Redox FT** - klawiatura ergonomiczna FalbaTech (rozszerzona).

## Hardware

- Shield: `redox` (oficjalny w upstream ZMK)
- Kontrolery: 2× nice!nano v2
- Wyświetlacz: nice!view (Sharp Memory LCD)
- RGB: per-key WS2812, 35 LED na każdej połówce
- 70 klawiszy (5 rzędów × 7 kolumn + 5 thumb na stronę)

## Warstwy

| # | Nazwa | Funkcja |
|---|-------|---------|
| 0 | DEF | QWERTY base |
| 1 | NAV | Strzałki, nawigacja |
| 2 | SYM | Symbole, brackets |
| 3 | ADJ | F-keys, **system**, **BT controls** |
| 4 | EXTRA | Mouse emulation + RGB controls |

## ZMK Studio

Aktywne. **Procedura odblokowania (jednakowa we wszystkich klawiaturach FalbaTech FT):**

> Trzymaj oba thumby aktywujące warstwy systemowe - wciśnij **skrajny lewy górny klawisz**.

## Bluetooth - obsługa 5 urządzeń

Klawiatura obsługuje **5 niezależnych profili Bluetooth**. W warstwie systemowej (ADJ):

| Klawisz | Funkcja |
|---------|---------|
| `Z` | Profil BT 0 |
| `X` | Profil BT 1 |
| `C` | Profil BT 2 |
| `V` | Profil BT 3 |
| `B` | Profil BT 4 |
| `N` | Wyczyść aktywny profil |
| `M` | Wyczyść wszystkie profile |
| `,` | Tryb USB |
| `.` | Tryb Bluetooth |

**Parowanie nowego urządzenia:**
1. ADJ + odpowiedni klawisz BT (Z-B)
2. Znajdź "Redox FT" w liście Bluetooth na komputerze
3. Sparuj

## RGB controls (warstwa EXTRA)

| Klawisz | Funkcja |
|---------|---------|
| `Q` | RGB on/off |
| `W` | Zmiana efektu |
| `E/R` | Hue +/- |
| `S/D` | Brightness +/- |
| `F/G` | Saturation +/- |
| `X/C` | Speed +/- |

## Build

GitHub Actions buduje 3 firmware:
- `redox_left-nice_nano-zmk.uf2`
- `redox_right-nice_nano-zmk.uf2`
- `settings_reset-nice_nano-zmk.uf2`

## Flashowanie

1. Lewa USB - 2× reset - przeciągnij `redox_left-...uf2`
2. Prawa USB - 2× reset - `redox_right-...uf2`
3. Połącz TRRS - klawiatura "Redox FT" w BT

## Wsparcie

FalbaTech - [falbatech.click](https://falbatech.click)
