#pragma once
#include <stdbool.h>
#include <stdint.h>

#include "pg/pg.h"
#include "sysapi.h"

#define UI_GREEN 0xA6E3A1u
#define UI_TEAL 0x94E2D5u
#define UI_RED 0xF38BA8u
#define UI_PEACH 0xFAB387u
#define UI_MAUVE 0xCBA6F7u

struct page {
    const char *section;
    const char *title;
    int icon;
    struct pg_widget *(*build)(void);
    void (*refresh)(void);
};

extern const struct page page_about, page_network, page_sound, page_display, page_appearance, page_storage,
    page_datetime, page_power;

struct pg_widget *ui_page(const char *title, const char *subtitle, struct pg_widget **content);
struct pg_widget *ui_card(int icon, uint32_t color, const char *title, const char *subtitle, struct pg_widget **header);
struct pg_widget *ui_card_plain(void);
struct pg_widget *ui_header(int icon, uint32_t color, int size, const char *title, const char *subtitle,
                            struct pg_widget **title_label, struct pg_widget **subtitle_label);
struct pg_widget *ui_row(const char *label, struct pg_widget *control);
struct pg_widget *ui_label(const char *text, int font, uint32_t color);
struct pg_widget *ui_hero(uint32_t left, uint32_t right, int padding);
struct pg_widget *ui_hairline(void);

typedef void (*ui_tile_fn)(int index, void *data);
struct pg_widget *ui_tile(const char *line1, const char *line2, int index, ui_tile_fn fn, void *data);
void ui_tile_set_checked(struct pg_widget *tile, bool checked);

void ui_human_bytes(uint64_t bytes, char *out, int cap);
void ui_os_release(const char *key, char *out, int cap, const char *fallback);
void ui_ip(uint32_t ip, char *out, int cap);

void ui_toast(const char *text, uint32_t color);
typedef void (*ui_confirm_fn)(void *data);
void ui_confirm(const char *title, const char *text, const char *action, ui_confirm_fn fn, void *data);
typedef void (*ui_password_fn)(const char *password, void *data);
void ui_password(const char *title, const char *prompt, ui_password_fn fn, void *data);

struct pg_widget *ui_overlay_new(struct pg_widget *content);
void ui_overlay_set_toast_offset(int left);
