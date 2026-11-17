#include <stdio.h>
#include <string.h>

#include "ui.h"

#define HISTORY 48

static struct {
    struct pg_widget *cpu_value, *cpu_detail;
    struct pg_widget *mem_value, *mem_detail, *mem_ring;
    struct pg_widget *up_value;
    struct pg_widget *load_label, *chart;
    struct pg_widget *spec_cpu, *spec_ram, *spec_display;
    uint32_t history[HISTORY];
    int count;
} ab;

static void trim(char *s)
{
    char *p = s;
    while (*p == ' ')
        p++;
    memmove(s, p, strlen(p) + 1);
    size_t n = strlen(s);
    while (n && s[n - 1] == ' ')
        s[--n] = 0;
}

static void uptime_text(uint64_t ms, char *out, int cap)
{
    uint64_t minutes = ms / 60000, days = minutes / 1440, hours = (minutes / 60) % 24, mins = minutes % 60;
    if (days)
        snprintf(out, (size_t)cap, "%llud %lluh %llum", (unsigned long long)days, (unsigned long long)hours,
                 (unsigned long long)mins);
    else if (hours)
        snprintf(out, (size_t)cap, "%lluh %llum", (unsigned long long)hours, (unsigned long long)mins);
    else
        snprintf(out, (size_t)cap, "%llum", (unsigned long long)mins);
}

static void chart_draw(struct pg_widget *w, struct pg_canvas *c, void *data)
{
    (void)data;
    const struct pg_theme *t = pg_theme();
    struct pg_rect r = w->rect;
    int ax = r.x + 4, ay = r.y + 6, aw = r.w - 8, ah = r.h - 10;
    for (int i = 0; i <= 4; i++)
        pgc_fill(c, ax, ay + ah * i / 4, aw, 1, pgc_mix(t->surface, t->border, 140));
    float step = (float)aw / HISTORY;
    int bw = (int)step - 3 < 2 ? 2 : (int)step - 3;
    for (int i = 0; i < ab.count; i++) {
        int slot = HISTORY - ab.count + i;
        uint32_t v = ab.history[i] > 100 ? 100 : ab.history[i];
        int h = (int)(ah * v / 100);
        if (h < 3)
            h = 3;
        int x = ax + (int)(slot * step);
        pgc_rrect(c, x, ay + ah - h, bw, h, 2, t->accent, i == ab.count - 1 ? 255 : 170);
    }
}

static struct pg_widget *tile(int icon, uint32_t color, const char *title, struct pg_widget **value,
                              struct pg_widget **detail, struct pg_widget *extra)
{
    struct pg_widget *card = ui_card_plain();
    pg_widget_set_expand(card, true, false);
    struct pg_widget *row = pg_hbox(12);
    struct pg_widget *text = pg_vbox(6);
    pg_widget_set_expand(text, true, false);
    struct pg_widget *t = pg_icon_tile_new(icon, 30, color);
    pg_widget_set_align(t, PG_ALIGN_START, PG_ALIGN_START);
    pg_widget_add(text, t);
    pg_widget_add(text, ui_label(title, PGF_SMALL, PG_COLOR_TEXT3));
    *value = ui_label("-", PGF_HEAD, PG_COLOR_TEXT);
    pg_label_set_ellipsize(*value, true);
    pg_widget_add(text, *value);
    *detail = ui_label("", PGF_SMALL, PG_COLOR_TEXT3);
    pg_label_set_wrap(*detail, true);
    pg_widget_add(text, *detail);
    pg_widget_add(row, text);
    if (extra) {
        pg_widget_set_align(extra, PG_ALIGN_END, PG_ALIGN_CENTER);
        pg_widget_add(row, extra);
    }
    pg_widget_add(card, row);
    return card;
}

static struct pg_widget *build(void)
{
    struct pg_widget *content;
    struct pg_widget *page = ui_page("About this PC", "Hardware, memory and system overview", &content);

    struct pg_widget *hero = ui_hero(0x5A6AA8, 0x3D8A99, 26);
    struct pg_widget *hrow = pg_hbox(22);
    struct pg_widget *logo = pg_vbox(0);
    pg_box_set_background(logo, 0x3E4B79, 18);
    pg_widget_set_size(logo, 80, 80);
    struct pg_widget *letter = ui_label("P", PGF_HERO, 0xFFFFFF);
    pg_label_set_text_align(letter, PG_ALIGN_CENTER);
    pg_widget_set_expand(letter, true, true);
    pg_widget_add(logo, letter);
    pg_widget_set_align(logo, PG_ALIGN_START, PG_ALIGN_CENTER);
    pg_widget_add(hrow, logo);
    struct pg_widget *htext = pg_vbox(6);
    pg_widget_set_align(htext, PG_ALIGN_FILL, PG_ALIGN_CENTER);
    pg_widget_add(htext, ui_label("PureC OS", PGF_HERO, 0xFFFFFF));
    char version[64], line[96];
    ui_os_release("VERSION", version, sizeof(version), "unknown");
    snprintf(line, sizeof(line), "Version %s  -  x86_64", version);
    pg_widget_add(htext, ui_label(line, PGF_MEDIUM, 0xF0F0F8));
    pg_widget_add(htext, ui_label("A 64-bit operating system written from scratch in C", PGF_SMALL, 0xC8CCE0));
    pg_widget_add(hrow, htext);
    pg_widget_add(hero, hrow);
    pg_widget_add(content, hero);

