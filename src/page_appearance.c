#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ui.h"

#define CONFIG "/config/appear.ini"
#define MAX_THEMES 16
#define MAX_WALLPAPERS 64

static const char *const font_ids[] = {"clean", "classic", "bold"};
static const char *const font_names[] = {"Clean", "Classic", "Bold"};
static const char *const size_names[] = {"8", "10", "12", "16", "24"};
static const int sizes[] = {8, 10, 12, 16, 24};

static struct {
    char theme[48];
    char wallpaper[256];
    char mode[32];
    char font[16];
    int font_size;
    struct pg_widget *theme_tiles[MAX_THEMES];
    int theme_count;
    struct pg_widget *walls;
    char *paths[MAX_WALLPAPERS];
    int wall_count;
    struct pg_widget *font_seg, *size_seg;
    bool syncing;
} ap;

struct theme_tile {
    struct pg_widget base;
    int index;
    bool checked;
};

static void load(void)
{
    snprintf(ap.theme, sizeof(ap.theme), "catppuccin-dark");
    ap.wallpaper[0] = 0;
    snprintf(ap.mode, sizeof(ap.mode), "stretch");
    snprintf(ap.font, sizeof(ap.font), "clean");
    ap.font_size = 8;
    int fd = open(CONFIG, O_RDONLY);
    if (fd < 0)
        return;
    char buf[2048];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0)
        return;
    buf[n] = 0;
    for (char *line = buf; line && *line;) {
        char *end = strchr(line, '\n');
        if (end)
            *end = 0;
        char *eq = strchr(line, '=');
        if (eq && eq > line) {
            *eq = 0;
            char *k = line, *v = eq + 1;
            while (*k == ' ')
                k++;
            char *ke = k + strlen(k);
            while (ke > k && (ke[-1] == ' ' || ke[-1] == '\t'))
                *--ke = 0;
            while (*v == ' ')
                v++;
            char *ve = v + strlen(v);
            while (ve > v && (ve[-1] == ' ' || ve[-1] == '\r' || ve[-1] == '\t'))
                *--ve = 0;
            if (strcmp(k, "theme") == 0)
                snprintf(ap.theme, sizeof(ap.theme), "%s", v);
            else if (strcmp(k, "wallpaper") == 0)
                snprintf(ap.wallpaper, sizeof(ap.wallpaper), "%s", v);
            else if (strcmp(k, "wallpaper_mode") == 0)
                snprintf(ap.mode, sizeof(ap.mode), "%s", v);
            else if (strcmp(k, "font") == 0)
                snprintf(ap.font, sizeof(ap.font), "%s", v);
            else if (strcmp(k, "font_size") == 0) {
                int s = atoi(v);
                ap.font_size = s < 8 ? 8 : s > 24 ? 24 : s;
            }
        }
        line = end ? end + 1 : NULL;
    }
}

static void sync_controls(void);

