#include <iostream>
#include  "MantidQtRefDetectorViewer/RefImageView.h"
#include  "MantidQtSpectrumViewer/ColorMaps.h"

#include "ui_RefImageView.h"
#include "MantidQtRefDetectorViewer/RefIVConnections.h"
#include "MantidQtRefDetectorViewer/RefImageDisplay.h"
#include "MantidQtRefDetectorViewer/RefSliderHandler.h"
#include "MantidQtRefDetectorViewer/RefRangeHandler.h"

#include <sstream>
#include <string>

namespace MantidQt
{
namespace RefDetectorViewer
{

/**
 *  Construct an SpectrumView to display data from the specified data source.
 *  The specified SpectrumDataSource must be constructed elsewhere and passed
 *  into this SpectrumView constructor.  Most other components of the SpectrumView
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
RefImageView::RefImageView( SpectrumView::SpectrumDataSource* dataSource,
                            int peakMin, int peakMax,
                            int backMin, int backMax,
                            int tofMin,  int tofMax)
{
  Ui_RefImageViewer* ui = new Ui_RefImageViewer();
  m_ui = ui;

  QMainWindow* window = this;

  ui->setupUi( window );
  window->resize( 1050, 800 );
  window->show();
  window->setAttribute(Qt::WA_DeleteOnClose);  // We just need to close the
                                               // window to trigger the
                                               // destructor and clean up
  window->setWindowTitle(QString::fromUtf8("Reflector Detector Viewer"));

  RefSliderHandler* slider_handler = new RefSliderHandler( ui );
  m_sliderHandler = slider_handler;

  RefRangeHandler* range_handler = new RefRangeHandler( ui );
  m_rangeHandler = range_handler;

  // Create the handler for comminicating peak/background/tof values to/from the ui
  // This ends up being owned by the RefImagePlotItem instance
  RefLimitsHandler* limits_handler = new RefLimitsHandler(ui);

  m_hGraph = new SpectrumView::GraphDisplay( ui->h_graphPlot, NULL, false );
  m_vGraph = new SpectrumView::GraphDisplay( ui->v_graphPlot, NULL, true );


  RefImageDisplay* image_display = new RefImageDisplay( ui->imagePlot,
                                                        slider_handler,
                                                        range_handler,
                                                        limits_handler,
                                                        m_hGraph, m_vGraph,
                                                        ui->image_table);
  m_imageDisplay = image_display;

  RefIVConnections * iv_connections = new RefIVConnections( ui, this,
                                                            image_display,
                                                            m_hGraph, m_vGraph );

  // Set validators on the QLineEdits to restrict them to integers
  ui->lineEdit_peakLeft->setValidator(new QIntValidator(this));
  ui->lineEdit_peakRight->setValidator(new QIntValidator(this));
  ui->lineEdit_backLeft->setValidator(new QIntValidator(this));
  ui->lineEdit_backRight->setValidator(new QIntValidator(this));
  ui->lineEdit_TOFmin->setValidator(new QIntValidator(this));
  ui->lineEdit_TOFmax->setValidator(new QIntValidator(this));

  //populate widgets with peak, back and tof values
  limits_handler->setPeakLeft(peakMin);
  limits_handler->setPeakRight(peakMax);
  limits_handler->setBackLeft(backMin);
  limits_handler->setBackRight(backMax);
  limits_handler->setTOFmin(tofMin);
  limits_handler->setTOFmax(tofMax);

  m_ivConnections = iv_connections;

  image_display->updateImage();
  iv_connections->peak_back_tof_range_update();

  image_display->setDataSource( dataSource );
}


RefImageView::~RefImageView()
{
  delete m_hGraph;
  delete m_vGraph;

  RefImageDisplay* image_display = static_cast<RefImageDisplay*>(m_imageDisplay);
  delete image_display;

  RefSliderHandler* slider_handler = static_cast<RefSliderHandler*>(m_sliderHandler);
  delete slider_handler;

  RefRangeHandler* range_handler = static_cast<RefRangeHandler*>(m_rangeHandler);
  delete range_handler;

  RefIVConnections* iv_connections =  static_cast<RefIVConnections*>(m_ivConnections);
  delete iv_connections;

  Ui_RefImageViewer* ui = static_cast<Ui_RefImageViewer*>(m_ui);
  delete ui;
}


RefIVConnections* RefImageView::getIVConnections()
{
  return m_ivConnections;
}

} // namespace RefDetectorViewer
} // namespace MantidQt
