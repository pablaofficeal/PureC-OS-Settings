#include <stdio.h>
#include <string.h>

#include "ui.h"
#include "vb.h"

#define MODES 8

static const int modes[MODES][2] = {{1024, 768}, {1280, 720}, {1280, 800}, {1366, 768},
                                    {1440, 900}, {1600, 900}, {1920, 1080}, {2560, 1440}};

static struct {
    struct pg_widget *preview, *size, *ratio;
    struct pg_widget *tiles[MODES];
    int sw, sh;
    int pending;
    uint32_t timeout;
} dp;

static int gcd(int a, int b)
{
    while (b) {
        int t = a % b;
        a = b;
        b = t;
    }
    return a ? a : 1;
}

static void ratio_text(int w, int h, char *out, int cap)
{
    static const int known[5][2] = {{16, 9}, {16, 10}, {4, 3}, {5, 4}, {21, 9}};
    if (w <= 0 || h <= 0) {
        snprintf(out, (size_t)cap, "?");
        return;
    }
    for (int i = 0; i < 5; i++) {
        long long lhs = (long long)w * known[i][1] * 1000, rhs = (long long)h * known[i][0] * 1000;
        long long diff = lhs - rhs;
        if (diff < 0)
            diff = -diff;
        if (diff * 1000 <= (long long)h * known[i][1] * 12 * 1000) {
            snprintf(out, (size_t)cap, "%d:%d", known[i][0], known[i][1]);
            return;
        }
    }
    int g = gcd(w, h);
    snprintf(out, (size_t)cap, "%d:%d", w / g, h / g);
}

static void preview_draw(struct pg_widget *w, struct pg_canvas *c, void *data)
{
    (void)data;
    struct pg_rect r = w->rect;
    float ratio = dp.sh > 0 ? (float)dp.sw / (float)dp.sh : 16.0f / 9.0f;
    int maxw = r.w - 10, maxh = r.h - 34;
    int fw = maxw, fh = (int)((float)fw / ratio);
    if (fh > maxh) {
        fh = maxh;
        fw = (int)((float)fh * ratio);
    }
    int fx = r.x + (r.w - fw) / 2, fy = r.y + 4;
    pgc_rrect(c, fx, fy, fw, fh, 10, 0x0B0B12, 255);
    int sx = fx + 9, sy = fy + 9, sw = fw - 18, sh = fh - 18;
    pgc_hgrad(c, sx, sy, sw, sh, 0x4D5C9E, 0x3A8E9C);
    pgc_rrect(c, sx + sw * 16 / 100, sy + sh * 22 / 100, sw * 40 / 100, sh * 40 / 100, 4, 0xFFFFFF, 215);
    pgc_rrect(c, sx + sw * 48 / 100, sy + sh * 38 / 100, sw * 32 / 100, sh * 40 / 100, 4, 0xFFFFFF, 120);
    int bar = sh * 7 / 100;
    pgc_fill(c, sx, sy + sh - bar, sw, bar, 0x30344A);
    int cx = r.x + r.w / 2;
    pgc_fill(c, cx - 30, fy + fh, 60, 14, 0x0B0B12);
    pgc_rrect(c, cx - 60, fy + fh + 14, 120, 8, 4, 0x0B0B12, 255);
}

static void refresh(void);

static bool mode_timeout(void *data)
{
    (void)data;
    dp.timeout = 0;
    if (dp.pending >= 0) {
        dp.pending = -1;
        ui_toast("This mode is not available on this display", UI_RED);
        refresh();
    }
    return false;
}

static void request_mode(int index, void *data)
{
    (void)data;
    refresh();
    int w = modes[index][0], h = modes[index][1];
    if (w == dp.sw && h == dp.sh)
        return;
    if (vb_server_version() < 3) {
        ui_toast("The display server is too old to switch modes", UI_RED);
        return;
    }
    dp.pending = index;
    char msg[64];
    snprintf(msg, sizeof(msg), "Switching to %d x %d...", w, h);
    ui_toast(msg, PG_COLOR_ACCENT);
    vb_set_mode(w, h);
    if (dp.timeout)
        pg_timer_remove(dp.timeout);
    dp.timeout = pg_timer_add(3000, mode_timeout, NULL);
}

static struct pg_widget *build(void)
{
    dp.pending = -1;
    pg_screen_size(&dp.sw, &dp.sh);
    struct pg_widget *content;
    struct pg_widget *page = ui_page("Display", "Resolution of the screen", &content);

    struct pg_widget *hero = ui_card_plain();
    struct pg_widget *row = pg_hbox(28);
    dp.preview = pg_drawing_new(290, 190, preview_draw, NULL, NULL);
    pg_widget_add(row, dp.preview);
    struct pg_widget *text = pg_vbox(8);
    pg_widget_set_align(text, PG_ALIGN_FILL, PG_ALIGN_CENTER);
    pg_widget_add(text, ui_label("CURRENT MODE", PGF_SMALL, PG_COLOR_TEXT3));
    dp.size = ui_label("", PGF_HERO, PG_COLOR_TEXT);
    pg_widget_add(text, dp.size);
    dp.ratio = ui_label("", PGF_BODY, PG_COLOR_TEXT3);
    pg_widget_add(text, dp.ratio);
    struct pg_widget *chips = pg_hbox(8);
    pg_widget_add(chips, pg_badge_new("Live switching", UI_GREEN));
    pg_widget_add(chips, pg_badge_new("Saved for next boot", UI_TEAL));
    pg_widget_add(text, chips);
    pg_widget_add(row, text);
    pg_widget_add(hero, row);
    pg_widget_add(content, hero);

    struct pg_widget *res = ui_card(PGI_MONITOR_SCREENSHOT, PG_COLOR_ACCENT, "Resolution",
                                    "Works in QEMU and VirtualBox; real UEFI keeps the boot mode", NULL);
    struct pg_widget *grid = pg_grid_new(10, 10);
    for (int i = 0; i < MODES; i++) {
        char l1[32], l2[16];
        snprintf(l1, sizeof(l1), "%d x %d", modes[i][0], modes[i][1]);
        ratio_text(modes[i][0], modes[i][1], l2, sizeof(l2));
        dp.tiles[i] = ui_tile(l1, l2, i, request_mode, NULL);
        pg_widget_set_expand(dp.tiles[i], true, false);
        pg_grid_attach(grid, dp.tiles[i], i % 4, i / 4, 1, 1);
    }
    pg_widget_add(res, grid);
    pg_widget_add(content, res);
    return page;
}

static void refresh(void)
{
    int w = 0, h = 0;
    pg_screen_size(&w, &h);
    bool changed = w != dp.sw || h != dp.sh;
    dp.sw = w;
    dp.sh = h;
    if (changed && dp.pending >= 0) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Resolution changed to %d x %d", w, h);
        ui_toast(msg, UI_GREEN);
        dp.pending = -1;
        if (dp.timeout) {
            pg_timer_remove(dp.timeout);
            dp.timeout = 0;
        }
    }
    char buf[64], ratio[16];
    snprintf(buf, sizeof(buf), "%d x %d", w, h);
    pg_label_set_text(dp.size, buf);
    ratio_text(w, h, ratio, sizeof(ratio));
    snprintf(buf, sizeof(buf), "%s  -  32-bit color", ratio);
    pg_label_set_text(dp.ratio, buf);
    for (int i = 0; i < MODES; i++)
        ui_tile_set_checked(dp.tiles[i], modes[i][0] == w && modes[i][1] == h);
    pg_widget_queue_draw(dp.preview);
}

const struct page page_display = {"SYSTEM", "Display", PGI_MONITOR, build, refresh};
