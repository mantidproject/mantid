#include "MSDFit.h"
#include "../General/UserInputValidator.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/LegacyQwt/RangeSelector.h"

#include <QFileInfo>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("MSDFit");
}

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
MSDFit::MSDFit(QWidget *parent)
    : IndirectFitAnalysisTab(parent), m_uiForm(new Ui::MSDFit) {
  m_uiForm->setupUi(parent);
  IndirectFitAnalysisTab::addPropertyBrowserToUI(m_uiForm.get());
}

void MSDFit::setup() {
  auto fitRangeSelector = m_uiForm->ppPlotTop->addRangeSelector("MSDRange");

  auto gaussian = FunctionFactory::Instance().createFunction("MSDGauss");
  auto peters = FunctionFactory::Instance().createFunction("MSDPeters");
  auto yi = FunctionFactory::Instance().createFunction("MSDYi");
  addComboBoxFunctionGroup("Gaussian", {gaussian});
  addComboBoxFunctionGroup("Peters", {peters});
  addComboBoxFunctionGroup("Yi", {yi});

  modelSelection(selectedFitType());
  disablePlotGuess();

  connect(fitRangeSelector, SIGNAL(minValueChanged(double)), this,
          SLOT(xMinSelected(double)));
  connect(fitRangeSelector, SIGNAL(maxValueChanged(double)), this,
          SLOT(xMaxSelected(double)));

  connect(m_uiForm->dsSampleInput, SIGNAL(dataReady(const QString &)), this,
          SLOT(newDataLoaded(const QString &)));
  connect(m_uiForm->pbSingleFit, SIGNAL(clicked()), this, SLOT(singleFit()));

  // Update plot when fit type changes
  connect(m_uiForm->spPlotSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(setSelectedSpectrum(int)));
  connect(m_uiForm->spPlotSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(updatePreviewPlots()));

  connect(m_uiForm->spSpectraMin, SIGNAL(valueChanged(int)), this,
          SLOT(specMinChanged(int)));
  connect(m_uiForm->spSpectraMin, SIGNAL(valueChanged(int)), this,
          SLOT(setMinimumSpectrum(int)));
  connect(m_uiForm->spSpectraMax, SIGNAL(valueChanged(int)), this,
          SLOT(specMaxChanged(int)));
  connect(m_uiForm->spSpectraMax, SIGNAL(valueChanged(int)), this,
          SLOT(setMaximumSpectrum(int)));

  connect(m_uiForm->pbPlot, SIGNAL(clicked()), this, SLOT(plotClicked()));
  connect(m_uiForm->pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));
  connect(m_uiForm->pbPlotPreview, SIGNAL(clicked()), this,
          SLOT(plotCurrentPreview()));

  connect(m_uiForm->ckPlotGuess, SIGNAL(stateChanged(int)), this,
          SLOT(plotGuess()));
}

void MSDFit::run() {
  if (!validate())
    return;

  // Set the result workspace for Python script export
  const auto model = selectedFitType();
  QString dataName = m_uiForm->dsSampleInput->getCurrentDataName();

  const int specMin = m_uiForm->spSpectraMin->value();
  const int specMax = m_uiForm->spSpectraMax->value();

  m_pythonExportWsName =
      dataName.left(dataName.lastIndexOf("_")).toStdString() + "_s" +
      std::to_string(specMin) + "_to_s" + std::to_string(specMax) + "_" +
      model.toStdString() + "_msd";

  auto msdAlg =
      msdFitAlgorithm(modelToAlgorithmProperty(model), specMin, specMax);
  runFitAlgorithm(msdAlg);
}

void MSDFit::singleFit() {
  if (!validate())
    return;

  // Set the result workspace for Python script export
  const auto model = selectedFitType();
  QString dataName = m_uiForm->dsSampleInput->getCurrentDataName();
  int fitSpec = m_uiForm->spPlotSpectrum->value();

  m_pythonExportWsName =
      dataName.left(dataName.lastIndexOf("_")).toStdString() + "_s" +
      std::to_string(fitSpec) + "_" + model.toStdString() + "_msd";

  auto msdAlg =
      msdFitAlgorithm(modelToAlgorithmProperty(model), fitSpec, fitSpec);
  runFitAlgorithm(msdAlg);
}

