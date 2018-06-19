#ifndef MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITPLOTPRESENTER_H_
#define MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITPLOTPRESENTER_H_

#include "IndirectFitPlotModel.h"

#include "IndirectFitPlotView.h"
#include "LazyAsyncRunner.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class DLLExport IndirectFitPlotPresenter : public QObject {
  Q_OBJECT
public:
  IndirectFitPlotPresenter(IndirectFittingModel *model,
                           IndirectFitPlotView *view);

  std::size_t getSelectedDataIndex() const;
  std::size_t getSelectedSpectrum() const;
  int getSelectedSpectrumIndex() const;
  bool isCurrentlySelected(std::size_t dataIndex, std::size_t spectrum) const;

public slots:
  void setStartX(double);
  void setEndX(double);
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
  void selectedFitDataChanged(std::size_t);
  void noFitDataSelected();
  void plotSpectrumChanged(std::size_t);
  void fitSingleSpectrum(std::size_t, std::size_t);
  void startXChanged(double);
  void endXChanged(double);
  void fwhmChanged(double);
  void backgroundChanged(double);
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
  void updateGuess(bool doPlotGuess);
  void updateFitRangeSelector();
  void plotCurrentPreview();
  void emitFitSingleSpectrum();
  void emitFWHMChanged(double minimum, double maximum);

private:
  void plotInput(Mantid::API::MatrixWorkspace_sptr workspace,
                 std::size_t spectrum);
  void plotFit(Mantid::API::MatrixWorkspace_sptr workspace,
               std::size_t spectrum);
  void plotDifference(Mantid::API::MatrixWorkspace_sptr workspace,
                      std::size_t spectrum);
  void clearInput();
  void clearFit();
  void clearDifference();
  void plotGuess(Mantid::API::MatrixWorkspace_sptr workspace);
  void plotGuessInSeparateWindow(Mantid::API::MatrixWorkspace_sptr workspace);
  void plotInput();
  void plotResult(Mantid::API::MatrixWorkspace_sptr workspace);
  void updatePlotRange(const std::pair<double, double> &range);
  void clearGuess();
  void updateHWHMSelector();
  void setHWHM(double value);
  void updateBackgroundSelector();
  void emitSelectedFitDataChanged();

  std::string getPlotString(std::size_t spectrum) const;

  std::unique_ptr<IndirectFitPlotModel> m_model;
  IndirectFitPlotView *m_view;

  bool m_plotGuessInSeparateWindow;
  MantidQt::API::PythonRunner m_pythonRunner;
  QtLazyAsyncRunner<std::function<void()>> m_plotExternalGuessRunner;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
