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
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <sstream>
#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
auto constexpr spinAsymmetryWorkspacePrefix = "__isis_reflectometry_spin_asymmetry_";
auto constexpr alignmentWorkspacePrefix = "__isis_reflectometry_alignment_";

struct GaussianParameters {
  double height;
  double centre;
  double sigma;
};

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

double detectorId(Mantid::API::MatrixWorkspace const &workspace, size_t const workspaceIndex) {
  auto const &detectorIds = workspace.getSpectrum(workspaceIndex).getDetectorIDs();
  if (!detectorIds.empty()) {
    return static_cast<double>(*detectorIds.cbegin());
  }
  return static_cast<double>(workspaceIndex);
}

double theta(Mantid::API::MatrixWorkspace const &workspace, size_t const workspaceIndex) {
  try {
    return workspace.spectrumInfo().twoTheta(workspaceIndex) * 90.0 / M_PI;
  } catch (std::exception const &) {
    return detectorId(workspace, workspaceIndex);
  }
}

double xValueForWorkspaceIndex(Mantid::API::MatrixWorkspace const &workspace, size_t const workspaceIndex,
                               AlignmentXAxis const xAxis) {
  return xAxis == AlignmentXAxis::Theta ? theta(workspace, workspaceIndex) : detectorId(workspace, workspaceIndex);
}

Mantid::API::MatrixWorkspace_sptr createPointWorkspace(size_t const numberOfPoints) {
  return Mantid::API::WorkspaceFactory::Instance().create("Workspace2D", 1, numberOfPoints, numberOfPoints);
}

Mantid::API::MatrixWorkspace_sptr createAlignmentProfileWorkspace(Mantid::API::MatrixWorkspace_sptr const &workspace,
                                                                  AlignmentXAxis const xAxis) {
  auto const integratedWorkspace = integrateSpectra(workspace);
  auto profileWorkspace = createPointWorkspace(workspace->getNumberHistograms());

  auto &x = profileWorkspace->dataX(0);
  auto &y = profileWorkspace->dataY(0);
  auto &e = profileWorkspace->dataE(0);
  for (size_t workspaceIndex = 0; workspaceIndex < workspace->getNumberHistograms(); ++workspaceIndex) {
    x[workspaceIndex] = xValueForWorkspaceIndex(*workspace, workspaceIndex, xAxis);
    y[workspaceIndex] = integratedWorkspace->y(workspaceIndex).front();
    e[workspaceIndex] = integratedWorkspace->e(workspaceIndex).front();
  }
  return profileWorkspace;
}

GaussianParameters estimateGaussianParameters(Mantid::API::MatrixWorkspace const &workspace) {
  auto const &x = workspace.x(0);
  auto const &y = workspace.y(0);
  auto const maxY = std::max_element(y.cbegin(), y.cend());
  auto const maxIndex = std::distance(y.cbegin(), maxY);
  auto const width = x.empty() ? 1.0 : std::abs(x.back() - x.front());
  return {*maxY, x[maxIndex], std::max(width / 10.0, 1.0)};
}

std::string gaussianFunctionString(GaussianParameters const &parameters) {
  auto stream = std::ostringstream{};
  stream << "name=Gaussian,Height=" << parameters.height << ",PeakCentre=" << parameters.centre
         << ",Sigma=" << parameters.sigma;
  return stream.str();
}

GaussianParameters fitGaussian(Mantid::API::MatrixWorkspace_sptr const &workspace) {
  auto const initialParameters = estimateGaussianParameters(*workspace);

  try {
    auto algorithm = createAlgorithm("Fit");
    algorithm->setProperty("Function", gaussianFunctionString(initialParameters));
    algorithm->setProperty("InputWorkspace", workspace);
    algorithm->setProperty("WorkspaceIndex", 0);
    algorithm->setProperty("Output", "__NotUsed__");
    algorithm->setProperty("OutputParametersOnly", true);
    algorithm->execute();

    Mantid::API::IFunction_sptr function = algorithm->getProperty("Function");
    return {function->getParameter("Height"), function->getParameter("PeakCentre"),
            std::max(std::abs(function->getParameter("Sigma")), std::numeric_limits<double>::epsilon())};
  } catch (std::exception const &) {
    return initialParameters;
  }
}

double gaussianYValue(double const x, GaussianParameters const &parameters) {
  auto const scaledX = (x - parameters.centre) / parameters.sigma;
  return parameters.height * std::exp(-0.5 * scaledX * scaledX);
}

