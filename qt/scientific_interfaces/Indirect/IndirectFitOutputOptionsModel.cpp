// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitOutputOptionsModel.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/Workspace.h"

using namespace Mantid::API;

namespace {

std::string noWorkspaceErrorMessage() {
  return "This process has failed:\n\n No workspace found";
}

MatrixWorkspace_sptr convertToMatrixWorkspace(Workspace_sptr workspace) {
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
  else
    return std::unordered_map<std::string, std::size_t>();
}

IAlgorithm_sptr saveNexusProcessedAlgorithm(Workspace_sptr workspace,
                                            std::string const &filename) {
  auto saveAlg = AlgorithmManager::Instance().create("SaveNexusProcessed");
  saveAlg->setProperty("InputWorkspace", workspace);
  saveAlg->setProperty("Filename", filename);
  return saveAlg;
}

void saveWorkspace(WorkspaceGroup_sptr resultWorkspace) {
  auto const filename = Mantid::Kernel::ConfigService::Instance().getString(
                            "defaultsave.directory") +
                        resultWorkspace->getName() + ".nxs";
  saveNexusProcessedAlgorithm(resultWorkspace, filename)->execute();
}

bool workspaceIsPlottable(MatrixWorkspace_sptr workspace) {
  return workspace->y(0).size() > 1;
}

bool containsPlottableWorkspace(WorkspaceGroup_sptr group) {
  for (auto const &workspace : *group)
    if (workspaceIsPlottable(convertToMatrixWorkspace(workspace)))
      return true;
  return false;
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

// IndirectFitOutputOptionsModel::IndirectFitOutputOptionsModel(
//    std::unique_ptr<IndirectFitAnalysisTab> tab)
//    : m_tab(std::move(tab)) {}

IndirectFitOutputOptionsModel::IndirectFitOutputOptionsModel() {}

void IndirectFitOutputOptionsModel::setActivePlotWorkspace(
    WorkspaceGroup_sptr workspace) {
  m_plotWorkspace = workspace;
}

void IndirectFitOutputOptionsModel::setActiveParameters(
    std::vector<std::string> const &parameters) {
  m_parameters = parameters;
}

void IndirectFitOutputOptionsModel::plotResult(std::string const &plotType) {
  if (m_plotWorkspace) {
    if (plotType == "All")
      plotAll(m_plotWorkspace);
    else
      plotParameter(m_plotWorkspace, plotType);
  } else
    throw std::runtime_error(noWorkspaceErrorMessage());
}

void IndirectFitOutputOptionsModel::plotAll(WorkspaceGroup_sptr workspaces) {
  for (auto const &workspace : *workspaces)
    plotWorkspace(convertToMatrixWorkspace(workspace));
}

void IndirectFitOutputOptionsModel::plotParameter(
    WorkspaceGroup_sptr workspaces, std::string const &parameter) {
  for (auto const &workspace : *workspaces)
    plotWorkspace(convertToMatrixWorkspace(workspace), parameter);
}

void IndirectFitOutputOptionsModel::plotWorkspace(
    MatrixWorkspace_sptr workspace, std::string const &parameter) {
  if (workspaceIsPlottable(workspace)) {
    if (parameter.empty())
      plotSpectra(workspace);
    else
      plotSpectrum(workspace, parameter);
  }
}

void IndirectFitOutputOptionsModel::plotSpectra(
    MatrixWorkspace_sptr workspace) {
  for (auto index = 0u; index < workspace->getNumberHistograms(); ++index)
    break;
  // m_tab->plotSpectrum(workspace->getName(), index, true);
}

void IndirectFitOutputOptionsModel::plotSpectrum(
    MatrixWorkspace_sptr workspace, std::string const &parameterToPlot) {
  auto const labels = extractAxisLabels(workspace, 1);
  for (auto const &parameter : m_parameters) {
    if (parameter == parameterToPlot) {
      auto const param = labels.find(parameter);
      if (param != labels.end())
        break;
      // m_tab->plotSpectrum(workspace->getName(), param->second, true);
    }
  }
}

void IndirectFitOutputOptionsModel::saveResult() const {
  if (m_plotWorkspace)
    saveWorkspace(m_plotWorkspace);
  else
    throw std::runtime_error(noWorkspaceErrorMessage());
}

bool IndirectFitOutputOptionsModel::plotWorkspaceIsPlottable() const {
  if (m_plotWorkspace)
    return containsPlottableWorkspace(m_plotWorkspace);
  return false;
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
