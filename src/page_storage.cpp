#include "pages.h"
#include "sysapi.h"

#include <QtWidgets/qprogressbar.h>

static QString transportText(int transport)
{
    switch (transport) {
    case SA_TRANSPORT_ATA: return QStringLiteral("ATA");
    case SA_TRANSPORT_AHCI: return QStringLiteral("SATA (AHCI)");
    case SA_TRANSPORT_USB:
    case SA_TRANSPORT_USB_EHCI: return QStringLiteral("USB");
    default: return QStringLiteral("Disk");
    }
}

class StoragePage : public Page
{
public:
    StoragePage()
        : Page(QStringLiteral("Storage"), QStringLiteral("Disks, file system and free space"))
    {
        auto *sys = new Card;
        auto *top = new QHBoxLayout;
        top->setSpacing(18);
        top->addWidget(new IconBadge(QStringLiteral("hard_drive"), Theme::accent, 64));
        auto *col = new QVBoxLayout;
        col->setSpacing(2);
        m_sysTitle = new QLabel;
        m_sysTitle->setStyleSheet(QStringLiteral("font-size: 17px; font-weight: bold;"));
        m_sysDetail = new QLabel;
        m_sysDetail->setObjectName(QStringLiteral("muted"));
        col->addStretch(1);
        col->addWidget(m_sysTitle);
        col->addWidget(m_sysDetail);
        col->addStretch(1);
        top->addLayout(col, 1);
        sys->body()->addLayout(top);
        m_bar = new QProgressBar;
        m_bar->setTextVisible(false);
        m_bar->setRange(0, 1000);
        sys->body()->addSpacing(6);
        sys->body()->addWidget(m_bar);
        auto *nums = new QHBoxLayout;
        m_used = new QLabel;
        m_used->setStyleSheet(QStringLiteral("font-weight: bold;"));
        m_free = new QLabel;
        m_free->setObjectName(QStringLiteral("muted"));
        nums->addWidget(m_used);
        nums->addStretch(1);
        nums->addWidget(m_free);
        sys->body()->addLayout(nums);
        m_fsMeta = new QLabel;
        m_fsMeta->setObjectName(QStringLiteral("muted"));
        sys->body()->addWidget(m_fsMeta);
        content()->addWidget(sys);

        auto *disks = new Card;
        auto *head = disks->addHeader(QStringLiteral("hard_drive"), QStringLiteral("Disks"), QStringLiteral("Looking for devices"));
        m_count = qobject_cast<QLabel *>(head->itemAt(1)->layout()->itemAt(1)->widget());
        m_disks = new QVBoxLayout;
        m_disks->setSpacing(2);
        disks->body()->addLayout(m_disks);
        content()->addWidget(disks);

        auto *note = new Card;
        auto *nl = new QHBoxLayout;
        nl->setSpacing(14);
        nl->addWidget(new IconBadge(QStringLiteral("verified_user"), Theme::green, 34));
        auto *texts = new QVBoxLayout;
        texts->setSpacing(1);
        auto *t = new QLabel(QStringLiteral("Partitioning and formatting live in the terminal"));
        t->setStyleSheet(QStringLiteral("font-weight: bold;"));
        auto *s = new QLabel(QStringLiteral("fdisk and mkfs always ask for a separate confirmation before changing a disk."));
        s->setObjectName(QStringLiteral("muted"));
        texts->addWidget(t);
        texts->addWidget(s);
        nl->addLayout(texts, 1);
        note->body()->addLayout(nl);
        content()->addWidget(note);
        content()->addStretch(1);
    }

