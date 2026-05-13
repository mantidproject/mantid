// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "PlotOptionsProvider.h"

namespace MantidQt::CustomInterfaces::ISISReflectometry {

std::vector<PlotOutputType> PlotOptionsProvider::availableTypes(std::string const &instrumentName) const {
  (void)instrumentName;
  return {PlotOutputType::ReflectivityCurve};
}

PlotOptions PlotOptionsProvider::optionsFor(PlotOutputType outputType, PlotLayout layout) const {
  return reflectivityCurvePlotOptions(outputType, layout);
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
