#include "EnggDiffMultiRunFittingViewQtWidget.h"
#include "EnggDiffMultiRunFittingWidgetModel.h"
#include "EnggDiffMultiRunFittingWidgetPresenter.h"

#include "MantidKernel/make_unique.h"

namespace MantidQt {
namespace CustomInterfaces {

EnggDiffMultiRunFittingViewQtWidget::EnggDiffMultiRunFittingViewQtWidget(
    boost::shared_ptr<IEnggDiffractionUserMsg> userMessageProvider)
    : m_userMessageProvider(userMessageProvider) {
  m_ui.setupUi(this);

  auto model =
      Mantid::Kernel::make_unique<EnggDiffMultiRunFittingWidgetModel>();
  m_presenter =
      Mantid::Kernel::make_unique<EnggDiffMultiRunFittingWidgetPresenter>(
          std::move(model), this);
  m_presenter->notify(IEnggDiffMultiRunFittingWidgetPresenter::Start);
}

void EnggDiffMultiRunFittingViewQtWidget::addFittedPeaks(
    const int runNumber, const size_t bank,
    const Mantid::API::MatrixWorkspace_sptr ws) {
  m_fittedPeaksRunNumberToAdd = runNumber;
  m_fittedPeaksBankIDToAdd = bank;
  m_fittedPeaksWorkspaceToAdd = ws;
  m_presenter->notify(IEnggDiffMultiRunFittingWidgetPresenter::AddFittedPeaks);
}

void EnggDiffMultiRunFittingViewQtWidget::addFocusedRun(
    const int runNumber, const size_t bank,
    const Mantid::API::MatrixWorkspace_sptr ws) {
  m_focusedRunBankIDToAdd = bank;
  m_focusedRunNumberToAdd = runNumber;
  m_focusedWorkspaceToAdd = ws;
  m_presenter->notify(IEnggDiffMultiRunFittingWidgetPresenter::AddFocusedRun);
}

boost::optional<Mantid::API::MatrixWorkspace_sptr>
EnggDiffMultiRunFittingViewQtWidget::getFittedPeaks(const int runNumber,
                                                    const size_t bank) const {
  m_fittedPeaksBankIDToReturn = bank;
  m_fittedPeaksRunNumberToReturn = runNumber;
  m_presenter->notify(IEnggDiffMultiRunFittingWidgetPresenter::GetFittedPeaks);
  return m_fittedPeaksWorkspaceToReturn;
}

Mantid::API::MatrixWorkspace_sptr
EnggDiffMultiRunFittingViewQtWidget::getFittedPeaksWorkspaceToAdd() const {
  return m_fittedPeaksWorkspaceToAdd;
}

size_t EnggDiffMultiRunFittingViewQtWidget::getFittedPeaksBankIDToAdd() const {
  return m_fittedPeaksBankIDToAdd;
}

int EnggDiffMultiRunFittingViewQtWidget::getFittedPeaksRunNumberToAdd() const {
  return m_fittedPeaksRunNumberToAdd;
}

size_t
EnggDiffMultiRunFittingViewQtWidget::getFittedPeaksBankIDToReturn() const {
  return m_fittedPeaksBankIDToReturn;
}

int EnggDiffMultiRunFittingViewQtWidget::getFittedPeaksRunNumberToReturn()
    const {
  return m_fittedPeaksRunNumberToReturn;
}

boost::optional<Mantid::API::MatrixWorkspace_sptr>
EnggDiffMultiRunFittingViewQtWidget::getFocusedRun(const int runNumber,
                                                   const size_t bank) const {
  m_focusedRunNumberToReturn = runNumber;
  m_focusedRunBankIDToReturn = bank;
  m_presenter->notify(IEnggDiffMultiRunFittingWidgetPresenter::GetFocusedRun);
  return m_focusedWorkspaceToReturn;
}

Mantid::API::MatrixWorkspace_sptr
EnggDiffMultiRunFittingViewQtWidget::getFocusedWorkspaceToAdd() const {
  return m_focusedWorkspaceToAdd;
}

size_t EnggDiffMultiRunFittingViewQtWidget::getFocusedRunBankIDToAdd() const {
  return m_focusedRunBankIDToAdd;
}

size_t
EnggDiffMultiRunFittingViewQtWidget::getFocusedRunBankIDToReturn() const {
  return m_focusedRunBankIDToReturn;
}

int EnggDiffMultiRunFittingViewQtWidget::getFocusedRunNumberToAdd() const {
  return m_focusedRunNumberToAdd;
}

int EnggDiffMultiRunFittingViewQtWidget::getFocusedRunNumberToReturn() const {
  return m_focusedRunNumberToReturn;
}

void EnggDiffMultiRunFittingViewQtWidget::setFittedPeaksWorkspaceToReturn(
    const Mantid::API::MatrixWorkspace_sptr ws) {
  m_fittedPeaksWorkspaceToReturn = ws;
}

void EnggDiffMultiRunFittingViewQtWidget::setFocusedRunWorkspaceToReturn(
    const Mantid::API::MatrixWorkspace_sptr ws) {
  m_focusedWorkspaceToReturn = ws;
}

void EnggDiffMultiRunFittingViewQtWidget::userError(
    const std::string &errorTitle, const std::string &errorDescription) {
  m_userMessageProvider->userError(errorTitle, errorDescription);
}

} // CustomInterfaces
} // MantidQt
