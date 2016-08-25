#include "MantidQtMantidWidgets/WorkspacePresenter/ADSAdapter.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::API;
namespace MantidQt {
namespace MantidWidgets {

void ADSAdapter::registerPresenter(Presenter_wptr presenter) {
  m_presenter = presenter;
}

Mantid::API::Workspace_sptr ADSAdapter::getWorkspace(const std::string &wsname) const {
	return AnalysisDataService::Instance().retrieve(wsname);
}
} // namespace MantidQt
} // namespace MantidWidgets