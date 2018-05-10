#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGQTWIDGET_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGQTWIDGET_H_

#include "DllConfig.h"
#include "IEnggDiffMultiRunFittingWidgetPresenter.h"
#include "IEnggDiffMultiRunFittingWidgetView.h"
#include "IEnggDiffractionPythonRunner.h"
#include "IEnggDiffractionUserMsg.h"
#include "ui_EnggDiffMultiRunFittingWidget.h"

#include <boost/shared_ptr.hpp>
#include <qwt_plot_curve.h>
#include <qwt_plot_zoomer.h>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ENGGDIFFRACTION_DLL EnggDiffMultiRunFittingQtWidget
    : public QWidget,
      public IEnggDiffMultiRunFittingWidgetView {
  Q_OBJECT

public:
  EnggDiffMultiRunFittingQtWidget(
      boost::shared_ptr<IEnggDiffractionPythonRunner> pythonRunner);

  ~EnggDiffMultiRunFittingQtWidget() override;

  std::vector<RunLabel> getAllRunLabels() const override;

  boost::optional<RunLabel> getSelectedRunLabel() const override;

  void plotFittedPeaks(
      const std::vector<boost::shared_ptr<QwtData>> &curve) override;

  void
  plotFocusedRun(const std::vector<boost::shared_ptr<QwtData>> &curve) override;

  void plotToSeparateWindow(
      const std::string &focusedRunName,
      const boost::optional<std::string> fittedPeaksName) override;

  void reportNoRunSelectedForPlot() override;

  void reportPlotInvalidFittedPeaks(const RunLabel &runLabel) override;

  void reportPlotInvalidFocusedRun(const RunLabel &runLabel) override;

  void resetCanvas() override;

  void setEnabled(const bool enabled) override;

  void setMessageProvider(
      boost::shared_ptr<IEnggDiffractionUserMsg> messageProvider) override;

  void setPresenter(boost::shared_ptr<IEnggDiffMultiRunFittingWidgetPresenter>
                        presenter) override;

  bool showFitResultsSelected() const override;

  void updateRunList(const std::vector<RunLabel> &runLabels) override;

signals:
  void removeRunClicked();
  void runSelected();

private slots:
  void processSelectRun();
  void plotFittedPeaksStateChanged();
  void processPlotToSeparateWindow();
  void processRemoveRun();

private:
  void cleanUpPlot();

  bool hasSelectedRunLabel() const;

  void resetPlotZoomLevel();

  void setupUI();

  void userError(const std::string &errorTitle,
                 const std::string &errorDescription);

  std::vector<std::unique_ptr<QwtPlotCurve>> m_fittedPeaksCurves;

  std::vector<std::unique_ptr<QwtPlotCurve>> m_focusedRunCurves;

  std::unique_ptr<QwtPlotZoomer> m_zoomTool;

  boost::shared_ptr<IEnggDiffMultiRunFittingWidgetPresenter> m_presenter;

  boost::shared_ptr<IEnggDiffractionPythonRunner> m_pythonRunner;

  Ui::EnggDiffMultiRunFittingWidget m_ui;

  boost::shared_ptr<IEnggDiffractionUserMsg> m_userMessageProvider;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGQTWIDGET_H_
