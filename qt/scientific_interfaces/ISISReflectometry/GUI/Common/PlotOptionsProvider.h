// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IPlotOptionsProvider.h"

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/// Converts plotting-tab output selections into concrete plot options.
class MANTIDQT_ISISREFLECTOMETRY_DLL PlotOptionsProvider : public IPlotOptionsProvider {
public:
  /// Return reflectometry plot output types available for the selected instrument.
  std::vector<PlotOutputType> availableTypes(std::string const &instrumentName) const override;
  /// Return default plot options for an output type and layout.
  PlotOptions optionsFor(PlotOutputType outputType, PlotLayout layout) const override;
  /// Return plot options using any selected detector-map or alignment axes.
  PlotOptions optionsFor(PlotOutputSelection const &outputSelection, PlotLayout layout) const override;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
