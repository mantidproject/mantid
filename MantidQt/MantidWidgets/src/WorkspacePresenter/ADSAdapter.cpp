#include "MantidQtMantidWidgets/WorkspacePresenter/ADSAdapter.h"
#include "MantidQtMantidWidgets/WorkspacePresenter/WorkspaceProviderNotifiable.h"
#include <MantidAPI/AnalysisDataService.h>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

namespace MantidQt {
namespace MantidWidgets {

ADSAdapter::ADSAdapter()
    : m_addObserver(*this, &ADSAdapter::handleAddWorkspace) {

  AnalysisDataServiceImpl &dataStore = AnalysisDataService::Instance();
  dataStore.notificationCenter.addObserver(m_addObserver);
}

ADSAdapter::~ADSAdapter() {
  Mantid::API::AnalysisDataService::Instance()
      .notificationCenter.removeObserver(m_addObserver);
}

void ADSAdapter::registerPresenter(Presenter_wptr presenter) {
  m_presenter = std::move(presenter);
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

} // namespace MantidQt
} // namespace MantidWidgets