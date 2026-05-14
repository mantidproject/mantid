// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"

#include <string>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

enum class PlotOutputType { ReflectivityCurve, DetectorMap, SpinAsymmetry, Alignment };

enum class PlotStyle { Line, Colorfill };

enum class PlotLayout { Individual, Overplot, Tiled };

enum class AxisScale { Linear, Log };

struct MANTIDQT_ISISREFLECTOMETRY_DLL PlotAxis {
  std::string label;
  std::string unit;
  AxisScale scale{AxisScale::Linear};
};

struct MANTIDQT_ISISREFLECTOMETRY_DLL PlotOptions {
  PlotOutputType outputType{PlotOutputType::ReflectivityCurve};
  PlotStyle plotStyle{PlotStyle::Line};
  PlotLayout layout{PlotLayout::Individual};
  PlotAxis xAxis;
  PlotAxis yAxis;
  bool showErrors{false};
  std::string windowTitle;
};

struct MANTIDQT_ISISREFLECTOMETRY_DLL PlotRequest {
  std::vector<std::string> workspaces;
  PlotOptions options;
};

inline bool operator==(PlotAxis const &lhs, PlotAxis const &rhs) {
  return lhs.label == rhs.label && lhs.unit == rhs.unit && lhs.scale == rhs.scale;
}

inline bool operator!=(PlotAxis const &lhs, PlotAxis const &rhs) { return !(lhs == rhs); }

inline bool operator==(PlotOptions const &lhs, PlotOptions const &rhs) {
  return lhs.outputType == rhs.outputType && lhs.plotStyle == rhs.plotStyle && lhs.layout == rhs.layout &&
         lhs.xAxis == rhs.xAxis && lhs.yAxis == rhs.yAxis && lhs.showErrors == rhs.showErrors &&
         lhs.windowTitle == rhs.windowTitle;
}

inline bool operator!=(PlotOptions const &lhs, PlotOptions const &rhs) { return !(lhs == rhs); }

inline bool operator==(PlotRequest const &lhs, PlotRequest const &rhs) {
  return lhs.workspaces == rhs.workspaces && lhs.options == rhs.options;
}

inline bool operator!=(PlotRequest const &lhs, PlotRequest const &rhs) { return !(lhs == rhs); }

MANTIDQT_ISISREFLECTOMETRY_DLL PlotOptions reflectivityCurvePlotOptions(PlotOutputType outputType, PlotLayout layout);
MANTIDQT_ISISREFLECTOMETRY_DLL PlotOptions detectorMapPlotOptions(PlotLayout layout);
MANTIDQT_ISISREFLECTOMETRY_DLL PlotOptions spinAsymmetryPlotOptions(PlotLayout layout);
MANTIDQT_ISISREFLECTOMETRY_DLL PlotOptions alignmentPlotOptions(PlotLayout layout);

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
