// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "PlotOptions.h"

#include <stdexcept>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
PlotAxis detectorMapXAxis(DetectorMapXAxis axis) {
  switch (axis) {
  case DetectorMapXAxis::TimeOfFlight:
    return PlotAxis{"Time of Flight", "", AxisScale::Linear};
  case DetectorMapXAxis::Lambda:
    return PlotAxis{"Lambda", "", AxisScale::Linear};
  }
  throw std::runtime_error("Unexpected detector map x axis.");
}

PlotAxis detectorMapYAxis(DetectorMapYAxis axis) {
  switch (axis) {
  case DetectorMapYAxis::DetectorId:
    return PlotAxis{"Detector ID", "", AxisScale::Linear};
  case DetectorMapYAxis::Theta:
    return PlotAxis{"Theta", "", AxisScale::Linear};
  }
  throw std::runtime_error("Unexpected detector map y axis.");
}

PlotAxis alignmentXAxis(AlignmentXAxis axis) {
  switch (axis) {
  case AlignmentXAxis::DetectorId:
    return PlotAxis{"Detector ID", "", AxisScale::Linear};
  case AlignmentXAxis::Theta:
    return PlotAxis{"Theta", "", AxisScale::Linear};
  }
  throw std::runtime_error("Unexpected alignment x axis.");
}
} // namespace

PlotOptions reflectivityCurvePlotOptions(PlotOutputType outputType, PlotLayout layout) {
  auto options = PlotOptions{};
  options.outputType = outputType;
  options.plotStyle = PlotStyle::Line;
  options.layout = layout;
  options.xAxis = PlotAxis{"Q", "", AxisScale::Log};
  options.yAxis = PlotAxis{"I", "", AxisScale::Log};
  options.showErrors = true;
  options.windowTitle = "ISIS Reflectometry Plot";
  return options;
}

PlotOptions detectorMapPlotOptions(DetectorMapXAxis xAxis, DetectorMapYAxis yAxis, PlotLayout layout) {
  auto options = PlotOptions{};
  options.outputType = PlotOutputType::DetectorMap;
  options.plotStyle = PlotStyle::Colorfill;
  options.layout = layout;
  options.xAxis = detectorMapXAxis(xAxis);
  options.yAxis = detectorMapYAxis(yAxis);
  options.zAxis = PlotAxis{"Intensity", "", AxisScale::Linear};
  options.windowTitle = "ISIS Reflectometry Detector Map";
  return options;
}

PlotOptions spinAsymmetryPlotOptions(PlotLayout layout) {
  auto options = PlotOptions{};
  options.outputType = PlotOutputType::SpinAsymmetry;
  options.plotStyle = PlotStyle::Line;
  options.layout = layout;
  options.xAxis = PlotAxis{"Qz", "", AxisScale::Linear};
  options.yAxis = PlotAxis{"Spin Asymmetry", "", AxisScale::Linear};
  options.showErrors = true;
  options.horizontalMarker = 0.0;
  options.windowTitle = "ISIS Reflectometry Spin Asymmetry";
  return options;
}

PlotOptions alignmentPlotOptions(AlignmentXAxis xAxis, PlotLayout layout) {
  auto options = PlotOptions{};
  options.outputType = PlotOutputType::Alignment;
  options.plotStyle = PlotStyle::Line;
  options.layout = layout;
  options.xAxis = alignmentXAxis(xAxis);
  options.yAxis = PlotAxis{"Integrated Intensity", "", AxisScale::Linear};
  options.showErrors = false;
  options.windowTitle = "ISIS Reflectometry Alignment";
  return options;
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
