// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "PlottingModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
auto constexpr spinAsymmetryWorkspacePrefix = "__isis_refl_spin_asym_";
auto constexpr alignmentWorkspacePrefix = "__isis_refl_align_";
auto constexpr detectorMapWorkspacePrefix = "__isis_refl_det_map_";

using IndexRegion = std::pair<int, int>;
using WorkspaceCreator = std::vector<std::string> (*)(std::vector<PlottingWorkspaceSelection> const &,
                                                      PlotOutputSelection const &);

struct WorkspaceGroupSelection {
  std::string key;
  std::vector<std::string> workspaceNames;
};

const std::unordered_map<std::string, IndexRegion> instrumentDetectorIndexRegions{
    {"INTER", {0, 0}}, {"POLREF", {4, 643}}, {"OFFSPEC", {0, 0}}, {"SURF", {0, 0}}, {"CRISP", {0, 0}},
};

IndexRegion configuredInstrumentRegion(std::unordered_map<std::string, IndexRegion> const &regions,
                                       std::string const &instrumentName, std::string const &regionName) {
  auto const region = regions.find(instrumentName);
  if (region == regions.cend()) {
    throw std::invalid_argument("Reflectometry plotting " + regionName + " region is not configured for instrument '" +
                                instrumentName + "'.");
  }
  return region->second;
}

IndexRegion detectorIndexRegion(std::string const &instrumentName) {
  return configuredInstrumentRegion(instrumentDetectorIndexRegions, instrumentName, "detector");
}

Mantid::API::IAlgorithm_sptr createAlgorithm(std::string const &name, int const version = -1) {
  auto algorithm = Mantid::API::AlgorithmManager::Instance().createUnmanaged(name, version);
  algorithm->initialize();
  algorithm->setChild(true);
  algorithm->setLogging(false);
  algorithm->setRethrows(true);
  return algorithm;
}

Mantid::API::MatrixWorkspace_sptr executeBinaryAlgorithm(std::string const &algorithmName,
                                                         Mantid::API::MatrixWorkspace_sptr const &lhsWorkspace,
                                                         Mantid::API::MatrixWorkspace_sptr const &rhsWorkspace) {
  auto algorithm = createAlgorithm(algorithmName);
  algorithm->setProperty("LHSWorkspace", lhsWorkspace);
  algorithm->setProperty("RHSWorkspace", rhsWorkspace);
  algorithm->setProperty("OutputWorkspace", "__NotUsed__");
  algorithm->execute();
  return algorithm->getProperty("OutputWorkspace");
}

Mantid::API::MatrixWorkspace_sptr extractDetectorSpectra(Mantid::API::MatrixWorkspace_sptr const &workspace,
                                                         std::string const &instrumentName) {
  auto algorithm = createAlgorithm("ExtractSpectra");
  algorithm->setProperty("InputWorkspace", workspace);
  auto const [idxMin, idxMax] = detectorIndexRegion(instrumentName);
  algorithm->setProperty("StartWorkspaceIndex", idxMin);
  algorithm->setProperty("EndWorkspaceIndex", idxMax);
  algorithm->setProperty("OutputWorkspace", "__NotUsed__");
  algorithm->execute();
  return algorithm->getProperty("OutputWorkspace");
}

Mantid::API::MatrixWorkspace_sptr convertToWavelength(Mantid::API::MatrixWorkspace_sptr const &workspace) {
  auto algorithm = createAlgorithm("ConvertUnits");
  algorithm->setProperty("InputWorkspace", workspace);
  algorithm->setProperty("Target", "Wavelength");
  algorithm->setProperty("EMode", "Elastic");
  algorithm->setProperty("OutputWorkspace", "__NotUsed__");
  algorithm->execute();
  return algorithm->getProperty("OutputWorkspace");
}

Mantid::API::MatrixWorkspace_sptr extractSpectrum(Mantid::API::MatrixWorkspace_sptr const &workspace,
                                                  size_t const workspaceIndex) {
  auto algorithm = createAlgorithm("ExtractSpectra");
  algorithm->setProperty("InputWorkspace", workspace);
  algorithm->setProperty("WorkspaceIndexList", std::vector<size_t>{workspaceIndex});
  algorithm->setProperty("OutputWorkspace", "__NotUsed__");
  algorithm->execute();
  return algorithm->getProperty("OutputWorkspace");
}

double theta(Mantid::API::MatrixWorkspace const &workspace, size_t const workspaceIndex) {
  return workspace.spectrumInfo().twoTheta(workspaceIndex) * 90.0 / M_PI;
}

