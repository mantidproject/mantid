// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FitScriptGeneratorPresenter.h"
#include "MantidQtWidgets/Common/FitScriptGeneratorModel.h"
#include "MantidQtWidgets/Common/FittingGlobals.h"
#include "MantidQtWidgets/Common/IFitScriptGeneratorView.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IFunction.h"
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
  m_model->subscribePresenter(this);
  m_view->subscribePresenter(this);
  setWorkspaces(workspaceNames, startX, endX);
}

FitScriptGeneratorPresenter::~FitScriptGeneratorPresenter() {}

void FitScriptGeneratorPresenter::notifyPresenter(ViewEvent const &event,
                                                  std::string const &arg1,
                                                  std::string const &arg2) {
  if (arg1.empty())
    UNUSED_ARG(arg1);
  if (arg2.empty())
    UNUSED_ARG(arg2);

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
    handleFunctionRemoved(arg1);
    return;
  case ViewEvent::FunctionAdded:
    handleFunctionAdded(arg1);
    return;
  case ViewEvent::FunctionReplaced:
    handleFunctionReplaced(arg1);
    return;
  case ViewEvent::ParameterChanged:
    handleParameterChanged(arg1);
    return;
  case ViewEvent::AttributeChanged:
    handleAttributeChanged(arg1);
    return;
  case ViewEvent::ParameterTieChanged:
    handleParameterTieChanged(arg1, arg2);
    return;
  }

  throw std::runtime_error("Failed to notify the FitScriptGeneratorPresenter.");
}

