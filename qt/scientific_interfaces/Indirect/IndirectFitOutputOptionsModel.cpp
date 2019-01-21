// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitOutputOptionsModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/Workspace.h"

using namespace Mantid::API;

namespace {

std::string noWorkspaceErrorMessage(std::string const &process) {
  return "The " + process +
         " of the result workspace failed:\n\n No workspace found";
}

MatrixWorkspace_const_sptr convertToMatrixWorkspace(Workspace_sptr workspace) {
  return boost::dynamic_pointer_cast<MatrixWorkspace>(workspace);
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

std::vector<std::string>
extractParameterNames(MatrixWorkspace_const_sptr workspace) {
  auto const axis = workspace->getAxis(1);
  if (axis->isText())
    return extractParameterNames(axis);
  return std::vector<std::string>();
}

std::vector<std::string> extractParameterNames(Workspace_sptr workspace) {
  return extractParameterNames(convertToMatrixWorkspace(workspace));
}

IAlgorithm_sptr saveNexusProcessedAlgorithm(Workspace_const_sptr workspace,
                                            std::string const &filename) {
  auto saveAlg = AlgorithmManager::Instance().create("SaveNexusProcessed");
  saveAlg->setProperty("InputWorkspace", workspace);
  saveAlg->setProperty("Filename", filename);
  return saveAlg;
}

void saveWorkspace(WorkspaceGroup_const_sptr resultWorkspace) {
  auto const filename = Mantid::Kernel::ConfigService::Instance().getString(
                            "defaultsave.directory") +
                        resultWorkspace->getName() + ".nxs";
  saveNexusProcessedAlgorithm(resultWorkspace, filename)->execute();
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

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectFitOutputOptionsModel::IndirectFitOutputOptionsModel()
    : m_resultGroup(), m_spectraToPlot() {}

void IndirectFitOutputOptionsModel::setResultWorkspace(
    WorkspaceGroup_sptr groupWorkspace) {
  m_resultGroup = groupWorkspace;
}

void IndirectFitOutputOptionsModel::setPDFWorkspace(
    WorkspaceGroup_sptr groupWorkspace) {
  m_pdfGroup = groupWorkspace;
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
    saveWorkspace(m_resultGroup);
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

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
