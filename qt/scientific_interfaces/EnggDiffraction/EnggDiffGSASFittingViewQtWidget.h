#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFGSASFITTINGVIEWQTWIDGET_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFGSASFITTINGVIEWQTWIDGET_H_

#include "DllConfig.h"
#include "IEnggDiffGSASFittingPresenter.h"
#include "IEnggDiffGSASFittingView.h"
#include "IEnggDiffractionUserMsg.h"

#include <qwt_plot_curve.h>

#include "ui_EnggDiffractionQtTabGSAS.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ENGGDIFFRACTION_DLL EnggDiffGSASFittingViewQtWidget
    : public QWidget,
      public IEnggDiffGSASFittingView {
  Q_OBJECT

public:
  EnggDiffGSASFittingViewQtWidget(
      boost::shared_ptr<IEnggDiffractionUserMsg> userMessageProvider);

  ~EnggDiffGSASFittingViewQtWidget() override;

  void displayLatticeParams(
      const Mantid::API::ITableWorkspace_sptr latticeParams) const override;

  void displayRwp(const double rwp) const override;

  std::vector<std::string> getFocusedFileNames() const override;

  std::string getGSASIIProjectPath() const override;

  std::string getInstrumentFileName() const override;

  std::string getPathToGSASII() const override;

  double getPawleyDMin() const override;

  double getPawleyNegativeWeight() const override;

  std::vector<std::string> getPhaseFileNames() const override;

  GSASRefinementMethod getRefinementMethod() const override;

  std::pair<int, size_t> getSelectedRunLabel() const override;

  void plotCurve(const std::vector<boost::shared_ptr<QwtData>> &curve) override;

  void resetCanvas() override;

  void setEnabled(const bool enabled);

  bool showRefinementResultsSelected() const override;

  void
  updateRunList(const std::vector<std::pair<int, size_t>> &runLabels) override;

  void userError(const std::string &errorTitle,
                 const std::string &errorDescription) const override;

  void userWarning(const std::string &warningTitle,
                   const std::string &warningDescription) const override;

private slots:
  void browseFocusedRun();
  void loadFocusedRun();
  void selectRun();

private:
  std::vector<QwtPlotCurve *> m_focusedRunCurves;

  boost::shared_ptr<IEnggDiffractionUserMsg> m_userMessageProvider;

  std::unique_ptr<IEnggDiffGSASFittingPresenter> m_presenter;

  Ui::EnggDiffractionQtTabGSAS m_ui;

  void setFocusedRunFileNames(const QStringList &filenames);

  void setupUI();
};

} // MantidQt
} // CustomInterfaces

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFGSASFITTINGVIEWQTWIDGET_H_
