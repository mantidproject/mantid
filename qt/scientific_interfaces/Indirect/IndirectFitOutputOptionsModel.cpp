// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitOutputOptionsModel.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectFitOutputOptionsModel::IndirectFitOutputOptionsModel() {}

void IndirectFitOutputOptionsModel::plotResult(std::string const &plotType) {
  auto const resultWorkspaces = m_fittingModel->getResultWorkspace();
  if (resultWorkspaces) {
    if (plotType == "All")
      plotAll(resultWorkspaces);
    else
      plotParameter(resultWorkspaces, plotType);
  }
}

void IndirectFitOutputOptionsModel::plotAll(WorkspaceGroup_sptr workspaces) {
  for (auto const &workspace : *workspaces)
    plotAll(convertToMatrixWorkspace(workspace));
}

void IndirectFitOutputOptionsModel::plotParameter(
    WorkspaceGroup_sptr workspaces, std::string const &parameter) {
  for (auto const &workspace : *workspaces)
    plotParameter(convertToMatrixWorkspace(workspace), parameter);
}

void IndirectFitOutputOptionsModel::plotAll(MatrixWorkspace_sptr workspace) {
  auto const numberOfDataPoints = workspace->blocksize();
  if (numberOfDataPoints > 1)
    plotSpectrum(workspace);
  else
    showMessageBox(
        "The plotting of data in one of the result workspaces failed:\n\n "
        "Workspace has only one data point");
}

void IndirectFitOutputOptionsModel::plotParameter(
    MatrixWorkspace_sptr workspace, std::string const &parameterToPlot) {
  auto const numberOfDataPoints = workspace->blocksize();
  if (numberOfDataPoints > 1)
    plotSpectrum(workspace, parameterToPlot);
  else
    showMessageBox(
        "The plotting of data in one of the result workspaces failed:\n\n "
        "Workspace has only one data point");
}

void IndirectFitOutputOptionsModel::plotSpectrum(
    MatrixWorkspace_sptr workspace, std::string const &parameterToPlot) {
  auto const name = QString::fromStdString(workspace->getName());
  auto const labels = IndirectTab::extractAxisLabels(workspace, 1);
  for (auto const &parameter : m_fittingModel->getFitParameterNames()) {
    if (boost::contains(parameter, parameterToPlot)) {
      auto it = labels.find(parameter);
      if (it != labels.end())
        IndirectTab::plotSpectrum(name, static_cast<int>(it->second), true);
    }
  }
}

void IndirectFitOutputOptionsModel::plotSpectrum(
    MatrixWorkspace_sptr workspace) {
  auto const name = QString::fromStdString(workspace->getName());
  for (auto i = 0u; i < workspace->getNumberHistograms(); ++i)
    IndirectTab::plotSpectrum(name, static_cast<int>(i), true);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
