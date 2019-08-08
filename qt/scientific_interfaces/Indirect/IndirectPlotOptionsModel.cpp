// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectPlotOptionsModel.h"

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

std::vector<std::string> splitStringBy(std::string const &str,
                                       std::string const &delimiter) {
  std::vector<std::string> subStrings;
  boost::split(subStrings, str, boost::is_any_of(delimiter));
  removeFromIterable(std::remove_if(subStrings.begin(), subStrings.end(),
                                    [](std::string const &subString) {
                                      return subString.empty();
                                    }),
                     subStrings);
  return subStrings;
}

std::string getIndicesRange(std::string const &str) {
  auto const bounds = splitStringBy(str, "-");
  return std::stoull(bounds[0]) > std::stoull(bounds[1])
             ? bounds[1] + "-" + bounds[0]
             : str;
}

std::string rearrangeIndicesSubString(std::string const &str) {
  return str.find("-") != std::string::npos ? getIndicesRange(str) : str;
}

// Swaps the two numbers in a spectra range if they go from large to small
std::string rearrangeIndicesRangeStrings(std::string const &str) {
  std::string indicesString;
  auto subStrings = splitStringBy(str, ",");
  for (auto it = subStrings.begin(); it < subStrings.end(); ++it) {
    indicesString += rearrangeIndicesSubString(*it);
    indicesString += it != subStrings.end() ? "," : "";
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

void insertWorkspaceNames(std::vector<std::string> &allNames,
                          std::string const &workspaceName) {
  auto &ads = AnalysisDataService::Instance();
  if (ads.doesExist(workspaceName)) {
    if (auto const group = ads.retrieveWS<WorkspaceGroup>(workspaceName)) {
      auto const groupContents = group->getNames();
      allNames.insert(allNames.end(), groupContents.begin(),
                      groupContents.end());
    } else if (auto const workspace =
                   ads.retrieveWS<MatrixWorkspace>(workspaceName)) {
      allNames.emplace_back(workspace->getName());
    }
  }
}

boost::optional<std::string>
checkWorkspaceSpectrumSize(MatrixWorkspace_const_sptr workspace) {
  if (workspace->y(0).size() < 2)
    return "Plot Spectra failed: There is only one data point to plot in " +
           workspace->getName() + ".";
  return boost::none;
}

boost::optional<std::string>
checkWorkspaceBinSize(MatrixWorkspace_const_sptr workspace) {
  if (workspace->getNumberHistograms() < 2)
    return "Plot Bins failed: There is only one data point to plot in " +
           workspace->getName() + ".";
  return boost::none;
}

std::map<std::string, std::string>
constructActions(boost::optional<std::map<std::string, std::string>> const
                     &availableActions) {
  std::map<std::string, std::string> actions;
  if (availableActions)
    actions = availableActions.get();
  if (actions.find("Plot Spectra") == actions.end())
    actions["Plot Spectra"] = "Plot Spectra";
  if (actions.find("Plot Bins") == actions.end())
    actions["Plot Bins"] = "Plot Bins";
  if (actions.find("Plot Contour") == actions.end())
    actions["Plot Contour"] = "Plot Contour";
  if (actions.find("Plot Tiled") == actions.end())
    actions["Plot Tiled"] = "Plot Tiled";
  return actions;
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {

IndirectPlotOptionsModel::IndirectPlotOptionsModel(
    IPyRunner *pythonRunner,
    boost::optional<std::map<std::string, std::string>> const &availableActions)
    : m_actions(constructActions(availableActions)), m_fixedIndices(false),
      m_workspaceIndices(boost::none), m_workspaceName(boost::none),
      m_plotter(std::make_unique<IndirectPlotter>(pythonRunner)) {}

/// Used by the unit tests so that m_plotter can be mocked
IndirectPlotOptionsModel::IndirectPlotOptionsModel(
    IndirectPlotter *plotter,
    boost::optional<std::map<std::string, std::string>> const &availableActions)
    : m_actions(constructActions(availableActions)), m_fixedIndices(false),
      m_workspaceIndices(boost::none), m_workspaceName(boost::none),
      m_plotter(std::move(plotter)) {}

IndirectPlotOptionsModel::~IndirectPlotOptionsModel() {}

bool IndirectPlotOptionsModel::setWorkspace(std::string const &workspaceName) {
  auto &ads = AnalysisDataService::Instance();
  if (ads.doesExist(workspaceName) &&
      ads.retrieveWS<MatrixWorkspace>(workspaceName)) {
    m_workspaceName = workspaceName;
    return true;
  }
  return false;
}

boost::optional<std::string> IndirectPlotOptionsModel::workspace() const {
  return m_workspaceName;
}

void IndirectPlotOptionsModel::removeWorkspace() {
  m_workspaceName = boost::none;
}

std::vector<std::string> IndirectPlotOptionsModel::getAllWorkspaceNames(
    std::vector<std::string> const &workspaceNames) const {
  std::vector<std::string> allNames;
  for (auto const &workspaceName : workspaceNames)
    insertWorkspaceNames(allNames, workspaceName);
  return allNames;
}

std::string
IndirectPlotOptionsModel::formatIndices(std::string const &indices) const {
  return formatIndicesString(indices);
}

void IndirectPlotOptionsModel::setFixedIndices(std::string const &indices) {
  m_fixedIndices = !indices.empty();
  if (m_fixedIndices)
    m_workspaceIndices = indices;
}

bool IndirectPlotOptionsModel::indicesFixed() const { return m_fixedIndices; }

bool IndirectPlotOptionsModel::setIndices(std::string const &indices) {
  bool valid = validateIndices(indices);
  if (valid)
    m_workspaceIndices = indices;
  else
    m_workspaceIndices = boost::none;
  return valid;
}

boost::optional<std::string> IndirectPlotOptionsModel::indices() const {
  return m_workspaceIndices;
}

bool IndirectPlotOptionsModel::validateIndices(
    std::string const &indices, MantidAxis const &axisType) const {
  auto &ads = AnalysisDataService::Instance();
  if (!indices.empty() && m_workspaceName &&
      ads.doesExist(m_workspaceName.get())) {
    if (auto const workspace =
            ads.retrieveWS<MatrixWorkspace>(m_workspaceName.get())) {
      if (axisType == MantidAxis::Spectrum)
        return validateSpectra(workspace, indices);
      return validateBins(workspace, indices);
    }
  }
  return false;
}

bool IndirectPlotOptionsModel::validateSpectra(
    MatrixWorkspace_sptr workspace, std::string const &spectra) const {
  auto const numberOfHistograms = workspace->getNumberHistograms();
  auto const lastIndex = std::stoul(splitStringBy(spectra, ",-").back());
  return lastIndex < numberOfHistograms;
}

bool IndirectPlotOptionsModel::validateBins(MatrixWorkspace_sptr workspace,
                                            std::string const &bins) const {
  auto const numberOfBins = workspace->y(0).size();
  auto const lastIndex = std::stoul(splitStringBy(bins, ",-").back());
  return lastIndex < numberOfBins;
}

void IndirectPlotOptionsModel::plotSpectra() {
  auto const workspaceName = workspace();
  auto const indicesString = indices();
  if (workspaceName && indicesString)
    m_plotter->plotSpectra(workspaceName.get(), indicesString.get());
}

void IndirectPlotOptionsModel::plotBins(std::string const &binIndices) {
  if (auto const workspaceName = workspace())
    m_plotter->plotBins(workspaceName.get(), binIndices);
}

void IndirectPlotOptionsModel::plotContour() {
  if (auto const workspaceName = workspace())
    m_plotter->plotContour(workspaceName.get());
}

void IndirectPlotOptionsModel::plotTiled() {
  auto const workspaceName = workspace();
  auto const indicesString = indices();
  if (workspaceName && indicesString)
    m_plotter->plotTiled(workspaceName.get(), indicesString.get());
}

boost::optional<std::string>
IndirectPlotOptionsModel::singleDataPoint(MantidAxis const &axisType) const {
  if (auto const workspaceName = workspace())
    return checkWorkspaceSize(workspaceName.get(), axisType);
  return boost::none;
}

boost::optional<std::string>
IndirectPlotOptionsModel::checkWorkspaceSize(std::string const &workspaceName,
                                             MantidAxis const &axisType) const {
  auto &ads = AnalysisDataService::Instance();
  if (ads.doesExist(workspaceName)) {
    if (auto const workspace = ads.retrieveWS<MatrixWorkspace>(workspaceName)) {
      if (axisType == MantidAxis::Spectrum)
        return checkWorkspaceSpectrumSize(workspace);
      return checkWorkspaceBinSize(workspace);
    }
  }
  return boost::none;
}

std::map<std::string, std::string>
IndirectPlotOptionsModel::availableActions() const {
  return m_actions;
}

} // namespace CustomInterfaces
} // namespace MantidQt
