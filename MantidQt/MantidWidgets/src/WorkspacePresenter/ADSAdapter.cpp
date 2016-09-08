#include "MantidQtMantidWidgets/WorkspacePresenter/ADSAdapter.h"
#include "MantidQtMantidWidgets/WorkspacePresenter/WorkspaceProviderNotifiable.h"
#include <MantidAPI/AnalysisDataService.h>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

namespace MantidQt {
namespace MantidWidgets {

ADSAdapter::ADSAdapter()
    : m_addObserver(*this, &ADSAdapter::handleAddWorkspace),
      m_deleteObserver(*this, &ADSAdapter::handleDeleteWorkspace),
      m_clearADSObserver(*this, &ADSAdapter::handleClearADS),
      m_renameObserver(*this, &ADSAdapter::handleRenameWorkspace) {

  AnalysisDataServiceImpl &dataStore = AnalysisDataService::Instance();
  dataStore.notificationCenter.addObserver(m_addObserver);
  dataStore.notificationCenter.addObserver(m_deleteObserver);
  dataStore.notificationCenter.addObserver(m_clearADSObserver);
  dataStore.notificationCenter.addObserver(m_renameObserver);
}

ADSAdapter::~ADSAdapter() {
  Mantid::API::AnalysisDataService::Instance()
      .notificationCenter.removeObserver(m_addObserver);
  Mantid::API::AnalysisDataService::Instance()
      .notificationCenter.removeObserver(m_deleteObserver);
  Mantid::API::AnalysisDataService::Instance()
      .notificationCenter.removeObserver(m_clearADSObserver);
  Mantid::API::AnalysisDataService::Instance()
      .notificationCenter.removeObserver(m_renameObserver);
}

void ADSAdapter::registerPresenter(Presenter_wptr presenter) {
  m_presenter = std::move(presenter);
}

void ADSAdapter::renameWorkspace(const std::string &oldName,
                                 const std::string &newName) {
  AnalysisDataService::Instance().rename(oldName, newName);
}

Mantid::API::Workspace_sptr
ADSAdapter::getWorkspace(const std::string &wsname) const {
  return AnalysisDataService::Instance().retrieve(wsname);
}

std::map<std::string, Mantid::API::Workspace_sptr>
ADSAdapter::topLevelItems() const {
  return AnalysisDataService::Instance().topLevelItems();
}

Presenter_sptr ADSAdapter::lockPresenter() {
  auto psptr = m_presenter.lock();

  if (psptr == nullptr)
    throw std::runtime_error("Unable to obtain reference to presenter");

  return std::move(psptr);
}

// ADS Observation methods
void ADSAdapter::handleAddWorkspace(Mantid::API::WorkspaceAddNotification_ptr) {
  auto presenter = lockPresenter();
  presenter->notifyFromWorkspaceProvider(
      WorkspaceProviderNotifiable::Flag::WorkspaceLoaded);
}

void ADSAdapter::handleDeleteWorkspace(
    Mantid::API::WorkspacePostDeleteNotification_ptr pNf) {
  auto presenter = lockPresenter();
  presenter->notifyFromWorkspaceProvider(
      WorkspaceProviderNotifiable::Flag::WorkspaceDeleted);
}

void ADSAdapter::handleClearADS(Mantid::API::ClearADSNotification_ptr pNf) {
  auto presenter = lockPresenter();
  presenter->notifyFromWorkspaceProvider(
      WorkspaceProviderNotifiable::Flag::WorkspaceDeleted);
}

void ADSAdapter::handleRenameWorkspace(
    Mantid::API::WorkspaceRenameNotification_ptr pNf) {
  auto presenter = lockPresenter();
  presenter->notifyFromWorkspaceProvider(
      WorkspaceProviderNotifiable::Flag::WorkspaceRenamed);
}

} // namespace MantidQt
} // namespace MantidWidgets