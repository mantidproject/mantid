#include "MantidQtRefDetectorViewer/RefMatrixWSImageView.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidQtRefDetectorViewer/RefIVConnections.h"
#include "MantidQtSpectrumViewer/ArrayDataSource.h"

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
RefMatrixWSImageView::RefMatrixWSImageView(MatrixWorkspace_sptr /*mat_ws*/)
    : m_imageView(NULL) {
  return;
  //  RefMatrixWSDataSource* source = new RefMatrixWSDataSource( mat_ws );
  //  image_view = new RefImageView( source );  // this is the QMainWindow
  //                                         // for the viewer.  It is
  //                                         // deleted when the window
  //                                         // is closed
}

RefMatrixWSImageView::RefMatrixWSImageView(QString wpsName, int peakMin,
                                           int peakMax, int backMin,
                                           int backMax, int tofMin,
                                           int tofMax) {
  IEventWorkspace_sptr ws;
  ws = AnalysisDataService::Instance().retrieveWS<IEventWorkspace>(
      wpsName.toStdString());

  const double totalYMin = 0.0;
  const size_t totalYMax = 255; // 303
  const size_t totalRows = 256; // 304

  const auto &xAxis = ws->x(0);
  const size_t sz = xAxis.size() - 1;
  const size_t totalCols = sz;

  double totalXMin = xAxis[0];
  double totalXMax = xAxis[sz];

  std::vector<float> data(totalYMax * sz);

  for (size_t px = 0; px < totalYMax; ++px) {
    // Retrieve data now
    const auto &yAxis = ws->y(px);
    for (size_t tof = 0; tof < sz; tof++)
      data[px * sz + tof] = static_cast<float>(yAxis[tof]);
  }

  auto source = boost::make_shared<SpectrumView::ArrayDataSource>(
      totalXMin, totalXMax, totalYMin, static_cast<double>(totalYMax),
      totalRows, totalCols, data);

  m_imageView = new RefImageView(source, peakMin, peakMax, backMin, backMax,
                                 tofMin, tofMax);
}

RefIVConnections *RefMatrixWSImageView::getConnections() {
  return m_imageView->getIVConnections();
}

RefMatrixWSImageView::~RefMatrixWSImageView() {
  // nothing to do here, since m_imageView is deleted when the window closes
}
