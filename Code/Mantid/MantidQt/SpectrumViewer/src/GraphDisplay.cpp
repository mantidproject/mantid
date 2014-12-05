#include <iostream>
#include <QtGui>
#include <QVector>
#include <QString>
#include <qwt_scale_engine.h>

#include "MantidQtSpectrumViewer/GraphDisplay.h"
#include "MantidQtSpectrumViewer/QtUtils.h"
#include "MantidQtSpectrumViewer/SVUtils.h"

namespace MantidQt
{
namespace SpectrumView
{

/**
 *  Construct a GraphDisplay to display selected graph on the specifed plot
 *  and to disply information in the specified table.
 *
 *  @param graphPlot   The QwtPlot where the graph will be displayed.
 *  @param graphTable  The QTableWidget where information about a
 *                     pointed at location will be displayed.
 *                     Can be NULL (e.g. the RefDetectorViewer doesn't use it).
 *  @param isVertical  Flag indicating whether this graph displays the
 *                     vertical or horizontal cut through the image.
 */
GraphDisplay::GraphDisplay( QwtPlot*      graphPlot,
                            QTableWidget* graphTable,
                            bool          isVertical ) :
  m_graphPlot(graphPlot),
  m_curve(new QwtPlotCurve("Curve 1")),
  m_graphTable(graphTable),

