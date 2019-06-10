// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_PLOTTING_MPL_RANGESELECTOR_H_
#define MANTIDQT_PLOTTING_MPL_RANGESELECTOR_H_

#include "MantidQtWidgets/MplCpp/VerticalMarker.h"
#include "MantidQtWidgets/Plotting/DllOption.h"
#include <QWidget>

namespace MantidQt {
namespace MantidWidgets {
class PreviewPlot;

/**
 * Displays several workpaces on a matplotlib figure
 */
class EXPORT_OPT_MANTIDQT_PLOTTING RangeSelector : public QObject {
  Q_OBJECT

public:
  enum SelectType { XMINMAX, XSINGLE, YMINMAX, YSINGLE };

  RangeSelector(PreviewPlot *plot, SelectType type = XMINMAX,
                bool visible = true, bool infoOnly = false,
                const QColor &colour = Qt::black);

  /// convenience overload
  void setRange(const std::pair<double, double> &range);

  double getMinimum();
  double getMaximum();

signals:
  void selectionChanged(double min, double max);

public slots:
  void setRange(const double min, const double max);
  void setMinimum(double value);
  void setMaximum(double value);
  void detach();
  void setColour(QColor colour);

private slots:
  void handleMouseDown(const QPoint &point);
  void handleMouseMove(const QPoint &point);
  void handleMouseUp(const QPoint &point);

  void redrawMarkers();

private:
  void updateMinMax(const double x, bool minMoved, bool maxMoved);
  void updateCursor();

  /// The preview plot containing the range selector
  PreviewPlot *m_plot;
  /// Type of selection
  SelectType m_type;
  /// The minimum and maximum limits for the range selector
  std::pair<double, double> m_limits;
  /// Current position of the line marking the minimum
  double m_minimum;
  /// Current position of the line marking the maximum
  double m_maximum;
  /// The minimum marker
  std::unique_ptr<MantidQt::Widgets::MplCpp::VerticalMarker> m_minMarker;

  /// The maximum marker
  std::unique_ptr<MantidQt::Widgets::MplCpp::VerticalMarker> m_maxMarker;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQT_PLOTTING_MPL_RANGESELECTOR_H_
