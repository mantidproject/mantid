
#include "MantidKernel/Logger.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidKernel/PropertyWithValue.h"
#include "qmainwindow.h"
#include <iostream>
#include <QApplication>
#include <QDir>
#include <QFrame>
#include <qlayout.h>
#include <QMessageBox>
#include <QSplashScreen>
#include <QThread>
#include "MantidQtAPI/BoolPropertyWidget.h"

using namespace Mantid::Kernel;
using namespace MantidQt::API;

/** Main application
 *
 * @param argc :: ignored
 * @param argv :: ignored
 * @return return code
 */
int main( int argc, char ** argv )
{
  QApplication app(argc, argv);
  app.setApplicationName("PropertyWidgets demo");
  QMainWindow * mainWin = new QMainWindow();

  QFrame * frame = new QFrame(mainWin);
  mainWin->setCentralWidget(frame);

  QHBoxLayout * layout = new QHBoxLayout(frame);
  frame->setLayout(layout);

  QFrame * frame1 = new QFrame(mainWin);
  frame1->setFrameStyle(QFrame::Box);
  QFrame * frame2 = new QFrame(mainWin);
  frame2->setFrameStyle(QFrame::Box);

  layout->addWidget(frame1);
  layout->addWidget(frame2);

  QVBoxLayout * layout1 = new QVBoxLayout(frame1);
  QGridLayout * grid = new QGridLayout(frame2);

  PropertyWithValue<bool> * boolProp = new PropertyWithValue<bool>("Boolean", true);
  BoolPropertyWidget * boolWidget1 = new BoolPropertyWidget(boolProp, frame1, NULL);
  layout1->addWidget(boolWidget1);
  BoolPropertyWidget * boolWidget2 = new BoolPropertyWidget(boolProp, frame2, grid, 2);

//  SliceViewer * slicer = new SliceViewer(frame);
//  slicer->resize(600,600);
//  layout->addWidget(slicer);
//  slicer->setWorkspace(mdew);

  mainWin->move(100, 100);
  mainWin->resize(700, 700);
  mainWin->show();

  app.exec();

  mainWin->close();
  delete mainWin;
  return 0;
}