  m_isVertical(isVertical),
  m_isLogX(false),
  m_imageX(0.0), m_imageY(0.0),
  m_rangeScale(1.0),
  m_minX(0.0), m_maxX(0.0),
  m_minY(0.0), m_maxY(0.0)
{
  if(isVertical)
    graphPlot->setAxisMaxMajor( QwtPlot::xBottom, 3 );
}


GraphDisplay::~GraphDisplay()
{
  m_curve->attach(0);
  delete m_curve;
}


/**
 * Set the data source from which the table information will be obtained
 * (must be set to allow information to be displayed in the table.)
 *
 * @param dataSource The SpectrumDataSource that provides information for
 *                   the table.
 */
void GraphDisplay::setDataSource( SpectrumDataSource_sptr dataSource )
{
  m_dataSource = dataSource;
}


/**
 * Set flag indicating whether or not to use a log scale on the x-axis
 *
 * @param isLogX  Pass in true to use a log scale on the x-axis and false
 *                to use a linear scale.
 */
void GraphDisplay::setLogX( bool isLogX )
{
  m_isLogX = isLogX;
}


/**
 * Set the actual data that will be displayed on the graph and the
 * coordinates on the image corresponding to this data.  The image
 * coordinates are needed to determine the point of interest, when the
 * user points at a location on the graph.
 *
 * @param xData     Vector of x coordinates of points to plot
 * @param yData     Vector of y coordinates of points to plot.  This should
 *                  be the same size as the xData vector.
 * @param cutValue  the cut value
 */
void GraphDisplay::setData(const QVector<double> & xData,
                           const QVector<double> & yData,
                                 double            cutValue )
{
  if ( xData.size() == 0 ||          // ignore invalid data vectors
       yData.size() == 0 ||
       xData.size() != yData.size() )
  {
    return;
  }

  m_curve->attach(0);               // detach from any plot, before changing
                                    // the data and attaching
  if ( m_isVertical )
  {
    m_imageX = cutValue;
    m_minY = yData[0];
    m_maxY = yData[yData.size()-1];
    SVUtils::FindValidInterval( xData, m_minX, m_maxX );
  }
  else
  {
    m_imageY = cutValue;
    m_minX = xData[0];
    m_maxX = xData[xData.size()-1];
    SVUtils::FindValidInterval( yData, m_minY, m_maxY );

    if ( m_isLogX )                // only set log scale for x if NOT vertical
    {
      QwtLog10ScaleEngine* log_engine = new QwtLog10ScaleEngine();
      m_graphPlot->setAxisScaleEngine( QwtPlot::xBottom, log_engine );
    }
    else
    {
      QwtLinearScaleEngine* linear_engine = new QwtLinearScaleEngine();
      m_graphPlot->setAxisScaleEngine( QwtPlot::xBottom, linear_engine );
    }
  }

  m_curve->setData( xData, yData );
  m_curve->attach( m_graphPlot );

  setRangeScale( m_rangeScale );

  m_graphPlot->setAutoReplot(true);
}


void GraphDisplay::clear()
{
  m_curve->attach(0);                 // detach from plot
  m_graphPlot->replot();
}


/**
 *  Set up axes using the specified scale factor and replot the graph.
 *  This is useful for seeing low-level values, by clipping off the higher
 *  magnitude values.
 *
 *  @param rangeScale Value between 0 and 1 indicating what fraction of
 *         graph value range should be plotted.
 */
void GraphDisplay::setRangeScale( double rangeScale )
{
  m_rangeScale = rangeScale;

  if ( m_isVertical )
  {
    double axis_max = m_rangeScale * ( m_maxX - m_minX ) + m_minX;
    m_graphPlot->setAxisScale( QwtPlot::xBottom, m_minX, axis_max );
    m_graphPlot->setAxisScale( QwtPlot::yLeft, m_minY, m_maxY );
  }
  else
  {
    double axis_max = m_rangeScale * ( m_maxY - m_minY ) + m_minY;
    m_graphPlot->setAxisScale( QwtPlot::yLeft, m_minY, axis_max );
    m_graphPlot->setAxisScale( QwtPlot::xBottom, m_minX, m_maxX );
  }
  m_graphPlot->replot();
}


/**
 * Show information about the specified point.
 *
 * @param point  The point that the user is currently pointing at with
 *               the mouse.
 */
void GraphDisplay::setPointedAtPoint( QPoint point )
{
  if ( m_dataSource == 0 )
  {
    return;
  }
  double x = m_graphPlot->invTransform( QwtPlot::xBottom, point.x() );
  double y = m_graphPlot->invTransform( QwtPlot::yLeft, point.y() );

  if ( m_isVertical )             // x can be anywhere on graph, y must be
  {                              // a valid data source position, vertically
    m_dataSource->restrictY( y );
  }
  else                           // y can be anywhere on graph, x must be
  {                              // a valid data source position, horizontally
    m_dataSource->restrictX( x );
  }

  showInfoList( x, y );
}


/**
 *  Get the information about a pointed at location and show it in the
 *  table.  NOTE: If this is the "horizontal" graph, the relevant coordinates
 *  are x and the m_imageY that generated the graph.  If this is the "vertical"
 *  graph, the relevant coordinates are y and the m_imageX that generated
 *  the graph.
 *  The method is a no-op if the table is not being used (e.g. as in the
 *  case of the RefDetectorViewer).
 *
 *  @param x  The x coordinate of the pointed at location on the graph.
 *  @param y  The y coordinate of the pointed at location on the graph.
 */
void GraphDisplay::showInfoList( double x, double y )
{
  // This whole method is a no-op if no table object was injected on construction
  if ( m_graphTable != NULL )
  {
    int n_infos = 0;
    int n_rows  = 1;
    std::vector<std::string> info_list;
    if ( m_dataSource != 0 )
    {
      if ( m_isVertical )
      {
        m_dataSource->getInfoList( m_imageX, y, info_list );
      }
      else
      {
        m_dataSource->getInfoList( x, m_imageY, info_list );
      }
    }
    else
    {
      return;
    }
    n_infos = (int)info_list.size()/2;
    n_rows += n_infos;

    m_graphTable->setRowCount(n_rows);
    m_graphTable->setColumnCount(2);
    m_graphTable->verticalHeader()->hide();
    m_graphTable->horizontalHeader()->hide();

    int width = 9;
    int prec  = 3;

    if ( m_isVertical )
    {
      QtUtils::SetTableEntry( 0, 0, "Value", m_graphTable );
      QtUtils::SetTableEntry( 0, 1, width, prec, x, m_graphTable );
    }
    else
    {
      QtUtils::SetTableEntry( 0, 0, "Value", m_graphTable );
      QtUtils::SetTableEntry( 0, 1, width, prec, y, m_graphTable );
    }

    for ( int i = 0; i < n_infos; i++ )
    {
      QtUtils::SetTableEntry( i+1, 0, info_list[2*i], m_graphTable );
      QtUtils::SetTableEntry( i+1, 1, info_list[2*i+1], m_graphTable );
    }

    m_graphTable->resizeColumnsToContents();
  }
}

} // namespace SpectrumView
} // namespace MantidQt
