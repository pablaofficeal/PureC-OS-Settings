#include "pages.h"
#include "sysapi.h"

#include <QtWidgets/qslider.h>

class SoundPage : public Page
{
public:
    SoundPage()
        : Page(QStringLiteral("Sound"), QStringLiteral("Volume, output device and audio test"))
    {
        auto *volume = new Card;
        auto *head = volume->addHeader(QStringLiteral("volume_up"), QStringLiteral("Output volume"),
                                       QStringLiteral("Applied instantly and remembered"));
        m_badge = qobject_cast<IconBadge *>(head->itemAt(0)->widget());
        m_percent = new QLabel;
        m_percent->setStyleSheet(QStringLiteral("font-size: 28px; font-weight: bold;"));
        head->addWidget(m_percent);
        m_slider = new QSlider(Qt::Horizontal);
        m_slider->setRange(0, 100);
        m_slider->setCursor(Qt::PointingHandCursor);
        auto *sliderRow = new QHBoxLayout;
        sliderRow->setContentsMargins(46, 4, 0, 6);
        sliderRow->addWidget(m_slider);
        volume->body()->addLayout(sliderRow);
        QObject::connect(m_slider, &QSlider::valueChanged, [this](int v) {
            if (m_updating)
                return;
            sa_audio_set_volume(uint32_t(v));
            m_percent->setText(QStringLiteral("%1%").arg(v));
        });
        auto *line = new QFrame;
        line->setObjectName(QStringLiteral("hairline"));
        volume->body()->addWidget(line);
        auto *muteRow = new QHBoxLayout;
        muteRow->setContentsMargins(4, 2, 4, 0);
        muteRow->addWidget(new QLabel(QStringLiteral("Mute")));
        muteRow->addStretch(1);
        m_mute = new Toggle;
        QObject::connect(m_mute, &QAbstractButton::toggled, [this](bool on) {
            if (m_updating)
                return;
            sa_audio_set_muted(on);
            refresh();
        });
        muteRow->addWidget(m_mute);
        volume->body()->addLayout(muteRow);
        content()->addWidget(volume);

        auto *outputs = new Card;
        auto *oh = outputs->addHeader(QStringLiteral("speaker"), QStringLiteral("Output device"), QStringLiteral("Audio"));
        m_backend = qobject_cast<QLabel *>(oh->itemAt(1)->layout()->itemAt(1)->widget());
        m_outputs = new QVBoxLayout;
        m_outputs->setSpacing(2);
        outputs->body()->addLayout(m_outputs);
        content()->addWidget(outputs);

        auto *check = new Card;
        auto *ch = check->addHeader(QStringLiteral("music_note"), QStringLiteral("Sound check"),
                                    QStringLiteral("Make sure your speakers work"));
        auto *test = new QPushButton(QStringLiteral(" Play test sound"));
        test->setObjectName(QStringLiteral("primary"));
        test->setIcon(QIcon(tintedIcon(QStringLiteral("play_arrow"), Theme::window, 32)));
        QObject::connect(test, &QPushButton::clicked, [] { sa_audio_test(); });
        ch->addWidget(test);
        auto *toneLabel = new QLabel(QStringLiteral("Tone generator"));
        toneLabel->setObjectName(QStringLiteral("muted"));
        check->body()->addSpacing(4);
        check->body()->addWidget(toneLabel);
        auto *toneRow = new QHBoxLayout;
        toneRow->setSpacing(10);
        m_tones = new Segmented({QStringLiteral("220 Hz"), QStringLiteral("440 Hz"), QStringLiteral("880 Hz"),
                                 QStringLiteral("1760 Hz")});
        m_tones->setCurrent(0);
        toneRow->addWidget(m_tones, 1);
        auto *play = new QPushButton(QStringLiteral(" Play"));
        play->setIcon(QIcon(tintedIcon(QStringLiteral("music_note"), Theme::text, 32)));
        QObject::connect(play, &QPushButton::clicked, [this] {
            static const uint32_t hz[] = {220, 440, 880, 1760};
            if (sa_audio_tone(hz[qBound(0, m_tones->current(), 3)], 600) < 0)
                toast(QStringLiteral("Audio output is not ready"), Theme::red);
        });
        toneRow->addWidget(play);
        check->body()->addLayout(toneRow);
        content()->addWidget(check);
        content()->addStretch(1);
    }

    void refresh() override
    {
        sa_audio st;
        if (!sa_audio(&st)) {
            m_percent->setText(QStringLiteral("--"));
            m_backend->setText(QStringLiteral("No audio device"));
            return;
        }
        m_updating = true;
        if (!m_slider->isSliderDown())
            m_slider->setValue(int(st.volume));
        m_mute->setChecked(st.muted);
        m_updating = false;
        m_percent->setText(QStringLiteral("%1%").arg(st.volume));
        m_badge->setIcon(st.muted ? QStringLiteral("volume_off") : QStringLiteral("volume_up"),
                         st.muted ? Theme::muted : Theme::accent);
        m_backend->setText(st.codec || st.dac ? QStringLiteral("Intel HD Audio") : QStringLiteral("Audio output"));
        QString signature = QStringLiteral("%1/%2/%3/%4/%5").arg(st.outputs).arg(st.selected).arg(st.codec).arg(st.dac).arg(st.pin);
        if (signature == m_signature)
            return;
        m_signature = signature;
        while (QLayoutItem *item = m_outputs->takeAt(0)) {
            delete item->widget();
            delete item;
        }
        uint32_t count = st.outputs ? st.outputs : 1;
        for (uint32_t i = 0; i < count; ++i) {
            bool active = i == st.selected;
            auto *row = new ListRow;
            row->setCheckable(true);
            row->setChecked(active);
            auto *radio = new QLabel;
            radio->setFixedSize(18, 18);
            radio->setStyleSheet(active ? QStringLiteral("border: 5px solid #89b4fa; border-radius: 9px; background: #1e1e2e;")
                                        : QStringLiteral("border: 2px solid #585b70; border-radius: 9px; background: transparent;"));
            row->row()->addWidget(radio);
            auto *texts = new QVBoxLayout;
            texts->setSpacing(1);
            auto *name = new QLabel(QStringLiteral("Output %1").arg(i + 1));
            name->setStyleSheet(QStringLiteral("font-weight: bold; background: transparent;"));
            auto *meta = new QLabel(active && (st.codec || st.dac)
                                        ? QStringLiteral("HD Audio codec %1, DAC %2, pin %3").arg(st.codec).arg(st.dac).arg(st.pin)
                                        : m_backend->text());
            meta->setObjectName(QStringLiteral("muted"));
            meta->setStyleSheet(QStringLiteral("background: transparent;"));
            texts->addWidget(name);
            texts->addWidget(meta);
            row->row()->addLayout(texts, 1);
            if (active)
                row->row()->addWidget(new Chip(QStringLiteral("Active"), Theme::accent));
            QObject::connect(row, &QPushButton::clicked, row, [this, i] {
                if (!sa_audio_select(i))
                    toast(QStringLiteral("Could not switch the output"), Theme::red);
                m_signature.clear();
                refresh();
            });
            m_outputs->addWidget(row);
        }
    }

private:
    IconBadge *m_badge;
    QLabel *m_percent, *m_backend;
    QSlider *m_slider;
    Toggle *m_mute;
    QVBoxLayout *m_outputs;
    Segmented *m_tones;
    QString m_signature;
    bool m_updating = false;
};

Page *makeSoundPage()
{
    return new SoundPage;
}