/*
 * Creates an initialized MSDFit Algorithm, using the model with the
 * specified name, to be run from the specified minimum spectrum to
 * the specified maximum spectrum.
 *
 * @param model   The name of the model to be used by the algorithm.
 * @param specMin The minimum spectrum to fit.
 * @param specMax The maximum spectrum to fit.
 * @return        An MSDFit Algorithm using the specified model, which
 *                will run across all spectrum between the specified
 *                minimum and maximum.
 */
IAlgorithm_sptr MSDFit::msdFitAlgorithm(const std::string &model, int specMin,
                                        int specMax) {
  auto wsName = m_uiForm->dsSampleInput->getCurrentDataName().toStdString();
  double xStart = m_dblManager->value(m_properties["StartX"]);
  double xEnd = m_dblManager->value(m_properties["EndX"]);

  setMinimumSpectrum(specMin);
  setMaximumSpectrum(specMax);

  IAlgorithm_sptr msdAlg = AlgorithmManager::Instance().create("MSDFit");
  msdAlg->initialize();
  msdAlg->setProperty("InputWorkspace", wsName);
  msdAlg->setProperty("Model", model);
  msdAlg->setProperty("XStart", xStart);
  msdAlg->setProperty("XEnd", xEnd);
  msdAlg->setProperty("SpecMin", boost::numeric_cast<long>(specMin));
  msdAlg->setProperty("SpecMax", boost::numeric_cast<long>(specMax));
  msdAlg->setProperty("OutputWorkspace", m_pythonExportWsName);

  return msdAlg;
}

bool MSDFit::validate() {
  UserInputValidator uiv;

  uiv.checkDataSelectorIsValid("Sample input", m_uiForm->dsSampleInput);

  auto range = std::make_pair(startX(), endX());
  uiv.checkValidRange("a range", range);

  int specMin = m_uiForm->spSpectraMin->value();
  int specMax = m_uiForm->spSpectraMax->value();
  auto specRange = std::make_pair(specMin, specMax + 1);
  uiv.checkValidRange("spectrum range", specRange);

  QString errors = uiv.generateErrorMessage();
  showMessageBox(errors);

  return errors.isEmpty();
}

void MSDFit::loadSettings(const QSettings &settings) {
  m_uiForm->dsSampleInput->readSettings(settings.group());
}

/**
 * Handles the completion of the MSDFit algorithm
 *
 * @param error If the algorithm chain failed
 */
void MSDFit::algorithmComplete(bool error) {
  if (error)
    return;

  IndirectFitAnalysisTab::fitAlgorithmComplete(m_pythonExportWsName +
                                               "_Parameters");
  // Enable plot and save
  m_uiForm->pbPlot->setEnabled(true);
  m_uiForm->pbSave->setEnabled(true);
}

void MSDFit::updatePreviewPlots() {
  const auto groupName = m_pythonExportWsName + "_Workspaces";
  IndirectFitAnalysisTab::updatePlot(groupName, m_uiForm->ppPlotTop,
                                     m_uiForm->ppPlotBottom);
  IndirectDataAnalysisTab::updatePlotRange("MSDRange", m_uiForm->ppPlotTop);
}

void MSDFit::rangeChanged(double, double) {
  IndirectDataAnalysisTab::updatePlotRange("MSDRange", m_uiForm->ppPlotTop);
}

void MSDFit::disablePlotGuess() {
  m_uiForm->ckPlotGuess->setEnabled(false);
  m_uiForm->ckPlotGuess->blockSignals(true);
}

void MSDFit::enablePlotGuess() {
  m_uiForm->ckPlotGuess->setEnabled(true);
  m_uiForm->ckPlotGuess->blockSignals(false);
}

