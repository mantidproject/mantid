#include "IndirectFitAnalysisTab.h"
#include "IndirectDataAnalysis.h"

#include "MantidAPI/FunctionFactory.h"

#include <QSettings>
#include <QString>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>

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
    : IndirectDataAnalysisTab(parent) {}

/*
 * Sets the default value for the property with the specified name,
 * in the property table of this fit analysis tab.
 *
 * @param propertyName  The name of the property whose default to set.
 * @param propertyValue The default value to set.
 */
void IndirectFitAnalysisTab::setDefaultPropertyValue(
    const QString &propertyName, double propertyValue) {
  m_defaultPropertyValues[propertyName] = propertyValue;
}

/*
 * Performs necessary state changes when the fit algorithm was run
 * and completed within this interface.
 *
 * @param paramWSName   The name of the workspace containing the fit
 *                      parameter values.
 * @param functionNames A vector containing the names of the functions
 *                      used in the fit.
 */
void IndirectFitAnalysisTab::fitAlgorithmComplete(
    const std::string &paramWSName, const QVector<QString> &functionNames,
    bool usedBackground) {
  QHash<QString, QString> propertyToParameter;

  if (usedBackground)
    propertyToParameter["f0.A0"] = "BackgroundA0";

  if (AnalysisDataService::Instance().doesExist(paramWSName)) {
    m_propertyToParameter =
        createPropertyToParameterMap(functionNames, propertyToParameter);
    m_parameterValues = IndirectTab::extractParametersFromTable(
        paramWSName, m_propertyToParameter.values().toSet(), minimumSpectrum(),
        maximumSpectrum());
  }
  updateProperties(selectedSpectrum());
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
  size_t index = boost::numeric_cast<size_t>(specNo);
  auto parameterNames = m_parameterValues.keys();

  // Iterate through all floating number properties in the property table.
  for (const auto &dblProperty : m_dblManager->properties()) {
    auto propertyName = dblProperty->propertyName();

    // Check whether values for this property were found in the
    // parameters workspace.
    if (m_parameterValues.contains[propertyName]) {
      auto parameterName = m_propertyToParameter[propertyName];
      auto parameters = m_parameterValues[parameterName];

      if (parameters.contains(index)) {
        m_dblManager->setValue(m_properties[propertyName], parameters[index]);
        break;
      }
    }
    // If parameter values were not found in fit for property at
    // the specified spectrum, update with default.
    if (m_defaultPropertyValues.contains[propertyName])
      m_dblManager->setValue(m_properties[propertyName],
                             m_defaultPropertyValues[propertyName]);
    else
      m_dblManager->setValue(m_properties[propertyName], 0.0);
  }
}

/*
 * Performs necessary state changes when new input data is loaded in
 * this fit analysis tab.
 *
 * @param wsName  The name of the loaded input workspace.
 */
void IndirectFitAnalysisTab::newDataLoaded(const QString &wsName) {
  auto inputWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      wsName.toStdString());
  setInputWorkspace(inputWs);
  setPreviewPlotWorkspace(inputWs);
  m_parameterValues.clear();
  m_propertyToParameter.clear();
}

/*
 * Adds the necessary prefix to the specified parameter, in order to match
 * the name in the parameters table created from a fit with multiple functions.
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
 * Adds the necessary prefix to the specified vector of parameters, in order to
 * match the names in the parameters table, created from a fit with multiple
 * functions.
 *
 * @param parameters     A vector containing the vector of parameters for each
 *                       function.
 * @param functionNames  A vector containing the names of the functions used in
 *                       the fit.
 * @return               A vector containing a vector of parameters with the
 *                       appropriate prefixes prepended for each function.
 */
QVector<QVector<QString>> IndirectFitAnalysisTab::addPrefixToParameters(
    const QVector<QVector<QString>> &parameters,
    const QVector<QString> &functionNames) const {
  QVector<QVector<QString>> parametersWithPrefix(parameters.size());

  for (int i = 0; i < parameters.size(); ++i) {
    parametersWithPrefix[i] = QVector<QString>(parameters[i].size());

    for (int j = 0; j < parameters[i].size(); ++j) {
      parametersWithPrefix[i][j] =
          addPrefixToParameter(parameters[i][j], functionNames[i][j], i);
    }
  }
  return parametersWithPrefix;
}

