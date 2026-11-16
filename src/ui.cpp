#include "ui.h"

#include <QtGui/qpainter.h>
#include <QtGui/qpainterpath.h>

QPixmap tintedIcon(const QString &name, const QColor &color, int size)
{
    QPixmap source(QStringLiteral(":/icons/%1.png").arg(name));
    QPixmap out(size, size);
    out.fill(Qt::transparent);
    if (source.isNull())
        return out;
    QPainter p(&out);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.drawPixmap(QRect(0, 0, size, size), source);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(out.rect(), color);
    return out;
}

QString humanBytes(quint64 bytes)
{
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    double value = double(bytes);
    int unit = 0;
    while (value >= 1024.0 && unit < 4) {
        value /= 1024.0;
        ++unit;
    }
    return unit == 0 ? QStringLiteral("%1 B").arg(bytes)
                     : QStringLiteral("%1 %2").arg(value, 0, 'f', 1).arg(QLatin1String(units[unit]));
}

QString styleSheet()
{
    return QStringLiteral(R"(
QWidget { color: #cdd6f4; font-size: 13px; }
QWidget#root { background: #1e1e2e; }
QFrame#sidebar { background: #181825; border-right: 1px solid #262738; }
QScrollArea { background: #1e1e2e; border: none; }
QWidget#pageBody { background: #1e1e2e; }
QFrame#card { background: #26273a; border: 1px solid #33344a; border-radius: 12px; }
QFrame#hero { border: none; border-radius: 14px; }
QLabel#pageTitle { font-size: 24px; font-weight: bold; }
QLabel#pageSubtitle { color: #9aa0bc; }
QLabel#cardTitle { font-size: 15px; font-weight: bold; }
QLabel#cardSubtitle, QLabel#muted { color: #9aa0bc; font-size: 12px; }
QLabel#section { color: #6c7086; font-size: 11px; font-weight: bold; }
QLabel#big { font-size: 30px; font-weight: bold; }
QPushButton#nav { text-align: left; padding: 9px 12px; border: none; border-radius: 8px; background: transparent; color: #a6adc8; }
QPushButton#nav:hover { background: #222334; }
QPushButton#nav:checked { background: #2c2e45; color: #cdd6f4; }
QPushButton { background: #313244; border: 1px solid #3b3d54; border-radius: 8px; padding: 8px 16px; }
QPushButton:hover { background: #3b3d54; }
QPushButton:disabled { color: #6c7086; }
QPushButton#primary { background: #89b4fa; color: #1e1e2e; border: none; font-weight: bold; }
QPushButton#primary:hover { background: #a4c4fb; }
QPushButton#danger { background: #f38ba8; color: #1e1e2e; border: none; font-weight: bold; }
QPushButton#listRow { background: transparent; border: none; border-radius: 8px; padding: 0px; }
QPushButton#listRow:hover { background: #2e2f44; }
QPushButton#listRow:checked { background: #34364d; }
QPushButton#tile { background: #232435; border: 1px solid #33344a; border-radius: 10px; text-align: left; padding: 10px 14px; }
QPushButton#tile:hover { border-color: #585b70; }
QPushButton#tile:checked { background: #2f3550; border: 1px solid #89b4fa; }
QFrame#segmented { background: #232435; border-radius: 10px; }
QFrame#segmented QPushButton { background: transparent; border: none; border-radius: 8px; padding: 9px; color: #a6adc8; }
QFrame#segmented QPushButton:checked { background: #89b4fa; color: #1e1e2e; font-weight: bold; }
QSlider::groove:horizontal { height: 6px; background: #313244; border-radius: 3px; }
QSlider::sub-page:horizontal { background: #89b4fa; border-radius: 3px; }
QSlider::handle:horizontal { background: #ffffff; width: 16px; height: 16px; margin: -5px 0; border-radius: 8px; }
QLineEdit { background: #11111b; border: 1px solid #45475a; border-radius: 8px; padding: 8px 10px; }
QLineEdit:focus { border-color: #89b4fa; }
QProgressBar { background: #313244; border: none; border-radius: 5px; max-height: 10px; }
QProgressBar::chunk { background: #89b4fa; border-radius: 5px; }
QScrollBar:vertical { background: transparent; width: 10px; }
QScrollBar::handle:vertical { background: #3b3d54; border-radius: 5px; min-height: 30px; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
QFrame#hairline { background: #33344a; max-height: 1px; min-height: 1px; border: none; }
QFrame#toast { background: #313244; border: 1px solid #45475a; border-radius: 10px; }
QDialog { background: #1e1e2e; }
)");
}

IconBadge::IconBadge(const QString &icon, const QColor &color, int size, QWidget *parent)
    : QLabel(parent), m_size(size)
{
    setFixedSize(size, size);
    setAlignment(Qt::AlignCenter);
    setIcon(icon, color);
}

void IconBadge::setIcon(const QString &icon, const QColor &color)
{
    QColor bg = color;
    bg.setAlpha(40);
    setStyleSheet(QStringLiteral("background: rgba(%1,%2,%3,%4); border-radius: %5px;")
                      .arg(bg.red()).arg(bg.green()).arg(bg.blue()).arg(bg.alpha()).arg(m_size / 4));
    setPixmap(tintedIcon(icon, color, m_size * 5 / 9));
}

Chip::Chip(const QString &text, const QColor &color, QWidget *parent)
    : QLabel(parent)
{
    setChip(text, color);
}

void Chip::setChip(const QString &text, const QColor &color)
{
    setText(text);
    setStyleSheet(QStringLiteral("background: rgba(%1,%2,%3,46); color: %4; border-radius: 9px; padding: 3px 9px; font-size: 11px;")
                      .arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.name()));
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

Card::Card(QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("card"));
    m_body = new QVBoxLayout(this);
    m_body->setContentsMargins(20, 18, 20, 18);
    m_body->setSpacing(10);
}

QHBoxLayout *Card::addHeader(const QString &icon, const QString &title, const QString &subtitle, const QColor &color)
{
    auto *row = new QHBoxLayout;
    row->setSpacing(12);
    row->addWidget(new IconBadge(icon, color, 34));
    auto *texts = new QVBoxLayout;
    texts->setSpacing(1);
    auto *t = new QLabel(title);
    t->setObjectName(QStringLiteral("cardTitle"));
    texts->addWidget(t);
    if (!subtitle.isEmpty()) {
        auto *s = new QLabel(subtitle);
        s->setObjectName(QStringLiteral("cardSubtitle"));
        texts->addWidget(s);
    }
    row->addLayout(texts, 1);
    m_body->addLayout(row);
    return row;
}

InfoRow::InfoRow(const QString &label, const QString &value, QWidget *parent)
    : QWidget(parent)
{
    auto *row = new QHBoxLayout(this);
    row->setContentsMargins(4, 6, 4, 6);
    auto *l = new QLabel(label);
    l->setObjectName(QStringLiteral("muted"));
    l->setStyleSheet(QStringLiteral("font-size: 13px;"));
    m_value = new QLabel(value);
    m_value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_value->setTextInteractionFlags(Qt::TextSelectableByMouse);
    row->addWidget(l);
    row->addStretch(1);
    row->addWidget(m_value);
}

void InfoRow::setValue(const QString &value)
{
    m_value->setText(value);
}

Toggle::Toggle(QWidget *parent)
    : QAbstractButton(parent)
{
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setFixedSize(sizeHint());
}

void Toggle::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QRectF r = QRectF(rect()).adjusted(1, 1, -1, -1);
    p.setPen(Qt::NoPen);
    p.setBrush(isChecked() ? Theme::accent : Theme::raised);
    p.drawRoundedRect(r, r.height() / 2, r.height() / 2);
    qreal d = r.height() - 6;
    qreal x = isChecked() ? r.right() - d - 3 : r.left() + 3;
    p.setBrush(Qt::white);
    p.drawEllipse(QRectF(x, r.top() + 3, d, d));
}

Segmented::Segmented(const QStringList &items, QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("segmented"));
    auto *row = new QHBoxLayout(this);
    row->setContentsMargins(4, 4, 4, 4);
    row->setSpacing(4);
    for (int i = 0; i < items.size(); ++i) {
        auto *b = new QPushButton(items.at(i));
        b->setCheckable(true);
        b->setCursor(Qt::PointingHandCursor);
        connect(b, &QPushButton::clicked, this, [this, i] {
            setCurrent(i);
            emit changed(i);
        });
        row->addWidget(b, 1);
        m_buttons.append(b);
    }
}

void Segmented::setCurrent(int index)
{
    m_current = index;
    for (int i = 0; i < m_buttons.size(); ++i)
        m_buttons.at(i)->setChecked(i == index);
}

Ring::Ring(int size, QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(size, size);
}

void Ring::setValue(int percent, const QString &label)
{
    m_percent = qBound(0, percent, 100);
    m_label = label;
    update();
}

void Ring::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    qreal pen = width() / 9.0;
    QRectF r = QRectF(rect()).adjusted(pen, pen, -pen, -pen);
    p.setPen(QPen(Theme::raised, pen, Qt::SolidLine, Qt::RoundCap));
    p.drawArc(r, 0, 360 * 16);
    p.setPen(QPen(Theme::teal, pen, Qt::SolidLine, Qt::RoundCap));
    p.drawArc(r, 90 * 16, -int(m_percent * 360 * 16 / 100));
    p.setPen(Theme::text);
    QFont f = font();
    f.setPixelSize(width() / 5);
    p.setFont(f);
    p.drawText(rect(), Qt::AlignCenter, m_label);
}

LoadChart::LoadChart(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(120);
}

void LoadChart::push(int percent)
{
    m_values.append(qBound(0, percent, 100));
    while (m_values.size() > 48)
        m_values.removeFirst();
    update();
}

void LoadChart::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QRectF area = QRectF(rect()).adjusted(4, 6, -4, -4);
    p.setPen(QPen(QColor(0x30, 0x31, 0x45), 1));
    for (int i = 0; i <= 4; ++i) {
        qreal y = area.top() + area.height() * i / 4.0;
        p.drawLine(QPointF(area.left(), y), QPointF(area.right(), y));
    }
    if (m_values.isEmpty())
        return;
    qreal step = area.width() / 48.0;
    qreal bar = qMax<qreal>(2.0, step - 3.0);
    p.setPen(Qt::NoPen);
    for (int i = 0; i < m_values.size(); ++i) {
        int v = m_values.at(m_values.size() - 1 - i);
        qreal h = qMax<qreal>(3.0, area.height() * v / 100.0);
        QRectF r(area.right() - (i + 1) * step + (step - bar) / 2, area.bottom() - h, bar, h);
        QColor c = Theme::accent;
        c.setAlpha(i == 0 ? 255 : 170);
        p.setBrush(c);
        p.drawRoundedRect(r, 2, 2);
    }
}

Page::Page(const QString &title, const QString &subtitle, QWidget *parent)
    : QScrollArea(parent)
{
    setWidgetResizable(true);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *body = new QWidget;
    body->setObjectName(QStringLiteral("pageBody"));
    m_content = new QVBoxLayout(body);
    m_content->setContentsMargins(44, 28, 44, 32);
    m_content->setSpacing(16);
    auto *t = new QLabel(title);
    t->setObjectName(QStringLiteral("pageTitle"));
    auto *s = new QLabel(subtitle);
    s->setObjectName(QStringLiteral("pageSubtitle"));
    m_content->addWidget(t);
    m_content->addWidget(s);
    m_content->addSpacing(6);
    setWidget(body);
}

ListRow::ListRow(QWidget *parent)
    : QPushButton(parent)
{
    setObjectName(QStringLiteral("listRow"));
    setCursor(Qt::PointingHandCursor);
    setMinimumHeight(54);
    m_row = new QHBoxLayout(this);
    m_row->setContentsMargins(14, 8, 14, 8);
    m_row->setSpacing(14);
}
