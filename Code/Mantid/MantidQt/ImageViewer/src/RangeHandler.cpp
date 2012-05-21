
#include <iostream>
#include <QLineEdit>

#include "MantidQtImageViewer/RangeHandler.h"
#include "MantidQtImageViewer/QtUtils.h"
#include "MantidQtImageViewer/IVUtils.h"
#include "MantidQtImageViewer/ErrorHandler.h"

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
  
  total_min_x   = data_source->GetXMin();
  total_max_x   = data_source->GetXMax();
  total_n_steps = data_source->GetNCols();

  double default_step = (total_max_x - total_min_x)/(double)total_n_steps;
  if ( total_n_steps > 2000 )
  {
    default_step = (total_max_x - total_min_x)/2000.0;
  }

  SetRange( total_min_x, total_max_x, default_step );
}


/**
 * Get the interval of values and the step size to use for rebinning the
 * spectra.  The range values are validated and adjusted if needed.  The
 * range values that are returned by this method will also be displayed in
 * the controls.
 *
 * @param min     This is the x value at the left edge of the first bin
 *                to display.
 * @param max     This is an x value at the right edge of the last bin
 *                to display.  This will be adjusted so that it is larger
 *                than min by an integer number of steps.  
 * @param step    This is size of the step to use between min and max.
 *                If it is less than zero, a log scale is requested.
 */
void RangeHandler::GetRange( double &min, double &max, double &step )
{
  QLineEdit* min_control  = iv_ui->x_min_input;
  QLineEdit* max_control  = iv_ui->x_max_input;
  QLineEdit* step_control = iv_ui->step_input;

  if ( !IVUtils::StringToDouble(  min_control->text().toStdString(), min ) )
  {
    ErrorHandler::Error("X Min is not a NUMBER! Value reset.");
  }
  if ( !IVUtils::StringToDouble(  max_control->text().toStdString(), max ) )
  {
    ErrorHandler::Error("X Max is not a NUMBER! Value reset.");
  }
  if ( !IVUtils::StringToDouble(  step_control->text().toStdString(), step ) )
  {
    ErrorHandler::Error("Step is not a NUMBER! Value reset.");
  }

                                 // just require step to be non-zero, no other
                                 // bounds. If zero, take a default step size  
  if ( step == 0 ) 
  {
    ErrorHandler::Error("Step = 0, resetting to default step");
    step = (total_max_x - total_min_x) / 2000.0;
  }

  if ( step > 0 )
  {
    if ( !IVUtils::FindValidInterval( min, max ) )
    {
      ErrorHandler::Warning( "[Min,Max] interval invalid, values adjusted" );
    }
  }
  else
  {
    if ( !IVUtils::FindValidLogInterval( min, max ) )
    {
      ErrorHandler::Warning("[Min,Max] log interval invalid, values adjusted");
    }
  }

  SetRange( min, max, step );
}


/**
 * Adjust the values to be consistent with the available data and 
 * diplay them in the controls.
 *
 * @param min     This is the x value at the left edge of the first bin.
 * @param max     This is an x value at the right edge of the last bin.
 * @param step    This is size of the step to use between min and max. 
 *                If it is less than zero, a log scale is requested.
 */
void RangeHandler::SetRange( double min, double max, double step )
{
  if ( !IVUtils::FindValidInterval( min, max ) )
  {
    ErrorHandler::Warning( "[Min,Max] interval invalid, values adjusted" );
  }

  if ( min < total_min_x || min > total_max_x )
  {
//    ErrorHandler::Warning("X Min out of range, resetting to range min.");
    min = total_min_x;
  }

  if ( max < total_min_x || max > total_max_x )
  {
//    ErrorHandler::Warning("X Max out of range, resetting to range max.");
    max = total_max_x;
  }

  if ( step == 0 )
  {
    ErrorHandler::Error("Step = 0, resetting to default step");
    step = (max-min)/2000.0;
  }

  QtUtils::SetText( 8, 2, min, iv_ui->x_min_input );
  QtUtils::SetText( 8, 2, max, iv_ui->x_max_input );
  QtUtils::SetText( 8, 4, step, iv_ui->step_input );
}


} // namespace MantidQt 
} // namespace ImageView
