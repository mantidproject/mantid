#include "EnggDiffGSASFittingPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

EnggDiffGSASFittingPresenter::EnggDiffGSASFittingPresenter(
    std::unique_ptr<IEnggDiffGSASFittingModel> model,
    std::unique_ptr<IEnggDiffGSASFittingView> view)
    : m_model(std::move(model)), m_view(std::move(view)) {}

EnggDiffGSASFittingPresenter::~EnggDiffGSASFittingPresenter() {
  throw new std::runtime_error("Virtual destructor not yet implemented");
}

void EnggDiffGSASFittingPresenter::notify(
    IEnggDiffGSASFittingPresenter::Notification notif) {
  throw new std::runtime_error("Notify not yet implemented");
}

} // MantidQt
} // CustomInterfaces
