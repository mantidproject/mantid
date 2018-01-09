#include "IndirectFitAnalysisTab.h"
#include "ui_ConvFit.h"
#include "ui_IqtFit.h"
#include "ui_JumpFit.h"
#include "ui_MSDFit.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/WorkspaceFactory.h"

#include <QString>

#include <algorithm>

using namespace Mantid::API;

namespace {

/**
 * Combines the two maps of parameter values, by adding the values from
 * the second into the first, where the parameters (keys) are taken from
 * the first map, and the value doesn't already exist in the first map.
 *
 * @param parameterValues1  The first parameter values map to combine with.
 * @param parameterValues2  The second parameter values map to combine with.
 * @return                  The combined map.
 */
template <typename Map>
Map combineParameterValues(const Map &parameterValues1,
                           const Map &parameterValues2) {
  auto combinedValues = parameterValues1;

  for (const auto &index : parameterValues1.keys()) {

    if (parameterValues2.contains(index)) {
      const auto &values1 = parameterValues1[index];
      const auto &values2 = parameterValues2[index];

      for (const auto &parameterName : values2.keys()) {
        if (!values1.contains(parameterName))
          combinedValues[index][parameterName] = values2[parameterName];
      }
    }
  }

  return combinedValues;
}

/*
 * Sets the value of each parameter, in a clone of the specified function, to 0.
 *
 * @param function  The function to create a clone of.
 * @return          A clone of the specified function, whose parameter values
 *                  are all set to 0.
 */
IFunction_sptr zeroFunction(IFunction_const_sptr function) {
  auto functionClone = function->clone();
  for (const auto &parameter : functionClone->getParameterNames())
    functionClone->setParameter(parameter, 0.0);
  return functionClone;
}

/*
 * Checks whether the specified functions have the same composition.
 *
 * @param func1 Function to compare.
 * @param func2 Function to compare.
 * @return      True if the specified functions have the same composition,
 *              False otherwise.
 */
bool equivalentFunctions(IFunction_const_sptr func1,
                         IFunction_const_sptr func2) {
  if (!func1)
    return false;

  if (!func2)
    return false;

  return zeroFunction(func1)->asString() == zeroFunction(func2)->asString();
}

class WidgetAdder : public boost::static_visitor<> {
public:
  WidgetAdder(QWidget *widget) : m_widget(widget) {}

