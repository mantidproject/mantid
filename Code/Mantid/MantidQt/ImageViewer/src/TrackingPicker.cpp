
#include "MantidQtImageViewer/TrackingPicker.h" 

namespace MantidQt
{
namespace ImageView
{


TrackingPicker::TrackingPicker( QwtPlotCanvas* canvas )
               :QwtPlotPicker( canvas )
{
  hide_readout = true;
}


void TrackingPicker::HideReadout( bool hide )
{
  this->hide_readout = hide;
}


QwtText TrackingPicker::trackerText( const QPoint & point ) const
{
  emit mouseMoved();
  if ( hide_readout )
  {
    return QwtText();
  }
  else                                      // call super class trackerText
  {                                         // so the tracker text still works
    return QwtPlotPicker::trackerText( point );
  }
}


QwtText TrackingPicker::trackerText( const QwtDoublePoint & pos ) const
{
  emit mouseMoved();
  if ( hide_readout )
  {
    return QwtText();
  }
  else                                      // call super class trackerText
  {                                         // so the tracker text still works
    return QwtPlotPicker::trackerText( pos );
  }
}

} // namespace MantidQt 
} // namespace ImageView
