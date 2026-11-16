#include "pages.h"
#include "sysapi.h"

#include <QtWidgets/qdialog.h>
#include <QtWidgets/qlineedit.h>

static QString ipText(uint32_t ip)
{
    return QStringLiteral("%1.%2.%3.%4").arg(ip & 0xFF).arg((ip >> 8) & 0xFF).arg((ip >> 16) & 0xFF).arg(ip >> 24);
}

static QString securityText(int security)
{
    switch (security) {
    case SA_SEC_OPEN: return QStringLiteral("Open");
    case SA_SEC_WEP: return QStringLiteral("WEP");
    case SA_SEC_WPA2: return QStringLiteral("WPA2");
    case SA_SEC_WPA3: return QStringLiteral("WPA3");
    default: return QStringLiteral("WPA2/WPA3");
    }
}

static QString signalIcon(int rssi)
{
    if (rssi >= -55)
        return QStringLiteral("wifi");
    if (rssi >= -67)
        return QStringLiteral("network_wifi_3_bar");
    if (rssi >= -78)
        return QStringLiteral("network_wifi_2_bar");
    return QStringLiteral("network_wifi_1_bar");
}

static QString askPassword(QWidget *parent, const QString &ssid)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(QStringLiteral("Connect to %1").arg(ssid));
    auto *col = new QVBoxLayout(&dialog);
    col->setContentsMargins(24, 22, 24, 20);
    col->setSpacing(12);
    auto *title = new QLabel(QStringLiteral("Password for \"%1\"").arg(ssid));
    title->setObjectName(QStringLiteral("cardTitle"));
    col->addWidget(title);
    auto *edit = new QLineEdit;
    edit->setEchoMode(QLineEdit::Password);
    edit->setMinimumWidth(320);
    col->addWidget(edit);
    auto *buttons = new QHBoxLayout;
    buttons->addStretch(1);
    auto *cancel = new QPushButton(QStringLiteral("Cancel"));
    auto *ok = new QPushButton(QStringLiteral("Connect"));
    ok->setObjectName(QStringLiteral("primary"));
    ok->setDefault(true);
    buttons->addWidget(cancel);
    buttons->addWidget(ok);
    col->addLayout(buttons);
    QObject::connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(ok, &QPushButton::clicked, &dialog, &QDialog::accept);
    QObject::connect(edit, &QLineEdit::returnPressed, &dialog, &QDialog::accept);
    edit->setFocus();
    if (dialog.exec() != QDialog::Accepted)
        return QString();
    return edit->text().isEmpty() ? QString() : edit->text();
}

class NetworkPage : public Page
{
public:
    NetworkPage()
        : Page(QStringLiteral("Network & Internet"), QStringLiteral("Wi-Fi, wired adapters and connectivity"))
    {
        auto *status = new Card;
        auto *sl = new QHBoxLayout;
        sl->setSpacing(18);
        m_badge = new IconBadge(QStringLiteral("wifi_off"), Theme::muted, 64);
        sl->addWidget(m_badge);
        auto *col = new QVBoxLayout;
        col->setSpacing(4);
        m_title = new QLabel;
        m_title->setStyleSheet(QStringLiteral("font-size: 20px; font-weight: bold;"));
        m_detail = new QLabel;
        m_detail->setObjectName(QStringLiteral("muted"));
        auto *chips = new QHBoxLayout;
        chips->setSpacing(6);
        m_internet = new Chip(QStringLiteral("Internet"), Theme::green);
        m_channel = new Chip(QString(), Theme::teal);
        m_security = new Chip(QString(), Theme::accent);
        chips->addWidget(m_internet);
        chips->addWidget(m_channel);
        chips->addWidget(m_security);
        chips->addStretch(1);
        col->addWidget(m_title);
        col->addWidget(m_detail);
        col->addLayout(chips);
        sl->addLayout(col, 1);
        m_disconnect = new QPushButton(QStringLiteral("Disconnect"));
        QObject::connect(m_disconnect, &QPushButton::clicked, [this] {
            if (sa_wifi_disconnect() < 0)
                toast(QStringLiteral("Could not disconnect"), Theme::red);
            refresh();
        });
        sl->addWidget(m_disconnect, 0, Qt::AlignVCenter);
        status->body()->addLayout(sl);
        content()->addWidget(status);

        auto *wifi = new Card;
        auto *head = wifi->addHeader(QStringLiteral("wifi"), QStringLiteral("Wi-Fi"), QStringLiteral("Searching..."));
        m_count = qobject_cast<QLabel *>(head->itemAt(1)->layout()->itemAt(1)->widget());
        auto *scan = new QPushButton(QStringLiteral(" Scan"));
        scan->setIcon(QIcon(tintedIcon(QStringLiteral("refresh"), Theme::text, 32)));
        QObject::connect(scan, &QPushButton::clicked, [this] {
            if (sa_wifi_scan() < 0)
                toast(QStringLiteral("Wi-Fi scan failed"), Theme::red);
            else
                toast(QStringLiteral("Scanning..."), Theme::accent);
        });
        head->addWidget(scan);
        auto *line = new QFrame;
        line->setObjectName(QStringLiteral("hairline"));
        wifi->body()->addWidget(line);
        m_list = new QVBoxLayout;
        m_list->setSpacing(2);
        wifi->body()->addLayout(m_list);
        content()->addWidget(wifi);

        auto *adapters = new Card;
        adapters->addHeader(QStringLiteral("lan"), QStringLiteral("Adapters"), QStringLiteral("Network interfaces of this computer"));
        m_ifaces = new QVBoxLayout;
        m_ifaces->setSpacing(2);
        adapters->body()->addLayout(m_ifaces);
        content()->addWidget(adapters);
        content()->addStretch(1);
        sa_wifi_scan();
    }

