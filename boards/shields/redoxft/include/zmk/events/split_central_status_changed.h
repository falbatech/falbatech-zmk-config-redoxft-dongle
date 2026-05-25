#pragma once

/*
 * Redox FT — event emitowany gdy peryferyjna połówka łączy/rozłącza się
 * SPDX-License-Identifier: MIT
 *
 * Pola:
 *   slot            — indeks połówki (0 = lewa, 1 = prawa)
 *   connected       — true = połączona, false = rozłączona
 *   reconnect_count — ile razy ta połówka była reconnectowana od startu
 *                     (rośnie tylko przy connected=true po wcześniejszym disconnect)
 *                     Przydatne do diagnozowania niestabilnych połączeń BT.
 */

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>

struct zmk_split_central_status_changed {
    uint8_t slot;
    bool    connected;
    uint8_t reconnect_count;
};

ZMK_EVENT_DECLARE(zmk_split_central_status_changed);
