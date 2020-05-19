// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/SliceViewerCoordConversion.h"

namespace MantidQt {
namespace MantidWidgets {
SliceViewerCoordConversion::SliceViewerCoordConversion()
    : m_xAxis(state::X), m_yAxis(state::Y) {}

std::tuple<double, double>
SliceViewerCoordConversion::toDataCoord(const double xDisplayCoord,
                                        const double yDisplayCoord) const {
  if (m_xAxis == state::Y) {
    return std::make_tuple(yDisplayCoord, xDisplayCoord);
  } else {
    return std::make_tuple(xDisplayCoord, yDisplayCoord);
  }
}

void SliceViewerCoordConversion::changeDimensions(const int xAxisState,
                                                  const int yAxisState) {
  if ((xAxisState == state::X && yAxisState == state::Y) ||
      (xAxisState == state::Y && yAxisState == state::X)) {
    m_xAxis = static_cast<state>(xAxisState);
    m_yAxis = static_cast<state>(yAxisState);
  }
}
} // namespace MantidWidgets
} // namespace MantidQt
