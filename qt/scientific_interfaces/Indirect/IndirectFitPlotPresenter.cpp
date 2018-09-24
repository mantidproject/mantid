#include "IndirectFitPlotPresenter.h"

#include "MantidQtWidgets/Common/SignalBlocker.h"

namespace {
using MantidQt::CustomInterfaces::IDA::DiscontinuousSpectra;
using MantidQt::CustomInterfaces::IDA::IndirectFitPlotView;

std::string createPlotString(const std::string &workspaceName,
                             const std::string &spectra) {
  std::string plotString = "from mantidplot import plotSpectrum\n";
  return plotString + "plotSpectrum(['" + workspaceName + "'], " + spectra +
         ")\n";
}

std::string createPlotString(const std::string &workspaceName,
                             std::size_t spectrum) {
  return createPlotString(workspaceName, std::to_string(spectrum));
}

struct UpdateAvailableSpectra : public boost::static_visitor<> {
public:
  explicit UpdateAvailableSpectra(IndirectFitPlotView *view) : m_view(view) {}

  void operator()(const std::pair<std::size_t, std::size_t> &spectra) {
    m_view->setAvailableSpectra(spectra.first, spectra.second);
  }

  void operator()(const DiscontinuousSpectra<std::size_t> &spectra) {
    m_view->setAvailableSpectra(spectra.begin(), spectra.end());
  }

private:
  IndirectFitPlotView *m_view;
};
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

using namespace Mantid::API;

IndirectFitPlotPresenter::IndirectFitPlotPresenter(IndirectFittingModel *model,
                                                   IndirectFitPlotView *view)
    : m_model(new IndirectFitPlotModel(model)), m_view(view),
      m_plotGuessInSeparateWindow(false) {
  connect(m_view, SIGNAL(selectedFitDataChanged(std::size_t)), this,
          SLOT(setActiveIndex(std::size_t)));
  connect(m_view, SIGNAL(selectedFitDataChanged(std::size_t)), this,
          SLOT(updateAvailableSpectra()));
  connect(m_view, SIGNAL(selectedFitDataChanged(std::size_t)), this,
          SLOT(updatePlots()));
  connect(m_view, SIGNAL(selectedFitDataChanged(std::size_t)), this,
          SLOT(updateFitRangeSelector()));
  connect(m_view, SIGNAL(selectedFitDataChanged(std::size_t)), this,
          SLOT(updateGuess()));
  connect(m_view, SIGNAL(selectedFitDataChanged(std::size_t)), this,
          SIGNAL(selectedFitDataChanged(std::size_t)));

  connect(m_view, SIGNAL(plotSpectrumChanged(std::size_t)), this,
          SLOT(setActiveSpectrum(std::size_t)));
  connect(m_view, SIGNAL(plotSpectrumChanged(std::size_t)), this,
          SLOT(updatePlots()));
  connect(m_view, SIGNAL(plotSpectrumChanged(std::size_t)), this,
          SLOT(updateFitRangeSelector()));
  connect(m_view, SIGNAL(plotSpectrumChanged(std::size_t)), this,
          SIGNAL(plotSpectrumChanged(std::size_t)));

  connect(m_view, SIGNAL(plotCurrentPreview()), this,
          SLOT(plotCurrentPreview()));

  connect(m_view, SIGNAL(fitSelectedSpectrum()), this,
          SLOT(emitFitSingleSpectrum()));

  connect(m_view, SIGNAL(plotGuessChanged(bool)), this,
          SLOT(updateGuess(bool)));

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

  connect(&m_pythonRunner, SIGNAL(runAsPythonScript(const QString &, bool)),
          this, SIGNAL(runAsPythonScript(const QString &, bool)));

  updateRangeSelectors();
  updateAvailableSpectra();
}

std::size_t IndirectFitPlotPresenter::getSelectedDataIndex() const {
  return m_model->getActiveDataIndex();
}

std::size_t IndirectFitPlotPresenter::getSelectedSpectrum() const {
  return m_model->getActiveSpectrum();
}

int IndirectFitPlotPresenter::getSelectedSpectrumIndex() const {
  return m_view->getSelectedSpectrumIndex();
}

bool IndirectFitPlotPresenter::isCurrentlySelected(std::size_t dataIndex,
                                                   std::size_t spectrum) const {
  return getSelectedDataIndex() == dataIndex &&
         getSelectedSpectrum() == spectrum;
}

void IndirectFitPlotPresenter::setActiveIndex(std::size_t index) {
  m_model->setActiveIndex(index);
}

void IndirectFitPlotPresenter::setActiveSpectrum(std::size_t spectrum) {
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
  const auto plotString = createPlotString(inputAndGuess->getName(), "[0,1]");
  m_pythonRunner.runPythonCode(QString::fromStdString(plotString));
}

void IndirectFitPlotPresenter::disablePlotGuessInSeparateWindow() {
  m_plotGuessInSeparateWindow = false;
  m_model->deleteExternalGuessWorkspace();
}

void IndirectFitPlotPresenter::appendLastDataToSelection() {
  const auto numberOfWorkspaces = m_model->numberOfWorkspaces();
  if (m_view->dataSelectionSize() == numberOfWorkspaces)
    m_view->setNameInDataSelection(m_model->getLastFitDataName(),
                                   numberOfWorkspaces - 1);
  else
    m_view->appendToDataSelection(m_model->getLastFitDataName());
}

void IndirectFitPlotPresenter::updateSelectedDataName() {
  m_view->setNameInDataSelection(m_model->getFitDataName(),
                                 m_model->getActiveDataIndex());
}

void IndirectFitPlotPresenter::updateDataSelection() {
  MantidQt::API::SignalBlocker<QObject> blocker(m_view);
  m_view->clearDataSelection();
  for (auto i = 0u; i < m_model->numberOfWorkspaces(); ++i)
    m_view->appendToDataSelection(m_model->getFitDataName(i));
  setActiveIndex(0);
  updateAvailableSpectra();
  emitSelectedFitDataChanged();
}

void IndirectFitPlotPresenter::updateAvailableSpectra() {
  if (m_model->getWorkspace()) {
    enableAllDataSelection();
    auto updateSpectra = UpdateAvailableSpectra(m_view);
    m_model->getSpectra().apply_visitor(updateSpectra);
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

void IndirectFitPlotPresenter::updatePlots() {
  const auto result = m_model->getResultWorkspace();
  if (result)
    plotResult(result);
  else
    plotInput();
  updateRangeSelectors();
  updateFitRangeSelector();
}

void IndirectFitPlotPresenter::plotInput() {
  const auto workspace = m_model->getWorkspace();
  if (workspace) {
    clearFit();
    clearDifference();
    plotInput(workspace, m_model->getActiveSpectrum());
    updatePlotRange(m_model->getWorkspaceRange());
  } else
    m_view->clear();
}

void IndirectFitPlotPresenter::plotResult(MatrixWorkspace_sptr result) {
  plotInput(result, 0);
  plotFit(result, 1);
  plotDifference(result, 2);
  updatePlotRange(m_model->getResultRange());
}

void IndirectFitPlotPresenter::updatePlotRange(
    const std::pair<double, double> &range) {
  MantidQt::API::SignalBlocker<QObject> blocker(m_view);
  m_view->setFitRange(range.first, range.second);
  m_view->setHWHMRange(range.first, range.second);
}

void IndirectFitPlotPresenter::plotInput(MatrixWorkspace_sptr workspace,
                                         std::size_t spectrum) {
  m_view->plotInTopPreview("Sample", workspace, spectrum, Qt::black);
}

void IndirectFitPlotPresenter::plotFit(MatrixWorkspace_sptr workspace,
                                       std::size_t spectrum) {
  m_view->plotInTopPreview("Fit", workspace, spectrum, Qt::red);
}

void IndirectFitPlotPresenter::plotDifference(MatrixWorkspace_sptr workspace,
                                              std::size_t spectrum) {
  m_view->plotInBottomPreview("Difference", workspace, spectrum, Qt::blue);
}

void IndirectFitPlotPresenter::clearInput() {
  m_view->removeFromTopPreview("Sample");
}

void IndirectFitPlotPresenter::clearFit() {
  m_view->removeFromTopPreview("Fit");
}

void IndirectFitPlotPresenter::clearDifference() {
  m_view->removeFromBottomPreview("Difference");
}

void IndirectFitPlotPresenter::updateFitRangeSelector() {
  const auto range = m_model->getRange();
  m_view->setFitRangeMinimum(range.first);
  m_view->setFitRangeMaximum(range.second);
}

void IndirectFitPlotPresenter::plotCurrentPreview() {
  if (m_model->getWorkspace()) {
    const auto plotString = getPlotString(m_model->getActiveSpectrum());
    m_pythonRunner.runPythonCode(QString::fromStdString(plotString));
  } else
    m_view->displayMessage("Workspace not found - data may not be loaded.");
}

void IndirectFitPlotPresenter::updateGuess() {
  if (m_model->canCalculateGuess()) {
    m_view->enablePlotGuess(true);
    updateGuess(m_view->isPlotGuessChecked());
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

void IndirectFitPlotPresenter::updateGuess(bool doPlotGuess) {
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
  m_view->plotInTopPreview("Guess", workspace, 0, Qt::green);
}

void IndirectFitPlotPresenter::plotGuessInSeparateWindow(
    Mantid::API::MatrixWorkspace_sptr workspace) {
  m_plotExternalGuessRunner.addCallback(
      [this, workspace]() { m_model->appendGuessToInput(workspace); });
}

void IndirectFitPlotPresenter::clearGuess() {
  m_view->removeFromTopPreview("Guess");
}

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

std::string
IndirectFitPlotPresenter::getPlotString(std::size_t spectrum) const {
  const auto result = m_model->getResultWorkspace();
  if (result)
    return createPlotString(result->getName(), "[0,1,2]");
  return createPlotString(m_model->getWorkspace()->getName(), spectrum);
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
  if (index >= 0)
    emit selectedFitDataChanged(static_cast<std::size_t>(index));
  else
    emit noFitDataSelected();
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
