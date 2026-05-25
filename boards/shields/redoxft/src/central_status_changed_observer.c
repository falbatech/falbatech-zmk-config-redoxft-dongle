/*
 * Redox FT Dongle — obserwator połączeń split central
 * SPDX-License-Identifier: MIT
 *
 * Śledzi połączenia peryferyjnych połówek (lewo/prawo) przez BT conn callbacks.
 * Emituje zmk_split_central_status_changed z:
 *   - numerem slotu (0=lewa, 1=prawa)
 *   - stanem połączenia
 *   - licznikiem reconnectów (rośnie przy każdym ponownym połączeniu)
 *
 * Zarządzanie slotami jest thread-safe (mutex).
 */

#include <zephyr/types.h>
#include <zephyr/init.h>
#include <zephyr/sys/mutex.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/ble.h>
#include <zmk/events/split_central_status_changed.h>

#ifndef ZMK_SPLIT_BLE_PERIPHERAL_COUNT
#define ZMK_SPLIT_BLE_PERIPHERAL_COUNT CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS
#endif

/* ── Stan slotu ─────────────────────────────────────────────────── */

enum ft_slot_state {
    FT_SLOT_FREE,
    FT_SLOT_CONNECTED,
};

struct ft_slot {
    enum ft_slot_state  state;
    struct bt_conn     *conn;
    uint8_t             reconnect_count; /* rośnie przy każdym (re)connect */
};

static struct ft_slot slots[ZMK_SPLIT_BLE_PERIPHERAL_COUNT];
static struct sys_mutex slots_mtx;

/* ── Wewnętrzne operacje na slotach ─────────────────────────────── */

/* Zwraca indeks slotu dla danego conn, lub -EINVAL jeśli nie znaleziono.
 * Wywołujący musi trzymać slots_mtx. */
static int slot_idx_for_conn_locked(struct bt_conn *conn)
{
    for (int i = 0; i < ZMK_SPLIT_BLE_PERIPHERAL_COUNT; i++) {
        if (slots[i].conn == conn) return i;
    }
    return -EINVAL;
}

/* Rezerwuje slot dla nowego połączenia.
 * Zwraca indeks albo -ENOMEM jeśli brak wolnych slotów. */
static int slot_reserve_locked(struct bt_conn *conn)
{
#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_PREF_WEAK_BOND)
    /* Tryb weak bond: zajmij pierwszy wolny slot */
    for (int i = 0; i < ZMK_SPLIT_BLE_PERIPHERAL_COUNT; i++) {
        if (slots[i].state == FT_SLOT_FREE) {
            slots[i].conn  = conn;
            slots[i].state = FT_SLOT_CONNECTED;
            slots[i].reconnect_count++;
            return i;
        }
    }
#else
    /* Tryb normalny: mapuj adres BT → stały slot przez zmk_ble */
    int i = zmk_ble_put_peripheral_addr(bt_conn_get_dst(conn));
    if (i >= 0 && slots[i].state == FT_SLOT_FREE) {
        slots[i].conn  = conn;
        slots[i].state = FT_SLOT_CONNECTED;
        slots[i].reconnect_count++;
        return i;
    }
#endif
    return -ENOMEM;
}

/* Zwalnia slot dla danego conn. */
static int slot_release_locked(struct bt_conn *conn)
{
    int i = slot_idx_for_conn_locked(conn);
    if (i < 0) return -EINVAL;

    LOG_DBG("Slot %d zwolniony (reconnect_count=%d)",
            i, slots[i].reconnect_count);

    slots[i].conn  = NULL;
    slots[i].state = FT_SLOT_FREE;
    return i; /* zwracamy indeks — potrzebny do emisji eventu */
}

/* ── BT conn callbacks ──────────────────────────────────────────── */

static void on_connected(struct bt_conn *conn, uint8_t err)
{
    char addr[BT_ADDR_LE_STR_LEN];
    struct bt_conn_info info;

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    bt_conn_get_info(conn, &info);

    /* Interesują nas tylko połączenia jako central */
    if (info.role != BT_CONN_ROLE_CENTRAL) {
        LOG_DBG("Pomijam rolę %d dla %s", info.role, addr);
        return;
    }

    if (err) {
        LOG_ERR("Błąd połączenia z %s: %u", addr, err);
        sys_mutex_lock(&slots_mtx, K_FOREVER);
        slot_release_locked(conn);
        sys_mutex_unlock(&slots_mtx);
        return;
    }

    LOG_INF("Połączono: %s  interval=%d latency=%d phy=%d",
            addr, info.le.interval, info.le.latency,
            info.le.phy ? info.le.phy->rx_phy : 0);

    sys_mutex_lock(&slots_mtx, K_FOREVER);
    int idx = slot_reserve_locked(conn);
    uint8_t rcount = (idx >= 0) ? slots[idx].reconnect_count : 0;
    sys_mutex_unlock(&slots_mtx);

    if (idx < 0) {
        LOG_ERR("Brak wolnego slotu dla %s (err %d)", addr, idx);
        return;
    }

    raise_zmk_split_central_status_changed(
        (struct zmk_split_central_status_changed){
            .slot            = (uint8_t)idx,
            .connected       = true,
            .reconnect_count = rcount,
        });
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("Rozłączono: %s (reason=0x%02x)", addr, reason);

    sys_mutex_lock(&slots_mtx, K_FOREVER);
    int idx = slot_release_locked(conn);
    sys_mutex_unlock(&slots_mtx);

    raise_zmk_split_central_status_changed(
        (struct zmk_split_central_status_changed){
            .slot            = (uint8_t)(idx >= 0 ? idx : 0),
            .connected       = false,
            .reconnect_count = 0,
        });
}

static struct bt_conn_cb ft_conn_cb = {
    .connected    = on_connected,
    .disconnected = on_disconnected,
};

/* ── Inicjalizacja ──────────────────────────────────────────────── */

static int ft_central_observer_init(void)
{
    sys_mutex_init(&slots_mtx);
    memset(slots, 0, sizeof(slots));
    bt_conn_cb_register(&ft_conn_cb);
    LOG_INF("FT central observer gotowy (%d slotów)",
            ZMK_SPLIT_BLE_PERIPHERAL_COUNT);
    return 0;
}

SYS_INIT(ft_central_observer_init, APPLICATION, CONFIG_ZMK_BLE_INIT_PRIORITY);
