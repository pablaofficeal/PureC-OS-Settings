#pragma once

#include <QtCore/qlist.h>
#include <QtCore/qtimer.h>
#include <QtWidgets/qstackedwidget.h>
#include <QtWidgets/qwidget.h>

class Page;
class QPushButton;

class MainWindow : public QWidget
{
public:
    MainWindow();
    void showToast(const QString &text, const QColor &color);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QWidget *buildSidebar();
    void addSection(class QVBoxLayout *layout, const QString &title);
    void addPage(class QVBoxLayout *layout, const QString &icon, const QString &title, Page *page);
    void select(int index);
    void placeToast();

    QStackedWidget *m_stack = nullptr;
    QList<QPushButton *> m_nav;
    QList<Page *> m_pages;
    QTimer m_timer;
    QTimer m_toastTimer;
    class QFrame *m_toast = nullptr;
    class QLabel *m_toastText = nullptr;
};
