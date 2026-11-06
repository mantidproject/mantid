// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "PlotOutputTypeProperties.h"

#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
template <typename T> bool contains(std::vector<T> const &values, T value) {
  return std::find(values.cbegin(), values.cend(), value) != values.cend();
}

std::vector<ReducedWorkspaceOutputType> allReducedWorkspaceOutputTypes() {
  return {ReducedWorkspaceOutputType::IvsQ, ReducedWorkspaceOutputType::IvsLambda,
          ReducedWorkspaceOutputType::IvsQBinned};
}

std::vector<ReducedWorkspaceOutputType> reflectivityReducedWorkspaceOutputTypes() {
  return {ReducedWorkspaceOutputType::IvsQ, ReducedWorkspaceOutputType::IvsQBinned};
}

const std::unordered_map<PlotOutputType, std::string> plotOutputTypeDisplayNames{
    {PlotOutputType::ReflectivityCurve, "Reflectivity Curve"},
    {PlotOutputType::DetectorMap, "Detector Map"},
    {PlotOutputType::SpinAsymmetry, "Spin Asymmetry"},
    {PlotOutputType::Alignment, "Alignment"},
    {PlotOutputType::ReflectivityAndSpinAsymmetry, "Reflectivity + Spin Asymmetry"},
};

PlotOutputTypeProperties const reflectivityCurveProperties{PlotOutputType::ReflectivityCurve,
                                                           PlotSelectionScope::Workspace,
                                                           reflectivityReducedWorkspaceOutputTypes(),
                                                           {.supportsOverplot = true,
                                                            .supportsAddToExistingPlot = true,
                                                            .excludesPostprocessedGroupOutputs = false,
                                                            .requiresWorkspaceGroupsForMultiPlot = false}};

PlotOutputTypeProperties const detectorMapProperties{PlotOutputType::DetectorMap,
                                                     PlotSelectionScope::Workspace,
                                                     allReducedWorkspaceOutputTypes(),
                                                     {.supportsOverplot = false,
                                                      .supportsAddToExistingPlot = false,
                                                      .excludesPostprocessedGroupOutputs = true,
                                                      .requiresWorkspaceGroupsForMultiPlot = false}};

PlotOutputTypeProperties spinAsymmetryPropertiesFor(PlotOutputType outputType,
                                                    std::vector<PlotRequestDefinition> plotRequests = {},
                                                    bool splitByScope = false) {
  return PlotOutputTypeProperties{outputType,
                                  PlotSelectionScope::WorkspaceGroup,
                                  {ReducedWorkspaceOutputType::IvsQBinned},
                                  {.supportsOverplot = true,
                                   .supportsAddToExistingPlot = true,
                                   .excludesPostprocessedGroupOutputs = false,
                                   .requiresWorkspaceGroupsForMultiPlot = true,
                                   .splitPlotRequestByPlotSelectionScope = splitByScope},
                                  std::move(plotRequests)};
}

auto const spinAsymmetryProperties = spinAsymmetryPropertiesFor(PlotOutputType::SpinAsymmetry);
auto const reflectivityAndSpinAsymmetryProperties = spinAsymmetryPropertiesFor(
    PlotOutputType::ReflectivityAndSpinAsymmetry,
    {{PlotOutputType::ReflectivityCurve, PlotRequestSettings{.layout = PlotLayout::Overplot}},
     {PlotOutputType::SpinAsymmetry, PlotRequestSettings{.layout = PlotLayout::Tiled,
                                                         .addToExistingPlot = true,
                                                         .tiledVertically = true,
                                                         .shareXAxisWhenTiled = true}}},
    true);

PlotOutputTypeProperties const alignmentProperties{PlotOutputType::Alignment,
                                                   PlotSelectionScope::Workspace,
                                                   allReducedWorkspaceOutputTypes(),
                                                   {.supportsOverplot = true,
                                                    .supportsAddToExistingPlot = true,
                                                    .excludesPostprocessedGroupOutputs = true,
                                                    .requiresWorkspaceGroupsForMultiPlot = false}};
} // namespace

PlotOutputTypeProperties::PlotOutputTypeProperties(
    PlotOutputType plotOutputType, PlotSelectionScope selectionScope,
    std::vector<ReducedWorkspaceOutputType> includedReducedWorkspaceOutputTypes,
    PlotOutputTypeCapabilities capabilities, std::vector<PlotRequestDefinition> plotRequests)
    : m_plotOutputType(plotOutputType), m_selectionScope(selectionScope),
      m_includedReducedWorkspaceOutputTypes(std::move(includedReducedWorkspaceOutputTypes)),
      m_capabilities(capabilities),
      m_plotRequests(plotRequests.empty() ? std::vector<PlotRequestDefinition>{{plotOutputType}}
                                          : std::move(plotRequests)) {}

std::vector<PlotRequestDefinition> const &PlotOutputTypeProperties::plotRequests() const { return m_plotRequests; }

PlotSelectionScope PlotOutputTypeProperties::selectionScope() const { return m_selectionScope; }

bool PlotOutputTypeProperties::splitPlotRequestByPlotSelectionScope() const {
  return m_capabilities.splitPlotRequestByPlotSelectionScope;
}

std::string const &PlotOutputTypeProperties::displayName() const {
  return plotOutputTypeDisplayNames.at(m_plotOutputType);
}

bool PlotOutputTypeProperties::allowsItemType(PlottingWorkspaceTreeItemType itemType) const {
  return itemType <= m_selectionScope;
}

bool PlotOutputTypeProperties::includesReducedWorkspaceOutput(ReducedWorkspaceOutputType outputType) const {
  return contains(m_includedReducedWorkspaceOutputTypes, outputType);
}

bool PlotOutputTypeProperties::supportsOverplot() const { return m_capabilities.supportsOverplot; }

bool PlotOutputTypeProperties::supportsAddToExistingPlot() const { return m_capabilities.supportsAddToExistingPlot; }

bool PlotOutputTypeProperties::excludesPostprocessedGroupOutputs() const {
  return m_capabilities.excludesPostprocessedGroupOutputs;
}

bool PlotOutputTypeProperties::requiresWorkspaceGroupsForMultiPlot() const {
  return m_capabilities.requiresWorkspaceGroupsForMultiPlot;
}

bool PlotOutputTypeProperties::showsDetectorMapProperties() const {
  return m_plotOutputType == PlotOutputType::DetectorMap;
}

bool PlotOutputTypeProperties::showsAlignmentProperties() const {
  return m_plotOutputType == PlotOutputType::Alignment;
}

bool PlotOutputTypeProperties::showsPlotProperties() const {
  return showsDetectorMapProperties() || showsAlignmentProperties();
}

PlotOutputTypeProperties const &plotOutputTypeProperties(PlotOutputType outputType) {
  switch (outputType) {
  case PlotOutputType::ReflectivityCurve:
    return reflectivityCurveProperties;
  case PlotOutputType::DetectorMap:
    return detectorMapProperties;
  case PlotOutputType::SpinAsymmetry:
    return spinAsymmetryProperties;
  case PlotOutputType::ReflectivityAndSpinAsymmetry:
    return reflectivityAndSpinAsymmetryProperties;
  case PlotOutputType::Alignment:
    return alignmentProperties;
  }
  throw std::runtime_error("Unexpected reflectometry plot output type.");
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
