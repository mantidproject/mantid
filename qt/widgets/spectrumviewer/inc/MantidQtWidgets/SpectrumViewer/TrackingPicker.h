// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef TRACKING_PICKER_H
#define TRACKING_PICKER_H

#include "MantidQtWidgets/SpectrumViewer/DllOptionSV.h"
#include <qwt_plot_canvas.h>
#include <qwt_plot_picker.h>

/**
    @class TrackingPicker

    This class is a QwtPlotPicker that will emit a signal whenever the
    mouse is moved.  It was adapted from the SliceViewer's CustomPicker

    @author Dennis Mikkelson
    @date   2012-04-03
 */

namespace MantidQt {
namespace SpectrumView {

class EXPORT_OPT_MANTIDQT_SPECTRUMVIEWER TrackingPicker : public QwtPlotPicker {
  Q_OBJECT

public:
  /// Construct a tracking picker to work with the specified canvas
  TrackingPicker(QwtPlotCanvas *canvas);

  /// Disable (or enable) position readout at cursor position, even if
  /// tracking is ON.  Tracking MUST be on for the mouseMoved signal to be
  /// emitted.
  void hideReadout(bool hide);

signals:
  /// This signal will be emitted for each mouse moved event
  void mouseMoved(const QPoint &point) const;

protected:
  /// Override base class method, to emit a mousedMoved() signal for each move
  QwtText trackerText(const QPoint &point) const override;
  QwtText trackerText(const QwtDoublePoint &pos) const override;

private:
  bool m_hideReadout;
};

} // namespace SpectrumView
} // namespace MantidQt

#endif // TRACKING_PICKER_H
