#include <iostream>
#include <QApplication>
#include <QSplashScreen>
#include <QMessageBox>
#include <QDir>
#include <QThread>

#include "MantidKernel/Logger.h"
#include "MantidKernel/MantidVersion.h"

#include "qmainwindow.h"
#include "MantidQtSliceViewer/SliceViewer.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"

using namespace Mantid;
using namespace Mantid::API;;
using namespace Mantid::MDEvents;
using namespace Mantid::Geometry;
using Mantid::Geometry::MDHistoDimension_sptr;
using Mantid::Geometry::MDHistoDimension;
using MantidQt::SliceViewer::SliceViewer;

/** Demo application for quickly testing the SliceViewer GUI.
 *
 * @author Janik Zikovsky
 * @date Oct 3, 2011
 */

// Little hack to avoid repeating code
#include "main_common.cpp"


/** Main application
 *
 * @param argc :: ignored
 * @param argv :: ignored
 * @return return code
 */
int main( int argc, char ** argv )
{
  QApplication app(argc, argv);
  app.setApplicationName("SliceViewer widget demo");
  QMainWindow * mainWin = new QMainWindow();

  IMDWorkspace_sptr mdew = makeDemoData(false);

  QFrame * frame = new QFrame(mainWin);
  mainWin->setCentralWidget(frame);

  QLayout * layout = new QVBoxLayout(frame);
  frame->setLayout(layout);

  SliceViewer * slicer = new SliceViewer(frame);
  slicer->resize(600,600);
  layout->addWidget(slicer);
  slicer->setWorkspace(mdew);
  mainWin->move(100, 100);
  mainWin->resize(700, 700);
  mainWin->show();

  app.exec();

  mainWin->close();
  delete mainWin;
  return 0;
}
