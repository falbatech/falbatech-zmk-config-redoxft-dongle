/*
 * Redox FT Dongle — FalbaTech Status Screen
 * GC9A01 240×240 okrągły TFT, nice!nano v2
 * SPDX-License-Identifier: MIT
 *
 * Funkcje:
 *  - Splash z logo FalbaTech (2.5s)
 *  - Nazwa aktywnej warstwy (Montserrat 28)
 *  - Wskaźniki połączenia L/R z baterią (10-segmentowy słupek)
 *  - 5 kropek profilu BT
 *  - Wskaźniki CAPS / NUM lock
 *  - WPM (słowa na minutę) — aktualizowany w czasie rzeczywistym
 *  - Tryb wyjścia: USB lub BLE
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/split_central_status_changed.h>
#include <zmk/events/hid_indicators_changed.h>
#include <zmk/events/wpm_state_changed.h>
#include <zmk/events/endpoint_selection_changed.h>

#include <zmk/keymap.h>
#include <zmk/ble.h>
#include <zmk/hid_indicators.h>
#include <zmk/hid.h>
#include <zmk/wpm.h>
#include <zmk/endpoints.h>

#include "falbatech_logo.h"

LV_IMG_DECLARE(zmk_studio_logo);

LOG_MODULE_REGISTER(ft_status_screen, CONFIG_LOG_DEFAULT_LEVEL);

/* ── Timing ────────────────────────────────────────────────────── */
#define SPLASH_MS       2500

/* ── Kolory (GC9A01, bez LV_COLOR_16_SWAP) ─────────────────────
 * lv_color_hex() na tym wyświetlaczu działa w kolejności BGR:
 *   chcesz zielony  → podaj 0xE00039
 *   chcesz szary    → podaj 0x884890
 *   chcesz pomarańczowy → podaj 0x00C8FF                         */
#define C_BG            0x000000   /* czarny    */
#define C_TEXT          0xFFFFFF   /* biały     */
#define C_ON            0xE00039   /* zielony   */
#define C_OFF           0x884890   /* szary     */
#define C_USB           0x00A0FF   /* niebieski (USB) */
#define C_BLE           0xE00039   /* zielony (BLE)   */
#define C_WPM           0xA0FF00   /* żółto-zielony   */

/* ── Pasek baterii ──────────────────────────────────────────────
 * 10 segmentów, każdy 5×18px z przerwą 2px                       */
#define BAR_SEGS        10
#define SEG_H            5
#define SEG_W           18
#define SEG_GAP          2

/* ── Stan globalny ─────────────────────────────────────────────── */
static bool                 splash_done     = false;
static bool                 left_conn       = false;
static bool                 right_conn      = false;
static int                  bat_left        = 0;
static int                  bat_right       = 0;
static uint8_t              left_reconnects = 0;
static uint8_t              right_reconnects = 0;
static zmk_hid_indicators_t hid_indicators  = 0;
static uint8_t              wpm             = 0;
static bool                 usb_output      = false;

/* ── Delayed work ────────────────────────────────────────────────*/
static struct k_work_delayable splash_work;

/* ── Widgety ────────────────────────────────────────────────────── */
static lv_obj_t *screen;
static lv_obj_t *splash_img;
static lv_obj_t *studio_logo;
static lv_obj_t *layer_lbl;
static lv_obj_t *left_lbl;
static lv_obj_t *right_lbl;
static lv_obj_t *left_pct;
static lv_obj_t *right_pct;
static lv_obj_t *left_segs[BAR_SEGS];
static lv_obj_t *right_segs[BAR_SEGS];
static lv_obj_t *bt_dots[5];
static lv_obj_t *caps_pill;
static lv_obj_t *num_pill;
static lv_obj_t *wpm_lbl;
static lv_obj_t *output_dot;
static lv_obj_t *output_lbl;

/* ═══════════════════════════════════════════════════════════════
 * Helpers
 * ═══════════════════════════════════════════════════════════════ */

static inline void set_hidden(lv_obj_t *o, bool hide)
{
    if (!o) return;
    if (hide) lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
    else      lv_obj_clear_flag(o, LV_OBJ_FLAG_HIDDEN);
}

