
#include <iostream>

#include <QScrollBar>
#include "MantidQtImageViewer/SliderHandler.h"

namespace MantidQt
{
namespace ImageView
{


SliderHandler::SliderHandler( Ui_MainWindow* iv_ui )
{
  this->iv_ui = iv_ui;
}

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

bool SliderHandler::HSliderOn()
{
  return iv_ui->imageHorizontalScrollBar->isEnabled();
}

bool SliderHandler::VSliderOn()
{
  return iv_ui->imageVerticalScrollBar->isEnabled();
}

void SliderHandler::GetHSliderInterval( int &x_min, int &x_max )
{
  QScrollBar* h_scroll = iv_ui->imageHorizontalScrollBar;
  int step  = h_scroll->pageStep();
  int value = h_scroll->value();

  x_min = value;     
  x_max = x_min + step;       
}

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
