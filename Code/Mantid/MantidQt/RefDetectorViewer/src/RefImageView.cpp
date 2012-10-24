#include <iostream>
#include  "MantidQtRefDetectorViewer/RefImageView.h"
#include  "MantidQtRefDetectorViewer/ColorMaps.h"

#include "ui_RefImageView.h"
#include "MantidQtRefDetectorViewer/RefIVConnections.h"
#include "MantidQtRefDetectorViewer/RefImageDisplay.h"
#include "MantidQtRefDetectorViewer/SliderHandler.h"
#include "MantidQtRefDetectorViewer/RangeHandler.h"

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
 */
RefImageView::RefImageView( RefImageDataSource* data_source )
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

  SliderHandler* slider_handler = new SliderHandler( ui );
  saved_slider_handler = slider_handler;

  RangeHandler* range_handler = new RangeHandler( ui );
  saved_range_handler = range_handler;

//  h_graph = new GraphDisplay( ui->h_graphPlot, ui->h_graph_table, false );
//  v_graph = new GraphDisplay( ui->v_graphPlot, ui->v_graph_table, true );

  h_graph = new GraphDisplay( ui->h_graphPlot, false );
  v_graph = new GraphDisplay( ui->v_graphPlot, true );

  RefImageDisplay* image_display = new RefImageDisplay( ui->imagePlot,
                                                  slider_handler,
                                                  range_handler,
                                                  h_graph, v_graph,
                                                  ui->image_table,
                                                  ui->radioButton_peakLeft,
                                                  ui->radioButton_peakRight,
                                                  ui->radioButton_backLeft,
                                                  ui->radioButton_backRight,
                                                  ui->radioButton_TOFmin,
                                                  ui->radioButton_TOFmax,
                                                  ui->lineEdit_peakLeft,
                                                  ui->lineEdit_peakRight,
                                                  ui->lineEdit_backLeft,
                                                  ui->lineEdit_backRight,
                                                  ui->lineEdit_TOFmin,
                                                  ui->lineEdit_TOFmax);
  saved_image_display = image_display;

  RefIVConnections * iv_connections = new RefIVConnections( ui, this, 
                                                     image_display, 
                                                     h_graph, v_graph );
    
  saved_iv_connections = iv_connections;

  image_display->SetDataSource( data_source );
}
    
  void* RefImageView::getIVConnections()
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

  SliderHandler* slider_handler = 
                             static_cast<SliderHandler*>(saved_slider_handler);
  delete  slider_handler;

  RangeHandler* range_handler = 
                             static_cast<RangeHandler*>(saved_range_handler);
  delete  range_handler;

  RefIVConnections* iv_connections = 
                             static_cast<RefIVConnections*>(saved_iv_connections);
  delete  iv_connections;

  Ui_RefImageViewer* ui = static_cast<Ui_RefImageViewer*>(saved_ui);
  delete  ui;
}


} // namespace MantidQt 
} // namespace ImageView 

