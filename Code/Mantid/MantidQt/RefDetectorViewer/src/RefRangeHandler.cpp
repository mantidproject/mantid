
#include <iostream>
#include <QLineEdit>

#include "MantidQtRefDetectorViewer/RefRangeHandler.h"
#include "MantidQtSpectrumViewer/QtUtils.h"
#include "MantidQtSpectrumViewer/IVUtils.h"
#include "MantidQtSpectrumViewer/ErrorHandler.h"

namespace MantidQt
{
namespace RefDetectorViewer
{
  using namespace SpectrumView;

/**
 *  Construct a RefRangeHandler object to manage min, max and step controls
 *  in the specified UI
 */
RefRangeHandler::RefRangeHandler( Ui_RefImageViewer* iv_ui )
{
  this->iv_ui = iv_ui;
}


/**
 * Configure the min, max and step controls for the specified data source.
 *
 * @param data_source  ImageDataSource that provides the data to be drawn
 */
void RefRangeHandler::ConfigureRangeControls( ImageDataSource* data_source )
{
  
    //x axis
  total_min_x   = data_source->GetXMin();
  total_max_x   = data_source->GetXMax();
  total_n_steps = data_source->GetNCols();
  total_min_y   = data_source->GetYMin();
  total_max_y   = data_source->GetYMax();

  double defaultx_step = (total_max_x - total_min_x)/(double)total_n_steps;
  if ( total_n_steps > 2000 )
  {
    defaultx_step = (total_max_x - total_min_x)/2000.0;
  }

  SetRange( total_min_x, total_max_x, defaultx_step, 'x');

    //y axis
    total_min_y   = data_source->GetYMin();
    total_max_y   = data_source->GetYMax();
    total_n_steps = data_source->GetNCols();
    
    double defaulty_step = (total_max_y - total_min_y)/(double)total_n_steps;
    if ( total_n_steps > 2000 )
    {
        defaulty_step = (total_max_y - total_min_y)/2000.0;
    }
    
    SetRange( total_min_y, total_max_y, defaulty_step, 'y' );


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
void RefRangeHandler::GetRange( double &min, double &max, double &step )
{ 
   double original_min  = min;
  double original_max  = max;
  double original_step = step;

  QLineEdit* min_control  = iv_ui->x_min_input;
  QLineEdit* max_control  = iv_ui->x_max_input;
//  QLineEdit* step_control = iv_ui->step_input;

  if ( !IVUtils::StringToDouble(  min_control->text().toStdString(), min ) )
  {
    ErrorHandler::Error("X Min is not a NUMBER! Value reset.");
    min = original_min;
  }
  if ( !IVUtils::StringToDouble(  max_control->text().toStdString(), max ) )
  {
    ErrorHandler::Error("X Max is not a NUMBER! Value reset.");
    max = original_max;
  }
//  if ( !IVUtils::StringToDouble(  step_control->text().toStdString(), step ) )
//  {
//    ErrorHandler::Error("Step is not a NUMBER! Value reset.");
//    step = original_step;
//  }

                                 // just require step to be non-zero, no other
                                 // bounds. If zero, take a default step size  
  if ( step == 0 ) 
  {
    ErrorHandler::Error("Step = 0, resetting to default step");
    step = original_step;
  }

  if ( step > 0 )
  {
    if ( !IVUtils::FindValidInterval( min, max ) )
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
    if ( !IVUtils::FindValidLogInterval( min, max ) )
    {
      ErrorHandler::Warning(
          "In GetRange: [Min,Max] log interval invalid, values adjusted");
      min  = original_min;
      max  = original_max;
      step = original_step;
    }
  }

  SetRange( min, max, step, 'x' );
}


/**
 * Adjust the values to be consistent with the available data and 
 * diplay them in the controls.
 *
 * @param min     This is the x value at the left edge of the first bin.
 * @param max     This is an x value at the right edge of the last bin.
 * @param step    This is size of the step to use between min and max. 
 *                If it is less than zero, a log scale is requested.
 * @param type    x or y
 */
void RefRangeHandler::SetRange( double min, double max, double step, char type )
{
    if (type == 'x') {  
    
  if ( !IVUtils::FindValidInterval( min, max ) )
  {
    ErrorHandler::Warning( 
            "In SetRange: [XMin,XMax] interval invalid, values adjusted" );
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
//  QtUtils::SetText( 8, 4, step, iv_ui->step_input );
        
    } 
    
    if (type == 'y') {  
        
        if ( !IVUtils::FindValidInterval( min, max ) )
        {
            ErrorHandler::Warning( 
                                  "In SetRange: [YMin,YMax] interval invalid, values adjusted" );
        }
        
        if ( min < total_min_y || min > total_max_y )
        {
            //    ErrorHandler::Warning("Y Min out of range, resetting to range min.");
            min = total_min_y;
        }
        
        if ( max < total_min_y || max > total_max_y )
        {
            //    ErrorHandler::Warning("Y Max out of range, resetting to range max.");
            max = total_max_y;
        }
        
        if ( step == 0 )
        {
            ErrorHandler::Error("Step = 0, resetting to default step");
            step = (max-min)/2000.0;
        }
        
        QtUtils::SetText( 8, 2, min, iv_ui->y_min_input );
        QtUtils::SetText( 8, 2, max, iv_ui->y_max_input );
        //  QtUtils::SetText( 8, 4, step, iv_ui->step_input );
        
    } 
    
}


} // namespace RefDetectorViewer
} // namespace MantidQt 
