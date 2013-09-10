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
 *  Construct an ImageView to display data from the specified data source.
 *  The specified ImageDataSource must be constructed elsewhere and passed
 *  into this ImageView constructor.  Most other components of the ImageView
 *  are managed by this class.  That is the graphs, image display and other
 *  parts of the ImageView are constructed here and are deleted when the
 *  ImageView destructor is called.
 *
 *  @param data_source  The source of the data that will be displayed. 
 *  @param peak_min The min peak value
 *  @param peak_max The max peak value
 *  @param back_min The min background value
 *  @param back_max The max background value
 *  @param tof_min The min time of flight value
 *  @param tof_max  The max time of flight value
 */
RefImageView::RefImageView( SpectrumView::ImageDataSource* data_source, int peak_min, int peak_max, int back_min, int back_max, int tof_min, int tof_max)
{
  Ui_RefImageViewer* ui = new Ui_RefImageViewer();
  saved_ui          = ui; 

  QMainWindow* window = this;

  ui->setupUi( window );
  window->resize( 1050, 800 );
  window->show();
  window->setAttribute(Qt::WA_DeleteOnClose);  // We just need to close the
                                               // window to trigger the 
                                               // destructor and clean up
  window->setWindowTitle(QString::fromUtf8("Reflector Detector Viewer"));

  RefSliderHandler* slider_handler = new RefSliderHandler( ui );
  saved_slider_handler = slider_handler;

  RefRangeHandler* range_handler = new RefRangeHandler( ui );
  saved_range_handler = range_handler;

  // Create the handler for comminicating peak/background/tof values to/from the ui
  // This ends up being owned by the RefImagePlotItem instance
  RefLimitsHandler* limits_handler = new RefLimitsHandler(ui);

  h_graph = new SpectrumView::GraphDisplay( ui->h_graphPlot, NULL, false );
  v_graph = new SpectrumView::GraphDisplay( ui->v_graphPlot, NULL, true );


  RefImageDisplay* image_display = new RefImageDisplay( ui->imagePlot,
                                                  slider_handler,
                                                  range_handler,
                                                  limits_handler,
                                                  h_graph, v_graph,
                                                  ui->image_table);
  saved_image_display = image_display;

  RefIVConnections * iv_connections = new RefIVConnections( ui, this, 
                                                     image_display, 
                                                     h_graph, v_graph );
  
  // Set validators on the QLineEdits to restrict them to integers
  ui->lineEdit_peakLeft->setValidator(new QIntValidator(this));
  ui->lineEdit_peakRight->setValidator(new QIntValidator(this));
  ui->lineEdit_backLeft->setValidator(new QIntValidator(this));
  ui->lineEdit_backRight->setValidator(new QIntValidator(this));
  ui->lineEdit_TOFmin->setValidator(new QIntValidator(this));
  ui->lineEdit_TOFmax->setValidator(new QIntValidator(this));

  //populate widgets with peak, back and tof values
  limits_handler->setPeakLeft(peak_min);
  limits_handler->setPeakRight(peak_max);
  limits_handler->setBackLeft(back_min);
  limits_handler->setBackRight(back_max);
  limits_handler->setTOFmin(tof_min);
  limits_handler->setTOFmax(tof_max);
    
  saved_iv_connections = iv_connections;

    image_display->UpdateImage();
    iv_connections->peak_back_tof_range_update();

    
  image_display->SetDataSource( data_source );
}
    
  RefIVConnections* RefImageView::getIVConnections()
  {
    return saved_iv_connections;
  }

RefImageView::~RefImageView()
{
//  std::cout << "ImageView destructor called" << std::endl;

  delete  h_graph;
  delete  v_graph;

  RefImageDisplay* image_display = static_cast<RefImageDisplay*>(saved_image_display);
  delete  image_display;

  RefSliderHandler* slider_handler =
                             static_cast<RefSliderHandler*>(saved_slider_handler);
  delete  slider_handler;

  RefRangeHandler* range_handler =
                             static_cast<RefRangeHandler*>(saved_range_handler);
  delete  range_handler;

  RefIVConnections* iv_connections = 
                             static_cast<RefIVConnections*>(saved_iv_connections);
  delete  iv_connections;

  Ui_RefImageViewer* ui = static_cast<Ui_RefImageViewer*>(saved_ui);
  delete  ui;
}


} // namespace RefDetectorViewer
} // namespace MantidQt 
