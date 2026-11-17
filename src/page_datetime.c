#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ui.h"

#define ZONES 26

static const struct {
    const char *id;
    const char *city;
    int minutes;
} zones[ZONES] = {
    {"UTC", "UTC", 0},
    {"Pacific/Honolulu", "Honolulu", -600},
    {"America/Los_Angeles", "Los Angeles", -480},
    {"America/Denver", "Denver", -420},
    {"America/Chicago", "Chicago", -360},
    {"America/New_York", "New York", -300},
    {"America/Sao_Paulo", "Sao Paulo", -180},
    {"Europe/London", "London", 0},
    {"Europe/Berlin", "Berlin", 60},
    {"Europe/Paris", "Paris", 60},
    {"Europe/Warsaw", "Warsaw", 60},
    {"Europe/Stockholm", "Stockholm", 60},
    {"Europe/Kyiv", "Kyiv", 120},
    {"Europe/Chisinau", "Chisinau", 120},
    {"Europe/Helsinki", "Helsinki", 120},
    {"Europe/Istanbul", "Istanbul", 180},
    {"Europe/Moscow", "Moscow", 180},
    {"Asia/Dubai", "Dubai", 240},
    {"Asia/Tashkent", "Tashkent", 300},
    {"Asia/Kolkata", "Kolkata", 330},
    {"Asia/Novosibirsk", "Novosibirsk", 420},
    {"Asia/Shanghai", "Shanghai", 480},
    {"Asia/Tokyo", "Tokyo", 540},
    {"Asia/Vladivostok", "Vladivostok", 600},
    {"Australia/Sydney", "Sydney", 600},
    {"Pacific/Auckland", "Auckland", 720},
};

