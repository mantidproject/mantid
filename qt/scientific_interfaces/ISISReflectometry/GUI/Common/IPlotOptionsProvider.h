// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "PlotOptions.h"

#include <string>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/// Supplies plot output types and rendering options for the plotting tab.
class MANTIDQT_ISISREFLECTOMETRY_DLL IPlotOptionsProvider {
public:
  virtual ~IPlotOptionsProvider() = default;
  /// Return the output types available for the selected instrument.
  virtual std::vector<PlotOutputType> availableTypes(std::string const &instrumentName) const = 0;
  /// Return plot options for output types that do not need extra axis selections.
  virtual PlotOptions optionsFor(PlotOutputType outputType, PlotLayout layout) const = 0;
  /// Return plot options for a full GUI output selection.
  virtual PlotOptions optionsFor(PlotOutputSelection const &outputSelection, PlotLayout layout) const = 0;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
