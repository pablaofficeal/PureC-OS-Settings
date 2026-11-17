#include "ui.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int imax(int a, int b) { return a > b ? a : b; }
static int imin(int a, int b) { return a < b ? a : b; }

struct pg_widget *ui_label(const char *text, int font, uint32_t color)
{
    return pg_label_new_font(text, font, color);
}

struct pg_widget *ui_header(int icon, uint32_t color, int size, const char *title, const char *subtitle,
                            struct pg_widget **title_label, struct pg_widget **subtitle_label)
{
    struct pg_widget *h = pg_hbox(12);
    struct pg_widget *tile = pg_icon_tile_new(icon, size, color);
    pg_widget_set_align(tile, PG_ALIGN_START, PG_ALIGN_CENTER);
    pg_widget_add(h, tile);
    struct pg_widget *text = pg_vbox(2);
    pg_widget_set_align(text, PG_ALIGN_FILL, PG_ALIGN_CENTER);
    pg_widget_set_expand(text, true, false);
    struct pg_widget *t = ui_label(title, size >= 56 ? PGF_TITLE : PGF_HEAD, PG_COLOR_TEXT);
    pg_label_set_ellipsize(t, true);
    pg_widget_add(text, t);
    struct pg_widget *s = ui_label(subtitle ? subtitle : "", PGF_SMALL, PG_COLOR_TEXT3);
    pg_label_set_ellipsize(s, true);
    pg_widget_set_visible(s, subtitle != NULL);
    pg_widget_add(text, s);
    pg_widget_add(h, text);
    if (title_label)
        *title_label = t;
    if (subtitle_label)
        *subtitle_label = s;
    return h;
}

struct pg_widget *ui_card_plain(void)
{
    struct pg_widget *card = pg_card_new(NULL, NULL, PG_NO_ICON);
    pg_card_set_padding(card, 20);
    pg_card_set_spacing(card, 10);
    return card;
}

struct pg_widget *ui_card(int icon, uint32_t color, const char *title, const char *subtitle, struct pg_widget **header)
{
    struct pg_widget *card = ui_card_plain();
    struct pg_widget *h = ui_header(icon, color, 34, title, subtitle, NULL, header ? NULL : NULL);
    pg_widget_set_margins(h, 0, 0, 0, 4);
    pg_widget_add(card, h);
    if (header)
        *header = h;
    return card;
}

struct pg_widget *ui_page(const char *title, const char *subtitle, struct pg_widget **content)
{
    struct pg_widget *box = pg_vbox(16);
    pg_box_set_padding(box, 44, 28, 44, 32);
    struct pg_widget *head = pg_vbox(4);
    pg_widget_add(head, ui_label(title, PGF_TITLE, PG_COLOR_TEXT));
    pg_widget_add(head, ui_label(subtitle, PGF_BODY, PG_COLOR_TEXT3));
    pg_widget_set_margins(head, 0, 0, 0, 6);
    pg_widget_add(box, head);
    struct pg_widget *scroll = pg_scroll_new();
    pg_widget_add(scroll, box);
    *content = box;
    return scroll;
}

struct pg_widget *ui_row(const char *label, struct pg_widget *control)
{
    struct pg_widget *h = pg_hbox(12);
    struct pg_widget *l = ui_label(label, PGF_BODY, PG_COLOR_TEXT);
    pg_widget_set_expand(l, true, false);
    pg_widget_set_align(l, PG_ALIGN_FILL, PG_ALIGN_CENTER);
    pg_widget_add(h, l);
    if (control) {
        pg_widget_set_align(control, PG_ALIGN_END, PG_ALIGN_CENTER);
        pg_widget_add(h, control);
    }
    return h;
}

struct pg_widget *ui_hairline(void)
{
    return pg_separator_new();
}

struct hero {
    struct pg_widget base;
    uint32_t left, right;
    int pad;
};

static void hero_measure(struct pg_widget *w, int *width, int *height)
{
    struct hero *h = (struct hero *)w;
    pg_widget_measure(w->first, width, height);
    *width += 2 * h->pad;
    *height += 2 * h->pad;
}

static int hero_hfw(struct pg_widget *w, int width)
{
    struct hero *h = (struct hero *)w;
    return (w->first ? pg_widget_height_for_width(w->first, width - 2 * h->pad) : 0) + 2 * h->pad;
}

static void hero_arrange(struct pg_widget *w)
{
    struct hero *h = (struct hero *)w;
    if (w->first)
        pg_widget_place(w->first, w->rect.x + h->pad, w->rect.y + h->pad, w->rect.w - 2 * h->pad,
                        w->rect.h - 2 * h->pad);
}

static void hero_draw(struct pg_widget *w, struct pg_canvas *c)
{
    struct hero *h = (struct hero *)w;
    pgc_rrect_grad(c, w->rect.x, w->rect.y, w->rect.w, w->rect.h, 14, h->left, h->right, true);
}

