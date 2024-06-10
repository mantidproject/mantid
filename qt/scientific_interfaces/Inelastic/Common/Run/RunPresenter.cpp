// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "RunPresenter.h"

#include "IRunSubscriber.h"
#include "RunView.h"

#include "MantidQtWidgets/Common/UserInputValidator.h"

namespace MantidQt {
namespace CustomInterfaces {

RunPresenter::RunPresenter(IRunSubscriber *subscriber, IRunView *view) : m_subscriber(subscriber), m_view(view) {
  m_view->subscribePresenter(this);
}

void RunPresenter::handleRunClicked() {
  if (validate()) {
    m_view->setRunEnabled(false);
    m_subscriber->handleRun();
  }
}

void RunPresenter::setRunEnabled(bool const enable) { m_view->setRunEnabled(enable); }

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