static const char *const weekdays[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
static const char *const months[] = {"January", "February", "March",     "April",   "May",      "June",
                                     "July",    "August",   "September", "October", "November", "December"};

static struct {
    struct pg_widget *time, *seconds, *date, *zone, *face;
    struct pg_widget *uptime, *source;
    struct pg_widget *tiles[ZONES];
    int h, m, s;
} dt;

static void offset_text(int minutes, char *out, int cap)
{
    int a = abs(minutes);
    snprintf(out, (size_t)cap, "UTC%c%02d:%02d", minutes < 0 ? '-' : '+', a / 60, a % 60);
}

static void hand(struct pg_canvas *c, float cx, float cy, float angle, float len, float width, uint32_t color)
{
    pgc_line(c, cx, cy, cx + pgc_sin(angle) * len, cy - pgc_cos(angle) * len, width, color);
}

static void face_draw(struct pg_widget *w, struct pg_canvas *c, void *data)
{
    (void)data;
    struct pg_rect r = w->rect;
    int cx = r.x + r.w / 2, cy = r.y + r.h / 2;
    int rad = r.w / 2 - 2;
    const float tau = 6.2831853f;
    pgc_circle(c, cx, cy, rad, 0xFFFFFF, 30);
    pgc_circle(c, cx, cy, rad * 8 / 10, 0xFFFFFF, 40);
    for (int i = 0; i < 12; i++) {
        float a = (float)i / 12.0f * tau;
        int x = cx + (int)(pgc_sin(a) * (float)rad * 0.9f), y = cy - (int)(pgc_cos(a) * (float)rad * 0.9f);
        pgc_circle(c, x, y, i % 3 == 0 ? 3 : 2, 0xFFFFFF, 200);
    }
    float fx = (float)cx, fy = (float)cy;
    hand(c, fx, fy, ((float)(dt.h % 12) + (float)dt.m / 60.0f) / 12.0f * tau, (float)rad * 0.42f, 5.0f, 0xFFFFFF);
    hand(c, fx, fy, ((float)dt.m + (float)dt.s / 60.0f) / 60.0f * tau, (float)rad * 0.62f, 3.5f, 0xFFFFFF);
    hand(c, fx, fy, (float)dt.s / 60.0f * tau, (float)rad * 0.68f, 1.5f, UI_RED);
    pgc_circle(c, cx, cy, 4, 0xFFFFFF, 255);
}

static void refresh(void);

static void zone_clicked(int index, void *data)
{
    (void)data;
    if (sa_zone_set(zones[index].id) < 0)
        ui_toast("Could not change the time zone", UI_RED);
    else
        ui_toast("Time zone updated", UI_GREEN);
    refresh();
}

static struct pg_widget *build(void)
{
    struct pg_widget *content;
    struct pg_widget *page = ui_page("Date & time", "Clock, time zone and uptime", &content);

    struct pg_widget *hero = ui_hero(0x48598F, 0x3A6D7C, 26);
    struct pg_widget *row = pg_hbox(24);
    struct pg_widget *text = pg_vbox(8);
    pg_widget_set_expand(text, true, false);
    pg_widget_set_align(text, PG_ALIGN_FILL, PG_ALIGN_CENTER);
    struct pg_widget *clock = pg_hbox(8);
    dt.time = ui_label("--:--", PGF_DIGITS, 0xFFFFFF);
    pg_widget_add(clock, dt.time);
    dt.seconds = ui_label("--", PGF_TITLE, 0xC8CCE0);
    pg_widget_set_align(dt.seconds, PG_ALIGN_START, PG_ALIGN_END);
    pg_widget_set_margins(dt.seconds, 0, 0, 0, 10);
    pg_widget_add(clock, dt.seconds);
    pg_widget_add(text, clock);
    dt.date = ui_label("", PGF_TITLE, 0xFFFFFF);
    pg_widget_add(text, dt.date);
    dt.zone = ui_label("", PGF_BODY, 0xE0E4F0);
    pg_widget_add(text, dt.zone);
    pg_widget_add(row, text);
    dt.face = pg_drawing_new(150, 150, face_draw, NULL, NULL);
    pg_widget_set_align(dt.face, PG_ALIGN_END, PG_ALIGN_CENTER);
    pg_widget_add(row, dt.face);
    pg_widget_add(hero, row);
    pg_widget_add(content, hero);

    struct pg_widget *sys = ui_card(PGI_TIMER_OUTLINE, PG_COLOR_ACCENT, "System clock", NULL, NULL);
    dt.uptime = pg_kv_new("Uptime", "-");
    pg_widget_add(sys, dt.uptime);
    pg_widget_add(sys, ui_hairline());
    dt.source = pg_kv_new("Time source", "-");
    pg_widget_add(sys, dt.source);
    pg_widget_add(content, sys);

    struct pg_widget *tz = ui_card(PGI_EARTH, PG_COLOR_ACCENT, "Time zone", "Daylight saving is handled automatically", NULL);
    struct pg_widget *grid = pg_grid_new(10, 10);
    for (int i = 0; i < ZONES; i++) {
        char off[16];
        offset_text(zones[i].minutes, off, sizeof(off));
        dt.tiles[i] = ui_tile(zones[i].city, off, i, zone_clicked, NULL);
        pg_widget_set_expand(dt.tiles[i], true, false);
        pg_grid_attach(grid, dt.tiles[i], i % 3, i / 3, 1, 1);
    }
    pg_widget_add(tz, grid);
    pg_widget_add(content, tz);
    return page;
}

static void refresh(void)
{
    struct sa_time t;
    if (!sa_time(&t))
        return;
    time_t local = (time_t)(t.unix_seconds + (int64_t)t.tz_minutes * 60);
    struct tm tm;
    if (!gmtime_r(&local, &tm))
        return;
    dt.h = tm.tm_hour;
    dt.m = tm.tm_min;
    dt.s = tm.tm_sec;
    char buf[96], off[16], zone[64];
    snprintf(buf, sizeof(buf), "%02d:%02d", tm.tm_hour, tm.tm_min);
    pg_label_set_text(dt.time, buf);
    snprintf(buf, sizeof(buf), "%02d", tm.tm_sec);
    pg_label_set_text(dt.seconds, buf);
    snprintf(buf, sizeof(buf), "%s, %d %s %d", weekdays[(tm.tm_wday % 7 + 7) % 7], tm.tm_mday,
             months[(tm.tm_mon % 12 + 12) % 12], tm.tm_year + 1900);
    pg_label_set_text(dt.date, buf);
    if (sa_zone_get(zone, sizeof(zone)) < 0)
        snprintf(zone, sizeof(zone), "UTC");
    offset_text(t.tz_minutes, off, sizeof(off));
    snprintf(buf, sizeof(buf), "%s  -  %s", zone, off);
    pg_label_set_text(dt.zone, buf);
    pg_widget_queue_draw(dt.face);
    uint64_t minutes = t.uptime_ms / 60000;
    if (minutes >= 60)
        snprintf(buf, sizeof(buf), "%lluh %llum", (unsigned long long)(minutes / 60), (unsigned long long)(minutes % 60));
    else
        snprintf(buf, sizeof(buf), "%llum", (unsigned long long)minutes);
    pg_kv_set_value(dt.uptime, buf);
    pg_kv_set_value(dt.source, t.source == 1 ? "Hardware RTC" : "Software clock");
    for (int i = 0; i < ZONES; i++)
        ui_tile_set_checked(dt.tiles[i], strcmp(zones[i].id, zone) == 0);
}

const struct page page_datetime = {"DEVICE", "Date & time", PGI_CLOCK_OUTLINE, build, refresh};
