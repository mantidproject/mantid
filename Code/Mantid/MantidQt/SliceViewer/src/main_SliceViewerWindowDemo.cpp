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
#include "MantidQtSliceViewer/SliceViewerWindow.h"

/** Demo application for quickly testing the SliceViewerWindow GUI.
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
  app.setApplicationName("SliceViewerWindow demo");
  IMDEventWorkspace_sptr mdew = makeDemoData();

  SliceViewerWindow * mainWin = new SliceViewerWindow("mdew", NULL);
  //mainWin->getSlicer()->getLineOverlay()->setSnap(0.5);
//  mainWin->getSlicer()->getLineOverlay()->setSnapLength(0.1);
  mainWin->move(100, 100);
  mainWin->resize(700, 700);
  mainWin->show();

  app.exec();

  mainWin->close();
  delete mainWin;
  return 0;
}

