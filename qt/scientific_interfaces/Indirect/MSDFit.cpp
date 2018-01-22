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
MSDFit::MSDFit(QWidget *parent) : IndirectFitAnalysisTab(parent) {
  m_uiForm.setupUi(parent);
  m_msdTree = m_propertyTree;
}

void MSDFit::setup() {
  // Tree Browser
  m_uiForm.properties->addWidget(m_msdTree);

  m_msdTree->setFactoryForManager(m_dblManager, m_dblEdFac);

  m_properties["FitRange"] = m_grpManager->addProperty("Fitting Range");
  m_properties["StartX"] = m_dblManager->addProperty("StartX");
  m_dblManager->setDecimals(m_properties["StartX"], NUM_DECIMALS);
  m_properties["EndX"] = m_dblManager->addProperty("EndX");
  m_dblManager->setDecimals(m_properties["EndX"], NUM_DECIMALS);
  m_properties["FitRange"]->addSubProperty(m_properties["StartX"]);
  m_properties["FitRange"]->addSubProperty(m_properties["EndX"]);
  m_msdTree->addProperty(m_properties["FitRange"]);

  m_properties["Gaussian"] = createFunctionProperty("Gaussian");
  m_properties["Peters"] = createFunctionProperty("Peters");
  m_properties["Yi"] = createFunctionProperty("Yi");

  auto fitRangeSelector = m_uiForm.ppPlotTop->addRangeSelector("MSDRange");
  m_dblManager->setValue(m_properties["StartX"],
                         fitRangeSelector->getMinimum());
  m_dblManager->setValue(m_properties["EndX"], fitRangeSelector->getMaximum());

  modelSelection(m_uiForm.cbModelInput->currentIndex());

  connect(fitRangeSelector, SIGNAL(minValueChanged(double)), this,
          SLOT(minChanged(double)));
  connect(fitRangeSelector, SIGNAL(maxValueChanged(double)), this,
          SLOT(maxChanged(double)));
  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(updateRS(QtProperty *, double)));

  connect(m_uiForm.dsSampleInput, SIGNAL(dataReady(const QString &)), this,
          SLOT(newDataLoaded(const QString &)));
  connect(m_uiForm.cbModelInput, SIGNAL(currentIndexChanged(int)), this,
          SLOT(modelSelection(int)));
  connect(m_uiForm.pbSingleFit, SIGNAL(clicked()), this, SLOT(singleFit()));

  connect(m_uiForm.spPlotSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(setSelectedSpectrum(int)));
  connect(m_uiForm.spPlotSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(updateProperties(int)));

  connect(m_uiForm.spPlotSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(updatePreviewPlots()));
  connect(m_uiForm.cbModelInput, SIGNAL(currentIndexChanged(int)), this,
          SLOT(updatePreviewPlots()));

  connect(m_dblManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(plotGuess()));
  connect(m_uiForm.ckPlotGuess, SIGNAL(stateChanged(int)), this,
          SLOT(plotGuess()));
  connect(m_uiForm.cbModelInput, SIGNAL(currentIndexChanged(int)), this,
          SLOT(plotGuess()));

  connect(m_uiForm.spSpectraMin, SIGNAL(valueChanged(int)), this,
          SLOT(specMinChanged(int)));
  connect(m_uiForm.spSpectraMax, SIGNAL(valueChanged(int)), this,
          SLOT(specMaxChanged(int)));

  connect(m_uiForm.pbPlot, SIGNAL(clicked()), this, SLOT(plotClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));
  connect(m_uiForm.pbPlotPreview, SIGNAL(clicked()), this,
          SLOT(plotCurrentPreview()));
}

void MSDFit::run() {
  if (!validate())
    return;

  // Set the result workspace for Python script export
  const auto model = m_uiForm.cbModelInput->currentText();
  setFitFunctions({model});
  QString dataName = m_uiForm.dsSampleInput->getCurrentDataName();

  const int specMin = m_uiForm.spSpectraMin->value();
  const int specMax = m_uiForm.spSpectraMax->value();

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
  const auto model = m_uiForm.cbModelInput->currentText();
  setFitFunctions({model});
  QString dataName = m_uiForm.dsSampleInput->getCurrentDataName();
  int fitSpec = m_uiForm.spPlotSpectrum->value();

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
  auto wsName = m_uiForm.dsSampleInput->getCurrentDataName().toStdString();
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

  uiv.checkDataSelectorIsValid("Sample input", m_uiForm.dsSampleInput);

  auto range = std::make_pair(m_dblManager->value(m_properties["StartX"]),
                              m_dblManager->value(m_properties["EndX"]));
  uiv.checkValidRange("a range", range);

  int specMin = m_uiForm.spSpectraMin->value();
  int specMax = m_uiForm.spSpectraMax->value();
  auto specRange = std::make_pair(specMin, specMax + 1);
  uiv.checkValidRange("spectrum range", specRange);

  QString errors = uiv.generateErrorMessage();
  showMessageBox(errors);

  return errors.isEmpty();
}

