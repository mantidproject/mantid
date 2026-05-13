// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IPlotOptionsProvider.h"

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL PlotOptionsProvider : public IPlotOptionsProvider {
public:
  std::vector<PlotOutputType> availableTypes(std::string const &instrumentName) const override;
  PlotOptions optionsFor(PlotOutputType outputType, PlotLayout layout) const override;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
