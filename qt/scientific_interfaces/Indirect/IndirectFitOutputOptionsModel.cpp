// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitOutputOptionsModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/Workspace.h"

using namespace Mantid::API;

namespace {

std::string noWorkspaceErrorMessage(std::string const &process) {
  return "The " + process + " of a workspace failed:\n\n No workspace found";
}

MatrixWorkspace_sptr convertToMatrixWorkspace(Workspace_sptr workspace) {
  return boost::dynamic_pointer_cast<MatrixWorkspace>(workspace);
}

WorkspaceGroup_sptr convertToGroupWorkspace(Workspace_sptr workspace) {
  return boost::dynamic_pointer_cast<WorkspaceGroup>(workspace);
}

Workspace_sptr getADSWorkspace(std::string const &workspaceName) {
  return AnalysisDataService::Instance().retrieveWS<Workspace>(workspaceName);
}

MatrixWorkspace_sptr getADSMatrixWorkspace(std::string const &workspaceName) {
  return convertToMatrixWorkspace(getADSWorkspace(workspaceName));
}

WorkspaceGroup_sptr getADSGroupWorkspace(std::string const &workspaceName) {
  return convertToGroupWorkspace(getADSWorkspace(workspaceName));
}

std::unordered_map<std::string, std::size_t> extractAxisLabels(Axis *axis) {
  auto const *textAxis = boost::static_pointer_cast<TextAxis>(axis);
  std::unordered_map<std::string, std::size_t> labels;

  for (auto i = 0u; i < textAxis->length(); ++i)
    labels[textAxis->label(i)] = i;
  return labels;
}

std::unordered_map<std::string, std::size_t>
extractAxisLabels(MatrixWorkspace_const_sptr workspace,
                  std::size_t const &axisIndex) {
  auto const axis = workspace->getAxis(axisIndex);
  if (axis->isText())
    return extractAxisLabels(axis);
  return std::unordered_map<std::string, std::size_t>();
}

std::vector<std::string> extractParameterNames(Axis *axis) {
  auto const *textAxis = boost::static_pointer_cast<TextAxis>(axis);
  std::vector<std::string> parameters;

  for (auto i = 0u; i < textAxis->length(); ++i)
    parameters.emplace_back(textAxis->label(i));
  return parameters;
}

std::vector<std::string> extractParameterNames(MatrixWorkspace_sptr workspace) {
  auto const axis = workspace->getAxis(1);
  if (axis->isText())
    return extractParameterNames(axis);
  return std::vector<std::string>();
}

std::vector<std::string> extractParameterNames(Workspace_sptr workspace) {
  return extractParameterNames(convertToMatrixWorkspace(workspace));
}

IAlgorithm_sptr saveNexusProcessedAlgorithm(Workspace_sptr workspace,
                                            std::string const &filename) {
  auto saveAlg = AlgorithmManager::Instance().create("SaveNexusProcessed");
  saveAlg->setProperty("InputWorkspace", workspace);
  saveAlg->setProperty("Filename", filename);
  return saveAlg;
}

void saveWorkspace(Workspace_sptr workspace) {
  auto const filename = Mantid::Kernel::ConfigService::Instance().getString(
                            "defaultsave.directory") +
                        workspace->getName() + ".nxs";
  saveNexusProcessedAlgorithm(workspace, filename)->execute();
}

void saveWorkspacesInGroup(WorkspaceGroup_const_sptr group) {
  for (auto const workspace : *group)
    saveWorkspace(workspace);
}

bool workspaceIsPlottable(MatrixWorkspace_const_sptr workspace) {
  return workspace->y(0).size() > 1;
}

bool containsPlottableWorkspace(WorkspaceGroup_const_sptr groupWorkspace) {
  for (auto const &workspace : *groupWorkspace)
    if (workspaceIsPlottable(convertToMatrixWorkspace(workspace)))
      return true;
  return false;
}

std::vector<std::string>
validateInputs(std::string const &inputWorkspaceName,
               std::string const &singleFitWorkspaceName,
               std::string const &outputName) {
  std::vector<std::string> errors;

  if (inputWorkspaceName.empty())
    errors.emplace_back("Select a valid input workspace.");
  if (singleFitWorkspaceName.empty())
    errors.emplace_back("Select a valid Single Fit Result workspace.");
  if (outputName.empty())
    errors.emplace_back("Enter a valid output workspace name.");

  return errors;
}

IAlgorithm_sptr replaceAlgorithm(MatrixWorkspace_sptr inputWorkspace,
                                 MatrixWorkspace_sptr singleFitWorkspace,
                                 std::string const &outputName) {
  auto replaceAlg =
      AlgorithmManager::Instance().create("IndirectReplaceFitResult");
  replaceAlg->setProperty("InputWorkspace", inputWorkspace);
  replaceAlg->setProperty("SingleFitWorkspace", singleFitWorkspace);
  replaceAlg->setProperty("OutputWorkspace", outputName);
  return replaceAlg;
}

template <typename Predicate>
void removeVectorElements(std::vector<std::string> &strings,
                          Predicate const &filter) {
  strings.erase(std::remove_if(strings.begin(), strings.end(), filter),
                strings.end());
}

bool doesStringEndWith(std::string const &str, std::string const &delimiter) {
  if (str.size() > delimiter.size())
    return str.substr(str.size() - delimiter.size(), str.size()) == delimiter;
  return false;
}

std::vector<std::string> filterByEndSuffix(std::vector<std::string> &strings,
                                           std::string const &delimiter) {
  removeVectorElements(strings, [&delimiter](std::string const &str) {
    return !doesStringEndWith(str, delimiter);
  });
  return strings;
}

bool doesGroupContain(std::string const &groupName,
                      MatrixWorkspace_sptr workspace) {
  auto const adsWorkspace = getADSWorkspace(groupName);
  if (adsWorkspace->isGroup()) {
    auto const group =
        boost::dynamic_pointer_cast<WorkspaceGroup>(adsWorkspace);
    return group->contains(workspace);
  }
  return false;
}

std::string filterByContents(std::vector<std::string> &strings,
                             MatrixWorkspace_sptr workspace) {
  removeVectorElements(strings, [&workspace](std::string const &str) {
    return !doesGroupContain(str, workspace);
  });
  return !strings.empty() ? strings[0] : "";
}

std::string findGroupWorkspaceContaining(MatrixWorkspace_sptr workspace) {
  auto workspaceNames = AnalysisDataService::Instance().getObjectNames();
  auto resultGroups = filterByEndSuffix(workspaceNames, "_Results");
  return !resultGroups.empty() ? filterByContents(resultGroups, workspace) : "";
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectFitOutputOptionsModel::IndirectFitOutputOptionsModel()
    : m_resultGroup(), m_pdfGroup(), m_spectraToPlot() {}

void IndirectFitOutputOptionsModel::setResultWorkspace(
    WorkspaceGroup_sptr groupWorkspace) {
  m_resultGroup = groupWorkspace;
}

void IndirectFitOutputOptionsModel::setPDFWorkspace(
    WorkspaceGroup_sptr groupWorkspace) {
  m_pdfGroup = groupWorkspace;
}

WorkspaceGroup_sptr IndirectFitOutputOptionsModel::getResultWorkspace() const {
  return m_resultGroup;
}

WorkspaceGroup_sptr IndirectFitOutputOptionsModel::getPDFWorkspace() const {
  return m_pdfGroup;
}

void IndirectFitOutputOptionsModel::removePDFWorkspace() { m_pdfGroup.reset(); }

bool IndirectFitOutputOptionsModel::isResultGroupPlottable() const {
  if (m_resultGroup)
    return containsPlottableWorkspace(m_resultGroup);
  return false;
}

bool IndirectFitOutputOptionsModel::isPDFGroupPlottable() const {
  if (m_pdfGroup)
    return containsPlottableWorkspace(m_pdfGroup);
  return false;
}

void IndirectFitOutputOptionsModel::clearSpectraToPlot() {
  m_spectraToPlot.clear();
}

std::vector<SpectrumToPlot>
IndirectFitOutputOptionsModel::getSpectraToPlot() const {
  return m_spectraToPlot;
}

void IndirectFitOutputOptionsModel::plotResult(std::string const &plotType) {
  if (m_resultGroup)
    plotResult(m_resultGroup, plotType);
  else
    throw std::runtime_error(noWorkspaceErrorMessage("plotting"));
}

void IndirectFitOutputOptionsModel::plotResult(
    WorkspaceGroup_const_sptr groupWorkspace, std::string const &plotType) {
  if (plotType == "All")
    plotAll(groupWorkspace);
  else
    plotParameter(groupWorkspace, plotType);
}

void IndirectFitOutputOptionsModel::plotAll(
    WorkspaceGroup_const_sptr groupWorkspace) {
  for (auto const &workspace : *groupWorkspace)
    plotAll(convertToMatrixWorkspace(workspace));
}

void IndirectFitOutputOptionsModel::plotAll(
    MatrixWorkspace_const_sptr workspace) {
  if (workspaceIsPlottable(workspace))
    plotAllSpectra(workspace);
}

void IndirectFitOutputOptionsModel::plotAllSpectra(
    MatrixWorkspace_const_sptr workspace) {
  for (auto index = 0u; index < workspace->getNumberHistograms(); ++index) {
    auto const plotInfo = std::make_pair(workspace->getName(), index);
    m_spectraToPlot.emplace_back(plotInfo);
  }
}

void IndirectFitOutputOptionsModel::plotParameter(
    WorkspaceGroup_const_sptr groupWorkspace, std::string const &parameter) {
  for (auto const &workspace : *groupWorkspace)
    plotParameter(convertToMatrixWorkspace(workspace), parameter);
}

void IndirectFitOutputOptionsModel::plotParameter(
    MatrixWorkspace_const_sptr workspace, std::string const &parameter) {
  if (workspaceIsPlottable(workspace))
    plotParameterSpectrum(workspace, parameter);
}

void IndirectFitOutputOptionsModel::plotParameterSpectrum(
    MatrixWorkspace_const_sptr workspace, std::string const &parameter) {
  auto const parameters = extractAxisLabels(workspace, 1);
  auto const iter = parameters.find(parameter);
  if (iter != parameters.end()) {
    auto const plotInfo = std::make_pair(workspace->getName(), iter->second);
    m_spectraToPlot.emplace_back(plotInfo);
  }
}

void IndirectFitOutputOptionsModel::plotPDF(std::string const &workspaceName,
                                            std::string const &plotType) {
  if (m_pdfGroup) {
    auto const workspace = m_pdfGroup->getItem(workspaceName);
    plotPDF(convertToMatrixWorkspace(workspace), plotType);
  } else
    throw std::runtime_error(noWorkspaceErrorMessage("plotting"));
}

void IndirectFitOutputOptionsModel::plotPDF(
    MatrixWorkspace_const_sptr workspace, std::string const &plotType) {
  if (plotType == "All")
    plotAll(workspace);
  else
    plotParameter(workspace, plotType);
}

void IndirectFitOutputOptionsModel::saveResult() const {
  if (m_resultGroup)
    saveWorkspacesInGroup(m_resultGroup);
  else
    throw std::runtime_error(noWorkspaceErrorMessage("saving"));
}

std::vector<std::string> IndirectFitOutputOptionsModel::getWorkspaceParameters(
    std::string const &selectedGroup) const {
  if (isResultGroupSelected(selectedGroup) && m_resultGroup)
    return extractParameterNames(m_resultGroup->getItem(0));
  else if (!isResultGroupSelected(selectedGroup) && m_pdfGroup)
    return extractParameterNames(m_pdfGroup->getItem(0));
  return std::vector<std::string>();
}

std::vector<std::string>
IndirectFitOutputOptionsModel::getPDFWorkspaceNames() const {
  if (m_pdfGroup)
    return m_pdfGroup->getNames();
  return std::vector<std::string>();
}

bool IndirectFitOutputOptionsModel::isResultGroupSelected(
    std::string const &selectedGroup) const {
  return selectedGroup == "Result Group";
}

void IndirectFitOutputOptionsModel::replaceFitResult(
    std::string const &inputName, std::string const &singleBinName,
    std::string const &outputName) {
  auto const errors = validateInputs(inputName, singleBinName, outputName);
  if (errors.empty())
    replaceFitResult(getADSMatrixWorkspace(inputName),
                     getADSMatrixWorkspace(singleBinName), outputName);
  else
    throw std::runtime_error(errors[0]);
}

void IndirectFitOutputOptionsModel::replaceFitResult(
    MatrixWorkspace_sptr inputWorkspace,
    MatrixWorkspace_sptr singleFitWorkspace, std::string const &outputName) {
  auto const replaceAlg =
      replaceAlgorithm(inputWorkspace, singleFitWorkspace, outputName);
  replaceAlg->execute();
  setOutputAsResultWorkspace(replaceAlg);
}

void IndirectFitOutputOptionsModel::setOutputAsResultWorkspace(
    IAlgorithm_sptr algorithm) {
  auto const outputName = algorithm->getPropertyValue("OutputWorkspace");
  auto const output = getADSMatrixWorkspace(outputName);
  setResultWorkspace(findGroupWorkspaceContaining(output));
}

void IndirectFitOutputOptionsModel::setResultWorkspace(
    std::string const &groupName) {
  if (!groupName.empty())
    setResultWorkspace(getADSGroupWorkspace(groupName));
  else
    throw std::runtime_error("The result group could not be found in the ADS.");
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
