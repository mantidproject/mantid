#include <iostream>
#include <sstream>

#include <QtGui>
#include <QVector>
#include <QString>
#include <qimage.h>

#include "MantidQtImageViewer/ImageDisplay.h"
#include "MantidQtImageViewer/ImageDataSource.h"
#include "MantidQtImageViewer/DataArray.h"
#include "MantidQtImageViewer/ColorMaps.h"
#include "MantidQtImageViewer/QtUtils.h"
#include "MantidQtImageViewer/IVUtils.h"

namespace MantidQt
{
namespace ImageView
{

/**
 * Make an ImageDisplay to display with the given widgets and controls.
 *
 * @param image_plot      The QwtPlot that will hold the image
 * @param slider_handler  The object that manages interaction with the
 *                        horizontal and vertical scroll bars
 * @param h_graph         The GraphDisplay for the graph showing horizontal
 *                        cuts through the image at the bottom of the image.
 * @param v_graph         The GraphDisplay for the graph showing vertical 
 *                        cuts through the image at the left side of the image.
 * @param table_widget    The widget where the information about a pointed
 *                        at location will be displayed.
 */
ImageDisplay::ImageDisplay(  QwtPlot*       image_plot,
                             SliderHandler* slider_handler,
                             RangeHandler*  range_handler,
                             GraphDisplay*  h_graph,
                             GraphDisplay*  v_graph,
                             QTableWidget*  table_widget )
{
  ColorMaps::getColorMap( ColorMaps::HEAT,
                          256,
                          color_table );

  this->image_plot     = image_plot;
  this->slider_handler = slider_handler;
  this->range_handler  = range_handler;

  image_plot_item = new ImagePlotItem;
  image_plot_item->setXAxis( QwtPlot::xBottom );
  image_plot_item->setYAxis( QwtPlot::yLeft );

  image_plot_item->attach( image_plot ); 

  h_graph_display  = h_graph;
  v_graph_display  = v_graph;
  image_table      = table_widget;

  data_source     = 0;

  double DEFAULT_INTENSITY = 30;
  SetIntensity( DEFAULT_INTENSITY );
}


ImageDisplay::~ImageDisplay()
{
  // std::cout << "ImageDisplay destructor called" << std::endl;
  delete image_plot_item;
}


/**
 * Set the data source from which the image and data table information will
 * be obtained.
 *
 * @param data_source The ImageDataSource that provides the array of values
 *                    and information for the table.
 */
void ImageDisplay::SetDataSource( ImageDataSource* data_source )
{
  this->data_source = data_source;
  h_graph_display->SetDataSource( data_source );
  v_graph_display->SetDataSource( data_source );

  total_y_min = data_source->GetYMin();
  total_y_max = data_source->GetYMax();

  total_x_min = data_source->GetXMin();
  total_x_max = data_source->GetXMax();
  
  int    n_rows = 500;         // get reasonable size initial image data
  int    n_cols = 500;     
                               // data_array is deleted in the ImagePlotItem
  data_array = data_source->GetDataArray( total_x_min, total_x_max,
                                          total_y_min, total_y_max,
                                          n_rows, n_cols,
                                          false );

  image_plot->setAxisScale( QwtPlot::xBottom, data_array->GetXMin(),
                                              data_array->GetXMax() );
  image_plot->setAxisScale( QwtPlot::yLeft, data_array->GetYMin(),
                                            data_array->GetYMax() );

  image_plot_item->SetData( data_array, &color_table );

  range_handler->ConfigureRangeControls( data_source );

  QRect draw_area;
  GetDisplayRectangle( draw_area );
  slider_handler->ConfigureSliders( draw_area, data_source );
}

/**
 *  Rebuild the scrollbars and image due to a change in the range xmin, xmax
 *  or step size.  It should be invoked when the user changes the values in
 *  the xmin, xmax or step controls.  It should not be called directly from
 *  other threads.
 */
void ImageDisplay::UpdateRange()
{
  if ( data_source == 0 )
  {
    return;   // no image data to update
  }

  if ( DataSourceRangeChanged() )
  {
    SetDataSource( data_source );   // re-initialize with the altered source
  }

  double min  = 0;
  double max  = 0;
  double step = 0;
  range_handler->GetRange( min, max, step );

  int n_bins = (int)(( max - min ) / step);   // range controls now determine
                                              // the number of bins
  QRect display_rect;
  GetDisplayRectangle( display_rect );
 
  slider_handler->ConfigureHSlider( n_bins, display_rect.width() );

  UpdateImage();
}

/**
 *  This will rebuild the image from the data source.  It should be invoked
 *  when the scroll bar is moved, the plot area is resize or the color or
 *  intensity tables are changed.  It should not be called directly from
 *  other threads.
 */
void ImageDisplay::UpdateImage()
{
  if ( data_source == 0 )
  {
    return;   // no image data to update
  }

  if ( DataSourceRangeChanged() )
  {
    SetDataSource( data_source );   // re-initialize with the altered source
  }

  QRect display_rect;
  GetDisplayRectangle( display_rect );

  double scale_y_min = data_source->GetYMin();
  double scale_y_max = data_source->GetYMax();

  double scale_x_min  = 0;
  double scale_x_max  = 0;
  double step = 0;
  range_handler->GetRange( scale_x_min, scale_x_max, step );

  int n_rows = (int)data_source->GetNRows();
  int n_cols = (int)( ( scale_x_max - scale_x_min ) / step  );

  if ( slider_handler->VSliderOn() )
  {
    int y_min;
    int y_max;
    slider_handler->GetVSliderInterval( y_min, y_max );

    double new_y_min = 0;
    double new_y_max = 0;

    IVUtils::Interpolate( 0, n_rows, y_min, 
                          scale_y_min, scale_y_max, new_y_min );
    IVUtils::Interpolate( 0, n_rows, y_max, 
                          scale_y_min, scale_y_max, new_y_max );

    scale_y_min = new_y_min;
    scale_y_max = new_y_max;
  }

  if ( slider_handler->HSliderOn() )
  {
    int x_min;
    int x_max;
    slider_handler->GetHSliderInterval( x_min, x_max );

    double new_x_min = 0;
    double new_x_max = 0;

    IVUtils::Interpolate( 0, n_cols, x_min,
                          scale_x_min, scale_x_max, new_x_min );
    IVUtils::Interpolate( 0, n_cols, x_max,
                          scale_x_min, scale_x_max, new_x_max );

    scale_x_min = new_x_min;
    scale_x_max = new_x_max;
  }

  if ( n_rows > display_rect.height() )
  {
    n_rows = display_rect.height();
  }

  if ( n_cols > display_rect.width() )
  {
    n_cols = display_rect.width();
  }
                                         // NOTE: The DataArray is deleted
                                         //       in the ImagePlotItem.
  data_array = data_source->GetDataArray( scale_x_min, scale_x_max, 
                                          scale_y_min, scale_y_max, 
                                          n_rows, n_cols,
                                          false );

  image_plot->setAxisScale( QwtPlot::xBottom, data_array->GetXMin(),
                                              data_array->GetXMax() );

  image_plot->setAxisScale( QwtPlot::yLeft, data_array->GetYMin(),
                                            data_array->GetYMax() );

  image_plot_item->SetData( data_array, &color_table );
  image_plot->replot();
}


/**
 *  Change the color table used to map intensity to color. 
 *
 *  @param new_color_table  The new color table used to map data values
 *                          to a RGB color.  This can have any positive 
 *                          number of values, but will typically have
 *                          256 entries.
 */
void ImageDisplay::SetColorScale( std::vector<QRgb> & new_color_table )
{
  color_table.resize( new_color_table.size() );
  for ( size_t i = 0; i < new_color_table.size(); i++ )
  {
    color_table[i] = new_color_table[i];
  }
  UpdateImage();
}


/**
 *  Change the control parameter (0...100) used to brighten the image.
 *  If the control parameter is 0, the mapping from data values to color
 *  table index will be linear.  As the control parameter is increased 
 *  the mapping becomes more and more non-linear in a way that emphasizes
 *  the lower level values.  This is similar to a log intensity scale.
 *  
 *  @param control_parameter  This is clamped between 0 (linear) and
 *                            100 (most emphasis on low intensity values)
 */
void ImageDisplay::SetIntensity( double control_parameter )
{
  size_t DEFAULT_SIZE = 100000;
  ColorMaps::getIntensityMap( control_parameter, DEFAULT_SIZE, intensity_table);
  image_plot_item->SetIntensityTable( &intensity_table );
  UpdateImage();
}


/**
 * Extract data from horizontal and vertical cuts across the image and
 * show those as graphs in the horizontal and vertical graphs and show
 * information about the specified point.
 *
 * @param point  The point that the user is currently pointing at with 
 *               the mouse.
 */
void ImageDisplay::SetPointedAtPoint( QPoint point )
{
  double x = image_plot->invTransform( QwtPlot::xBottom, point.x() );
  double y = image_plot->invTransform( QwtPlot::yLeft, point.y() );

  float *data   = data_array->GetData();

  size_t n_rows = data_array->GetNRows();
  size_t n_cols = data_array->GetNCols();

  double y_min = data_array->GetYMin();
  double y_max = data_array->GetYMax();

  double x_min = data_array->GetXMin();
  double x_max = data_array->GetXMax();

  if ( x < x_min )                      // restrict x to valid range since
    x = x_min;                          // Qt returns values outside of region
  else if ( x > x_max )
    x = x_max;

  if ( y < y_min )                      // restrict y to valid range since
    y = y_min;                          // Qt returns values outside of region
  else if ( y > y_max )
    y = y_max;

  double relative_y = (y-y_min)/(y_max-y_min);            //  in 0 to 1
  int    row = (int)(relative_y * (double)n_rows);
  if ( row > (int)n_rows - 1 )
  {
    row = (int)n_rows - 1;
  }
  else if ( row < 0 )
  {
    row = 0;
  }

  QVector<double> xData;
  QVector<double> yData;
  double x_val;
  for ( size_t col = 0; col < n_cols; col++ )
  {
    x_val = (double)col/(double)(n_cols-1) * (x_max-x_min) + x_min; 
    xData.push_back( x_val );
    yData.push_back( data[ row * n_cols + col ] );
  }
  h_graph_display->SetData( xData, yData, x, y );

  double relative_x = (x-x_min)/(x_max-x_min);           // in 0 to 1
  int    col = (int)(relative_x * (double)n_cols);
  if ( col > (int)n_cols - 1 )
  {
    col = (int)n_cols - 1;
  }
  else if ( col < 0 )
  {
    col = 0;
  }

  QVector<double> v_xData;
  QVector<double> v_yData;
  double y_val;
  for ( size_t row = 0; row < n_rows; row++ )
  {
    y_val = (double)row/(double)(n_rows-1) * (y_max-y_min) + y_min;
    v_yData.push_back( y_val );
    v_xData.push_back( data[ row * n_cols + col ] );
  }
  v_graph_display->SetData( v_xData, v_yData, x, y );

  ShowInfoList( x, y );
}


/**
 *  Get the information about a pointed at location and show it in the
 *  table. 
 *
 *  @param x  The x coordinate of the pointed at location on the image.
 *  @param y  The y coordinate of the pointed at location on the image.
 */
void ImageDisplay::ShowInfoList( double x, double y )
{
  std::vector<std::string> info_list;
  data_source->GetInfoList( x, y, info_list );
  int n_infos = (int)info_list.size() / 2;

  image_table->setRowCount(n_infos + 1);
  image_table->setColumnCount(2);
  image_table->verticalHeader()->hide();
  image_table->horizontalHeader()->hide();

  int width = 9;
  int prec  = 3;

  double value = data_array->GetValue( x, y );
  QtUtils::SetTableEntry( 0, 0, "Value", image_table );
  QtUtils::SetTableEntry( 0, 1, width, prec, value, image_table );

  for ( int i = 0; i < n_infos; i++ )
  {
    QtUtils::SetTableEntry( i+1, 0, info_list[2*i], image_table );
    QtUtils::SetTableEntry( i+1, 1, info_list[2*i+1], image_table );
  }
}


/**
 *  Get the rectangle currently covered by the image in pixel coordinates.
 *
 *  @param rect  A QRect object that will be filled out with position, width
 *               and height of the pixel region covered by the image.
 */
void ImageDisplay::GetDisplayRectangle( QRect &rect )
{
  QwtScaleMap xMap = image_plot->canvasMap( QwtPlot::xBottom ); 
  QwtScaleMap yMap = image_plot->canvasMap( QwtPlot::yLeft ); 

  double x_min = data_array->GetXMin();
  double x_max = data_array->GetXMax();
  double y_min = data_array->GetYMin();
  double y_max = data_array->GetYMax();

  int pix_x_min = (int)xMap.transform( x_min );
  int pix_x_max = (int)xMap.transform( x_max );
  int pix_y_min = (int)yMap.transform( y_min );
  int pix_y_max = (int)yMap.transform( y_max );

  rect.setLeft  ( pix_x_min );
  rect.setRight ( pix_x_max );
  rect.setBottom( pix_y_min );
  rect.setTop   ( pix_y_max );

  if ( rect.height() <= 1 ||        // must not have been drawn yet, so set
       rect.width()  <= 1  )        // some reasonable default guesses
  {
    rect.setLeft  (   6 );
    rect.setRight ( 440 );
    rect.setBottom( 440 );
    rect.setTop   (   6 );
  }
}


bool ImageDisplay::DataSourceRangeChanged()
{
  if ( total_y_min != data_source->GetYMin() ||
       total_y_max != data_source->GetYMax() ||
       total_x_min != data_source->GetXMin() ||
       total_x_max != data_source->GetXMax() )
  {
    return true;
  }
  else
  {
    return false;
  }
}


} // namespace MantidQt 
} // namespace ImageView 