/*
 * Adds the necessary prefix to the specified vector of parameters, in order to
 * match the names in the parameters table, created from a fit with a single
 * function.
 *
 * @param parameters     A vector containing the parameters of the function.
 * @param functionName   The name of the function used in the fit.
 * @return               The vector of parameters with the appropriate prefixes
 *                       prepended.
 */
QVector<QString> IndirectFitAnalysisTab::addPrefixToParameters(
    const QVector<QString> &parameters, const QString &functionName) const {
  QVector<QString> parametersWithPrefix(parameters.size());

  for (int i = 0; i < parameters.size(); i++)
    parametersWithPrefix[i] = addPrefixToParameter(parameters[i], functionName);
}

/*
 * Creates a map from the name of a property in the property table of this
 * fit analysis tab, to the name of the parameter as found in the parameter
 * workspace generated from a fit.
 *
 * @param functionNames          The name of the functions used in the fit.
 * @param propertyToParameterMap Pre-existing map to unite with the created
 *                               map.
 * @return                       A map from property name to parameter name.
 */
QHash<QString, QString> IndirectFitAnalysisTab::createPropertyToParameterMap(
    const QVector<QString> &functionNames,
    const QHash<QString, QString> &propertyToParameterMap) const {

  if (functionNames.size() == 1) {
    const auto parameters = getFunctionParameters(functionNames[0]);
    const auto parametersWithPrefix =
        addPrefixToParameters(parameters, functionNames[0]);

    return createPropertyToParameterMap(functionNames[0], parameters,
                                        parametersWithPrefix)
        .unite(propertyToParameterMap);
  } else if (!functionNames.isEmpty()) {
    const auto parameters = getFunctionParameters(functionNames);
    const auto parametersWithPrefix =
        addPrefixToParameters(parameters, functionNames);

    return createPropertyToParameterMap(functionNames, parameters,
                                        parametersWithPrefix)
        .unite(propertyToParameterMap);
  } else {
    return propertyToParameterMap;
  }
}

/*
 * Creates a map from the name of a property in the property table of this
 * fit analysis tab, to the name of the parameter as found in the parameter
 * workspace generated from a fit.
 *
 * @param functionNames         The names of the functions used in the fit.
 * @param parameters            A vector containing a vector of parameters, for
 *                              each function used in the fit.
 * @param parametersWithPrefix  A vector containing a vector of parameters with
 *                              the appropriate prefixes prepended, for each
 *                              function used in the fit.
 * @return                      A map from property name to parameter name.
 */
QHash<QString, QString> IndirectFitAnalysisTab::createPropertyToParameterMap(
    const QVector<QString> &functionNames,
    const QVector<QVector<QString>> &parameters,
    const QVector<QVector<QString>> &parametersWithPrefix) const {
  QHash<QString, QString> propertyToParameter;
  QHash<QString, int> functionUsageCount;

  for (int i = 0; i < parameters.size(); ++i) {
    QString functionName = functionNames[i];

    if (functionUsageCount.contains(functionName)) {
      functionName += QString::number(functionUsageCount[functionName]);
      functionUsageCount[functionName] += 1;
    } else {
      functionUsageCount[functionName] = 1;
    }

    for (int j = 0; j < parameters[i].size(); ++j)
      propertyToParameter[functionNames[i] + "." + parameters[i][j]] =
          parametersWithPrefix[i][j];
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
 * Extracts the parameters for the functions with the specified names.
 *
 * @param functionNames The names of the functions to extract the parameters
 *                      for.
 * @return              A vector containing a vector of parameters for each
 *                      specified fucntion.
 */
QVector<QVector<QString>> IndirectFitAnalysisTab::getFunctionParameters(
    QVector<QString> functionNames) const {
  QVector<QVector<QString>> parameters(functionNames.size());

  for (int i = 0; i < functionNames.size(); i++)
    parameters[i] = getFunctionParameters(functionNames[i]);
  return parameters;
}

/*
 * Extracts the parameters for the function with the specified name.
 *
 * @param functionName  The name of the function whose parameters to extract.
 * @return              The vector of parameters for the specified function.
 */
QVector<QString>
IndirectFitAnalysisTab::getFunctionParameters(QString functionName) const {
  auto func = FunctionFactory::Instance().createFunction(
      functionName.replace(" ", "").toStdString());
  return IndirectTab::convertStdStringVector(func->getParameterNames());
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
