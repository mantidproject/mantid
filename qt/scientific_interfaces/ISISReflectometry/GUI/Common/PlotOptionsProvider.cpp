// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "PlotOptionsProvider.h"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
bool supportsInstrumentSpecificPlotTypes(std::string const &instrumentName) {
  auto const supportedInstruments = std::array<std::string, 3>{"POLREF", "OFFSPEC", "CRISP"};
  return std::find(supportedInstruments.cbegin(), supportedInstruments.cend(), instrumentName) !=
         supportedInstruments.cend();
}
} // namespace

std::vector<PlotOutputType> PlotOptionsProvider::availableTypes(std::string const &instrumentName) const {
  if (supportsInstrumentSpecificPlotTypes(instrumentName)) {
    return {PlotOutputType::ReflectivityCurve, PlotOutputType::DetectorMap, PlotOutputType::SpinAsymmetry,
            PlotOutputType::Alignment};
  }
  return {PlotOutputType::ReflectivityCurve};
}

PlotOptions PlotOptionsProvider::optionsFor(PlotOutputType outputType, PlotLayout layout) const {
  return optionsFor(PlotOutputSelection{outputType}, layout);
}

PlotOptions PlotOptionsProvider::optionsFor(PlotOutputSelection const &outputSelection, PlotLayout layout) const {
  auto const outputType = outputSelection.outputType;
  switch (outputType) {
  case PlotOutputType::DetectorMap:
    return detectorMapPlotOptions(outputSelection.detectorMapXAxis, outputSelection.detectorMapYAxis, layout);
  case PlotOutputType::SpinAsymmetry:
    return spinAsymmetryPlotOptions(layout);
  case PlotOutputType::Alignment:
    return alignmentPlotOptions(outputSelection.alignmentXAxis, layout);
  case PlotOutputType::ReflectivityCurve:
    return reflectivityCurvePlotOptions(outputType, layout);
  }
  throw std::runtime_error("Unexpected reflectometry plot output type.");
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
