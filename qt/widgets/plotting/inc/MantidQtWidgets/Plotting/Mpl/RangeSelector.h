// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_PLOTTING_MPL_RANGESELECTOR_H_
#define MANTIDQT_PLOTTING_MPL_RANGESELECTOR_H_

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
                bool visible = true, bool infoOnly = false);

  /// convenience overload
  void setRange(const std::pair<double, double> &range);

  double getMinimum();
  double getMaximum();

public slots:
  void setRange(const double min, const double max);
  void setMinimum(double value);
  void setMaximum(double value);
  void detach();
  void setColour(QColor colour);

private:
  /// Type of selection
  SelectType m_type;
  /// Current position of the line marking the minimum
  double m_min;
  /// Current position of the line marking the maximum
  double m_max;
  /// Lowest allowed position of the lines
  double m_lower;
  /// Highest allowed position of the lines
  double m_higher;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQT_PLOTTING_MPL_RANGESELECTOR_H_
