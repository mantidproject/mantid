// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITPLOTPRESENTER_H_
#define MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITPLOTPRESENTER_H_

#include "DllConfig.h"

#include "IndexTypes.h"
#include "IndirectFitPlotModel.h"
#include "IndirectPlotter.h"

#include "IIndirectFitPlotView.h"
#include "LazyAsyncRunner.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL IndirectFitPlotPresenter : public QObject {
  Q_OBJECT

public:
  IndirectFitPlotPresenter(IndirectFittingModel *model,
                           IIndirectFitPlotView *view,
                           IPyRunner *pythonRunner = nullptr);

  void watchADS(bool watch);

  TableDatasetIndex getSelectedDataIndex() const;
  WorkspaceIndex getSelectedSpectrum() const;
  TableRowIndex getSelectedSpectrumIndex() const;
  TableRowIndex getSelectedDomainIndex() const;
  bool isCurrentlySelected(TableDatasetIndex dataIndex,
                           WorkspaceIndex spectrum) const;

  void setFitSingleSpectrumIsFitting(bool fitting);
  void setFitSingleSpectrumEnabled(bool enable);

public slots:
  void setStartX(double /*startX*/);
  void setEndX(double /*endX*/);
  void updatePlotSpectrum(WorkspaceIndex spectrum);
  void hideMultipleDataSelection();
  void showMultipleDataSelection();
  void updateRangeSelectors();
  void appendLastDataToSelection();
  void updateSelectedDataName();
  void updateDataSelection();
  void updateAvailableSpectra();
  void updatePlots();
  void updateGuess();
  void updateGuessAvailability();
  void enablePlotGuessInSeparateWindow();
  void disablePlotGuessInSeparateWindow();
  void disableSpectrumPlotSelection();
  void handlePlotSpectrumChanged(WorkspaceIndex spectrum);

signals:
  void selectedFitDataChanged(TableDatasetIndex /*_t1*/);
  void noFitDataSelected();
  void plotSpectrumChanged(WorkspaceIndex /*_t1*/);
  void fitSingleSpectrum(TableDatasetIndex /*_t1*/, WorkspaceIndex /*_t2*/);
  void startXChanged(double /*_t1*/);
  void endXChanged(double /*_t1*/);
  void fwhmChanged(double /*_t1*/);
  void backgroundChanged(double /*_t1*/);
  void runAsPythonScript(const QString &code, bool noOutput = false);

private slots:
  void setModelStartX(double value);
  void setModelEndX(double value);
  void setModelHWHM(double minimum, double maximum);
  void setModelBackground(double background);
  void setActiveIndex(TableDatasetIndex index);
  void setHWHMMaximum(double minimum);
  void setHWHMMinimum(double maximum);
  void plotGuess(bool doPlotGuess);
  void updateFitRangeSelector();
  void plotCurrentPreview();
  void emitFitSingleSpectrum();
  void emitFWHMChanged(double minimum, double maximum);
  void setActiveSpectrum(WorkspaceIndex spectrum);

private:
  void disableAllDataSelection();
  void enableAllDataSelection();
  void plotInput(Mantid::API::MatrixWorkspace_sptr workspace);
  void plotInput(Mantid::API::MatrixWorkspace_sptr workspace,
                 WorkspaceIndex spectrum);
  void plotFit(Mantid::API::MatrixWorkspace_sptr workspace);
  void plotFit(Mantid::API::MatrixWorkspace_sptr workspace,
               WorkspaceIndex spectrum);
  void plotDifference(Mantid::API::MatrixWorkspace_sptr workspace,
                      WorkspaceIndex spectrum);
  void clearInput();
  void clearFit();
  void clearDifference();
  void plotGuess(Mantid::API::MatrixWorkspace_sptr workspace);
  void plotGuessInSeparateWindow(Mantid::API::MatrixWorkspace_sptr workspace);
  void plotLines();
  void updatePlotRange(const std::pair<double, double> &range);
  void clearGuess();
  void updateHWHMSelector();
  void setHWHM(double value);
  void updateBackgroundSelector();
  void emitSelectedFitDataChanged();

  void plotSpectrum(WorkspaceIndex spectrum) const;

  std::unique_ptr<IndirectFitPlotModel> m_model;
  IIndirectFitPlotView *m_view;

  bool m_plotGuessInSeparateWindow;
  QtLazyAsyncRunner<std::function<void()>> m_plotExternalGuessRunner;
  std::unique_ptr<IndirectPlotter> m_plotter;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
