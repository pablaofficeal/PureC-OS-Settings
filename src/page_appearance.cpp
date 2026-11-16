#include "pages.h"

#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtGui/qimagereader.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpainterpath.h>

static const char *const AppearPath = "/config/appear.ini";

struct ThemeInfo {
    const char *id;
    const char *title;
    QRgb desktop, window, titlebar, text, muted, accent, danger;
};

static const ThemeInfo themes[] = {
    {"catppuccin-dark", "Catppuccin Dark", 0x181825, 0x1E1E2E, 0x313244, 0xCDD6F4, 0x9399B2, 0x89B4FA, 0xF38BA8},
    {"nord", "Nord", 0x2E3440, 0x3B4252, 0x434C5E, 0xECEFF4, 0x9AA0B0, 0x88C0D0, 0xBF616A},
    {"dracula", "Dracula", 0x282A36, 0x2E3247, 0x44475A, 0xF8F8F2, 0x9AA0B2, 0xBD93F9, 0xFF5555},
    {"light", "Light", 0xE6E9EF, 0xEFF1F5, 0xDCE0E8, 0x4C4F69, 0x8C8FA1, 0x1E66F5, 0xD20F39},
    {"tokyo-night", "Tokyo Night", 0x1A1B26, 0x24283B, 0x292E42, 0xC0CAF5, 0x9AA5CE, 0x7AA2F7, 0xF7768E},
};

struct Appearance {
    QString theme = QStringLiteral("catppuccin-dark");
    QString wallpaper;
    QString mode = QStringLiteral("stretch");
    QString font = QStringLiteral("clean");
    int fontSize = 8;
};

static Appearance loadAppearance()
{
    Appearance a;
    QFile f(QString::fromLatin1(AppearPath));
    if (!f.open(QIODevice::ReadOnly))
        return a;
    for (const QByteArray &raw : f.readAll().split('\n')) {
        QByteArray line = raw.trimmed();
        int eq = line.indexOf('=');
        if (eq <= 0)
            continue;
        QByteArray key = line.left(eq).trimmed();
        QString value = QString::fromUtf8(line.mid(eq + 1).trimmed());
        if (key == "theme")
            a.theme = value;
        else if (key == "wallpaper")
            a.wallpaper = value;
        else if (key == "wallpaper_mode")
            a.mode = value;
        else if (key == "font")
            a.font = value;
        else if (key == "font_size")
            a.fontSize = qBound(8, value.toInt(), 24);
    }
    return a;
}

static bool saveAppearance(const Appearance &a)
{
    QDir().mkpath(QStringLiteral("/config"));
    QFile f(QString::fromLatin1(AppearPath));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    QByteArray body = "theme=" + a.theme.toUtf8() + "\nwallpaper=" + a.wallpaper.toUtf8() + "\nwallpaper_mode=" +
                      a.mode.toUtf8() + "\nfont=" + a.font.toUtf8() + "\nfont_size=" + QByteArray::number(a.fontSize) + "\n";
    return f.write(body) == body.size();
}

static QStringList wallpaperFiles()
{
    QStringList out;
    for (const char *list : {"/config/wplist.ini", "/demo/wplist.ini"}) {
        QFile f(QString::fromLatin1(list));
        if (!f.open(QIODevice::ReadOnly))
            continue;
        for (const QByteArray &raw : f.readAll().split('\n')) {
            QString line = QString::fromUtf8(raw.trimmed());
            if (!line.isEmpty() && !line.startsWith(QLatin1Char('#')) && QFileInfo::exists(line) && !out.contains(line))
                out.append(line);
        }
        if (!out.isEmpty())
            break;
    }
    for (const char *dir : {"/usr/share/wallpapers", "/walpaper", "/config"}) {
        QDir d(QString::fromLatin1(dir));
        const QStringList names = d.entryList({QStringLiteral("*.bmp"), QStringLiteral("*.png"), QStringLiteral("*.BMP"),
                                               QStringLiteral("*.PNG")},
                                              QDir::Files, QDir::Name);
        for (const QString &name : names) {
            QString path = d.filePath(name);
            if (!out.contains(path))
                out.append(path);
        }
    }
    return out;
}