  template <typename Form> void operator()(Form form) const {
    form->properties->addWidget(m_widget);
  }

private:
  QWidget *m_widget;
};
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

/**
 * Constructor.
 *
 * @param parent :: the parent widget (an IndirectDataAnalysis object).
 */
IndirectFitAnalysisTab::IndirectFitAnalysisTab(QWidget *parent)
    : IndirectDataAnalysisTab(parent) {
  m_fitPropertyBrowser = new MantidWidgets::IndirectFitPropertyBrowser(parent);
  m_fitPropertyBrowser->init();

  connect(m_fitPropertyBrowser, SIGNAL(fitScheduled()), this,
          SLOT(executeSingleFit()));
  connect(m_fitPropertyBrowser, SIGNAL(sequentialFitScheduled()), this,
          SLOT(executeSequentialFit()));

  connect(m_fitPropertyBrowser, SIGNAL(parameterChanged(const IFunction *)),
          this, SLOT(plotGuess()));
  connect(m_fitPropertyBrowser, SIGNAL(xRangeChanged(double, double)), this,
          SLOT(rangeChanged(double, double)));

  connect(m_fitPropertyBrowser, SIGNAL(functionChanged()), this,
          SLOT(fitFunctionChanged()));
}

void IndirectFitAnalysisTab::addPropertyBrowserToUI(UIForm form) {
  boost::apply_visitor(WidgetAdder(m_fitPropertyBrowser), form);
}

IFunction_sptr IndirectFitAnalysisTab::background() const {
  return m_fitPropertyBrowser->background();
}

IFunction_sptr IndirectFitAnalysisTab::model() const {
  auto model = boost::dynamic_pointer_cast<CompositeFunction>(
      m_fitPropertyBrowser->getFittingFunction()->clone());
  auto index = m_fitPropertyBrowser->backgroundIndex();

  if (index != -1)
    model->removeFunction(static_cast<size_t>(index));
  return model;
}

size_t IndirectFitAnalysisTab::numberOfCustomFunctions(
    const std::string &functionName) const {
  return m_fitPropertyBrowser->numberOfCustomFunctions(functionName);
}

double IndirectFitAnalysisTab::startX() const {
  return m_fitPropertyBrowser->startX();
}

double IndirectFitAnalysisTab::endX() const {
  return m_fitPropertyBrowser->endX();
}

double
IndirectFitAnalysisTab::parameterValue(const std::string &functionName,
                                       const std::string &parameterName) {
  return m_fitPropertyBrowser->parameterValue(functionName, parameterName);
}

bool IndirectFitAnalysisTab::emptyFitFunction() const {
  return m_fitPropertyBrowser->compositeFunction()->nFunctions() == 0;
}

const std::string IndirectFitAnalysisTab::backgroundName() const {
  const auto background = m_fitPropertyBrowser->background();

  if (background)
    return background->name();
  else
    return "None";
}

void IndirectFitAnalysisTab::setParameterValue(const std::string &functionName,
                                               const std::string &parameterName,
                                               double value) {
  m_fitPropertyBrowser->setParameterValue(functionName, parameterName, value);
}

void IndirectFitAnalysisTab::addCheckBoxFunctionGroup(
    const QString &groupName, const std::vector<IFunction_sptr> &functions,
    bool defaultValue) {
  m_fitPropertyBrowser->addCheckBoxFunctionGroup(groupName, functions,
                                                 defaultValue);
}

void IndirectFitAnalysisTab::addSpinnerFunctionGroup(
    const QString &groupName, const std::vector<IFunction_sptr> &functions,
    int minimum, int maximum, int defaultValue) {
  m_fitPropertyBrowser->addSpinnerFunctionGroup(groupName, functions, minimum,
                                                maximum, defaultValue);
}

void IndirectFitAnalysisTab::addComboBoxFunctionGroup(
    const QString &groupName, const std::vector<IFunction_sptr> &functions) {
  m_fitPropertyBrowser->addComboBoxFunctionGroup(groupName, functions);
}

void IndirectFitAnalysisTab::setBackgroundOptions(
    const QStringList &backgrounds) {
  m_fitPropertyBrowser->setBackgroundOptions(backgrounds);
}

bool IndirectFitAnalysisTab::boolSettingValue(const QString &settingKey) const {
  return m_fitPropertyBrowser->boolSettingValue(settingKey);
}

int IndirectFitAnalysisTab::intSettingValue(const QString &settingKey) const {
  return m_fitPropertyBrowser->intSettingValue(settingKey);
}

double
IndirectFitAnalysisTab::doubleSettingValue(const QString &settingKey) const {
  return m_fitPropertyBrowser->doubleSettingValue(settingKey);
}

QString
IndirectFitAnalysisTab::enumSettingValue(const QString &settingKey) const {
  return m_fitPropertyBrowser->enumSettingValue(settingKey);
}

void IndirectFitAnalysisTab::addBoolCustomSetting(const QString &settingKey,
                                                  const QString &settingName,
                                                  bool defaultValue) {
  m_fitPropertyBrowser->addBoolCustomSetting(settingKey, settingName,
                                             defaultValue);
}

void IndirectFitAnalysisTab::addDoubleCustomSetting(const QString &settingKey,
                                                    const QString &settingName,
                                                    double defaultValue) {
  m_fitPropertyBrowser->addDoubleCustomSetting(settingKey, settingName,
                                               defaultValue);
}

void IndirectFitAnalysisTab::addIntCustomSetting(const QString &settingKey,
                                                 const QString &settingName,
                                                 int defaultValue) {
  m_fitPropertyBrowser->addIntCustomSetting(settingKey, settingName,
                                            defaultValue);
}

void IndirectFitAnalysisTab::addEnumCustomSetting(const QString &settingKey,
                                                  const QString &settingName,
                                                  const QStringList &options) {
  m_fitPropertyBrowser->addEnumCustomSetting(settingKey, settingName, options);
}

void IndirectFitAnalysisTab::addOptionalDoubleSetting(
    const QString &settingKey, const QString &settingName,
    const QString &optionKey, const QString &optionName, bool enabled,
    double defaultValue) {
  m_fitPropertyBrowser->addOptionalDoubleSetting(
      settingKey, settingName, optionKey, optionName, enabled, defaultValue);
}

void IndirectFitAnalysisTab::setSelectedSpectrum(int spectrum) {
  disablePlotGuess();
  size_t specNo = static_cast<size_t>(spectrum);

  if (m_parameterValues.contains(specNo)) {
    m_fitPropertyBrowser->updateParameterValues(defaultParameterValues());
    m_fitPropertyBrowser->updateParameterValues(
        m_parameterValues[selectedSpectrum()]);
  }

  IndirectDataAnalysisTab::setSelectedSpectrum(spectrum);
  updatePreviewPlots();
  enablePlotGuess();
}

QHash<QString, double> IndirectFitAnalysisTab::createDefaultValues() const {
  return QHash<QString, double>();
}

QHash<QString, double> IndirectFitAnalysisTab::fitParameterValues() const {
  const auto spectrum = selectedSpectrum();
  if (m_parameterValues.contains(spectrum))
    return m_parameterValues[spectrum];
  else
    return QHash<QString, double>();
}

QHash<QString, double> IndirectFitAnalysisTab::defaultParameterValues() const {
  QHash<QString, double> defaultValues;
  QHash<QString, double> fitValues = fitParameterValues();

  const auto function = m_fitPropertyBrowser->theFunction();

  for (const auto &shortParamName : m_defaultPropertyValues.keys()) {
    if (!fitValues.contains(shortParamName)) {
      const auto &expectedSuffix = "." + shortParamName;
      const auto &value = m_defaultPropertyValues[shortParamName];

      for (const auto &parameter : function->getParameterNames()) {
        const auto parameterName = QString::fromStdString(parameter);

        if (!fitValues.contains(parameterName) &&
            parameterName.endsWith(expectedSuffix))
          defaultValues[parameterName] = value;
      }
    }
  }
  return defaultValues;
}

/*
 * Sets the default value for the property with the specified name,
 * in the property table of this fit analysis tab.
 *
 * @param propertyName  The name of the property whose default to set.
 * @param propertyValue The default value to set.
 */
void IndirectFitAnalysisTab::setDefaultPropertyValue(
    const QString &propertyName, const double &propertyValue) {
  m_defaultPropertyValues[propertyName] = propertyValue;
}

/*
 * Removes the default value for the property with the specified name,
 * in the property table of this fit analysis tab.
 *
 * @param propertyName  The name of the property whose default to remove.
 */
void IndirectFitAnalysisTab::removeDefaultPropertyValue(
    const QString &propertyName) {
  m_defaultPropertyValues.remove(propertyName);
}

/*
 * Checks whether the property with the specified name has a default
 * property value.
 *
 * @param propertyName  The name of the property to check for the default of.
 * @return              True if the property with the specified name has a
 *                      default value, false otherwise.
 */
bool IndirectFitAnalysisTab::hasDefaultPropertyValue(
    const QString &propertyName) {
  return m_defaultPropertyValues.contains(propertyName);
}

QSet<QString> IndirectFitAnalysisTab::parameterNames() {
  auto parameterNames = m_fitPropertyBrowser->getParameterNames();

  if (m_fitPropertyBrowser->compositeFunction()->nFunctions() == 1) {
    for (int i = 0; i < parameterNames.size(); ++i) {
      if (parameterNames[i].contains("."))
        parameterNames[i] = parameterNames[i].split(".")[1];
    }
  }
  return parameterNames.toSet();
}

/*
 * Performs necessary state changes when the fit algorithm was run
 * and completed within this interface.
 *
 * @param paramWSName          The name of the workspace containing the fit
 *                             parameter values.
 * @param propertyToParameter  Pre-existing property to parameter map to unite.
 */
void IndirectFitAnalysisTab::fitAlgorithmComplete(
    const std::string &paramWSName) {

  if (AnalysisDataService::Instance().doesExist(paramWSName)) {
    auto parameterValues = IndirectTab::extractParametersFromTable(
        paramWSName, parameterNames(), minimumSpectrum(), maximumSpectrum());

    if (m_appendResults)
      m_parameterValues =
          combineParameterValues(parameterValues, m_parameterValues);
    else
      m_parameterValues = parameterValues;

    m_fitPropertyBrowser->updateParameterValues(defaultParameterValues());
    m_fitPropertyBrowser->updateParameterValues(
        parameterValues[selectedSpectrum()]);
  }

  connect(m_fitPropertyBrowser, SIGNAL(parameterChanged(const IFunction *)),
          this, SLOT(plotGuess()));
  updatePreviewPlots();
}

void IndirectFitAnalysisTab::xMinSelected(double xMin) {
  m_fitPropertyBrowser->setStartX(xMin);
}

void IndirectFitAnalysisTab::xMaxSelected(double xMax) {
  m_fitPropertyBrowser->setEndX(xMax);
}

/*
 * Performs necessary state changes when new input data is loaded in
 * this fit analysis tab.
 * - Sets preview plot and input workspaces.
 * - Updates default property values.
 * - Updates property table.
 * - Updates preview plots.
 *
 * @param wsName  The name of the loaded input workspace.
 */
void IndirectFitAnalysisTab::newInputDataLoaded(const QString &wsName) {
  auto inputWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      wsName.toStdString());
  m_fitPropertyBrowser->setWorkspaceName(wsName);
  setInputWorkspace(inputWs);
  m_defaultPropertyValues = createDefaultValues();
  m_fitPropertyBrowser->updateParameterValues(defaultParameterValues());
  setPreviewPlotWorkspace(inputWs);
  m_parameterValues.clear();
  updatePreviewPlots();
}

