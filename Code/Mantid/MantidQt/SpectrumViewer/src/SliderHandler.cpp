
#include <iostream>

#include <QScrollBar>
#include "MantidQtSpectrumViewer/SliderHandler.h"

namespace MantidQt
{
namespace SpectrumView
{

/**
 *  Construct a SliderHandler object to manage the image scrollbars from the 
 *  specified UI.
 */
SliderHandler::SliderHandler( Ui_SpectrumViewer* sv_ui ) : ISliderHandler()
{
  this->sv_ui = sv_ui;
}


/**
 * Configure the image scrollbars for the specified data and drawing area.
 *
 * @param draw_area    Rectangle specifiying the region where the image will
 *                     be drawn
 * @param data_source  SpectrumDataSource that provides the data to be drawn
 */
void SliderHandler::ConfigureSliders( QRect            draw_area, 
                                      SpectrumDataSource* data_source )
{
  QScrollBar* v_scroll = sv_ui->imageVerticalScrollBar;
  int n_rows = (int)data_source->GetNRows();
  ConfigureSlider( v_scroll, n_rows, draw_area.height(), n_rows );

  ConfigureHSlider( 2000, draw_area.width() );   // initial default, 2000 bins
}


/**
 *  Public method to configure the horizontal scrollbar to cover the 
 *  specified range of data columns, displayed in the specified number of
 *  pixels.  
 *
 *  @param n_data_steps  The number of columns in the data that should be 
 *                       displayed
 *  @param n_pixels      The number of pixels avaliable to show the data
 */
void SliderHandler::ConfigureHSlider( int         n_data_steps,
                                      int         n_pixels )
{
  QScrollBar* h_scroll = sv_ui->imageHorizontalScrollBar;
  ConfigureSlider( h_scroll, n_data_steps, n_pixels, 0 );
}


/**
 *  Configure the specified scrollbar to cover the specified range of data
 *  steps, displayed in the specified number of pixels.  
 *
 *  @param scroll_bar    The scroll bar that will be configured
 *  @param n_data_steps  The number of bins in the data that should be 
 *                       displayed
 *  @param n_pixels      The number of pixels avaliable to show the data
 *  @param val           The initial position of the scrollbar, between 0 and
 *                       n_data_steps.
 */
void SliderHandler::ConfigureSlider( QScrollBar* scroll_bar, 
                                     int         n_data_steps, 
                                     int         n_pixels, 
                                     int         val )
{
  int step = n_pixels;
  if ( step > n_data_steps )
  {
    step = n_data_steps;
  }
  if ( step <= 0 )
  {
    step = 1;
  }

  int max  = n_data_steps - step;
  if ( max <= 0 )
  {
    max = 0;
  }

  if ( val < 0 )
  {
    val = 0;
  }

  if ( val > max )
  {
    val = max;
  }

  scroll_bar->setMinimum( 0 );
  scroll_bar->setMaximum( max );
  scroll_bar->setPageStep( step );
  scroll_bar->setValue( val );
}


/**
 * Return true if the image horizontal scrollbar is enabled.
 */
bool SliderHandler::HSliderOn()
{
  return sv_ui->imageHorizontalScrollBar->isEnabled();
}


/**
 * Return true if the image vertical scrollbar is enabled.
 */
bool SliderHandler::VSliderOn()
{
  return sv_ui->imageVerticalScrollBar->isEnabled();
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
void SliderHandler::GetHSliderInterval( int &x_min, int &x_max )
{
  QScrollBar* h_scroll = sv_ui->imageHorizontalScrollBar;
  int step  = h_scroll->pageStep();
  int value = h_scroll->value();

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
void SliderHandler::GetVSliderInterval( int &y_min, int &y_max )
{
  QScrollBar* v_scroll = sv_ui->imageVerticalScrollBar;
  int max   = v_scroll->maximum();
  int step  = v_scroll->pageStep();
  int value = v_scroll->value();
                                
  y_min = max - value;        // invert value since scale increases from 
  y_max = y_min + step;       // bottom to top, but scroll bar increases
                              // the other way.
}


} // namespace SpectrumView
} // namespace MantidQt 
