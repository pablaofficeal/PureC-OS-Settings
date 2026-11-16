#include "pages.h"

#include <QtCore/qtimer.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qpainter.h>
#include <QtGui/qscreen.h>
#include <QtWidgets/qgridlayout.h>

extern "C" {
#include "vb.h"
}

static QString ratioText(int w, int h)
{
    struct Known {
        int a, b;
    };
    static const Known known[] = {{16, 9}, {16, 10}, {4, 3}, {5, 4}, {21, 9}};
    for (const Known &k : known)
        if (qAbs(double(w) / h - double(k.a) / k.b) < 0.012)
            return QStringLiteral("%1:%2").arg(k.a).arg(k.b);
    int a = w, b = h;
    while (b) {
        int t = a % b;
        a = b;
        b = t;
    }
    return QStringLiteral("%1:%2").arg(w / a).arg(h / a);
}

class MonitorPreview : public QWidget
{
public:
    MonitorPreview()
    {
        setFixedSize(290, 190);
    }
    void setRatio(double r)
    {
        m_ratio = r;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        qreal maxW = width() - 10, maxH = height() - 34;
        qreal w = maxW, h = w / m_ratio;
        if (h > maxH) {
            h = maxH;
            w = h * m_ratio;
        }
        QRectF frame((width() - w) / 2, 4, w, h);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0x0B, 0x0B, 0x12));
        p.drawRoundedRect(frame, 10, 10);
        QRectF screen = frame.adjusted(9, 9, -9, -9);
        QLinearGradient g(screen.topLeft(), screen.topRight());
        g.setColorAt(0, QColor(0x4D, 0x5C, 0x9E));
        g.setColorAt(1, QColor(0x3A, 0x8E, 0x9C));
        p.setBrush(g);
        p.drawRect(screen);
        p.setBrush(QColor(255, 255, 255, 215));
        p.drawRect(QRectF(screen.left() + screen.width() * 0.16, screen.top() + screen.height() * 0.22,
                          screen.width() * 0.4, screen.height() * 0.4));
        p.setBrush(QColor(255, 255, 255, 120));
        p.drawRect(QRectF(screen.left() + screen.width() * 0.48, screen.top() + screen.height() * 0.38,
                          screen.width() * 0.32, screen.height() * 0.4));
        p.setBrush(QColor(0x30, 0x34, 0x4A));
        p.drawRect(QRectF(screen.left(), screen.bottom() - screen.height() * 0.07, screen.width(), screen.height() * 0.07));
        p.setBrush(QColor(0x0B, 0x0B, 0x12));
        p.drawRect(QRectF(width() / 2.0 - 30, frame.bottom(), 60, 14));
        p.drawRoundedRect(QRectF(width() / 2.0 - 60, frame.bottom() + 12, 120, 8), 4, 4);
    }

private:
    double m_ratio = 16.0 / 9.0;
};

class DisplayPage : public Page
{
public:
    DisplayPage()
        : Page(QStringLiteral("Display"), QStringLiteral("Resolution of the screen"))
    {
        auto *hero = new Card;
        auto *hl = new QHBoxLayout;
        hl->setSpacing(28);
        m_preview = new MonitorPreview;
        hl->addWidget(m_preview);
        auto *col = new QVBoxLayout;
        col->setSpacing(6);
        auto *cap = new QLabel(QStringLiteral("CURRENT MODE"));
        cap->setObjectName(QStringLiteral("section"));
        m_mode = new QLabel;
        m_mode->setObjectName(QStringLiteral("big"));
        m_detail = new QLabel;
        m_detail->setObjectName(QStringLiteral("muted"));
        m_detail->setStyleSheet(QStringLiteral("font-size: 13px;"));
        auto *chips = new QHBoxLayout;
        chips->setSpacing(6);
        chips->addWidget(new Chip(QStringLiteral("Live switching"), Theme::green));
        chips->addWidget(new Chip(QStringLiteral("Saved for next boot"), Theme::teal));
        chips->addStretch(1);
        col->addStretch(1);
        col->addWidget(cap);
        col->addWidget(m_mode);
        col->addWidget(m_detail);
        col->addSpacing(6);
        col->addLayout(chips);
        col->addStretch(1);
        hl->addLayout(col, 1);
        hero->body()->addLayout(hl);
        content()->addWidget(hero);

        auto *modes = new Card;
        modes->addHeader(QStringLiteral("aspect_ratio"), QStringLiteral("Resolution"),
                         QStringLiteral("Works in QEMU and VirtualBox; real UEFI keeps the boot mode"));
        auto *grid = new QGridLayout;
        grid->setSpacing(10);
        static const QSize list[] = {{1024, 768}, {1280, 720}, {1280, 800}, {1366, 768},
                                     {1440, 900}, {1600, 900}, {1920, 1080}, {2560, 1440}};
        int i = 0;
        for (const QSize &s : list) {
            auto *tile = new QPushButton(QStringLiteral("%1 x %2\n%3").arg(s.width()).arg(s.height()).arg(ratioText(s.width(), s.height())));
            tile->setObjectName(QStringLiteral("tile"));
            tile->setCheckable(true);
            tile->setCursor(Qt::PointingHandCursor);
            tile->setMinimumHeight(58);
            QObject::connect(tile, &QPushButton::clicked, tile, [this, s] { requestMode(s); });
            grid->addWidget(tile, i / 4, i % 4);
            m_tiles.append({tile, s});
            ++i;
        }
        modes->body()->addLayout(grid);
        content()->addWidget(modes);
        content()->addStretch(1);
        if (QScreen *screen = QGuiApplication::primaryScreen())
            QObject::connect(screen, &QScreen::geometryChanged, this, [this] {
                if (m_pending.isValid()) {
                    toast(QStringLiteral("Resolution changed to %1 x %2").arg(m_pending.width()).arg(m_pending.height()));
                    m_pending = QSize();
                }
                refresh();
            });
    }

    void refresh() override
    {
        QScreen *screen = QGuiApplication::primaryScreen();
        if (!screen)
            return;
        QSize s = screen->geometry().size();
        m_mode->setText(QStringLiteral("%1 x %2").arg(s.width()).arg(s.height()));
        m_detail->setText(QStringLiteral("%1  -  32-bit color").arg(ratioText(s.width(), s.height())));
        m_preview->setRatio(double(s.width()) / qMax(1, s.height()));
        for (const auto &t : m_tiles)
            t.first->setChecked(t.second == s);
    }

private:
    void requestMode(const QSize &s)
    {
        refresh();
        QScreen *screen = QGuiApplication::primaryScreen();
        if (screen && screen->geometry().size() == s)
            return;
        if (vb_server_version() < 3) {
            toast(QStringLiteral("The display server is too old to switch modes"), Theme::red);
            return;
        }
        m_pending = s;
        toast(QStringLiteral("Switching to %1 x %2...").arg(s.width()).arg(s.height()), Theme::accent);
        vb_set_mode(s.width(), s.height());
        QTimer::singleShot(3000, this, [this, s] {
            if (m_pending == s) {
                m_pending = QSize();
                toast(QStringLiteral("This mode is not available on this display"), Theme::red);
                refresh();
            }
        });
    }

    MonitorPreview *m_preview;
    QLabel *m_mode, *m_detail;
    QList<QPair<QPushButton *, QSize>> m_tiles;
    QSize m_pending;
};

Page *makeDisplayPage()
{
    return new DisplayPage;
}
