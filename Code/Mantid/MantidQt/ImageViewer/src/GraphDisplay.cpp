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
  delete curve;
}


void GraphDisplay::SetDataSource( ImageDataSource* data_source )
{
  this->data_source = data_source;
}


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


void GraphDisplay::SetPointedAtPoint( QPoint point )
{
  double x = graph_plot->invTransform( QwtPlot::xBottom, point.x() );
  double y = graph_plot->invTransform( QwtPlot::yLeft, point.y() );
  ShowInfoList( x, y );
}

void GraphDisplay::ShowInfoList( double x, double y )
{
  int n_infos = 0;
  int n_rows  = 2;
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
  n_infos = (int)info_list.size()/2;
  n_rows += n_infos; 

  graph_table->setRowCount(n_rows);
  graph_table->setColumnCount(2);
  graph_table->verticalHeader()->hide();
  graph_table->horizontalHeader()->hide();

  int width = 9;
  int prec  = 3;
  QtUtils::SetTableEntry( 0, 0, "X", graph_table );
  QtUtils::SetTableEntry( 0, 1, width, prec, x, graph_table );

  QtUtils::SetTableEntry( 1, 0, "Y", graph_table );
  QtUtils::SetTableEntry( 1, 1, width, prec, y, graph_table );

  for ( int i = 0; i < n_infos; i++ )
  {
    QtUtils::SetTableEntry( i+2, 0, info_list[2*i], graph_table );
    QtUtils::SetTableEntry( i+2, 1, info_list[2*i+1], graph_table );
  }

}

} // namespace MantidQt 
} // namespace ImageView 
