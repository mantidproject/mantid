
#include <iostream>

#include <QScrollBar>
#include "MantidQtRefDetectorViewer/RefSliderHandler.h"

namespace MantidQt
{
namespace RefDetectorViewer
{

/**
 *  Construct a RefSliderHandler object to manage the image scrollbars from the
 *  specified UI.
 */
RefSliderHandler::RefSliderHandler( Ui_RefImageViewer* ivUI ) : m_ivUI(ivUI)
{
}


/**
 * Configure the image scrollbars for the specified data and drawing area.
 *
 * @param drawArea    Rectangle specifiying the region where the image will
 *                    be drawn
 * @param dataSource  SpectrumDataSource that provides the data to be drawn
 */
void RefSliderHandler::configureSliders( QRect drawArea,
                                         SpectrumView::SpectrumDataSource_sptr dataSource )
{
  QScrollBar* v_scroll = m_ivUI->imageVerticalScrollBar;
  int n_rows = (int)dataSource->getNRows();
  configureSlider( v_scroll, n_rows, drawArea.height(), n_rows );

  configureHSlider( 2000, drawArea.width() );   // initial default, 2000 bins
}


/**
 *  Public method to configure the horizontal scrollbar to cover the
 *  specified range of data columns, displayed in the specified number of
 *  pixels.
 *
 *  @param nDataSteps  The number of columns in the data that should be
 *                     displayed
 *  @param nPixels     The number of pixels avaliable to show the data
 */
void RefSliderHandler::configureHSlider( int         nDataSteps,
                                         int         nPixels )
{
  QScrollBar* h_scroll = m_ivUI->imageHorizontalScrollBar;
  configureSlider( h_scroll, nDataSteps, nPixels, 0 );
}


/**
 *  Configure the specified scrollbar to cover the specified range of data
 *  steps, displayed in the specified number of pixels.
 *
 *  @param scrollBar   The scroll bar that will be configured
 *  @param nDataSteps  The number of bins in the data that should be
 *                     displayed
 *  @param nPixels     The number of pixels avaliable to show the data
 *  @param val         The initial position of the scrollbar, between 0 and
 *                     nDataSteps.
 */
void RefSliderHandler::configureSlider( QScrollBar* scrollBar,
                                        int         nDataSteps,
                                        int         nPixels,
                                        int         val )
{
  int step = nPixels;
  if ( step > nDataSteps )
    step = nDataSteps;

  if ( step <= 0 )
    step = 1;

  int max  = nDataSteps - step;
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
bool RefSliderHandler::hSliderOn()
{
  return m_ivUI->imageHorizontalScrollBar->isEnabled();
}


/**
 * Return true if the image vertical scrollbar is enabled.
 */
bool RefSliderHandler::vSliderOn()
{
  return m_ivUI->imageVerticalScrollBar->isEnabled();
}


/**
 * Get the range of columns to display in the image.  NOTE: xMin will be
 * the smaller column number in the array, corresponding to lower values on
 * the calibrated x-scale.
 *
 * @param xMin   This will be set to the first bin number to display in the
 *                x direction.
 * @param xMax   This will be set to the last bin number to display in the
 *                x direction
 */
void RefSliderHandler::getHSliderInterval( int &xMin, int &xMax )
{
  QScrollBar* h_scroll = m_ivUI->imageHorizontalScrollBar;
  int step  = h_scroll->pageStep();
  int value = h_scroll->value();

  xMin = value;
  xMax = xMin + step;
}


/**
 * Get the range of rows to display in the image.  NOTE: yMin will be
 * the smaller row number in the array, corresponding to lower values on
 * the calibrated y-scale.
 *
 * @param yMin   This will be set to the first bin number to display in the
 *                y direction.
 * @param yMax   This will be set to the last bin number to display in the
 *                y direction
 */
void RefSliderHandler::getVSliderInterval( int &yMin, int &yMax )
{
  QScrollBar* v_scroll = m_ivUI->imageVerticalScrollBar;
  int max   = v_scroll->maximum();
  int step  = v_scroll->pageStep();
  int value = v_scroll->value();

  yMin = max - value;       // invert value since scale increases from
  yMax = yMin + step;       // bottom to top, but scroll bar increases
                              // the other way.
}


} // namespace RefDetectorViewer
} // namespace MantidQt
