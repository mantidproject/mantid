#include "IqtFit.h"

#include "../General/UserInputValidator.h"
#include "MantidQtWidgets/LegacyQwt/RangeSelector.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <QMenu>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("IqtFit");
}

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IqtFit::IqtFit(QWidget *parent)
    : IndirectFitAnalysisTab(parent), m_uiForm(new Ui::IqtFit) {
  m_uiForm->setupUi(parent);
  IndirectFitAnalysisTab::addPropertyBrowserToUI(m_uiForm.get());
}

void IqtFit::setup() {
  setMinimumSpectrum(0);
  setMaximumSpectrum(0);

  m_uiForm->ckPlotGuess->setChecked(false);
  enablePlotGuess();

  // Create custom function groups
  const auto exponential =
      FunctionFactory::Instance().createFunction("ExpDecay");
  const auto stretchedExponential =
      FunctionFactory::Instance().createFunction("StretchExp");
  addSpinnerFunctionGroup("Exponential", {exponential}, 0, 2);
  addCheckBoxFunctionGroup("Stretched Exponential", {stretchedExponential});

  // Add custom settings
  addBoolCustomSetting("ConstrainIntensities", "Constrain Intensities");
  addBoolCustomSetting("ConstrainBeta", "Constrain Beta Over All Q");

  // Set available background options
  setBackgroundOptions({"None", "FlatBackground"});

  auto fitRangeSelector = m_uiForm->ppPlotTop->addRangeSelector("IqtFitRange");
  connect(fitRangeSelector, SIGNAL(minValueChanged(double)), this,
          SLOT(xMinSelected(double)));
  connect(fitRangeSelector, SIGNAL(maxValueChanged(double)), this,
          SLOT(xMaxSelected(double)));

  // Signal/slot ui connections
  connect(m_uiForm->dsSampleInput, SIGNAL(dataReady(const QString &)), this,
          SLOT(newDataLoaded(const QString &)));
  connect(m_uiForm->pbSingle, SIGNAL(clicked()), this, SLOT(singleFit()));

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

  connect(m_uiForm->cbPlotType, SIGNAL(currentIndexChanged(QString)), this,
          SLOT(updateCurrentPlotOption(QString)));
  connect(m_uiForm->pbPlot, SIGNAL(clicked()), this, SLOT(plotWorkspace()));
  connect(m_uiForm->pbSave, SIGNAL(clicked()), this, SLOT(saveResult()));
  connect(m_uiForm->pbPlotPreview, SIGNAL(clicked()), this,
          SLOT(plotCurrentPreview()));
  connect(m_uiForm->ckPlotGuess, SIGNAL(stateChanged(int)), this,
          SLOT(plotGuess()));
}

void IqtFit::run() {
  if (!validate())
    return;

  executeSequentialFit();
}

/**
 * Constructs the desired output base name for the  IqtFitMultiple
 * @param inputName     :: Name of the inputworkspace
 * @param fitType       :: The type of fit that is being performed
 * @param multi         :: If the fit is running the IqtFitMultiple
 * @param specMin       :: Minimum number of spectra being fitted
 * @param specMax       :: Maximum number of spectra being fitted
 * @return the base name
 */
std::string IqtFit::constructBaseName(const std::string &inputName,
                                      const std::string &fitType,
                                      const bool &multi, const size_t &specMin,
                                      const size_t &specMax) const {
  QString functionType = QString::fromStdString(fitType);
  if (multi) {
    functionType += "mult_s";
  }

  QString baseName = QString::fromStdString(inputName);
  baseName = baseName.left(baseName.lastIndexOf("_"));
  baseName += "_IqtFit_";
  baseName += functionType;
  baseName += QString::number(specMin);

  // Check whether a single spectrum is being fit
  if (specMin != specMax) {
    baseName += "_to_";
    baseName += QString::number(specMax);
  }
  const auto baseName_str = baseName.toStdString();
  return baseName_str;
}

QString IqtFit::fitTypeString() const {
  const auto numberOfExponential = numberOfCustomFunctions("ExpDecay");
  const auto numberOfStretched = numberOfCustomFunctions("StretchExp");
  QString fitType;

  if (numberOfExponential > 0)
    fitType = QString::number(numberOfExponential) + "E";

  if (numberOfStretched > 0)
    fitType += QString::number(numberOfStretched) + "S";

  if (fitType.isEmpty())
    return "s";
  else
    return fitType + "_s";
}

MatrixWorkspace_sptr IqtFit::fitWorkspace() const {
  auto replaceAlg = replaceInfinityAndNaN(inputWorkspace());
  replaceAlg->execute();
  return replaceAlg->getProperty("OutputWorkspace");
}

