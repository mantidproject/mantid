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
//  MDHistoWorkspace_sptr ws = makeFakeMDHistoWorkspace(1.0, 3, numBins, double(numBins)/2.0);
//  for (size_t x=0;x<numBins; x++)
//    for (size_t y=0;y<numBins; y++)
//      for (size_t z=0;z<numBins; z++)
//      {
//        signal_t signal = ( sin(double(x)/5) + sin(double(y)/2) + sin(double(z)/20) ) * 5 + double(x)*0.05 + double(y)*0.05;
//        signal *= 0.1;
//        ws->setSignalAt(x+y*numBins+z*numBins*numBins, signal);
//      }
//
//  MDHistoWorkspace_sptr ws4 = makeFakeMDHistoWorkspace(1.0, 4, 20, 20.0);
//  MDHistoWorkspace_sptr ws2 = makeFakeMDHistoWorkspace(1.0, 2, numBins*3, 10.0);
//  MDHistoWorkspace_sptr ws3 = makeFakeMDHistoWorkspace(1.0, 3, numBins*2, 10.0);

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

  //  numBins=50;
//  double mid = 25;
//  MDHistoWorkspace_sptr ws = makeFakeMDHistoWorkspace(1.0, 4, numBins, double(numBins)/10.0);
//  for (size_t x=0;x<numBins; x++)
//    for (size_t y=0;y<numBins; y++)
//      for (size_t z=0;z<numBins; z++)
//        for (size_t t=0;t<numBins; t++)
//        {
//          signal_t signal = 0;
//          double v;
//          v = (double(x)-mid); signal += 25-sqrt(v*v);
//          v = (double(y)-mid); signal += 25-sqrt(v*v);
//          v = (double(z)-mid); signal += 25-sqrt(v*v);
//          v = (double(t)-mid); signal += 25-sqrt(v*v);
//          signal *= 0.1;
//          ws->setSignalAt(x+y*numBins+z*numBins*numBins+t*numBins*numBins*numBins, signal);
//        }

//  DimensionSliceWidget * widget;
//  widget = new DimensionSliceWidget(mainWin) ;
//  layout->addWidget( widget );
//  widget->setDimension(0, ws->getDimension(0));
//
//  widget = new DimensionSliceWidget(mainWin) ;
//  layout->addWidget( widget );
//  widget->setDimension(1, ws->getDimension(1));

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
