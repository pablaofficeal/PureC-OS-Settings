#include "pages.h"
#include "sysapi.h"

#include <QtCore/qfile.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qscreen.h>
#include <QtWidgets/qgridlayout.h>

static QString osField(const QByteArray &key, const QString &fallback)
{
    QFile f(QStringLiteral("/etc/os-release"));
    if (!f.open(QIODevice::ReadOnly))
        return fallback;
    for (const QByteArray &line : f.readAll().split('\n')) {
        if (!line.startsWith(key + '='))
            continue;
        QByteArray v = line.mid(key.size() + 1).trimmed();
        if (v.startsWith('"') && v.endsWith('"') && v.size() >= 2)
            v = v.mid(1, v.size() - 2);
        return QString::fromUtf8(v);
    }
    return fallback;
}

QString osName()
{
    return osField("PRETTY_NAME", QStringLiteral("PureC OS"));
}

QString osVersion()
{
    return osField("VERSION", QStringLiteral("unknown"));
}

static QString uptimeText(quint64 ms)
{
    quint64 minutes = ms / 60000;
    quint64 days = minutes / 1440, hours = (minutes / 60) % 24, mins = minutes % 60;
    if (days)
        return QStringLiteral("%1d %2h %3m").arg(days).arg(hours).arg(mins);
    if (hours)
        return QStringLiteral("%1h %2m").arg(hours).arg(mins);
    return QStringLiteral("%1m").arg(mins);
}

static QString cpuSummary(const sa_cpu &cpu)
{
    QString cores = QStringLiteral("%1 core%2").arg(cpu.cores).arg(cpu.cores == 1 ? "" : "s");
    if (cpu.freq_hz)
        return cores + QStringLiteral(" @ %1 GHz").arg(double(cpu.freq_hz) / 1e9, 0, 'f', 2);
    return cores;
}

class AboutPage : public Page
{
public:
    AboutPage()
        : Page(QStringLiteral("About this PC"), QStringLiteral("Hardware, memory and system overview"))
    {
        auto *hero = new QFrame;
        hero->setObjectName(QStringLiteral("hero"));
        hero->setStyleSheet(QStringLiteral(
            "QFrame#hero { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #5a6aa8, stop:1 #3d8a99); }"
            "QLabel { background: transparent; }"));
        hero->setMinimumHeight(150);
        auto *hl = new QHBoxLayout(hero);
        hl->setContentsMargins(28, 24, 28, 24);
        hl->setSpacing(22);
        auto *logo = new QLabel(QStringLiteral("P"));
        logo->setFixedSize(80, 80);
        logo->setAlignment(Qt::AlignCenter);
        logo->setStyleSheet(QStringLiteral("background: rgba(255,255,255,40); border-radius: 18px; color: white; font-size: 34px; font-weight: bold;"));
        hl->addWidget(logo);
        auto *tl = new QVBoxLayout;
        tl->setSpacing(4);
        auto *name = new QLabel(QStringLiteral("PureC OS"));
        name->setStyleSheet(QStringLiteral("color: white; font-size: 30px; font-weight: bold;"));
        auto *ver = new QLabel(QStringLiteral("Version %1  -  x86_64").arg(osVersion()));
        ver->setStyleSheet(QStringLiteral("color: rgba(255,255,255,210);"));
        auto *tag = new QLabel(QStringLiteral("A 64-bit operating system written from scratch in C"));
        tag->setStyleSheet(QStringLiteral("color: rgba(255,255,255,170); font-size: 12px;"));
        tl->addStretch(1);
        tl->addWidget(name);
        tl->addWidget(ver);
        tl->addWidget(tag);
        tl->addStretch(1);
        hl->addLayout(tl, 1);
        content()->addWidget(hero);

        auto *tiles = new QHBoxLayout;
        tiles->setSpacing(16);
        m_cpuValue = new QLabel;
        m_cpuName = new QLabel;
        tiles->addWidget(tile(QStringLiteral("developer_board"), QStringLiteral("Processor"), m_cpuValue, m_cpuName, nullptr), 1);
        m_memValue = new QLabel;
        m_memDetail = new QLabel;
        m_ring = new Ring(64);
        tiles->addWidget(tile(QStringLiteral("memory"), QStringLiteral("Memory"), m_memValue, m_memDetail, m_ring), 1);
        m_upValue = new QLabel;
        auto *since = new QLabel(QStringLiteral("Since last boot"));
        tiles->addWidget(tile(QStringLiteral("timer"), QStringLiteral("Uptime"), m_upValue, since, nullptr, Theme::green), 1);
        content()->addLayout(tiles);

        auto *load = new Card;
        auto *head = load->addHeader(QStringLiteral("speed"), QStringLiteral("Processor load"), QStringLiteral("Last 48 seconds"));
        m_load = new QLabel;
        m_load->setStyleSheet(QStringLiteral("color: #89b4fa; font-size: 22px; font-weight: bold;"));
        head->addWidget(m_load);
        m_chart = new LoadChart;
        load->body()->addWidget(m_chart);
        content()->addWidget(load);

        auto *spec = new Card;
        spec->addHeader(QStringLiteral("info"), QStringLiteral("Device specifications"), QString());
        m_os = new InfoRow(QStringLiteral("Operating system"));
        m_proc = new InfoRow(QStringLiteral("Processor"));
        m_ram = new InfoRow(QStringLiteral("Installed RAM"));
        m_display = new InfoRow(QStringLiteral("Display"));
        m_root = new InfoRow(QStringLiteral("System drive"));
        for (InfoRow *r : {m_os, m_proc, m_ram, m_display, m_root})
            spec->body()->addWidget(r);
        content()->addWidget(spec);
        content()->addStretch(1);
        m_os->setValue(QStringLiteral("%1 (x86_64)").arg(osName()));
        sa_fs fs;
        sa_fs_info(&fs);
        m_root->setValue(QStringLiteral("%1 (%2)").arg(QString::fromUtf8(fs.root), QString::fromUtf8(fs.type)));
    }

