#include <stdio.h>
#include <string.h>

#include "ui.h"

#define MAX_NETS 48

static struct {
    struct pg_widget *badge, *title, *detail;
    struct pg_widget *chip_net, *chip_channel, *chip_security;
    struct pg_widget *disconnect;
    struct pg_widget *wifi_sub, *wifi_list, *wifi_empty;
    struct pg_widget *if_list;
    struct sa_wifi_net nets[MAX_NETS];
    int count;
    char current[33];
    char wifi_sig[2048];
    char if_sig[512];
    char pending[33];
} nw;

static const char *security_text(int s)
{
    switch (s) {
    case SA_SEC_OPEN: return "Open";
    case SA_SEC_WEP: return "WEP";
    case SA_SEC_WPA2: return "WPA2";
    case SA_SEC_WPA3: return "WPA3";
    default: return "WPA2/WPA3";
    }
}

static int signal_icon(int rssi)
{
    if (rssi >= -55)
        return PGI_WIFI_STRENGTH_4;
    if (rssi >= -67)
        return PGI_WIFI_STRENGTH_3;
    if (rssi >= -78)
        return PGI_WIFI_STRENGTH_2;
    return PGI_WIFI_STRENGTH_1;
}

static void refresh(void);

static void disconnect(struct pg_widget *w, void *data)
{
    (void)w;
    (void)data;
    if (sa_wifi_disconnect() < 0)
        ui_toast("Could not disconnect", UI_RED);
    nw.wifi_sig[0] = 0;
    refresh();
}

static void scan(struct pg_widget *w, void *data)
{
    (void)w;
    (void)data;
    if (sa_wifi_scan() < 0)
        ui_toast("Wi-Fi scan failed", UI_RED);
    else
        ui_toast("Scanning...", PG_COLOR_ACCENT);
}

static void connect_to(const char *ssid, const char *password)
{
    char msg[96];
    snprintf(msg, sizeof(msg), "Connecting to %s...", ssid);
    ui_toast(msg, PG_COLOR_ACCENT);
    pg_window_flush(nw.wifi_list->window);
    int rc = sa_wifi_connect(ssid, password);
    snprintf(msg, sizeof(msg), rc < 0 ? "Could not connect to %s" : "Connected to %s", ssid);
    ui_toast(msg, rc < 0 ? UI_RED : UI_GREEN);
    nw.wifi_sig[0] = 0;
    refresh();
}

static void got_password(const char *password, void *data)
{
    (void)data;
    connect_to(nw.pending, password);
}

static void net_clicked(struct pg_widget *w, void *data)
{
    (void)data;
    int i = pg_list_selected(w);
    pg_list_set_selected(w, -1);
    if (i < 0 || i >= nw.count)
        return;
    const struct sa_wifi_net *n = &nw.nets[i];
    if (!n->ssid[0] || strcmp(n->ssid, nw.current) == 0)
        return;
    snprintf(nw.pending, sizeof(nw.pending), "%s", n->ssid);
    if (n->security != SA_SEC_OPEN) {
        char title[64], prompt[80];
        snprintf(title, sizeof(title), "Connect to %s", n->ssid);
        snprintf(prompt, sizeof(prompt), "Password for \"%s\"", n->ssid);
        ui_password(title, prompt, got_password, NULL);
        return;
    }
    connect_to(nw.pending, "");
}

static struct pg_widget *build(void)
{
    struct pg_widget *content;
    struct pg_widget *page = ui_page("Network & Internet", "Wi-Fi, wired adapters and connectivity", &content);

