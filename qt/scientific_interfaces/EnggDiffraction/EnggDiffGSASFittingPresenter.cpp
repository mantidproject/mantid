#include "EnggDiffGSASFittingPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

EnggDiffGSASFittingPresenter::EnggDiffGSASFittingPresenter(
    std::unique_ptr<IEnggDiffGSASFittingModel> model,
    std::unique_ptr<IEnggDiffGSASFittingView> view)
    : m_model(std::move(model)), m_view(std::move(view)),
      m_viewHasClosed(false) {}

EnggDiffGSASFittingPresenter::~EnggDiffGSASFittingPresenter() {
  throw new std::runtime_error("Virtual destructor not yet implemented");
}

void EnggDiffGSASFittingPresenter::notify(
    IEnggDiffGSASFittingPresenter::Notification notif) {

  if (m_viewHasClosed) {
    return;
  }

  switch (notif) {

  case IEnggDiffGSASFittingPresenter::Start:
    processStart();
    break;

  case IEnggDiffGSASFittingPresenter::ShutDown:
    processShutDown();
    break;
  }
}

void EnggDiffGSASFittingPresenter::processStart() {}

void EnggDiffGSASFittingPresenter::processShutDown() { m_viewHasClosed = true; }

} // MantidQt
} // CustomInterfaces
