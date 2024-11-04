// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Spectroscopy/RunWidget/RunPresenter.h"

#include "MantidQtWidgets/Spectroscopy/RunWidget/IRunSubscriber.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/RunView.h"

#include "MantidKernel/UsageService.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

namespace MantidQt {
namespace CustomInterfaces {

RunPresenter::RunPresenter(IRunSubscriber *subscriber, IRunView *view) : m_subscriber(subscriber), m_view(view) {
  m_view->subscribePresenter(this);
}

void RunPresenter::handleRunClicked() {
  if (validate()) {
    setRunEnabled(false);
    try {
      m_subscriber->handleRun();
    } catch (std::exception const &ex) {
      m_view->displayWarning(ex.what());
      setRunEnabled(true);
    }
    Mantid::Kernel::UsageService::Instance().registerFeatureUsage(Mantid::Kernel::FeatureType::Interface,
                                                                  m_subscriber->getSubscriberName(), false);
  }
}

void RunPresenter::setRunEnabled(bool const enable) { m_view->setRunText(enable ? "Run" : "Running..."); }

void RunPresenter::setRunText(std::string const &text) { m_view->setRunText(text); }

bool RunPresenter::validate(std::unique_ptr<IUserInputValidator> validator) const {
  m_subscriber->handleValidation(validator.get());

  const auto error = validator->generateErrorMessage();
  if (!error.empty()) {
    m_view->displayWarning(error);
  }
  return error.empty();
}

} // namespace CustomInterfaces
} // namespace MantidQt
