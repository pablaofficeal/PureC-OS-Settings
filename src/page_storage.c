#include <stdio.h>
#include <string.h>

#include "ui.h"

static struct {
    struct pg_widget *title, *detail, *bar, *used, *free_label, *stats;
    struct pg_widget *disks_sub, *disks;
    char sig[512];
} st;

static const char *transport_text(int t)
{
    switch (t) {
    case SA_TRANSPORT_ATA: return "ATA";
    case SA_TRANSPORT_AHCI: return "SATA (AHCI)";
    case SA_TRANSPORT_USB:
    case SA_TRANSPORT_USB_EHCI: return "USB";
    default: return "Disk";
    }
}

static void short_root(const struct sa_fs *fs, bool ok, char *out, int cap)
{
    const char *r = ok ? fs->root : "";
    const char *slash = strrchr(r, '/');
    snprintf(out, (size_t)cap, "%s", slash ? slash + 1 : r);
}

static struct pg_widget *build(void)
{
    struct pg_widget *content;
    struct pg_widget *page = ui_page("Storage", "Disks, file system and free space", &content);

    struct pg_widget *sys = ui_card_plain();
    pg_widget_add(sys, ui_header(PGI_HARDDISK, PG_COLOR_ACCENT, 64, "System drive", "", &st.title, &st.detail));
    st.bar = pg_progress_new();
    pg_widget_set_size(st.bar, -1, 10);
    pg_widget_add(sys, st.bar);
    struct pg_widget *row = pg_hbox(12);
    st.used = ui_label("", PGF_MEDIUM, PG_COLOR_TEXT);
    pg_widget_set_expand(st.used, true, false);
    pg_widget_add(row, st.used);
    st.free_label = ui_label("", PGF_BODY, PG_COLOR_TEXT3);
    pg_widget_add(row, st.free_label);
    pg_widget_add(sys, row);
    st.stats = ui_label("", PGF_SMALL, PG_COLOR_TEXT3);
    pg_widget_add(sys, st.stats);
    pg_widget_add(content, sys);

    struct pg_widget *disks = ui_card_plain();
    pg_widget_add(disks, ui_header(PGI_HARDDISK, PG_COLOR_ACCENT, 34, "Disks", "Looking for devices", NULL, &st.disks_sub));
    st.disks = pg_vbox(10);
    pg_widget_add(disks, st.disks);
    pg_widget_add(content, disks);

    struct pg_widget *note = ui_card_plain();
    struct pg_widget *nh = ui_header(PGI_SHIELD_CHECK, UI_GREEN, 34, "Partitioning and formatting live in the terminal",
                                     "fdisk and mkfs always ask for a separate confirmation before changing a disk.", NULL,
                                     NULL);
    pg_widget_add(note, nh);
    pg_widget_add(content, note);
    return page;
}

static struct pg_widget *disk_row(const struct sa_disk *d, const char *root)
{
    struct pg_widget *row = pg_hbox(14);
    bool usb = d->transport == SA_TRANSPORT_USB || d->transport == SA_TRANSPORT_USB_EHCI;
    struct pg_widget *icon = pg_icon_tile_new(usb ? PGI_USB : PGI_HARDDISK, 38, usb ? UI_TEAL : PG_COLOR_ACCENT);
    pg_widget_set_align(icon, PG_ALIGN_START, PG_ALIGN_CENTER);
    pg_widget_add(row, icon);
    struct pg_widget *text = pg_vbox(4);
    pg_widget_set_expand(text, true, false);
    char model[64], meta[128];
    snprintf(model, sizeof(model), "%s", d->model);
    char *p = model;
    while (*p == ' ')
        p++;
    size_t n = strlen(p);
    while (n && p[n - 1] == ' ')
        p[--n] = 0;
    pg_widget_add(text, ui_label(*p ? p : d->name, PGF_MEDIUM, PG_COLOR_TEXT));
    snprintf(meta, sizeof(meta), "%s  -  %s  -  %u B sectors", d->name, transport_text(d->transport), d->sector_size);
    pg_widget_add(text, ui_label(meta, PGF_SMALL, PG_COLOR_TEXT3));
    struct pg_widget *chips = pg_hbox(6);
    pg_widget_add(chips, pg_badge_new(d->operational ? "Healthy" : "Not ready", d->operational ? UI_GREEN : UI_PEACH));
    pg_widget_add(chips, pg_badge_new(d->writable ? "Read / write" : "Read only", UI_TEAL));
    size_t nl = strlen(d->name);
    if (root[0] && (strcmp(d->name, root) == 0 || (nl && strncmp(root, d->name, nl) == 0)))
        pg_widget_add(chips, pg_badge_new("System", PG_COLOR_ACCENT));
    pg_widget_add(text, chips);
    pg_widget_add(row, text);
    char size[32];
    ui_human_bytes(d->sectors * d->sector_size, size, sizeof(size));
    struct pg_widget *sl = ui_label(size, PGF_HEAD, PG_COLOR_TEXT);
    pg_widget_set_align(sl, PG_ALIGN_END, PG_ALIGN_CENTER);
    pg_widget_add(row, sl);
    return row;
}

