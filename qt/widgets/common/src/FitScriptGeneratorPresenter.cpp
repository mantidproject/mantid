// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FitScriptGeneratorPresenter.h"
#include "MantidQtWidgets/Common/FittingGlobals.h"
#include "MantidQtWidgets/Common/IFitScriptGeneratorModel.h"
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

FitScriptGeneratorPresenter::FitScriptGeneratorPresenter(IFitScriptGeneratorView *view, IFitScriptGeneratorModel *model,
                                                         QStringList const &workspaceNames, double startX, double endX)
    : m_warnings(), m_view(view), m_model(model) {
  m_model->subscribePresenter(this);
  m_view->subscribePresenter(this);
  setWorkspaces(workspaceNames, startX, endX);
}

FitScriptGeneratorPresenter::~FitScriptGeneratorPresenter() {}

void FitScriptGeneratorPresenter::notifyPresenter(ViewEvent const &event, std::string const &arg1,
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
  case ViewEvent::ParameterConstraintRemoved:
    handleParameterConstraintRemoved(arg1);
    return;
  case ViewEvent::ParameterConstraintChanged:
    handleParameterConstraintChanged(arg1, arg2);
    return;
  default:
    throw std::runtime_error("Failed to notify the FitScriptGeneratorPresenter.");
  }
}

void FitScriptGeneratorPresenter::notifyPresenter(ViewEvent const &event, std::vector<std::string> const &vec) {
  switch (event) {
  case ViewEvent::GlobalParametersChanged:
    handleGlobalParametersChanged(vec);
    return;
  default:
    throw std::runtime_error("Failed to notify the FitScriptGeneratorPresenter.");
  }
}

