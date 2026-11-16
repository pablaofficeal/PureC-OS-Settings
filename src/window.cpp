#include "window.h"
#include "pages.h"
#include "sysapi.h"

#include <QtCore/qpointer.h>
#include <QtGui/qpainter.h>
#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qpushbutton.h>

static QPointer<MainWindow> g_window;

void toast(const QString &text, const QColor &color)
{
    if (g_window)
        g_window->showToast(text, color);
}

static QPixmap avatar(const QString &name, int size)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0x58, 0x6B, 0x9A));
    p.drawEllipse(pm.rect());
    p.setPen(Qt::white);
    QFont f = p.font();
    f.setPixelSize(size * 2 / 5);
    f.setBold(true);
    p.setFont(f);
    p.drawText(pm.rect(), Qt::AlignCenter, name.left(1).toUpper());
    return pm;
}

static QPixmap appIcon(int size)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    QLinearGradient g(0, 0, size, size);
    g.setColorAt(0, QColor(0x89, 0xB4, 0xFA));
    g.setColorAt(1, QColor(0x9D, 0x7C, 0xF0));
    p.setPen(Qt::NoPen);
    p.setBrush(g);
    p.drawRoundedRect(QRectF(0, 0, size, size), size / 4.0, size / 4.0);
    int inner = size * 3 / 5;
    p.drawPixmap((size - inner) / 2, (size - inner) / 2, tintedIcon(QStringLiteral("settings"), Qt::white, inner));
    return pm;
}

MainWindow::MainWindow()
{
    g_window = this;
    setObjectName(QStringLiteral("root"));
    setAttribute(Qt::WA_StyledBackground);
    setWindowTitle(QStringLiteral("Settings"));
    auto *row = new QHBoxLayout(this);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);
    m_stack = new QStackedWidget;
    row->addWidget(buildSidebar());
    row->addWidget(m_stack, 1);
    m_toast = new QFrame(this);
    m_toast->setObjectName(QStringLiteral("toast"));
    auto *tl = new QHBoxLayout(m_toast);
    tl->setContentsMargins(16, 10, 16, 10);
    m_toastText = new QLabel;
    tl->addWidget(m_toastText);
    m_toast->hide();
    m_toastTimer.setSingleShot(true);
    connect(&m_toastTimer, &QTimer::timeout, m_toast, &QWidget::hide);
    connect(&m_timer, &QTimer::timeout, this, [this] {
        if (auto *page = static_cast<Page *>(m_stack->currentWidget()))
            page->refresh();
    });
    m_timer.start(1000);
    select(0);
    resize(1180, 780);
}

QWidget *MainWindow::buildSidebar()
{
    auto *side = new QFrame;
    side->setObjectName(QStringLiteral("sidebar"));
    side->setFixedWidth(252);
    auto *col = new QVBoxLayout(side);
    col->setContentsMargins(14, 20, 14, 16);
    col->setSpacing(2);

    auto *head = new QHBoxLayout;
    head->setSpacing(12);
    auto *icon = new QLabel;
    icon->setPixmap(appIcon(40));
    head->addWidget(icon);
    auto *names = new QVBoxLayout;
    names->setSpacing(0);
    auto *title = new QLabel(QStringLiteral("Settings"));
    title->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: bold;"));
    auto *ver = new QLabel(osName());
    ver->setObjectName(QStringLiteral("muted"));
    names->addWidget(title);
    names->addWidget(ver);
    head->addLayout(names, 1);
    col->addLayout(head);
    col->addSpacing(16);

    char user[32] = {0};
    uint32_t uid = 0;
    sa_user(user, sizeof(user), &uid);
    auto *card = new QFrame;
    card->setStyleSheet(QStringLiteral("QFrame { background: #222334; border-radius: 10px; }"));
    auto *cl = new QHBoxLayout(card);
    cl->setContentsMargins(10, 8, 10, 8);
    cl->setSpacing(10);
    auto *av = new QLabel;
    av->setPixmap(avatar(QString::fromUtf8(user), 32));
    cl->addWidget(av);
    auto *ul = new QVBoxLayout;
    ul->setSpacing(0);
    auto *un = new QLabel(QString::fromUtf8(user));
    un->setStyleSheet(QStringLiteral("font-weight: bold; background: transparent;"));
    auto *role = new QLabel(uid == 0 ? QStringLiteral("Administrator") : QStringLiteral("Standard user"));
    role->setObjectName(QStringLiteral("muted"));
    role->setStyleSheet(QStringLiteral("background: transparent;"));
    ul->addWidget(un);
    ul->addWidget(role);
    cl->addLayout(ul, 1);
    col->addWidget(card);
    col->addSpacing(12);

    addSection(col, QStringLiteral("SYSTEM"));
    addPage(col, QStringLiteral("info"), QStringLiteral("About"), makeAboutPage());
    addPage(col, QStringLiteral("wifi"), QStringLiteral("Network"), makeNetworkPage());
    addPage(col, QStringLiteral("volume_up"), QStringLiteral("Sound"), makeSoundPage());
    addPage(col, QStringLiteral("desktop_windows"), QStringLiteral("Display"), makeDisplayPage());
    addSection(col, QStringLiteral("PERSONAL"));
    addPage(col, QStringLiteral("palette"), QStringLiteral("Appearance"), makeAppearancePage());
    addSection(col, QStringLiteral("DEVICE"));
    addPage(col, QStringLiteral("hard_drive"), QStringLiteral("Storage"), makeStoragePage());
    addPage(col, QStringLiteral("schedule"), QStringLiteral("Date & time"), makeDateTimePage());
    addPage(col, QStringLiteral("power_settings_new"), QStringLiteral("Power"), makePowerPage());
    col->addStretch(1);
    return side;
}

void MainWindow::addSection(QVBoxLayout *layout, const QString &title)
{
    layout->addSpacing(10);
    auto *l = new QLabel(title);
    l->setObjectName(QStringLiteral("section"));
    l->setContentsMargins(12, 0, 0, 4);
    layout->addWidget(l);
}

void MainWindow::addPage(QVBoxLayout *layout, const QString &icon, const QString &title, Page *page)
{
    int index = m_pages.size();
    auto *b = new QPushButton(QStringLiteral("  ") + title);
    b->setObjectName(QStringLiteral("nav"));
    b->setCheckable(true);
    b->setCursor(Qt::PointingHandCursor);
    b->setIcon(QIcon(tintedIcon(icon, Theme::muted, 36)));
    b->setIconSize(QSize(18, 18));
    connect(b, &QPushButton::clicked, this, [this, index] { select(index); });
    layout->addWidget(b);
    m_nav.append(b);
    m_pages.append(page);
    m_stack->addWidget(page);
}

void MainWindow::select(int index)
{
    for (int i = 0; i < m_nav.size(); ++i)
        m_nav.at(i)->setChecked(i == index);
    m_stack->setCurrentIndex(index);
    m_pages.at(index)->refresh();
}

void MainWindow::showToast(const QString &text, const QColor &color)
{
    m_toastText->setText(text);
    m_toastText->setStyleSheet(QStringLiteral("color: %1; font-weight: bold; background: transparent;").arg(color.name()));
    m_toast->adjustSize();
    placeToast();
    m_toast->show();
    m_toast->raise();
    m_toastTimer.start(2600);
}

void MainWindow::placeToast()
{
    if (!m_toast)
        return;
    int x = 252 + (width() - 252 - m_toast->width()) / 2;
    m_toast->move(x, height() - m_toast->height() - 24);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    placeToast();
}