static const struct pg_widget_class hero_class = {
    .name = "hero",
    .size = sizeof(struct hero),
    .measure = hero_measure,
    .height_for_width = hero_hfw,
    .arrange = hero_arrange,
    .draw = hero_draw,
};

struct pg_widget *ui_hero(uint32_t left, uint32_t right, int padding)
{
    struct hero *h = (struct hero *)pg_widget_new(&hero_class);
    h->left = left;
    h->right = right;
    h->pad = padding;
    return &h->base;
}

struct tile {
    struct pg_widget base;
    char line1[64];
    char line2[64];
    bool checked;
    int index;
    ui_tile_fn fn;
    void *data;
};

static void tile_measure(struct pg_widget *w, int *width, int *height)
{
    struct tile *t = (struct tile *)w;
    *width = imax(pgc_text_w(PGF_MEDIUM, t->line1), pgc_text_w(PGF_SMALL, t->line2)) + 28;
    *height = t->line2[0] ? 56 : 44;
}

static void tile_draw(struct pg_widget *w, struct pg_canvas *c)
{
    struct tile *t = (struct tile *)w;
    const struct pg_theme *p = pg_theme();
    struct pg_rect r = w->rect;
    uint32_t bg = t->checked ? pgc_mix(p->surface, p->accent, 40) : (w->hovered ? p->surface2 : p->bg);
    pgc_rrect(c, r.x, r.y, r.w, r.h, 10, bg, 255);
    pgc_rrect_stroke(c, r.x, r.y, r.w, r.h, 10, t->checked ? p->accent : p->border, 255);
    int y = r.y + (r.h - (t->line2[0] ? pgc_font_h(PGF_MEDIUM) + 4 + pgc_font_h(PGF_SMALL) : pgc_font_h(PGF_MEDIUM))) / 2;
    pgc_text_fit(c, PGF_MEDIUM, r.x + 14, y, r.w - 24, t->line1, t->checked ? p->text : p->text2);
    if (t->line2[0])
        pgc_text_fit(c, PGF_SMALL, r.x + 14, y + pgc_font_h(PGF_MEDIUM) + 4, r.w - 24, t->line2, p->text3);
    pg_widget_draw_focus(w, c, 10);
}

static bool tile_event(struct pg_widget *w, const struct pg_event *e)
{
    struct tile *t = (struct tile *)w;
    bool fire = false;
    if (e->type == PG_EVENT_POINTER_DOWN)
        return e->button == PG_BUTTON_LEFT;
    if (e->type == PG_EVENT_POINTER_UP) {
        fire = w->pressed && e->x >= w->rect.x && e->y >= w->rect.y && e->x < w->rect.x + w->rect.w &&
               e->y < w->rect.y + w->rect.h;
    } else if (e->type == PG_EVENT_KEY_DOWN && (e->key == PG_KEY_SPACE || e->key == PG_KEY_ENTER)) {
        fire = true;
    } else {
        return false;
    }
    if (fire && t->fn)
        t->fn(t->index, t->data);
    return true;
}

static const struct pg_widget_class tile_class = {
    .name = "tile",
    .size = sizeof(struct tile),
    .measure = tile_measure,
    .draw = tile_draw,
    .event = tile_event,
};

struct pg_widget *ui_tile(const char *line1, const char *line2, int index, ui_tile_fn fn, void *data)
{
    struct tile *t = (struct tile *)pg_widget_new(&tile_class);
    snprintf(t->line1, sizeof(t->line1), "%s", line1 ? line1 : "");
    snprintf(t->line2, sizeof(t->line2), "%s", line2 ? line2 : "");
    t->index = index;
    t->fn = fn;
    t->data = data;
    t->base.focusable = true;
    return &t->base;
}

void ui_tile_set_checked(struct pg_widget *w, bool checked)
{
    if (!w || w->cls != &tile_class)
        return;
    struct tile *t = (struct tile *)w;
    if (t->checked != checked) {
        t->checked = checked;
        pg_widget_queue_draw(w);
    }
}

void ui_human_bytes(uint64_t bytes, char *out, int cap)
{
    static const char *const units[] = {"B", "KB", "MB", "GB", "TB"};
    double v = (double)bytes;
    int unit = 0;
    while (v >= 1024.0 && unit < 4) {
        v /= 1024.0;
        unit++;
    }
    if (!unit) {
        snprintf(out, (size_t)cap, "%llu B", (unsigned long long)bytes);
        return;
    }
    unsigned long long tenths = (unsigned long long)(v * 10.0 + 0.5);
    snprintf(out, (size_t)cap, "%llu.%llu %s", tenths / 10, tenths % 10, units[unit]);
}

