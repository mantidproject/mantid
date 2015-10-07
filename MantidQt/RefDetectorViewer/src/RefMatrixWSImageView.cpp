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
RefMatrixWSImageView::RefMatrixWSImageView( MatrixWorkspace_sptr /*mat_ws*/ ) :
  m_imageView(NULL)
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

  std::vector<float> data(static_cast<size_t>(totalYMax) * sz);

  std::vector<double> yAxis;
  for (size_t px = 0; px < totalYMax; px++)
  {
    // Retrieve data now
    yAxis = ws->readY(px);
    for (size_t tof = 0; tof < sz; tof++)
      data[px * sz + tof] = static_cast<float>(yAxis[tof]);
  }

  SpectrumView::ArrayDataSource_sptr source =
    SpectrumView::ArrayDataSource_sptr( new SpectrumView::ArrayDataSource(totalXMin, totalXMax,
                                                                          totalYMin, totalYMax,
                                                                          totalRows, totalCols,
                                                                          data) );

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
