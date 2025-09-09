// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Plotting/AxisID.h"
#include "MantidQtWidgets/Plotting/DllOption.h"

#include <string>
#include <vector>

// Forward Declarations
class QString;
class QVariant;
template <typename T1, typename T2> class QHash;

namespace MantidQt::MantidWidgets {
class EXPORT_OPT_MANTIDQT_PLOTTING IPlotView {
public:
  virtual void setScaleLinear(const AxisID axisID) = 0;
  virtual void setScaleLog(const AxisID axisID) = 0;
  virtual void setScaleSymLog(const AxisID axisID, const double lintresh) = 0;
  virtual void setScale(const AxisID axisID, const std::string &scale,
                        const QHash<QString, QVariant> &additionalProperties) = 0;
  virtual void plot(const std::vector<Mantid::API::MatrixWorkspace_sptr> &workspaces,
                    const std::vector<int> &workspaceIndices, const bool plotErrorBars) = 0;
  virtual void setAxisLimit(const AxisID axisID, const double axMin, const double axMax) = 0;
};
} // namespace MantidQt::MantidWidgets
