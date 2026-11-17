#include <stdio.h>
#include <string.h>

#include "ui.h"

#define SIDEBAR_W 252
#define PAGE_COUNT 8

static const struct page *const pages[PAGE_COUNT] = {
    &page_about, &page_network, &page_sound, &page_display,
    &page_appearance, &page_storage, &page_datetime, &page_power,
};

static const char *const sections[] = {"SYSTEM", "PERSONAL", "DEVICE"};

static struct {
    struct pg_window *win;
    struct pg_widget *stack;
    struct pg_widget *lists[3];
    int first[3];
    int current;
    char user[32];
    uint32_t uid;
} app;

static void refresh_current(void)
{
    if (pages[app.current]->refresh)
        pages[app.current]->refresh();
}

static bool tick(void *data)
{
    (void)data;
    refresh_current();
    return true;
}

static void show_page(int index)
{
    if (index < 0 || index >= PAGE_COUNT)
        return;
    app.current = index;
    pg_stack_set_index(app.stack, index);
    for (int s = 0; s < 3; s++) {
        int local = index - app.first[s];
        int count = pg_list_count(app.lists[s]);
        pg_list_set_selected(app.lists[s], local >= 0 && local < count ? local : -1);
    }
    refresh_current();
}

static void nav_changed(struct pg_widget *w, void *data)
{
    int s = (int)(intptr_t)data;
    int i = pg_list_selected(w);
    if (i >= 0)
        show_page(app.first[s] + i);
}

static void logo_draw(struct pg_widget *w, struct pg_canvas *c, void *data)
{
    (void)data;
    struct pg_rect r = w->rect;
    int s = r.h;
    pgc_rrect_grad(c, r.x, r.y, s, s, s / 4, 0x89B4FA, 0x9D7CF0, true);
    int px = pgc_icon_px(PG_ICON_S);
    pgc_icon(c, PGI_COG, PG_ICON_S, r.x + (s - px) / 2, r.y + (s - px) / 2, 0xFFFFFF);
}

static void avatar_draw(struct pg_widget *w, struct pg_canvas *c, void *data)
{
    (void)data;
    struct pg_rect r = w->rect;
    int s = r.h;
    pgc_circle(c, r.x + s / 2, r.y + s / 2, s / 2, 0x586B9A, 255);
    char letter[2] = {app.user[0] ? app.user[0] : '?', 0};
    if (letter[0] >= 'a' && letter[0] <= 'z')
        letter[0] = (char)(letter[0] - 'a' + 'A');
    pgc_text_center(c, PGF_HEAD, r.x, r.y + (s - pgc_font_h(PGF_HEAD)) / 2, s, letter, 0xFFFFFF);
}

static struct pg_widget *sidebar(void)
{
    struct pg_widget *side = pg_vbox(2);
    pg_box_set_padding(side, 14, 20, 14, 16);
    pg_box_set_background(side, PG_COLOR_SIDEBAR, 0);
    pg_widget_set_size(side, SIDEBAR_W, -1);

    struct pg_widget *head = pg_hbox(12);
    struct pg_widget *logo = pg_drawing_new(40, 40, logo_draw, NULL, NULL);
    pg_widget_add(head, logo);
    struct pg_widget *names = pg_vbox(2);
    pg_widget_set_align(names, PG_ALIGN_FILL, PG_ALIGN_CENTER);
    pg_widget_add(names, ui_label("Settings", PGF_HEAD, PG_COLOR_TEXT));
    char os[64];
    ui_os_release("PRETTY_NAME", os, sizeof(os), "PureC OS");
    struct pg_widget *osl = ui_label(os, PGF_SMALL, PG_COLOR_TEXT3);
    pg_label_set_ellipsize(osl, true);
    pg_widget_add(names, osl);
    pg_widget_add(head, names);
    pg_widget_set_margins(head, 0, 0, 0, 16);
    pg_widget_add(side, head);

    if (!sa_user(app.user, sizeof(app.user), &app.uid))
        snprintf(app.user, sizeof(app.user), "user");
    struct pg_widget *user = pg_hbox(10);
    pg_box_set_padding(user, 10, 8, 10, 8);
    pg_box_set_background(user, PG_COLOR_SURFACE, 10);
    pg_widget_add(user, pg_drawing_new(32, 32, avatar_draw, NULL, NULL));
    struct pg_widget *who = pg_vbox(1);
    pg_widget_set_align(who, PG_ALIGN_FILL, PG_ALIGN_CENTER);
    pg_widget_add(who, ui_label(app.user, PGF_MEDIUM, PG_COLOR_TEXT));
    pg_widget_add(who, ui_label(app.uid == 0 ? "Administrator" : "Standard user", PGF_SMALL, PG_COLOR_TEXT3));
    pg_widget_add(user, who);
    pg_widget_set_margins(user, 0, 0, 0, 12);
    pg_widget_add(side, user);

    for (int s = 0, p = 0; s < 3; s++) {
        struct pg_widget *label = ui_label(sections[s], PGF_SMALL, PG_COLOR_TEXT3);
        pg_widget_set_margins(label, 12, 10, 0, 4);
        pg_widget_add(side, label);
        app.lists[s] = pg_list_new();
        pg_list_set_sidebar(app.lists[s], true);
        app.first[s] = p;
        while (p < PAGE_COUNT && strcmp(pages[p]->section, sections[s]) == 0) {
            pg_list_add(app.lists[s], pages[p]->icon, pages[p]->title, NULL);
            p++;
        }
        pg_list_set_selected(app.lists[s], -1);
        pg_widget_on(app.lists[s], PG_SIGNAL_CHANGED, nav_changed, (void *)(intptr_t)s);
        pg_widget_add(side, app.lists[s]);
    }
    pg_widget_add(side, pg_spacer_new());
    return side;
}

static bool on_key(struct pg_window *win, const struct pg_event *e, void *data)
{
    (void)data;
    if ((e->mods & PG_MOD_CTRL) && (e->key == PG_KEY_Q || e->key == PG_KEY_W)) {
        pg_window_close(win);
        return true;
    }
    return false;
}

int main(int argc, char **argv)
{
    if (!pg_init(argc, argv)) {
        fprintf(stderr, "settings: cannot connect to videod\n");
        return 1;
    }
    app.win = pg_window_new("Settings", 1180, 780, PG_WINDOW_RESIZABLE);
    if (!app.win) {
        fprintf(stderr, "settings: cannot create a window\n");
        return 1;
    }
    pg_window_set_min_size(app.win, 900, 560);
    pg_window_on_key(app.win, on_key, NULL);
    struct pg_widget *body = pg_hbox(0);
    pg_widget_add(body, sidebar());
    app.stack = pg_stack_new();
    pg_widget_set_expand(app.stack, true, true);
    for (int i = 0; i < PAGE_COUNT; i++)
        pg_widget_add(app.stack, pages[i]->build());
    pg_widget_add(body, app.stack);
    struct pg_widget *root = ui_overlay_new(body);
    ui_overlay_set_toast_offset(SIDEBAR_W);
    pg_window_set_child(app.win, root);
    show_page(0);
    pg_timer_add(1000, tick, NULL);
    int code = pg_run();
    pg_shutdown();
    return code;
}
