
#include <iostream>

#include <QScrollBar>
#include "MantidQtImageViewer/SliderHandler.h"

namespace MantidQt
{
namespace ImageView
{

/**
 *  Construct a SliderHandler object to manage the image sliders from the 
 *  specified UI.
 */
SliderHandler::SliderHandler( Ui_MainWindow* iv_ui )
{
  this->iv_ui = iv_ui;
}


/**
 * Configure the image sliders for the specified data and drawing area.
 *
 * @param draw_area    Rectangle specifiying the region where the image will
 *                     be drawn
 * @param data_source  ImageDataSource that provides the data to be drawn
 */
void SliderHandler::ConfigureSliders( QRect            draw_area, 
                                      ImageDataSource* data_source )
{
  QScrollBar* v_scroll = iv_ui->imageVerticalScrollBar;
  int n_rows = (int)data_source->GetNRows();
  int min  = 0;
  int step = draw_area.height();
  if ( step > n_rows )
  {
    step = n_rows;
  }
  if ( step <= 0 )
  {
    step = 1;
  }

  int max  = n_rows - step;
  if ( max <= 0 )
    max = min;
  v_scroll->setMinimum( 0 );
  v_scroll->setMaximum( max );
  v_scroll->setPageStep( step );
  v_scroll->setValue( max );

  QScrollBar* h_scroll = iv_ui->imageHorizontalScrollBar;
  int n_cols = (int)data_source->GetNCols();
  min  = 0;
  step = draw_area.width();
  if ( step > n_cols )
  {
    step = n_cols;
  }
  if ( step <= 0 )
  {
    step = 1;
  }

  max  = n_cols - step;
  if ( max <= 0 )
    max = min;
  h_scroll->setMinimum( 0 );
  h_scroll->setMaximum( max );
  h_scroll->setPageStep( step );
  h_scroll->setValue( 0 );
}


/**
 * Return true if the image horizontal slider is enabled.
 */
bool SliderHandler::HSliderOn()
{
  return iv_ui->imageHorizontalScrollBar->isEnabled();
}


/**
 * Return true if the image vertical slider is enabled.
 */
bool SliderHandler::VSliderOn()
{
  return iv_ui->imageVerticalScrollBar->isEnabled();
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
  QScrollBar* h_scroll = iv_ui->imageHorizontalScrollBar;
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
  QScrollBar* v_scroll = iv_ui->imageVerticalScrollBar;
  int max   = v_scroll->maximum();
  int step  = v_scroll->pageStep();
  int value = v_scroll->value();
                                
  y_min = max - value;        // invert value since scale increases from 
  y_max = y_min + step;       // bottom to top, but scroll bar increases
                              // the other way.
}


} // namespace MantidQt 
} // namespace ImageView
