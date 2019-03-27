// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/WorkspacePresenter/ADSAdapter.h"
#include "MantidQtWidgets/Common/WorkspacePresenter/WorkspaceProviderNotifiable.h"
#include <MantidAPI/AnalysisDataService.h>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

namespace MantidQt {
namespace MantidWidgets {

ADSAdapter::ADSAdapter()
    : m_addObserver(*this, &ADSAdapter::handleAddWorkspace),
      m_replaceObserver(*this, &ADSAdapter::handleReplaceWorkspace),
      m_deleteObserver(*this, &ADSAdapter::handleDeleteWorkspace),
      m_clearADSObserver(*this, &ADSAdapter::handleClearADS),
      m_renameObserver(*this, &ADSAdapter::handleRenameWorkspace),
      m_groupworkspacesObserver(*this, &ADSAdapter::handleGroupWorkspaces),
      m_ungroupworkspaceObserver(*this, &ADSAdapter::handleUnGroupWorkspace),
      m_workspaceGroupUpdateObserver(*this,
                                     &ADSAdapter::handleWorkspaceGroupUpdate) {
  // Register all observers.
  auto &nc = AnalysisDataService::Instance().notificationCenter;
  nc.addObserver(m_addObserver);
  nc.addObserver(m_replaceObserver);
  nc.addObserver(m_deleteObserver);
  nc.addObserver(m_clearADSObserver);
  nc.addObserver(m_renameObserver);
  nc.addObserver(m_groupworkspacesObserver);
  nc.addObserver(m_ungroupworkspaceObserver);
  nc.addObserver(m_workspaceGroupUpdateObserver);
}

ADSAdapter::~ADSAdapter() {
  // remove all observers
  auto &nc = AnalysisDataService::Instance().notificationCenter;
  nc.removeObserver(m_addObserver);
  nc.removeObserver(m_replaceObserver);
  nc.removeObserver(m_deleteObserver);
  nc.removeObserver(m_clearADSObserver);
  nc.removeObserver(m_renameObserver);
  nc.removeObserver(m_groupworkspacesObserver);
  nc.removeObserver(m_ungroupworkspaceObserver);
  nc.removeObserver(m_workspaceGroupUpdateObserver);
}

void ADSAdapter::registerPresenter(Presenter_wptr presenter) {
  m_presenter = std::move(presenter);
}

bool ADSAdapter::doesWorkspaceExist(const std::string &wsname) const {
  return AnalysisDataService::Instance().doesExist(wsname);
}

std::map<std::string, Mantid::API::Workspace_sptr>
ADSAdapter::topLevelItems() const {
  return AnalysisDataService::Instance().topLevelItems();
}

/// Locks the presenter as shared_ptr for use internally.
Presenter_sptr ADSAdapter::lockPresenter() {
  auto psptr = m_presenter.lock();

  if (psptr == nullptr)
    throw std::runtime_error("Unable to obtain reference to presenter");

  return psptr;
}

std::string ADSAdapter::getOldName() const { return m_oldName; }
std::string ADSAdapter::getNewName() const { return m_newName; }

// ADS Observation methods
void ADSAdapter::handleAddWorkspace(Mantid::API::WorkspaceAddNotification_ptr /*unused*/) {
  auto presenter = lockPresenter();
  presenter->notifyFromWorkspaceProvider(
      WorkspaceProviderNotifiable::Flag::WorkspaceLoaded);
}

void ADSAdapter::handleReplaceWorkspace(
    Mantid::API::WorkspaceAfterReplaceNotification_ptr /*unused*/) {
  auto presenter = lockPresenter();
  presenter->notifyFromWorkspaceProvider(
      WorkspaceProviderNotifiable::Flag::GenericUpdateNotification);
}

void ADSAdapter::handleDeleteWorkspace(
    Mantid::API::WorkspacePostDeleteNotification_ptr /*unused*/) {
  auto presenter = lockPresenter();
  presenter->notifyFromWorkspaceProvider(
      WorkspaceProviderNotifiable::Flag::WorkspaceDeleted);
}

void ADSAdapter::handleClearADS(Mantid::API::ClearADSNotification_ptr /*unused*/) {
  auto presenter = lockPresenter();
  presenter->notifyFromWorkspaceProvider(
      WorkspaceProviderNotifiable::Flag::WorkspacesCleared);
}

void ADSAdapter::handleRenameWorkspace(
    Mantid::API::WorkspaceRenameNotification_ptr pNf) {
  // store old and new names when workspace rename occurs.
  m_oldName = pNf->objectName();
  m_newName = pNf->newObjectName();
  auto presenter = lockPresenter();
  presenter->notifyFromWorkspaceProvider(
      WorkspaceProviderNotifiable::Flag::WorkspaceRenamed);
}

void ADSAdapter::handleGroupWorkspaces(
    Mantid::API::WorkspacesGroupedNotification_ptr /*unused*/) {
  auto presenter = lockPresenter();
  presenter->notifyFromWorkspaceProvider(
      WorkspaceProviderNotifiable::Flag::WorkspacesGrouped);
}

void ADSAdapter::handleUnGroupWorkspace(
    Mantid::API::WorkspaceUnGroupingNotification_ptr /*unused*/) {
  auto presenter = lockPresenter();
  presenter->notifyFromWorkspaceProvider(
      WorkspaceProviderNotifiable::Flag::WorkspacesUngrouped);
}

void ADSAdapter::handleWorkspaceGroupUpdate(
    Mantid::API::GroupUpdatedNotification_ptr /*unused*/) {
  auto presenter = lockPresenter();
  presenter->notifyFromWorkspaceProvider(
      WorkspaceProviderNotifiable::Flag::WorkspaceGroupUpdated);
}

} // namespace MantidWidgets
} // namespace MantidQt
