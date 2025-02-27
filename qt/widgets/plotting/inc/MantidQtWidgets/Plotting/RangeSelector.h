// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/MplCpp/RangeMarker.h"
#include "MantidQtWidgets/Plotting/DllOption.h"

#include <QWidget>

namespace MantidQt {
namespace MantidWidgets {
class PreviewPlot;

/**
 * Displays a two lines for selecting a range on a previewplot
 */
class EXPORT_OPT_MANTIDQT_PLOTTING RangeSelector : public QObject {
  Q_OBJECT

public:
  enum SelectType { XMINMAX, YMINMAX };

  RangeSelector(PreviewPlot *plot, SelectType type = XMINMAX, bool visible = true, bool infoOnly = false,
                const QColor &colour = Qt::black);

  void setColour(const QColor &colour);
  void setRange(const std::pair<double, double> &range);
  std::pair<double, double> getRange() const;

  void setMinimum(double min);
  void setMaximum(double max);
  double getMinimum() const;
  double getMaximum() const;

  void setVisible(bool visible);
  void setBounds(const double min, const double max);

  void detach();

  // cppcheck-suppress unknownMacro
public slots:
  void setRange(double min, double max);

signals:
  void selectionChanged(double min, double max);
  void minValueChanged(double min);
  void maxValueChanged(double max);

private slots:
  void handleMouseDown(const QPoint &point);
  void handleMouseMove(const QPoint &point);
  void handleMouseUp(const QPoint &point);

  void redrawMarker();

private:
  std::tuple<double, double> getAxisRange(const SelectType &type) const;
  QString selectTypeAsQString(const SelectType &type) const;

  /// The preview plot containing the range selector
  PreviewPlot *m_plot;
  /// The range marker
  std::unique_ptr<MantidQt::Widgets::MplCpp::RangeMarker> m_rangeMarker;
  /// The type of the range marker
  SelectType m_type;
  /// Is the marker visible or hidden
  bool m_visible;
  ///	Is the marker moving
  bool m_markerMoving;
};

} // namespace MantidWidgets
} // namespace MantidQt