static void refresh(void)
{
    struct sa_fs fs;
    bool ok = sa_fs_info(&fs);
    char root[32], buf[160], a[32], b[32];
    short_root(&fs, ok, root, sizeof(root));
    snprintf(buf, sizeof(buf), "System drive (%s)", root[0] ? root : "unknown");
    pg_label_set_text(st.title, buf);
    snprintf(buf, sizeof(buf), "%s file system, mounted at /", ok && fs.type[0] ? fs.type : "ext2");
    pg_label_set_text(st.detail, buf);
    if (!ok || !fs.total_bytes) {
        pg_progress_set(st.bar, 0);
        pg_label_set_text(st.used, "Not an ext2 disk");
        pg_label_set_text(st.free_label, "");
        pg_label_set_text(st.stats, "");
    } else {
        uint64_t used = fs.total_bytes - fs.free_bytes;
        pg_progress_set(st.bar, (uint32_t)(used * 1000 / fs.total_bytes));
        ui_human_bytes(used, a, sizeof(a));
        snprintf(buf, sizeof(buf), "%s used", a);
        pg_label_set_text(st.used, buf);
        ui_human_bytes(fs.free_bytes, a, sizeof(a));
        ui_human_bytes(fs.total_bytes, b, sizeof(b));
        snprintf(buf, sizeof(buf), "%s free of %s", a, b);
        pg_label_set_text(st.free_label, buf);
        snprintf(buf, sizeof(buf), "%u files and folders  -  %u-byte blocks  -  %u block groups",
                 fs.inodes_total - fs.inodes_free, fs.block_size, fs.groups);
        pg_label_set_text(st.stats, buf);
    }

    struct sa_disk list[8];
    int n = sa_disks(list, 8);
    if (n < 0)
        n = 0;
    char sig[sizeof(st.sig)];
    int len = snprintf(sig, sizeof(sig), "%s", root);
    for (int i = 0; i < n && len < (int)sizeof(sig) - 64; i++)
        len += snprintf(sig + len, sizeof(sig) - (size_t)len, "|%s:%llu:%d", list[i].name,
                        (unsigned long long)list[i].sectors, list[i].operational);
    if (strcmp(sig, st.sig) == 0 && st.sig[0])
        return;
    snprintf(st.sig, sizeof(st.sig), "%s", sig);
    snprintf(buf, sizeof(buf), "%d device%s connected", n, n == 1 ? "" : "s");
    pg_label_set_text(st.disks_sub, buf);
    pg_widget_clear(st.disks);
    for (int i = 0; i < n; i++) {
        if (i)
            pg_widget_add(st.disks, ui_hairline());
        pg_widget_add(st.disks, disk_row(&list[i], root));
    }
}

const struct page page_storage = {"DEVICE", "Storage", PGI_HARDDISK, build, refresh};
