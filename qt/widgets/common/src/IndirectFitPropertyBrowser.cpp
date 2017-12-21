#include "MantidQtWidgets/Common/IndirectFitPropertyBrowser.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidQtWidgets/Common/PropertyHandler.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/StringEditorFactory.h"

// Suppress a warning coming out of code that isn't ours
#if defined(__INTEL_COMPILER)
#pragma warning disable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qteditorfactory.h"
#if defined(__INTEL_COMPILER)
#pragma warning enable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
#endif

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidAPI/IPeakFunction.h"

#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qttreepropertybrowser.h"

#include <Poco/ActiveResult.h>

#include <QAction>
#include <QFormLayout>
#include <QMessageBox>
#include <QSettings>

#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QSplitter>

#include <QMenu>
#include <QSignalMapper>

#include <QCheckBox>

namespace MantidQt {
namespace MantidWidgets {

/**
 * Constructor
 * @param parent :: The parent widget - must be an ApplicationWindow
 * @param mantidui :: The UI form for MantidPlot
 */
IndirectFitPropertyBrowser::IndirectFitPropertyBrowser(QWidget *parent,
                                                       QObject *mantidui)
    : FitPropertyBrowser(parent, mantidui), m_backgroundHandler(nullptr) {}

void IndirectFitPropertyBrowser::init() {
  QWidget *w = new QWidget(this);

  QSettings settings;
  settings.beginGroup("Mantid/FitBrowser");

  m_customFunctionGroups =
      m_groupManager->addProperty("Custom Function Groups");

  m_customSettingsGroup = m_groupManager->addProperty("Custom Settings");

  /* Create background selection */
  m_backgroundSelection = m_enumManager->addProperty("Background");

  /* Create function group */
  QtProperty *functionsGroup = m_groupManager->addProperty("Functions");

  /* Create fitting range group */
  QtProperty *fittingRangeGroup = m_groupManager->addProperty("Fitting Range");
  m_startX = addDoubleProperty("StartX");
  m_endX = addDoubleProperty("EndX");
  fittingRangeGroup->addSubProperty(m_startX);
  fittingRangeGroup->addSubProperty(m_endX);

  /* Create input - output properties */
  QtProperty *settingsGroup = m_groupManager->addProperty("Settings");

  const auto backgroundSelection =
      m_enumManager->addProperty("Background Type");

  m_minimizer = m_enumManager->addProperty("Minimizer");
  m_minimizers << "Levenberg-Marquardt"
               << "Levenberg-MarquardtMD"
               << "Trust Region"
               << "Simplex"
               << "FABADA"
               << "Conjugate gradient (Fletcher-Reeves imp.)"
               << "Conjugate gradient (Polak-Ribiere imp.)"
               << "BFGS"
               << "Damped GaussNewton";

  m_ignoreInvalidData = m_boolManager->addProperty("Ignore invalid data");
  setIgnoreInvalidData(settings.value("Ignore invalid data", false).toBool());

  m_enumManager->setEnumNames(m_minimizer, m_minimizers);
  m_costFunction = m_enumManager->addProperty("Cost function");
  m_costFunctions << "Least squares"
                  << "Rwp"
                  << "Unweighted least squares";
  m_enumManager->setEnumNames(m_costFunction, m_costFunctions);
  m_maxIterations = m_intManager->addProperty("Max Iterations");
  m_intManager->setValue(m_maxIterations,
                         settings.value("Max Iterations", 500).toInt());

  m_peakRadius = m_intManager->addProperty("Peak Radius");
  m_intManager->setValue(m_peakRadius,
                         settings.value("Peak Radius", 0).toInt());

  m_plotDiff = m_boolManager->addProperty("Plot Difference");
  bool plotDiff = settings.value("Plot Difference", QVariant(true)).toBool();
  m_boolManager->setValue(m_plotDiff, plotDiff);

  m_convolveMembers = m_boolManager->addProperty("Convolve Composite Members");

  m_showParamErrors = m_boolManager->addProperty("Show Parameter Errors");
  bool showParamErrors =
      settings.value(m_showParamErrors->propertyName(), false).toBool();
  m_boolManager->setValue(m_showParamErrors, showParamErrors);
  m_parameterManager->setErrorsEnabled(showParamErrors);

  m_evaluationType = m_enumManager->addProperty("Evaluate Function As");
  m_evaluationType->setToolTip(
      "Consider using Histogram fit which may produce more accurate results.");
  m_evaluationTypes << "CentrePoint"
                    << "Histogram";
  m_enumManager->setEnumNames(m_evaluationType, m_evaluationTypes);
  int evaluationType =
      settings.value(m_evaluationType->propertyName(), 0).toInt();
  m_enumManager->setValue(m_evaluationType, evaluationType);

  m_xColumn = m_columnManager->addProperty("XColumn");
  m_yColumn = m_columnManager->addProperty("YColumn");
  m_errColumn = m_columnManager->addProperty("ErrColumn");

  settingsGroup->addSubProperty(m_minimizer);
  settingsGroup->addSubProperty(m_ignoreInvalidData);
  settingsGroup->addSubProperty(m_costFunction);
  settingsGroup->addSubProperty(m_maxIterations);
  settingsGroup->addSubProperty(m_peakRadius);
  settingsGroup->addSubProperty(m_plotDiff);
  settingsGroup->addSubProperty(m_showParamErrors);
  settingsGroup->addSubProperty(m_evaluationType);

  /* Create editors and assign them to the managers */
  createEditors(w);

  updateDecimals();

  m_browser->addProperty(m_customFunctionGroups);
  m_browser->addProperty(m_backgroundSelection);
  m_browser->addProperty(fittingRangeGroup);
  m_functionsGroup = m_browser->addProperty(functionsGroup);
  m_settingsGroup = m_browser->addProperty(settingsGroup);

  initLayout(w);
  QStringList backgrounds = {"None"};
  backgrounds.append(registeredBackgrounds());
  m_enumManager->setEnumNames(m_backgroundSelection, backgrounds);
}

Mantid::API::IFunction_sptr IndirectFitPropertyBrowser::background() const {
  if (m_backgroundHandler != nullptr)
    return m_backgroundHandler->function()->clone();
  else
    return nullptr;
}

Mantid::API::IFunction_sptr IndirectFitPropertyBrowser::model() const {
  auto model = boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(
      compositeFunction()->clone());

  if (model) {
    const auto backgroundFunction = background();

    for (size_t i = 0u; i < model->nFunctions(); ++i) {

      if (model->getFunction(i) == backgroundFunction) {
        model->removeFunction(i);
        return model;
      }
    }
  }
  return model;
}

size_t IndirectFitPropertyBrowser::numberOfCustomFunctions(
    const std::string &functionName) const {

  if (m_customFunctionCount.find(functionName) != m_customFunctionCount.end())
    return m_customFunctionCount.at(functionName);
  else
    return 0;
}

void IndirectFitPropertyBrowser::updateParameterValues(
    const QHash<QString, double> &parameterValues) {
  updateParameterValues(getHandler(), parameterValues);
}

void IndirectFitPropertyBrowser::updateParameterValue(
    const QString &parameterName, const double &parameterValue) {
  const auto &function = getHandler()->function().get();
  function->setParameter(parameterName.toStdString(), parameterValue);
  getHandler()->updateParameters();
  emit parameterChanged(function);
}

void IndirectFitPropertyBrowser::updateParameterValues(
    PropertyHandler *functionHandler,
    const QHash<QString, double> &parameterValues) {
  const auto &function = functionHandler->function().get();

  for (const auto &parameterName : parameterValues.keys()) {

    if (parameterName.contains("."))
      function->setParameter(parameterName.toStdString(),
                             parameterValues[parameterName]);
    else
      function->setParameter("f0." + parameterName.toStdString(),
                             parameterValues[parameterName]);
  }
  functionHandler->updateParameters();
  emit parameterChanged(function);
}

void IndirectFitPropertyBrowser::setBackgroundOptions(
    const QStringList &backgrounds) {
  m_enumManager->setEnumNames(m_backgroundSelection, backgrounds);
}

bool IndirectFitPropertyBrowser::boolSettingValue(
    const QString &settingKey) const {
  return m_boolManager->value(m_customSettings[settingKey]);
}

int IndirectFitPropertyBrowser::intSettingValue(
    const QString &settingKey) const {
  return m_intManager->value(m_customSettings[settingKey]);
}

double IndirectFitPropertyBrowser::doubleSettingValue(
    const QString &settingKey) const {
  return m_doubleManager->value(m_customSettings[settingKey]);
}

QString
IndirectFitPropertyBrowser::enumSettingValue(const QString &settingKey) const {
  return m_enumManager->value(m_customSettings[settingKey]);
}

void IndirectFitPropertyBrowser::addBoolCustomSetting(
    const QString &settingKey, const QString &settingName, bool defaultValue) {
  auto settingProperty = m_boolManager->addProperty(settingName);
  m_boolManager->setValue(settingProperty, defaultValue);
  addCustomSetting(settingKey, settingProperty);
}

void IndirectFitPropertyBrowser::addIntCustomSetting(const QString &settingKey,
                                                     const QString &settingName,
                                                     int defaultValue) {
  auto settingProperty = m_intManager->addProperty(settingName);
  m_intManager->setValue(settingProperty, defaultValue);
  addCustomSetting(settingKey, settingProperty);
}

void IndirectFitPropertyBrowser::addDoubleCustomSetting(
    const QString &settingKey, const QString &settingName,
    double defaultValue) {
  auto settingProperty = m_doubleManager->addProperty(settingName);
  m_doubleManager->setValue(settingProperty, defaultValue);
  addCustomSetting(settingKey, settingProperty);
}

void IndirectFitPropertyBrowser::addEnumCustomSetting(
    const QString &settingKey, const QString &settingName,
    const QStringList &options) {
  auto settingProperty = m_enumManager->addProperty(settingName);
  m_enumManager->setEnumNames(settingProperty, options);
  addCustomSetting(settingKey, settingProperty);
}

void IndirectFitPropertyBrowser::addCustomSetting(const QString &settingKey,
                                                  QtProperty *settingProperty) {
  m_customSettingsGroup->addSubProperty(settingProperty);

  if (m_customSettings.isEmpty())
    m_browser->insertProperty(m_customSettingsGroup, m_backgroundSelection);

  m_customSettings[settingKey] = settingProperty;
}

void IndirectFitPropertyBrowser::addCheckBoxFunctionGroup(
    const QString &groupName,
    const std::vector<Mantid::API::IFunction_sptr> &functions,
    bool defaultValue) {
  m_functionsAsCheckBox.insert(
      createFunctionGroupProperty(groupName, m_boolManager));
  addCustomFunctionGroup(groupName, functions);
}

void IndirectFitPropertyBrowser::addSpinnerFunctionGroup(
    const QString &groupName,
    const std::vector<Mantid::API::IFunction_sptr> &functions, int minimum,
    int maximum, int defaultValue) {
  auto intProperty = createFunctionGroupProperty(groupName, m_intManager);
  m_intManager->setMinimum(intProperty, minimum);
  m_intManager->setMaximum(intProperty, maximum);
  m_intManager->setValue(intProperty, defaultValue);
  m_functionsAsSpinner.insert(intProperty);
  addCustomFunctionGroup(groupName, functions);
}

void IndirectFitPropertyBrowser::addComboBoxFunctionGroup(
    const QString &groupName,
    const std::vector<Mantid::API::IFunction_sptr> &functions) {
  if (!m_functionsInComboBox) {
    m_functionsInComboBox =
        createFunctionGroupProperty("Function Group", m_enumManager);
    m_groupToFunctionList["None"] = {};
  }

  auto groupNames = m_enumManager->enumNames(m_functionsInComboBox)
                    << groupName;
  groupNames.prepend("None");
  m_enumManager->setEnumNames(m_functionsInComboBox, groupNames);
  addCustomFunctionGroup(groupName, functions);
}

void IndirectFitPropertyBrowser::addCustomFunctionGroup(
    const QString &groupName,
    const std::vector<Mantid::API::IFunction_sptr> &functions) {
  m_groupToFunctionList[groupName] = functions;

  for (const auto &function : functions) {
    const auto functionName = function->name();
    if (m_customFunctionCount.find(functionName) == m_customFunctionCount.end())
      m_customFunctionCount[functionName] = 0;
  }
}

void IndirectFitPropertyBrowser::addCustomFunctions(QtProperty *prop,
                                                    const QString &groupName,
                                                    const int &multiples) {
  for (int i = 0; i < multiples; ++i)
    addCustomFunctions(prop, groupName);
}

void IndirectFitPropertyBrowser::addCustomFunctions(QtProperty *prop,
                                                    const QString &groupName) {
  if (!m_functionHandlers.contains(prop))
    m_functionHandlers.insert(prop, QVector<PropertyHandler *>());

  for (const auto &function : m_groupToFunctionList[groupName]) {
    m_functionHandlers[prop] << addFunction(function->asString());
    m_customFunctionCount[function->name()] += 1;
  }
}

void IndirectFitPropertyBrowser::clearCustomFunctions(QtProperty *prop) {
  for (const auto &functionHandler : m_functionHandlers[prop]) {
    removeFunction(functionHandler);
    m_customFunctionCount[functionHandler->function()->name()] -= 1;
  }
  m_functionHandlers[prop].clear();
}

QtProperty *IndirectFitPropertyBrowser::createFunctionGroupProperty(
    const QString &groupName, QtAbstractPropertyManager *propertyManager) {
  auto functionProperty = propertyManager->addProperty(groupName);
  m_customFunctionGroups->addSubProperty(functionProperty);
  return functionProperty;
}

void IndirectFitPropertyBrowser::removeFunction(PropertyHandler *handler) {

  for (const auto &prop : m_functionHandlers.keys()) {
    int i = m_functionHandlers[prop].indexOf(handler);

    if (i >= 0) {
      m_functionHandlers[prop].remove(i);

      if (m_functionsAsSpinner.contains(prop))
        m_intManager->setValue(prop, m_intManager->value(prop) - 1);
    }
  }
  FitPropertyBrowser::removeFunction(handler);
}

void IndirectFitPropertyBrowser::fit() { emit fitScheduled(); }

void IndirectFitPropertyBrowser::sequentialFit() {
  emit sequentialFitScheduled();
}

void IndirectFitPropertyBrowser::enumChanged(QtProperty *prop) {

  if (prop == m_functionsInComboBox) {
    clearCustomFunctions(prop);
    addCustomFunctions(prop, enumValue(prop));
  } else if (prop == m_backgroundSelection) {

    if (m_backgroundHandler != nullptr)
      removeFunction(m_backgroundHandler);

    const auto backgroundName = enumValue(prop).toStdString();
    if (backgroundName != "None")
      m_backgroundHandler = addFunction(backgroundName);
    else
      m_backgroundHandler = nullptr;
  } else if (m_customSettings.values().contains(prop)) {
    emit customEnumChanged(prop->propertyName(), enumValue(prop));
  }
  FitPropertyBrowser::enumChanged(prop);
}

void IndirectFitPropertyBrowser::boolChanged(QtProperty *prop) {

  if (m_functionsAsCheckBox.contains(prop)) {
    if (m_boolManager->value(prop))
      addCustomFunctions(prop, prop->propertyName());
    else
      clearCustomFunctions(prop);
  } else if (m_customSettings.values().contains(prop)) {
    emit customBoolChanged(prop->propertyName(), m_boolManager->value(prop));
  }
  FitPropertyBrowser::boolChanged(prop);
}

void IndirectFitPropertyBrowser::intChanged(QtProperty *prop) {

  if (m_functionsAsSpinner.contains(prop)) {
    clearCustomFunctions(prop);
    addCustomFunctions(prop, prop->propertyName(), m_intManager->value(prop));
  } else if (m_customSettings.values().contains(prop)) {
    emit customIntChanged(prop->propertyName(), m_intManager->value(prop));
  }
  FitPropertyBrowser::intChanged(prop);
}

QString IndirectFitPropertyBrowser::enumValue(QtProperty *prop) const {
  const auto values = m_enumManager->enumNames(prop);
  const auto selectedIndex = m_enumManager->value(prop);
  return values[selectedIndex];
}

} // namespace MantidWidgets
} // namespace MantidQt
