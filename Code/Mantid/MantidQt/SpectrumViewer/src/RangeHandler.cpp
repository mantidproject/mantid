
#include <iostream>
#include <QLineEdit>

#include "MantidQtSpectrumViewer/RangeHandler.h"
#include "MantidQtSpectrumViewer/QtUtils.h"
#include "MantidQtSpectrumViewer/SVUtils.h"
#include "MantidQtSpectrumViewer/ErrorHandler.h"

namespace MantidQt
{
namespace SpectrumView
{

/**
 *  Construct a RangeHandler object to manage min, max and step controls 
 *  in the specified UI
 */
RangeHandler::RangeHandler( Ui_SpectrumViewer* iv_ui ) : IRangeHandler()
{
  this->iv_ui = iv_ui;
}


/**
 * Configure the min, max and step controls for the specified data source.
 *
 * @param data_source  SpectrumDataSource that provides the data to be drawn
 */
void RangeHandler::ConfigureRangeControls( SpectrumDataSource* data_source )
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
 * @param min     On input, this should be the default value that the
 *                min should be set to, if getting the range fails.
 *                On output this is will be set to the x value at the 
 *                left edge of the first bin to display, if getting the
 *                range succeeds.
 * @param max     On input, this should be the default value that the
 *                max should be set to if getting the range fails.
 *                On output, if getting the range succeeds, this will
 *                be set an x value at the right edge of the last bin
 *                to display.  This will be adjusted so that it is larger
 *                than min by an integer number of steps.  
 * @param step    On input this should be the default number of steps
 *                to use if getting the range information fails.
 *                On output, this is size of the step to use between
 *                min and max.  If it is less than zero, a log scale 
 *                is requested.
 */
void RangeHandler::GetRange( double &min, double &max, double &step )
{
  double original_min  = min;
  double original_max  = max;
  double original_step = step;

  QLineEdit* min_control  = iv_ui->x_min_input;
  QLineEdit* max_control  = iv_ui->x_max_input;
  QLineEdit* step_control = iv_ui->step_input;

  if ( !SVUtils::StringToDouble(  min_control->text().toStdString(), min ) )
  {
    ErrorHandler::Error("X Min is not a NUMBER! Value reset.");
    min = original_min;
  }
  if ( !SVUtils::StringToDouble(  max_control->text().toStdString(), max ) )
  {
    ErrorHandler::Error("X Max is not a NUMBER! Value reset.");
    max = original_max;
  }
  if ( !SVUtils::StringToDouble(  step_control->text().toStdString(), step ) )
  {
    ErrorHandler::Error("Step is not a NUMBER! Value reset.");
    step = original_step;
  }

                                 // just require step to be non-zero, no other
                                 // bounds. If zero, take a default step size  
  if ( step == 0 ) 
  {
    ErrorHandler::Error("Step = 0, resetting to default step");
    step = original_step;
  }

  if ( step > 0 )
  {
    if ( !SVUtils::FindValidInterval( min, max ) )
    {
      ErrorHandler::Warning( 
             "In GetRange: [Min,Max] interval invalid, values adjusted" );
      min  = original_min;
      max  = original_max;
      step = original_step;
    }
  }
  else
  {
    if ( !SVUtils::FindValidLogInterval( min, max ) )
    {
      ErrorHandler::Warning(
          "In GetRange: [Min,Max] log interval invalid, values adjusted");
      min  = original_min;
      max  = original_max;
      step = original_step;
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
  if ( !SVUtils::FindValidInterval( min, max ) )
  {
    ErrorHandler::Warning( 
            "In SetRange: [Min,Max] interval invalid, values adjusted" );
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
  QtUtils::SetText( 8, 6, step, iv_ui->step_input );
}


} // namespace SpectrumView
} // namespace MantidQt 
