// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "OutputPlotOptionsModel.h"
#include "SettingsHelper.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/Strings.h"

using namespace Mantid::API;
using namespace Mantid::Kernel::Strings;

namespace {

template <typename BeginIter, typename Iterable>
void removeFromIterable(BeginIter const &beginIter, Iterable &iterable) {
  iterable.erase(beginIter, iterable.end());
}

std::vector<std::string> splitStringBy(std::string const &str, std::string const &delimiter) {
  std::vector<std::string> subStrings;
  boost::split(subStrings, str, boost::is_any_of(delimiter));
  removeFromIterable(std::remove_if(subStrings.begin(), subStrings.end(),
                                    [](std::string const &subString) { return subString.empty(); }),
                     subStrings);
  return subStrings;
}

std::string getIndicesRange(std::string const &str) {
  auto const bounds = splitStringBy(str, "-");
  if (std::stoull(bounds[0]) > std::stoull(bounds[1])) {
    return bounds[1] + "-" + bounds[0];
  } else if (std::stoull(bounds[0]) < std::stoull(bounds[1])) {
    return str;
  } else {
    return bounds[0];
  }
}

std::string rearrangeIndicesSubString(std::string const &str) {
  return str.find("-") != std::string::npos ? getIndicesRange(str) : str;
}

// Swaps the two numbers in a spectra range if they go from large to small
std::string rearrangeIndicesRangeStrings(std::string const &str) {
  std::string indicesString;
  auto subStrings = splitStringBy(str, ",");
  for (auto it = subStrings.begin(); it != subStrings.end(); ++it) {
    indicesString += rearrangeIndicesSubString(*it);
    indicesString += it != std::prev(subStrings.end()) ? "," : "";
  }
  return indicesString;
}

std::string formatIndicesString(std::string str) {
  // Remove spaces
  removeFromIterable(std::remove_if(str.begin(), str.end(), isspace), str);
  // Rearrange range strings
  auto indices = parseRange(rearrangeIndicesRangeStrings(str));
  std::sort(indices.begin(), indices.end());
  // Remove duplicate entries
  removeFromIterable(std::unique(indices.begin(), indices.end()), indices);
  return joinCompress(indices.begin(), indices.end());
}

void insertWorkspaceNames(std::vector<std::string> &allNames, std::string const &workspaceName) {
  auto &ads = AnalysisDataService::Instance();
  if (ads.doesExist(workspaceName)) {
    if (auto const group = ads.retrieveWS<WorkspaceGroup>(workspaceName)) {
      auto const groupContents = group->getNames();
      allNames.insert(allNames.end(), groupContents.begin(), groupContents.end());
    } else if (auto const workspace = ads.retrieveWS<MatrixWorkspace>(workspaceName)) {
      allNames.emplace_back(workspace->getName());
    }
  }
}

boost::optional<std::string> checkWorkspaceSpectrumSize(const MatrixWorkspace_const_sptr &workspace) {
  if (workspace->y(0).size() < 2)
    return "Plot Spectra failed: There is only one data point to plot in " + workspace->getName() + ".";
  return boost::none;
}

boost::optional<std::string> checkWorkspaceBinSize(const MatrixWorkspace_const_sptr &workspace) {
  if (workspace->getNumberHistograms() < 2)
    return "Plot Bins failed: There is only one data point to plot in " + workspace->getName() + ".";
  return boost::none;
}

std::map<std::string, std::string>
constructActions(boost::optional<std::map<std::string, std::string>> const &availableActions) {
  std::map<std::string, std::string> actions;
  if (availableActions)
    actions = availableActions.get();
  actions.insert({"Plot Spectra", "Plot Spectra"});
  actions.insert({"Plot Bins", "Plot Bins"});
  actions.insert({"Open Slice Viewer", "Open Slice Viewer"});
  actions.insert({"Plot Tiled", "Plot Tiled"});
  return actions;
}

} // namespace

