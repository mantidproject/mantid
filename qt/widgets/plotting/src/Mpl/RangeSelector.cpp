// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/Mpl/RangeSelector.h"
#include "MantidQtWidgets/Plotting/Mpl/PreviewPlot.h"

namespace MantidQt {
namespace MantidWidgets {

RangeSelector::RangeSelector(PreviewPlot *plot, SelectType, bool visible,
                             bool infoOnly)
    : QObject() {
  Q_UNUSED(plot);
  Q_UNUSED(visible);
  Q_UNUSED(infoOnly);
}

void RangeSelector::setRange(const std::pair<double, double> &range) {
  setRange(range.first, range.second);
}

void RangeSelector::setRange(const double min, const double max) {
  Q_UNUSED(min);
  Q_UNUSED(max);
  throw std::runtime_error("RangeSelector::setRange not implemented");
}

void RangeSelector::setMinimum(double value) {
  Q_UNUSED(value);
  throw std::runtime_error("RangeSelector::setMinimum not implemented");
}

void RangeSelector::setMaximum(double value) {
  Q_UNUSED(value);
  throw std::runtime_error("RangeSelector::setMaximum not implemented");
}

} // namespace MantidWidgets
} // namespace MantidQt
