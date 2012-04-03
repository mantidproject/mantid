
#include  "MantidQtImageViewer/ImageView.h"

namespace MantidQt
{
namespace ImageView
{


ImageView::ImageView( ImageDataSource* data_source )
{
  ui     = new Ui_MainWindow();
  window = new QMainWindow();

  ui->setupUi( window );
  window->resize( 1050, 800 );
  window->show();

  slider_handler = new SliderHandler( ui );

  h_graph = new GraphDisplay( ui->h_graphPlot, ui->h_graph_table, false );
  v_graph = new GraphDisplay( ui->v_graphPlot, ui->v_graph_table, true );

  image_display = new ImageDisplay( ui->imagePlot,
                                    slider_handler,
                                    h_graph, v_graph,
                                    ui->image_table );

  iv_connections = new IVConnections( ui, image_display, h_graph, v_graph );

  image_display->SetDataSource( data_source );
}


ImageView::~ImageView()
{
  delete  image_display;
  delete  slider_handler;
  delete  h_graph;
  delete  v_graph;
  delete  iv_connections;
  delete  window;
  delete  ui;
}


} // namespace MantidQt 
} // namespace ImageView 