namespace MantidQt::CustomInterfaces {

OutputPlotOptionsModel::OutputPlotOptionsModel(
    boost::optional<std::map<std::string, std::string>> const &availableActions)
    : m_actions(constructActions(availableActions)), m_fixedIndices(false), m_workspaceIndices(boost::none),
      m_workspaceName(boost::none), m_plotter(std::make_unique<ExternalPlotter>()) {}

/// Used by the unit tests so that m_plotter can be mocked
OutputPlotOptionsModel::OutputPlotOptionsModel(
    ExternalPlotter *plotter, boost::optional<std::map<std::string, std::string>> const &availableActions)
    : m_actions(constructActions(availableActions)), m_fixedIndices(false), m_workspaceIndices(boost::none),
      m_workspaceName(boost::none), m_plotter(plotter) {}

OutputPlotOptionsModel::~OutputPlotOptionsModel() = default;

bool OutputPlotOptionsModel::setWorkspace(std::string const &workspaceName) {
  auto &ads = AnalysisDataService::Instance();
  if (ads.doesExist(workspaceName) && ads.retrieveWS<MatrixWorkspace>(workspaceName)) {
    m_workspaceName = workspaceName;
    return true;
  }
  return false;
}

boost::optional<std::string> OutputPlotOptionsModel::workspace() const { return m_workspaceName; }

void OutputPlotOptionsModel::removeWorkspace() { m_workspaceName = boost::none; }

std::vector<std::string>
OutputPlotOptionsModel::getAllWorkspaceNames(std::vector<std::string> const &workspaceNames) const {
  std::vector<std::string> allNames;
  for (auto const &workspaceName : workspaceNames)
    insertWorkspaceNames(allNames, workspaceName);
  return allNames;
}

void OutputPlotOptionsModel::setUnit(std::string const &unit) { m_unit = unit; }

boost::optional<std::string> OutputPlotOptionsModel::unit() { return m_unit; }

std::string OutputPlotOptionsModel::formatIndices(std::string const &indices) const {
  return formatIndicesString(indices);
}

void OutputPlotOptionsModel::setFixedIndices(std::string const &indices) {
  m_fixedIndices = !indices.empty();
  if (m_fixedIndices)
    m_workspaceIndices = indices;
}

bool OutputPlotOptionsModel::indicesFixed() const { return m_fixedIndices; }

bool OutputPlotOptionsModel::setIndices(std::string const &indices) {
  bool valid = validateIndices(indices);
  if (valid)
    m_workspaceIndices = indices;
  else
    m_workspaceIndices = boost::none;
  return valid;
}

boost::optional<std::string> OutputPlotOptionsModel::indices() const { return m_workspaceIndices; }

bool OutputPlotOptionsModel::validateIndices(std::string const &indices, MantidAxis const &axisType) const {
  auto &ads = AnalysisDataService::Instance();
  if (!indices.empty() && m_workspaceName && ads.doesExist(m_workspaceName.get())) {
    if (auto const matrixWs = ads.retrieveWS<MatrixWorkspace>(m_workspaceName.get())) {
      if (axisType == MantidAxis::Spectrum)
        return validateSpectra(matrixWs, indices);
      return validateBins(matrixWs, indices);
    }
  }
  return false;
}

bool OutputPlotOptionsModel::validateSpectra(const MatrixWorkspace_sptr &workspace, std::string const &spectra) const {
  auto const numberOfHistograms = workspace->getNumberHistograms();
  auto const lastIndex = std::stoul(splitStringBy(spectra, ",-").back());
  return lastIndex < numberOfHistograms;
}

bool OutputPlotOptionsModel::validateBins(const MatrixWorkspace_sptr &workspace, std::string const &bins) const {
  auto const numberOfBins = workspace->y(0).size();
  auto const lastIndex = std::stoul(splitStringBy(bins, ",-").back());
  return lastIndex < numberOfBins;
}

std::string OutputPlotOptionsModel::convertUnit(const std::string &workspaceName, const std::string &unit) {
  const std::string convertedWorkspaceName = workspaceName + "_" + unit;

  IAlgorithm_sptr convertUnits = AlgorithmManager::Instance().create("ConvertUnits");
  convertUnits->initialize();
  convertUnits->setProperty("InputWorkspace", workspaceName);
  convertUnits->setProperty("OutputWorkspace", convertedWorkspaceName);
  convertUnits->setProperty("Target", unit);
  convertUnits->execute();

  return convertedWorkspaceName;
}

void OutputPlotOptionsModel::plotSpectra() {
  auto const workspaceName = workspace();
  auto const indicesString = indices();
  auto const unitName = unit();

  if (workspaceName && indicesString) {
    std::string plotWorkspaceName = unitName ? convertUnit(workspaceName.get(), unitName.get()) : workspaceName.get();
    m_plotter->plotSpectra(plotWorkspaceName, indicesString.get(), SettingsHelper::externalPlotErrorBars());
  }
}

void OutputPlotOptionsModel::plotBins(std::string const &binIndices) {
  if (auto const workspaceName = workspace())
    m_plotter->plotBins(workspaceName.get(), binIndices, SettingsHelper::externalPlotErrorBars());
}

void OutputPlotOptionsModel::showSliceViewer() {
  auto const workspaceName = workspace();
  auto const unitName = unit();

  if (workspaceName) {
    std::string plotWorkspaceName = unitName ? convertUnit(workspaceName.get(), unitName.get()) : workspaceName.get();
    m_plotter->showSliceViewer(plotWorkspaceName);
  }
}

void OutputPlotOptionsModel::plotTiled() {
  auto const workspaceName = workspace();
  auto const indicesString = indices();
  if (workspaceName && indicesString)
    m_plotter->plotTiled(workspaceName.get(), indicesString.get(), SettingsHelper::externalPlotErrorBars());
}

boost::optional<std::string> OutputPlotOptionsModel::singleDataPoint(MantidAxis const &axisType) const {
  if (auto const workspaceName = workspace())
    return checkWorkspaceSize(workspaceName.get(), axisType);
  return boost::none;
}

boost::optional<std::string> OutputPlotOptionsModel::checkWorkspaceSize(std::string const &workspaceName,
                                                                        MantidAxis const &axisType) const {
  auto &ads = AnalysisDataService::Instance();
  if (ads.doesExist(workspaceName)) {
    if (auto const matrixWs = ads.retrieveWS<MatrixWorkspace>(workspaceName)) {
      if (axisType == MantidAxis::Spectrum)
        return checkWorkspaceSpectrumSize(matrixWs);
      return checkWorkspaceBinSize(matrixWs);
    }
  }
  return boost::none;
}

std::map<std::string, std::string> OutputPlotOptionsModel::availableActions() const { return m_actions; }

} // namespace MantidQt::CustomInterfaces
