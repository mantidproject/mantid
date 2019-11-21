// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitPlotPresenter.h"

#include "MantidQtWidgets/Common/SignalBlocker.h"

#include <QTimer>

namespace {
using MantidQt::CustomInterfaces::IDA::IIndirectFitPlotView;
using MantidQt::CustomInterfaces::IDA::Spectra;
using MantidQt::CustomInterfaces::IDA::WorkspaceIndex;

struct UpdateAvailableSpectra : public boost::static_visitor<> {
public:
  explicit UpdateAvailableSpectra(IIndirectFitPlotView *view) : m_view(view) {}

  void operator()(const Spectra &spectra) {
    if (spectra.isContinuous()) {
      auto const minmax = spectra.getMinMax();
      m_view->setAvailableSpectra(minmax.first, minmax.second);
    } else {
      m_view->setAvailableSpectra(spectra.begin(), spectra.end());
    }
  }

private:
  IIndirectFitPlotView *m_view;
};
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

using namespace Mantid::API;

IndirectFitPlotPresenter::IndirectFitPlotPresenter(IndirectFittingModel *model,
                                                   IIndirectFitPlotView *view,
                                                   IPyRunner *pythonRunner)
    : m_model(new IndirectFitPlotModel(model)), m_view(view),
      m_plotGuessInSeparateWindow(false),
      m_plotter(std::make_unique<IndirectPlotter>(pythonRunner)) {
  connect(m_view, SIGNAL(selectedFitDataChanged(TableDatasetIndex)), this,
          SLOT(setActiveIndex(TableDatasetIndex)));
  connect(m_view, SIGNAL(selectedFitDataChanged(TableDatasetIndex)), this,
          SLOT(updateAvailableSpectra()));
  connect(m_view, SIGNAL(selectedFitDataChanged(TableDatasetIndex)), this,
          SLOT(updatePlots()));
  connect(m_view, SIGNAL(selectedFitDataChanged(TableDatasetIndex)), this,
          SLOT(updateFitRangeSelector()));
  connect(m_view, SIGNAL(selectedFitDataChanged(TableDatasetIndex)), this,
          SLOT(updateGuess()));
  connect(m_view, SIGNAL(selectedFitDataChanged(TableDatasetIndex)), this,
          SIGNAL(selectedFitDataChanged(TableDatasetIndex)));

  connect(m_view, SIGNAL(plotSpectrumChanged(WorkspaceIndex)), this,
          SLOT(setActiveSpectrum(WorkspaceIndex)));
  connect(m_view, SIGNAL(plotSpectrumChanged(WorkspaceIndex)), this,
          SLOT(updatePlots()));
  connect(m_view, SIGNAL(plotSpectrumChanged(WorkspaceIndex)), this,
          SLOT(updateFitRangeSelector()));
  connect(m_view, SIGNAL(plotSpectrumChanged(WorkspaceIndex)), this,
          SIGNAL(plotSpectrumChanged(WorkspaceIndex)));

  connect(m_view, SIGNAL(plotCurrentPreview()), this,
          SLOT(plotCurrentPreview()));

  connect(m_view, SIGNAL(fitSelectedSpectrum()), this,
          SLOT(emitFitSingleSpectrum()));

  connect(m_view, SIGNAL(plotGuessChanged(bool)), this, SLOT(plotGuess(bool)));

  connect(m_view, SIGNAL(startXChanged(double)), this,
          SLOT(setModelStartX(double)));
  connect(m_view, SIGNAL(endXChanged(double)), this,
          SLOT(setModelEndX(double)));

  connect(m_view, SIGNAL(startXChanged(double)), this,
          SIGNAL(startXChanged(double)));
  connect(m_view, SIGNAL(endXChanged(double)), this,
          SIGNAL(endXChanged(double)));

  connect(m_view, SIGNAL(hwhmMaximumChanged(double)), this,
          SLOT(setHWHMMinimum(double)));
  connect(m_view, SIGNAL(hwhmMinimumChanged(double)), this,
          SLOT(setHWHMMaximum(double)));
  connect(m_view, SIGNAL(hwhmChanged(double, double)), this,
          SLOT(setModelHWHM(double, double)));
  connect(m_view, SIGNAL(hwhmChanged(double, double)), this,
          SLOT(emitFWHMChanged(double, double)));

  connect(m_view, SIGNAL(backgroundChanged(double)), this,
          SLOT(setModelBackground(double)));
  connect(m_view, SIGNAL(backgroundChanged(double)), this,
          SIGNAL(backgroundChanged(double)));

  updateRangeSelectors();
  updateAvailableSpectra();
}

void IndirectFitPlotPresenter::watchADS(bool watch) { m_view->watchADS(watch); }

TableDatasetIndex IndirectFitPlotPresenter::getSelectedDataIndex() const {
  return m_model->getActiveDataIndex();
}

WorkspaceIndex IndirectFitPlotPresenter::getSelectedSpectrum() const {
  return m_model->getActiveSpectrum();
}

TableRowIndex IndirectFitPlotPresenter::getSelectedSpectrumIndex() const {
  return m_view->getSelectedSpectrumIndex();
}

TableRowIndex IndirectFitPlotPresenter::getSelectedDomainIndex() const {
  return m_model->getActiveDomainIndex();
}

bool IndirectFitPlotPresenter::isCurrentlySelected(
    TableDatasetIndex dataIndex, WorkspaceIndex spectrum) const {
  return getSelectedDataIndex() == dataIndex &&
         getSelectedSpectrum() == spectrum;
}

void IndirectFitPlotPresenter::setActiveIndex(TableDatasetIndex index) {
  m_model->setActiveIndex(index);
}

void IndirectFitPlotPresenter::setActiveSpectrum(WorkspaceIndex spectrum) {
  m_model->setActiveSpectrum(spectrum);
}

void IndirectFitPlotPresenter::setModelStartX(double startX) {
  m_model->setStartX(startX);
}

void IndirectFitPlotPresenter::setModelEndX(double endX) {
  m_model->setEndX(endX);
}

void IndirectFitPlotPresenter::setModelHWHM(double minimum, double maximum) {
  m_model->setFWHM(maximum - minimum);
}

void IndirectFitPlotPresenter::setModelBackground(double background) {
  m_model->setBackground(background);
}

void IndirectFitPlotPresenter::hideMultipleDataSelection() {
  m_view->hideMultipleDataSelection();
}

void IndirectFitPlotPresenter::showMultipleDataSelection() {
  m_view->showMultipleDataSelection();
}

void IndirectFitPlotPresenter::setStartX(double startX) {
  m_view->setFitRangeMinimum(startX);
}

void IndirectFitPlotPresenter::setEndX(double endX) {
  m_view->setFitRangeMaximum(endX);
}

void IndirectFitPlotPresenter::updatePlotSpectrum(WorkspaceIndex spectrum) {
  m_view->setPlotSpectrum(spectrum);
  setActiveSpectrum(spectrum);
  updatePlots();
}

void IndirectFitPlotPresenter::updateRangeSelectors() {
  updateBackgroundSelector();
  updateHWHMSelector();
}

void IndirectFitPlotPresenter::setHWHMMaximum(double minimum) {
  m_view->setHWHMMaximum(m_model->calculateHWHMMaximum(minimum));
}

void IndirectFitPlotPresenter::setHWHMMinimum(double maximum) {
  m_view->setHWHMMinimum(m_model->calculateHWHMMinimum(maximum));
}

void IndirectFitPlotPresenter::enablePlotGuessInSeparateWindow() {
  m_plotGuessInSeparateWindow = true;
  const auto inputAndGuess =
      m_model->appendGuessToInput(m_model->getGuessWorkspace());
  m_plotter->plotSpectra(inputAndGuess->getName(), "0-1");
}

void IndirectFitPlotPresenter::disablePlotGuessInSeparateWindow() {
  m_plotGuessInSeparateWindow = false;
  m_model->deleteExternalGuessWorkspace();
}

void IndirectFitPlotPresenter::appendLastDataToSelection() {
  const auto workspaceCount = m_model->numberOfWorkspaces();
  if (m_view->dataSelectionSize() == workspaceCount)
    m_view->setNameInDataSelection(m_model->getLastFitDataName(),
                                   workspaceCount - TableDatasetIndex{1});
  else
    m_view->appendToDataSelection(m_model->getLastFitDataName());
}

void IndirectFitPlotPresenter::updateSelectedDataName() {
  m_view->setNameInDataSelection(m_model->getFitDataName(),
                                 m_model->getActiveDataIndex());
}

void IndirectFitPlotPresenter::updateDataSelection() {
  MantidQt::API::SignalBlocker blocker(m_view);
  m_view->clearDataSelection();
  for (TableDatasetIndex i{0}; i < m_model->numberOfWorkspaces(); ++i)
    m_view->appendToDataSelection(m_model->getFitDataName(i));
  setActiveIndex(TableDatasetIndex{0});
  updateAvailableSpectra();
  emitSelectedFitDataChanged();
}

void IndirectFitPlotPresenter::updateAvailableSpectra() {
  if (m_model->getWorkspace()) {
    enableAllDataSelection();
    auto updateSpectra = UpdateAvailableSpectra(m_view);
    updateSpectra(m_model->getSpectra());
    setActiveSpectrum(m_view->getSelectedSpectrum());
  } else
    disableAllDataSelection();
}

void IndirectFitPlotPresenter::disableAllDataSelection() {
  m_view->enableSpectrumSelection(false);
  m_view->enableFitRangeSelection(false);
}

void IndirectFitPlotPresenter::enableAllDataSelection() {
  m_view->enableSpectrumSelection(true);
  m_view->enableFitRangeSelection(true);
}

void IndirectFitPlotPresenter::setFitSingleSpectrumIsFitting(bool fitting) {
  m_view->setFitSingleSpectrumText(fitting ? "Fitting..."
                                           : "Fit Single Spectrum");
}

void IndirectFitPlotPresenter::setFitSingleSpectrumEnabled(bool enable) {
  m_view->setFitSingleSpectrumEnabled(enable);
}

void IndirectFitPlotPresenter::updatePlots() {
  m_view->clearPreviews();

  plotLines();

  updateRangeSelectors();
  updateFitRangeSelector();
}

void IndirectFitPlotPresenter::plotLines() {
  if (auto const resultWorkspace = m_model->getResultWorkspace()) {
    plotFit(resultWorkspace);
    updatePlotRange(m_model->getResultRange());
  } else if (auto const inputWorkspace = m_model->getWorkspace()) {
    plotInput(inputWorkspace);
    updatePlotRange(m_model->getWorkspaceRange());
  }
}

void IndirectFitPlotPresenter::plotInput(MatrixWorkspace_sptr workspace) {
  plotInput(workspace, m_model->getActiveSpectrum());
  if (auto doGuess = m_view->isPlotGuessChecked())
    plotGuess(doGuess);
}

void IndirectFitPlotPresenter::plotInput(MatrixWorkspace_sptr workspace,
                                         WorkspaceIndex spectrum) {
  m_view->plotInTopPreview("Sample", workspace, spectrum, Qt::black);
}

void IndirectFitPlotPresenter::plotFit(MatrixWorkspace_sptr workspace) {
  plotInput(workspace, WorkspaceIndex{0});
  if (auto doGuess = m_view->isPlotGuessChecked())
    plotGuess(doGuess);
  plotFit(workspace, WorkspaceIndex{1});
  plotDifference(workspace, WorkspaceIndex{2});
}

void IndirectFitPlotPresenter::plotFit(MatrixWorkspace_sptr workspace,
                                       WorkspaceIndex spectrum) {
  m_view->plotInTopPreview("Fit", workspace, spectrum, Qt::red);
}

void IndirectFitPlotPresenter::plotDifference(MatrixWorkspace_sptr workspace,
                                              WorkspaceIndex spectrum) {
  m_view->plotInBottomPreview("Difference", workspace, spectrum, Qt::blue);
}

void IndirectFitPlotPresenter::updatePlotRange(
    const std::pair<double, double> &range) {
  MantidQt::API::SignalBlocker blocker(m_view);
  m_view->setFitRange(range.first, range.second);
  m_view->setHWHMRange(range.first, range.second);
}

void IndirectFitPlotPresenter::updateFitRangeSelector() {
  const auto range = m_model->getRange();
  m_view->setFitRangeMinimum(range.first);
  m_view->setFitRangeMaximum(range.second);
}

void IndirectFitPlotPresenter::plotCurrentPreview() {
  const auto inputWorkspace = m_model->getWorkspace();
  if (inputWorkspace && !inputWorkspace->getName().empty()) {
    plotSpectrum(m_model->getActiveSpectrum());
  } else
    m_view->displayMessage("Workspace not found - data may not be loaded.");
}

void IndirectFitPlotPresenter::updateGuess() {
  if (m_model->canCalculateGuess()) {
    m_view->enablePlotGuess(true);
    plotGuess(m_view->isPlotGuessChecked());
  } else {
    m_view->enablePlotGuess(false);
    clearGuess();
  }
}

void IndirectFitPlotPresenter::updateGuessAvailability() {
  if (m_model->canCalculateGuess())
    m_view->enablePlotGuess(true);
  else
    m_view->enablePlotGuess(false);
}

void IndirectFitPlotPresenter::plotGuess(bool doPlotGuess) {
  if (doPlotGuess) {
    const auto guessWorkspace = m_model->getGuessWorkspace();
    if (guessWorkspace->x(0).size() >= 2) {
      plotGuess(guessWorkspace);
      if (m_plotGuessInSeparateWindow)
        plotGuessInSeparateWindow(guessWorkspace);
    }
  } else if (m_plotGuessInSeparateWindow)
    plotGuessInSeparateWindow(m_model->getGuessWorkspace());
  else
    clearGuess();
}

void IndirectFitPlotPresenter::plotGuess(
    Mantid::API::MatrixWorkspace_sptr workspace) {
  m_view->plotInTopPreview("Guess", workspace, WorkspaceIndex{0}, Qt::green);
}

void IndirectFitPlotPresenter::plotGuessInSeparateWindow(
    Mantid::API::MatrixWorkspace_sptr workspace) {
  m_plotExternalGuessRunner.addCallback(
      [this, workspace]() { m_model->appendGuessToInput(workspace); });
}

void IndirectFitPlotPresenter::clearGuess() { updatePlots(); }

void IndirectFitPlotPresenter::updateHWHMSelector() {
  const auto hwhm = m_model->getFirstHWHM();
  m_view->setHWHMRangeVisible(hwhm ? true : false);

  if (hwhm)
    setHWHM(*hwhm);
}

void IndirectFitPlotPresenter::setHWHM(double hwhm) {
  const auto centre = m_model->getFirstPeakCentre().get_value_or(0.);
  m_view->setHWHMMaximum(centre + hwhm);
  m_view->setHWHMMinimum(centre - hwhm);
}

void IndirectFitPlotPresenter::updateBackgroundSelector() {
  const auto background = m_model->getFirstBackgroundLevel();
  m_view->setBackgroundRangeVisible(background ? true : false);

  if (background)
    m_view->setBackgroundLevel(*background);
}

void IndirectFitPlotPresenter::plotSpectrum(WorkspaceIndex spectrum) const {
  const auto resultWs = m_model->getResultWorkspace();
  if (resultWs)
    m_plotter->plotSpectra(resultWs->getName(), "0-2");
  else
    m_plotter->plotSpectra(m_model->getWorkspace()->getName(),
                           std::to_string(spectrum.value));
}

void IndirectFitPlotPresenter::emitFitSingleSpectrum() {
  emit fitSingleSpectrum(m_model->getActiveDataIndex(),
                         m_model->getActiveSpectrum());
}

void IndirectFitPlotPresenter::emitFWHMChanged(double minimum, double maximum) {
  emit fwhmChanged(maximum - minimum);
}

void IndirectFitPlotPresenter::emitSelectedFitDataChanged() {
  const auto index = m_view->getSelectedDataIndex();
  if (index.value >= 0)
    emit selectedFitDataChanged(index);
  else
    emit noFitDataSelected();
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
