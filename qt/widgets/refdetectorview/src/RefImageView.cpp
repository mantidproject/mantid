// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/RefDetectorView/RefImageView.h"
#include "MantidQtWidgets/SpectrumViewer/ColorMaps.h"

#include "MantidQtWidgets/RefDetectorView/RefIVConnections.h"
#include "MantidQtWidgets/RefDetectorView/RefImageDisplay.h"
#include "MantidQtWidgets/RefDetectorView/RefRangeHandler.h"
#include "MantidQtWidgets/RefDetectorView/RefSliderHandler.h"
#include "ui_RefImageView.h"

#include <sstream>
#include <string>

namespace MantidQt {
namespace RefDetectorViewer {

/**
 *  Construct an SpectrumView to display data from the specified data source.
 *  The specified SpectrumDataSource must be constructed elsewhere and passed
 *  into this SpectrumView constructor.  Most other components of the
 *SpectrumView
 *  are managed by this class.  That is the graphs, image display and other
 *  parts of the SpectrumView are constructed here and are deleted when the
 *  SpectrumView destructor is called.
 *
 *  @param dataSource  The source of the data that will be displayed.
 *  @param peakMin     The min peak value
 *  @param peakMax     The max peak value
 *  @param backMin     The min background value
 *  @param backMax     The max background value
 *  @param tofMin      The min time of flight value
 *  @param tofMax      The max time of flight value
 */
RefImageView::RefImageView(SpectrumView::SpectrumDataSource_sptr dataSource,
                           int peakMin, int peakMax, int backMin, int backMax,
                           int tofMin, int tofMax)
    : m_ui(new Ui::RefImageViewer()) {
  QMainWindow *window = this;

  m_ui->setupUi(window);
  window->resize(1050, 800);
  window->show();
  window->setAttribute(Qt::WA_DeleteOnClose); // We just need to close the
                                              // window to trigger the
                                              // destructor and clean up
  window->setWindowTitle(QString::fromUtf8("Reflector Detector Viewer"));

  m_sliderHandler = new RefSliderHandler(m_ui);
  m_rangeHandler = new RefRangeHandler(m_ui);

  // Create the handler for comminicating peak/background/tof values to/from the
  // ui
  // This ends up being owned by the RefImagePlotItem instance
  RefLimitsHandler *limits_handler = new RefLimitsHandler(m_ui);

  m_hGraph = new SpectrumView::GraphDisplay(m_ui->h_graphPlot, nullptr, false);
  m_vGraph = new SpectrumView::GraphDisplay(m_ui->v_graphPlot, nullptr, true);

  m_imageDisplay = new RefImageDisplay(m_ui->imagePlot, m_sliderHandler,
                                       m_rangeHandler, limits_handler, m_hGraph,
                                       m_vGraph, m_ui->image_table);

  RefIVConnections *iv_connections =
      new RefIVConnections(m_ui, this, m_imageDisplay, m_hGraph, m_vGraph);

  // Set validators on the QLineEdits to restrict them to integers
  m_ui->lineEdit_peakLeft->setValidator(new QIntValidator(this));
  m_ui->lineEdit_peakRight->setValidator(new QIntValidator(this));
  m_ui->lineEdit_backLeft->setValidator(new QIntValidator(this));
  m_ui->lineEdit_backRight->setValidator(new QIntValidator(this));
  m_ui->lineEdit_TOFmin->setValidator(new QIntValidator(this));
  m_ui->lineEdit_TOFmax->setValidator(new QIntValidator(this));

  // populate widgets with peak, back and tof values
  limits_handler->setPeakLeft(peakMin);
  limits_handler->setPeakRight(peakMax);
  limits_handler->setBackLeft(backMin);
  limits_handler->setBackRight(backMax);
  limits_handler->setTOFmin(tofMin);
  limits_handler->setTOFmax(tofMax);

  m_ivConnections = iv_connections;

  m_imageDisplay->updateImage();
  iv_connections->peakBackTofRangeUpdate();

  m_imageDisplay->setDataSource(dataSource);
}

RefImageView::~RefImageView() {
  delete m_imageDisplay;
  delete m_sliderHandler;
  delete m_rangeHandler;
  delete m_ivConnections;
  delete m_ui;
}

RefIVConnections *RefImageView::getIVConnections() { return m_ivConnections; }

} // namespace RefDetectorViewer
} // namespace MantidQt
