
#include  "MantidQtImageViewer/ImageView.h"
#include  "MantidQtImageViewer/ColorMaps.h"

namespace MantidQt
{
namespace ImageView
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

                                                 // ####### hack for testing
/*
  std::vector<double>* intensity_table = new std::vector<double>; 
  ColorMaps::getIntensityMap( 30, 100000, *intensity_table );
  image_display->SetIntensityTable( intensity_table );
*/
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

