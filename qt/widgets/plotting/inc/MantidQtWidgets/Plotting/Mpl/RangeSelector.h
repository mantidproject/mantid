// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_PLOTTING_MPL_RANGESELECTOR_H_
#define MANTIDQT_PLOTTING_MPL_RANGESELECTOR_H_

#include "MantidQtWidgets/MplCpp/RangeMarker.h"
#include "MantidQtWidgets/Plotting/DllOption.h"

#include <QWidget>

namespace MantidQt {
namespace MantidWidgets {
class PreviewPlot;

/**
 * Displays a two vertical lines for selecting a range on a previewplot
 */
class EXPORT_OPT_MANTIDQT_PLOTTING RangeSelector : public QObject {
  Q_OBJECT

public:
  enum SelectType { XMINMAX, XSINGLE, YMINMAX, YSINGLE };

  RangeSelector(PreviewPlot *plot, SelectType type = XMINMAX,
                bool visible = true, bool infoOnly = false,
                const QColor &colour = Qt::black);

  void setColour(const QColor &colour);
  void setRange(const std::pair<double, double> &range);
  void setRange(const double min, const double max);
  std::pair<double, double> getRange() const;

  void setMinimum(const double min);
  void setMaximum(const double max);
  double getMinimum() const;
  double getMaximum() const;

  void setVisible(bool visible);

  void detach();

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
  /// Is the marker visible or hidden
  bool m_visible;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQT_PLOTTING_MPL_RANGESELECTOR_H_