double xValueForFractionalWorkspaceIndex(Mantid::API::MatrixWorkspace const &workspace, double const workspaceIndex,
                                         AlignmentXAxis const xAxis) {
  if (xAxis == AlignmentXAxis::DetectorId) {
    return workspaceIndex;
  }
  auto const lowerIndex = static_cast<size_t>(std::floor(workspaceIndex));
  auto const upperIndex = static_cast<size_t>(std::ceil(workspaceIndex));
  auto const fraction = workspaceIndex - static_cast<double>(lowerIndex);
  auto const lowerTheta = theta(workspace, lowerIndex);
  return lowerTheta + fraction * (theta(workspace, upperIndex) - lowerTheta);
}

double detectorMapYAxisValue(Mantid::API::MatrixWorkspace const &workspace, size_t const workspaceIndex,
                             DetectorMapYAxis const yAxis) {
  return yAxis == DetectorMapYAxis::Theta ? theta(workspace, workspaceIndex) : static_cast<double>(workspaceIndex);
}

void setDetectorMapYAxisValues(Mantid::API::MatrixWorkspace_sptr const &workspace, DetectorMapYAxis const yAxis) {
  auto values = std::vector<double>{};
  values.reserve(workspace->getNumberHistograms());
  for (size_t workspaceIndex = 0; workspaceIndex < workspace->getNumberHistograms(); ++workspaceIndex) {
    values.emplace_back(detectorMapYAxisValue(*workspace, workspaceIndex, yAxis));
  }
  workspace->replaceAxis(1, std::make_unique<Mantid::API::NumericAxis>(std::move(values)));
}

Mantid::API::MatrixWorkspace_sptr createPointWorkspace(size_t const numberOfPoints) {
  return Mantid::API::WorkspaceFactory::Instance().create("Workspace2D", 1, numberOfPoints, numberOfPoints);
}

void setAlignmentXAxisValues(Mantid::API::MatrixWorkspace_sptr const &profileWorkspace,
                             Mantid::API::MatrixWorkspace const &rawWorkspace, AlignmentXAxis const xAxis) {
  if (xAxis == AlignmentXAxis::DetectorId) {
    return;
  }
  auto &x = profileWorkspace->dataX(0);
  std::transform(x.begin(), x.end(), x.begin(), [&rawWorkspace, &xAxis](const auto &x) {
    return xValueForFractionalWorkspaceIndex(rawWorkspace, x, xAxis);
  });
}

struct SpecularPeakFit {
  Mantid::API::MatrixWorkspace_sptr profileWorkspace;
  Mantid::API::MatrixWorkspace_sptr fitWorkspace;
  double peakCentre;
};

SpecularPeakFit fitSpecularPeak(Mantid::API::MatrixWorkspace_sptr const &workspace) {
  auto algorithm = createAlgorithm("FindReflectometryLines", 3);
  algorithm->setProperty("InputWorkspace", workspace);
  algorithm->setProperty("BackgroundType", "Linear");
  algorithm->setProperty("OutputProfileWorkspace", "__NotUsed__");
  algorithm->setProperty("OutputFitWorkspace", "__NotUsed__");
  algorithm->execute();
  return {algorithm->getProperty("OutputProfileWorkspace"), algorithm->getProperty("OutputFitWorkspace"),
          algorithm->getProperty("LineCentre")};
}

Mantid::API::MatrixWorkspace_sptr createPeakCentreWorkspace(Mantid::API::MatrixWorkspace_sptr const &profileWorkspace,
                                                            double const peakCentre) {
  auto peakCentreWorkspace = createPointWorkspace(2);
  auto const &profileY = profileWorkspace->y(0);
  auto const firstFinite =
      std::find_if(profileY.cbegin(), profileY.cend(), [](double value) { return std::isfinite(value); });
  if (firstFinite == profileY.cend()) {
    throw std::runtime_error("Cannot create a peak centre marker for a profile without finite values.");
  }
  auto minY = *firstFinite;
  auto maxY = *firstFinite;
  for (auto value = std::next(firstFinite); value != profileY.cend(); ++value) {
    if (std::isfinite(*value)) {
      minY = std::min(minY, *value);
      maxY = std::max(maxY, *value);
    }
  }

  auto &x = peakCentreWorkspace->dataX(0);
  auto &y = peakCentreWorkspace->dataY(0);
  auto &e = peakCentreWorkspace->dataE(0);
  x[0] = peakCentre;
  x[1] = peakCentre;
  y[0] = minY;
  y[1] = maxY;
  e[0] = 0.0;
  e[1] = 0.0;
  return peakCentreWorkspace;
}

Mantid::API::WorkspaceGroup_sptr workspaceGroupFromADS(std::string const &workspaceName) {
  auto &ads = Mantid::API::AnalysisDataService::Instance();
  if (!ads.doesExist(workspaceName)) {
    return nullptr;
  }
  return std::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(ads.retrieveWS<Mantid::API::Workspace>(workspaceName));
}

bool workspaceHasRunNumber(Mantid::API::MatrixWorkspace const &workspace, std::string const &runNumber) {
  auto const &run = workspace.run();
  return run.hasProperty("run_number") && run.getProperty("run_number")->value() == runNumber;
}

bool workspaceHasCurrentPeriod(Mantid::API::MatrixWorkspace const &workspace, int const period) {
  auto const &run = workspace.run();
  try {
    return run.hasProperty("current_period") && run.getPropertyAsIntegerValue("current_period") == period;
  } catch (std::exception const &) {
    return false;
  }
}

bool workspaceMatchesTOFSelection(Mantid::API::MatrixWorkspace const &workspace,
                                  PlottingWorkspaceSelection const &selection, std::string const &expectedRunNumber) {
  if (!workspaceHasRunNumber(workspace, expectedRunNumber)) {
    return false;
  }
  return !selection.period || workspaceHasCurrentPeriod(workspace, *selection.period);
}

Mantid::API::MatrixWorkspace_sptr rawTOFWorkspaceForSelection(PlottingWorkspaceSelection const &workspace) {
  if (workspace.runNumbers.size() != 1) {
    return nullptr;
  }

  auto const &expectedRunNumber = workspace.runNumbers.front();
  for (auto const &groupName : std::vector<std::string>{"TOF", "__TOF"}) {
    auto const workspaceGroup = workspaceGroupFromADS(groupName);
    if (!workspaceGroup) {
      continue;
    }
    for (auto index = 0u; index < workspaceGroup->size(); ++index) {
      auto const groupMember = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(workspaceGroup->getItem(index));
      if (groupMember && workspaceMatchesTOFSelection(*groupMember, workspace, expectedRunNumber)) {
        return groupMember;
      }
    }
  }
  return nullptr;
}

std::optional<std::pair<std::string, std::string>>
spinAsymmetryUpDownWorkspaces(std::vector<std::string> const &workspaces) {
  // Reflectometry polarization correction algorithms output workspace groups in canonical spin-state order,
  // regardless of the user-specified input order. PNR outputs U then D; PA outputs UU first and DD last.
  if (workspaces.size() == 2) {
    return std::make_pair(workspaces.front(), workspaces.back());
  }
  if (workspaces.size() == 4) {
    return std::make_pair(workspaces.front(), workspaces.back());
  }
  return std::nullopt;
}

bool workspaceExists(std::string const &workspaceName) {
  return Mantid::API::AnalysisDataService::Instance().doesExist(workspaceName);
}

std::string createSpinAsymmetryWorkspace(WorkspaceGroupSelection const &workspaceGroup) {
  auto const &workspaces = workspaceGroup.workspaceNames;
  auto const upDownWorkspaces = spinAsymmetryUpDownWorkspaces(workspaces);
  if (!upDownWorkspaces) {
    return "";
  }

  auto const outputWorkspace = std::string{spinAsymmetryWorkspacePrefix} + workspaceGroup.key;
  if (workspaceExists(outputWorkspace)) {
    return outputWorkspace;
  }

  auto &ads = Mantid::API::AnalysisDataService::Instance();
  auto const upWorkspace = ads.retrieveWS<Mantid::API::MatrixWorkspace>(upDownWorkspaces->first);
  auto const downWorkspace = ads.retrieveWS<Mantid::API::MatrixWorkspace>(upDownWorkspaces->second);

  auto const numeratorWorkspace = executeBinaryAlgorithm("Minus", upWorkspace, downWorkspace);
  auto const denominatorWorkspace = executeBinaryAlgorithm("Plus", upWorkspace, downWorkspace);
  auto const spinAsymmetryWorkspace = executeBinaryAlgorithm("Divide", numeratorWorkspace, denominatorWorkspace);
  ads.addOrReplace(outputWorkspace, spinAsymmetryWorkspace);
  return outputWorkspace;
}

std::optional<std::string> groupingKey(PlottingWorkspaceSelection const &workspace) {
  if (workspace.workspaceGroupName.empty()) {
    return std::nullopt;
  }
  return workspace.workspaceGroupName;
}

std::vector<WorkspaceGroupSelection>
workspaceGroups(std::vector<PlottingWorkspaceSelection> const &selectedWorkspaces) {
  auto keys = std::vector<std::string>{};
  auto groupedWorkspaces = std::vector<WorkspaceGroupSelection>{};
  for (auto const &workspace : selectedWorkspaces) {
    auto const key = groupingKey(workspace);
    if (!key) {
      continue;
    }
    auto const keyIter = std::find(keys.cbegin(), keys.cend(), *key);
    if (keyIter == keys.cend()) {
      keys.emplace_back(*key);
      groupedWorkspaces.push_back({*key, {workspace.workspaceName}});
    } else {
      groupedWorkspaces[std::distance(keys.cbegin(), keyIter)].workspaceNames.emplace_back(workspace.workspaceName);
    }
  }
  return groupedWorkspaces;
}

std::vector<std::string> selectedWorkspaceNames(std::vector<PlottingWorkspaceSelection> const &selectedWorkspaces) {
  auto workspaceNames = std::vector<std::string>{};
  workspaceNames.reserve(selectedWorkspaces.size());
  std::transform(selectedWorkspaces.cbegin(), selectedWorkspaces.cend(), std::back_inserter(workspaceNames),
                 [](const auto &workspace) { return workspace.workspaceName; });
  return workspaceNames;
}

std::vector<std::string> createSpinAsymmetryWorkspaces(std::vector<PlottingWorkspaceSelection> const &workspaces) {
  auto outputWorkspaces = std::vector<std::string>{};
  for (auto const &workspaceGroup : workspaceGroups(workspaces)) {
    auto outputWorkspace = createSpinAsymmetryWorkspace(workspaceGroup);
    if (!outputWorkspace.empty()) {
      outputWorkspaces.emplace_back(std::move(outputWorkspace));
    }
  }
  return outputWorkspaces;
}

std::string alignmentXAxisSuffix(AlignmentXAxis const xAxis) {
  return xAxis == AlignmentXAxis::DetectorId ? "" : "_theta";
}

std::string detectorMapAxisSuffix(DetectorMapXAxis const xAxis, DetectorMapYAxis const yAxis) {
  auto suffix = std::string{};
  if (xAxis == DetectorMapXAxis::Lambda) {
    suffix += "_lambda";
  }
  if (yAxis == DetectorMapYAxis::Theta) {
    suffix += "_theta";
  }
  return suffix;
}

std::string alignmentWorkspaceName(PlottingWorkspaceSelection const &workspace, AlignmentXAxis const xAxis) {
  return std::string{alignmentWorkspacePrefix} + workspace.workspaceName + alignmentXAxisSuffix(xAxis);
}

std::string detectorMapWorkspaceName(PlottingWorkspaceSelection const &workspace, DetectorMapXAxis const xAxis,
                                     DetectorMapYAxis const yAxis) {
  return std::string{detectorMapWorkspacePrefix} + workspace.workspaceName + detectorMapAxisSuffix(xAxis, yAxis);
}

std::string createAlignmentWorkspace(PlottingWorkspaceSelection const &workspace, AlignmentXAxis const xAxis,
                                     std::string const &instrumentName) {
  auto &ads = Mantid::API::AnalysisDataService::Instance();
  auto const rawWorkspace = rawTOFWorkspaceForSelection(workspace);
  if (!rawWorkspace) {
    return "";
  }

  auto const outputWorkspace = alignmentWorkspaceName(workspace, xAxis);
  if (workspaceExists(outputWorkspace)) {
    return outputWorkspace;
  }

  auto const rawProfileWorkspace = outputWorkspace + "_profile";
  auto const fittedPeakWorkspace = outputWorkspace + "_fitted_peak";
  auto const peakCentreWorkspace = outputWorkspace + "_peak_centre";

  auto detectorSpectraWorkspace = extractDetectorSpectra(rawWorkspace, instrumentName);
  auto peakFit = fitSpecularPeak(detectorSpectraWorkspace);
  auto fittedWorkspace = peakFit.fitWorkspace ? extractSpectrum(peakFit.fitWorkspace, 1) : nullptr;
  auto const peakCentre = xValueForFractionalWorkspaceIndex(*detectorSpectraWorkspace, peakFit.peakCentre, xAxis);
  setAlignmentXAxisValues(peakFit.profileWorkspace, *detectorSpectraWorkspace, xAxis);
  if (fittedWorkspace) {
    setAlignmentXAxisValues(fittedWorkspace, *detectorSpectraWorkspace, xAxis);
  }
  auto centreWorkspace = createPeakCentreWorkspace(peakFit.profileWorkspace, peakCentre);

  auto group = std::make_shared<Mantid::API::WorkspaceGroup>();
  ads.addOrReplace(rawProfileWorkspace, peakFit.profileWorkspace);
  group->addWorkspace(peakFit.profileWorkspace);
  if (fittedWorkspace) {
    ads.addOrReplace(fittedPeakWorkspace, fittedWorkspace);
    group->addWorkspace(fittedWorkspace);
  }
  ads.addOrReplace(peakCentreWorkspace, centreWorkspace);
  group->addWorkspace(centreWorkspace);

  ads.addOrReplace(outputWorkspace, group);
  return outputWorkspace;
}

