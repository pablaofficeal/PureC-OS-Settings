#pragma once

#include <QtCore/qlist.h>
#include <QtGui/qcolor.h>
#include <QtGui/qpixmap.h>
#include <QtWidgets/qabstractbutton.h>
#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qframe.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qscrollarea.h>

namespace Theme {
inline const QColor window(0x1E, 0x1E, 0x2E);
inline const QColor sidebar(0x18, 0x18, 0x25);
inline const QColor card(0x26, 0x27, 0x3A);
inline const QColor cardBorder(0x33, 0x34, 0x4A);
inline const QColor raised(0x31, 0x32, 0x44);
inline const QColor text(0xCD, 0xD6, 0xF4);
inline const QColor muted(0x9A, 0xA0, 0xBC);
inline const QColor faint(0x6C, 0x70, 0x86);
inline const QColor accent(0x89, 0xB4, 0xFA);
inline const QColor green(0xA6, 0xE3, 0xA1);
inline const QColor teal(0x94, 0xE2, 0xD5);
inline const QColor red(0xF3, 0x8B, 0xA8);
inline const QColor peach(0xFA, 0xB3, 0x87);
inline const QColor mauve(0xCB, 0xA6, 0xF7);
}

QPixmap tintedIcon(const QString &name, const QColor &color, int size);
QString humanBytes(quint64 bytes);
QString styleSheet();

class IconBadge : public QLabel
{
public:
    IconBadge(const QString &icon, const QColor &color, int size = 36, QWidget *parent = nullptr);
    void setIcon(const QString &icon, const QColor &color);

private:
    int m_size;
};

class Chip : public QLabel
{
public:
    Chip(const QString &text, const QColor &color, QWidget *parent = nullptr);
    void setChip(const QString &text, const QColor &color);
};

class Card : public QFrame
{
public:
    explicit Card(QWidget *parent = nullptr);
    QVBoxLayout *body() const { return m_body; }
    QHBoxLayout *addHeader(const QString &icon, const QString &title, const QString &subtitle,
                           const QColor &color = Theme::accent);

private:
    QVBoxLayout *m_body;
};

class InfoRow : public QWidget
{
public:
    InfoRow(const QString &label, const QString &value = QString(), QWidget *parent = nullptr);
    void setValue(const QString &value);

private:
    QLabel *m_value;
};

class Toggle : public QAbstractButton
{
public:
    explicit Toggle(QWidget *parent = nullptr);
    QSize sizeHint() const override { return QSize(46, 26); }

protected:
    void paintEvent(QPaintEvent *event) override;
};

class Segmented : public QFrame
{
    Q_OBJECT
public:
    Segmented(const QStringList &items, QWidget *parent = nullptr);
    int current() const { return m_current; }
    void setCurrent(int index);

signals:
    void changed(int index);

private:
    QList<QPushButton *> m_buttons;
    int m_current = -1;
};

class Ring : public QWidget
{
public:
    explicit Ring(int size, QWidget *parent = nullptr);
    void setValue(int percent, const QString &label);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_percent = 0;
    QString m_label;
};

class LoadChart : public QWidget
{
public:
    explicit LoadChart(QWidget *parent = nullptr);
    void push(int percent);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QList<int> m_values;
};

class Page : public QScrollArea
{
public:
    Page(const QString &title, const QString &subtitle, QWidget *parent = nullptr);
    QVBoxLayout *content() const { return m_content; }
    virtual void refresh() {}

private:
    QVBoxLayout *m_content;
};

class ListRow : public QPushButton
{
public:
    explicit ListRow(QWidget *parent = nullptr);
    QHBoxLayout *row() const { return m_row; }

private:
    QHBoxLayout *m_row;
};

void toast(const QString &text, const QColor &color = Theme::green);