void FitScriptGeneratorPresenter::notifyPresenter(ViewEvent const &event, FittingMode fittingMode) {
  switch (event) {
  case ViewEvent::FittingModeChanged:
    handleFittingModeChanged(fittingMode);
    return;
  default:
    throw std::runtime_error("Failed to notify the FitScriptGeneratorPresenter.");
  }
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

void FitScriptGeneratorPresenter::handleSelectionChanged() {
  m_view->setSimultaneousMode(m_model->isSimultaneousMode());

  if (m_view->hasLoadedData()) {
    updateFunctionInViewFromModel(m_view->currentRow());
  } else {
    m_view->clearFunction();
  }
}

void FitScriptGeneratorPresenter::handleStartXChanged() {
  updateXLimitForDomain(&IFitScriptGeneratorView::startX, &FitScriptGeneratorPresenter::updateStartX);
}

void FitScriptGeneratorPresenter::handleEndXChanged() {
  updateXLimitForDomain(&IFitScriptGeneratorView::endX, &FitScriptGeneratorPresenter::updateEndX);
}

void FitScriptGeneratorPresenter::handleFunctionRemoved(std::string const &function) {
  updateFunctionStructure(&FitScriptGeneratorPresenter::removeFunction, function);
}

void FitScriptGeneratorPresenter::handleFunctionAdded(std::string const &function) {
  updateFunctionStructure(&FitScriptGeneratorPresenter::addFunction, function);
}

void FitScriptGeneratorPresenter::handleFunctionReplaced(std::string const &function) {
  updateFunctionStructure(&FitScriptGeneratorPresenter::setFunction, function);
}

void FitScriptGeneratorPresenter::handleParameterChanged(std::string const &parameter) {
  updateFunctionsInModel(&FitScriptGeneratorPresenter::updateParameterValue, parameter,
                         m_view->parameterValue(parameter));
  handleSelectionChanged();
}

void FitScriptGeneratorPresenter::handleAttributeChanged(std::string const &attribute) {
  updateFunctionsInModel(&FitScriptGeneratorPresenter::updateAttributeValue, attribute,
                         m_view->attributeValue(attribute));
}

void FitScriptGeneratorPresenter::handleParameterTieChanged(std::string const &parameter, std::string const &tie) {
  updateFunctionsInModel(&FitScriptGeneratorPresenter::updateParameterTie, parameter, tie);

  checkForWarningMessages();
  setGlobalTies(m_model->getGlobalTies());
  handleSelectionChanged();
}

void FitScriptGeneratorPresenter::handleParameterConstraintRemoved(std::string const &parameter) {
  updateFunctionsInModel(&FitScriptGeneratorPresenter::removeParameterConstraint, parameter);
  handleSelectionChanged();
}

void FitScriptGeneratorPresenter::handleParameterConstraintChanged(std::string const &functionIndex,
                                                                   std::string const &constraint) {
  updateFunctionsInModel(&FitScriptGeneratorPresenter::updateParameterConstraint, functionIndex, constraint);
  handleSelectionChanged();
}

void FitScriptGeneratorPresenter::handleGlobalParametersChanged(std::vector<std::string> const &globalParameters) {
  try {
    m_model->setGlobalParameters(globalParameters);
  } catch (std::invalid_argument const &ex) {
    m_view->displayWarning(ex.what());
  }
  handleSelectionChanged();
}

void FitScriptGeneratorPresenter::handleFittingModeChanged(FittingMode fittingMode) {
  m_model->setFittingMode(fittingMode);
  handleSelectionChanged();
}

void FitScriptGeneratorPresenter::setGlobalTies(std::vector<GlobalTie> const &globalTies) {
  m_view->setGlobalTies(globalTies);
}

void FitScriptGeneratorPresenter::setGlobalParameters(std::vector<GlobalParameter> const &globalParameters) {
  m_view->setGlobalParameters(globalParameters);
}

void FitScriptGeneratorPresenter::setWorkspaces(QStringList const &workspaceNames, double startX, double endX) {
  for (auto const &workspaceName : workspaceNames)
    addWorkspace(workspaceName.toStdString(), startX, endX);
  checkForWarningMessages();
}

void FitScriptGeneratorPresenter::addWorkspaces(std::vector<MatrixWorkspace_const_sptr> const &workspaces,
                                                std::vector<WorkspaceIndex> const &workspaceIndices) {
  for (auto const &workspace : workspaces) {
    for (auto const &workspaceIndex : workspaceIndices) {
      auto const xData = workspace->x(workspaceIndex.value);
      addWorkspace(workspace, workspaceIndex, xData.front(), xData.back());
    }
  }
  checkForWarningMessages();
}

void FitScriptGeneratorPresenter::addWorkspace(std::string const &workspaceName, double startX, double endX) {
  auto &ads = AnalysisDataService::Instance();
  if (ads.doesExist(workspaceName))
    addWorkspace(ads.retrieveWS<MatrixWorkspace>(workspaceName), startX, endX);
}

void FitScriptGeneratorPresenter::addWorkspace(MatrixWorkspace_const_sptr const &workspace, double startX,
                                               double endX) {
  for (auto index = 0u; index < workspace->getNumberHistograms(); ++index)
    addWorkspace(workspace, WorkspaceIndex{index}, startX, endX);
}

void FitScriptGeneratorPresenter::addWorkspace(MatrixWorkspace_const_sptr const &workspace,
                                               WorkspaceIndex workspaceIndex, double startX, double endX) {
  addWorkspace(workspace->getName(), workspaceIndex, startX, endX);
}

void FitScriptGeneratorPresenter::addWorkspace(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                               double startX, double endX) {
  try {
    m_model->addWorkspaceDomain(workspaceName, workspaceIndex, startX, endX);
    m_view->addWorkspaceDomain(workspaceName, workspaceIndex, startX, endX);
  } catch (std::invalid_argument const &ex) {
    m_warnings.emplace_back(ex.what());
  }
}

void FitScriptGeneratorPresenter::updateStartX(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                               double startX) {
  if (!m_model->updateStartX(workspaceName, workspaceIndex, startX)) {
    m_view->resetSelection();
    m_view->displayWarning("The StartX provided must be within the x limits of "
                           "its workspace, and less than the EndX.");
  }
}

void FitScriptGeneratorPresenter::updateEndX(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                             double endX) {
  if (!m_model->updateEndX(workspaceName, workspaceIndex, endX)) {
    m_view->resetSelection();
    m_view->displayWarning("The EndX provided must be within the x limits of "
                           "its workspace, and greater than the StartX.");
  }
}

void FitScriptGeneratorPresenter::updateParameterValue(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                                       std::string const &parameter, double newValue) {
  auto const equivalentParameter =
      m_model->getEquivalentFunctionIndexForDomain(workspaceName, workspaceIndex, parameter);
  m_model->updateParameterValue(workspaceName, workspaceIndex, equivalentParameter, newValue);
}

void FitScriptGeneratorPresenter::updateAttributeValue(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                                       std::string const &fullAttribute,
                                                       IFunction::Attribute const &newValue) {
  m_model->updateAttributeValue(workspaceName, workspaceIndex, fullAttribute, newValue);
}

void FitScriptGeneratorPresenter::updateParameterTie(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                                     std::string const &parameter, std::string const &tie) {
  try {
    auto const [equivalentParameter, equivalentTie] =
        convertFunctionIndexOfParameterTie(workspaceName, workspaceIndex, parameter, tie);
    m_model->updateParameterTie(workspaceName, workspaceIndex, equivalentParameter, equivalentTie);
  } catch (std::invalid_argument const &ex) {
    m_warnings.emplace_back(ex.what());
  }
}

void FitScriptGeneratorPresenter::removeParameterConstraint(std::string const &workspaceName,
                                                            WorkspaceIndex workspaceIndex,
                                                            std::string const &parameter) {
  m_model->removeParameterConstraint(workspaceName, workspaceIndex, parameter);
}

void FitScriptGeneratorPresenter::updateParameterConstraint(std::string const &workspaceName,
                                                            WorkspaceIndex workspaceIndex,
                                                            std::string const &functionIndex,
                                                            std::string const &constraint) {
  auto const equivalentFunctionIndex =
      m_model->getEquivalentFunctionIndexForDomain(workspaceName, workspaceIndex, functionIndex);
  m_model->updateParameterConstraint(workspaceName, workspaceIndex, equivalentFunctionIndex, constraint);
}

void FitScriptGeneratorPresenter::removeFunction(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                                 std::string const &function) {
  m_model->removeFunction(workspaceName, workspaceIndex, function);
}

void FitScriptGeneratorPresenter::addFunction(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                              std::string const &function) {
  m_model->addFunction(workspaceName, workspaceIndex, function);
}

void FitScriptGeneratorPresenter::setFunction(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                              std::string const &function) {
  m_model->setFunction(workspaceName, workspaceIndex, function);
}

void FitScriptGeneratorPresenter::updateFunctionInViewFromModel(FitDomainIndex domainIndex) {
  auto const workspaceName = m_view->workspaceName(domainIndex);
  auto const workspaceIndex = m_view->workspaceIndex(domainIndex);
  m_view->setFunction(m_model->getFunction(workspaceName, workspaceIndex));
  setGlobalParameters(m_model->getGlobalParameters());
}

template <typename GetX, typename UpdateX>
void FitScriptGeneratorPresenter::updateXLimitForDomain(GetX &&getX, UpdateX &&updateX) {
  if (m_view->hasLoadedData()) {
    auto const domainIndex = m_view->currentRow();
    auto const newValue = std::invoke(std::forward<GetX>(getX), m_view, domainIndex);
    invokeFunctionForDomain(domainIndex, updateX, newValue);
  }
}

template <typename UpdateFunction>
void FitScriptGeneratorPresenter::updateFunctionStructure(UpdateFunction &&updateFunction,
                                                          std::string const &function) {
  if (m_view->hasLoadedData()) {
    updateFunctionsInModel(updateFunction, function);
  } else {
    m_view->displayWarning("Data needs to be loaded using Add Workspace.");
    m_view->clearFunction();
  }
}

template <typename UpdateFunction, typename... Args>
void FitScriptGeneratorPresenter::updateFunctionsInModel(UpdateFunction &&updateFunction, Args... arguments) {
  for (auto const &rowIndex : getRowIndices()) {
    invokeFunctionForDomain(rowIndex, updateFunction, arguments...);
  }
}

template <typename Function, typename... Args>
void FitScriptGeneratorPresenter::invokeFunctionForDomain(FitDomainIndex domainIndex, Function &&func,
                                                          Args... arguments) {
  auto const workspaceName = m_view->workspaceName(domainIndex);
  auto const workspaceIndex = m_view->workspaceIndex(domainIndex);

  std::invoke(std::forward<Function>(func), this, workspaceName, workspaceIndex, arguments...);
}

std::vector<FitDomainIndex> FitScriptGeneratorPresenter::getRowIndices() const {
  return m_view->applyFunctionChangesToAll() ? m_view->allRows() : m_view->selectedRows();
}

std::tuple<std::string, std::string> FitScriptGeneratorPresenter::convertFunctionIndexOfParameterTie(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex, std::string const &parameter,
    std::string const &tie) const {
  auto const equivalentParameter =
      m_model->getEquivalentFunctionIndexForDomain(workspaceName, workspaceIndex, parameter);
  auto const equivalentTie = m_model->getEquivalentParameterTieForDomain(workspaceName, workspaceIndex, parameter, tie);
  return {equivalentParameter, equivalentTie};
}

void FitScriptGeneratorPresenter::checkForWarningMessages() {
  if (!m_warnings.empty()) {
    std::stringstream ss;
    std::copy(m_warnings.cbegin(), m_warnings.cend(), std::ostream_iterator<std::string>(ss, "\n"));
    m_view->displayWarning(ss.str());
    m_warnings.clear();
  }
}

} // namespace MantidWidgets
} // namespace MantidQt
