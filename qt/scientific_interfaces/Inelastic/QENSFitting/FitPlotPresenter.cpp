// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FitPlotPresenter.h"
#include "FitPlotModel.h"
#include "FitPlotView.h"
#include "FitTab.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsHelper.h"

#include <utility>

using namespace MantidQt::Widgets::MplCpp;

namespace MantidQt::CustomInterfaces::Inelastic {

struct HoldRedrawing {
  IFitPlotView *m_view;

  HoldRedrawing(IFitPlotView *view) : m_view(view) { m_view->allowRedraws(false); }
  ~HoldRedrawing() {
    m_view->allowRedraws(true);
    m_view->redrawPlots();
  }
};

using namespace Mantid::API;

FitPlotPresenter::FitPlotPresenter(IFitTab *tab, IFitPlotView *view, IFitPlotModel *model)
    : m_tab(tab), m_view(view), m_model(model), m_plotter(std::make_unique<ExternalPlotter>()) {
  m_view->subscribePresenter(this);
}

void FitPlotPresenter::handleSelectedFitDataChanged(WorkspaceID workspaceID) {
  setActiveIndex(workspaceID);
  updateAvailableSpectra();
  updatePlots();
  updateGuess();
  m_tab->handlePlotSpectrumChanged();
}

void FitPlotPresenter::handlePlotSpectrumChanged(WorkspaceIndex spectrum) {
  setActiveSpectrum(spectrum);
  updatePlots();
  m_tab->handlePlotSpectrumChanged();
}

void FitPlotPresenter::watchADS(bool watch) { m_view->watchADS(watch); }

WorkspaceID FitPlotPresenter::getActiveWorkspaceID() const { return m_model->getActiveWorkspaceID(); }

WorkspaceIndex FitPlotPresenter::getActiveWorkspaceIndex() const { return m_model->getActiveWorkspaceIndex(); }

FitDomainIndex FitPlotPresenter::getSelectedDomainIndex() const { return m_model->getActiveDomainIndex(); }

bool FitPlotPresenter::isCurrentlySelected(WorkspaceID workspaceID, WorkspaceIndex spectrum) const {
  return getActiveWorkspaceID() == workspaceID && getActiveWorkspaceIndex() == spectrum;
}

void FitPlotPresenter::setActiveIndex(WorkspaceID workspaceID) { m_model->setActiveIndex(workspaceID); }

void FitPlotPresenter::setActiveSpectrum(WorkspaceIndex spectrum) {
  m_model->setActiveSpectrum(spectrum);
  m_view->setPlotSpectrum(spectrum);
}

void FitPlotPresenter::setStartX(double value) { m_view->setFitRangeMinimum(value); }

void FitPlotPresenter::setEndX(double value) { m_view->setFitRangeMaximum(value); }

void FitPlotPresenter::setXBounds(std::pair<double, double> const &bounds) { m_view->setFitRangeBounds(bounds); }

void FitPlotPresenter::updateRangeSelectors() {
  updateBackgroundSelector();
  updateHWHMSelector();
}

void FitPlotPresenter::handleStartXChanged(double value) { m_tab->handleStartXChanged(value); }

void FitPlotPresenter::handleEndXChanged(double value) { m_tab->handleEndXChanged(value); }

void FitPlotPresenter::handleHWHMMaximumChanged(double minimum) {
  m_view->setHWHMMaximum(m_model->calculateHWHMMaximum(minimum));
}

void FitPlotPresenter::handleHWHMMinimumChanged(double maximum) {
  m_view->setHWHMMinimum(m_model->calculateHWHMMinimum(maximum));
}

void FitPlotPresenter::appendLastDataToSelection(std::vector<std::string> const &displayNames) {
  const auto workspaceCount = displayNames.size();
  if (m_view->dataSelectionSize() == workspaceCount) {
    // if adding a spectra to an existing workspace, update all the combo box
    // entries.
    for (size_t i = 0; i < workspaceCount; i++) {
      m_view->setNameInDataSelection(displayNames[i], WorkspaceID(i));
    }
  } else
    m_view->appendToDataSelection(displayNames.back());
}

void FitPlotPresenter::updateDataSelection(std::vector<std::string> const &displayNames) {
  m_view->clearDataSelection();
  const auto workspaceCount = displayNames.size();
  for (size_t i = 0; i < workspaceCount; ++i) {
    m_view->appendToDataSelection(displayNames[i]);
  }
  setActiveIndex(WorkspaceID{0});
  setActiveSpectrum(WorkspaceIndex{0});
  updateAvailableSpectra();
  m_tab->handlePlotSpectrumChanged();
}

void FitPlotPresenter::updateAvailableSpectra() {
  if (m_model->getWorkspace()) {
    enableAllDataSelection();
    auto spectra = m_model->getSpectra(m_model->getActiveWorkspaceID());
    if (spectra.isContinuous()) {
      auto const minmax = spectra.getMinMax();
      m_view->setAvailableSpectra(minmax.first, minmax.second);
    } else {
      m_view->setAvailableSpectra(spectra.begin(), spectra.end());
    }
    setActiveSpectrum(m_view->getSelectedSpectrum());
  } else
    disableAllDataSelection();
}

void FitPlotPresenter::disableAllDataSelection() {
  m_view->enableSpectrumSelection(false);
  m_view->enableFitRangeSelection(false);
}

void FitPlotPresenter::enableAllDataSelection() {
  m_view->enableSpectrumSelection(true);
  m_view->enableFitRangeSelection(true);
}

void FitPlotPresenter::setFitFunction(Mantid::API::MultiDomainFunction_sptr function) {
  m_model->setFitFunction(std::move(function));
}

void FitPlotPresenter::setFitSingleSpectrumIsFitting(bool fitting) {
  m_view->setFitSingleSpectrumText(fitting ? "Fitting..." : "Fit Single Spectrum");
}

void FitPlotPresenter::setFitSingleSpectrumEnabled(bool enable) { m_view->setFitSingleSpectrumEnabled(enable); }

void FitPlotPresenter::updatePlots() {
  HoldRedrawing holdRedrawing(m_view);
  m_view->clearPreviews();
  plotLines();

  updateRangeSelectors();
  updateFitRangeSelector();
}

void FitPlotPresenter::updateFit() {
  HoldRedrawing holdRedrawing(m_view);
  updateGuess();
}

void FitPlotPresenter::plotLines() {
  if (auto const resultWorkspace = m_model->getResultWorkspace()) {
    plotInput(m_model->getWorkspace(), m_model->getActiveWorkspaceIndex());
    plotFit(resultWorkspace);
    updatePlotRange(m_model->getResultRange());
  } else if (auto const inputWorkspace = m_model->getWorkspace()) {
    plotInput(inputWorkspace);
    updatePlotRange(m_model->getWorkspaceRange());
  }
}

void FitPlotPresenter::plotInput(MatrixWorkspace_sptr workspace) {
  plotInput(std::move(workspace), m_model->getActiveWorkspaceIndex());
  if (auto doGuess = m_view->isPlotGuessChecked())
    handlePlotGuess(doGuess);
}

void FitPlotPresenter::plotInput(MatrixWorkspace_sptr workspace, WorkspaceIndex spectrum) {
  m_view->plotInTopPreview("Sample", std::move(workspace), spectrum, Qt::black);
}

void FitPlotPresenter::plotFit(const MatrixWorkspace_sptr &workspace) {
  if (auto doGuess = m_view->isPlotGuessChecked())
    handlePlotGuess(doGuess);
  plotFit(workspace, WorkspaceIndex{1});
  plotDifference(workspace, WorkspaceIndex{2});
}

void FitPlotPresenter::plotFit(MatrixWorkspace_sptr workspace, WorkspaceIndex spectrum) {
  m_view->plotInTopPreview("Fit", std::move(workspace), spectrum, Qt::red);
}

void FitPlotPresenter::plotDifference(MatrixWorkspace_sptr workspace, WorkspaceIndex spectrum) {
  m_view->plotInBottomPreview("Difference", std::move(workspace), spectrum, Qt::blue);
}

void FitPlotPresenter::updatePlotRange(const std::pair<double, double> &range) {
  m_view->setFitRange(range.first, range.second);
  m_view->setHWHMRange(range.first, range.second);
}

void FitPlotPresenter::updateFitRangeSelector() {
  const auto range = m_model->getRange();
  m_view->setFitRangeMinimum(range.first);
  m_view->setFitRangeMaximum(range.second);
}

void FitPlotPresenter::handlePlotCurrentPreview() {
  const auto inputWorkspace = m_model->getWorkspace();
  if (inputWorkspace && !inputWorkspace->getName().empty()) {
    plotSpectrum(m_model->getActiveWorkspaceIndex());
  } else
    m_view->displayMessage("Workspace not found - data may not be loaded.");
}

void FitPlotPresenter::updateGuess() {
  if (m_model->canCalculateGuess()) {
    m_view->enablePlotGuess(true);
    handlePlotGuess(m_view->isPlotGuessChecked());
  } else {
    m_view->enablePlotGuess(false);
    clearGuess();
  }
}

void FitPlotPresenter::updateGuessAvailability() {
  if (m_model->canCalculateGuess())
    m_view->enablePlotGuess(true);
  else
    m_view->enablePlotGuess(false);
}

void FitPlotPresenter::handlePlotGuess(bool doPlotGuess) {
  if (doPlotGuess) {
    const auto guessWorkspace = m_model->getGuessWorkspace();
    if (guessWorkspace && guessWorkspace->x(0).size() >= 2) {
      plotGuess(guessWorkspace);
    }
  } else {
    clearGuess();
  }
}

void FitPlotPresenter::plotGuess(Mantid::API::MatrixWorkspace_sptr workspace) {
  m_view->plotInTopPreview("Guess", std::move(workspace), WorkspaceIndex{0}, Qt::green);
}

void FitPlotPresenter::clearGuess() {
  m_view->removeFromTopPreview("Guess");
  m_view->redrawPlots();
}

void FitPlotPresenter::updateHWHMSelector() {
  const auto hwhm = m_model->getFirstHWHM();
  m_view->setHWHMRangeVisible(hwhm ? true : false);

  if (hwhm)
    setHWHM(*hwhm);
}

void FitPlotPresenter::setHWHM(double hwhm) {
  const auto centre = m_model->getFirstPeakCentre().value_or(0.);
  m_view->setHWHMMaximum(centre + hwhm);
  m_view->setHWHMMinimum(centre - hwhm);
}

void FitPlotPresenter::updateBackgroundSelector() {
  const auto background = m_model->getFirstBackgroundLevel();
  m_view->setBackgroundRangeVisible(background ? true : false);

  if (background)
    m_view->setBackgroundLevel(*background);
}

void FitPlotPresenter::plotSpectrum(WorkspaceIndex spectrum) const {
  const auto resultWs = m_model->getResultWorkspace();
  const auto errorBars = SettingsHelper::externalPlotErrorBars();
  if (resultWs)
    m_plotter->plotSpectra(resultWs->getName(), "0-2", errorBars);
  else
    m_plotter->plotSpectra(m_model->getWorkspace()->getName(), std::to_string(spectrum.value), errorBars);
}

void FitPlotPresenter::handleFitSingleSpectrum() { m_tab->handleSingleFitClicked(); }

void FitPlotPresenter::handleFWHMChanged(double minimum, double maximum) {
  m_tab->handleFwhmChanged(maximum - minimum);
}

void FitPlotPresenter::handleBackgroundChanged(double value) { m_tab->handleBackgroundChanged(value); }

} // namespace MantidQt::CustomInterfaces::Inelastic
