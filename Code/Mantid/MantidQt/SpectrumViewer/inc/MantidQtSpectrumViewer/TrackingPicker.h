#ifndef TRACKING_PICKER_H
#define TRACKING_PICKER_H

#include <qwt_plot_picker.h>
#include <qwt_plot_canvas.h>
#include "MantidQtSpectrumViewer/DllOptionSV.h"

/**
    @class TrackingPicker

    This class is a QwtPlotPicker that will emit a signal whenever the
    mouse is moved.  It was adapted from the SliceViewer's CustomPicker

    @author Dennis Mikkelson
    @date   2012-04-03

    Copyright © 2012 ORNL, STFC Rutherford Appleton Laboratories

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Code Documentation is available at
                 <http://doxygen.mantidproject.org>
 */

namespace MantidQt
{
namespace SpectrumView
{

class EXPORT_OPT_MANTIDQT_SPECTRUMVIEWER TrackingPicker : public QwtPlotPicker
{
  Q_OBJECT

public:

  /// Construct a tracking picker to work with the specified canvas
  TrackingPicker(QwtPlotCanvas* canvas);

  /// Disable (or enable) position readout at cursor position, even if
  /// tracking is ON.  Tracking MUST be on for the mouseMoved signal to be
  /// emitted.
  void hideReadout( bool hide );

signals:
  /// This signal will be emitted for each mouse moved event
  void mouseMoved(const QPoint & point) const;

protected:

  /// Override base class method, to emit a mousedMoved() signal for each move
  QwtText trackerText( const QPoint & point ) const;
  QwtText trackerText( const QwtDoublePoint & pos) const;

private:
  bool m_hideReadout;

};

} // namespace SpectrumView
} // namespace MantidQt

#endif  // TRACKING_PICKER_H
