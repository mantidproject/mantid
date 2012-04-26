
#include <iostream>
#include <QLineEdit>

#include "MantidQtImageViewer/RangeHandler.h"
#include "MantidQtImageViewer/IVUtils.h"

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
  
  double min_x   = data_source->GetXMin();
  double max_x   = data_source->GetXMax();
  size_t n_steps = 2000;

  double step = (max_x - min_x) / (double)n_steps;

  QLineEdit* min_control  = iv_ui->x_min_input;
  QLineEdit* max_control  = iv_ui->x_max_input;
  QLineEdit* step_control = iv_ui->step_input;

  std::string min_text;
  IVUtils::Format( 7, 1, min_x, min_text );
  QString q_min_text = QString::fromStdString( min_text );

  std::string max_text;
  IVUtils::Format( 7, 1, max_x, max_text );
  QString q_max_text = QString::fromStdString( max_text );

  std::string step_text;
  IVUtils::Format( 7, 4, step, step_text );
  QString q_step_text = QString::fromStdString( step_text );

  min_control->setText( q_min_text );
  max_control->setText( q_max_text );
  step_control->setText( q_step_text );
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
