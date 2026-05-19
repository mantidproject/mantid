// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"

#include <optional>
#include <string>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

enum class PlotOutputType { ReflectivityCurve, DetectorMap, SpinAsymmetry, Alignment };

enum class PlotStyle { Line, Colorfill };

enum class PlotLayout { Individual, Overplot, Tiled };

enum class AxisScale { Linear, Log };

enum class DetectorMapXAxis { TimeOfFlight, Lambda };

enum class DetectorMapYAxis { DetectorId, Theta };

enum class AlignmentXAxis { DetectorId, Theta };

struct MANTIDQT_ISISREFLECTOMETRY_DLL PlotOutputOptions {
  PlotOutputOptions() = default;
  explicit PlotOutputOptions(PlotOutputType outputType) : outputType(outputType) {}
  PlotOutputOptions(PlotOutputType outputType, DetectorMapXAxis detectorMapXAxis, DetectorMapYAxis detectorMapYAxis)
      : outputType(outputType), detectorMapXAxis(detectorMapXAxis), detectorMapYAxis(detectorMapYAxis) {}
  PlotOutputOptions(PlotOutputType outputType, DetectorMapXAxis detectorMapXAxis, DetectorMapYAxis detectorMapYAxis,
                    AlignmentXAxis alignmentXAxis)
      : outputType(outputType), detectorMapXAxis(detectorMapXAxis), detectorMapYAxis(detectorMapYAxis),
        alignmentXAxis(alignmentXAxis) {}

  PlotOutputType outputType{PlotOutputType::ReflectivityCurve};
  DetectorMapXAxis detectorMapXAxis{DetectorMapXAxis::TimeOfFlight};
  DetectorMapYAxis detectorMapYAxis{DetectorMapYAxis::DetectorId};
  AlignmentXAxis alignmentXAxis{AlignmentXAxis::DetectorId};
  std::string instrumentName;
};

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
  PlotAxis zAxis;
  bool showErrors{false};
  std::optional<double> horizontalMarker;
  std::string windowTitle;
};

struct MANTIDQT_ISISREFLECTOMETRY_DLL PlotRequest {
  std::vector<std::string> workspaces;
  PlotOptions options;
};

MANTIDQT_ISISREFLECTOMETRY_DLL PlotOptions reflectivityCurvePlotOptions(PlotOutputType outputType, PlotLayout layout);
MANTIDQT_ISISREFLECTOMETRY_DLL PlotOptions detectorMapPlotOptions(DetectorMapXAxis xAxis, DetectorMapYAxis yAxis,
                                                                  PlotLayout layout);
MANTIDQT_ISISREFLECTOMETRY_DLL PlotOptions spinAsymmetryPlotOptions(PlotLayout layout);
MANTIDQT_ISISREFLECTOMETRY_DLL PlotOptions alignmentPlotOptions(AlignmentXAxis xAxis, PlotLayout layout);

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