static QPixmap themePreview(const ThemeInfo &t, int w, int h)
{
    QPixmap pm(w, h);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(t.desktop));
    p.drawRoundedRect(QRectF(0, 0, w, h), 8, 8);
    QRectF win(w * 0.1, h * 0.14, w * 0.8, h * 0.72);
    p.setBrush(QColor(t.window));
    p.drawRoundedRect(win, 5, 5);
    p.setBrush(QColor(t.titlebar));
    QPainterPath top;
    top.addRoundedRect(QRectF(win.left(), win.top(), win.width(), h * 0.15), 5, 5);
    p.drawPath(top);
    p.setBrush(QColor(t.accent));
    p.drawEllipse(QPointF(win.right() - 18, win.top() + h * 0.075), 3, 3);
    p.setBrush(QColor(t.danger));
    p.drawEllipse(QPointF(win.right() - 9, win.top() + h * 0.075), 3, 3);
    p.setBrush(QColor(t.text));
    p.drawRoundedRect(QRectF(win.left() + 9, win.top() + h * 0.24, win.width() * 0.55, 4), 2, 2);
    p.setBrush(QColor(t.muted));
    p.drawRoundedRect(QRectF(win.left() + 9, win.top() + h * 0.36, win.width() * 0.4, 3), 1.5, 1.5);
    p.drawRoundedRect(QRectF(win.left() + 9, win.top() + h * 0.46, win.width() * 0.48, 3), 1.5, 1.5);
    p.setBrush(QColor(t.accent));
    p.drawRoundedRect(QRectF(win.right() - win.width() * 0.32 - 8, win.bottom() - h * 0.17, win.width() * 0.32, h * 0.11), 3, 3);
    return pm;
}

