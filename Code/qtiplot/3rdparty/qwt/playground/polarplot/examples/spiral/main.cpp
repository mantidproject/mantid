#include <qapplication.h>
#include <qlayout.h>
#include "settingseditor.h"
#include "plot.h"

int main(int argc, char **argv)
{
    QApplication a(argc, argv);

    QWidget mainWindow;
    Plot *plot = new Plot(&mainWindow);
    SettingsEditor *settingsEditor = new SettingsEditor(&mainWindow);
    settingsEditor->showSettings(plot->settings());

    QHBoxLayout *layout = new QHBoxLayout(&mainWindow);
    layout->addWidget(settingsEditor, 0);
    layout->addWidget(plot, 10);

    QObject::connect(settingsEditor, SIGNAL(edited(const PlotSettings&)),
        plot, SLOT(applySettings(const PlotSettings&)) );

#if QT_VERSION < 0x040000
    a.setMainWidget(&mainWindow);
#endif

    mainWindow.resize(800,600);
    mainWindow.show();

    return a.exec();
}

