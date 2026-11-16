#include "pages.h"
#include "sysapi.h"

#include <QtCore/qdatetime.h>
#include <QtGui/qpainter.h>
#include <QtWidgets/qgridlayout.h>

struct Zone {
    const char *id;
    const char *city;
    int minutes;
};

static const Zone zones[] = {
    {"UTC", "UTC", 0}, {"Pacific/Honolulu", "Honolulu", -600}, {"America/Los_Angeles", "Los Angeles", -480},
    {"America/Denver", "Denver", -420}, {"America/Chicago", "Chicago", -360}, {"America/New_York", "New York", -300},
    {"America/Sao_Paulo", "Sao Paulo", -180}, {"Europe/London", "London", 0}, {"Europe/Berlin", "Berlin", 60},
    {"Europe/Paris", "Paris", 60}, {"Europe/Warsaw", "Warsaw", 60}, {"Europe/Stockholm", "Stockholm", 60},
    {"Europe/Kyiv", "Kyiv", 120}, {"Europe/Chisinau", "Chisinau", 120}, {"Europe/Helsinki", "Helsinki", 120},
    {"Europe/Istanbul", "Istanbul", 180}, {"Europe/Moscow", "Moscow", 180}, {"Asia/Dubai", "Dubai", 240},
    {"Asia/Tashkent", "Tashkent", 300}, {"Asia/Kolkata", "Kolkata", 330}, {"Asia/Novosibirsk", "Novosibirsk", 420},
    {"Asia/Shanghai", "Shanghai", 480}, {"Asia/Tokyo", "Tokyo", 540}, {"Asia/Vladivostok", "Vladivostok", 600},
    {"Australia/Sydney", "Sydney", 600}, {"Pacific/Auckland", "Auckland", 720},
};

static QString offsetText(int minutes)
{
    int a = qAbs(minutes);
    return QStringLiteral("UTC%1%2:%3").arg(minutes < 0 ? QLatin1Char('-') : QLatin1Char('+'))
        .arg(a / 60, 2, 10, QLatin1Char('0')).arg(a % 60, 2, 10, QLatin1Char('0'));
}

class ClockFace : public QWidget
{
public:
    ClockFace()
    {
        setFixedSize(150, 150);
    }
    void setTime(const QTime &t)
    {
        m_time = t;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QPointF c(width() / 2.0, height() / 2.0);
        qreal r = width() / 2.0 - 2;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 255, 255, 30));
        p.drawEllipse(c, r, r);
        p.setBrush(QColor(255, 255, 255, 40));
        p.drawEllipse(c, r * 0.8, r * 0.8);
        p.setBrush(QColor(255, 255, 255, 200));
        for (int i = 0; i < 12; ++i) {
            qreal a = i * M_PI / 6;
            qreal d = i % 3 == 0 ? 3.2 : 2.0;
            p.drawEllipse(QPointF(c.x() + qSin(a) * r * 0.9, c.y() - qCos(a) * r * 0.9), d, d);
        }
        auto hand = [&](qreal fraction, qreal length, qreal width, const QColor &color) {
            qreal a = fraction * 2 * M_PI;
            p.setPen(QPen(color, width, Qt::SolidLine, Qt::RoundCap));
            p.drawLine(c, QPointF(c.x() + qSin(a) * length, c.y() - qCos(a) * length));
        };
        qreal h = (m_time.hour() % 12 + m_time.minute() / 60.0) / 12.0;
        qreal m = (m_time.minute() + m_time.second() / 60.0) / 60.0;
        hand(h, r * 0.42, 5, Qt::white);
        hand(m, r * 0.62, 3.5, Qt::white);
        hand(m_time.second() / 60.0, r * 0.68, 1.5, QColor(0xF3, 0x8B, 0xA8));
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::white);
        p.drawEllipse(c, 4, 4);
    }

private:
    QTime m_time;
};

