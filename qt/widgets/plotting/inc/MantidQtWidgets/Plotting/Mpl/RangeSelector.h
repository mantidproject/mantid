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

public slots:
  void setRange(const double min, const double max);
  void setMinimum(double value);
  void setMaximum(double value);
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQT_PLOTTING_MPL_RANGESELECTOR_H_