static void apply(void)
{
    mkdir("/config", 0755);
    char buf[512];
    int len = snprintf(buf, sizeof(buf), "theme=%s\nwallpaper=%s\nwallpaper_mode=%s\nfont=%s\nfont_size=%d\n", ap.theme,
                       ap.wallpaper, ap.mode, ap.font, ap.font_size);
    int fd = open(CONFIG, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    bool ok = fd >= 0 && write(fd, buf, (size_t)len) == len;
    if (fd >= 0)
        close(fd);
    if (ok) {
        pg_theme_reload();
        ui_toast("Appearance updated", UI_GREEN);
    } else {
        ui_toast("Cannot write /config/appear.ini", UI_RED);
    }
    sync_controls();
}

static void tile_measure(struct pg_widget *w, int *width, int *height)
{
    (void)w;
    *width = 150;
    *height = 118;
}

static void tile_draw(struct pg_widget *w, struct pg_canvas *c)
{
    struct theme_tile *t = (struct theme_tile *)w;
    const struct pg_theme *cur = pg_theme();
    struct pg_theme p;
    const char *name = pg_theme_name((unsigned)t->index);
    if (!pg_theme_preview(name, &p))
        return;
    struct pg_rect r = w->rect;
    int px = r.x + (r.w - 136) / 2, py = r.y + 4;
    if (t->checked)
        pgc_rrect_stroke(c, px - 3, py - 3, 142, 92, 11, cur->accent, 255);
    else if (w->hovered)
        pgc_rrect_stroke(c, px - 3, py - 3, 142, 92, 11, cur->border, 255);
    pgc_rrect(c, px, py, 136, 86, 8, p.bg, 255);
    int wx = px + 136 * 10 / 100, wy = py + 86 * 14 / 100, ww = 136 * 80 / 100, wh = 86 * 72 / 100;
    pgc_rrect(c, wx, wy, ww, wh, 5, p.window, 255);
    int th = wh * 15 / 100;
    pgc_fill(c, wx + 2, wy, ww - 4, th, p.titlebar);
    pgc_circle(c, wx + ww - 18, wy + th / 2, 3, p.accent, 255);
    pgc_circle(c, wx + ww - 9, wy + th / 2, 3, p.danger, 255);
    pgc_fill(c, wx + 9, wy + wh * 24 / 100 + 4, ww * 55 / 100, 4, p.text);
    pgc_fill(c, wx + 9, wy + wh * 36 / 100 + 4, ww * 40 / 100, 3, p.text3);
    pgc_fill(c, wx + 9, wy + wh * 46 / 100 + 4, ww * 48 / 100, 3, p.text3);
    pgc_rrect(c, wx + ww - ww * 32 / 100 - 8, wy + wh - wh * 11 / 100 - 6, ww * 32 / 100, wh * 11 / 100, 3, p.accent,
              255);
    pgc_text_center(c, t->checked ? PGF_MEDIUM : PGF_BODY, r.x, py + 94, r.w, name,
                    t->checked ? cur->text : cur->text3);
    pg_widget_draw_focus(w, c, 10);
}

static bool tile_event(struct pg_widget *w, const struct pg_event *e)
{
    struct theme_tile *t = (struct theme_tile *)w;
    bool fire;
    if (e->type == PG_EVENT_POINTER_DOWN)
        return e->button == PG_BUTTON_LEFT;
    if (e->type == PG_EVENT_POINTER_UP)
        fire = w->pressed && e->x >= w->rect.x && e->y >= w->rect.y && e->x < w->rect.x + w->rect.w &&
               e->y < w->rect.y + w->rect.h;
    else if (e->type == PG_EVENT_KEY_DOWN && (e->key == PG_KEY_SPACE || e->key == PG_KEY_ENTER))
        fire = true;
    else
        return false;
    if (fire) {
        snprintf(ap.theme, sizeof(ap.theme), "%s", pg_theme_name((unsigned)t->index));
        apply();
    }
    return true;
}

static const struct pg_widget_class theme_tile_class = {
    .name = "theme-tile",
    .size = sizeof(struct theme_tile),
    .measure = tile_measure,
    .draw = tile_draw,
    .event = tile_event,
};

static int compare(const void *a, const void *b)
{
    return strcmp(*(char *const *)a, *(char *const *)b);
}

static bool listed(const char *path)
{
    for (int i = 0; i < ap.wall_count; i++)
        if (strcmp(ap.paths[i], path) == 0)
            return true;
    return false;
}

static void add_path(const char *path)
{
    struct stat st;
    if (ap.wall_count >= MAX_WALLPAPERS || listed(path) || stat(path, &st) != 0 || !S_ISREG(st.st_mode))
        return;
    char *copy = strdup(path);
    if (copy)
        ap.paths[ap.wall_count++] = copy;
}

static bool read_list(const char *file)
{
    int fd = open(file, O_RDONLY);
    if (fd < 0)
        return false;
    char buf[4096];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    int before = ap.wall_count;
    if (n > 0) {
        buf[n] = 0;
        for (char *line = buf; line && *line;) {
            char *end = strchr(line, '\n');
            if (end)
                *end = 0;
            size_t len = strlen(line);
            while (len && (line[len - 1] == '\r' || line[len - 1] == ' '))
                line[--len] = 0;
            if (line[0] && line[0] != '#')
                add_path(line);
            line = end ? end + 1 : NULL;
        }
    }
    return ap.wall_count > before;
}

static bool image_name(const char *name)
{
    size_t n = strlen(name);
    if (n < 5)
        return false;
    const char *ext = name + n - 4;
    return strcmp(ext, ".bmp") == 0 || strcmp(ext, ".png") == 0 || strcmp(ext, ".BMP") == 0 ||
           strcmp(ext, ".PNG") == 0;
}

static void scan_dir(const char *dir)
{
    DIR *d = opendir(dir);
    if (!d)
        return;
    char *names[MAX_WALLPAPERS];
    int n = 0;
    struct dirent *e;
    while ((e = readdir(d)) && n < MAX_WALLPAPERS)
        if (image_name(e->d_name))
            names[n++] = strdup(e->d_name);
    closedir(d);
    qsort(names, (size_t)n, sizeof(names[0]), compare);
    for (int i = 0; i < n; i++) {
        char path[512];
        if (names[i]) {
            snprintf(path, sizeof(path), "%s/%s", dir, names[i]);
            add_path(path);
            free(names[i]);
        }
    }
}

static void find_wallpapers(void)
{
    for (int i = 0; i < ap.wall_count; i++)
        free(ap.paths[i]);
    ap.wall_count = 0;
    if (!read_list("/config/wplist.ini"))
        read_list("/demo/wplist.ini");
    scan_dir("/usr/share/wallpapers");
    scan_dir("/walpaper");
    scan_dir("/config");
}

static void rebuild_walls(void)
{
    ap.syncing = true;
    find_wallpapers();
    pg_list_clear(ap.walls);
    pg_list_add(ap.walls, PGI_PALETTE, "Solid color", "Uses the theme desktop color");
    int selected = ap.wallpaper[0] ? -1 : 0;
    for (int i = 0; i < ap.wall_count; i++) {
        const char *slash = strrchr(ap.paths[i], '/');
        pg_list_add(ap.walls, PGI_WALLPAPER, slash ? slash + 1 : ap.paths[i], ap.paths[i]);
        if (strcmp(ap.paths[i], ap.wallpaper) == 0)
            selected = i + 1;
    }
    pg_list_set_selected(ap.walls, selected);
    ap.syncing = false;
}

static void sync_controls(void)
{
    ap.syncing = true;
    for (int i = 0; i < ap.theme_count; i++) {
        struct theme_tile *t = (struct theme_tile *)ap.theme_tiles[i];
        t->checked = strcmp(pg_theme_name((unsigned)i), ap.theme) == 0;
        pg_widget_queue_draw(&t->base);
    }
    int f = strcmp(ap.font, "classic") == 0 ? 1 : strcmp(ap.font, "bold") == 0 ? 2 : 0;
    pg_segmented_set_selected(ap.font_seg, f);
    int best = 0;
    for (int i = 1; i < 5; i++)
        if (abs(sizes[i] - ap.font_size) < abs(sizes[best] - ap.font_size))
            best = i;
    pg_segmented_set_selected(ap.size_seg, best);
    ap.syncing = false;
    rebuild_walls();
}

static void wall_changed(struct pg_widget *w, void *data)
{
    (void)data;
    if (ap.syncing)
        return;
    int i = pg_list_selected(w);
    if (i < 0)
        return;
    snprintf(ap.wallpaper, sizeof(ap.wallpaper), "%s", i == 0 ? "" : ap.paths[i - 1]);
    apply();
}

static void font_changed(struct pg_widget *w, void *data)
{
    (void)data;
    if (ap.syncing)
        return;
    snprintf(ap.font, sizeof(ap.font), "%s", font_ids[pg_segmented_selected(w)]);
    apply();
}

static void size_changed(struct pg_widget *w, void *data)
{
    (void)data;
    if (ap.syncing)
        return;
    ap.font_size = sizes[pg_segmented_selected(w)];
    apply();
}

static struct pg_widget *build(void)
{
    load();
    struct pg_widget *content;
    struct pg_widget *page = ui_page("Appearance", "Theme, wallpaper and fonts", &content);

    struct pg_widget *themes = ui_card(PGI_PALETTE, PG_COLOR_ACCENT, "Color theme", "Desktop, panels and menus", NULL);
    struct pg_widget *grid = pg_grid_new(8, 8);
    ap.theme_count = (int)pg_theme_count();
    if (ap.theme_count > MAX_THEMES)
        ap.theme_count = MAX_THEMES;
    for (int i = 0; i < ap.theme_count; i++) {
        struct theme_tile *t = (struct theme_tile *)pg_widget_new(&theme_tile_class);
        t->index = i;
        t->base.focusable = true;
        ap.theme_tiles[i] = &t->base;
        pg_grid_attach(grid, &t->base, i % 4, i / 4, 1, 1);
    }
    pg_widget_set_align(grid, PG_ALIGN_START, PG_ALIGN_START);
    pg_widget_add(themes, grid);
    pg_widget_add(content, themes);

    struct pg_widget *walls =
        ui_card(PGI_WALLPAPER, PG_COLOR_ACCENT, "Wallpaper", "BMP or PNG, stretched. Add your own to /usr/share/wallpapers", NULL);
    ap.walls = pg_list_new();
    pg_widget_on(ap.walls, PG_SIGNAL_CHANGED, wall_changed, NULL);
    pg_widget_add(walls, ap.walls);
    pg_widget_add(content, walls);

    struct pg_widget *fonts = ui_card(PGI_FORMAT_FONT, PG_COLOR_ACCENT, "Desktop font", "Labels of the desktop and the panel", NULL);
    pg_widget_add(fonts, ui_label("Style", PGF_SMALL, PG_COLOR_TEXT3));
    ap.font_seg = pg_segmented_new(font_names, 3);
    pg_widget_on(ap.font_seg, PG_SIGNAL_CHANGED, font_changed, NULL);
    pg_widget_add(fonts, ap.font_seg);
    pg_widget_add(fonts, ui_label("Size", PGF_SMALL, PG_COLOR_TEXT3));
    ap.size_seg = pg_segmented_new(size_names, 5);
    pg_widget_on(ap.size_seg, PG_SIGNAL_CHANGED, size_changed, NULL);
    pg_widget_add(fonts, ap.size_seg);
    pg_widget_add(content, fonts);

    sync_controls();
    return page;
}

const struct page page_appearance = {"PERSONAL", "Appearance", PGI_PALETTE, build, NULL};