    void refresh() override
    {
        sa_fs fs;
        bool have = sa_fs_info(&fs);
        QString root = QString::fromUtf8(fs.root);
        QString shortRoot = root.section(QLatin1Char('/'), -1);
        m_sysTitle->setText(QStringLiteral("System drive (%1)").arg(shortRoot.isEmpty() ? QStringLiteral("unknown") : shortRoot));
        m_sysDetail->setText(QStringLiteral("%1 file system, mounted at /").arg(QString::fromUtf8(fs.type).isEmpty() ? QStringLiteral("ext2") : QString::fromUtf8(fs.type)));
        if (have && fs.total_bytes) {
            quint64 used = fs.total_bytes - fs.free_bytes;
            m_bar->setValue(int(used * 1000 / fs.total_bytes));
            m_used->setText(QStringLiteral("%1 used").arg(humanBytes(used)));
            m_free->setText(QStringLiteral("%1 free of %2").arg(humanBytes(fs.free_bytes), humanBytes(fs.total_bytes)));
            m_fsMeta->setText(QStringLiteral("%1 files and folders  -  %2-byte blocks  -  %3 block groups")
                                  .arg(fs.inodes_total - fs.inodes_free).arg(fs.block_size).arg(fs.groups));
        } else {
            m_bar->setValue(0);
            m_used->setText(QStringLiteral("Not an ext2 disk"));
            m_free->clear();
            m_fsMeta->clear();
        }
        sa_disk list[8];
        int n = sa_disks(list, 8);
        if (n < 0)
            n = 0;
        QString signature = shortRoot;
        for (int i = 0; i < n; ++i)
            signature += QStringLiteral("|%1:%2:%3").arg(QString::fromUtf8(list[i].name)).arg(list[i].sectors).arg(list[i].operational);
        if (signature == m_signature)
            return;
        m_signature = signature;
        m_count->setText(QStringLiteral("%1 device%2 connected").arg(n).arg(n == 1 ? "" : "s"));
        while (QLayoutItem *item = m_disks->takeAt(0)) {
            if (QWidget *gone = item->widget()) {
                gone->hide();
                gone->deleteLater();
            }
            delete item;
        }
        for (int i = 0; i < n; ++i) {
            const sa_disk &d = list[i];
            QString name = QString::fromUtf8(d.name);
            bool usb = d.transport == SA_TRANSPORT_USB || d.transport == SA_TRANSPORT_USB_EHCI;
            auto *row = new QWidget;
            auto *rl = new QHBoxLayout(row);
            rl->setContentsMargins(4, 10, 4, 10);
            rl->setSpacing(14);
            rl->addWidget(new IconBadge(usb ? QStringLiteral("usb") : QStringLiteral("hard_drive"), usb ? Theme::teal : Theme::accent, 38),
                          0, Qt::AlignTop);
            auto *texts = new QVBoxLayout;
            texts->setSpacing(3);
            auto *model = new QLabel(QString::fromUtf8(d.model).trimmed().isEmpty() ? name : QString::fromUtf8(d.model).trimmed());
            model->setStyleSheet(QStringLiteral("font-weight: bold;"));
            auto *meta = new QLabel(QStringLiteral("%1  -  %2  -  %3 B sectors").arg(name, transportText(d.transport)).arg(d.sector_size));
            meta->setObjectName(QStringLiteral("muted"));
            auto *chips = new QHBoxLayout;
            chips->setSpacing(6);
            chips->addWidget(new Chip(d.operational ? QStringLiteral("Healthy") : QStringLiteral("Not ready"),
                                      d.operational ? Theme::green : Theme::peach));
            chips->addWidget(new Chip(d.writable ? QStringLiteral("Read / write") : QStringLiteral("Read only"), Theme::teal));
            if (!shortRoot.isEmpty() && (name == shortRoot || shortRoot.startsWith(name)))
                chips->addWidget(new Chip(QStringLiteral("System"), Theme::accent));
            chips->addStretch(1);
            texts->addWidget(model);
            texts->addWidget(meta);
            texts->addLayout(chips);
            rl->addLayout(texts, 1);
            auto *size = new QLabel(humanBytes(d.sectors * d.sector_size));
            size->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: bold;"));
            rl->addWidget(size, 0, Qt::AlignTop);
            m_disks->addWidget(row);
            if (i + 1 < n) {
                auto *line = new QFrame;
                line->setObjectName(QStringLiteral("hairline"));
                m_disks->addWidget(line);
            }
        }
    }

private:
    QLabel *m_sysTitle, *m_sysDetail, *m_used, *m_free, *m_fsMeta, *m_count;
    QProgressBar *m_bar;
    QVBoxLayout *m_disks;
    QString m_signature;
};

Page *makeStoragePage()
{
    return new StoragePage;
}