void MSDFit::plotGuess() {

  if (m_uiForm->ckPlotGuess->isEnabled() &&
      m_uiForm->ckPlotGuess->isChecked()) {
    IndirectFitAnalysisTab::plotGuess(m_uiForm->ppPlotTop);
  } else {
    m_uiForm->ppPlotTop->removeSpectrum("Guess");
  }
}

void MSDFit::updatePlotOptions() {}

/**
 * Called when new data has been loaded by the data selector.
 *
 * Configures ranges for spin boxes before raw plot is done.
 *
 * @param wsName Name of new workspace loaded
 */
void MSDFit::newDataLoaded(const QString wsName) {
  m_pythonExportWsName = "";
  IndirectFitAnalysisTab::newInputDataLoaded(wsName);
  auto const &workspace = inputWorkspace();
  int maxWsIndex = 0;

  if (workspace) {
    maxWsIndex = static_cast<int>(workspace->getNumberHistograms()) - 1;
  }

  m_uiForm->spPlotSpectrum->setMaximum(maxWsIndex);
  m_uiForm->spPlotSpectrum->setMinimum(0);
  m_uiForm->spPlotSpectrum->setValue(0);

  m_uiForm->spSpectraMin->setMaximum(maxWsIndex);
  m_uiForm->spSpectraMin->setMinimum(0);

  m_uiForm->spSpectraMax->setMaximum(maxWsIndex);
  m_uiForm->spSpectraMax->setMinimum(0);
  m_uiForm->spSpectraMax->setValue(maxWsIndex);
}

/**
 * Handles the user entering a new minimum spectrum index.
 *
 * Prevents the user entering an overlapping spectra range.
 *
 * @param value Minimum spectrum index
 */
void MSDFit::specMinChanged(int value) {
  m_uiForm->spSpectraMax->setMinimum(value);
}

/**
 * Handles the user entering a new maximum spectrum index.
 *
 * Prevents the user entering an overlapping spectra range.
 *
 * @param value Maximum spectrum index
 */
void MSDFit::specMaxChanged(int value) {
  m_uiForm->spSpectraMin->setMaximum(value);
}

void MSDFit::startXChanged(double startX) {
  auto rangeSelector = m_uiForm->ppPlotTop->getRangeSelector("MSDRange");
  rangeSelector->blockSignals(true);
  rangeSelector->setMinimum(startX);
  rangeSelector->blockSignals(false);
}

void MSDFit::endXChanged(double endX) {
  auto rangeSelector = m_uiForm->ppPlotTop->getRangeSelector("MSDRange");
  rangeSelector->blockSignals(true);
  rangeSelector->setMaximum(endX);
  rangeSelector->blockSignals(false);
}

void MSDFit::modelSelection(const QString &model) {

  if (!m_pythonExportWsName.empty()) {
    auto idx = m_pythonExportWsName.rfind("_");
    m_pythonExportWsName = m_pythonExportWsName.substr(0, idx);
    idx = m_pythonExportWsName.rfind("_");
    m_pythonExportWsName = m_pythonExportWsName.substr(0, idx);
    m_pythonExportWsName += "_" + model.toStdString() + "_msd";
  }

  m_uiForm->ckPlotGuess->setChecked(false);
}

/*
 * Given the selected model in the interface, returns the name of
 * the associated model to pass to the MSDFit algorithm.
 *
 * @param model The name of the model as displayed in the interface.
 * @return      The name of the model as defined in the MSDFit algorithm.
 */
std::string MSDFit::modelToAlgorithmProperty(const QString &model) {

  if (model == "Gaussian")
    return "Gauss";
  else if (model == "Peters")
    return "Peters";
  else if (model == "Yi")
    return "Yi";
  else
    return model.toStdString();
}

/**
 * Handles saving of workspace
 */
void MSDFit::saveClicked() {
  IndirectFitAnalysisTab::saveResult(m_pythonExportWsName);
}

/**
 * Handles mantid plotting
 */
void MSDFit::plotClicked() {
  IndirectFitAnalysisTab::plotResult(m_pythonExportWsName + "_Workspaces",
                                     "All");
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
