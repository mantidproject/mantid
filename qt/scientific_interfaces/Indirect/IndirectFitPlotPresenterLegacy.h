// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITPLOTPRESENTERLEGACY_H_
#define MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITPLOTPRESENTERLEGACY_H_

#include "DllConfig.h"

#include "IndirectFitPlotModelLegacy.h"
#include "IndirectPlotter.h"

#include "IIndirectFitPlotViewLegacy.h"
#include "LazyAsyncRunner.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL IndirectFitPlotPresenterLegacy : public QObject {
  Q_OBJECT

public:
  IndirectFitPlotPresenterLegacy(IndirectFittingModelLegacy *model,
                                 IIndirectFitPlotViewLegacy *view,
                                 IPyRunner *pythonRunner = nullptr);

  void watchADS(bool watch);

  std::size_t getSelectedDataIndex() const;
  std::size_t getSelectedSpectrum() const;
  int getSelectedSpectrumIndex() const;
  bool isCurrentlySelected(std::size_t dataIndex, std::size_t spectrum) const;

  void setFitSingleSpectrumIsFitting(bool fitting);
  void setFitSingleSpectrumEnabled(bool enable);

public slots:
  void setStartX(double /*startX*/);
  void setEndX(double /*endX*/);
  void updatePlotSpectrum(int spectrum);
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

signals:
  void selectedFitDataChanged(std::size_t /*_t1*/);
  void noFitDataSelected();
  void plotSpectrumChanged(std::size_t /*_t1*/);
  void fitSingleSpectrum(std::size_t /*_t1*/, std::size_t /*_t2*/);
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
  void setActiveIndex(std::size_t index);
  void setActiveSpectrum(std::size_t spectrum);
  void setHWHMMaximum(double minimum);
  void setHWHMMinimum(double maximum);
  void plotGuess(bool doPlotGuess);
  void updateFitRangeSelector();
  void plotCurrentPreview();
  void emitFitSingleSpectrum();
  void emitFWHMChanged(double minimum, double maximum);

private:
  void disableAllDataSelection();
  void enableAllDataSelection();
  void plotInput(Mantid::API::MatrixWorkspace_sptr workspace);
  void plotInput(Mantid::API::MatrixWorkspace_sptr workspace,
                 std::size_t spectrum);
  void plotFit(Mantid::API::MatrixWorkspace_sptr workspace);
  void plotFit(Mantid::API::MatrixWorkspace_sptr workspace,
               std::size_t spectrum);
  void plotDifference(Mantid::API::MatrixWorkspace_sptr workspace,
                      std::size_t spectrum);
  void plotGuess(Mantid::API::MatrixWorkspace_sptr workspace);
  void plotGuessInSeparateWindow(Mantid::API::MatrixWorkspace_sptr workspace);
  void plotLines();
  void updatePlotRange(const std::pair<double, double> &range);
  void clearGuess();
  void updateHWHMSelector();
  void setHWHM(double value);
  void updateBackgroundSelector();
  void emitSelectedFitDataChanged();

  void plotSpectrum(std::size_t spectrum) const;

  std::unique_ptr<IndirectFitPlotModelLegacy> m_model;
  IIndirectFitPlotViewLegacy *m_view;

  bool m_plotGuessInSeparateWindow;
  QtLazyAsyncRunner<std::function<void()>> m_plotExternalGuessRunner;
  std::unique_ptr<IndirectPlotter> m_plotter;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
