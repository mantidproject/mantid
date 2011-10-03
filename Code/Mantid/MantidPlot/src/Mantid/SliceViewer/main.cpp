#include <iostream>
#include <QApplication>
#include <QSplashScreen>
#include <QMessageBox>
#include <QDir>
#include <QThread>

#include "ApplicationWindow.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/MantidVersion.h"

#include "Mantid/MantidApplication.h"
#include "qmainwindow.h"
#include "Mantid/SliceViewer/SliceViewer.h"


/** Demo application for quickly testing the SliceViewer GUI.
 *
 * @author Janik Zikovsky
 * @date Oct 3, 2011
 */


/** Main application
 *
 * @param argc :: ignored
 * @param argv :: ignored
 * @return return code
 */
int main( int argc, char ** argv )
{
  QApplication app(argc, argv);
  app.setOrganizationName("JanikTech");
  app.setApplicationName("Application Example");
  QMainWindow * mainWin = new QMainWindow();

  QFrame * frame = new QFrame(mainWin);
  mainWin->setCentralWidget(frame);

  QLayout * layout = new QVBoxLayout(frame);
  frame->setLayout(layout);

  SliceViewer * slicer = new SliceViewer(frame);
  slicer->resize(600,600);
  layout->addWidget(slicer);

  mainWin->move(100,100);
  mainWin->resize(700, 700);
  mainWin->show();
  return app.exec();

}
