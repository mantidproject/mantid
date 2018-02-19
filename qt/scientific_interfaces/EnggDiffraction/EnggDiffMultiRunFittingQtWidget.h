#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGQTWIDGET_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGQTWIDGET_H_

#include "DllConfig.h"
#include "IEnggDiffMultiRunFittingWidgetPresenter.h"
#include "IEnggDiffMultiRunFittingWidgetView.h"
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
      boost::shared_ptr<IEnggDiffMultiRunFittingWidgetPresenter> presenter,
      boost::shared_ptr<IEnggDiffractionUserMsg> messageProvider);

  ~EnggDiffMultiRunFittingQtWidget() override;

  RunLabel getSelectedRunLabel() const override;

  void plotFittedPeaks(
      const std::vector<boost::shared_ptr<QwtData>> &curve) override;

  void
  plotFocusedRun(const std::vector<boost::shared_ptr<QwtData>> &curve) override;

  void reportPlotInvalidFittedPeaks(const RunLabel &runLabel) override;

  void reportPlotInvalidFocusedRun(const RunLabel &runLabel) override;

  void resetCanvas() override;

  bool showFitResultsSelected() const override;

  void updateRunList(const std::vector<RunLabel> &runLabels) override;

  void userError(const std::string &errorTitle,
                 const std::string &errorDescription) override;

private slots:
  void processSelectRun();

private:
  void cleanUpPlot();

  void resetPlotZoomLevel();

  void setupUI();

  std::vector<std::unique_ptr<QwtPlotCurve>> m_focusedRunCurves;

  std::unique_ptr<QwtPlotZoomer> m_zoomTool;

  boost::shared_ptr<IEnggDiffMultiRunFittingWidgetPresenter> m_presenter;

  Ui::EnggDiffMultiRunFittingWidget m_ui;

  boost::shared_ptr<IEnggDiffractionUserMsg> m_userMessageProvider;
};

} // CustomInterfaces
} // MantidQt

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGQTWIDGET_H_