void ui_os_release(const char *key, char *out, int cap, const char *fallback)
{
    snprintf(out, (size_t)cap, "%s", fallback);
    int fd = open("/etc/os-release", O_RDONLY);
    if (fd < 0)
        return;
    char buf[2048];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0)
        return;
    buf[n] = 0;
    size_t klen = strlen(key);
    for (char *line = buf; line && *line;) {
        char *end = strchr(line, '\n');
        if (end)
            *end = 0;
        if (strncmp(line, key, klen) == 0 && line[klen] == '=') {
            char *v = line + klen + 1;
            while (*v == ' ' || *v == '\t')
                v++;
            size_t len = strlen(v);
            while (len && (v[len - 1] == ' ' || v[len - 1] == '\r' || v[len - 1] == '\t'))
                v[--len] = 0;
            if (len >= 2 && v[0] == '"' && v[len - 1] == '"') {
                v[len - 1] = 0;
                v++;
            }
            snprintf(out, (size_t)cap, "%s", v);
            return;
        }
        line = end ? end + 1 : NULL;
    }
}

void ui_ip(uint32_t ip, char *out, int cap)
{
    snprintf(out, (size_t)cap, "%u.%u.%u.%u", ip & 255u, (ip >> 8) & 255u, (ip >> 16) & 255u, (ip >> 24) & 255u);
}

struct overlay {
    struct pg_widget base;
    struct pg_widget *content;
    struct pg_widget *scrim;
    struct pg_widget *toast;
    struct pg_widget *toast_label;
    int toast_left;
    uint32_t toast_timer;
};

static struct overlay *overlay;

static void overlay_measure(struct pg_widget *w, int *width, int *height)
{
    pg_widget_measure(((struct overlay *)w)->content, width, height);
}

static void overlay_arrange(struct pg_widget *w)
{
    struct overlay *o = (struct overlay *)w;
    struct pg_rect r = w->rect;
    pg_widget_place(o->content, r.x, r.y, r.w, r.h);
    if (o->scrim->visible)
        pg_widget_place(o->scrim, r.x, r.y, r.w, r.h);
    if (o->toast->visible) {
        int tw, th;
        pg_widget_measure(o->toast, &tw, &th);
        int area_x = r.x + o->toast_left, area_w = r.w - o->toast_left;
        tw = imin(tw, area_w - 32);
        pg_widget_place(o->toast, area_x + (area_w - tw) / 2, r.y + r.h - th - 24, tw, th);
    }
}

static const struct pg_widget_class overlay_class = {
    .name = "overlay",
    .size = sizeof(struct overlay),
    .measure = overlay_measure,
    .arrange = overlay_arrange,
};

static void scrim_draw(struct pg_widget *w, struct pg_canvas *c)
{
    pgc_fill_a(c, w->rect.x, w->rect.y, w->rect.w, w->rect.h, 0x000000, 120);
}

static void scrim_arrange(struct pg_widget *w)
{
    struct pg_widget *d = w->first;
    if (!d)
        return;
    int dw = 420, dh = pg_widget_height_for_width(d, dw);
    pg_widget_place(d, w->rect.x + (w->rect.w - dw) / 2, w->rect.y + (w->rect.h - dh) / 2, dw, dh);
}

static void dialog_close(void);

static bool scrim_event(struct pg_widget *w, const struct pg_event *e)
{
    (void)w;
    if (e->type == PG_EVENT_KEY_DOWN && e->key == PG_KEY_ESCAPE) {
        dialog_close();
        return true;
    }
    return e->type == PG_EVENT_POINTER_DOWN || e->type == PG_EVENT_POINTER_UP || e->type == PG_EVENT_WHEEL ||
           e->type == PG_EVENT_POINTER_MOVE;
}

static const struct pg_widget_class scrim_class = {
    .name = "scrim",
    .arrange = scrim_arrange,
    .draw = scrim_draw,
    .event = scrim_event,
};

struct pg_widget *ui_overlay_new(struct pg_widget *content)
{
    overlay = (struct overlay *)pg_widget_new(&overlay_class);
    overlay->content = content;
    pg_widget_add(&overlay->base, content);
    overlay->scrim = pg_widget_new(&scrim_class);
    overlay->scrim->visible = false;
    pg_widget_add(&overlay->base, overlay->scrim);
    overlay->toast = pg_hbox(0);
    pg_box_set_padding(overlay->toast, 16, 10, 16, 10);
    pg_box_set_background(overlay->toast, PG_COLOR_SURFACE3, 10);
    overlay->toast_label = ui_label("", PGF_MEDIUM, UI_GREEN);
    pg_label_set_ellipsize(overlay->toast_label, false);
    pg_widget_add(overlay->toast, overlay->toast_label);
    overlay->toast->visible = false;
    pg_widget_add(&overlay->base, overlay->toast);
    return &overlay->base;
}