/*
 * Clears all slots connected to the batch runners signals.
 */
void IndirectFitAnalysisTab::clearBatchRunnerSlots() {
  m_batchAlgoRunner->disconnect();
}

void IndirectFitAnalysisTab::fitFunctionChanged() {
  const auto spectrum = selectedSpectrum();

  if (m_parameterValues.contains(spectrum)) {
    m_fitPropertyBrowser->updateParameterValues(defaultParameterValues());
    m_fitPropertyBrowser->updateParameterValues(
        m_parameterValues[selectedSpectrum()]);
  } else
    m_fitPropertyBrowser->updateParameterValues(defaultParameterValues());
  updatePreviewPlots();
}

/*
 * Saves the result workspace with the specified name, in the default
 * save directory.
 *
 * @param resultName  The name of the workspace to save.
 */
void IndirectFitAnalysisTab::saveResult(const std::string &resultName) {
  // check workspace exists
  const auto wsFound = checkADSForPlotSaveWorkspace(resultName, false);
  // process workspace after check
  if (wsFound) {
    QString saveDir = QString::fromStdString(
        Mantid::Kernel::ConfigService::Instance().getString(
            "defaultsave.directory"));
    // Check validity of save path
    QString QresultWsName = QString::fromStdString(resultName);
    const auto fullPath = saveDir.append(QresultWsName).append(".nxs");
    addSaveWorkspaceToQueue(QresultWsName, fullPath);
    m_batchAlgoRunner->executeBatchAsync();
  }
}

