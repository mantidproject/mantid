// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_MANTIDWIDGETS_PEAKPICKER_H_
#define MANTIDQT_MANTIDWIDGETS_PEAKPICKER_H_

#include "MantidKernel/System.h"

#include "MantidAPI/IPeakFunction.h"
#include "MantidQtWidgets/Plotting/DllOption.h"

#include <qwt_plot.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_picker.h>

namespace MantidQt {
namespace MantidWidgets {

/** PeakPicker : A simplified version of PeakPickerTool, available for use on
  general QwtPlots.
*/
class EXPORT_OPT_MANTIDQT_PLOTTING PeakPicker : public QwtPlotPicker,
                                                public QwtPlotItem {
  Q_OBJECT

public:
  /// Constructor
  PeakPicker(QwtPlot *plot, QColor color);

  /// Correct QwtPlotItem type info
  int rtti() const override { return QwtPlotItem::Rtti_PlotMarker; }

  /// Draw the peak picker
  /// @see QwtPlotItem::draw
  void draw(QPainter *painter, const QwtScaleMap &xMap, const QwtScaleMap &yMap,
            const QRect &canvasRect) const override;

  /// @return Currently represented peak
  Mantid::API::IPeakFunction_const_sptr peak() const;

  /// @param peak :: New peak to represent
  void setPeak(const Mantid::API::IPeakFunction_const_sptr &peak);

signals:
  /// Emitted when the peak picker is moved or resized in any way
  void changed();

private:
  /// Size of the dragging region. Bigger value means it's easier to hit the
  /// dragging area, though
  /// harder to get to the right element if there are lots of them.
  static const double DRAG_SENSITIVITY;

  /// Default cursor to use when not dragging
  static const Qt::CursorShape DEFAULT_CURSOR;

  /// Event filter installed for the plot
  bool eventFilter(QObject *object, QEvent *event) override;

  /// The plot peak picker operates on
  QwtPlot *m_plot;

  /// Pens used for drawing
  QPen m_basePen, m_widthPen;

  /// Dragging status flags
  bool m_isMoving, m_isResizing;

  /// Currently represented peak
  Mantid::API::IPeakFunction_sptr m_peak;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTIDQT_MANTIDWIDGETS_PEAKPICKER_H_ */