static QPixmap thumbnail(const QString &path, const QColor &solid, int w, int h)
{
    QPixmap pm(w, h);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath clip;
    clip.addRoundedRect(QRectF(0, 0, w, h), 6, 6);
    p.setClipPath(clip);
    if (path.isEmpty()) {
        p.fillRect(pm.rect(), solid);
        return pm;
    }
    QImageReader reader(path);
    QSize size = reader.size();
    if (size.isValid())
        reader.setScaledSize(size.scaled(w * 2, h * 2, Qt::KeepAspectRatioByExpanding));
    QImage img = reader.read();
    if (img.isNull()) {
        QLinearGradient g(0, 0, w, h);
        g.setColorAt(0, QColor(0x4D, 0x5C, 0x9E));
        g.setColorAt(1, QColor(0x3A, 0x8E, 0x9C));
        p.fillRect(pm.rect(), g);
        p.drawPixmap((w - 20) / 2, (h - 20) / 2, tintedIcon(QStringLiteral("image"), Qt::white, 20));
        return pm;
    }
    p.drawImage(QRect(0, 0, w, h), img.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    return pm;
}

class AppearancePage : public Page
{
public:
    AppearancePage()
        : Page(QStringLiteral("Appearance"), QStringLiteral("Theme, wallpaper and fonts"))
    {
        m_state = loadAppearance();

        auto *themesCard = new Card;
        themesCard->addHeader(QStringLiteral("palette"), QStringLiteral("Color theme"), QStringLiteral("Desktop, panels and menus"));
        auto *row = new QHBoxLayout;
        row->setSpacing(14);
        for (const ThemeInfo &t : themes) {
            auto *col = new QVBoxLayout;
            col->setSpacing(8);
            auto *b = new QPushButton;
            b->setCheckable(true);
            b->setCursor(Qt::PointingHandCursor);
            b->setFixedSize(150, 100);
            b->setIcon(QIcon(themePreview(t, 136, 86)));
            b->setIconSize(QSize(136, 86));
            b->setStyleSheet(QStringLiteral("QPushButton { background: transparent; border: 2px solid transparent; border-radius: 11px; padding: 0px; }"
                                            "QPushButton:hover { border-color: #45475a; }"
                                            "QPushButton:checked { border-color: #89b4fa; }"));
            QString id = QString::fromLatin1(t.id);
            QObject::connect(b, &QPushButton::clicked, b, [this, id] {
                m_state.theme = id;
                apply();
            });
            auto *label = new QLabel(QString::fromLatin1(t.title));
            col->addWidget(b);
            col->addWidget(label);
            row->addLayout(col);
            m_themeButtons.append({b, id});
            m_themeLabels.append(label);
        }
        row->addStretch(1);
        themesCard->body()->addLayout(row);
        content()->addWidget(themesCard);

        auto *wp = new Card;
        wp->addHeader(QStringLiteral("image"), QStringLiteral("Wallpaper"), QStringLiteral("BMP or PNG, stretched. Add your own to /usr/share/wallpapers"));
        m_wallpapers = new QVBoxLayout;
        m_wallpapers->setSpacing(2);
        wp->body()->addLayout(m_wallpapers);
        content()->addWidget(wp);

        auto *fonts = new Card;
        fonts->addHeader(QStringLiteral("text_fields"), QStringLiteral("Desktop font"), QStringLiteral("Labels of the desktop and the panel"));
        auto *faceLabel = new QLabel(QStringLiteral("Style"));
        faceLabel->setObjectName(QStringLiteral("muted"));
        fonts->body()->addWidget(faceLabel);
        m_face = new Segmented({QStringLiteral("Clean"), QStringLiteral("Classic"), QStringLiteral("Bold")});
        QObject::connect(m_face, &Segmented::changed, [this](int i) {
            static const char *ids[] = {"clean", "classic", "bold"};
            m_state.font = QString::fromLatin1(ids[qBound(0, i, 2)]);
            apply();
        });
        fonts->body()->addWidget(m_face);
        auto *sizeLabel = new QLabel(QStringLiteral("Size"));
        sizeLabel->setObjectName(QStringLiteral("muted"));
        fonts->body()->addWidget(sizeLabel);
        m_size = new Segmented({QStringLiteral("8"), QStringLiteral("10"), QStringLiteral("12"), QStringLiteral("16"),
                                QStringLiteral("24")});
        QObject::connect(m_size, &Segmented::changed, [this](int i) {
            static const int sizes[] = {8, 10, 12, 16, 24};
            m_state.fontSize = sizes[qBound(0, i, 4)];
            apply();
        });
        fonts->body()->addWidget(m_size);
        content()->addWidget(fonts);
        content()->addStretch(1);
        rebuildWallpapers();
        sync();
    }

private:
    const ThemeInfo &currentTheme() const
    {
        for (const ThemeInfo &t : themes)
            if (m_state.theme == QLatin1String(t.id))
                return t;
        return themes[0];
    }

    void apply()
    {
        if (!saveAppearance(m_state))
            toast(QStringLiteral("Cannot write /config/appear.ini"), Theme::red);
        else
            toast(QStringLiteral("Appearance updated"));
        sync();
        rebuildWallpapers();
    }

    void sync()
    {
        for (int i = 0; i < m_themeButtons.size(); ++i) {
            bool on = m_themeButtons.at(i).second == m_state.theme;
            m_themeButtons.at(i).first->setChecked(on);
            m_themeLabels.at(i)->setStyleSheet(on ? QStringLiteral("font-weight: bold;") : QStringLiteral("color: #9aa0bc;"));
        }
        m_face->setCurrent(m_state.font == QLatin1String("classic") ? 1 : m_state.font == QLatin1String("bold") ? 2 : 0);
        static const int sizes[] = {8, 10, 12, 16, 24};
        int best = 0;
        for (int i = 0; i < 5; ++i)
            if (qAbs(sizes[i] - m_state.fontSize) < qAbs(sizes[best] - m_state.fontSize))
                best = i;
        m_size->setCurrent(best);
    }

    void rebuildWallpapers()
    {
        while (QLayoutItem *item = m_wallpapers->takeAt(0)) {
            if (QWidget *gone = item->widget()) {
                gone->hide();
                gone->deleteLater();
            }
            delete item;
        }
        QStringList paths = wallpaperFiles();
        paths.prepend(QString());
        QColor solid(currentTheme().desktop);
        for (const QString &path : paths) {
            bool active = path == m_state.wallpaper;
            auto *row = new ListRow;
            row->setCheckable(true);
            row->setChecked(active);
            auto *thumb = new QLabel;
            thumb->setPixmap(thumbnail(path, solid, 56, 34));
            thumb->setStyleSheet(QStringLiteral("background: transparent;"));
            row->row()->addWidget(thumb);
            auto *texts = new QVBoxLayout;
            texts->setSpacing(1);
            auto *name = new QLabel(path.isEmpty() ? QStringLiteral("Solid color") : QFileInfo(path).fileName());
            name->setStyleSheet(QStringLiteral("font-weight: bold; background: transparent;"));
            auto *meta = new QLabel(path.isEmpty() ? QStringLiteral("Uses the theme desktop color") : path);
            meta->setObjectName(QStringLiteral("muted"));
            meta->setStyleSheet(QStringLiteral("background: transparent;"));
            texts->addWidget(name);
            texts->addWidget(meta);
            row->row()->addLayout(texts, 1);
            auto *radio = new QLabel;
            radio->setFixedSize(18, 18);
            radio->setStyleSheet(active ? QStringLiteral("border: 5px solid #89b4fa; border-radius: 9px; background: #1e1e2e;")
                                        : QStringLiteral("border: 2px solid #585b70; border-radius: 9px; background: transparent;"));
            row->row()->addWidget(radio);
            QObject::connect(row, &QPushButton::clicked, row, [this, path] {
                m_state.wallpaper = path;
                apply();
            });
            m_wallpapers->addWidget(row);
        }
    }

    Appearance m_state;
    QList<QPair<QPushButton *, QString>> m_themeButtons;
    QList<QLabel *> m_themeLabels;
    QVBoxLayout *m_wallpapers;
    Segmented *m_face, *m_size;
};

Page *makeAppearancePage()
{
    return new AppearancePage;
}
