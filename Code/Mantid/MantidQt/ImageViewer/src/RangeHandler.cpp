
#include <iostream>
#include <QLineEdit>

#include "MantidQtImageViewer/RangeHandler.h"

namespace MantidQt
{
namespace ImageView
{

/**
 *  Construct a RangeHandler object to manage min, max and step controls 
 *  in the specified UI
 */
RangeHandler::RangeHandler( Ui_MainWindow* iv_ui )
{
  this->iv_ui = iv_ui;
}


/**
 * Configure the min, max and step controls for the specified data source.
 *
 * @param data_source  ImageDataSource that provides the data to be drawn
 */
void RangeHandler::ConfigureRangeControls( ImageDataSource* data_source )
{
  
  double min_x     = data_source->GetXMin();
  double max_x     = data_source->GetXMax();
  size_t max_steps = 2000;

  QLineEdit* min_control  = iv_ui->x_min_input;
  QLineEdit* max_control  = iv_ui->x_max_input;
  QLineEdit* step_control = iv_ui->step_input;

  min_control->setText( "Hi min" );
  max_control->setText( "Hi max" );
  step_control->setText( "Hi step" );
}


/**
 * Get the interval of values and the step size to use for rebinning the
 * spectra.  
 *
 * @param min     This is the x value at the left edge of the first bin
 *                to display.
 * @param max     This is an x value at the right edge of the last bin
 *                to display.  This will be adjusted so that it is larger
 *                than min by an integer number of steps.  
 * @param step    This is size of the step to use between min and max.
 */
void RangeHandler::GetRange( double &min, double &max, double step )
{
}


} // namespace MantidQt 
} // namespace ImageView
