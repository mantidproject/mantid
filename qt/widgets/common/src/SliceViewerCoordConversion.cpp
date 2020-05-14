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

std::vector<double>
SliceViewerCoordConversion::toDataCoord(const double xDisplayCoord,
                                        const double yDisplayCoord) const {
  std::vector<double> dataCoords;
  dataCoords.reserve(2);
  if (m_xAxis == state::Y) {
    dataCoords.emplace_back(yDisplayCoord);
    dataCoords.emplace_back(xDisplayCoord);
  } else {
    dataCoords.emplace_back(xDisplayCoord);
    dataCoords.emplace_back(yDisplayCoord);
  }
  return dataCoords;
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