static lv_obj_t *box(lv_obj_t *parent, int w, int h, uint32_t color, int r)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_color(o, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_radius(o, r, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    return o;
}

static lv_obj_t *label(lv_obj_t *parent, const char *txt,
                        const lv_font_t *font, uint32_t color)
{
    lv_obj_t *o = lv_label_create(parent);
    lv_label_set_text(o, txt);
    lv_obj_set_style_text_font(o, font, 0);
    lv_obj_set_style_text_color(o, lv_color_hex(color), 0);
    return o;
}

static lv_obj_t *pill(lv_obj_t *parent, const char *txt, int x_ofs)
{
    lv_obj_t *o = lv_label_create(parent);
    lv_label_set_text(o, txt);
    lv_obj_set_style_text_font(o, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(o, lv_color_hex(C_BG), 0);
    lv_obj_set_style_bg_color(o, lv_color_hex(C_ON), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(o, 4, 0);
    lv_obj_set_style_pad_hor(o, 5, 0);
    lv_obj_set_style_pad_ver(o, 2, 0);
    lv_obj_align(o, LV_ALIGN_CENTER, x_ofs, -20);
    set_hidden(o, true);
    return o;
}

/* ═══════════════════════════════════════════════════════════════
 * Update functions
 * ═══════════════════════════════════════════════════════════════ */

static void update_layer(void)
{
    if (!splash_done) { set_hidden(layer_lbl, true); return; }
    set_hidden(layer_lbl, false);
    uint8_t idx  = zmk_keymap_highest_layer_active();
    const char *n = zmk_keymap_layer_name(idx);
    lv_label_set_text(layer_lbl, n ? n : "Layer");
}

static void draw_bar(lv_obj_t **segs, int pct)
{
    int filled = (CLAMP(pct, 0, 100) + 9) / 10;
    for (int i = 0; i < BAR_SEGS; i++) {
        if (!segs[i]) continue;
        lv_obj_set_style_bg_color(segs[i],
            lv_color_hex(i < filled ? C_ON : C_OFF), 0);
    }
}

static void update_battery(void)
{
    bool show_l = splash_done && left_conn;
    bool show_r = splash_done && right_conn;

    set_hidden(left_pct,  !show_l);
    set_hidden(right_pct, !show_r);
    set_hidden(left_lbl,  !splash_done);
    set_hidden(right_lbl, !splash_done);

    for (int i = 0; i < BAR_SEGS; i++) {
        set_hidden(left_segs[i],  !show_l);
        set_hidden(right_segs[i], !show_r);
    }

    if (show_l) {
        lv_label_set_text_fmt(left_pct, "%d%%", bat_left);
        draw_bar(left_segs, bat_left);
    }
    if (show_r) {
        lv_label_set_text_fmt(right_pct, "%d%%", bat_right);
        draw_bar(right_segs, bat_right);
    }

    if (splash_done) {
        /* Kolor L/R: zielony = połączone, szary = nie, żółty = niestabilne */
        #define C_UNSTABLE 0x00BEFF   /* żółty */
        lv_obj_set_style_text_color(left_lbl,
            lv_color_hex(!left_conn   ? C_OFF :
                         left_reconnects > 2 ? C_UNSTABLE : C_ON), 0);
        lv_obj_set_style_text_color(right_lbl,
            lv_color_hex(!right_conn  ? C_OFF :
                         right_reconnects > 2 ? C_UNSTABLE : C_ON), 0);
        /* "L!" / "R!" gdy połączenie niestabilne */
        lv_label_set_text(left_lbl,  left_reconnects  > 2 ? "L!" : "L");
        lv_label_set_text(right_lbl, right_reconnects > 2 ? "R!" : "R");
    }
}

static void update_bt_profile(void)
{
    if (!splash_done) {
        for (int i = 0; i < 5; i++) set_hidden(bt_dots[i], true);
        return;
    }
    uint8_t active = zmk_ble_active_profile_index();
    for (int i = 0; i < 5; i++) {
        if (!bt_dots[i]) continue;
        set_hidden(bt_dots[i], false);
        bool is_active = (i == active);
        lv_obj_set_size(bt_dots[i], is_active ? 10 : 8, is_active ? 10 : 8);
        lv_obj_set_style_bg_color(bt_dots[i],
            lv_color_hex(is_active ? C_ON : C_OFF), 0);
        lv_obj_align(bt_dots[i], LV_ALIGN_BOTTOM_MID,
                     -32 + (i * 16), -14);
    }
}

static void update_hid(void)
{
    set_hidden(caps_pill, !(splash_done && (hid_indicators & HID_KBD_LED_CAPS_LOCK)));
    set_hidden(num_pill,  !(splash_done && (hid_indicators & HID_KBD_LED_NUM_LOCK)));
}

static void update_wpm(void)
{
    if (!splash_done) { set_hidden(wpm_lbl, true); return; }
    set_hidden(wpm_lbl, false);
    lv_label_set_text_fmt(wpm_lbl, "%d WPM", wpm);
}

static void update_output(void)
{
    if (!splash_done) {
        set_hidden(output_dot, true);
        set_hidden(output_lbl, true);
        return;
    }
    set_hidden(output_dot, false);
    set_hidden(output_lbl, false);
    lv_obj_set_style_bg_color(output_dot,
        lv_color_hex(usb_output ? C_USB : C_BLE), 0);
    lv_label_set_text(output_lbl, usb_output ? "USB" : "BLE");
}

static void refresh_all(void)
{
    update_layer();
    update_bt_profile();
    update_battery();
    update_hid();
    update_wpm();
    update_output();
}

/* ═══════════════════════════════════════════════════════════════
 * Build UI
 * ═══════════════════════════════════════════════════════════════ */

static void build_splash(void)
{
    splash_img = lv_image_create(screen);
    lv_image_set_src(splash_img, &falbatech_logo_large);
    lv_obj_align(splash_img, LV_ALIGN_CENTER, 0, 0);
}

static void build_header(void)
{
    /* Studio logo — lewy górny */
    studio_logo = lv_image_create(screen);
    lv_image_set_src(studio_logo, &zmk_studio_logo);
    lv_obj_align(studio_logo, LV_ALIGN_TOP_MID, 20, 10);
    set_hidden(studio_logo, true);

    /* Nazwa warstwy — środek ekranu */
    layer_lbl = label(screen, "Base", &lv_font_montserrat_28, C_TEXT);
    lv_obj_align(layer_lbl, LV_ALIGN_CENTER, 0, -42);
    set_hidden(layer_lbl, true);
}

static void build_battery_side(lv_obj_t **segs, lv_obj_t **pct_out,
                                lv_obj_t **lbl_out, int sign,
                                const char *side_char)
{
    int x_bar = sign * 76;
    int x_lbl = sign * 48;

    /* Literka L lub R (wskaźnik połączenia) */
    *lbl_out = label(screen, side_char, &lv_font_montserrat_14, C_OFF);
    lv_obj_align(*lbl_out, LV_ALIGN_CENTER, x_lbl, 2);
    set_hidden(*lbl_out, true);

    /* Procent baterii */
    *pct_out = label(screen, "0%", &lv_font_montserrat_14, C_TEXT);
    lv_obj_align(*pct_out, LV_ALIGN_CENTER, x_bar, -6);
    set_hidden(*pct_out, true);

    /* Segmentowany słupek baterii */
    int total_h = BAR_SEGS * SEG_H + (BAR_SEGS - 1) * SEG_GAP;
    int start_y = 40 + total_h / 2;
    for (int i = 0; i < BAR_SEGS; i++) {
        segs[i] = box(screen, SEG_W, SEG_H, C_OFF, 2);
        lv_obj_align(segs[i], LV_ALIGN_CENTER,
                     x_bar, start_y - i * (SEG_H + SEG_GAP));
        set_hidden(segs[i], true);
    }
}

static void build_battery(void)
{
    build_battery_side(left_segs,  &left_pct,  &left_lbl,  -1, "L");
    build_battery_side(right_segs, &right_pct, &right_lbl,  1, "R");
}

static void build_bt_dots(void)
{
    for (int i = 0; i < 5; i++) {
        bt_dots[i] = box(screen, 8, 8, C_OFF, 4);
        lv_obj_align(bt_dots[i], LV_ALIGN_BOTTOM_MID,
                     -32 + (i * 16), -14);
        set_hidden(bt_dots[i], true);
    }
}

static void build_hid_indicators(void)
{
    caps_pill = pill(screen, "CAPS", -28);
    num_pill  = pill(screen, "NUM",   28);
}

static void build_wpm(void)
{
    /* WPM — prawy dolny, nad kropkami BT */
    wpm_lbl = label(screen, "0 WPM", &lv_font_montserrat_14, C_WPM);
    lv_obj_align(wpm_lbl, LV_ALIGN_BOTTOM_MID, 0, -36);
    set_hidden(wpm_lbl, true);
}

static void build_output(void)
{
    /* Mała kropka + etykieta "USB" / "BLE" — lewy dolny narożnik */
    output_dot = box(screen, 8, 8, C_BLE, 4);
    lv_obj_align(output_dot, LV_ALIGN_BOTTOM_LEFT, 28, -22);
    set_hidden(output_dot, true);

    output_lbl = label(screen, "BLE", &lv_font_montserrat_14, C_TEXT);
    lv_obj_align(output_lbl, LV_ALIGN_BOTTOM_LEFT, 40, -26);
    set_hidden(output_lbl, true);
}

/* ═══════════════════════════════════════════════════════════════
 * Splash → main transition
 * ═══════════════════════════════════════════════════════════════ */

static void show_main(struct k_work *work)
{
    if (splash_img) {
        lv_obj_del(splash_img);
        splash_img = NULL;
    }
    splash_done = true;
    set_hidden(studio_logo, false);
    refresh_all();
}

/* ═══════════════════════════════════════════════════════════════
 * Event listener
 * ═══════════════════════════════════════════════════════════════ */

static int ft_screen_listener(const zmk_event_t *eh)
{
    /* Połączenie split — obsługiwane zawsze (też przed splash) */
    const struct zmk_split_central_status_changed *sc =
        as_zmk_split_central_status_changed(eh);
    if (sc) {
        if (sc->slot == 0) {
            left_conn = sc->connected;
            if (sc->connected) left_reconnects  = sc->reconnect_count;
            else               bat_left = 0;
        } else if (sc->slot == 1) {
            right_conn = sc->connected;
            if (sc->connected) right_reconnects = sc->reconnect_count;
            else               bat_right = 0;
        }
        if (splash_done) update_battery();
        return ZMK_EV_EVENT_BUBBLE;
    }

    /* Reszta tylko po splash */
    if (!splash_done) return ZMK_EV_EVENT_BUBBLE;

    if (as_zmk_layer_state_changed(eh)) {
        update_layer();
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (as_zmk_ble_active_profile_changed(eh)) {
        update_bt_profile();
        return ZMK_EV_EVENT_BUBBLE;
    }

    const struct zmk_peripheral_battery_state_changed *pb =
        as_zmk_peripheral_battery_state_changed(eh);
    if (pb) {
        if      (pb->source == 0) bat_left  = pb->state_of_charge;
        else if (pb->source == 1) bat_right = pb->state_of_charge;
        update_battery();
        return ZMK_EV_EVENT_BUBBLE;
    }

    const struct zmk_hid_indicators_changed *hi =
        as_zmk_hid_indicators_changed(eh);
    if (hi) {
        hid_indicators = hi->indicators;
        update_hid();
        return ZMK_EV_EVENT_BUBBLE;
    }

    /* WPM — aktualizacja na żywo */
    const struct zmk_wpm_state_changed *wpm_ev =
        as_zmk_wpm_state_changed(eh);
    if (wpm_ev) {
        wpm = wpm_ev->state;
        update_wpm();
        return ZMK_EV_EVENT_BUBBLE;
    }

    /* Zmiana wyjścia USB ↔ BLE */
    if (as_zmk_endpoint_selection_changed(eh)) {
        usb_output = (zmk_endpoints_selected().transport == ZMK_TRANSPORT_USB);
        update_output();
        return ZMK_EV_EVENT_BUBBLE;
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(ft_screen, ft_screen_listener);
ZMK_SUBSCRIPTION(ft_screen, zmk_split_central_status_changed);
ZMK_SUBSCRIPTION(ft_screen, zmk_layer_state_changed);
ZMK_SUBSCRIPTION(ft_screen, zmk_ble_active_profile_changed);
ZMK_SUBSCRIPTION(ft_screen, zmk_peripheral_battery_state_changed);
ZMK_SUBSCRIPTION(ft_screen, zmk_hid_indicators_changed);
ZMK_SUBSCRIPTION(ft_screen, zmk_wpm_state_changed);
ZMK_SUBSCRIPTION(ft_screen, zmk_endpoint_selection_changed);

/* ═══════════════════════════════════════════════════════════════
 * Inicjalizacja ekranu — wywoływana przez ZMK display system
 * ═══════════════════════════════════════════════════════════════ */

lv_obj_t *zmk_display_status_screen(void)
{
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(C_BG), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    build_splash();
    build_header();
    build_battery();
    build_bt_dots();
    build_hid_indicators();
    build_wpm();
    build_output();

    k_work_init_delayable(&splash_work, show_main);
    k_work_schedule(&splash_work, K_MSEC(SPLASH_MS));

    /* Inicjalizuj stan wyjścia */
    usb_output = (zmk_endpoints_selected().transport == ZMK_TRANSPORT_USB);

    return screen;
}
