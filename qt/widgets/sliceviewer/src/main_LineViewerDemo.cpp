#include <QApplication>
#include <QDir>
#include <QMessageBox>
#include <QSplashScreen>
#include <QThread>

#include "MantidKernel/Logger.h"
#include "MantidKernel/MantidVersion.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/VMD.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidQtSliceViewer/LineViewer.h"
#include "qmainwindow.h"

using namespace Mantid;
using namespace Mantid::API;
;
using namespace Mantid::MDEvents;
using namespace Mantid::Geometry;
using Mantid::Geometry::MDHistoDimension;
using Mantid::Geometry::MDHistoDimension_sptr;
using Mantid::Kernel::VMD;
using MantidQt::SliceViewer::LineViewer;

/** Demo application for quickly testing the LineViewer GUI.
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
int main(int argc, char **argv) {
  QApplication app(argc, argv);
  app.setOrganizationName("JanikTech");
  app.setApplicationName("Application Example");
  QMainWindow *mainWin = new QMainWindow();

  IMDWorkspace_sptr mdew = makeDemoData();

  QFrame *frame = new QFrame(mainWin);
  mainWin->setCentralWidget(frame);

  QLayout *layout = new QVBoxLayout(frame);
  frame->setLayout(layout);

  LineViewer *line = new LineViewer(frame);
  line->resize(600, 600);
  layout->addWidget(line);
  line->setWorkspace(mdew);
  line->setStart(VMD(-1, 0, 0));
  line->setEnd(VMD(+1, 0, 0));
  line->setWidth(VMD(+0.3, 0, 0.3));
  line->setPlanarWidth(0.2);
  line->setNumBins(1000);
  line->setFreeDimensions(false, 0, 1);
  line->showPreview();
  mainWin->move(100, 100);
  mainWin->resize(700, 700);
  mainWin->show();

  app.exec();

  mainWin->close();
  delete mainWin;
  return 0;
}
