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

class MANTIDQT_ISISREFLECTOMETRY_DLL IPlotOptionsProvider {
public:
  virtual ~IPlotOptionsProvider() = default;
  virtual std::vector<PlotOutputType> availableTypes(std::string const &instrumentName) const = 0;
  virtual PlotOptions optionsFor(PlotOutputType outputType, PlotLayout layout) const = 0;
  virtual PlotOptions optionsFor(PlotOutputOptions const &outputOptions, PlotLayout layout) const = 0;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
