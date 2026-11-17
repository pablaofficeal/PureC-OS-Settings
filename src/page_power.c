#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ui.h"

static struct {
    struct pg_widget *battery, *badge, *percent, *state, *bar;
    struct pg_widget *thermal, *zone, *fan;
} pw;

static void do_power(void *data)
{
    sa_power(data != NULL);
}

static void restart(struct pg_widget *w, void *data)
{
    (void)w;
    (void)data;
    ui_confirm("Restart now?", "The computer restarts right away.", "Restart", do_power, (void *)1);
}

static void power_off(struct pg_widget *w, void *data)
{
    (void)w;
    (void)data;
    ui_confirm("Power off now?", "The computer shuts down right away.", "Power off", do_power, NULL);
}

static struct pg_widget *build(void)
{
    struct pg_widget *content;
    struct pg_widget *page = ui_page("Power", "Battery, temperature and shutdown", &content);

    pw.battery = ui_card_plain();
    struct pg_widget *row = pg_hbox(16);
    pw.badge = pg_icon_tile_new(PGI_BATTERY, 64, PG_COLOR_ACCENT);
    pg_widget_set_align(pw.badge, PG_ALIGN_START, PG_ALIGN_CENTER);
    pg_widget_add(row, pw.badge);
    struct pg_widget *text = pg_vbox(6);
    pg_widget_set_expand(text, true, false);
    pg_widget_set_align(text, PG_ALIGN_FILL, PG_ALIGN_CENTER);
    pw.percent = ui_label("", PGF_HERO, PG_COLOR_TEXT);
    pg_widget_add(text, pw.percent);
    pw.state = ui_label("", PGF_BODY, PG_COLOR_TEXT3);
    pg_widget_add(text, pw.state);
    pg_widget_add(row, text);
    pg_widget_add(pw.battery, row);
    pw.bar = pg_progress_new();
    pg_widget_set_size(pw.bar, -1, 10);
    pg_widget_add(pw.battery, pw.bar);
    pg_widget_set_visible(pw.battery, false);
    pg_widget_add(content, pw.battery);

    pw.thermal = ui_card(PGI_THERMOMETER, UI_PEACH, "Temperature", NULL, NULL);
    pw.zone = pg_kv_new("Thermal zone", "-");
    pg_widget_add(pw.thermal, pw.zone);
    pw.fan = pg_kv_new("Fan", "-");
    pg_widget_add(pw.thermal, pw.fan);
    pg_widget_set_visible(pw.thermal, false);
    pg_widget_add(content, pw.thermal);

    struct pg_widget *shut =
        ui_card(PGI_POWER, UI_RED, "Shut down or restart", "Open documents in other apps may be lost", NULL);
    struct pg_widget *buttons = pg_hbox(10);
    struct pg_widget *r = pg_button_new("Restart", PGI_RESTART, PG_BUTTON_SECONDARY);
    pg_widget_on(r, PG_SIGNAL_CLICKED, restart, NULL);
    pg_widget_add(buttons, r);
    struct pg_widget *off = pg_button_new("Power off", PGI_POWER, PG_BUTTON_DANGER);
    pg_widget_on(off, PG_SIGNAL_CLICKED, power_off, NULL);
    pg_widget_add(buttons, off);
    pg_widget_set_align(buttons, PG_ALIGN_START, PG_ALIGN_START);
    pg_widget_add(shut, buttons);
    pg_widget_add(content, shut);
    return page;
}

static void refresh(void)
{
    char buf[96];
    struct sa_battery b;
    bool have = sa_battery(&b);
    pg_widget_set_visible(pw.battery, have);
    if (have) {
        uint32_t color = b.charging ? UI_GREEN : b.percent <= 15 ? UI_RED : PG_COLOR_ACCENT;
        pg_icon_set(pw.badge, b.charging ? PGI_BATTERY_CHARGING : PGI_BATTERY, color);
        snprintf(buf, sizeof(buf), "%u%%", b.percent);
        pg_label_set_text(pw.percent, buf);
        if (b.charging)
            snprintf(buf, sizeof(buf), "Charging");
        else if (b.minutes)
            snprintf(buf, sizeof(buf), "On battery  -  about %uh %um left", b.minutes / 60, b.minutes % 60);
        else
            snprintf(buf, sizeof(buf), "On battery");
        pg_label_set_text(pw.state, buf);
        pg_progress_set(pw.bar, (b.percent > 100 ? 100 : b.percent) * 10);
    }
    struct sa_thermal t;
    bool hot = sa_thermal(&t);
    pg_widget_set_visible(pw.thermal, hot);
    if (hot) {
        snprintf(buf, sizeof(buf), "%s  %d.%d C", t.zone, t.deci_c / 10, abs(t.deci_c % 10));
        pg_kv_set_value(pw.zone, buf);
        if (t.fan_rpm)
            snprintf(buf, sizeof(buf), "%u rpm", t.fan_rpm);
        else
            snprintf(buf, sizeof(buf), "not reported");
        pg_kv_set_value(pw.fan, buf);
    }
}

const struct page page_power = {"DEVICE", "Power", PGI_POWER, build, refresh};