IAlgorithm_sptr IqtFit::singleFitAlgorithm() {
  size_t specNo = static_cast<size_t>(m_uiForm->spPlotSpectrum->value());
  return iqtFitAlgorithm(inputWorkspace(), specNo, specNo);
}

IAlgorithm_sptr IqtFit::sequentialFitAlgorithm() {
  size_t specMin = static_cast<size_t>(m_uiForm->spSpectraMin->value());
  size_t specMax = static_cast<size_t>(m_uiForm->spSpectraMax->value());
  return iqtFitAlgorithm(inputWorkspace(), specMin, specMax);
}

IAlgorithm_sptr IqtFit::iqtFitAlgorithm(MatrixWorkspace_sptr inputWs,
                                        const size_t &specMin,
                                        const size_t &specMax) {
  const bool constrainBeta = boolSettingValue("ConstrainBeta");
  const bool constrainIntens = boolSettingValue("ConstrainIntensities");

  const auto fitType = fitTypeString().toStdString();
  m_plotOption = m_uiForm->cbPlotType->currentText().toStdString();

  m_baseName =
      constructBaseName(inputWs->getName(), fitType, true, specMin, specMax);

  IAlgorithm_sptr iqtFitAlg;

  if (!constrainBeta) {
    iqtFitAlg = AlgorithmManager::Instance().create("IqtFitSequential");
  } else {
    iqtFitAlg = AlgorithmManager::Instance().create("IqtFitMultiple");
  }

  iqtFitAlg->initialize();
  iqtFitAlg->setProperty("FitType", fitType);
  iqtFitAlg->setProperty("SpecMin", boost::numeric_cast<long>(specMin));
  iqtFitAlg->setProperty("SpecMax", boost::numeric_cast<long>(specMax));
  iqtFitAlg->setProperty("ConstrainIntensities", constrainIntens);
  iqtFitAlg->setProperty("OutputResultWorkspace", m_baseName + "_Result");
  iqtFitAlg->setProperty("OutputParameterWorkspace",
                         m_baseName + "_Parameters");
  iqtFitAlg->setProperty("OutputWorkspaceGroup", m_baseName + "_Workspaces");
  return iqtFitAlg;
}

void IqtFit::setMaxIterations(Mantid::API::IAlgorithm_sptr fitAlgorithm,
                              int maxIterations) const {
  fitAlgorithm->setProperty("MaxIterations", static_cast<long>(maxIterations));
}

void IqtFit::updatePlotOptions() {
  IndirectFitAnalysisTab::updatePlotOptions(m_uiForm->cbPlotType);
}

/**
 * Plot workspace based on user input
 */
void IqtFit::plotWorkspace() {
  IndirectFitAnalysisTab::plotResult(m_baseName + "_Result",
                                     QString::fromStdString(m_plotOption));
}

/**
 * Save the result of the algorithm
 */
void IqtFit::saveResult() {
  IndirectFitAnalysisTab::saveResult(m_baseName + "_Result");
}

/**
 * Handles completion of the IqtFitMultiple algorithm.
 * @param error True if the algorithm was stopped due to error, false otherwise
 * @param outputWsName The name of the output workspace
 */
void IqtFit::algorithmComplete(bool error) {

  if (error) {
    QString msg =
        "There was an error executing the fitting algorithm. Please see the "
        "Results Log pane for more details.";
    showMessageBox(msg);
    return;
  }

  IndirectFitAnalysisTab::fitAlgorithmComplete(m_baseName + "_Parameters");
  m_uiForm->pbPlot->setEnabled(true);
  m_uiForm->pbSave->setEnabled(true);
  m_uiForm->cbPlotType->setEnabled(true);
}

bool IqtFit::validate() {
  UserInputValidator uiv;

  uiv.checkDataSelectorIsValid("Sample", m_uiForm->dsSampleInput);

  QString error = uiv.generateErrorMessage();
  showMessageBox(error);

  if (!inputWorkspace()) {
    QString msg = "Input workspace was deleted from the Analysis Data Service "
                  "before Algorithm could run.";
    showMessageBox(msg);
  }

  return error.isEmpty();
}

void IqtFit::loadSettings(const QSettings &settings) {
  m_uiForm->dsSampleInput->readSettings(settings.group());
}

/**
 * Called when new data has been loaded by the data selector.
 *
 * Configures ranges for spin boxes before raw plot is done.
 *
 * @param wsName Name of new workspace loaded
 */
