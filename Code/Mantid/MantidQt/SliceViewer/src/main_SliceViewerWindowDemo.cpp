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
#include "MantidAPI/IMDWorkspace.h"

/** Demo application for quickly testing the SliceViewerWindow GUI.
 *
 * @author Janik Zikovsky
 * @date Oct 3, 2011
 */

// Little hack to avoid repeating code
#include "main_common.cpp"

using MantidQt::SliceViewer::SliceViewerWindow;

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
  IMDWorkspace_sptr mdew = makeDemoData(true);

  //SliceViewerWindow * mainWin = new SliceViewerWindow("workspace_2d");
  SliceViewerWindow * mainWin = new SliceViewerWindow("mdew");
  //mainWin->getSlicer()->getLineOverlay()->setSnap(0.5);
//  mainWin->getSlicer()->getLineOverlay()->setSnapLength(0.1);
  mainWin->move(100, 100);
  mainWin->resize(700, 700);
  mainWin->getSlicer()->setXYDim(0,1);
  mainWin->show();

  app.exec();

  mainWin->close();
  return 0;
}

