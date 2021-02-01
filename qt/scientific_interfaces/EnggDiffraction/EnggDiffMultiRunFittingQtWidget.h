// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "IEnggDiffMultiRunFittingWidgetPresenter.h"
#include "IEnggDiffMultiRunFittingWidgetView.h"
#include "IEnggDiffractionPythonRunner.h"
#include "IEnggDiffractionUserMsg.h"
#include "ui_EnggDiffMultiRunFittingWidget.h"

#include <memory>
#include <qwt_plot_curve.h>
#include <qwt_plot_zoomer.h>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ENGGDIFFRACTION_DLL EnggDiffMultiRunFittingQtWidget : public QWidget,
                                                                     public IEnggDiffMultiRunFittingWidgetView {
  Q_OBJECT

public:
  EnggDiffMultiRunFittingQtWidget(std::shared_ptr<IEnggDiffractionPythonRunner> pythonRunner);

  ~EnggDiffMultiRunFittingQtWidget() override;

  std::vector<RunLabel> getAllRunLabels() const override;

  boost::optional<RunLabel> getSelectedRunLabel() const override;

  void plotFittedPeaks(const std::vector<std::shared_ptr<QwtData>> &curve) override;

  void plotFocusedRun(const std::vector<std::shared_ptr<QwtData>> &curve) override;

  void plotToSeparateWindow(const std::string &focusedRunName,
                            const boost::optional<std::string> fittedPeaksName) override;

  void reportNoRunSelectedForPlot() override;

  void reportPlotInvalidFittedPeaks(const RunLabel &runLabel) override;

  void reportPlotInvalidFocusedRun(const RunLabel &runLabel) override;

  void resetCanvas() override;

  void setEnabled(const bool enabled) override;

  void setMessageProvider(std::shared_ptr<IEnggDiffractionUserMsg> messageProvider) override;

  void setPresenter(std::shared_ptr<IEnggDiffMultiRunFittingWidgetPresenter> presenter) override;

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

  void userError(const std::string &errorTitle, const std::string &errorDescription);

  std::vector<std::unique_ptr<QwtPlotCurve>> m_fittedPeaksCurves;

  std::vector<std::unique_ptr<QwtPlotCurve>> m_focusedRunCurves;

  std::unique_ptr<QwtPlotZoomer> m_zoomTool;

  std::shared_ptr<IEnggDiffMultiRunFittingWidgetPresenter> m_presenter;

  std::shared_ptr<IEnggDiffractionPythonRunner> m_pythonRunner;

  Ui::EnggDiffMultiRunFittingWidget m_ui;

  std::shared_ptr<IEnggDiffractionUserMsg> m_userMessageProvider;
};

} // namespace CustomInterfaces
} // namespace MantidQt
