#include <iostream>

#include <QScrollBar>
#include "MantidQtSpectrumViewer/SliderHandler.h"
#include "MantidQtSpectrumViewer/SVUtils.h"

namespace MantidQt
{
namespace SpectrumView
{

/**
 *  Construct a SliderHandler object to manage the image scrollbars from the
 *  specified UI.
 */
SliderHandler::SliderHandler( Ui_SpectrumViewer* svUI )
  : ISliderHandler(), m_svUI(svUI)
{
}


/**
 * Reconfigure the image scrollbars for the specified data and drawing area.
 *
 * Used when the size of the plot area has changed.
 *
 * @param drawArea    Rectangle specifiying the region where the image will
 *                    be drawn
 * @param dataSource  SpectrumDataSource that provides the data to be drawn
 */
void SliderHandler::reConfigureSliders( QRect drawArea,
                                        SpectrumDataSource_sptr dataSource )
{
  QScrollBar* vScroll = m_svUI->imageVerticalScrollBar;

  int oldVValue = vScroll->value();
  int numRows = (int)dataSource->getNRows();
  int step  = vScroll->pageStep();

  configureSlider( vScroll, numRows, drawArea.height(), oldVValue );

  vScroll->setValue(oldVValue + (step / 2));
}


/**
 * Configure the image scrollbars for the specified data and drawing area.
 *
 * @param drawArea    Rectangle specifiying the region where the image will
 *                    be drawn
 * @param dataSource  SpectrumDataSource that provides the data to be drawn
 */
void SliderHandler::configureSliders( QRect drawArea,
                                      SpectrumDataSource_sptr dataSource )
{
  QScrollBar* vScroll = m_svUI->imageVerticalScrollBar;
  int numRows = (int)dataSource->getNRows();
  configureSlider( vScroll, numRows, drawArea.height(), numRows );

  configureHSlider( 2000, drawArea.width() );   // initial default, 2000 bins
}


/**
 *  Public method to configure the horizontal scrollbar to cover the
 *  specified range of data columns, displayed in the specified number of
 *  pixels.
 *
 *  @param numDataSetps  The number of columns in the data that should be
 *                       displayed
 *  @param numPixels      The number of pixels avaliable to show the data
 */
void SliderHandler::configureHSlider( int numDataSetps,
                                      int numPixels )
{
  QScrollBar* hScroll = m_svUI->imageHorizontalScrollBar;
  configureSlider( hScroll, numDataSetps, numPixels, 0 );
}


/**
 *  Configure the specified scrollbar to cover the specified range of data
 *  steps, displayed in the specified number of pixels.
 *
 *  @param scrollBar    The scroll bar that will be configured
 *  @param numDataSetps  The number of bins in the data that should be
 *                       displayed
 *  @param numPixels      The number of pixels avaliable to show the data
 *  @param val           The initial position of the scrollbar, between 0 and
 *                       numDataSetps.
 */
void SliderHandler::configureSlider( QScrollBar* scrollBar,
                                     int         numDataSetps,
                                     int         numPixels,
                                     int         val )
{
  int step = numPixels;
  if ( step > numDataSetps )
    step = numDataSetps;

  if ( step <= 0 )
    step = 1;

  int max  = numDataSetps - step;
  if ( max <= 0 )
    max = 0;

  if ( val < 0 )
    val = 0;

  if ( val > max )
    val = max;

  scrollBar->setMinimum( 0 );
  scrollBar->setMaximum( max );
  scrollBar->setPageStep( step );
  scrollBar->setValue( val );
}


/**
 * Return true if the image horizontal scrollbar is enabled.
 */
bool SliderHandler::hSliderOn()
{
  return m_svUI->imageHorizontalScrollBar->isEnabled();
}


/**
 * Return true if the image vertical scrollbar is enabled.
 */
bool SliderHandler::vSliderOn()
{
  return m_svUI->imageVerticalScrollBar->isEnabled();
}


/**
 * Get the range of columns to display in the image.  NOTE: x_min will be
 * the smaller column number in the array, corresponding to lower values on
 * the calibrated x-scale.
 *
 * @param x_min   This will be set to the first bin number to display in the
 *                x direction.
 * @param x_max   This will be set to the last bin number to display in the
 *                x direction
 */
void SliderHandler::getHSliderInterval( int &x_min, int &x_max )
{
  QScrollBar* hScroll = m_svUI->imageHorizontalScrollBar;
  int step  = hScroll->pageStep();
  int value = hScroll->value();

  x_min = value;
  x_max = x_min + step;
}


/**
 * Get the range of rows to display in the image.  NOTE: y_min will be
 * the smaller row number in the array, corresponding to lower values on
 * the calibrated y-scale.
 *
 * @param y_min   This will be set to the first bin number to display in the
 *                y direction.
 * @param y_max   This will be set to the last bin number to display in the
 *                y direction
 */
void SliderHandler::getVSliderInterval( int &y_min, int &y_max )
{
  QScrollBar* vScroll = m_svUI->imageVerticalScrollBar;
  int max   = vScroll->maximum();
  int step  = vScroll->pageStep();
  int value = vScroll->value();

  y_min = max - value;        // invert value since scale increases from
  y_max = y_min + step;       // bottom to top, but scroll bar increases
                              // the other way.
}


} // namespace SpectrumView
} // namespace MantidQt
