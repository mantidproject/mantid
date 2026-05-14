// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "PlotOptions.h"

namespace MantidQt::CustomInterfaces::ISISReflectometry {

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

PlotOptions detectorMapPlotOptions(PlotLayout layout) {
  auto options = PlotOptions{};
  options.outputType = PlotOutputType::DetectorMap;
  options.plotStyle = PlotStyle::Colorfill;
  options.layout = layout;
  options.xAxis = PlotAxis{"Time of Flight", "", AxisScale::Linear};
  options.yAxis = PlotAxis{"Detector ID", "", AxisScale::Linear};
  options.windowTitle = "ISIS Reflectometry Detector Map";
  return options;
}

PlotOptions spinAsymmetryPlotOptions(PlotLayout layout) {
  auto options = PlotOptions{};
  options.outputType = PlotOutputType::SpinAsymmetry;
  options.plotStyle = PlotStyle::Line;
  options.layout = layout;
  options.xAxis = PlotAxis{"Qz", "", AxisScale::Linear};
  options.yAxis = PlotAxis{"", "%", AxisScale::Linear};
  options.showErrors = true;
  options.windowTitle = "ISIS Reflectometry Spin Asymmetry";
  return options;
}

PlotOptions alignmentPlotOptions(PlotLayout layout) {
  auto options = PlotOptions{};
  options.outputType = PlotOutputType::Alignment;
  options.plotStyle = PlotStyle::Line;
  options.layout = layout;
  options.xAxis = PlotAxis{"Detector ID", "", AxisScale::Linear};
  options.yAxis = PlotAxis{"Integrated Intensity", "", AxisScale::Linear};
  options.showErrors = true;
  options.windowTitle = "ISIS Reflectometry Alignment";
  return options;
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
