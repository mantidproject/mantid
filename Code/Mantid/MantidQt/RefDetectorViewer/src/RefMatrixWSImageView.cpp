#include <iostream>
#include "MantidQtRefDetectorViewer/RefMatrixWSImageView.h"
#include "MantidQtSpectrumViewer/ArrayDataSource.h"
#include "MantidQtRefDetectorViewer/RefIVConnections.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"
#include "MantidAPI/IEventWorkspace.h"

using Mantid::API::MatrixWorkspace_sptr;
using namespace MantidQt;
using namespace RefDetectorViewer;
using Mantid::API::WorkspaceProperty;
using Mantid::API::Algorithm;
using namespace Mantid::Kernel;
using namespace Mantid::API;

/**
 * Construct an ImageView for the specified matrix workspace
 */
RefMatrixWSImageView::RefMatrixWSImageView( MatrixWorkspace_sptr /*mat_ws*/ )
{
  return;
  //  RefMatrixWSDataSource* source = new RefMatrixWSDataSource( mat_ws );
//  image_view = new RefImageView( source );  // this is the QMainWindow
//                                         // for the viewer.  It is
//                                         // deleted when the window
//                                         // is closed
}

RefMatrixWSImageView::RefMatrixWSImageView( QString wpsName,
                                            int peakMin, int peakMax,
                                            int backMin, int backMax,
                                            int tofMin,  int tofMax)
{
  IEventWorkspace_sptr ws;
  ws = AnalysisDataService::Instance().retrieveWS<IEventWorkspace>(wpsName.toStdString());

  const double totalYMin = 0.0;
  const double totalYMax = 255.0; //303
  const size_t totalRows = 256;   //304

  std::vector<double> xAxis = ws->readX(0);
  const size_t sz = xAxis.size() - 1;
  const size_t totalCols = sz;

  double totalXMin = xAxis[0];
  double totalXMax = xAxis[sz];

  float *data = new float[static_cast<size_t>(totalYMax) * sz];

//  std::cout << "Starting the for loop " << std::endl;
//  std::cout << "total_xmax: " << total_xmax << std::endl;
//  std::cout << "sz is : " << sz << std::endl;

  std::vector<double> yAxis;
  for (size_t px = 0; px < totalYMax; px++)
  {
    // Retrievedata now
    yAxis = ws->readY(px);
    for (size_t tof = 0; tof < sz; tof++)
      data[px*sz + tof] = static_cast<float>(yAxis[tof]);
  }

  SpectrumView::ArrayDataSource* source = new SpectrumView::ArrayDataSource(totalXMin, totalXMax,
                                                                            totalYMin, totalYMax,
                                                                            totalRows, totalCols,
                                                                            data);

//    std::cout << "ws->readX(0).size(): " << ws->readX(0).size() << std::endl;

  m_imageView = new RefImageView( source,
                                  peakMin, peakMax,
                                  backMin, backMax,
                                  tofMin,  tofMax);
}


RefIVConnections* RefMatrixWSImageView::getConnections()
{
  return m_imageView->getIVConnections();
}


RefMatrixWSImageView::~RefMatrixWSImageView()
{
  // nothing to do here, since image_view is deleted when the window closes
}