Mantid::API::MatrixWorkspace_sptr createFittedPeakWorkspace(Mantid::API::MatrixWorkspace_sptr const &profileWorkspace,
                                                            GaussianParameters const &parameters) {
  auto fittedPeakWorkspace = createPointWorkspace(profileWorkspace->blocksize());
  auto &x = fittedPeakWorkspace->dataX(0);
  auto &y = fittedPeakWorkspace->dataY(0);
  auto &e = fittedPeakWorkspace->dataE(0);
  for (size_t pointIndex = 0; pointIndex < profileWorkspace->blocksize(); ++pointIndex) {
    x[pointIndex] = profileWorkspace->x(0)[pointIndex];
    y[pointIndex] = gaussianYValue(x[pointIndex], parameters);
    e[pointIndex] = 0.0;
  }
  return fittedPeakWorkspace;
}

Mantid::API::MatrixWorkspace_sptr createPeakCentreWorkspace(Mantid::API::MatrixWorkspace_sptr const &profileWorkspace,
                                                            GaussianParameters const &parameters) {
  auto peakCentreWorkspace = createPointWorkspace(2);
  auto const &profileY = profileWorkspace->y(0);
  auto const yRange = std::minmax_element(profileY.cbegin(), profileY.cend());

  auto &x = peakCentreWorkspace->dataX(0);
  auto &y = peakCentreWorkspace->dataY(0);
  auto &e = peakCentreWorkspace->dataE(0);
  x[0] = parameters.centre;
  x[1] = parameters.centre;
  y[0] = *yRange.first;
  y[1] = *yRange.second;
  e[0] = 0.0;
  e[1] = 0.0;
  return peakCentreWorkspace;
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

std::string groupingKey(PlottingWorkspaceSelection const &workspace) {
  if (!workspace.workspaceGroupName.empty()) {
    return std::string{"workspace-group:"} + workspace.workspaceGroupName;
  }

  auto key = std::string{"run:"} + workspace.groupName;
  for (auto const &runNumber : workspace.runNumbers) {
    key += ":" + runNumber;
  }
  return key;
}

std::vector<std::vector<std::string>>
workspaceGroups(std::vector<PlottingWorkspaceSelection> const &selectedWorkspaces) {
  auto keys = std::vector<std::string>{};
  auto groupedWorkspaces = std::vector<std::vector<std::string>>{};
  for (auto const &workspace : selectedWorkspaces) {
    auto const key = groupingKey(workspace);
    auto const keyIter = std::find(keys.cbegin(), keys.cend(), key);
    if (keyIter == keys.cend()) {
      keys.emplace_back(key);
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

std::string createAlignmentWorkspace(std::string const &workspaceName, AlignmentXAxis const xAxis, size_t const index) {
  auto &ads = Mantid::API::AnalysisDataService::Instance();
  if (!ads.doesExist(workspaceName)) {
    return "";
  }

  auto const workspace = ads.retrieveWS<Mantid::API::MatrixWorkspace>(workspaceName);
  auto const outputWorkspace = std::string{alignmentWorkspacePrefix} + std::to_string(index);
  auto const rawWorkspace = outputWorkspace + "_raw";
  auto const fittedPeakWorkspace = outputWorkspace + "_fitted_peak";
  auto const peakCentreWorkspace = outputWorkspace + "_peak_centre";

  auto profileWorkspace = createAlignmentProfileWorkspace(workspace, xAxis);
  auto const parameters = fitGaussian(profileWorkspace);
  auto fittedWorkspace = createFittedPeakWorkspace(profileWorkspace, parameters);
  auto centreWorkspace = createPeakCentreWorkspace(profileWorkspace, parameters);

  ads.addOrReplace(rawWorkspace, profileWorkspace);
  ads.addOrReplace(fittedPeakWorkspace, fittedWorkspace);
  ads.addOrReplace(peakCentreWorkspace, centreWorkspace);

  auto group = std::make_shared<Mantid::API::WorkspaceGroup>();
  group->addWorkspace(profileWorkspace);
  group->addWorkspace(fittedWorkspace);
  group->addWorkspace(centreWorkspace);
  ads.addOrReplace(outputWorkspace, group);
  return outputWorkspace;
}

std::vector<std::string> createAlignmentWorkspaces(std::vector<PlottingWorkspaceSelection> const &workspaces,
                                                   AlignmentXAxis const xAxis) {
  auto outputWorkspaces = std::vector<std::string>{};
  for (auto const &workspace : selectedWorkspaceNames(workspaces)) {
    auto outputWorkspace = createAlignmentWorkspace(workspace, xAxis, outputWorkspaces.size());
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
    return createAlignmentWorkspaces(workspaces, options.alignmentXAxis);
  }
  return selectedWorkspaceNames(workspaces);
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
