// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_MANTIDWIDGET_QWT_SINGLESELECTOR_H
#define MANTIDQT_MANTIDWIDGET_QWT_SINGLESELECTOR_H

#include "MantidQtWidgets/Plotting/DllOption.h"

#include <qwt_plot.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_picker.h>

#include <QPen>

namespace MantidQt {
namespace MantidWidgets {
class PreviewPlot;

/**
 * Displays a line for selecting a value on a QwtPlot in MantidQt.
 */
class EXPORT_OPT_MANTIDQT_PLOTTING SingleSelector : public QwtPlotPicker {
  Q_OBJECT
public:
  enum SelectType { XSINGLE, YSINGLE };

  SingleSelector(QwtPlot *plot, SelectType type = XSINGLE, bool visible = true,
                 bool infoOnly = false);
  SingleSelector(PreviewPlot *plot, SelectType type = XSINGLE,
                 bool visible = true, bool infoOnly = false);
  ~SingleSelector() override{};

  void setColour(const QColor &colour);

  void setBounds(const std::pair<double, double> &bounds);
  void setBounds(const double minimum, const double maximum);
  void setLowerBound(const double minimum);
  void setUpperBound(const double maximum);

  void setPosition(const double position);
  double getPosition() const;

  void setVisible(bool visible);

  void detach();

signals:
  void valueChanged(double position);

private:
  void init();
  void setLinePosition(const double position);
  bool isInsideBounds(double /*x*/);
  bool isMarkerMoving(double /*x*/, double /*xPlusdx*/);
  bool eventFilter(QObject * /*unused*/, QEvent * /*unused*/) override;

  /// type of selection widget is for
  SelectType m_type;

  /// The current position of the line
  double m_position;
  /// The lower bound allowed for the lines position
  double m_lowerBound;
  /// The upper bound allowed for the lines position
  double m_upperBound;
  /// The line object in the plot marking the position
  QwtPlotMarker *m_singleMarker;

  /// The plot
  QwtPlot *m_plot;
  /// The canvas
  QwtPlotCanvas *m_canvas;

  /// Stores whether or not the marker is moving
  bool m_markerMoving;

  /// Whether the line should be visible
  bool m_visible;

  /// The pen object used to define line style, colour, etc
  QPen *m_pen;
  /// The cursor object to display when an item is being moved
  QCursor m_moveCursor;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTIDQT_MANTIDWIDGET_QWT_SINGLESELECTOR_H */
