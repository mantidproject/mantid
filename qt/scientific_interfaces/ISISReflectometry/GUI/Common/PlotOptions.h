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

class QWidget;

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/// The scientific output type selected in the plotting tab.
enum class PlotOutputType { ReflectivityCurve, DetectorMap, SpinAsymmetry, Alignment };

/// The matplotlib rendering style used for the selected output.
enum class PlotStyle { Line, Colorfill };

/// The relationship between newly plotted workspaces.
enum class PlotLayout { Individual, Overplot, Tiled };

/// The scale applied to a matplotlib axis.
enum class AxisScale { Linear, Log };

/// The horizontal axis available when generating detector map workspaces.
enum class DetectorMapXAxis { TimeOfFlight, Lambda };

/// The vertical axis available when generating detector map workspaces.
enum class DetectorMapYAxis { DetectorId, Theta };

/// The horizontal axis available when generating alignment plot workspaces.
enum class AlignmentXAxis { DetectorId, Theta };

/// User-facing plot output selection collected from the plotting tab controls.
struct MANTIDQT_ISISREFLECTOMETRY_DLL PlotOutputSelection {
  PlotOutputSelection() = default;
  explicit PlotOutputSelection(PlotOutputType outputType) : outputType(outputType) {}
  PlotOutputSelection(PlotOutputType outputType, DetectorMapXAxis detectorMapXAxis, DetectorMapYAxis detectorMapYAxis)
      : outputType(outputType), detectorMapXAxis(detectorMapXAxis), detectorMapYAxis(detectorMapYAxis) {}
  PlotOutputSelection(PlotOutputType outputType, DetectorMapXAxis detectorMapXAxis, DetectorMapYAxis detectorMapYAxis,
                      AlignmentXAxis alignmentXAxis)
      : outputType(outputType), detectorMapXAxis(detectorMapXAxis), detectorMapYAxis(detectorMapYAxis),
        alignmentXAxis(alignmentXAxis) {}

  PlotOutputType outputType{PlotOutputType::ReflectivityCurve};
  DetectorMapXAxis detectorMapXAxis{DetectorMapXAxis::TimeOfFlight};
  DetectorMapYAxis detectorMapYAxis{DetectorMapYAxis::DetectorId};
  AlignmentXAxis alignmentXAxis{AlignmentXAxis::DetectorId};
  std::string instrumentName;
};

/// Label, unit and scale metadata for one matplotlib axis.
struct MANTIDQT_ISISREFLECTOMETRY_DLL PlotAxis {
  std::string label;
  std::string unit;
  AxisScale scale{AxisScale::Linear};
};

/// Complete matplotlib options derived from a plot output selection.
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

/// Workspaces and display options to pass from presenters to the plotter.
struct MANTIDQT_ISISREFLECTOMETRY_DLL PlotRequest {
  std::vector<std::string> workspaces;
  PlotOptions options;
  QWidget *parentWidget{nullptr};
  bool addToExistingPlot{false};
  bool tiledVertically{false};
};

/// Create plot options for a reflectivity-curve style output.
MANTIDQT_ISISREFLECTOMETRY_DLL PlotOptions reflectivityCurvePlotOptions(PlotOutputType outputType, PlotLayout layout);
/// Create plot options for a detector map generated from the selected axes.
MANTIDQT_ISISREFLECTOMETRY_DLL PlotOptions detectorMapPlotOptions(DetectorMapXAxis xAxis, DetectorMapYAxis yAxis,
                                                                  PlotLayout layout);
/// Create plot options for a spin-asymmetry plot.
MANTIDQT_ISISREFLECTOMETRY_DLL PlotOptions spinAsymmetryPlotOptions(PlotLayout layout);
/// Create plot options for an alignment plot generated from the selected axis.
MANTIDQT_ISISREFLECTOMETRY_DLL PlotOptions alignmentPlotOptions(AlignmentXAxis xAxis, PlotLayout layout);

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