void MSDFit::loadSettings(const QSettings &settings) {
  m_uiForm.dsSampleInput->readSettings(settings.group());
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
  m_uiForm.pbPlot->setEnabled(true);
  m_uiForm.pbSave->setEnabled(true);
}

void MSDFit::updatePreviewPlots() {
  const auto groupName = m_pythonExportWsName + "_Workspaces";
  IndirectFitAnalysisTab::updatePlot(groupName, m_uiForm.ppPlotTop,
                                     m_uiForm.ppPlotBottom);
  IndirectDataAnalysisTab::updatePlotRange("MSDRange", m_uiForm.ppPlotTop);
}

void MSDFit::disablePlotGuess() {
  m_uiForm.ckPlotGuess->setEnabled(false);
  disconnect(m_dblManager, SIGNAL(propertyChanged(QtProperty *)), this,
             SLOT(plotGuess()));
  disconnect(m_uiForm.ckPlotGuess, SIGNAL(stateChanged(int)), this,
             SLOT(plotGuess()));
  disconnect(m_uiForm.cbModelInput, SIGNAL(currentIndexChanged(int)), this,
             SLOT(plotGuess()));
}

void MSDFit::enablePlotGuess() {
  m_uiForm.ckPlotGuess->setEnabled(true);
  connect(m_dblManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(plotGuess()));
  connect(m_uiForm.ckPlotGuess, SIGNAL(stateChanged(int)), this,
          SLOT(plotGuess()));
  connect(m_uiForm.cbModelInput, SIGNAL(currentIndexChanged(int)), this,
          SLOT(plotGuess()));
  plotGuess();
}

void MSDFit::plotGuess() {

  if (m_uiForm.ckPlotGuess->isChecked()) {
    QString modelName = m_uiForm.cbModelInput->currentText();
    IndirectDataAnalysisTab::plotGuess(m_uiForm.ppPlotTop,
                                       createFunction(modelName));
  } else {
    m_uiForm.ppPlotTop->removeSpectrum("Guess");
  }
}

IFunction_sptr MSDFit::createFunction(const QString &modelName) {
  return createPopulatedFunction(
      m_properties[modelName]->propertyName().toStdString(),
      m_properties[modelName]);
}

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

  m_uiForm.spPlotSpectrum->setMaximum(maxWsIndex);
  m_uiForm.spPlotSpectrum->setMinimum(0);
  m_uiForm.spPlotSpectrum->setValue(0);

  m_uiForm.spSpectraMin->setMaximum(maxWsIndex);
  m_uiForm.spSpectraMin->setMinimum(0);

  m_uiForm.spSpectraMax->setMaximum(maxWsIndex);
  m_uiForm.spSpectraMax->setMinimum(0);
  m_uiForm.spSpectraMax->setValue(maxWsIndex);
}

/**
 * Handles the user entering a new minimum spectrum index.
 *
 * Prevents the user entering an overlapping spectra range.
 *
 * @param value Minimum spectrum index
 */
void MSDFit::specMinChanged(int value) {
  m_uiForm.spSpectraMax->setMinimum(value);
}

/**
 * Handles the user entering a new maximum spectrum index.
 *
 * Prevents the user entering an overlapping spectra range.
 *
 * @param value Maximum spectrum index
 */
void MSDFit::specMaxChanged(int value) {
  m_uiForm.spSpectraMin->setMaximum(value);
}

void MSDFit::minChanged(double val) {
  m_dblManager->setValue(m_properties["StartX"], val);
}

void MSDFit::maxChanged(double val) {
  m_dblManager->setValue(m_properties["EndX"], val);
}

void MSDFit::updateRS(QtProperty *prop, double val) {
  auto fitRangeSelector = m_uiForm.ppPlotTop->getRangeSelector("MSDRange");

  if (prop == m_properties["StartX"])
    fitRangeSelector->setMinimum(val);
  else if (prop == m_properties["EndX"])
    fitRangeSelector->setMaximum(val);
}

void MSDFit::modelSelection(int selected) {
  const auto model = m_uiForm.cbModelInput->itemText(selected);

  if (!m_pythonExportWsName.empty()) {
    auto idx = m_pythonExportWsName.rfind("_");
    m_pythonExportWsName = m_pythonExportWsName.substr(0, idx);
    idx = m_pythonExportWsName.rfind("_");
    m_pythonExportWsName = m_pythonExportWsName.substr(0, idx);
    m_pythonExportWsName += "_" + model.toStdString() + "_msd";
  }

  m_uiForm.ckPlotGuess->setChecked(false);
  setPropertyFunctions({model});
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

IFunction_sptr MSDFit::getFunction(const QString &functionName) const {
  if (functionName.startsWith("Msd"))
    return IndirectFitAnalysisTab::getFunction(functionName);
  if (functionName == "Gaussian")
    return IndirectFitAnalysisTab::getFunction("MsdGauss");
  else
    return IndirectFitAnalysisTab::getFunction("Msd" + functionName);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
