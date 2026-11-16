#include "ui.h"
#include "window.h"

#include <QtGui/qfontdatabase.h>
#include <QtWidgets/qapplication.h>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Settings"));
    for (const char *name : {"DejaVuSans.ttf", "DejaVuSans-Bold.ttf"}) {
        for (const char *dir : {"/usr/share/settings/fonts/", "/usr/share/fonts/"})
            if (QFontDatabase::addApplicationFont(QString::fromLatin1(dir) + QLatin1String(name)) >= 0)
                break;
    }
    QFont font(QStringLiteral("DejaVu Sans"));
    font.setPixelSize(13);
    app.setFont(font);
    app.setStyleSheet(styleSheet());
    MainWindow window;
    window.show();
    return app.exec();
}
