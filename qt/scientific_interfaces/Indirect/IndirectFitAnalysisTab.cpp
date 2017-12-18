#include "IndirectFitAnalysisTab.h"

#include "MantidAPI/FunctionFactory.h"

#include <QString>

using namespace Mantid::API;

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
  m_stringManager = new QtStringPropertyManager(m_parentWidget);
  m_propertyTree = new QtTreePropertyBrowser();
}

/*
 * Sets the fit functions used in the most recent fit within this fit
 * analysis tab.
 *
 * @param fitFunctions  The functions used in the most recent fit.
 */
void IndirectFitAnalysisTab::setFitFunctions(
    const QVector<QString> &fitFunctions) {
  m_appendResults = fitFunctions == m_fitFunctions;

  if (!m_appendResults)
    m_fitFunctions = fitFunctions;
}

/*
 * Sets the functions shown in the property table of this fit analysis
 * tab. Also the functions to be used in the next fit.
 *
 * @param fitFunctions  The functions to use in the next fit.
 */
void IndirectFitAnalysisTab::setPropertyFunctions(
    const QVector<QString> &functions) {
  clearFunctionProperties();
  m_propertyFunctions = functions;

  for (const auto &propertyFunction : m_propertyFunctions) {
    updateProperty(propertyFunction, selectedSpectrum());
    m_propertyTree->addProperty(m_properties[propertyFunction]);
  }
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

  if (m_propertyFunctions.contains(propertyName))
    updateProperty(propertyName, selectedSpectrum());
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

/*
 * Check whether a parameter value from a parameters workspace has been
 * found and saved for the current input, for the property with the
 * specified name and for the specified spectrum.
 *
 * @param propertyName    The name of the property associated to the parameter.
 * @param spectrumNumber  The spectrum for which the parameter is defined.
 */
bool IndirectFitAnalysisTab::hasParameterValue(const QString &propertyName,
                                               const size_t &spectrumNumber) {
  return m_parameterValues.contains(propertyName) &&
         m_parameterValues[propertyName].contains(spectrumNumber);
}

/*
 * Performs necessary state changes when the fit algorithm was run
 * and completed within this interface.
 *
 * @param paramWSName     The name of the workspace containing the fit
 *                        parameter values.
 */
void IndirectFitAnalysisTab::fitAlgorithmComplete(
    const std::string &paramWSName) {
  fitAlgorithmComplete(paramWSName, QHash<QString, QString>());
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
    const std::string &paramWSName,
    const QHash<QString, QString> &propertyToParameter) {

  if (AnalysisDataService::Instance().doesExist(paramWSName)) {
    m_propertyToParameter =
        createPropertyToParameterMap(m_fitFunctions).unite(propertyToParameter);
    auto parameterValues = IndirectTab::extractParametersFromTable(
        paramWSName, m_propertyToParameter.values().toSet(), minimumSpectrum(),
        maximumSpectrum());

    if (m_appendResults)
      m_parameterValues =
          combineParameterValues(parameterValues, m_parameterValues);
    else
      m_parameterValues = parameterValues;
  }

  updateProperties(selectedSpectrum());
  updatePreviewPlots();
}

/*
 * Combines the two maps of parameter values, by adding the values from
 * the second into the first, where the parameters (keys) are taken from
 * the first map, and the value doesn't already exist in the first map.
 *
 * @param parameterValues1  The first parameter values map to combine with.
 * @param parameterValues2  The second parameter values map to combine with.
 * @return                  The combined map.
 */
QHash<QString, QHash<size_t, double>>
IndirectFitAnalysisTab::combineParameterValues(
    const QHash<QString, QHash<size_t, double>> &parameterValues1,
    const QHash<QString, QHash<size_t, double>> &parameterValues2) {
  auto combinedValues = parameterValues1;

  for (const auto &parameterName : parameterValues1.keys()) {

    if (parameterValues2.contains(parameterName)) {
      const auto &values1 = parameterValues1[parameterName];
      const auto &values2 = parameterValues2[parameterName];

      for (const auto &index : values2.keys()) {
        if (!values1.contains(index))
          combinedValues[parameterName][index] = values2[index];
      }
    }
  }

  return combinedValues;
}

/*
 * Updates the values of the properties in the property table of
 * this fit analysis tab, using the parameter values found in the
 * fit, for the specified spectrum.
 *
 * @param specNo  The spectrum number for which the parameter values
 *                will be extracted for.
 */
void IndirectFitAnalysisTab::updateProperties(int specNo) {
  disablePlotGuess();

  size_t index = boost::numeric_cast<size_t>(specNo);

  // Iterate through all floating number properties in the property table.
  for (const auto &propertyName : m_properties.keys())
    updateProperty(propertyName, index);

  enablePlotGuess();
}

/*
 * Updates the value of the property, with the specified name, in the
 * property table of this fit analysis tab.
 *
 * @param propertyName  The name of the property to update.
 * @param index         The spectrum number selected in this tab.
 */
void IndirectFitAnalysisTab::updateProperty(const QString &propertyName,
                                            const size_t &index) {
  // Check whether values for this property were found in the
  // parameters workspace.
  if (m_propertyToParameter.contains(propertyName)) {
    const auto &parameterName = m_propertyToParameter[propertyName];
    const auto &parameters = m_parameterValues[parameterName];

    if (parameters.contains(index)) {
      m_dblManager->setValue(m_properties[propertyName], parameters[index]);
      return;
    }
  }

  const auto functionAndParameter = propertyName.split(".");
  // If parameter values were not found in fit for property at
  // the specified spectrum, update with default.
  if (functionAndParameter.size() > 1 &&
      hasDefaultPropertyValue(functionAndParameter[1]))
    m_dblManager->setValue(m_properties[propertyName],
                           m_defaultPropertyValues[functionAndParameter[1]]);
}

/*
 * Clears the property tree of all functions.
 */
void IndirectFitAnalysisTab::clearFunctionProperties() {
  for (const auto &propertyFunction : m_propertyFunctions)
    m_propertyTree->removeProperty(m_properties[propertyFunction]);
}

/*
 * Fixes the selected item in the property browser tree.
 *
 * @param tree  The tree whose selected item to fix.
 */
void IndirectFitAnalysisTab::fixSelectedItem() {
  QtBrowserItem *item = m_propertyTree->currentItem();

  // Determine what the property is.
  QtProperty *prop = item->property();
  QtProperty *fixedProp = m_stringManager->addProperty(prop->propertyName());
  QtProperty *fprlbl = m_stringManager->addProperty("Fixed");
  fixedProp->addSubProperty(fprlbl);
  m_stringManager->setValue(fixedProp, prop->valueText());

  item->parent()->property()->addSubProperty(fixedProp);
  m_fixedProps[fixedProp] = prop;
  item->parent()->property()->removeSubProperty(prop);
}

/*
 * Unfixes the selected item in the property browser tree.
 *
 * @param tree  The tree whose selected item to fix.
 */
void IndirectFitAnalysisTab::unFixSelectedItem() {
  QtBrowserItem *item = m_propertyTree->currentItem();

  QtProperty *prop = item->property();
  if (prop->subProperties().empty()) {
    item = item->parent();
    prop = item->property();
  }

  item->parent()->property()->addSubProperty(m_fixedProps[prop]);
  item->parent()->property()->removeSubProperty(prop);
  m_fixedProps.remove(prop);
  QtProperty *proplbl = prop->subProperties()[0];
  delete proplbl;
  delete prop;
}

/*
 * Checks whether the specified property can be fixed.
 *
 * @param prop  The property to check for fixability.
 * @return      True if the property is fixable, False otherwise.
 */
bool IndirectFitAnalysisTab::isFixable(QtProperty const *prop) const {
  return prop->propertyManager() == m_dblManager;
}

/*
 * Checks whether the specified property is fixed.
 *
 * @param prop  The property to check.
 * @return      True if the property is fixed, False otherwise.
 */
bool IndirectFitAnalysisTab::isFixed(QtProperty const *prop) const {
  return prop->propertyManager() == m_stringManager;
}

void IndirectFitAnalysisTab::fitContextMenu(const QString &menuName) {
  QtBrowserItem *item(nullptr);

  item = m_propertyTree->currentItem();

  if (!item)
    return;

  QtProperty *prop = item->property();
  auto fixed = isFixed(prop);

  if (!isFixable(prop) && !isFixed(prop))
    return;

  // Create the menu
  QMenu *menu = new QMenu(menuName, m_propertyTree);
  QAction *action;

  if (!fixed) {
    action = new QAction("Fix", m_parentWidget);
    connect(action, SIGNAL(triggered()), this, SLOT(fixSelectedItem()));
  } else {
    action = new QAction("Remove Fix", m_parentWidget);
    connect(action, SIGNAL(triggered()), this, SLOT(unFixSelectedItem()));
  }

  menu->addAction(action);

  // Show the menu
  menu->popup(QCursor::pos());
}

/*
 * Performs necessary state changes when new input data is loaded in
 * this fit analysis tab.
 * - Sets preview plot and input workspaces.
 * - Clears stored parameter values.
 * - Updates property table.
 * - Updates preview plot.
 *
 * @param wsName  The name of the loaded input workspace.
 */
void IndirectFitAnalysisTab::newInputDataLoaded(const QString &wsName) {
  auto inputWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      wsName.toStdString());
  setInputWorkspace(inputWs);
  setPreviewPlotWorkspace(inputWs);
  m_parameterValues.clear();
  m_propertyToParameter.clear();
  m_fitFunctions.clear();
  updateProperties(selectedSpectrum());
  updatePreviewPlots();
}