/*
 * Plots the result workspace with the specified name, using the specified
 * plot type. Plot type can either be 'None', 'All' or the name of a
 * parameter. In the case of 'None', nothing will be plotted. In the case of
 * 'All', everything will be plotted. In the case of a parameter name, only
 * the spectra created from that parameter will be plotted.
 *
 * @param resultName  The name of the workspace to plot.
 * @param plotType    The plot type specifying what to plot.
 */
void IndirectFitAnalysisTab::plotResult(const std::string &resultName,
                                        const QString &plotType) {
  const auto wsFound = checkADSForPlotSaveWorkspace(resultName, true);
  if (wsFound) {
    MatrixWorkspace_sptr resultWs =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(resultName);
    QString resultWsQName = QString::fromStdString(resultName);

    // Handle plot result
    if (plotType.compare("All") == 0) {
      const auto specEnd = (int)resultWs->getNumberHistograms();
      for (int i = 0; i < specEnd; ++i)
        IndirectTab::plotSpectrum(resultWsQName, i);
    } else {
      QHash<QString, size_t> labels =
          IndirectTab::extractAxisLabels(resultWs, 1);

      for (const auto &parameter : m_fitPropertyBrowser->getParameterNames()) {
        if (parameter == plotType) {
          if (labels.contains(parameter))
            IndirectTab::plotSpectrum(resultWsQName, (int)labels[parameter]);
        }
      }
    }
  }
}

