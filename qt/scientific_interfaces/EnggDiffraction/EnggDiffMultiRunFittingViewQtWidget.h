#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETVIEW_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETVIEW_H_

#include "DllConfig.h"
#include "IEnggDiffMultiRunFittingWidgetPresenter.h"
#include "IEnggDiffMultiRunFittingWidgetView.h"
#include "ui_EnggDiffMultiRunFittingQtWidget.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ENGGDIFFRACTION_DLL EnggDiffMultiRunFittingViewQtWidget
    : public QWidget,
      public IEnggDiffMultiRunFittingWidgetView {
  Q_OBJECT

public:
  EnggDiffMultiRunFittingViewQtWidget();

  void addFittedPeaks(const int runNumber, const size_t bank,
                      const Mantid::API::MatrixWorkspace_sptr ws) override;

  void addFocusedRun(const int runNumber, const size_t bank,
                     const Mantid::API::MatrixWorkspace_sptr ws) override;

  boost::optional<Mantid::API::MatrixWorkspace_sptr>
  getFittedPeaks(const int runNumber, const size_t bank) const override;

  Mantid::API::MatrixWorkspace_sptr
  getFittedPeaksWorkspaceToAdd() const override;

  size_t getFittedPeaksBankIDToAdd() const override;

  size_t getFittedPeaksBankIDToReturn() const override;

  int getFittedPeaksRunNumberToReturn() const override;

  int getFittedPeaksRunNumberToAdd() const override;

  boost::optional<Mantid::API::MatrixWorkspace_sptr>
  getFocusedRun(const int runNumber, const size_t bank) const override;

  Mantid::API::MatrixWorkspace_sptr getFocusedWorkspaceToAdd() const override;

  size_t getFocusedRunBankIDToAdd() const override;

  size_t getFocusedRunBankIDToReturn() const override;

  int getFocusedRunNumberToAdd() const override;

  int getFocusedRunNumberToReturn() const override;

  void setFittedPeaksWorkspaceToReturn(
      const Mantid::API::MatrixWorkspace_sptr ws) override;

  void setFocusedRunWorkspaceToReturn(
      const Mantid::API::MatrixWorkspace_sptr ws) override;

  void userError(const std::string &errorTitle,
                 const std::string &errorDescription) override;

private:
  size_t m_fittedPeaksBankIDToAdd;

  mutable size_t m_fittedPeaksBankIDToReturn;

  int m_fittedPeaksRunNumberToAdd;

  mutable int m_fittedPeaksRunNumberToReturn;

  Mantid::API::MatrixWorkspace_sptr m_fittedPeaksWorkspaceToAdd;

  boost::optional<Mantid::API::MatrixWorkspace_sptr>
      m_fittedPeaksWorkspaceToReturn;

  size_t m_focusedRunBankIDToAdd;

  mutable size_t m_focusedRunBankIDToReturn;

  int m_focusedRunNumberToAdd;

  mutable int m_focusedRunNumberToReturn;
  
  Mantid::API::MatrixWorkspace_sptr m_focusedWorkspaceToAdd;

  boost::optional<Mantid::API::MatrixWorkspace_sptr>
      m_focusedWorkspaceToReturn;

  std::unique_ptr<IEnggDiffMultiRunFittingWidgetPresenter> m_presenter;

  Ui::EnggDiffMultiRunFittingWidget m_ui;
};

} // CustomInterfaces
} // MantidQt

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETVIEW_H_
