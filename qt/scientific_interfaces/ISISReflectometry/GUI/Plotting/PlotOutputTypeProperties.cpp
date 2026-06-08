// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "PlotOutputTypeProperties.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
template <typename T> bool contains(std::vector<T> const &values, T value) {
  return std::find(values.cbegin(), values.cend(), value) != values.cend();
}

std::vector<PlottingWorkspaceTreeItemType> allSelectableItemTypes() {
  return {PlottingWorkspaceTreeItemType::Group, PlottingWorkspaceTreeItemType::Run,
          PlottingWorkspaceTreeItemType::WorkspaceGroup, PlottingWorkspaceTreeItemType::Workspace};
}

std::vector<PlottingWorkspaceOutputType> allReflectometryWorkspaceOutputTypes() {
  return {PlottingWorkspaceOutputType::IvsQ, PlottingWorkspaceOutputType::IvsLambda,
          PlottingWorkspaceOutputType::IvsQBinned};
}

std::vector<PlottingWorkspaceTreeItemType> groupOrRunItemTypes() {
  return {PlottingWorkspaceTreeItemType::Group, PlottingWorkspaceTreeItemType::Run,
          PlottingWorkspaceTreeItemType::WorkspaceGroup};
}

std::vector<PlottingWorkspaceOutputType> reflectivityWorkspaceOutputTypes() {
  return {PlottingWorkspaceOutputType::IvsQ, PlottingWorkspaceOutputType::IvsQBinned};
}

PlotOutputTypeProperties const reflectivityCurveProperties{"Reflectivity Curve",
                                                           allSelectableItemTypes(),
                                                           reflectivityWorkspaceOutputTypes(),
                                                           {.supportsOverplot = true,
                                                            .supportsAddToExistingPlot = true,
                                                            .excludesPostprocessedGroupOutputs = false,
                                                            .requiresWorkspaceGroupsForMultiPlot = false},
                                                           {.detectorMap = false, .alignment = false}};

PlotOutputTypeProperties const detectorMapProperties{"Detector Map",
                                                     allSelectableItemTypes(),
                                                     allReflectometryWorkspaceOutputTypes(),
                                                     {.supportsOverplot = false,
                                                      .supportsAddToExistingPlot = false,
                                                      .excludesPostprocessedGroupOutputs = true,
                                                      .requiresWorkspaceGroupsForMultiPlot = false},
                                                     {.detectorMap = true, .alignment = false}};

PlotOutputTypeProperties const spinAsymmetryProperties{"Spin Asymmetry",
                                                       groupOrRunItemTypes(),
                                                       {PlottingWorkspaceOutputType::IvsQBinned},
                                                       {.supportsOverplot = true,
                                                        .supportsAddToExistingPlot = true,
                                                        .excludesPostprocessedGroupOutputs = false,
                                                        .requiresWorkspaceGroupsForMultiPlot = true},
                                                       {.detectorMap = false, .alignment = false}};

PlotOutputTypeProperties const alignmentProperties{"Alignment",
                                                   allSelectableItemTypes(),
                                                   allReflectometryWorkspaceOutputTypes(),
                                                   {.supportsOverplot = true,
                                                    .supportsAddToExistingPlot = true,
                                                    .excludesPostprocessedGroupOutputs = true,
                                                    .requiresWorkspaceGroupsForMultiPlot = false},
                                                   {.detectorMap = false, .alignment = true}};
} // namespace

PlotOutputTypeProperties::PlotOutputTypeProperties(
    QString displayName, std::vector<PlottingWorkspaceTreeItemType> selectableItemTypes,
    std::vector<PlottingWorkspaceOutputType> includedWorkspaceOutputTypes, PlotOutputTypeCapabilities capabilities,
    PlotOutputTypePropertyControls propertyControls)
    : m_displayName(std::move(displayName)), m_selectableItemTypes(std::move(selectableItemTypes)),
      m_includedWorkspaceOutputTypes(std::move(includedWorkspaceOutputTypes)), m_capabilities(capabilities),
      m_propertyControls(propertyControls) {}

QString const &PlotOutputTypeProperties::displayName() const { return m_displayName; }

bool PlotOutputTypeProperties::allowsItemType(PlottingWorkspaceTreeItemType itemType) const {
  return contains(m_selectableItemTypes, itemType);
}

bool PlotOutputTypeProperties::includesWorkspaceOutput(PlottingWorkspaceOutputType outputType) const {
  return contains(m_includedWorkspaceOutputTypes, outputType);
}

bool PlotOutputTypeProperties::supportsOverplot() const { return m_capabilities.supportsOverplot; }

bool PlotOutputTypeProperties::supportsAddToExistingPlot() const { return m_capabilities.supportsAddToExistingPlot; }

bool PlotOutputTypeProperties::excludesPostprocessedGroupOutputs() const {
  return m_capabilities.excludesPostprocessedGroupOutputs;
}

bool PlotOutputTypeProperties::requiresWorkspaceGroupsForMultiPlot() const {
  return m_capabilities.requiresWorkspaceGroupsForMultiPlot;
}

bool PlotOutputTypeProperties::showsDetectorMapProperties() const { return m_propertyControls.detectorMap; }

bool PlotOutputTypeProperties::showsAlignmentProperties() const { return m_propertyControls.alignment; }

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