    void refresh() override
    {
        sa_wifi_status st;
        bool ok = sa_wifi_status(&st);
        m_connected = ok && st.connected ? QString::fromUtf8(st.ssid) : QString();
        if (!ok || !st.device) {
            m_badge->setIcon(QStringLiteral("wifi_off"), Theme::muted);
            m_title->setText(QStringLiteral("No Wi-Fi adapter"));
            m_detail->setText(QStringLiteral("Wired adapters are listed below"));
        } else if (st.connected) {
            m_badge->setIcon(QStringLiteral("wifi"), Theme::green);
            m_title->setText(QStringLiteral("Connected to %1").arg(QString::fromUtf8(st.ssid)));
            m_detail->setText(st.ip ? QStringLiteral("%1  -  %2").arg(QString::fromUtf8(st.iface), ipText(st.ip))
                                    : QString::fromUtf8(st.iface));
            m_channel->setChip(QStringLiteral("Channel %1").arg(st.channel), Theme::teal);
            m_security->setChip(securityText(st.security), Theme::accent);
        } else {
            m_badge->setIcon(QStringLiteral("wifi"), Theme::muted);
            m_title->setText(QStringLiteral("Not connected"));
            m_detail->setText(QStringLiteral("Choose a network below"));
        }
        bool online = ok && st.connected;
        m_internet->setVisible(online && st.ip);
        m_channel->setVisible(online);
        m_security->setVisible(online);
        m_disconnect->setVisible(online);
        refreshNetworks();
        refreshIfaces();
    }

private:
    static void clear(QLayout *layout)
    {
        while (QLayoutItem *item = layout->takeAt(0)) {
            delete item->widget();
            delete item;
        }
    }

