#include <iostream>
#include <QApplication>
#include <QSplashScreen>
#include <QMessageBox>
#include <QDir>
#include <QThread>

#include "MantidKernel/Logger.h"
#include "MantidKernel/MantidVersion.h"

#include "qmainwindow.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidQtSliceViewer/LineViewer.h"

using namespace Mantid;
using namespace Mantid::API;;
using namespace Mantid::MDEvents;
using namespace Mantid::Geometry;
using Mantid::Geometry::MDHistoDimension_sptr;
using Mantid::Geometry::MDHistoDimension;
using MantidQt::SliceViewer::LineViewer;

/** Demo application for quickly testing the LineViewer GUI.
 *
 * @author Janik Zikovsky
 * @date Oct 3, 2011
 */



/** Creates a fake MDHistoWorkspace
 *
 * @param signal :: signal and error squared in every point
 * @param numDims :: number of dimensions to create. They will range from 0 to max
 * @param numBins :: bins in each dimensions
 * @param max :: max position in each dimension
 * @return the MDHisto
 */
Mantid::MDEvents::MDHistoWorkspace_sptr makeFakeMDHistoWorkspace(double signal, size_t numDims, size_t numBins,
    double max)
{
  Mantid::MDEvents::MDHistoWorkspace * ws = NULL;
  if (numDims ==1)
  {
    ws = new Mantid::MDEvents::MDHistoWorkspace(
        MDHistoDimension_sptr(new MDHistoDimension("x","x","m", 0.0, max, numBins)) );
  }
  else if (numDims == 2)
  {
    ws = new Mantid::MDEvents::MDHistoWorkspace(
        MDHistoDimension_sptr(new MDHistoDimension("x","x","m", 0.0, max, numBins)),
        MDHistoDimension_sptr(new MDHistoDimension("y","y","m", 0.0, max, numBins))  );
  }
  else if (numDims == 3)
  {
    ws = new Mantid::MDEvents::MDHistoWorkspace(
        MDHistoDimension_sptr(new MDHistoDimension("x","x","m", 0.0, max, numBins)),
        MDHistoDimension_sptr(new MDHistoDimension("yy","y","furlongs", 0.0, max, numBins)),
        MDHistoDimension_sptr(new MDHistoDimension("energy","z","meV", 0.0, max, numBins))   );
  }
  else if (numDims == 4)
  {
    ws = new Mantid::MDEvents::MDHistoWorkspace(
        MDHistoDimension_sptr(new MDHistoDimension("x","x","m", 0.0, max, numBins)),
        MDHistoDimension_sptr(new MDHistoDimension("y","y","m", 0.0, max, numBins)),
        MDHistoDimension_sptr(new MDHistoDimension("z","z","m", 0.0, max, numBins)),
        MDHistoDimension_sptr(new MDHistoDimension("t","z","m", 0.0, max, numBins))
        );
  }
  Mantid::MDEvents::MDHistoWorkspace_sptr ws_sptr(ws);
  ws_sptr->setTo(signal, signal);
  return ws_sptr;
}



//-------------------------------------------------------------------------------
/** Add a fake "peak"*/
static void addPeak(size_t num, double x, double y, double z, double radius)
{
  std::ostringstream mess;
  mess << num << ", " << x << ", " << y << ", " << z << ", " << radius;
  FrameworkManager::Instance().exec("FakeMDEventData", 6,
      "InputWorkspace", "mdew",
      "PeakParams", mess.str().c_str(),
      "RandomSeed", "1234");
}


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

  // Create a fake workspace
  size_t numBins;

  numBins=100;

  // ---- Start with empty MDEW ----
  FrameworkManager::Instance().exec("CreateMDWorkspace", 16,
      "Dimensions", "3",
      "Extents", "-10,10,-10,10,-10,10",
      "Names", "h,k,l",
      "Units", "lattice,lattice,lattice",
      "SplitInto", "5",
      "SplitThreshold", "500",
      "MaxRecursionDepth", "5",
      "OutputWorkspace", "mdew");
  addPeak(15000,0,0,0, 1);
  addPeak(5000,0,0,0, 0.3);
  addPeak(5000,0,0,0, 0.2);
  addPeak(5000,0,0,0, 0.1);
//  addPeak(12000,0,0,0, 0.03);
  IMDEventWorkspace_sptr mdew = boost::dynamic_pointer_cast<IMDEventWorkspace>( AnalysisDataService::Instance().retrieve("mdew") );
  mdew->splitAllIfNeeded(NULL);


  LineViewer * line = new LineViewer(frame);
  line->resize(600,600);
  layout->addWidget(line);
  //line->setWorkspace(mdew);
  mainWin->move(100, 100);
  mainWin->resize(700, 700);
  mainWin->show();

  app.exec();

  mainWin->close();
  delete mainWin;
  return 0;
}