    void refresh() override
    {
        sa_cpu cpu;
        if (sa_cpu_info(&cpu)) {
            m_cpuValue->setText(cpuSummary(cpu));
            m_cpuName->setText(QString::fromUtf8(cpu.name).trimmed());
            m_proc->setValue(QString::fromUtf8(cpu.name).trimmed());
            m_upValue->setText(uptimeText(cpu.uptime_ms));
            m_load->setText(QStringLiteral("%1%").arg(cpu.usage));
            m_chart->push(int(cpu.usage));
        }
        sa_mem mem;
        if (sa_memory_info(&mem) && mem.total) {
            m_memValue->setText(humanBytes(mem.total));
            m_memDetail->setText(QStringLiteral("%1 used, %2 free").arg(humanBytes(mem.used), humanBytes(mem.available)));
            int pct = int(mem.used * 100 / mem.total);
            m_ring->setValue(pct, QStringLiteral("%1%").arg(pct));
            m_ram->setValue(humanBytes(mem.total));
        }
        if (QScreen *s = QGuiApplication::primaryScreen())
            m_display->setValue(QStringLiteral("%1 x %2, 32-bit color").arg(s->geometry().width()).arg(s->geometry().height()));
    }

private:
    QWidget *tile(const QString &icon, const QString &title, QLabel *value, QLabel *detail, QWidget *side,
                  const QColor &color = Theme::accent)
    {
        auto *card = new Card;
        auto *row = new QHBoxLayout;
        auto *col = new QVBoxLayout;
        col->setSpacing(4);
        auto *top = new QHBoxLayout;
        top->setSpacing(10);
        top->addWidget(new IconBadge(icon, color, 30));
        auto *t = new QLabel(title);
        t->setObjectName(QStringLiteral("muted"));
        top->addWidget(t, 1);
        col->addLayout(top);
        col->addSpacing(6);
        value->setStyleSheet(QStringLiteral("font-size: 17px; font-weight: bold;"));
        detail->setObjectName(QStringLiteral("muted"));
        detail->setWordWrap(true);
        col->addWidget(value);
        col->addWidget(detail);
        row->addLayout(col, 1);
        if (side)
            row->addWidget(side, 0, Qt::AlignVCenter);
        card->body()->addLayout(row);
        return card;
    }

    QLabel *m_cpuValue, *m_cpuName, *m_memValue, *m_memDetail, *m_upValue, *m_load;
    Ring *m_ring;
    LoadChart *m_chart;
    InfoRow *m_os, *m_proc, *m_ram, *m_display, *m_root;
};

Page *makeAboutPage()
{
    return new AboutPage;
}