void IqtFit::newDataLoaded(const QString wsName) {
  IndirectFitAnalysisTab::newInputDataLoaded(wsName);
  m_baseName.clear();

  int maxWsIndex =
      static_cast<int>(inputWorkspace()->getNumberHistograms()) - 1;

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
 * Update the current plot option selected
 */
void IqtFit::updateCurrentPlotOption(QString newOption) {
  m_plotOption = newOption.toStdString();
}

void IqtFit::updatePreviewPlots() {
  // If there is a result workspace plot then plot it
  const auto groupName = m_baseName + "_Workspaces";
  IndirectFitAnalysisTab::updatePlot(groupName, m_uiForm->ppPlotTop,
                                     m_uiForm->ppPlotBottom);

  IndirectDataAnalysisTab::updatePlotRange("IqtFitRange", m_uiForm->ppPlotTop);
  plotGuess();
}

void IqtFit::rangeChanged(double, double) {
  IndirectDataAnalysisTab::updatePlotRange("IqtFitRange", m_uiForm->ppPlotTop);
}

QHash<QString, double> IqtFit::createDefaultValues() const {
  QHash<QString, double> defaultValues;
  // intensity is always 1-background
  double height = 1.0;
  if (background() != nullptr)
    height -= background()->getParameter("A0");
  defaultValues["Height"] = height;

  auto inputWs = inputWorkspace();
  double tau = 0;

  if (inputWs) {
    auto x = inputWs->x(0);
    auto y = inputWs->y(0);

    if (x.size() > 4) {
      tau = -x[4] / log(y[4]);
    }
  }

  defaultValues["Lifetime"] = tau;
  defaultValues["Stretching"] = 1.0;
  defaultValues["A0"] = 0.0;
  return defaultValues;
}

/**
 * Handles the user entering a new minimum spectrum index.
 *
 * Prevents the user entering an overlapping spectra range.
 *
 * @param value Minimum spectrum index
 */
void IqtFit::specMinChanged(int value) {
  m_uiForm->spSpectraMax->setMinimum(value);
}

/**
 * Handles the user entering a new maximum spectrum index.
 *
 * Prevents the user entering an overlapping spectra range.
 *
 * @param value Maximum spectrum index
 */
void IqtFit::specMaxChanged(int value) {
  m_uiForm->spSpectraMin->setMaximum(value);
}

void IqtFit::startXChanged(double startX) {
  auto rangeSelector = m_uiForm->ppPlotTop->getRangeSelector("IqtFitRange");
  rangeSelector->blockSignals(true);
  rangeSelector->setMinimum(startX);
  rangeSelector->blockSignals(false);
}

void IqtFit::endXChanged(double endX) {
  auto rangeSelector = m_uiForm->ppPlotTop->getRangeSelector("IqtFitRange");
  rangeSelector->blockSignals(true);
  rangeSelector->setMaximum(endX);
  rangeSelector->blockSignals(false);
}

void IqtFit::singleFit() {
  if (!validate())
    return;

  size_t specNo = m_uiForm->spPlotSpectrum->text().toULongLong();
  runFitAlgorithm(iqtFitAlgorithm(inputWorkspace(), specNo, specNo));
}

void IqtFit::disablePlotGuess() {
  m_uiForm->ckPlotGuess->setEnabled(false);
  m_uiForm->ckPlotGuess->blockSignals(true);
}

void IqtFit::enablePlotGuess() {
  m_uiForm->ckPlotGuess->setEnabled(true);
  m_uiForm->ckPlotGuess->blockSignals(false);
  plotGuess();
}

void IqtFit::plotGuess() {

  if (m_uiForm->ckPlotGuess->isChecked())
    IndirectFitAnalysisTab::plotGuess(m_uiForm->ppPlotTop);
  else {
    m_uiForm->ppPlotTop->removeSpectrum("Guess");
    m_uiForm->ckPlotGuess->setChecked(false);
  }
}

IAlgorithm_sptr
IqtFit::replaceInfinityAndNaN(Mantid::API::MatrixWorkspace_sptr inputWS) const {
  auto replaceAlg = AlgorithmManager::Instance().create("ReplaceSpecialValues");
  replaceAlg->setChild(true);
  replaceAlg->initialize();
  replaceAlg->setProperty("InputWorkspace", inputWS);
  replaceAlg->setProperty("NaNValue", 0.0);
  replaceAlg->setProperty("InfinityError", 0.0);
  replaceAlg->setProperty("OutputWorkspace", inputWS->getName() + "_nospecial");
  return replaceAlg;
}
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
