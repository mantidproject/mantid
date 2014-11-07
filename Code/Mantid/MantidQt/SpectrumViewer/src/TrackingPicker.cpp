#include "MantidQtSpectrumViewer/TrackingPicker.h"

namespace MantidQt
{
namespace SpectrumView
{

/**
 *  Construct a tracking picker to work with the specified canvas
 *
 *  @param canvas  Pointer to the QwtPlotCanvas this picker will work with
 */
TrackingPicker::TrackingPicker( QwtPlotCanvas* canvas )
  : QwtPlotPicker( canvas ), m_hideReadout(true)
{
}


/**
 * Enable or disable the position readout at the mouse location for this
 * picker.
 *
 * @param hide  If true, the position readout at the mouse position will
 *              be turned off.
 */
void TrackingPicker::hideReadout( bool hide )
{
  m_hideReadout = hide;
}


/**
 *  This overrides the base class trackerText() function so that we can
 *  continuously emit a signal as the mouse is moved.
 *
 *  @param point  The current mouse location.
 */
QwtText TrackingPicker::trackerText( const QPoint & point ) const
{
  emit mouseMoved(point);
  if ( m_hideReadout )
  {
    return QwtText();
  }
  else                                      // call super class trackerText
  {                                         // so the tracker text still works
    return QwtPlotPicker::trackerText( point );
  }
}


/**
 *  This overrides the base class trackerText() function so that we can
 *  continuously emit a signal as the mouse is moved.
 *
 *  @param pos  The current mouse location.
 */
QwtText TrackingPicker::trackerText( const QwtDoublePoint & pos ) const
{
  emit mouseMoved(pos.toPoint());
  if ( m_hideReadout )
  {
    return QwtText();
  }
  else                                      // call super class trackerText
  {                                         // so the tracker text still works
    return QwtPlotPicker::trackerText( pos );
  }
}

} // namespace SpectrumView
} // namespace MantidQt
