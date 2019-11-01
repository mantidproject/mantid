// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitPlotPresenterLegacy.h"

#include "MantidQtWidgets/Common/SignalBlocker.h"

#include <QTimer>

namespace {
using MantidQt::CustomInterfaces::IDA::DiscontinuousSpectra;
using MantidQt::CustomInterfaces::IDA::IIndirectFitPlotViewLegacy;

struct UpdateAvailableSpectra : public boost::static_visitor<> {
public:
  explicit UpdateAvailableSpectra(IIndirectFitPlotViewLegacy *view)
      : m_view(view) {}

  void operator()(const std::pair<std::size_t, std::size_t> &spectra) {
    m_view->setAvailableSpectra(spectra.first, spectra.second);
  }

  void operator()(const DiscontinuousSpectra<std::size_t> &spectra) {
    m_view->setAvailableSpectra(spectra.begin(), spectra.end());
  }

private:
  IIndirectFitPlotViewLegacy *m_view;
};
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

using namespace Mantid::API;

IndirectFitPlotPresenterLegacy::IndirectFitPlotPresenterLegacy(
    IndirectFittingModelLegacy *model, IIndirectFitPlotViewLegacy *view,
    IPyRunner *pythonRunner)
    : m_model(new IndirectFitPlotModelLegacy(model)), m_view(view),
      m_plotGuessInSeparateWindow(false),
      m_plotter(std::make_unique<IndirectPlotter>(pythonRunner)) {
  connect(m_view, SIGNAL(selectedFitDataChanged(std::size_t)), this,
          SLOT(setActiveIndex(std::size_t)));
  connect(m_view, SIGNAL(selectedFitDataChanged(std::size_t)), this,
          SLOT(updateAvailableSpectra()));
  connect(m_view, SIGNAL(selectedFitDataChanged(std::size_t)), this,
          SLOT(updatePlots()));
  connect(m_view, SIGNAL(selectedFitDataChanged(std::size_t)), this,
          SIGNAL(selectedFitDataChanged(std::size_t)));

  connect(m_view, SIGNAL(plotSpectrumChanged(std::size_t)), this,
          SLOT(setActiveSpectrum(std::size_t)));
  connect(m_view, SIGNAL(plotSpectrumChanged(std::size_t)), this,
          SLOT(updatePlots()));
  connect(m_view, SIGNAL(plotSpectrumChanged(std::size_t)), this,
          SIGNAL(plotSpectrumChanged(std::size_t)));

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

void IndirectFitPlotPresenterLegacy::watchADS(bool watch) {
  m_view->watchADS(watch);
}

std::size_t IndirectFitPlotPresenterLegacy::getSelectedDataIndex() const {
  return m_model->getActiveDataIndex();
}

std::size_t IndirectFitPlotPresenterLegacy::getSelectedSpectrum() const {
  return m_model->getActiveSpectrum();
}

int IndirectFitPlotPresenterLegacy::getSelectedSpectrumIndex() const {
  return m_view->getSelectedSpectrumIndex();
}

bool IndirectFitPlotPresenterLegacy::isCurrentlySelected(
    std::size_t dataIndex, std::size_t spectrum) const {
  return getSelectedDataIndex() == dataIndex &&
         getSelectedSpectrum() == spectrum;
}

void IndirectFitPlotPresenterLegacy::setActiveIndex(std::size_t index) {
  m_model->setActiveIndex(index);
}

void IndirectFitPlotPresenterLegacy::setActiveSpectrum(std::size_t spectrum) {
  m_model->setActiveSpectrum(spectrum);
}

void IndirectFitPlotPresenterLegacy::setModelStartX(double startX) {
  m_model->setStartX(startX);
}

void IndirectFitPlotPresenterLegacy::setModelEndX(double endX) {
  m_model->setEndX(endX);
}

void IndirectFitPlotPresenterLegacy::setModelHWHM(double minimum,
                                                  double maximum) {
  m_model->setFWHM(maximum - minimum);
}

void IndirectFitPlotPresenterLegacy::setModelBackground(double background) {
  m_model->setBackground(background);
}

void IndirectFitPlotPresenterLegacy::hideMultipleDataSelection() {
  m_view->hideMultipleDataSelection();
}

void IndirectFitPlotPresenterLegacy::showMultipleDataSelection() {
  m_view->showMultipleDataSelection();
}

void IndirectFitPlotPresenterLegacy::setStartX(double startX) {
  m_view->setFitRangeMinimum(startX);
}

void IndirectFitPlotPresenterLegacy::setEndX(double endX) {
  m_view->setFitRangeMaximum(endX);
}

void IndirectFitPlotPresenterLegacy::updatePlotSpectrum(int spectrum) {
  m_view->setPlotSpectrum(spectrum);
  setActiveSpectrum(static_cast<std::size_t>(spectrum));
  updatePlots();
}

void IndirectFitPlotPresenterLegacy::updateRangeSelectors() {
  updateBackgroundSelector();
  updateHWHMSelector();
}

void IndirectFitPlotPresenterLegacy::setHWHMMaximum(double minimum) {
  m_view->setHWHMMaximum(m_model->calculateHWHMMaximum(minimum));
}

void IndirectFitPlotPresenterLegacy::setHWHMMinimum(double maximum) {
  m_view->setHWHMMinimum(m_model->calculateHWHMMinimum(maximum));
}

void IndirectFitPlotPresenterLegacy::enablePlotGuessInSeparateWindow() {
  m_plotGuessInSeparateWindow = true;
  const auto inputAndGuess =
      m_model->appendGuessToInput(m_model->getGuessWorkspace());
  m_plotter->plotSpectra(inputAndGuess->getName(), "0-1");
}

void IndirectFitPlotPresenterLegacy::disablePlotGuessInSeparateWindow() {
  m_plotGuessInSeparateWindow = false;
  m_model->deleteExternalGuessWorkspace();
}

void IndirectFitPlotPresenterLegacy::appendLastDataToSelection() {
  const auto workspaceCount = m_model->numberOfWorkspaces();
  if (m_view->dataSelectionSize() == workspaceCount)
    m_view->setNameInDataSelection(m_model->getLastFitDataName(),
                                   workspaceCount - 1);
  else
    m_view->appendToDataSelection(m_model->getLastFitDataName());
}

void IndirectFitPlotPresenterLegacy::updateSelectedDataName() {
  m_view->setNameInDataSelection(m_model->getFitDataName(),
                                 m_model->getActiveDataIndex());
}

void IndirectFitPlotPresenterLegacy::updateDataSelection() {
  MantidQt::API::SignalBlocker blocker(m_view);
  m_view->clearDataSelection();
  for (auto i = 0u; i < m_model->numberOfWorkspaces(); ++i)
    m_view->appendToDataSelection(m_model->getFitDataName(i));
  setActiveIndex(0);
  updateAvailableSpectra();
  emitSelectedFitDataChanged();
}

void IndirectFitPlotPresenterLegacy::updateAvailableSpectra() {
  if (m_model->getWorkspace()) {
    enableAllDataSelection();
    auto updateSpectra = UpdateAvailableSpectra(m_view);
    m_model->getSpectra().apply_visitor(updateSpectra);
    setActiveSpectrum(m_view->getSelectedSpectrum());
  } else
    disableAllDataSelection();
}

void IndirectFitPlotPresenterLegacy::disableAllDataSelection() {
  m_view->enableSpectrumSelection(false);
  m_view->enableFitRangeSelection(false);
}

void IndirectFitPlotPresenterLegacy::enableAllDataSelection() {
  m_view->enableSpectrumSelection(true);
  m_view->enableFitRangeSelection(true);
}

void IndirectFitPlotPresenterLegacy::setFitSingleSpectrumIsFitting(
    bool fitting) {
  m_view->setFitSingleSpectrumText(fitting ? "Fitting..."
                                           : "Fit Single Spectrum");
}

void IndirectFitPlotPresenterLegacy::setFitSingleSpectrumEnabled(bool enable) {
  m_view->setFitSingleSpectrumEnabled(enable);
}

void IndirectFitPlotPresenterLegacy::updatePlots() {
  m_view->clearPreviews();

  plotLines();

  updateRangeSelectors();
  updateFitRangeSelector();
}

void IndirectFitPlotPresenterLegacy::plotLines() {
  if (auto const resultWorkspace = m_model->getResultWorkspace()) {
    plotFit(resultWorkspace);
    updatePlotRange(m_model->getResultRange());
  } else if (auto const inputWorkspace = m_model->getWorkspace()) {
    plotInput(inputWorkspace);
    updatePlotRange(m_model->getWorkspaceRange());
  }
}

void IndirectFitPlotPresenterLegacy::plotInput(MatrixWorkspace_sptr workspace) {
  plotInput(workspace, m_model->getActiveSpectrum());
  if (auto doGuess = m_view->isPlotGuessChecked())
    plotGuess(doGuess);
}

void IndirectFitPlotPresenterLegacy::plotInput(MatrixWorkspace_sptr workspace,
                                               std::size_t spectrum) {
  m_view->plotInTopPreview("Sample", workspace, spectrum, Qt::black);
}

void IndirectFitPlotPresenterLegacy::plotFit(MatrixWorkspace_sptr workspace) {
  plotInput(workspace, 0);
  if (auto doGuess = m_view->isPlotGuessChecked())
    plotGuess(doGuess);
  plotFit(workspace, 1);
  plotDifference(workspace, 2);
}

void IndirectFitPlotPresenterLegacy::plotFit(MatrixWorkspace_sptr workspace,
                                             std::size_t spectrum) {
  m_view->plotInTopPreview("Fit", workspace, spectrum, Qt::red);
}

void IndirectFitPlotPresenterLegacy::plotDifference(
    MatrixWorkspace_sptr workspace, std::size_t spectrum) {
  m_view->plotInBottomPreview("Difference", workspace, spectrum, Qt::blue);
}

void IndirectFitPlotPresenterLegacy::updatePlotRange(
    const std::pair<double, double> &range) {
  MantidQt::API::SignalBlocker blocker(m_view);
  m_view->setFitRange(range.first, range.second);
  m_view->setHWHMRange(range.first, range.second);
}

void IndirectFitPlotPresenterLegacy::updateFitRangeSelector() {
  const auto range = m_model->getRange();
  m_view->setFitRangeMinimum(range.first);
  m_view->setFitRangeMaximum(range.second);
}

void IndirectFitPlotPresenterLegacy::plotCurrentPreview() {
  const auto inputWorkspace = m_model->getWorkspace();
  if (inputWorkspace && !inputWorkspace->getName().empty()) {
    plotSpectrum(m_model->getActiveSpectrum());
  } else
    m_view->displayMessage("Workspace not found - data may not be loaded.");
}

void IndirectFitPlotPresenterLegacy::updateGuess() {
  if (m_model->canCalculateGuess()) {
    m_view->enablePlotGuess(true);
    plotGuess(m_view->isPlotGuessChecked());
  } else {
    m_view->enablePlotGuess(false);
    clearGuess();
  }
}

void IndirectFitPlotPresenterLegacy::updateGuessAvailability() {
  if (m_model->canCalculateGuess())
    m_view->enablePlotGuess(true);
  else
    m_view->enablePlotGuess(false);
}

void IndirectFitPlotPresenterLegacy::plotGuess(bool doPlotGuess) {
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

void IndirectFitPlotPresenterLegacy::plotGuess(
    Mantid::API::MatrixWorkspace_sptr workspace) {
  m_view->plotInTopPreview("Guess", workspace, 0, Qt::green);
}

void IndirectFitPlotPresenterLegacy::plotGuessInSeparateWindow(
    Mantid::API::MatrixWorkspace_sptr workspace) {
  m_plotExternalGuessRunner.addCallback(
      [this, workspace]() { m_model->appendGuessToInput(workspace); });
}

void IndirectFitPlotPresenterLegacy::clearGuess() { updatePlots(); }

void IndirectFitPlotPresenterLegacy::updateHWHMSelector() {
  const auto hwhm = m_model->getFirstHWHM();
  m_view->setHWHMRangeVisible(hwhm ? true : false);

  if (hwhm)
    setHWHM(*hwhm);
}

void IndirectFitPlotPresenterLegacy::setHWHM(double hwhm) {
  const auto centre = m_model->getFirstPeakCentre().get_value_or(0.);
  m_view->setHWHMMaximum(centre + hwhm);
  m_view->setHWHMMinimum(centre - hwhm);
}

void IndirectFitPlotPresenterLegacy::updateBackgroundSelector() {
  const auto background = m_model->getFirstBackgroundLevel();
  m_view->setBackgroundRangeVisible(background ? true : false);

  if (background)
    m_view->setBackgroundLevel(*background);
}

void IndirectFitPlotPresenterLegacy::plotSpectrum(std::size_t spectrum) const {
  const auto resultWs = m_model->getResultWorkspace();
  if (resultWs)
    m_plotter->plotSpectra(resultWs->getName(), "0-2");
  else
    m_plotter->plotSpectra(m_model->getWorkspace()->getName(),
                           std::to_string(spectrum));
}

void IndirectFitPlotPresenterLegacy::emitFitSingleSpectrum() {
  emit fitSingleSpectrum(m_model->getActiveDataIndex(),
                         m_model->getActiveSpectrum());
}

void IndirectFitPlotPresenterLegacy::emitFWHMChanged(double minimum,
                                                     double maximum) {
  emit fwhmChanged(maximum - minimum);
}

void IndirectFitPlotPresenterLegacy::emitSelectedFitDataChanged() {
  const auto index = m_view->getSelectedDataIndex();
  if (index >= 0)
    emit selectedFitDataChanged(static_cast<std::size_t>(index));
  else
    emit noFitDataSelected();
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt