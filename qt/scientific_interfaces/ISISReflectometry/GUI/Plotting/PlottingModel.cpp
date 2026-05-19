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
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <exception>
#include <iterator>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
auto constexpr spinAsymmetryWorkspacePrefix = "__isis_refl_spin_asym_";
auto constexpr alignmentWorkspacePrefix = "__isis_refl_align_";

struct GaussianParameters {
  double height;
  double centre;
  double sigma;
};

// Update these - only set for POLREF
const std::unordered_map<std::string, std::pair<int, int>> instrumentBackgroundIndexRegions{
    {"INTER", {0, 0}}, {"POLREF", {360, 450}}, {"OFFSPEC", {0, 0}}, {"SURF", {0, 0}}, {"CRISP", {0, 0}},
};

// Update these - only set for POLREF
const std::unordered_map<std::string, std::pair<int, int>> instrumentDetectorIndexRegions{
    {"INTER", {0, 0}}, {"POLREF", {4, 643}}, {"OFFSPEC", {0, 0}}, {"SURF", {0, 0}}, {"CRISP", {0, 0}},
};

std::pair<int, int> regionForInstrument(std::unordered_map<std::string, std::pair<int, int>> const &regions,
                                        std::string const &instrumentName) {
  auto const region = regions.find(instrumentName);
  if (region == regions.cend()) {
    throw std::invalid_argument("Alignment plotting is not configured for instrument '" + instrumentName + "'.");
  }
  return region->second;
}

Mantid::API::IAlgorithm_sptr createAlgorithm(std::string const &name) {
  auto algorithm = Mantid::API::AlgorithmManager::Instance().createUnmanaged(name);
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

Mantid::API::MatrixWorkspace_sptr integrateSpectra(Mantid::API::MatrixWorkspace_sptr const &workspace) {
  auto algorithm = createAlgorithm("Integration");
  algorithm->setProperty("InputWorkspace", workspace);
  algorithm->setProperty("OutputWorkspace", "__NotUsed__");
  algorithm->execute();
  return algorithm->getProperty("OutputWorkspace");
}

Mantid::API::MatrixWorkspace_sptr extractDetectorSpectra(Mantid::API::MatrixWorkspace_sptr const &workspace,
                                                         std::string const &instrumentName) {
  auto algorithm = createAlgorithm("ExtractSpectra");
  algorithm->setProperty("InputWorkspace", workspace);
  auto const [idxMin, idxMax] = regionForInstrument(instrumentDetectorIndexRegions, instrumentName);
  algorithm->setProperty("StartWorkspaceIndex", idxMin);
  algorithm->setProperty("EndWorkspaceIndex", idxMax);
  algorithm->setProperty("OutputWorkspace", "__NotUsed__");
  algorithm->execute();
  return algorithm->getProperty("OutputWorkspace");
}

Mantid::API::MatrixWorkspace_sptr transpose(Mantid::API::MatrixWorkspace_sptr const &workspace) {
  auto algorithm = createAlgorithm("Transpose");
  algorithm->setProperty("InputWorkspace", workspace);
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

double xValueForWorkspaceIndex(Mantid::API::MatrixWorkspace const &workspace, size_t const workspaceIndex,
                               AlignmentXAxis const xAxis) {
  return xAxis == AlignmentXAxis::Theta ? theta(workspace, workspaceIndex) : static_cast<double>(workspaceIndex);
}

Mantid::API::MatrixWorkspace_sptr createPointWorkspace(size_t const numberOfPoints) {
  return Mantid::API::WorkspaceFactory::Instance().create("Workspace2D", 1, numberOfPoints, numberOfPoints);
}

void setAlignmentXAxisValues(Mantid::API::MatrixWorkspace_sptr const &profileWorkspace,
                             Mantid::API::MatrixWorkspace const &rawWorkspace, AlignmentXAxis const xAxis) {
  auto &x = profileWorkspace->dataX(0);
  for (size_t idx = 0; idx < x.size(); ++idx) {
    x[idx] = xValueForWorkspaceIndex(rawWorkspace, idx, xAxis);
  }
}

GaussianParameters estimateGaussianParameters(Mantid::API::MatrixWorkspace const &workspace) {
  auto const &x = workspace.x(0);
  auto const &y = workspace.y(0);
  auto const maxY = std::max_element(y.cbegin(), y.cend());
  auto const maxIndex = std::distance(y.cbegin(), maxY);
  return {*maxY, x[maxIndex], 2.5}; // vary the estimated sigma per instrument?
}

std::string gaussianFunctionString(GaussianParameters const &parameters) {
  auto stream = std::ostringstream{};
  stream << "name=Gaussian,Height=" << parameters.height << ",PeakCentre=" << parameters.centre
         << ",Sigma=" << parameters.sigma;
  return stream.str();
}

GaussianParameters gaussianParametersFromTable(Mantid::API::ITableWorkspace_sptr const &peaks,
                                               GaussianParameters const &fallbackParameters) {
  if (!peaks || peaks->rowCount() == 0) {
    return fallbackParameters;
  }
  return {.height = peaks->cell<double>(0, 3),
          .centre = peaks->cell<double>(0, 1),
          .sigma = peaks->cell<double>(0, 2) / 2.3548};
}

GaussianParameters findPeaks(Mantid::API::MatrixWorkspace_sptr const &workspace) {
  auto const fallbackParameters = estimateGaussianParameters(*workspace);

  try {
    auto algorithm = createAlgorithm("FindPeaks");
    algorithm->setProperty("InputWorkspace", workspace);
    algorithm->setProperty("WorkspaceIndex", 0);
    algorithm->setProperty("PeaksList", "__NotUsed__");
    algorithm->execute();

    Mantid::API::ITableWorkspace_sptr peaks = algorithm->getProperty("PeaksList");
    return gaussianParametersFromTable(peaks, fallbackParameters);
  } catch (std::exception const &) {
    return fallbackParameters;
  }
}

Mantid::API::MatrixWorkspace_sptr subtractBackground(Mantid::API::MatrixWorkspace_sptr const &workspace,
                                                     Mantid::API::MatrixWorkspace_sptr const &rawWorkspace,
                                                     AlignmentXAxis const xAxis, std::string const &instrumentName) {
  auto algorithm = createAlgorithm("CalculateFlatBackground");
  algorithm->setProperty("InputWorkspace", workspace);
  algorithm->setProperty("OutputWorkspace", "__NotUsed__");
  auto const [idxMin, idxMax] = regionForInstrument(instrumentBackgroundIndexRegions, instrumentName);
  algorithm->setProperty("StartX", xValueForWorkspaceIndex(*rawWorkspace, idxMin, xAxis));
  algorithm->setProperty("EndX", xValueForWorkspaceIndex(*rawWorkspace, idxMax, xAxis));
  algorithm->execute();
  return algorithm->getProperty("OutputWorkspace");
}

Mantid::API::MatrixWorkspace_sptr fitGaussian(Mantid::API::MatrixWorkspace_sptr const &workspace,
                                              GaussianParameters const &parameters) {
  auto algorithm = createAlgorithm("Fit");
  algorithm->setProperty("Function", gaussianFunctionString(parameters));
  algorithm->setProperty("InputWorkspace", workspace);
  algorithm->setProperty("WorkspaceIndex", 0);
  algorithm->setProperty("Output", "__NotUsed__");
  algorithm->execute();
  return algorithm->getProperty("OutputWorkspace");
}

Mantid::API::MatrixWorkspace_sptr createPeakCentreWorkspace(Mantid::API::MatrixWorkspace_sptr const &profileWorkspace,
                                                            double const peakCentre) {
  auto peakCentreWorkspace = createPointWorkspace(2);
  auto const &profileY = profileWorkspace->y(0);
  auto const yRange = std::minmax_element(profileY.cbegin(), profileY.cend());

  auto &x = peakCentreWorkspace->dataX(0);
  auto &y = peakCentreWorkspace->dataY(0);
  auto &e = peakCentreWorkspace->dataE(0);
  x[0] = peakCentre;
  x[1] = peakCentre;
  y[0] = *yRange.first;
  y[1] = *yRange.second;
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

std::string createSpinAsymmetryWorkspace(std::vector<std::string> const &workspaces, size_t index) {
  auto const upDownWorkspaces = spinAsymmetryUpDownWorkspaces(workspaces);
  if (!upDownWorkspaces) {
    return "";
  }

  auto const outputWorkspace = std::string{spinAsymmetryWorkspacePrefix} + std::to_string(index);
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
  return std::string{"workspace-group:"} + workspace.workspaceGroupName;
}

std::vector<std::vector<std::string>>
workspaceGroups(std::vector<PlottingWorkspaceSelection> const &selectedWorkspaces) {
  auto keys = std::vector<std::string>{};
  auto groupedWorkspaces = std::vector<std::vector<std::string>>{};
  for (auto const &workspace : selectedWorkspaces) {
    auto const key = groupingKey(workspace);
    if (!key) {
      continue;
    }
    auto const keyIter = std::find(keys.cbegin(), keys.cend(), *key);
    if (keyIter == keys.cend()) {
      keys.emplace_back(*key);
      groupedWorkspaces.push_back({workspace.workspaceName});
    } else {
      groupedWorkspaces[std::distance(keys.cbegin(), keyIter)].emplace_back(workspace.workspaceName);
    }
  }
  return groupedWorkspaces;
}

std::vector<std::string> selectedWorkspaceNames(std::vector<PlottingWorkspaceSelection> const &selectedWorkspaces) {
  auto workspaceNames = std::vector<std::string>{};
  workspaceNames.reserve(selectedWorkspaces.size());
  for (auto const &workspace : selectedWorkspaces) {
    workspaceNames.emplace_back(workspace.workspaceName);
  }
  return workspaceNames;
}

std::vector<std::string> createSpinAsymmetryWorkspaces(std::vector<PlottingWorkspaceSelection> const &workspaces) {
  auto outputWorkspaces = std::vector<std::string>{};
  for (auto const &workspaceGroup : workspaceGroups(workspaces)) {
    auto outputWorkspace = createSpinAsymmetryWorkspace(workspaceGroup, outputWorkspaces.size());
    if (!outputWorkspace.empty()) {
      outputWorkspaces.emplace_back(std::move(outputWorkspace));
    }
  }
  return outputWorkspaces;
}

std::string safeWorkspaceName(std::string workspaceName) {
  std::replace_if(
      workspaceName.begin(), workspaceName.end(),
      [](unsigned char const character) { return !std::isalnum(character) && character != '_'; }, '_');
  return workspaceName;
}

std::string alignmentWorkspaceName(PlottingWorkspaceSelection const &workspace) {
  return std::string{alignmentWorkspacePrefix} + safeWorkspaceName(workspace.workspaceName);
}

std::string createAlignmentWorkspace(PlottingWorkspaceSelection const &workspace, AlignmentXAxis const xAxis,
                                     std::string const &instrumentName) {
  auto &ads = Mantid::API::AnalysisDataService::Instance();
  auto const rawWorkspace = rawTOFWorkspaceForSelection(workspace);
  if (!rawWorkspace) {
    return "";
  }

  auto const outputWorkspace = alignmentWorkspaceName(workspace);
  auto const rawProfileWorkspace = outputWorkspace + "_raw_sub_bg";
  auto const fittedPeakWorkspace = outputWorkspace + "_fitted_peak";
  auto const peakCentreWorkspace = outputWorkspace + "_peak_centre";

  auto detectorSpectraWorkspace = extractDetectorSpectra(rawWorkspace, instrumentName);
  auto integratedWorkspace = integrateSpectra(detectorSpectraWorkspace);
  auto profileWorkspace = transpose(integratedWorkspace);
  setAlignmentXAxisValues(profileWorkspace, *detectorSpectraWorkspace, xAxis);
  auto profileWorkspaceNoBG = subtractBackground(profileWorkspace, rawWorkspace, xAxis, instrumentName);
  auto fitParameters = findPeaks(profileWorkspaceNoBG);
  auto fitOutputWorkspace = fitGaussian(profileWorkspaceNoBG, fitParameters);
  auto fittedWorkspace = extractSpectrum(fitOutputWorkspace, 1);
  auto centreWorkspace = createPeakCentreWorkspace(profileWorkspaceNoBG, fitParameters.centre);

  ads.addOrReplace(rawProfileWorkspace, profileWorkspaceNoBG);
  ads.addOrReplace(fittedPeakWorkspace, fittedWorkspace);
  ads.addOrReplace(peakCentreWorkspace, centreWorkspace);

  auto group = std::make_shared<Mantid::API::WorkspaceGroup>();
  group->addWorkspace(profileWorkspaceNoBG);
  group->addWorkspace(fittedWorkspace);
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
} // namespace

std::vector<std::string> PlottingModel::workspacesForPlotting(std::vector<PlottingWorkspaceSelection> const &workspaces,
                                                              PlotOutputOptions const &options) const {
  if (options.outputType == PlotOutputType::SpinAsymmetry) {
    return createSpinAsymmetryWorkspaces(workspaces);
  }
  if (options.outputType == PlotOutputType::Alignment) {
    return createAlignmentWorkspaces(workspaces, options.alignmentXAxis, options.instrumentName);
  }
  return selectedWorkspaceNames(workspaces);
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