    struct pg_widget *tiles = pg_hbox(16);
    struct pg_widget *d1, *d3;
    pg_widget_add(tiles, tile(PGI_CHIP, PG_COLOR_ACCENT, "Processor", &ab.cpu_value, &ab.cpu_detail, NULL));
    ab.mem_ring = pg_ring_new(32, 7);
    pg_ring_set_color(ab.mem_ring, UI_TEAL);
    pg_widget_add(tiles, tile(PGI_MEMORY, PG_COLOR_ACCENT, "Memory", &ab.mem_value, &ab.mem_detail, ab.mem_ring));
    pg_widget_add(tiles, tile(PGI_TIMER_OUTLINE, UI_GREEN, "Uptime", &ab.up_value, &d3, NULL));
    pg_label_set_text(d3, "Since last boot");
    (void)d1;
    pg_widget_add(content, tiles);

    struct pg_widget *header;
    struct pg_widget *load = ui_card(PGI_FLASH, PG_COLOR_ACCENT, "Processor load", "Last 48 seconds", &header);
    ab.load_label = ui_label("0%", PGF_TITLE, PG_COLOR_ACCENT);
    pg_widget_set_align(ab.load_label, PG_ALIGN_END, PG_ALIGN_CENTER);
    pg_widget_add(header, ab.load_label);
    ab.chart = pg_drawing_new(200, 120, chart_draw, NULL, NULL);
    pg_widget_add(load, ab.chart);
    pg_widget_add(content, load);

    struct pg_widget *spec = ui_card(PGI_INFORMATION_OUTLINE, PG_COLOR_ACCENT, "Device specifications", NULL, NULL);
    char os[64], buf[128];
    ui_os_release("PRETTY_NAME", os, sizeof(os), "PureC OS");
    snprintf(buf, sizeof(buf), "%s (x86_64)", os);
    pg_widget_add(spec, pg_kv_new("Operating system", buf));
    ab.spec_cpu = pg_kv_new("Processor", "-");
    pg_widget_add(spec, ab.spec_cpu);
    ab.spec_ram = pg_kv_new("Installed RAM", "-");
    pg_widget_add(spec, ab.spec_ram);
    ab.spec_display = pg_kv_new("Display", "-");
    pg_widget_add(spec, ab.spec_display);
    struct sa_fs fs;
    if (sa_fs_info(&fs))
        snprintf(buf, sizeof(buf), "%s (%s)", fs.root, fs.type);
    else
        snprintf(buf, sizeof(buf), "unknown");
    pg_widget_add(spec, pg_kv_new("System drive", buf));
    pg_widget_add(content, spec);
    return page;
}

static void refresh(void)
{
    char buf[160], a[32], b[32];
    struct sa_cpu cpu;
    if (sa_cpu_info(&cpu)) {
        cpu.name[sizeof(cpu.name) - 1] = 0;
        trim(cpu.name);
        int n = snprintf(buf, sizeof(buf), "%u core%s", cpu.cores, cpu.cores == 1 ? "" : "s");
        if (cpu.freq_hz) {
            unsigned long long centi = (unsigned long long)(cpu.freq_hz / 10000000ull);
            snprintf(buf + n, sizeof(buf) - (size_t)n, " @ %llu.%02llu GHz", centi / 100, centi % 100);
        }
        pg_label_set_text(ab.cpu_value, buf);
        pg_label_set_text(ab.cpu_detail, cpu.name);
        pg_kv_set_value(ab.spec_cpu, cpu.name);
        uptime_text(cpu.uptime_ms, buf, sizeof(buf));
        pg_label_set_text(ab.up_value, buf);
        snprintf(buf, sizeof(buf), "%u%%", cpu.usage);
        pg_label_set_text(ab.load_label, buf);
        if (ab.count == HISTORY) {
            memmove(ab.history, ab.history + 1, sizeof(ab.history[0]) * (HISTORY - 1));
            ab.count--;
        }
        ab.history[ab.count++] = cpu.usage;
        pg_widget_queue_draw(ab.chart);
    }
    struct sa_mem mem;
    if (sa_memory_info(&mem) && mem.total) {
        ui_human_bytes(mem.total, buf, sizeof(buf));
        pg_label_set_text(ab.mem_value, buf);
        pg_kv_set_value(ab.spec_ram, buf);
        ui_human_bytes(mem.used, a, sizeof(a));
        ui_human_bytes(mem.available, b, sizeof(b));
        snprintf(buf, sizeof(buf), "%s used, %s free", a, b);
        pg_label_set_text(ab.mem_detail, buf);
        uint32_t pct = (uint32_t)(mem.used * 100 / mem.total);
        pg_ring_set(ab.mem_ring, pct * 10);
        snprintf(buf, sizeof(buf), "%u%%", pct);
        pg_ring_set_text(ab.mem_ring, buf);
    }
    int sw = 0, sh = 0;
    pg_screen_size(&sw, &sh);
    snprintf(buf, sizeof(buf), "%d x %d, 32-bit color", sw, sh);
    pg_kv_set_value(ab.spec_display, buf);
}

const struct page page_about = {"SYSTEM", "About", PGI_INFORMATION_OUTLINE, build, refresh};
