#include <iostream>
#include <sstream>
#include <cfloat>

#include <QtGui>
#include <QVector>
#include <QString>
#include <qimage.h>
#include <qwt_scale_engine.h>

#include "MantidQtSpectrumViewer/SpectrumDisplay.h"
#include "MantidQtSpectrumViewer/SpectrumDataSource.h"
#include "MantidQtSpectrumViewer/DataArray.h"
#include "MantidQtSpectrumViewer/ColorMaps.h"
#include "MantidQtSpectrumViewer/QtUtils.h"
#include "MantidQtSpectrumViewer/SVUtils.h"
#include "MantidQtSpectrumViewer/SliderHandler.h"

namespace MantidQt
{
namespace SpectrumView
{

/**
 * Make an SpectrumDisplay to display with the given widgets and controls.
 *
 * @param spectrumPlot   The QwtPlot that will hold the image
 * @param sliderHandler  The object that manages interaction with the
 *                       horizontal and vertical scroll bars
 * @param rangeHander  The object that manages interaction with the range.
 * @param hGraph        The GraphDisplay for the graph showing horizontal
 *                       cuts through the image at the bottom of the image.
 * @param vGraph        The GraphDisplay for the graph showing vertical
 *                       cuts through the image at the left side of the image.
 * @param tableWidget   The widget where the information about a pointed
 *                       at location will be displayed.
 */
SpectrumDisplay::SpectrumDisplay(  QwtPlot*         spectrumPlot,
                                   ISliderHandler*  sliderHandler,
                                   IRangeHandler*   rangeHander,
                                   GraphDisplay*    hGraph,
                                   GraphDisplay*    vGraph,
                                   QTableWidget*    tableWidget ) :
  m_spectrumPlot(spectrumPlot),
  m_sliderHandler(sliderHandler),
  m_rangeHandler(rangeHander),
  m_hGraphDisplay(hGraph),
  m_vGraphDisplay(vGraph),
  m_pointedAtX(0.0), m_pointedAtY(0.0),
  m_imageTable(tableWidget),
  m_totalXMin(0.0), m_totalXMax(0.0),
  m_totalYMin(0.0), m_totalYMax(0.0)
{
  ColorMaps::GetColorMap( ColorMaps::HEAT,
                          256,
                          m_positiveColorTable );
  ColorMaps::GetColorMap( ColorMaps::GRAY,
                          256,
                          m_negativeColorTable );

  m_spectrumPlotItem = new SpectrumPlotItem;
  setupSpectrumPlotItem();
}


SpectrumDisplay::~SpectrumDisplay()
{
  delete m_spectrumPlotItem;
}


bool SpectrumDisplay::hasData(const std::string& wsName,
                              const boost::shared_ptr<Mantid::API::Workspace> ws)
{
  return m_dataSource->hasData(wsName, ws);
}


/// Set some properties of the SpectrumPlotItem object
void SpectrumDisplay::setupSpectrumPlotItem()
{
  m_spectrumPlotItem->setXAxis( QwtPlot::xBottom );
  m_spectrumPlotItem->setYAxis( QwtPlot::yLeft );

  m_spectrumPlotItem->attach( m_spectrumPlot );

  double DEFAULT_INTENSITY = 30;
  setIntensity( DEFAULT_INTENSITY );
}


/**
 * Set the data source from which the image and data table information will
 * be obtained.
 *
 * @param dataSource The SpectrumDataSource that provides the array of values
 *                    and information for the table.
 */
void SpectrumDisplay::setDataSource( SpectrumDataSource_sptr dataSource )
{
  m_dataSource = dataSource;

  m_hGraphDisplay->setDataSource( m_dataSource );
  m_vGraphDisplay->setDataSource( m_dataSource );

  m_totalYMin = m_dataSource->getYMin();
  m_totalYMax = m_dataSource->getYMax();

  m_totalXMin = m_dataSource->getXMin();
  m_totalXMax = m_dataSource->getXMax();

  m_pointedAtX = DBL_MAX;
  m_pointedAtY = DBL_MAX;

  int n_rows = static_cast<int>(m_totalYMax - m_totalYMin); // get reasonable size initial image data
  int n_cols = 500;

  // m_dataArray is deleted in the SpectrumPlotItem
  m_dataArray = m_dataSource->getDataArray( m_totalXMin, m_totalXMax,
                                            m_totalYMin, m_totalYMax,
                                            n_rows, n_cols,
                                            false );

  m_spectrumPlot->setAxisScale( QwtPlot::xBottom, m_dataArray->getXMin(), m_dataArray->getXMax() );
  m_spectrumPlot->setAxisScale( QwtPlot::yLeft,   m_dataArray->getYMin(), m_dataArray->getYMax() );

  m_spectrumPlotItem->setData( m_dataArray,
                               &m_positiveColorTable,
                               &m_negativeColorTable );

  m_rangeHandler->configureRangeControls( m_dataSource );

  QRect drawArea;
  getDisplayRectangle( drawArea );
  m_sliderHandler->configureSliders( drawArea, m_dataSource );
}


/**
 *  Rebuild the scrollbars and image due to a change in the range xmin, xmax
 *  or step size.  It should be invoked when the user changes the values in
 *  the xmin, xmax or step controls.  It should not be called directly from
 *  other threads.
 */
void SpectrumDisplay::updateRange()
{
  if ( m_dataSource == 0 )
    return;   // No image data to update

  if ( dataSourceRangeChanged() )
    setDataSource( m_dataSource );   // Re-initialize with the altered source

  QRect displayRect;
  getDisplayRectangle( displayRect );
  // Range controls now determine the number of bins

  double min  = m_totalXMin;
  double max  = m_totalXMax;
  double step = (m_totalXMax - m_totalXMin) / 2000;
  m_rangeHandler->getRange( min, max, step );

  int numBins = SVUtils::NumSteps( min, max, step );
  if ( numBins == 0 )
    return;

  m_sliderHandler->configureHSlider( numBins, displayRect.width() );

  updateImage();
}


/**
 * Updates the rnages of the scroll bars when the window is resized.
 */
void SpectrumDisplay::handleResize()
{
  QRect draw_area;
  getDisplayRectangle( draw_area );

  // Notify the sliders of the resize
  SliderHandler * sliderHandler = dynamic_cast<SliderHandler*>(m_sliderHandler);
  if(sliderHandler)
    sliderHandler->reConfigureSliders(draw_area, m_dataSource);
}


/**
 *  This will rebuild the image from the data source.  It should be invoked
 *  when the scroll bar is moved, the plot area is resize or the color or
 *  intensity tables are changed.  It should not be called directly from
 *  other threads.
 */
void SpectrumDisplay::updateImage()
{
  if ( m_dataSource == 0 )
  {
    return;   // no image data to update
  }

  if ( dataSourceRangeChanged() )
  {
    setDataSource( m_dataSource );   // re-initialize with the altered source
  }

  QRect display_rect;
  getDisplayRectangle( display_rect );

  double scale_y_min = m_dataSource->getYMin();
  double scale_y_max = m_dataSource->getYMax();

  double scale_x_min  = m_totalXMin;
  double scale_x_max  = m_totalXMax;
  double x_step = (m_totalXMax - m_totalXMin)/2000;

  m_rangeHandler->getRange( scale_x_min, scale_x_max, x_step );

  int n_rows = (int)m_dataSource->getNRows();
  int n_cols = SVUtils::NumSteps( scale_x_min, scale_x_max, x_step );

                                     // This works for linear or log scales
  if ( n_rows == 0 || n_cols == 0 )
  {
    return;                          // can't draw empty image
  }

  if ( m_sliderHandler->vSliderOn() )
  {
    int y_min;
    int y_max;
    m_sliderHandler->getVSliderInterval( y_min, y_max );

    double new_y_min = 0;
    double new_y_max = 0;

    SVUtils::Interpolate( 0, n_rows, y_min,
                          scale_y_min, scale_y_max, new_y_min );
    SVUtils::Interpolate( 0, n_rows, y_max,
                          scale_y_min, scale_y_max, new_y_max );

    scale_y_min = new_y_min;
    scale_y_max = new_y_max;
  }

  if ( m_sliderHandler->hSliderOn() )
  {
    int x_min;
    int x_max;
    m_sliderHandler->getHSliderInterval( x_min, x_max );
                            // NOTE: The interval [xmin,xmax] is always
                            // found linearly.  For log_x, we need to adjust it

    double new_x_min = 0;
    double new_x_max = 0;

    if ( x_step > 0 )       // linear scale, so interpolate linearly
    {
      SVUtils::Interpolate( 0, n_cols, x_min,
                            scale_x_min, scale_x_max, new_x_min );
      SVUtils::Interpolate( 0, n_cols, x_max,
                            scale_x_min, scale_x_max, new_x_max );
    }
    else                    // log scale, so interpolate "logarithmically"
    {
      SVUtils::LogInterpolate( 0, n_cols, x_min,
                               scale_x_min, scale_x_max, new_x_min );
      SVUtils::LogInterpolate( 0, n_cols, x_max,
                               scale_x_min, scale_x_max, new_x_max );
    }

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

  bool is_log_x = ( x_step < 0 );
                                         // NOTE: The DataArray is deleted
  m_dataArray = m_dataSource->getDataArray( scale_x_min, scale_x_max,
                                            scale_y_min, scale_y_max,
                                            n_rows, n_cols,
                                            is_log_x );

  is_log_x = m_dataArray->isLogX();       // Data source might not be able to
                                         // provide log binned data, so check
                                         // if log binned data was returned.

  m_spectrumPlot->setAxisScale( QwtPlot::xBottom, m_dataArray->getXMin(),
                                                  m_dataArray->getXMax() );

  if ( is_log_x )
  {
    QwtLog10ScaleEngine* log_engine = new QwtLog10ScaleEngine();
    m_spectrumPlot->setAxisScaleEngine( QwtPlot::xBottom, log_engine );
  }
  else
  {
    QwtLinearScaleEngine* linear_engine = new QwtLinearScaleEngine();
    m_spectrumPlot->setAxisScaleEngine( QwtPlot::xBottom, linear_engine );
  }

  m_spectrumPlot->setAxisScale( QwtPlot::yLeft, m_dataArray->getYMin(),
                                                m_dataArray->getYMax() );

  m_spectrumPlotItem->setData( m_dataArray,
                               &m_positiveColorTable,
                               &m_negativeColorTable );
  m_spectrumPlot->replot();

  setVGraph( m_pointedAtX );
  setHGraph( m_pointedAtY );
}


/**
 *  Change the color tables used to map intensity to color. Two tables are
 *  used to allow psuedo-log scaling based on the magnitude of a value.
 *  Typically if the positive color table is colorful, such as the "HEAT"
 *  scale, the negative color table should be a gray scale to easily
 *  distinguish between positive and negative values.
 *
 *  @param positiveColorTable  The new color table used to map positive data
 *                             values to an RGB color.  This can have any
 *                             positive number of values, but will typically
 *                             have 256 entries.
 *  @param negativeColorTable  The new color table used to map negative data
 *                             values to an RGB color.  This must have the
 *                             same number of entries as the positive
 *                             color table.
 */
void SpectrumDisplay::setColorScales( std::vector<QRgb> & positiveColorTable,
                                      std::vector<QRgb> & negativeColorTable )
{
  m_positiveColorTable.resize( positiveColorTable.size() );
  for ( size_t i = 0; i < positiveColorTable.size(); i++ )
    m_positiveColorTable[i] = positiveColorTable[i];

  this->m_negativeColorTable.resize( negativeColorTable.size() );
  for ( size_t i = 0; i < negativeColorTable.size(); i++ )
    m_negativeColorTable[i] = negativeColorTable[i];

  updateImage();
}


/**
 *  Change the control parameter (0...100) used to brighten the image.
 *  If the control parameter is 0, the mapping from data values to color
 *  table index will be linear.  As the control parameter is increased
 *  the mapping becomes more and more non-linear in a way that emphasizes
 *  the lower level values.  This is similar to a log intensity scale.
 *
 *  @param controlParameter  This is clamped between 0 (linear) and
 *                           100 (most emphasis on low intensity values)
 */
void SpectrumDisplay::setIntensity( double controlParameter )
{
  size_t DEFAULT_SIZE = 100000;
  ColorMaps::GetIntensityMap( controlParameter, DEFAULT_SIZE, m_intensityTable);
  m_spectrumPlotItem->setIntensityTable( &m_intensityTable );
  updateImage();
}


QPoint SpectrumDisplay::getPlotTransform( QPair<double, double> values )
{
  double x = m_spectrumPlot->transform( QwtPlot::xBottom, values.first );
  double y = m_spectrumPlot->transform( QwtPlot::yLeft,   values.second );

  return QPoint((int)x, (int)y);
}


QPair<double, double> SpectrumDisplay::getPlotInvTransform( QPoint point )
{
  double x = m_spectrumPlot->invTransform( QwtPlot::xBottom, point.x() );
  double y = m_spectrumPlot->invTransform( QwtPlot::yLeft,   point.y() );

  return qMakePair(x,y);
}


/**
 * Extract data from horizontal and vertical cuts across the image and
 * show those as graphs in the horizontal and vertical graphs and show
 * information about the specified point.
 *
 * @param point  The point that the user is currently pointing at with
 *               the mouse.
 * @param mouseClick Which mouse button was clicked (used by derived class)
 * @return A pair containing the (x,y) values in the graph of the point
 */
QPair<double,double> SpectrumDisplay::setPointedAtPoint( QPoint point, int /*mouseClick*/ )
{
  if ( m_dataSource == 0 || m_dataArray == 0 )
    return qMakePair(0.0, 0.0);

  QPair<double, double> transPoints = getPlotInvTransform(point);

  setHGraph( transPoints.second );
  setVGraph( transPoints.first );

  showInfoList( transPoints.first, transPoints.second );

  return transPoints;
}


/*
 *  Extract data for Horizontal graph from the image at the specified y value.
 *  If the y value is NOT in the y-interval covered by the data array, just
 *  return.
 *
 *  @param y   The y-value of the horizontal cut through the image.
 */
void SpectrumDisplay::setHGraph( double y )
{
  if ( y < m_dataArray->getYMin() || y > m_dataArray->getYMax() )
  {
    m_hGraphDisplay->clear();
    return;
  }

  m_pointedAtY = y;

  std::vector<float>data   = m_dataArray->getData();

  size_t n_cols = m_dataArray->getNCols();

  double x_min = m_dataArray->getXMin();
  double x_max = m_dataArray->getXMax();

  size_t row = m_dataArray->rowOfY( y );

  QVector<double> xData;
  QVector<double> yData;

  xData.push_back( x_min );                              // start at x_min
  yData.push_back( data[ row * n_cols ] );
  for ( size_t col = 0; col < n_cols; col++ )
  {
    double x_val = m_dataArray->xOfColumn( col );
    xData.push_back( x_val );                           // mark data at col
    yData.push_back( data[ row * n_cols + col ] );      // centers
  }
  xData.push_back( x_max );                             // end at x_max
  yData.push_back( data[ row * n_cols + n_cols-1 ] );

  m_hGraphDisplay->setLogX( m_dataArray->isLogX() );
  m_hGraphDisplay->setData( xData, yData, y );
}


/*
 *  Extract data for vertical graph from the image at the specified x value.
 *  If the x value is NOT in the x-interval covered by the data array, just
 *  return.
 *
 *  @param x   The x-value of the vertical cut through the image.
 */
void SpectrumDisplay::setVGraph( double x )
{
  if ( x < m_dataArray->getXMin() || x > m_dataArray->getXMax() )
  {
    m_vGraphDisplay->clear();
    return;
  }

  m_pointedAtX = x;

  std::vector<float> data   = m_dataArray->getData();

  size_t n_rows = m_dataArray->getNRows();
  size_t n_cols = m_dataArray->getNCols();

  double y_min = m_dataArray->getYMin();
  double y_max = m_dataArray->getYMax();

  size_t col = m_dataArray->columnOfX( x );

  QVector<double> v_xData;
  QVector<double> v_yData;


  v_yData.push_back( y_min );                     // start at y_min
  v_xData.push_back( data[col] );
  for ( size_t row = 0; row < n_rows; row++ )     // mark data at row centres
  {
    double y_val = m_dataArray->yOfRow( row );
    v_yData.push_back( y_val );
    v_xData.push_back( data[ row * n_cols + col ] );
  }
  v_yData.push_back( y_max );                     // end at y_max
  v_xData.push_back( data[ (n_rows-1) * n_cols + col] );

  m_vGraphDisplay->setData( v_xData, v_yData, x );
}


/**
 *  Get the information about a pointed at location and show it in the
 *  table.
 *
 *  @param x  The x coordinate of the pointed at location on the image.
 *  @param y  The y coordinate of the pointed at location on the image.
 */
std::vector<std::string> SpectrumDisplay::showInfoList( double x, double y )
{
  std::vector<std::string> info_list;
  m_dataSource->getInfoList( x, y, info_list );
  int n_infos = (int)info_list.size() / 2;

  m_imageTable->setRowCount(n_infos + 1);
  m_imageTable->setColumnCount(2);
  m_imageTable->verticalHeader()->hide();
  m_imageTable->horizontalHeader()->hide();

  int width = 9;
  int prec  = 3;

  double value = m_dataArray->getValue( x, y );
  QtUtils::SetTableEntry( 0, 0, "Value", m_imageTable );
  QtUtils::SetTableEntry( 0, 1, width, prec, value, m_imageTable );

  for ( int i = 0; i < n_infos; i++ )
  {
    QtUtils::SetTableEntry( i+1, 0, info_list[2*i], m_imageTable );
    QtUtils::SetTableEntry( i+1, 1, info_list[2*i+1], m_imageTable );
  }

  m_imageTable->resizeColumnsToContents();

  return info_list;
}


/**
 * Gets the X value being pointed at.
 *
 * @returns X value
 */
double SpectrumDisplay::getPointedAtX()
{
  return m_pointedAtX;
}


/**
 * Gets the Y value being pointed at.
 *
 * @returns Y value
 */
double SpectrumDisplay::getPointedAtY()
{
  return m_pointedAtY;
}


/**
 *  Get the rectangle currently covered by the image in pixel coordinates.
 *
 *  @param rect  A QRect object that will be filled out with position, width
 *               and height of the pixel region covered by the image.
 */
void SpectrumDisplay::getDisplayRectangle( QRect &rect )
{
  QwtScaleMap xMap = m_spectrumPlot->canvasMap( QwtPlot::xBottom );
  QwtScaleMap yMap = m_spectrumPlot->canvasMap( QwtPlot::yLeft );

  double x_min = m_dataArray->getXMin();
  double x_max = m_dataArray->getXMax();
  double y_min = m_dataArray->getYMin();
  double y_max = m_dataArray->getYMax();

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


bool SpectrumDisplay::dataSourceRangeChanged()
{
  return ( m_totalYMin != m_dataSource->getYMin() ||
           m_totalYMax != m_dataSource->getYMax() ||
           m_totalXMin != m_dataSource->getXMin() ||
           m_totalXMax != m_dataSource->getXMax() );
}


} // namespace SpectrumView
} // namespace MantidQt
