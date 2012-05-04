
#include <iostream>
#include <QtGui>
#include <QVector>
#include <QString>

#include "MantidQtImageViewer/GraphDisplay.h"
#include "MantidQtImageViewer/QtUtils.h"
#include "MantidQtImageViewer/IVUtils.h"

namespace MantidQt
{
namespace ImageView
{

/**
 *  Construct a GraphDisplay to display selected graph on the specifed plot 
 *  and to disply information in the specified table.
 *
 *  @param graph_plot    The QwtPlot where the graph will be displayed.
 *  @param graph_table   The QTableWidget where information about a 
 *                       pointed at location will be displayed.
 *  @param is_vertical   Flag indicating whether this graph displays the
 *                       vertical or horizontal cut through the image.
 */
GraphDisplay::GraphDisplay( QwtPlot*      graph_plot, 
                            QTableWidget* graph_table,
                            bool          is_vertical )
{
  this->graph_plot  = graph_plot;
  this->graph_table = graph_table;
  this->data_source = 0;
  
  this->is_vertical = is_vertical;
  image_x = 0;
  image_y = 0;

  if ( is_vertical )
  {
    graph_plot->setAxisMaxMajor( QwtPlot::xBottom, 3 );
  }

  curve = new QwtPlotCurve("Curve 1");
}


GraphDisplay::~GraphDisplay()
{
  // std::cout << "GraphDisplay destructor called" << std::endl;
  delete curve;
}


/**
 * Set the data source from which the table information will be obtained
 * (must be set to allow information to be displayed in the table.)
 *
 * @param data_source The ImageDataSource that provides information for
 *                    the table.
 */
void GraphDisplay::SetDataSource( ImageDataSource* data_source )
{
  this->data_source = data_source;
}


/**
 * Set the actual data that will be displayed on the graph and the 
 * coordinates on the image corresponding to this data.  The image
 * coordinates are needed to determine the point of interest, when the
 * user points at a location on the graph.
 *
 * @param xData    Vector of x coordinates of points to plot
 * @param yData    Vector of y coordinates of points to plot.  This should
 *                 be the same size as the xData vector.
 * @param image_x  X-coordinate of point on image that generated the graph
 *                 data
 * @param image_y  Y-coordinate of point on image that generated the graph
 *                 data
 */
void GraphDisplay::SetData(const QVector<double> & xData, 
                           const QVector<double> & yData,
                                 double            image_x,
                                 double            image_y )
{
  if ( xData.size() == 0 ||          // ignore invalid data vectors
       yData.size() == 0 ||
       xData.size() != yData.size()    )
  {
    return;
  }

  this->image_x = image_x;
  this->image_y = image_y;

  curve->attach(0);                 // detach from any plot, before changing
                                    // the data and attaching
  if ( is_vertical )
  {
    double min_y = yData[0];
    double max_y = yData[yData.size()-1];

    double min_x;
    double max_x;
    IVUtils::FindValidInterval( xData, min_x, max_x );

    graph_plot->setAxisScale( QwtPlot::xBottom, min_x, max_x ); 
    graph_plot->setAxisScale( QwtPlot::yLeft, min_y, max_y );
  }
  else
  {
    double min_x = xData[0];
    double max_x = xData[xData.size()-1];

    double min_y;
    double max_y;
    IVUtils::FindValidInterval( yData, min_y, max_y );
    graph_plot->setAxisScale( QwtPlot::yLeft, min_y, max_y );
    graph_plot->setAxisScale( QwtPlot::xBottom, min_x, max_x );
  }

  curve->setData( xData, yData );
  curve->attach( graph_plot );
  graph_plot->replot(); 
  graph_plot->setAutoReplot(true);
}


/**
 * Show information about the specified point.
 *
 * @param point  The point that the user is currently pointing at with 
 *               the mouse.
 */
void GraphDisplay::SetPointedAtPoint( QPoint point )
{
  if ( data_source == 0 )
  {
    return;
  }
  double x = graph_plot->invTransform( QwtPlot::xBottom, point.x() );
  double y = graph_plot->invTransform( QwtPlot::yLeft, point.y() );

  data_source->RestrictX( x );
  data_source->RestrictY( y );

  ShowInfoList( x, y );
}


/**
 *  Get the information about a pointed at location and show it in the
 *  table.  NOTE: If this is the "horizontal" graph, the relevant coordinates
 *  are x and the image_y that generated the graph.  If this is the "vertical"
 *  graph, the relevant coordinates are y and the image_x that generated 
 *  the graph.
 *
 *  @param x  The x coordinate of the pointed at location on the graph.
 *  @param y  The y coordinate of the pointed at location on the graph.
 */
void GraphDisplay::ShowInfoList( double x, double y )
{
  int n_infos = 0;
  int n_rows  = 1;
  std::vector<std::string> info_list;
  if ( data_source != 0 )
  {
    if ( is_vertical )
    {
      data_source->GetInfoList( image_x, y, info_list );
    }
    else
    {
      data_source->GetInfoList( x, image_y, info_list );
    }
  }
  else
  {
    return;
  }
  n_infos = (int)info_list.size()/2;
  n_rows += n_infos; 

  graph_table->setRowCount(n_rows);
  graph_table->setColumnCount(2);
  graph_table->verticalHeader()->hide();
  graph_table->horizontalHeader()->hide();

  int width = 9;
  int prec  = 3;

  if ( is_vertical )
  {
    QtUtils::SetTableEntry( 0, 0, "Value", graph_table );
    QtUtils::SetTableEntry( 0, 1, width, prec, x, graph_table );
  }
  else
  {
    QtUtils::SetTableEntry( 0, 0, "Value", graph_table );
    QtUtils::SetTableEntry( 0, 1, width, prec, y, graph_table );
  }

  for ( int i = 0; i < n_infos; i++ )
  {
    QtUtils::SetTableEntry( i+1, 0, info_list[2*i], graph_table );
    QtUtils::SetTableEntry( i+1, 1, info_list[2*i+1], graph_table );
  }

}

} // namespace MantidQt 
} // namespace ImageView 