void FitScriptGeneratorPresenter::notifyPresenter(
    ViewEvent const &event, FittingMode const &fittingMode) {
  switch (event) {
  case ViewEvent::FittingModeChanged:
    handleFittingModeChanged(fittingMode);
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
  m_view->setSimultaneousMode(m_model->getFittingMode() ==
                              FittingMode::Simultaneous);

  auto const selectedRows = m_view->selectedRows();
  if (!selectedRows.empty()) {
    auto const workspaceName = m_view->workspaceName(selectedRows[0]);
    auto const workspaceIndex = m_view->workspaceIndex(selectedRows[0]);
    m_view->setFunction(m_model->getFunction(workspaceName, workspaceIndex));
  } else {
    m_view->clearFunction();
  }
}

void FitScriptGeneratorPresenter::handleFunctionRemoved(
    std::string const &function) {
  auto const rowIndices = m_view->isAddRemoveFunctionForAllChecked()
                              ? m_view->allRows()
                              : m_view->selectedRows();

  if (!rowIndices.empty()) {
    removeFunctionForDomains(function, rowIndices);
    handleSelectionChanged();
  }
}

void FitScriptGeneratorPresenter::handleFunctionAdded(
    std::string const &function) {
  auto const rowIndices = m_view->isAddRemoveFunctionForAllChecked()
                              ? m_view->allRows()
                              : m_view->selectedRows();

  if (!rowIndices.empty()) {
    addFunctionForDomains(function, rowIndices);
  } else {
    m_view->displayWarning("Data needs to be loaded before adding a function.");
    m_view->clearFunction();
  }
}

void FitScriptGeneratorPresenter::handleFunctionReplaced(
    std::string const &function) {
  auto const rowIndices = m_view->isAddRemoveFunctionForAllChecked()
                              ? m_view->allRows()
                              : m_view->selectedRows();

  if (!rowIndices.empty()) {
    setFunctionForDomains(function, rowIndices);
  } else {
    m_view->displayWarning("Data needs to be loaded before adding a function.");
    m_view->clearFunction();
  }
}

void FitScriptGeneratorPresenter::handleParameterChanged(
    std::string const &parameter) {
  auto const rowIndices = m_view->allRows();
  auto const newValue = m_view->parameterValue(parameter);

  for (auto const &rowIndex : rowIndices) {
    auto const workspaceName = m_view->workspaceName(rowIndex);
    auto const workspaceIndex = m_view->workspaceIndex(rowIndex);
    auto const equivalentParameter = m_model->getEquivalentParameterForDomain(
        workspaceName, workspaceIndex, parameter);
    m_model->updateParameterValue(workspaceName, workspaceIndex,
                                  equivalentParameter, newValue);
  }

  handleSelectionChanged();
}

void FitScriptGeneratorPresenter::handleAttributeChanged(
    std::string const &attribute) {
  auto const rowIndices = m_view->allRows();
  auto const newValue = m_view->attributeValue(attribute);

  for (auto const &rowIndex : rowIndices) {
    auto const workspaceName = m_view->workspaceName(rowIndex);
    auto const workspaceIndex = m_view->workspaceIndex(rowIndex);
    m_model->updateAttributeValue(workspaceName, workspaceIndex, attribute,
                                  newValue);
  }
}

void FitScriptGeneratorPresenter::handleParameterTieChanged(
    std::string const &parameter, std::string const &tie) {
  auto const rowIndices = m_view->allRows();

  for (auto const &rowIndex : rowIndices) {
    auto const workspaceName = m_view->workspaceName(rowIndex);
    auto const workspaceIndex = m_view->workspaceIndex(rowIndex);
    auto const equivalentParameter = m_model->getEquivalentParameterForDomain(
        workspaceName, workspaceIndex, parameter);
    auto const equivalentTie = m_model->getEquivalentParameterTieForDomain(
        workspaceName, workspaceIndex, parameter, tie);
    m_model->updateParameterTie(workspaceName, workspaceIndex,
                                equivalentParameter, equivalentTie);
  }

  setGlobalTies(m_model->getGlobalTies());
  handleSelectionChanged();
}

void FitScriptGeneratorPresenter::handleFittingModeChanged(
    FittingMode const &fittingMode) {
  m_model->setFittingMode(fittingMode);
  handleSelectionChanged();
}

void FitScriptGeneratorPresenter::setGlobalTies(
    std::vector<GlobalTie> const &globalTies) {
  m_view->setGlobalTies(globalTies);
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
  if (!m_model->updateStartX(workspaceName, workspaceIndex, startX)) {
    m_view->resetSelection();
    m_view->displayWarning("The StartX provided must be within the x limits of "
                           "its workspace, and less than the EndX.");
  }
}

void FitScriptGeneratorPresenter::updateEndX(std::string const &workspaceName,
                                             WorkspaceIndex workspaceIndex,
                                             double endX) {
  if (!m_model->updateEndX(workspaceName, workspaceIndex, endX)) {
    m_view->resetSelection();
    m_view->displayWarning("The EndX provided must be within the x limits of "
                           "its workspace, and greater than the StartX.");
  }
}

void FitScriptGeneratorPresenter::removeFunctionForDomains(
    std::string const &function,
    std::vector<FitDomainIndex> const &domainIndices) {
  for (auto const &domainIndex : domainIndices) {
    auto const workspaceName = m_view->workspaceName(domainIndex);
    auto const workspaceIndex = m_view->workspaceIndex(domainIndex);
    m_model->removeFunction(workspaceName, workspaceIndex, function);
  }
}

void FitScriptGeneratorPresenter::addFunctionForDomains(
    std::string const &function,
    std::vector<FitDomainIndex> const &domainIndices) {
  for (auto const &domainIndex : domainIndices) {
    auto const workspaceName = m_view->workspaceName(domainIndex);
    auto const workspaceIndex = m_view->workspaceIndex(domainIndex);
    m_model->addFunction(workspaceName, workspaceIndex, function);
  }
}

void FitScriptGeneratorPresenter::setFunctionForDomains(
    std::string const &function,
    std::vector<FitDomainIndex> const &domainIndices) {
  for (auto const &domainIndex : domainIndices) {
    auto const workspaceName = m_view->workspaceName(domainIndex);
    auto const workspaceIndex = m_view->workspaceIndex(domainIndex);
    m_model->setFunction(workspaceName, workspaceIndex, function);
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
