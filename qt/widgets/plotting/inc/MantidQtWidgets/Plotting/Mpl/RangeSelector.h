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

  void setRange(const std::pair<double, double> &range);
  std::pair<double, double> getRange() const;

signals:
  void selectionChanged(double min, double max);

public slots:
  void setRange(const double min, const double max);
  void detach();
  void setColour(QColor colour);

private slots:
  void handleMouseDown(const QPoint &point);
  void handleMouseMove(const QPoint &point);
  void handleMouseUp(const QPoint &point);

  void redrawMarker();

private:
  /// The preview plot containing the range selector
  PreviewPlot *m_plot;
  /// The minimum marker
  std::unique_ptr<MantidQt::Widgets::MplCpp::RangeMarker> m_rangeMarker;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQT_PLOTTING_MPL_RANGESELECTOR_H_
