// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Plotting/AxisID.h"
#include "MantidQtWidgets/Plotting/DllOption.h"

#include <vector>

namespace MantidQt::MantidWidgets {
class EXPORT_OPT_MANTIDQT_PLOTTING IPlotView {
public:
  void setScaleLinear(const AxisID axisID);
  void setScaleLog(const AxisID axisID);
  void plot(const std::vector<Mantid::API::MatrixWorkspace_sptr> &workspaces, const std::vector<int> &workspaceIndices);
};
} // namespace MantidQt::MantidWidgets