std::vector<std::string> createAlignmentWorkspaces(std::vector<PlottingWorkspaceSelection> const &workspaces,
                                                   AlignmentXAxis const xAxis, std::string const &instrumentName) {
  auto outputWorkspaces = std::vector<std::string>{};
  for (auto const &workspace : workspaces) {
    auto outputWorkspace = createAlignmentWorkspace(workspace, xAxis, instrumentName);
    if (!outputWorkspace.empty()) {
      outputWorkspaces.emplace_back(std::move(outputWorkspace));
    }
  }
  return outputWorkspaces;
}

std::string createDetectorMapWorkspace(PlottingWorkspaceSelection const &workspace, DetectorMapXAxis const xAxis,
                                       DetectorMapYAxis const yAxis, std::string const &instrumentName) {
  auto &ads = Mantid::API::AnalysisDataService::Instance();
  auto const rawWorkspace = rawTOFWorkspaceForSelection(workspace);
  if (!rawWorkspace) {
    return "";
  }

  auto const outputWorkspace = detectorMapWorkspaceName(workspace, xAxis, yAxis);
  if (workspaceExists(outputWorkspace)) {
    return outputWorkspace;
  }

  auto detectorMapWorkspace = extractDetectorSpectra(rawWorkspace, instrumentName);
  if (xAxis == DetectorMapXAxis::Lambda) {
    detectorMapWorkspace = convertToWavelength(detectorMapWorkspace);
  }
  setDetectorMapYAxisValues(detectorMapWorkspace, yAxis);

  ads.addOrReplace(outputWorkspace, detectorMapWorkspace);
  return outputWorkspace;
}

std::vector<std::string> createDetectorMapWorkspaces(std::vector<PlottingWorkspaceSelection> const &workspaces,
                                                     DetectorMapXAxis const xAxis, DetectorMapYAxis const yAxis,
                                                     std::string const &instrumentName) {
  auto outputWorkspaces = std::vector<std::string>{};
  for (auto const &workspace : workspaces) {
    auto outputWorkspace = createDetectorMapWorkspace(workspace, xAxis, yAxis, instrumentName);
    if (!outputWorkspace.empty()) {
      outputWorkspaces.emplace_back(std::move(outputWorkspace));
    }
  }
  return outputWorkspaces;
}

std::vector<std::string> selectedWorkspaceNamesForSelection(std::vector<PlottingWorkspaceSelection> const &workspaces,
                                                            PlotOutputSelection const &) {
  return selectedWorkspaceNames(workspaces);
}

std::vector<std::string> spinAsymmetryWorkspacesForSelection(std::vector<PlottingWorkspaceSelection> const &workspaces,
                                                             PlotOutputSelection const &) {
  return createSpinAsymmetryWorkspaces(workspaces);
}

std::vector<std::string> alignmentWorkspacesForSelection(std::vector<PlottingWorkspaceSelection> const &workspaces,
                                                         PlotOutputSelection const &selection) {
  return createAlignmentWorkspaces(workspaces, selection.alignmentXAxis, selection.instrumentName);
}

std::vector<std::string> detectorMapWorkspacesForSelection(std::vector<PlottingWorkspaceSelection> const &workspaces,
                                                           PlotOutputSelection const &selection) {
  return createDetectorMapWorkspaces(workspaces, selection.detectorMapXAxis, selection.detectorMapYAxis,
                                     selection.instrumentName);
}

WorkspaceCreator workspaceCreatorFor(PlotOutputType const outputType) {
  switch (outputType) {
  case PlotOutputType::ReflectivityCurve:
    return selectedWorkspaceNamesForSelection;
  case PlotOutputType::DetectorMap:
    return detectorMapWorkspacesForSelection;
  case PlotOutputType::SpinAsymmetry:
    return spinAsymmetryWorkspacesForSelection;
  case PlotOutputType::Alignment:
    return alignmentWorkspacesForSelection;
  }
  throw std::runtime_error("Unexpected reflectometry plot output type.");
}
} // namespace

std::vector<std::string> PlottingModel::workspacesForPlotting(std::vector<PlottingWorkspaceSelection> const &workspaces,
                                                              PlotOutputSelection const &selection) const {
  return workspaceCreatorFor(selection.outputType)(workspaces, selection);
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
