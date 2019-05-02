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
    : QObject() {}

void RangeSelector::setRange(const std::pair<double, double> &range) {
  setRange(range.first, range.second);
}

void RangeSelector::setRange(const double min, const double max) {
  throw std::runtime_error("RangeSelector::setRange not implemented");
}

void RangeSelector::setMinimum(double value) {
  throw std::runtime_error("RangeSelector::setMinimum not implemented");
}

void RangeSelector::setMaximum(double value) {
  throw std::runtime_error("RangeSelector::setMaximum not implemented");
}

} // namespace MantidWidgets
} // namespace MantidQt
