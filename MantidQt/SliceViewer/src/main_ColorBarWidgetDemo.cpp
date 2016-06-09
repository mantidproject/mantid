#include "MantidQtMantidWidgets/ColorBarWidget.h"
#include "qmainwindow.h"
#include <QApplication>
#include <QDir>
#include <QMessageBox>
#include <QSplashScreen>
#include <QThread>

/** Demo application for quickly testing the ColorBarWidget GUI.
 *
 * @author Janik Zikovsky
 * @date Oct 3, 2011
 */

using namespace MantidQt::SliceViewer;

/** Main application
 *
 * @param argc :: ignored
 * @param argv :: ignored
 * @return return code
 */
int main(int argc, char **argv) {
  double min = 0;
  double max = 100;

  QApplication app(argc, argv);
  app.setOrganizationName("MantidProject");
  app.setApplicationName("Color Bar Widget Example");
  QMainWindow *mainWin = new QMainWindow();

  QFrame *frame = new QFrame(mainWin);
  mainWin->setCentralWidget(frame);

  QLayout *layout = new QVBoxLayout(frame);
  frame->setLayout(layout);

  MantidQt::MantidWidgets::ColorBarWidget *widget = new ColorBarWidget(frame);

  widget->setViewRange(min, max);
  widget->setLog(false);

  layout->addWidget(widget);
  mainWin->move(100, 100);
  mainWin->resize(40, 500);
  mainWin->show();

  app.exec();

  mainWin->close();
  delete mainWin;
  return 0;
}
