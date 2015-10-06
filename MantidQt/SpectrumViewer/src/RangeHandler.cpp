#include <iostream>
#include <QLineEdit>

#include "MantidQtSpectrumViewer/RangeHandler.h"
#include "MantidQtSpectrumViewer/QtUtils.h"
#include "MantidQtSpectrumViewer/SVUtils.h"
#include "MantidKernel/Logger.h"


namespace
{
  Mantid::Kernel::Logger g_log("SpectrumView");
}


namespace MantidQt
{
namespace SpectrumView
{

/**
 *  Construct a RangeHandler object to manage min, max and step controls
 *  in the specified UI
 */
RangeHandler::RangeHandler( Ui_SpectrumViewer* svUI ) :
  IRangeHandler(),
  m_svUI(svUI),
  m_totalMinX(0.0), m_totalMaxX(0.0),
  m_totalNSteps(0)
{
}


/**
 * Configure the min, max and step controls for the specified data source.
 *
 * @param dataSource  SpectrumDataSource that provides the data to be drawn
 */
void RangeHandler::configureRangeControls( SpectrumDataSource_sptr dataSource )
{
  m_totalMinX   = dataSource->getXMin();
  m_totalMaxX   = dataSource->getXMax();
  m_totalNSteps = dataSource->getNCols();

  double defaultStep = (m_totalMaxX - m_totalMinX) / (double)m_totalNSteps;
  if ( m_totalNSteps > 2000 )
    defaultStep = (m_totalMaxX - m_totalMinX) / 2000.0;

  setRange( m_totalMinX, m_totalMaxX, defaultStep );
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
void RangeHandler::getRange( double &min, double &max, double &step )
{
  double originalMin  = min;
  double originalMax  = max;
  double originalStep = step;

  QLineEdit* minControl  = m_svUI->x_min_input;
  QLineEdit* maxControl  = m_svUI->x_max_input;
  QLineEdit* stepControl = m_svUI->step_input;

  bool minIsNumber = false;
  bool maxIsNumber = false;
  bool stepIsNumber = false;

  min = minControl->text().toDouble(&minIsNumber);
  max = maxControl->text().toDouble(&maxIsNumber);
  step = stepControl->text().toDouble(&stepIsNumber);

  if(!minIsNumber)
  {
    g_log.information("X Min is not a NUMBER! Value reset.");
    min = originalMin;
  }

  if(!maxIsNumber)
  {
    g_log.information("X Max is not a NUMBER! Value reset.");
    max = originalMax;
  }

  if(!stepIsNumber)
  {
    g_log.information("Step is not a NUMBER! Value reset.");
    step = originalStep;
  }

  // Just require step to be non-zero, no other bounds. If zero, take a default step size
  if ( step == 0 )
  {
    g_log.information("Step = 0, resetting to default step");
    step = originalStep;
  }

  if ( step > 0 )
  {
    if ( !SVUtils::FindValidInterval( min, max ) )
    {
      g_log.information( "In GetRange: [Min,Max] interval invalid, values adjusted" );
      min  = originalMin;
      max  = originalMax;
      step = originalStep;
    }
  }
  else
  {
    if ( !SVUtils::FindValidLogInterval( min, max ) )
    {
      g_log.information( "In GetRange: [Min,Max] log interval invalid, values adjusted");
      min  = originalMin;
      max  = originalMax;
      step = originalStep;
    }
  }

  setRange( min, max, step );
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
void RangeHandler::setRange( double min, double max, double step )
{
  if ( !SVUtils::FindValidInterval( min, max ) )
    g_log.information("In SetRange: [Min,Max] interval invalid, values adjusted" );

  if ( min < m_totalMinX || min > m_totalMaxX )
  {
    g_log.information("X Min out of range, resetting to range min.");
    min = m_totalMinX;
  }

  if ( max < m_totalMinX || max > m_totalMaxX )
  {
    g_log.information("X Max out of range, resetting to range max.");
    max = m_totalMaxX;
  }

  if ( step == 0 )
  {
    g_log.information("Step = 0, resetting to default step");
    step = (max - min) / 2000.0;
  }

  QtUtils::SetText( 8, 2, min, m_svUI->x_min_input );
  QtUtils::SetText( 8, 2, max, m_svUI->x_max_input );
  QtUtils::SetText( 8, 6, step, m_svUI->step_input );
}


} // namespace SpectrumView
} // namespace MantidQt
