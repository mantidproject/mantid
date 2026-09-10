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

std::vector<PlottingWorkspaceTreeItemType> allSelectableItemTypes() {
  return {PlottingWorkspaceTreeItemType::ReductionGroup, PlottingWorkspaceTreeItemType::Run,
          PlottingWorkspaceTreeItemType::WorkspaceGroup, PlottingWorkspaceTreeItemType::Workspace};
}

std::vector<ReducedWorkspaceOutputType> allReducedWorkspaceOutputTypes() {
  return {ReducedWorkspaceOutputType::IvsQ, ReducedWorkspaceOutputType::IvsLambda,
          ReducedWorkspaceOutputType::IvsQBinned};
}

std::vector<PlottingWorkspaceTreeItemType> groupOrRunItemTypes() {
  return {PlottingWorkspaceTreeItemType::ReductionGroup, PlottingWorkspaceTreeItemType::Run,
          PlottingWorkspaceTreeItemType::WorkspaceGroup};
}

std::vector<ReducedWorkspaceOutputType> reflectivityReducedWorkspaceOutputTypes() {
  return {ReducedWorkspaceOutputType::IvsQ, ReducedWorkspaceOutputType::IvsQBinned};
}

const std::unordered_map<PlotOutputType, std::string> plotOutputTypeDisplayNames{
    {PlotOutputType::ReflectivityCurve, "Reflectivity Curve"},
    {PlotOutputType::DetectorMap, "Detector Map"},
    {PlotOutputType::SpinAsymmetry, "Spin Asymmetry"},
    {PlotOutputType::Alignment, "Alignment"},
};

PlotOutputTypeProperties const reflectivityCurveProperties{PlotOutputType::ReflectivityCurve,
                                                           allSelectableItemTypes(),
                                                           reflectivityReducedWorkspaceOutputTypes(),
                                                           {.supportsOverplot = true,
                                                            .supportsAddToExistingPlot = true,
                                                            .excludesPostprocessedGroupOutputs = false,
                                                            .requiresWorkspaceGroupsForMultiPlot = false}};

PlotOutputTypeProperties const detectorMapProperties{PlotOutputType::DetectorMap,
                                                     allSelectableItemTypes(),
                                                     allReducedWorkspaceOutputTypes(),
                                                     {.supportsOverplot = false,
                                                      .supportsAddToExistingPlot = false,
                                                      .excludesPostprocessedGroupOutputs = true,
                                                      .requiresWorkspaceGroupsForMultiPlot = false}};

PlotOutputTypeProperties const spinAsymmetryProperties{PlotOutputType::SpinAsymmetry,
                                                       groupOrRunItemTypes(),
                                                       {ReducedWorkspaceOutputType::IvsQBinned},
                                                       {.supportsOverplot = true,
                                                        .supportsAddToExistingPlot = true,
                                                        .excludesPostprocessedGroupOutputs = false,
                                                        .requiresWorkspaceGroupsForMultiPlot = true}};

PlotOutputTypeProperties const alignmentProperties{PlotOutputType::Alignment,
                                                   allSelectableItemTypes(),
                                                   allReducedWorkspaceOutputTypes(),
                                                   {.supportsOverplot = true,
                                                    .supportsAddToExistingPlot = true,
                                                    .excludesPostprocessedGroupOutputs = true,
                                                    .requiresWorkspaceGroupsForMultiPlot = false}};
} // namespace

PlotOutputTypeProperties::PlotOutputTypeProperties(
    PlotOutputType plotOutputType, std::vector<PlottingWorkspaceTreeItemType> selectableItemTypes,
    std::vector<ReducedWorkspaceOutputType> includedReducedWorkspaceOutputTypes,
    PlotOutputTypeCapabilities capabilities)
    : m_plotOutputType(plotOutputType), m_selectableItemTypes(std::move(selectableItemTypes)),
      m_includedReducedWorkspaceOutputTypes(std::move(includedReducedWorkspaceOutputTypes)),
      m_capabilities(capabilities) {}

std::string const &PlotOutputTypeProperties::displayName() const {
  return plotOutputTypeDisplayNames.at(m_plotOutputType);
}

bool PlotOutputTypeProperties::allowsItemType(PlottingWorkspaceTreeItemType itemType) const {
  return contains(m_selectableItemTypes, itemType);
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
  case PlotOutputType::Alignment:
    return alignmentProperties;
  }
  throw std::runtime_error("Unexpected reflectometry plot output type.");
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
