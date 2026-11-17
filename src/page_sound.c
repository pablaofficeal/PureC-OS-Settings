#include <stdio.h>
#include <string.h>

#include "ui.h"

static const char *const tones[] = {"220 Hz", "440 Hz", "880 Hz", "1760 Hz"};
static const uint32_t tone_hz[] = {220, 440, 880, 1760};

static struct {
    struct pg_widget *volume_header;
    struct pg_widget *percent, *slider, *mute;
    struct pg_widget *device_sub, *outputs;
    struct pg_widget *tone;
    char sig[96];
    bool syncing;
} sd;

static void refresh(void);

static void volume_changed(struct pg_widget *w, void *data)
{
    (void)data;
    if (sd.syncing)
        return;
    int v = pg_slider_value(w);
    sa_audio_set_volume((uint32_t)v);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", v);
    pg_label_set_text(sd.percent, buf);
}

static void mute_changed(struct pg_widget *w, void *data)
{
    (void)data;
    if (sd.syncing)
        return;
    sa_audio_set_muted(pg_toggle_active(w));
    refresh();
}

static void output_changed(struct pg_widget *w, void *data)
{
    (void)data;
    if (sd.syncing)
        return;
    int i = pg_list_selected(w);
    if (i >= 0 && !sa_audio_select((uint32_t)i))
        ui_toast("Could not switch the output", UI_RED);
    sd.sig[0] = 0;
    refresh();
}

static void play_test(struct pg_widget *w, void *data)
{
    (void)w;
    (void)data;
    sa_audio_test();
}

static void play_tone(struct pg_widget *w, void *data)
{
    (void)w;
    (void)data;
    int i = pg_segmented_selected(sd.tone);
    if (i < 0 || i > 3)
        i = 0;
    if (sa_audio_tone(tone_hz[i], 600) < 0)
        ui_toast("Audio output is not ready", UI_RED);
}

static struct pg_widget *build(void)
{
    struct pg_widget *content;
    struct pg_widget *page = ui_page("Sound", "Volume, output device and audio test", &content);

    struct pg_widget *vol = ui_card_plain();
    sd.volume_header = ui_header(PGI_VOLUME_HIGH, PG_COLOR_ACCENT, 34, "Output volume", "Applied instantly and remembered",
                                 NULL, NULL);
    sd.percent = ui_label("--", PGF_TITLE, PG_COLOR_TEXT);
    pg_widget_set_align(sd.percent, PG_ALIGN_END, PG_ALIGN_CENTER);
    pg_widget_add(sd.volume_header, sd.percent);
    pg_widget_add(vol, sd.volume_header);
    sd.slider = pg_slider_new(0, 100, 50);
    pg_widget_set_margins(sd.slider, 46, 0, 0, 0);
    pg_widget_on(sd.slider, PG_SIGNAL_CHANGED, volume_changed, NULL);
    pg_widget_add(vol, sd.slider);
    pg_widget_add(vol, ui_hairline());
    sd.mute = pg_toggle_new(false);
    pg_widget_on(sd.mute, PG_SIGNAL_CHANGED, mute_changed, NULL);
    pg_widget_add(vol, ui_row("Mute", sd.mute));
    pg_widget_add(content, vol);

    struct pg_widget *dev = ui_card_plain();
    pg_widget_add(dev, ui_header(PGI_SPEAKER, PG_COLOR_ACCENT, 34, "Output device", "Audio output", NULL, &sd.device_sub));
    sd.outputs = pg_list_new();
    pg_widget_on(sd.outputs, PG_SIGNAL_CHANGED, output_changed, NULL);
    pg_widget_add(dev, sd.outputs);
    pg_widget_add(content, dev);

    struct pg_widget *head;
    struct pg_widget *check = ui_card(PGI_MUSIC_NOTE, PG_COLOR_ACCENT, "Sound check", "Make sure your speakers work", &head);
    struct pg_widget *test = pg_button_new("Play test sound", PGI_PLAY_CIRCLE_OUTLINE, PG_BUTTON_PRIMARY);
    pg_widget_set_align(test, PG_ALIGN_END, PG_ALIGN_CENTER);
    pg_widget_on(test, PG_SIGNAL_CLICKED, play_test, NULL);
    pg_widget_add(head, test);
    pg_widget_add(check, ui_label("Tone generator", PGF_SMALL, PG_COLOR_TEXT3));
    struct pg_widget *row = pg_hbox(12);
    sd.tone = pg_segmented_new(tones, 4);
    pg_widget_set_expand(sd.tone, true, false);
    pg_widget_add(row, sd.tone);
    struct pg_widget *play = pg_button_new("Play", PGI_MUSIC_NOTE, PG_BUTTON_SECONDARY);
    pg_widget_on(play, PG_SIGNAL_CLICKED, play_tone, NULL);
    pg_widget_add(row, play);
    pg_widget_add(check, row);
    pg_widget_add(content, check);
    return page;
}

static void refresh(void)
{
    struct sa_audio st;
    sd.syncing = true;
    if (!sa_audio(&st)) {
        pg_label_set_text(sd.percent, "--");
        pg_label_set_text(sd.device_sub, "No audio device");
        sd.syncing = false;
        return;
    }
    char buf[96];
    snprintf(buf, sizeof(buf), "%u%%", st.volume);
    if (!pg_slider_dragging(sd.slider)) {
        pg_slider_set_value(sd.slider, (int)st.volume);
        pg_label_set_text(sd.percent, buf);
    }
    pg_toggle_set_active(sd.mute, st.muted);
    pg_icon_set(sd.volume_header->first, st.muted ? PGI_VOLUME_OFF : PGI_VOLUME_HIGH,
                st.muted ? PG_COLOR_TEXT3 : PG_COLOR_ACCENT);
    const char *backend = st.codec || st.dac ? "Intel HD Audio" : "Audio output";
    pg_label_set_text(sd.device_sub, backend);
    char sig[96];
    snprintf(sig, sizeof(sig), "%u/%u/%u/%u/%u", st.outputs, st.selected, st.codec, st.dac, st.pin);
    if (strcmp(sig, sd.sig) != 0) {
        snprintf(sd.sig, sizeof(sd.sig), "%s", sig);
        pg_list_clear(sd.outputs);
        uint32_t count = st.outputs ? st.outputs : 1;
        for (uint32_t i = 0; i < count; i++) {
            char name[32];
            snprintf(name, sizeof(name), "Output %u", i + 1);
            if (i == st.selected && (st.codec || st.dac))
                snprintf(buf, sizeof(buf), "HD Audio codec %u, DAC %u, pin %u", st.codec, st.dac, st.pin);
            else
                snprintf(buf, sizeof(buf), "%s", backend);
            int idx = pg_list_add(sd.outputs, PGI_SPEAKER, name, buf);
            if (i == st.selected)
                pg_list_set_trailing(sd.outputs, idx, "Active");
        }
        pg_list_set_selected(sd.outputs, (int)st.selected);
    }
    sd.syncing = false;
}

const struct page page_sound = {"SYSTEM", "Sound", PGI_VOLUME_HIGH, build, refresh};
