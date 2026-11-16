#include "pages.h"
#include "sysapi.h"

#include <QtWidgets/qdialog.h>
#include <QtWidgets/qprogressbar.h>

static bool confirm(QWidget *parent, const QString &title, const QString &text, const QString &action)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    auto *col = new QVBoxLayout(&dialog);
    col->setContentsMargins(26, 24, 26, 20);
    col->setSpacing(10);
    auto *t = new QLabel(title);
    t->setObjectName(QStringLiteral("cardTitle"));
    auto *s = new QLabel(text);
    s->setObjectName(QStringLiteral("muted"));
    s->setWordWrap(true);
    col->addWidget(t);
    col->addWidget(s);
    col->addSpacing(8);
    auto *buttons = new QHBoxLayout;
    buttons->addStretch(1);
    auto *cancel = new QPushButton(QStringLiteral("Cancel"));
    auto *ok = new QPushButton(action);
    ok->setObjectName(QStringLiteral("danger"));
    buttons->addWidget(cancel);
    buttons->addWidget(ok);
    col->addLayout(buttons);
    QObject::connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(ok, &QPushButton::clicked, &dialog, &QDialog::accept);
    dialog.setMinimumWidth(380);
    return dialog.exec() == QDialog::Accepted;
}

class PowerPage : public Page
{
public:
    PowerPage()
        : Page(QStringLiteral("Power"), QStringLiteral("Battery, temperature and shutdown"))
    {
        m_battery = new Card;
        auto *bl = new QHBoxLayout;
        bl->setSpacing(18);
        m_batteryBadge = new IconBadge(QStringLiteral("battery_full"), Theme::green, 64);
        bl->addWidget(m_batteryBadge);
        auto *col = new QVBoxLayout;
        col->setSpacing(4);
        m_percent = new QLabel;
        m_percent->setObjectName(QStringLiteral("big"));
        m_state = new QLabel;
        m_state->setObjectName(QStringLiteral("muted"));
        col->addStretch(1);
        col->addWidget(m_percent);
        col->addWidget(m_state);
        col->addStretch(1);
        bl->addLayout(col, 1);
        m_battery->body()->addLayout(bl);
        m_bar = new QProgressBar;
        m_bar->setTextVisible(false);
        m_bar->setRange(0, 100);
        m_battery->body()->addWidget(m_bar);
        content()->addWidget(m_battery);

        m_thermal = new Card;
        m_thermal->addHeader(QStringLiteral("thermostat"), QStringLiteral("Temperature"), QString(), Theme::peach);
        m_temp = new InfoRow(QStringLiteral("Thermal zone"));
        m_fan = new InfoRow(QStringLiteral("Fan"));
        m_thermal->body()->addWidget(m_temp);
        m_thermal->body()->addWidget(m_fan);
        content()->addWidget(m_thermal);

        auto *actions = new Card;
        actions->addHeader(QStringLiteral("power_settings_new"), QStringLiteral("Shut down or restart"),
                           QStringLiteral("Open documents in other apps may be lost"), Theme::red);
        auto *row = new QHBoxLayout;
        row->setSpacing(10);
        auto *restart = new QPushButton(QStringLiteral(" Restart"));
        restart->setIcon(QIcon(tintedIcon(QStringLiteral("restart_alt"), Theme::text, 32)));
        auto *off = new QPushButton(QStringLiteral(" Power off"));
        off->setObjectName(QStringLiteral("danger"));
        off->setIcon(QIcon(tintedIcon(QStringLiteral("power_settings_new"), Theme::window, 32)));
        QObject::connect(restart, &QPushButton::clicked, [this] {
            if (confirm(this, QStringLiteral("Restart now?"), QStringLiteral("The computer restarts right away."), QStringLiteral("Restart")))
                sa_power(true);
        });
        QObject::connect(off, &QPushButton::clicked, [this] {
            if (confirm(this, QStringLiteral("Power off now?"), QStringLiteral("The computer shuts down right away."), QStringLiteral("Power off")))
                sa_power(false);
        });
        row->addWidget(restart);
        row->addWidget(off);
        row->addStretch(1);
        actions->body()->addSpacing(4);
        actions->body()->addLayout(row);
        content()->addWidget(actions);
        content()->addStretch(1);
    }

    void refresh() override
    {
        sa_battery b;
        bool have = sa_battery(&b);
        m_battery->setVisible(have);
        if (have) {
            m_percent->setText(QStringLiteral("%1%").arg(b.percent));
            QString state = b.charging ? QStringLiteral("Charging") : QStringLiteral("On battery");
            if (!b.charging && b.minutes)
                state += QStringLiteral("  -  about %1h %2m left").arg(b.minutes / 60).arg(b.minutes % 60);
            m_state->setText(state);
            m_bar->setValue(int(b.percent));
            QColor c = b.charging ? Theme::green : b.percent <= 15 ? Theme::red : Theme::accent;
            m_batteryBadge->setIcon(b.charging ? QStringLiteral("battery_charging_full") : QStringLiteral("battery_full"), c);
        }
        sa_thermal t;
        bool hot = sa_thermal(&t);
        m_thermal->setVisible(hot);
        if (hot) {
            m_temp->setValue(QStringLiteral("%1  %2.%3 C").arg(QString::fromUtf8(t.zone)).arg(t.deci_c / 10).arg(qAbs(t.deci_c % 10)));
            m_fan->setValue(t.fan_rpm ? QStringLiteral("%1 rpm").arg(t.fan_rpm) : QStringLiteral("not reported"));
        }
    }

private:
    Card *m_battery, *m_thermal;
    IconBadge *m_batteryBadge;
    QLabel *m_percent, *m_state;
    QProgressBar *m_bar;
    InfoRow *m_temp, *m_fan;
};

Page *makePowerPage()
{
    return new PowerPage;
}