/*
 * Fills the specified combo-box, with the possible parameters which
 * can be plot separately.
 *
 * @param comboBox  The combo box to fill.
 */
void IndirectFitAnalysisTab::fillPlotTypeComboBox(QComboBox *comboBox) {
  comboBox->clear();
  comboBox->addItem("All");

  QSet<QString> parameters;
  for (const auto &parameter : m_fitPropertyBrowser->getParameterNames())
    parameters.insert(parameter.right(parameter.lastIndexOf('.')));
  comboBox->addItems(parameters.toList());
}

/*
 * Updates the preview plots in this fit analysis tab, given the name
 * of the output workspace from a fit.
 *
 * @param workspaceName   The name of the workspace to plot.
 * @param fitPreviewPlot  The preview plot widget in which to plot the fit.
 * @param diffPreviewPlot The preview plot widget in which to plot the
 *                        difference between the fit and sample data.
 */
void IndirectFitAnalysisTab::updatePlot(
    const std::string &workspaceName,
    MantidQt::MantidWidgets::PreviewPlot *fitPreviewPlot,
    MantidQt::MantidWidgets::PreviewPlot *diffPreviewPlot) {

  if (equivalentFunctions(m_fitFunction,
                          m_fitPropertyBrowser->compositeFunction()))
    IndirectDataAnalysisTab::updatePlot(workspaceName, fitPreviewPlot,
                                        diffPreviewPlot);
  else
    IndirectDataAnalysisTab::updatePlot("", fitPreviewPlot, diffPreviewPlot);
}

IAlgorithm_sptr IndirectFitAnalysisTab::singleFitAlgorithm() {
  auto algorithm = AlgorithmManager::Instance().create("Fit");
  algorithm->setProperty("WorkspaceIndex",
                         m_fitPropertyBrowser->workspaceIndex());
  return algorithm;
}

IAlgorithm_sptr IndirectFitAnalysisTab::sequentialFitAlgorithm() {
  return singleFitAlgorithm();
}

void IndirectFitAnalysisTab::executeSingleFit() {
  runFitAlgorithm(singleFitAlgorithm());
}

void IndirectFitAnalysisTab::executeSequentialFit() {
  runFitAlgorithm(sequentialFitAlgorithm());
}

IFunction_sptr IndirectFitAnalysisTab::fitFunction() const {
  return m_fitPropertyBrowser->getFittingFunction();
}

MatrixWorkspace_sptr IndirectFitAnalysisTab::fitWorkspace() const {
  return boost::dynamic_pointer_cast<MatrixWorkspace>(
      m_fitPropertyBrowser->getWorkspace());
}

/*
 * Runs the specified fit algorithm and calls the algorithmComplete
 * method of this fit analysis tab once completed.
 *
 * @param fitAlgorithm      The fit algorithm to run.
 */
void IndirectFitAnalysisTab::runFitAlgorithm(IAlgorithm_sptr fitAlgorithm) {
  disconnect(m_fitPropertyBrowser, SIGNAL(parameterChanged(const IFunction *)),
             this, SLOT(plotGuess()));

  fitAlgorithm->setProperty("InputWorkspace", fitWorkspace());
  fitAlgorithm->setProperty("Function", fitFunction()->asString());
  fitAlgorithm->setProperty("StartX", m_fitPropertyBrowser->startX());
  fitAlgorithm->setProperty("EndX", m_fitPropertyBrowser->endX());
  fitAlgorithm->setProperty("Minimizer", m_fitPropertyBrowser->minimizer(true));
  fitAlgorithm->setProperty("MaxIterations",
                            m_fitPropertyBrowser->maxIterations());

  if (fitAlgorithm->existsProperty("Convolve")) {
    fitAlgorithm->setProperty("Convolve",
                              m_fitPropertyBrowser->convolveMembers());
  }

  if (fitAlgorithm->existsProperty("PeakRadius")) {
    fitAlgorithm->setProperty("PeakRadius",
                              m_fitPropertyBrowser->getPeakRadius());
  }

  m_fitFunction = m_fitPropertyBrowser->getFittingFunction()->clone();
  m_batchAlgoRunner->addAlgorithm(fitAlgorithm);
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(algorithmComplete(bool)));
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(clearBatchRunnerSlots()));
  m_batchAlgoRunner->executeBatchAsync();
}

