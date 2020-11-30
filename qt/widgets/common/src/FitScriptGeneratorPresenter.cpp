// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FitScriptGeneratorPresenter.h"
#include "MantidQtWidgets/Common/FitScriptGeneratorModel.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <stdexcept>

namespace MantidQt {
namespace MantidWidgets {

FitScriptGeneratorPresenter::FitScriptGeneratorPresenter(
    FitScriptGeneratorView *view, FitScriptGeneratorModel *model,
    QStringList const &workspaceNames, double startX, double endX)
    : m_view(view), m_model(model) {
  m_view->subscribePresenter(this);
  setWorkspaces(workspaceNames, startX, endX);
}

FitScriptGeneratorPresenter::~FitScriptGeneratorPresenter() {}

void FitScriptGeneratorPresenter::notifyPresenter(ViewEvent const &event) {
  switch (event) {
  case ViewEvent::RemoveClicked:
    handleRemoveClicked();
    return;
  case ViewEvent::AddClicked:
    handleAddWorkspaceClicked();
    return;
  case ViewEvent::StartXChanged:
    return;
  case ViewEvent::EndXChanged:
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
}

void FitScriptGeneratorPresenter::handleAddWorkspaceClicked() {
  m_view->openAddWorkspaceDialog();
}

void FitScriptGeneratorPresenter::setWorkspaces(
    QStringList const &workspaceNames, double startX, double endX) {
  for (auto const &workspaceName : workspaceNames)
    addWorkspace(workspaceName.toStdString(), startX, endX);
}

void FitScriptGeneratorPresenter::addWorkspace(std::string const &workspaceName,
                                               double startX, double endX) {
  auto &ads = AnalysisDataService::Instance();
  if (ads.doesExist(workspaceName))
    addWorkspace(ads.retrieveWS<MatrixWorkspace>(workspaceName), startX, endX);
}

void FitScriptGeneratorPresenter::addWorkspace(
    MatrixWorkspace_const_sptr const &workspace, double startX, double endX) {
  for (auto index = 0u; index < workspace->getNumberHistograms(); ++index) {
    m_view->addWorkspaceDomain(workspace->getName(), index, startX, endX);
    m_model->addWorkspaceDomain(workspace->getName(), index, startX, endX);
  }
}

} // namespace MantidWidgets
} // namespace MantidQt
