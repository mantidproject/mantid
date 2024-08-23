// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FitOutputOptionsModel.h"

#include <utility>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/Workspace.h"

#include <memory>

using namespace Mantid::API;

namespace {

std::string noWorkspaceErrorMessage(std::string const &process) {
  return "The " + process + " of a workspace failed:\n\n No workspace found";
}

MatrixWorkspace_sptr convertToMatrixWorkspace(const Workspace_sptr &workspace) {
  return std::dynamic_pointer_cast<MatrixWorkspace>(workspace);
}

WorkspaceGroup_sptr convertToGroupWorkspace(const Workspace_sptr &workspace) {
  return std::dynamic_pointer_cast<WorkspaceGroup>(workspace);
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
  auto const *textAxis = static_cast<TextAxis *>(axis);
  std::unordered_map<std::string, std::size_t> labels;

  for (auto i = 0u; i < textAxis->length(); ++i)
    labels[textAxis->label(i)] = i;
  return labels;
}

std::unordered_map<std::string, std::size_t> extractAxisLabels(const MatrixWorkspace_const_sptr &workspace,
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

std::vector<std::string> extractParameterNames(const MatrixWorkspace_sptr &workspace) {
  auto const axis = workspace->getAxis(1);
  if (axis->isText())
    return extractParameterNames(axis);
  return std::vector<std::string>();
}

std::vector<std::string> extractParameterNames(const Workspace_sptr &workspace) {
  return extractParameterNames(convertToMatrixWorkspace(workspace));
}

IAlgorithm_sptr saveNexusProcessedAlgorithm(const Workspace_sptr &workspace, std::string const &filename) {
  auto saveAlg = AlgorithmManager::Instance().create("SaveNexusProcessed");
  saveAlg->setProperty("InputWorkspace", workspace);
  saveAlg->setProperty("Filename", filename);
  return saveAlg;
}

void saveWorkspace(const Workspace_sptr &workspace) {
  auto const filename =
      Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory") + workspace->getName() + ".nxs";
  saveNexusProcessedAlgorithm(workspace, filename)->execute();
}

void saveWorkspacesInGroup(const WorkspaceGroup_const_sptr &group) {
  for (auto const &workspace : *group)
    saveWorkspace(workspace);
}

bool workspaceIsPlottable(const MatrixWorkspace_const_sptr &workspace) { return workspace->y(0).size() > 1; }

bool containsPlottableWorkspace(const WorkspaceGroup_const_sptr &groupWorkspace) {
  return std::any_of(groupWorkspace->begin(), groupWorkspace->end(),
                     [](auto const &workspace) { return workspaceIsPlottable(convertToMatrixWorkspace(workspace)); });
}

std::vector<std::string> validateInputs(std::string const &inputWorkspaceName,
                                        std::string const &singleFitWorkspaceName, std::string const &outputName) {
  std::vector<std::string> errors;

  if (inputWorkspaceName.empty())
    errors.emplace_back("Select a valid input workspace.");
  if (singleFitWorkspaceName.empty())
    errors.emplace_back("Select a valid Single Fit Result workspace.");
  if (outputName.empty())
    errors.emplace_back("Enter a valid output workspace name.");

  return errors;
}

IAlgorithm_sptr replaceAlgorithm(const MatrixWorkspace_sptr &inputWorkspace,
                                 const MatrixWorkspace_sptr &singleFitWorkspace, std::string const &outputName) {
  auto replaceAlg = AlgorithmManager::Instance().create("IndirectReplaceFitResult");
  replaceAlg->setProperty("InputWorkspace", inputWorkspace);
  replaceAlg->setProperty("SingleFitWorkspace", singleFitWorkspace);
  replaceAlg->setProperty("OutputWorkspace", outputName);
  return replaceAlg;
}

template <typename Predicate> void removeVectorElements(std::vector<std::string> &strings, Predicate const &filter) {
  strings.erase(std::remove_if(strings.begin(), strings.end(), filter), strings.end());
}

bool doesStringEndWith(std::string const &str, std::string const &delimiter) {
  if (str.size() > delimiter.size())
    return str.substr(str.size() - delimiter.size(), str.size()) == delimiter;
  return false;
}

std::vector<std::string> filterByEndSuffix(std::vector<std::string> &strings, std::string const &delimiter) {
  removeVectorElements(strings, [&delimiter](std::string const &str) { return !doesStringEndWith(str, delimiter); });
  return strings;
}

bool doesGroupContain(std::string const &groupName, const MatrixWorkspace_sptr &workspace) {
  auto const adsWorkspace = getADSWorkspace(groupName);
  if (adsWorkspace->isGroup()) {
    auto const group = std::dynamic_pointer_cast<WorkspaceGroup>(adsWorkspace);
    return group->contains(workspace);
  }
  return false;
}

std::string filterByContents(std::vector<std::string> &strings, MatrixWorkspace_sptr workspace) {
  removeVectorElements(strings, [&workspace](std::string const &str) { return !doesGroupContain(str, workspace); });
  return !strings.empty() ? strings[0] : "";
}

std::string findGroupWorkspaceContaining(MatrixWorkspace_sptr workspace) {
  auto workspaceNames = AnalysisDataService::Instance().getObjectNames();
  auto resultGroups = filterByEndSuffix(workspaceNames, "_Results");
  return !resultGroups.empty() ? filterByContents(resultGroups, std::move(workspace)) : "";
}

} // namespace

namespace MantidQt::CustomInterfaces::Inelastic {

FitOutputOptionsModel::FitOutputOptionsModel() : m_resultGroup(), m_pdfGroup() {}

void FitOutputOptionsModel::setResultWorkspace(WorkspaceGroup_sptr groupWorkspace) { m_resultGroup = groupWorkspace; }

void FitOutputOptionsModel::setPDFWorkspace(WorkspaceGroup_sptr groupWorkspace) { m_pdfGroup = groupWorkspace; }

WorkspaceGroup_sptr FitOutputOptionsModel::getResultWorkspace() const { return m_resultGroup; }

WorkspaceGroup_sptr FitOutputOptionsModel::getPDFWorkspace() const { return m_pdfGroup; }

void FitOutputOptionsModel::removePDFWorkspace() { m_pdfGroup.reset(); }

bool FitOutputOptionsModel::isSelectedGroupPlottable(std::string const &selectedGroup) const {
  return isResultGroupSelected(selectedGroup) ? isResultGroupPlottable() : isPDFGroupPlottable();
}

bool FitOutputOptionsModel::isResultGroupPlottable() const {
  if (m_resultGroup)
    return containsPlottableWorkspace(m_resultGroup);
  return false;
}

bool FitOutputOptionsModel::isPDFGroupPlottable() const {
  if (m_pdfGroup)
    return containsPlottableWorkspace(m_pdfGroup);
  return false;
}

std::vector<SpectrumToPlot> FitOutputOptionsModel::plotResult(std::string const &plotType) const {
  if (!m_resultGroup) {
    throw std::runtime_error(noWorkspaceErrorMessage("plotting"));
  }
  std::vector<SpectrumToPlot> spectraToPlot;
  plotResult(spectraToPlot, m_resultGroup, plotType);
  return spectraToPlot;
}

void FitOutputOptionsModel::plotResult(std::vector<SpectrumToPlot> &spectraToPlot,
                                       const WorkspaceGroup_const_sptr &groupWorkspace,
                                       std::string const &plotType) const {
  if (plotType == "All")
    plotAll(spectraToPlot, groupWorkspace);
  else
    plotParameter(spectraToPlot, groupWorkspace, plotType);
}

void FitOutputOptionsModel::plotAll(std::vector<SpectrumToPlot> &spectraToPlot,
                                    const WorkspaceGroup_const_sptr &groupWorkspace) const {
  for (auto const &workspace : *groupWorkspace)
    plotAll(spectraToPlot, convertToMatrixWorkspace(workspace));
}

void FitOutputOptionsModel::plotAll(std::vector<SpectrumToPlot> &spectraToPlot,
                                    const MatrixWorkspace_const_sptr &workspace) const {
  if (workspaceIsPlottable(workspace))
    plotAllSpectra(spectraToPlot, workspace);
}

void FitOutputOptionsModel::plotAllSpectra(std::vector<SpectrumToPlot> &spectraToPlot,
                                           const MatrixWorkspace_const_sptr &workspace) const {
  for (auto index = 0u; index < workspace->getNumberHistograms(); ++index) {
    auto const plotInfo = std::make_pair(workspace->getName(), index);
    spectraToPlot.emplace_back(plotInfo);
  }
}

void FitOutputOptionsModel::plotParameter(std::vector<SpectrumToPlot> &spectraToPlot,
                                          const WorkspaceGroup_const_sptr &groupWorkspace,
                                          std::string const &parameter) const {
  for (auto const &workspace : *groupWorkspace)
    plotParameter(spectraToPlot, convertToMatrixWorkspace(workspace), parameter);
}

void FitOutputOptionsModel::plotParameter(std::vector<SpectrumToPlot> &spectraToPlot,
                                          const MatrixWorkspace_const_sptr &workspace,
                                          std::string const &parameter) const {
  if (workspaceIsPlottable(workspace))
    plotParameterSpectrum(spectraToPlot, workspace, parameter);
}

void FitOutputOptionsModel::plotParameterSpectrum(std::vector<SpectrumToPlot> &spectraToPlot,
                                                  const MatrixWorkspace_const_sptr &workspace,
                                                  std::string const &parameter) const {
  auto const parameters = extractAxisLabels(workspace, 1);
  auto const iter = parameters.find(parameter);
  if (iter != parameters.end()) {
    auto const plotInfo = std::make_pair(workspace->getName(), iter->second);
    spectraToPlot.emplace_back(plotInfo);
  }
}

std::vector<SpectrumToPlot> FitOutputOptionsModel::plotPDF(std::string const &workspaceName,
                                                           std::string const &plotType) const {
  if (!m_pdfGroup) {
    throw std::runtime_error(noWorkspaceErrorMessage("plotting"));
  }
  std::vector<SpectrumToPlot> spectraToPlot;
  plotPDF(spectraToPlot, convertToMatrixWorkspace(m_pdfGroup->getItem(workspaceName)), plotType);
  return spectraToPlot;
}

void FitOutputOptionsModel::plotPDF(std::vector<SpectrumToPlot> &spectraToPlot,
                                    const MatrixWorkspace_const_sptr &workspace, std::string const &plotType) const {
  if (plotType == "All")
    plotAll(spectraToPlot, workspace);
  else
    plotParameter(spectraToPlot, workspace, plotType);
}

void FitOutputOptionsModel::saveResult() const {
  if (m_resultGroup)
    saveWorkspacesInGroup(m_resultGroup);
  else
    throw std::runtime_error(noWorkspaceErrorMessage("saving"));
}

std::vector<std::string> FitOutputOptionsModel::getWorkspaceParameters(std::string const &selectedGroup) const {
  if (isResultGroupSelected(selectedGroup) && m_resultGroup)
    return extractParameterNames(m_resultGroup->getItem(0));
  else if (!isResultGroupSelected(selectedGroup) && m_pdfGroup)
    return extractParameterNames(m_pdfGroup->getItem(0));
  return std::vector<std::string>();
}

std::vector<std::string> FitOutputOptionsModel::getPDFWorkspaceNames() const {
  if (m_pdfGroup)
    return m_pdfGroup->getNames();
  return std::vector<std::string>();
}

bool FitOutputOptionsModel::isResultGroupSelected(std::string const &selectedGroup) const {
  return selectedGroup == "Result Group";
}

void FitOutputOptionsModel::replaceFitResult(std::string const &inputName, std::string const &singleBinName,
                                             std::string const &outputName) {
  auto const errors = validateInputs(inputName, singleBinName, outputName);
  if (errors.empty())
    replaceFitResult(getADSMatrixWorkspace(inputName), getADSMatrixWorkspace(singleBinName), outputName);
  else
    throw std::runtime_error(errors[0]);
}

void FitOutputOptionsModel::replaceFitResult(const MatrixWorkspace_sptr &inputWorkspace,
                                             const MatrixWorkspace_sptr &singleFitWorkspace,
                                             std::string const &outputName) {
  auto const replaceAlg = replaceAlgorithm(inputWorkspace, singleFitWorkspace, outputName);
  replaceAlg->execute();
  setOutputAsResultWorkspace(replaceAlg);
}

void FitOutputOptionsModel::setOutputAsResultWorkspace(const IAlgorithm_sptr &algorithm) {
  auto const outputName = algorithm->getPropertyValue("OutputWorkspace");
  auto const output = getADSMatrixWorkspace(outputName);
  setResultWorkspace(findGroupWorkspaceContaining(output));
}

void FitOutputOptionsModel::setResultWorkspace(std::string const &groupName) {
  if (!groupName.empty())
    setResultWorkspace(getADSGroupWorkspace(groupName));
  else
    throw std::runtime_error("The result group could not be found in the ADS.");
}

} // namespace MantidQt::CustomInterfaces::Inelastic