/*
* Plots a guess of the fit for the specified function, in the
* specified preview plot widget.
*
* @param previewPlot The preview plot widget in which to plot
*                    the guess.
* @param function    The function to fit.
*/
void IndirectFitAnalysisTab::plotGuess(
  MantidQt::MantidWidgets::PreviewPlot *previewPlot,
  IFunction_const_sptr function) {
  previewPlot->removeSpectrum("Guess");

  if (inputWorkspace()) {

    if (!m_guessWorkspace || selectedSpectrum() != m_guessSpectrum) {
      m_guessWorkspace = createGuessWorkspace(function, selectedSpectrum());
      m_guessSpectrum = selectedSpectrum();
    }

    // Check whether the guess workspace has enough data points
    // to plot
    if (m_guessWorkspace->x(0).size() >= 2) {
      previewPlot->addSpectrum("Guess", m_guessWorkspace, 0, Qt::green);
    }
  }
}

/*
* Creates a guess workspace, for approximating a fit with the specified
* function on the input workspace.
*
* @param func    The function to fit.
* @param wsIndex The index of the input workspace to create a guess for.
* @return        A guess workspace containing the guess data for the fit.
*/
MatrixWorkspace_sptr
IndirectFitAnalysisTab::createGuessWorkspace(IFunction_const_sptr func,
  size_t wsIndex) {
  const auto inputWS = inputWorkspace();
  const auto binIndexLow = inputWS->binIndexOf(startX());
  const auto binIndexHigh = inputWS->binIndexOf(endX());
  const auto nData = binIndexHigh - binIndexLow;

  const auto &xPoints = inputWS->points(wsIndex);

  std::vector<double> dataX(nData);
  std::copy(&xPoints[binIndexLow], &xPoints[binIndexLow + nData],
    dataX.begin());
  const auto dataY = computeOutput(func, dataX);

  if (dataY.empty())
    return WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);

  IAlgorithm_sptr createWsAlg =
    createWorkspaceAlgorithm("__GuessAnon", 1, dataX, dataY);
  createWsAlg->execute();
  return createWsAlg->getProperty("OutputWorkspace");
}

/*
* Computes the output vector of applying the specified function to
* the specified input vector.
*
* @param func    The function to apply.
* @param dataX   Vector of input data.
* @return        Vector containing values calculated from applying
*                the specified function to the input data.
*/
std::vector<double>
IndirectFitAnalysisTab::computeOutput(IFunction_const_sptr func,
  const std::vector<double> &dataX) {
  if (dataX.empty())
    return std::vector<double>();

  FunctionDomain1DVector domain(dataX);
  FunctionValues outputData(domain);
  func->function(domain, outputData);

  std::vector<double> dataY(dataX.size());
  for (size_t i = 0; i < dataY.size(); i++) {
    dataY[i] = outputData.getCalculated(i);
  }
  return dataY;
}

/*
* Generates and returns an algorithm for creating a workspace, with
* the specified name, number of spectra and containing the specified
* x data and y data.
*
* @param workspaceName The name of the workspace to create.
* @param numSpec       The number of spectra in the workspace to create.
* @param dataX         The x data to add to the created workspace.
* @param dataY         The y data to add to the created workspace.
* @return              An algorithm for creating the workspace.
*/
IAlgorithm_sptr IndirectFitAnalysisTab::createWorkspaceAlgorithm(
  const std::string &workspaceName, int numSpec,
  const std::vector<double> &dataX, const std::vector<double> &dataY) {
  IAlgorithm_sptr createWsAlg =
    AlgorithmManager::Instance().create("CreateWorkspace");
  createWsAlg->initialize();
  createWsAlg->setChild(true);
  createWsAlg->setLogging(false);
  createWsAlg->setProperty("OutputWorkspace", workspaceName);
  createWsAlg->setProperty("NSpec", numSpec);
  createWsAlg->setProperty("DataX", dataX);
  createWsAlg->setProperty("DataY", dataY);
  return createWsAlg;
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
