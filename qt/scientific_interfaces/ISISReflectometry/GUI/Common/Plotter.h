// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "IPlotter.h"

#include <QtGlobal>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/// Python-backed plotter for ISIS Reflectometry plotting-tab requests.
class MANTIDQT_ISISREFLECTOMETRY_DLL Plotter : public IPlotter {
public:
  /// Plot the requested workspaces and apply post-plot labels, markers and parent window state.
  void plot(PlotRequest const &request) const override;
  /// Return true if matplotlib currently has an active figure.
  bool hasActiveFigure() const override;
  /// Return true if the active figure can accept an overplotted line plot.
  bool canOverplotActiveFigure() const override;
};

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