/*
 * Clears all slots connected to the batch runners signals.
 */
void IndirectFitAnalysisTab::clearBatchRunnerSlots() {
  m_batchAlgoRunner->disconnect();
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
    if (plotType.compare("None") != 0) {
      if (plotType.compare("All") == 0) {
        const auto specEnd = (int)resultWs->getNumberHistograms();
        for (int i = 0; i < specEnd; ++i)
          IndirectTab::plotSpectrum(resultWsQName, i);
      } else {
        QHash<QString, size_t> labels =
            IndirectTab::extractAxisLabels(resultWs, 1);

        for (const auto &propertyName : m_propertyToParameter.keys()) {
          const auto functionAndParameter = propertyName.split(".");
          const auto &parameter = functionAndParameter.last();

          if (parameter == plotType) {
            const auto label = m_propertyToParameter[propertyName];
            if (labels.contains(label))
              IndirectTab::plotSpectrum(resultWsQName, (int)labels[label]);
          }
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

  if (m_propertyFunctions.isEmpty())
    return;

  comboBox->addItem("All");
  comboBox->addItem("None");

  QSet<QString> parameters;
  for (const auto &fitFunction : m_propertyFunctions) {
    for (const auto &parameter : getFunctionParameters(fitFunction))
      parameters.insert(parameter);
  }
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
  if (m_fitFunctions == m_propertyFunctions)
    IndirectDataAnalysisTab::updatePlot(workspaceName, fitPreviewPlot,
                                        diffPreviewPlot);
  else
    IndirectDataAnalysisTab::updatePlot("", fitPreviewPlot, diffPreviewPlot);
}

/*
 * Runs the specified fit algorithm and calls the algorithmComplete
 * method of this fit analysis tab once completed.
 *
 * @param fitAlgorithm      The fit algorithm to run.
 */
void IndirectFitAnalysisTab::runFitAlgorithm(
    Mantid::API::IAlgorithm_sptr fitAlgorithm) {
  m_batchAlgoRunner->addAlgorithm(fitAlgorithm);
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(algorithmComplete(bool)));
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(clearBatchRunnerSlots()));
  m_batchAlgoRunner->executeBatchAsync();
}

/*
 * Adds the necessary prefix to the specified parameter, in order to match
 * the name in the parameters table created from a fit with multiple
 * functions.
 *
 * @param parameter      The name of the parameter.
 * @param functionNumber The index of the function which owns this parameter.
 * @return               A string containing the parameter name with the
 *                       appropriate prefix prepended.
 */
QString
IndirectFitAnalysisTab::addPrefixToParameter(const QString &parameter,
                                             const QString &,
                                             const int &functionNumber) const {
  return "f" + QString::number(functionNumber) + "." + parameter;
}

/*
 * Adds the necessary prefix to the specified parameter, in order to match
 * the name in the parameters table created from a fit with a single function.
 *
 * @param parameter      The name of the parameter.
 * @return               A string containing the parameter name with the
 *                       appropriate prefix prepended.
 */
QString IndirectFitAnalysisTab::addPrefixToParameter(const QString &parameter,
                                                     const QString &) const {
  return parameter;
}

/*
 * Adds the necessary prefix to the specified vector of parameters, in order
 * to match the names in the parameters table, created from a fit with
 * multiple functions.
 *
 * @param parameters     A vector containing the vector of parameters for each
 *                       function.
 * @param functionNames  A vector containing the names of the functions used
 *                       in the fit.
 * @return               A vector containing a vector of parameters with the
 *                       appropriate prefixes prepended for each function.
 */
QVector<QVector<QString>> IndirectFitAnalysisTab::addPrefixToParameters(
    const QVector<QVector<QString>> &parameters,
    const QVector<QString> &functionNames) const {
  QVector<QVector<QString>> parametersWithPrefix(parameters.size());

  for (int i = 0; i < parameters.size(); ++i) {
    const auto &functionName = functionNames[i];
    const auto &functionParameters = parameters[i];
    parametersWithPrefix[i] = QVector<QString>(parameters[i].size());

    for (int j = 0; j < functionParameters.size(); ++j)
      parametersWithPrefix[i][j] =
          addPrefixToParameter(functionParameters[j], functionName, i);
  }
  return parametersWithPrefix;
}

/*
 * Adds the necessary prefix to the specified vector of parameters, in order
 * to match the names in the parameters table, created from a fit with a
 * single function.
 *
 * @param parameters     A vector containing the parameters of the function.
 * @param functionName   The name of the function used in the fit.
 * @return               The vector of parameters with the appropriate
 * prefixes prepended.
 */
QVector<QString> IndirectFitAnalysisTab::addPrefixToParameters(
    const QVector<QString> &parameters, const QString &functionName) const {
  QVector<QString> parametersWithPrefix(parameters.size());

  for (int i = 0; i < parameters.size(); ++i)
    parametersWithPrefix[i] = addPrefixToParameter(parameters[i], functionName);
  return parametersWithPrefix;
}

/*
 * Creates a map from the name of a property in the property table of this
 * fit analysis tab, to the name of the parameter as found in the parameter
 * workspace generated from a fit.
 *
 * @param functionNames          The name of the functions used in the fit.
 * @return                       A map from property name to parameter name.
 */
QHash<QString, QString> IndirectFitAnalysisTab::createPropertyToParameterMap(
    const QVector<QString> &functionNames) const {

  if (functionNames.size() == 1) {
    const auto parameters = getFunctionParameters(functionNames[0]);
    const auto parametersWithPrefix =
        addPrefixToParameters(parameters, functionNames[0]);

    return createPropertyToParameterMap(functionNames[0], parameters,
                                        parametersWithPrefix);
  } else if (!functionNames.isEmpty()) {
    const auto parameters = getFunctionParameters(functionNames);
    const auto parametersWithPrefix =
        addPrefixToParameters(parameters, functionNames);

    return createPropertyToParameterMap(functionNames, parameters,
                                        parametersWithPrefix);
  } else {
    return QHash<QString, QString>();
  }
}

/*
 * Creates a map from the name of a property in the property table of this
 * fit analysis tab, to the name of the parameter as found in the parameter
 * workspace generated from a fit.
 *
 * @param functionNames         The names of the functions used in the fit.
 * @param parameters            A vector containing a vector of parameters,
 * for each function used in the fit.
 * @param parametersWithPrefix  A vector containing a vector of parameters
 * with the appropriate prefixes prepended, for each function used in the fit.
 * @return                      A map from property name to parameter name.
 */
QHash<QString, QString> IndirectFitAnalysisTab::createPropertyToParameterMap(
    const QVector<QString> &functionNames,
    const QVector<QVector<QString>> &parameters,
    const QVector<QVector<QString>> &parametersWithPrefix) const {
  QHash<QString, QString> propertyToParameter;

  for (int i = 0; i < parameters.size(); ++i) {
    const auto &functionName = functionNames[i];
    const auto &functionParams = parameters[i];
    const auto &functionParamsWithPrefix = parametersWithPrefix[i];

    for (int j = 0; j < functionParams.size(); ++j)
      propertyToParameter[functionName + "." + functionParams[j]] =
          functionParamsWithPrefix[j];
  }
  return propertyToParameter;
}

/*
 * Creates a map from the name of a property in the property table of this
 * fit analysis tab, to the name of the parameter as found in the parameter
 * workspace generated from a fit.
 *
 * @param functionName          The name of the function used in the fit.
 * @param parameters            A vector containing the parameters, for the
 *                              function used in the fit.
 * @param parametersWithPrefix  A vector containing the parameters with the
 *                              appropriate prefixes prepended.
 * @return                      A map from property name to parameter name.
 */
QHash<QString, QString> IndirectFitAnalysisTab::createPropertyToParameterMap(
    const QString &functionName, const QVector<QString> &parameters,
    const QVector<QString> &parametersWithPrefix) const {
  QHash<QString, QString> propertyToParameter;

  for (int i = 0; i < parameters.size(); ++i)
    propertyToParameter[functionName + "." + parameters[i]] =
        parametersWithPrefix[i];
  return propertyToParameter;
}

/*
 * Creates a QtProperty containing a property for each parameter in the
 * function with the specified name. Properties have name in the format:
 * FunctionName.ParameterName Adds default values for the parameters of the
 * function, into this fit analysis tab's double manager.
 *
 * @param functionName  The name of the function to create a QtProperty from.
 * @param addParameters Adds parameters from function to QtProperty if True,
 *                      else returns empty QtProperty.
 */
QtProperty *
IndirectFitAnalysisTab::createFunctionProperty(const QString &functionName,
                                               const bool &addParameters) {
  return createFunctionProperty(m_grpManager->addProperty(functionName),
                                addParameters);
}

/*
 * Adds properties to the specified property for each parameter of the
 * function with the same name as the name of the specified property.
 * Properties have names in the format: FunctionName.ParameterName
 * Adds default values for the parameters of the function, into this fit
 * analysis tab's double manager.
 *
 * @param functionGroup The group property to add parameters to.
 * @param addParameters Adds parameters from function to QtProperty if True,
 *                      else returns empty QtProperty.
 */
QtProperty *
IndirectFitAnalysisTab::createFunctionProperty(QtProperty *functionGroup,
                                               const bool &addParameters) {
  const auto &functionName = functionGroup->propertyName();
  const auto parameters = getFunctionParameters(functionName);

  for (const auto &parameter : parameters) {
    QString paramName = functionName + "." + parameter;
    m_properties[paramName] = m_dblManager->addProperty(parameter);
    m_dblManager->setDecimals(m_properties[paramName], NUM_DECIMALS);

    if (addParameters)
      functionGroup->addSubProperty(m_properties[paramName]);
  }
  return functionGroup;
}

/*
 * Extracts the parameters for the functions with the specified names.
 *
 * @param functionNames The names of the functions to extract the parameters
 *                      for.
 * @return              A vector containing a vector of parameters for each
 *                      specified fucntion.
 */
QVector<QVector<QString>> IndirectFitAnalysisTab::getFunctionParameters(
    const QVector<QString> &functionNames) const {
  QVector<QVector<QString>> parameters(functionNames.size());

  for (int i = 0; i < functionNames.size(); ++i)
    parameters[i] = getFunctionParameters(functionNames[i]);
  return parameters;
}

/*
 * Extracts the parameters for the function with the specified name.
 *
 * @param functionName  The name of the function whose parameters to extract.
 * @return              The vector of parameters for the specified function.
 */
QVector<QString> IndirectFitAnalysisTab::getFunctionParameters(
    const QString &functionName) const {
  const auto func = getFunction(functionName);
  return IndirectTab::convertStdStringVector(func->getParameterNames());
}

IFunction_sptr
IndirectFitAnalysisTab::getFunction(const QString &functionName) const {
  return FunctionFactory::Instance().createFunction(functionName.toStdString());
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