    void refreshNetworks()
    {
        sa_wifi_net nets[48];
        int n = sa_wifi_list(nets, 48);
        if (n < 0)
            n = 0;
        QString signature = m_connected;
        for (int i = 0; i < n; ++i)
            signature += QStringLiteral("|%1:%2:%3").arg(QString::fromUtf8(nets[i].ssid)).arg(nets[i].rssi / 6).arg(nets[i].security);
        if (signature == m_signature)
            return;
        m_signature = signature;
        clear(m_list);
        if (m_count)
            m_count->setText(QStringLiteral("%1 network%2 available").arg(n).arg(n == 1 ? "" : "s"));
        if (!n) {
            auto *empty = new QLabel(QStringLiteral("No networks found yet"));
            empty->setObjectName(QStringLiteral("muted"));
            empty->setContentsMargins(14, 10, 14, 10);
            m_list->addWidget(empty);
            return;
        }
        for (int i = 0; i < n; ++i) {
            QString ssid = QString::fromUtf8(nets[i].ssid);
            bool hidden = ssid.isEmpty();
            bool current = !hidden && ssid == m_connected;
            auto *row = new ListRow;
            auto *icon = new QLabel;
            icon->setPixmap(tintedIcon(signalIcon(nets[i].rssi), current ? Theme::green : Theme::muted, 20));
            icon->setStyleSheet(QStringLiteral("background: transparent;"));
            row->row()->addWidget(icon);
            auto *texts = new QVBoxLayout;
            texts->setSpacing(1);
            auto *name = new QLabel(hidden ? QStringLiteral("Hidden network") : ssid);
            name->setStyleSheet(hidden ? QStringLiteral("color: #9aa0bc; background: transparent;")
                                       : QStringLiteral("font-weight: bold; background: transparent;"));
            auto *meta = new QLabel(QStringLiteral("%1  -  ch %2  -  %3 GHz  -  %4 dBm")
                                        .arg(securityText(nets[i].security))
                                        .arg(nets[i].channel)
                                        .arg(nets[i].channel > 14 ? QStringLiteral("5") : QStringLiteral("2.4"))
                                        .arg(nets[i].rssi));
            meta->setObjectName(QStringLiteral("muted"));
            meta->setStyleSheet(QStringLiteral("background: transparent;"));
            texts->addWidget(name);
            texts->addWidget(meta);
            row->row()->addLayout(texts, 1);
            if (current) {
                row->row()->addWidget(new Chip(QStringLiteral("Connected"), Theme::green));
            } else if (nets[i].security != SA_SEC_OPEN) {
                auto *lock = new QLabel;
                lock->setPixmap(tintedIcon(QStringLiteral("lock"), Theme::faint, 16));
                lock->setStyleSheet(QStringLiteral("background: transparent;"));
                row->row()->addWidget(lock);
            }
            int security = nets[i].security;
            QObject::connect(row, &QPushButton::clicked, row, [this, ssid, security, current, hidden] {
                if (current || hidden)
                    return;
                QString password;
                if (security != SA_SEC_OPEN) {
                    password = askPassword(this, ssid);
                    if (password.isEmpty())
                        return;
                }
                toast(QStringLiteral("Connecting to %1...").arg(ssid), Theme::accent);
                int rc = sa_wifi_connect(ssid.toUtf8().constData(), password.toUtf8().constData());
                if (rc < 0)
                    toast(QStringLiteral("Could not connect to %1").arg(ssid), Theme::red);
                else
                    toast(QStringLiteral("Connected to %1").arg(ssid), Theme::green);
                m_signature.clear();
                refresh();
            });
            m_list->addWidget(row);
        }
    }

    void refreshIfaces()
    {
        sa_iface list[8];
        int n = sa_ifaces(list, 8);
        if (n < 0)
            n = 0;
        QString signature;
        for (int i = 0; i < n; ++i)
            signature += QStringLiteral("|%1:%2:%3:%4").arg(QString::fromUtf8(list[i].name)).arg(list[i].link).arg(list[i].ip).arg(list[i].has_ip);
        if (signature == m_ifSignature)
            return;
        m_ifSignature = signature;
        clear(m_ifaces);
        for (int i = 0; i < n; ++i) {
            auto *row = new QWidget;
            auto *rl = new QHBoxLayout(row);
            rl->setContentsMargins(14, 8, 14, 8);
            rl->setSpacing(14);
            auto *icon = new QLabel;
            QString name = QString::fromUtf8(list[i].name);
            icon->setPixmap(tintedIcon(name.startsWith(QLatin1String("wl")) ? QStringLiteral("wifi") : QStringLiteral("lan"),
                                       list[i].link ? Theme::accent : Theme::faint, 20));
            rl->addWidget(icon);
            auto *texts = new QVBoxLayout;
            texts->setSpacing(1);
            auto *title = new QLabel(name);
            title->setStyleSheet(QStringLiteral("font-weight: bold;"));
            const uint8_t *m = list[i].mac;
            auto *meta = new QLabel(QStringLiteral("%1  -  MAC %2:%3:%4:%5:%6:%7")
                                        .arg(list[i].has_ip ? ipText(list[i].ip) : QStringLiteral("no address"))
                                        .arg(m[0], 2, 16, QLatin1Char('0')).arg(m[1], 2, 16, QLatin1Char('0'))
                                        .arg(m[2], 2, 16, QLatin1Char('0')).arg(m[3], 2, 16, QLatin1Char('0'))
                                        .arg(m[4], 2, 16, QLatin1Char('0')).arg(m[5], 2, 16, QLatin1Char('0')));
            meta->setObjectName(QStringLiteral("muted"));
            texts->addWidget(title);
            texts->addWidget(meta);
            rl->addLayout(texts, 1);
            rl->addWidget(new Chip(list[i].link ? QStringLiteral("Link up") : QStringLiteral("No link"),
                                   list[i].link ? Theme::green : Theme::faint));
            m_ifaces->addWidget(row);
        }
    }

    IconBadge *m_badge;
    QLabel *m_title, *m_detail, *m_count = nullptr;
    Chip *m_internet, *m_channel, *m_security;
    QPushButton *m_disconnect;
    QVBoxLayout *m_list, *m_ifaces;
    QString m_connected, m_signature, m_ifSignature;
};

Page *makeNetworkPage()
{
    return new NetworkPage;
}
