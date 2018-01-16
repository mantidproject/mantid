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

using namespace Mantid::API;

namespace MantidQt {
namespace MantidWidgets {

/**
 * Constructor
 * @param parent :: The parent widget - must be an ApplicationWindow
 * @param mantidui :: The UI form for MantidPlot
 */
IndirectFitPropertyBrowser::IndirectFitPropertyBrowser(QWidget *parent,
                                                       QObject *mantidui)
    : FitPropertyBrowser(parent, mantidui), m_functionsInComboBox(nullptr),
      m_backgroundHandler(nullptr) {}

void IndirectFitPropertyBrowser::init() {
  QWidget *w = new QWidget(this);
  setFeatures(QDockWidget::DockWidgetFloatable |
              QDockWidget::DockWidgetMovable);

  QSettings settings;
  settings.beginGroup("Mantid/FitBrowser");

  m_customFunctionGroups =
      m_groupManager->addProperty("Custom Function Groups");

  m_backgroundGroup = m_groupManager->addProperty("Background");

  m_customSettingsGroup = m_groupManager->addProperty("Custom Settings");

  /* Create background selection */
  m_backgroundSelection = m_enumManager->addProperty("Background Type");
  m_backgroundGroup->addSubProperty(m_backgroundSelection);

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
  settingsGroup->addSubProperty(m_convolveMembers);
  settingsGroup->addSubProperty(m_showParamErrors);
  settingsGroup->addSubProperty(m_evaluationType);

  /* Create editors and assign them to the managers */
  createEditors(w);

  updateDecimals();

  m_browser->addProperty(m_customFunctionGroups);
  m_browser->addProperty(fittingRangeGroup);
  m_functionsGroup = m_browser->addProperty(functionsGroup);
  m_settingsGroup = m_browser->addProperty(settingsGroup);

  initLayout(w);
}

IFunction_sptr IndirectFitPropertyBrowser::background() const {
  if (m_backgroundHandler != nullptr)
    return m_backgroundHandler->function()->clone();
  else
    return nullptr;
}

int IndirectFitPropertyBrowser::backgroundIndex() const {
  if (m_backgroundHandler != nullptr)
    return functionIndex(m_backgroundHandler->function());
  else
    return -1;
}

int IndirectFitPropertyBrowser::functionIndex(IFunction_sptr function) const {
  for (size_t i = 0u; i < compositeFunction()->nFunctions(); ++i) {
    if (compositeFunction()->getFunction(i) == function)
      return static_cast<int>(i);
  }
  return -1;
}

QString IndirectFitPropertyBrowser::selectedFitType() const {
  const auto index = m_enumManager->value(m_functionsInComboBox);
  return m_enumManager->enumNames(m_functionsInComboBox)[index];
}

QString IndirectFitPropertyBrowser::backgroundName() const {
  auto background = enumValue(m_backgroundSelection);
  if (background.isEmpty())
    return "None";
  else
    return background;
}

QString IndirectFitPropertyBrowser::backgroundPrefix() const {
  return m_backgroundHandler->functionPrefix();
}

size_t IndirectFitPropertyBrowser::numberOfCustomFunctions(
    const std::string &functionName) const {

  if (m_customFunctionCount.find(functionName) != m_customFunctionCount.end())
    return m_customFunctionCount.at(functionName);
  else
    return 0;
}

double IndirectFitPropertyBrowser::parameterValue(
    const std::string &functionName, const std::string &parameterName) const {
  const auto composite = compositeFunction();

  for (size_t i = 0u; i < composite->nFunctions(); ++i) {
    const auto function = composite->getFunction(i);

    if (function->name() == functionName)
      return function->getParameter(parameterName);
  }
  return 0.0;
}

void IndirectFitPropertyBrowser::setParameterValue(
    const std::string &functionName, const std::string &parameterName,
    double value) {
  const auto composite = compositeFunction();

  for (size_t i = 0u; i < composite->nFunctions(); ++i) {
    const auto function = composite->getFunction(i);

    if (function->name() == functionName &&
        function->hasParameter(parameterName)) {
      function->setParameter(parameterName, value);
      emit parameterChanged(function.get());
    }
  }
  updateParameters();
}

void IndirectFitPropertyBrowser::setBackground(
    const std::string &backgroundName) {
  if (m_backgroundHandler != nullptr && backgroundIndex() >= 0)
    FitPropertyBrowser::removeFunction(m_backgroundHandler);

  if (backgroundName != "None") {
    m_backgroundHandler = addFunction(backgroundName);
  } else
    m_backgroundHandler = nullptr;
}

void IndirectFitPropertyBrowser::updateParameterValues(
    const QHash<QString, double> &parameterValues) {
  updateParameterValues(getHandler(), parameterValues);
}

void IndirectFitPropertyBrowser::updateParameterValues(
    PropertyHandler *functionHandler,
    const QHash<QString, double> &parameterValues) {
  const auto &function = functionHandler->function().get();

  for (const auto &parameterName : parameterValues.keys()) {
    const auto parameter = parameterName.toStdString();
    const auto &parameterValue = parameterValues[parameterName];

    if (parameterName.contains(".") && function->hasParameter(parameter))
      function->setParameter(parameter, parameterValue);
    else if (function->hasParameter("f0." + parameter))
      function->setParameter("f0." + parameter, parameterValue);
  }

  getHandler()->updateParameters();
  emit parameterChanged(function);
}

void IndirectFitPropertyBrowser::setBackgroundOptions(
    const QStringList &backgrounds) {
  const auto currentlyHidden =
      m_enumManager->enumNames(m_backgroundSelection).isEmpty();
  const auto doHide = backgrounds.isEmpty();

  if (doHide && !currentlyHidden)
    m_browser->removeProperty(m_backgroundGroup);
  else if (!doHide && currentlyHidden)
    m_browser->insertProperty(m_backgroundGroup, m_customFunctionGroups);

  m_enumManager->setEnumNames(m_backgroundSelection, backgrounds);
}

void IndirectFitPropertyBrowser::moveCustomFunctionsToEnd() {
  blockSignals(true);
  for (auto &handlerProperty : m_orderedFunctionGroups) {
    auto &handlers = m_functionHandlers[handlerProperty];

    for (int i = 0; i < handlers.size(); ++i) {
      auto &handler = handlers[i];
      const auto function = handler->function();
      handler->removeFunction();
      handler = addFunction(function->asString());
    }
  }
  blockSignals(false);
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

  if (m_customSettings.isEmpty()) {
    if (m_enumManager->enumNames(m_backgroundSelection).isEmpty())
      m_browser->insertProperty(m_customSettingsGroup, m_customFunctionGroups);
    else
      m_browser->insertProperty(m_customSettingsGroup, m_backgroundGroup);
  }

  m_customSettings[settingKey] = settingProperty;
}

void IndirectFitPropertyBrowser::addOptionalDoubleSetting(
    const QString &settingKey, const QString &settingName,
    const QString &optionKey, const QString &optionName, bool enabled,
    double defaultValue) {
  auto settingProperty = m_doubleManager->addProperty(settingName);
  m_doubleManager->setValue(settingProperty, defaultValue);
  addOptionalSetting(settingKey, settingProperty, optionKey, optionName,
                     enabled);
}

void IndirectFitPropertyBrowser::addOptionalSetting(const QString &settingKey,
                                                    QtProperty *settingProperty,
                                                    const QString &optionKey,
                                                    const QString &optionName,
                                                    bool enabled) {
  auto optionProperty = m_boolManager->addProperty(optionName);
  m_customSettingsGroup->addSubProperty(optionProperty);
  m_optionProperties.insert(optionProperty);
  m_optionalProperties[optionProperty] = settingProperty;
  m_customSettings[optionKey] = optionProperty;
  m_customSettings[settingKey] = settingProperty;

  if (enabled)
    optionProperty->addSubProperty(settingProperty);
}

void IndirectFitPropertyBrowser::addCheckBoxFunctionGroup(
    const QString &groupName, const std::vector<IFunction_sptr> &functions,
    bool defaultValue) {
  m_functionsAsCheckBox.insert(
      createFunctionGroupProperty(groupName, m_boolManager));
  addCustomFunctionGroup(groupName, functions);
}

void IndirectFitPropertyBrowser::addSpinnerFunctionGroup(
    const QString &groupName, const std::vector<IFunction_sptr> &functions,
    int minimum, int maximum, int defaultValue) {
  auto intProperty = createFunctionGroupProperty(groupName, m_intManager);
  m_intManager->setMinimum(intProperty, minimum);
  m_intManager->setMaximum(intProperty, maximum);
  m_intManager->setValue(intProperty, defaultValue);
  m_functionsAsSpinner.insert(intProperty);
  addCustomFunctionGroup(groupName, functions);
}

void IndirectFitPropertyBrowser::addComboBoxFunctionGroup(
    const QString &groupName, const std::vector<IFunction_sptr> &functions) {
  if (m_functionsInComboBox == nullptr) {
    m_functionsInComboBox =
        createFunctionGroupProperty("Fit Type", m_enumManager, true);
    m_groupToFunctionList["None"] = {};
    m_enumManager->setEnumNames(m_functionsInComboBox, {"None"});
  }

  auto groupNames = m_enumManager->enumNames(m_functionsInComboBox)
                    << groupName;
  m_enumManager->setEnumNames(m_functionsInComboBox, groupNames);
  addCustomFunctionGroup(groupName, functions);
}

void IndirectFitPropertyBrowser::addCustomFunctionGroup(
    const QString &groupName, const std::vector<IFunction_sptr> &functions) {
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

  blockSignals(true);
  for (const auto &function : m_groupToFunctionList[groupName]) {
    m_functionHandlers[prop] << addFunction(function->asString());
    m_customFunctionCount[function->name()] += 1;
  }
  blockSignals(false);

  emit functionChanged();
}

void IndirectFitPropertyBrowser::clearCustomFunctions(QtProperty *prop) {
  blockSignals(true);
  for (const auto &functionHandler : m_functionHandlers[prop]) {
    if (functionIndex(functionHandler->function()) >= 0) {
      FitPropertyBrowser::removeFunction(functionHandler);
      m_customFunctionCount[functionHandler->function()->name()] -= 1;
    }
  }
  blockSignals(false);
  m_functionHandlers[prop].clear();

  emit removePlotSignal(getHandler());
  emit functionRemoved();
  emit functionChanged();
}

QtProperty *IndirectFitPropertyBrowser::createFunctionGroupProperty(
    const QString &groupName, QtAbstractPropertyManager *propertyManager,
    bool atFront) {
  auto functionProperty = propertyManager->addProperty(groupName);

  if (atFront && !m_customFunctionGroups->subProperties().isEmpty()) {
    auto firstProperty = m_customFunctionGroups->subProperties().first();
    m_customFunctionGroups->insertSubProperty(functionProperty, firstProperty);
    m_customFunctionGroups->removeSubProperty(firstProperty);
    m_customFunctionGroups->insertSubProperty(firstProperty, functionProperty);
  } else {
    m_customFunctionGroups->addSubProperty(functionProperty);
  }
  m_orderedFunctionGroups.append(functionProperty);
  return functionProperty;
}

void IndirectFitPropertyBrowser::removeFunction(PropertyHandler *handler) {

  for (const auto &prop : m_functionHandlers.keys()) {
    auto &functionHandlers = m_functionHandlers[prop];
    int i = functionHandlers.indexOf(handler);

    if (i >= 0) {
      functionHandlers.remove(i);
      m_customFunctionCount[handler->function()->name()] -= 1;

      if (m_functionsAsSpinner.contains(prop))
        m_intManager->setValue(prop, m_intManager->value(prop) - 1);
      else if (m_functionsAsCheckBox.contains(prop))
        m_boolManager->setValue(prop, false);
      else if (prop == m_functionsInComboBox)
        m_enumManager->setValue(m_functionsInComboBox, 0);
    }
  }

  if (handler == m_backgroundHandler)
    m_enumManager->setValue(m_backgroundSelection, 0);

  if (functionIndex(handler->function()) >= 0)
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
    setBackground(enumValue(prop).toStdString());
  } else if (m_customSettings.values().contains(prop)) {
    emit customEnumChanged(prop->propertyName(), enumValue(prop));
  }
  FitPropertyBrowser::enumChanged(prop);
}

void IndirectFitPropertyBrowser::boolChanged(QtProperty *prop) {
  const auto propertyName = prop->propertyName();

  if (m_optionProperties.contains(prop)) {
    if (m_boolManager->value(prop))
      prop->addSubProperty(m_optionalProperties[prop]);
    else
      prop->removeSubProperty(m_optionalProperties[prop]);
  }

  if (m_functionsAsCheckBox.contains(prop)) {
    if (m_boolManager->value(prop))
      addCustomFunctions(prop, propertyName);
    else
      clearCustomFunctions(prop);
  } else if (m_customSettings.values().contains(prop)) {
    emit customBoolChanged(propertyName, m_boolManager->value(prop));
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
  if (values.isEmpty())
    return "";
  const auto selectedIndex = m_enumManager->value(prop);
  return values[selectedIndex];
}

} // namespace MantidWidgets
} // namespace MantidQt
