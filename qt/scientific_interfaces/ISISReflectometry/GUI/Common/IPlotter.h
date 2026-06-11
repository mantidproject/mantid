// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "PlotOptions.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/// Interface for plotting workspaces requested by the ISIS Reflectometry GUI.
class MANTIDQT_ISISREFLECTOMETRY_DLL IPlotter {
public:
  virtual ~IPlotter() = default;
  /// Plot the requested workspaces according to the supplied output options.
  virtual void plot(PlotRequest const &request) const = 0;
  /// Return true if matplotlib currently has an active figure.
  virtual bool hasActiveFigure() const = 0;
  /// Return true if the active figure can accept an overplotted line plot.
  virtual bool canOverplotActiveFigure() const = 0;
};

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