class DateTimePage : public Page
{
public:
    DateTimePage()
        : Page(QStringLiteral("Date & time"), QStringLiteral("Clock, time zone and uptime"))
    {
        auto *hero = new QFrame;
        hero->setObjectName(QStringLiteral("hero"));
        hero->setStyleSheet(QStringLiteral(
            "QFrame#hero { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #48598f, stop:1 #3a6d7c); }"
            "QLabel { background: transparent; color: white; }"));
        auto *hl = new QHBoxLayout(hero);
        hl->setContentsMargins(36, 26, 36, 26);
        auto *col = new QVBoxLayout;
        col->setSpacing(6);
        auto *timeRow = new QHBoxLayout;
        timeRow->setSpacing(8);
        m_time = new QLabel;
        m_time->setStyleSheet(QStringLiteral("font-size: 64px; font-weight: bold;"));
        m_seconds = new QLabel;
        m_seconds->setStyleSheet(QStringLiteral("font-size: 22px; font-weight: bold; color: rgba(255,255,255,170);"));
        timeRow->addWidget(m_time);
        timeRow->addWidget(m_seconds, 0, Qt::AlignBottom);
        timeRow->addStretch(1);
        m_date = new QLabel;
        m_date->setStyleSheet(QStringLiteral("font-size: 22px; font-weight: bold;"));
        m_zone = new QLabel;
        m_zone->setStyleSheet(QStringLiteral("color: rgba(255,255,255,200);"));
        col->addLayout(timeRow);
        col->addWidget(m_date);
        col->addWidget(m_zone);
        hl->addLayout(col, 1);
        m_clock = new ClockFace;
        hl->addWidget(m_clock);
        content()->addWidget(hero);

        auto *sys = new Card;
        sys->addHeader(QStringLiteral("timer"), QStringLiteral("System clock"), QString());
        m_uptime = new InfoRow(QStringLiteral("Uptime"));
        m_source = new InfoRow(QStringLiteral("Time source"));
        sys->body()->addWidget(m_uptime);
        auto *line = new QFrame;
        line->setObjectName(QStringLiteral("hairline"));
        sys->body()->addWidget(line);
        sys->body()->addWidget(m_source);
        content()->addWidget(sys);

        auto *tz = new Card;
        tz->addHeader(QStringLiteral("public"), QStringLiteral("Time zone"), QStringLiteral("Daylight saving is handled automatically"));
        auto *grid = new QGridLayout;
        grid->setSpacing(8);
        int i = 0;
        for (const Zone &z : zones) {
            auto *b = new QPushButton(QStringLiteral("%1\n%2").arg(QString::fromLatin1(z.city), offsetText(z.minutes)));
            b->setObjectName(QStringLiteral("tile"));
            b->setCheckable(true);
            b->setCursor(Qt::PointingHandCursor);
            b->setMinimumHeight(52);
            QString id = QString::fromLatin1(z.id);
            QObject::connect(b, &QPushButton::clicked, b, [this, id] {
                if (sa_zone_set(id.toUtf8().constData()) < 0)
                    toast(QStringLiteral("Could not change the time zone"), Theme::red);
                else
                    toast(QStringLiteral("Time zone updated"));
                refresh();
            });
            grid->addWidget(b, i / 3, i % 3);
            m_zones.append({b, id});
            ++i;
        }
        tz->body()->addLayout(grid);
        content()->addWidget(tz);
        content()->addStretch(1);
    }

    void refresh() override
    {
        sa_time t;
        if (!sa_time(&t))
            return;
        QDateTime local = QDateTime::fromSecsSinceEpoch(t.unix_seconds + qint64(t.tz_minutes) * 60, QTimeZone::UTC);
        m_time->setText(local.toString(QStringLiteral("HH:mm")));
        m_seconds->setText(local.toString(QStringLiteral("ss")));
        m_date->setText(QLocale(QLocale::English).toString(local.date(), QStringLiteral("dddd, d MMMM yyyy")));
        m_clock->setTime(local.time());
        char zone[64] = {0};
        if (sa_zone_get(zone, sizeof(zone)) < 0)
            qstrcpy(zone, "UTC");
        QString current = QString::fromUtf8(zone);
        m_zone->setText(QStringLiteral("%1  -  %2").arg(current, offsetText(t.tz_minutes)));
        quint64 minutes = t.uptime_ms / 60000;
        m_uptime->setValue(minutes >= 60 ? QStringLiteral("%1h %2m").arg(minutes / 60).arg(minutes % 60)
                                         : QStringLiteral("%1m").arg(minutes));
        m_source->setValue(t.source == 1 ? QStringLiteral("Hardware RTC") : QStringLiteral("Software clock"));
        for (const auto &z : m_zones)
            z.first->setChecked(z.second == current);
    }

private:
    QLabel *m_time, *m_seconds, *m_date, *m_zone;
    ClockFace *m_clock;
    InfoRow *m_uptime, *m_source;
    QList<QPair<QPushButton *, QString>> m_zones;
};

Page *makeDateTimePage()
{
    return new DateTimePage;
}