    struct pg_widget *status = ui_card_plain();
    struct pg_widget *row = pg_hbox(16);
    nw.badge = pg_icon_tile_new(PGI_WIFI, 64, PG_COLOR_TEXT3);
    pg_widget_set_align(nw.badge, PG_ALIGN_START, PG_ALIGN_CENTER);
    pg_widget_add(row, nw.badge);
    struct pg_widget *text = pg_vbox(6);
    pg_widget_set_expand(text, true, false);
    pg_widget_set_align(text, PG_ALIGN_FILL, PG_ALIGN_CENTER);
    nw.title = ui_label("", PGF_TITLE, PG_COLOR_TEXT);
    pg_label_set_ellipsize(nw.title, true);
    pg_widget_add(text, nw.title);
    nw.detail = ui_label("", PGF_BODY, PG_COLOR_TEXT3);
    pg_widget_add(text, nw.detail);
    struct pg_widget *chips = pg_hbox(8);
    nw.chip_net = pg_badge_new("Internet", UI_GREEN);
    nw.chip_channel = pg_badge_new("", UI_TEAL);
    nw.chip_security = pg_badge_new("", PG_COLOR_ACCENT);
    pg_widget_add(chips, nw.chip_net);
    pg_widget_add(chips, nw.chip_channel);
    pg_widget_add(chips, nw.chip_security);
    pg_widget_add(text, chips);
    pg_widget_add(row, text);
    nw.disconnect = pg_button_new("Disconnect", PG_NO_ICON, PG_BUTTON_SECONDARY);
    pg_widget_set_align(nw.disconnect, PG_ALIGN_END, PG_ALIGN_CENTER);
    pg_widget_on(nw.disconnect, PG_SIGNAL_CLICKED, disconnect, NULL);
    pg_widget_add(row, nw.disconnect);
    pg_widget_add(status, row);
    pg_widget_add(content, status);

    struct pg_widget *wifi = ui_card_plain();
    struct pg_widget *head = ui_header(PGI_WIFI, PG_COLOR_ACCENT, 34, "Wi-Fi", "Searching...", NULL, &nw.wifi_sub);
    struct pg_widget *scan_button = pg_button_new("Scan", PGI_REFRESH, PG_BUTTON_SECONDARY);
    pg_widget_set_align(scan_button, PG_ALIGN_END, PG_ALIGN_CENTER);
    pg_widget_on(scan_button, PG_SIGNAL_CLICKED, scan, NULL);
    pg_widget_add(head, scan_button);
    pg_widget_add(wifi, head);
    pg_widget_add(wifi, ui_hairline());
    nw.wifi_empty = ui_label("No networks found yet", PGF_BODY, PG_COLOR_TEXT3);
    pg_widget_add(wifi, nw.wifi_empty);
    nw.wifi_list = pg_list_new();
    pg_widget_on(nw.wifi_list, PG_SIGNAL_CHANGED, net_clicked, NULL);
    pg_widget_add(wifi, nw.wifi_list);
    pg_widget_add(content, wifi);

    struct pg_widget *adapters =
        ui_card(PGI_LAN, PG_COLOR_ACCENT, "Adapters", "Network interfaces of this computer", NULL);
    nw.if_list = pg_list_new();
    pg_widget_add(adapters, nw.if_list);
    pg_widget_add(content, adapters);

    sa_wifi_scan();
    return page;
}

static void refresh_status(void)
{
    struct sa_wifi_status st;
    bool ok = sa_wifi_status(&st);
    char buf[128], ip[32];
    nw.current[0] = 0;
    bool connected = ok && st.device && st.connected;
    if (!ok || !st.device) {
        pg_icon_set(nw.badge, PGI_WIFI_OFF, PG_COLOR_TEXT3);
        pg_label_set_text(nw.title, "No Wi-Fi adapter");
        pg_label_set_text(nw.detail, "Wired adapters are listed below");
    } else if (connected) {
        snprintf(nw.current, sizeof(nw.current), "%s", st.ssid);
        pg_icon_set(nw.badge, PGI_WIFI, UI_GREEN);
        snprintf(buf, sizeof(buf), "Connected to %s", st.ssid);
        pg_label_set_text(nw.title, buf);
        if (st.ip) {
            ui_ip(st.ip, ip, sizeof(ip));
            snprintf(buf, sizeof(buf), "%s  -  %s", st.iface, ip);
        } else {
            snprintf(buf, sizeof(buf), "%s", st.iface);
        }
        pg_label_set_text(nw.detail, buf);
        snprintf(buf, sizeof(buf), "Channel %d", st.channel);
        pg_badge_set(nw.chip_channel, buf, UI_TEAL);
        pg_badge_set(nw.chip_security, security_text(st.security), PG_COLOR_ACCENT);
    } else {
        pg_icon_set(nw.badge, PGI_WIFI, PG_COLOR_TEXT3);
        pg_label_set_text(nw.title, "Not connected");
        pg_label_set_text(nw.detail, "Choose a network below");
    }
    pg_widget_set_visible(nw.chip_net, connected && st.ip);
    pg_widget_set_visible(nw.chip_channel, connected);
    pg_widget_set_visible(nw.chip_security, connected);
    pg_widget_set_visible(nw.disconnect, connected);
}

