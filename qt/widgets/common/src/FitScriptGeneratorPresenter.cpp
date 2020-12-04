// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FitScriptGeneratorPresenter.h"
#include "MantidQtWidgets/Common/FitScriptGeneratorModel.h"
#include "MantidQtWidgets/Common/IFitScriptGeneratorView.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <stdexcept>

using namespace Mantid::API;

namespace MantidQt {
namespace MantidWidgets {

FitScriptGeneratorPresenter::FitScriptGeneratorPresenter(
    IFitScriptGeneratorView *view, IFitScriptGeneratorModel *model,
    QStringList const &workspaceNames, double startX, double endX)
    : m_warnings(), m_view(view), m_model(model) {
  m_view->subscribePresenter(this);
  setWorkspaces(workspaceNames, startX, endX);
}

FitScriptGeneratorPresenter::~FitScriptGeneratorPresenter() {}

void FitScriptGeneratorPresenter::notifyPresenter(ViewEvent const &event,
                                                  std::string const &arg) {
  if (arg.empty())
    UNUSED_ARG(arg);

  switch (event) {
  case ViewEvent::RemoveClicked:
    handleRemoveClicked();
    return;
  case ViewEvent::AddClicked:
    handleAddWorkspaceClicked();
    return;
  case ViewEvent::StartXChanged:
    handleStartXChanged();
    return;
  case ViewEvent::EndXChanged:
    handleEndXChanged();
    return;
  case ViewEvent::SelectionChanged:
    handleSelectionChanged();
    return;
  case ViewEvent::FunctionRemoved:
    handleFunctionRemoved(arg);
    return;
  case ViewEvent::FunctionAdded:
    handleFunctionAdded(arg);
    return;
  }

  throw std::runtime_error("Failed to notify the FitScriptGeneratorPresenter.");
}

void FitScriptGeneratorPresenter::openFitScriptGenerator() { m_view->show(); }

void FitScriptGeneratorPresenter::handleRemoveClicked() {
  for (auto const &index : m_view->selectedRows()) {
    auto const workspaceName = m_view->workspaceName(index);
    auto const workspaceIndex = m_view->workspaceIndex(index);

    m_view->removeWorkspaceDomain(workspaceName, workspaceIndex);
    m_model->removeWorkspaceDomain(workspaceName, workspaceIndex);
  }

  handleSelectionChanged();
}

void FitScriptGeneratorPresenter::handleAddWorkspaceClicked() {
  if (m_view->openAddWorkspaceDialog()) {
    auto const workspaces = m_view->getDialogWorkspaces();
    auto const workspaceIndices = m_view->getDialogWorkspaceIndices();

    if (!workspaces.empty() && !workspaceIndices.empty())
      addWorkspaces(workspaces, workspaceIndices);
  }
}

void FitScriptGeneratorPresenter::handleStartXChanged() {
  auto const selectedRows = m_view->selectedRows();
  if (!selectedRows.empty()) {
    auto const workspaceName = m_view->workspaceName(selectedRows[0]);
    auto const workspaceIndex = m_view->workspaceIndex(selectedRows[0]);
    auto const startX = m_view->startX(selectedRows[0]);

    updateStartX(workspaceName, workspaceIndex, startX);
  }
}

void FitScriptGeneratorPresenter::handleEndXChanged() {
  auto const selectedRows = m_view->selectedRows();
  if (!selectedRows.empty()) {
    auto const workspaceName = m_view->workspaceName(selectedRows[0]);
    auto const workspaceIndex = m_view->workspaceIndex(selectedRows[0]);
    auto const endX = m_view->endX(selectedRows[0]);

    updateEndX(workspaceName, workspaceIndex, endX);
  }
}

void FitScriptGeneratorPresenter::handleSelectionChanged() {
  auto const selectedRows = m_view->selectedRows();
  if (!selectedRows.empty()) {
    auto const workspaceName = m_view->workspaceName(selectedRows[0]);
    auto const workspaceIndex = m_view->workspaceIndex(selectedRows[0]);

    auto const composite = m_model->getFunction(workspaceName, workspaceIndex);
    m_view->setFunction(composite);
  } else {
    m_view->clearFunction();
  }
}

void FitScriptGeneratorPresenter::handleFunctionRemoved(
    std::string const &function) {
  auto const rowIndices = m_view->isAddFunctionToAllDomainsChecked()
                              ? m_view->allRows()
                              : m_view->selectedRows();

  for (auto const &rowIndex : rowIndices) {
    auto const workspaceName = m_view->workspaceName(rowIndex);
    auto const workspaceIndex = m_view->workspaceIndex(rowIndex);
    m_model->removeFunction(workspaceName, workspaceIndex, function);
  }
}

void FitScriptGeneratorPresenter::handleFunctionAdded(
    std::string const &function) {
  auto const rowIndices = m_view->isAddFunctionToAllDomainsChecked()
                              ? m_view->allRows()
                              : m_view->selectedRows();

  for (auto const &rowIndex : rowIndices) {
    auto const workspaceName = m_view->workspaceName(rowIndex);
    auto const workspaceIndex = m_view->workspaceIndex(rowIndex);
    m_model->addFunction(workspaceName, workspaceIndex, function);
  }
}

void FitScriptGeneratorPresenter::setWorkspaces(
    QStringList const &workspaceNames, double startX, double endX) {
  for (auto const &workspaceName : workspaceNames)
    addWorkspace(workspaceName.toStdString(), startX, endX);
  checkForWarningMessages();
}

void FitScriptGeneratorPresenter::addWorkspaces(
    std::vector<MatrixWorkspace_const_sptr> const &workspaces,
    std::vector<WorkspaceIndex> const &workspaceIndices) {
  for (auto const &workspace : workspaces)
    for (auto const &workspaceIndex : workspaceIndices) {
      auto const xData = workspace->x(workspaceIndex.value);
      addWorkspace(workspace, workspaceIndex, xData.front(), xData.back());
    }
  checkForWarningMessages();
}

void FitScriptGeneratorPresenter::addWorkspace(std::string const &workspaceName,
                                               double startX, double endX) {
  auto &ads = AnalysisDataService::Instance();
  if (ads.doesExist(workspaceName))
    addWorkspace(ads.retrieveWS<MatrixWorkspace>(workspaceName), startX, endX);
}

void FitScriptGeneratorPresenter::addWorkspace(
    MatrixWorkspace_const_sptr const &workspace, double startX, double endX) {
  for (auto index = 0u; index < workspace->getNumberHistograms(); ++index)
    addWorkspace(workspace, WorkspaceIndex{index}, startX, endX);
}

void FitScriptGeneratorPresenter::addWorkspace(
    MatrixWorkspace_const_sptr const &workspace, WorkspaceIndex workspaceIndex,
    double startX, double endX) {
  addWorkspace(workspace->getName(), workspaceIndex, startX, endX);
}

void FitScriptGeneratorPresenter::addWorkspace(std::string const &workspaceName,
                                               WorkspaceIndex workspaceIndex,
                                               double startX, double endX) {
  try {
    m_model->addWorkspaceDomain(workspaceName, workspaceIndex, startX, endX);
    m_view->addWorkspaceDomain(workspaceName, workspaceIndex, startX, endX);
  } catch (std::invalid_argument const &ex) {
    m_warnings.emplace_back(ex.what());
  }
}

void FitScriptGeneratorPresenter::updateStartX(std::string const &workspaceName,
                                               WorkspaceIndex workspaceIndex,
                                               double startX) {
  if (m_model->isStartXValid(workspaceName, workspaceIndex, startX))
    m_model->updateStartX(workspaceName, workspaceIndex, startX);
  else {
    m_view->resetSelection();
    m_view->displayWarning("The StartX provided must be within the x limits of "
                           "its workspace, and less than the EndX.");
  }
}

void FitScriptGeneratorPresenter::updateEndX(std::string const &workspaceName,
                                             WorkspaceIndex workspaceIndex,
                                             double endX) {
  if (m_model->isEndXValid(workspaceName, workspaceIndex, endX))
    m_model->updateEndX(workspaceName, workspaceIndex, endX);
  else {
    m_view->resetSelection();
    m_view->displayWarning("The EndX provided must be within the x limits of "
                           "its workspace, and greater than the StartX.");
  }
}

void FitScriptGeneratorPresenter::checkForWarningMessages() {
  if (!m_warnings.empty()) {
    std::stringstream ss;
    std::copy(m_warnings.cbegin(), m_warnings.cend(),
              std::ostream_iterator<std::string>(ss, "\n"));
    m_view->displayWarning(ss.str());
    m_warnings.clear();
  }
}

} // namespace MantidWidgets
} // namespace MantidQt