void ui_overlay_set_toast_offset(int left)
{
    if (overlay)
        overlay->toast_left = left;
}

static bool toast_hide(void *data)
{
    (void)data;
    overlay->toast_timer = 0;
    pg_widget_set_visible(overlay->toast, false);
    pg_widget_queue_layout(&overlay->base);
    if (overlay->base.window)
        pg_window_queue_draw(overlay->base.window);
    return false;
}

void ui_toast(const char *text, uint32_t color)
{
    if (!overlay)
        return;
    pg_label_set_text(overlay->toast_label, text);
    pg_label_set_color(overlay->toast_label, color ? color : UI_GREEN);
    pg_widget_set_visible(overlay->toast, true);
    if (overlay->toast_timer)
        pg_timer_remove(overlay->toast_timer);
    overlay->toast_timer = pg_timer_add(2600, toast_hide, NULL);
    pg_widget_queue_layout(&overlay->base);
    if (overlay->base.window)
        pg_window_queue_draw(overlay->base.window);
}

static struct {
    struct pg_widget *entry;
    ui_confirm_fn confirm;
    ui_password_fn password;
    void *data;
} dialog;

static void dialog_close(void)
{
    pg_widget_clear(overlay->scrim);
    pg_widget_set_visible(overlay->scrim, false);
    dialog.entry = NULL;
    if (overlay->base.window)
        pg_window_queue_draw(overlay->base.window);
}

static void dialog_cancel(struct pg_widget *w, void *data)
{
    (void)w;
    (void)data;
    dialog_close();
}

static void dialog_accept(struct pg_widget *w, void *data)
{
    (void)w;
    (void)data;
    if (dialog.password) {
        char pw[128];
        snprintf(pw, sizeof(pw), "%s", dialog.entry ? pg_entry_text(dialog.entry) : "");
        ui_password_fn fn = dialog.password;
        void *d = dialog.data;
        dialog_close();
        if (pw[0])
            fn(pw, d);
        memset(pw, 0, sizeof(pw));
        return;
    }
    ui_confirm_fn fn = dialog.confirm;
    void *d = dialog.data;
    dialog_close();
    if (fn)
        fn(d);
}

static struct pg_widget *dialog_open(const char *title, const char *text, const char *action, bool danger,
                                     bool password)
{
    dialog_close();
    struct pg_widget *card = pg_card_new(NULL, NULL, PG_NO_ICON);
    pg_card_set_padding(card, 24);
    pg_card_set_spacing(card, 12);
    pg_widget_add(card, ui_label(title, PGF_HEAD, PG_COLOR_TEXT));
    if (text && *text) {
        struct pg_widget *t = ui_label(text, PGF_BODY, PG_COLOR_TEXT2);
        pg_label_set_wrap(t, true);
        pg_widget_add(card, t);
    }
    if (password) {
        dialog.entry = pg_entry_new("Password");
        pg_entry_set_password(dialog.entry, true);
        pg_widget_on(dialog.entry, PG_SIGNAL_ACTIVATE, dialog_accept, NULL);
        pg_widget_add(card, dialog.entry);
    }
    struct pg_widget *buttons = pg_hbox(10);
    pg_widget_set_margins(buttons, 0, 6, 0, 0);
    pg_widget_add(buttons, pg_spacer_new());
    struct pg_widget *cancel = pg_button_new("Cancel", PG_NO_ICON, PG_BUTTON_SECONDARY);
    pg_widget_on(cancel, PG_SIGNAL_CLICKED, dialog_cancel, NULL);
    pg_widget_add(buttons, cancel);
    struct pg_widget *ok = pg_button_new(action, PG_NO_ICON, danger ? PG_BUTTON_DANGER : PG_BUTTON_PRIMARY);
    pg_widget_on(ok, PG_SIGNAL_CLICKED, dialog_accept, NULL);
    pg_widget_add(buttons, ok);
    pg_widget_add(card, buttons);
    pg_widget_add(overlay->scrim, card);
    pg_widget_set_visible(overlay->scrim, true);
    pg_widget_grab_focus(dialog.entry ? dialog.entry : ok);
    if (overlay->base.window)
        pg_window_queue_draw(overlay->base.window);
    return card;
}

void ui_confirm(const char *title, const char *text, const char *action, ui_confirm_fn fn, void *data)
{
    dialog_open(title, text, action, true, false);
    dialog.confirm = fn;
    dialog.password = NULL;
    dialog.data = data;
}

void ui_password(const char *title, const char *prompt, ui_password_fn fn, void *data)
{
    dialog_open(title, prompt, "Connect", false, true);
    dialog.confirm = NULL;
    dialog.password = fn;
    dialog.data = data;
}