static void refresh_networks(void)
{
    int n = sa_wifi_list(nw.nets, MAX_NETS);
    if (n < 0)
        n = 0;
    char sig[sizeof(nw.wifi_sig)];
    int len = snprintf(sig, sizeof(sig), "%s", nw.current);
    for (int i = 0; i < n && len < (int)sizeof(sig) - 64; i++)
        len += snprintf(sig + len, sizeof(sig) - (size_t)len, "|%s:%d:%d", nw.nets[i].ssid, nw.nets[i].rssi / 6,
                        nw.nets[i].security);
    if (strcmp(sig, nw.wifi_sig) == 0)
        return;
    snprintf(nw.wifi_sig, sizeof(nw.wifi_sig), "%s", sig);
    nw.count = n;
    char buf[64];
    snprintf(buf, sizeof(buf), "%d network%s available", n, n == 1 ? "" : "s");
    pg_label_set_text(nw.wifi_sub, buf);
    pg_widget_set_visible(nw.wifi_empty, n == 0);
    pg_list_clear(nw.wifi_list);
    for (int i = 0; i < n; i++) {
        const struct sa_wifi_net *net = &nw.nets[i];
        char meta[96];
        snprintf(meta, sizeof(meta), "%s  -  ch %d  -  %s GHz  -  %d dBm", security_text(net->security), net->channel,
                 net->channel > 14 ? "5" : "2.4", net->rssi);
        int idx = pg_list_add(nw.wifi_list, signal_icon(net->rssi), net->ssid[0] ? net->ssid : "Hidden network", meta);
        bool current = net->ssid[0] && strcmp(net->ssid, nw.current) == 0;
        pg_list_set_trailing(nw.wifi_list, idx, current ? "Connected" : net->security != SA_SEC_OPEN ? "Secured" : NULL);
    }
}

static void refresh_ifaces(void)
{
    struct sa_iface list[8];
    int n = sa_ifaces(list, 8);
    if (n < 0)
        n = 0;
    char sig[sizeof(nw.if_sig)];
    int len = 0;
    sig[0] = 0;
    for (int i = 0; i < n && len < (int)sizeof(sig) - 64; i++)
        len += snprintf(sig + len, sizeof(sig) - (size_t)len, "|%s:%d:%u:%d", list[i].name, list[i].link, list[i].ip,
                        list[i].has_ip);
    if (strcmp(sig, nw.if_sig) == 0 && n)
        return;
    snprintf(nw.if_sig, sizeof(nw.if_sig), "%s", sig);
    pg_list_clear(nw.if_list);
    for (int i = 0; i < n; i++) {
        char ip[32], meta[96];
        if (list[i].has_ip)
            ui_ip(list[i].ip, ip, sizeof(ip));
        else
            snprintf(ip, sizeof(ip), "no address");
        const uint8_t *m = list[i].mac;
        snprintf(meta, sizeof(meta), "%s  -  MAC %02x:%02x:%02x:%02x:%02x:%02x", ip, m[0], m[1], m[2], m[3], m[4], m[5]);
        int idx = pg_list_add(nw.if_list, strncmp(list[i].name, "wl", 2) == 0 ? PGI_WIFI : PGI_LAN, list[i].name, meta);
        pg_list_set_trailing(nw.if_list, idx, list[i].link ? "Link up" : "No link");
    }
}

static void refresh(void)
{
    refresh_status();
    refresh_networks();
    refresh_ifaces();
}

const struct page page_network = {"SYSTEM", "Network", PGI_WIFI, build, refresh};
