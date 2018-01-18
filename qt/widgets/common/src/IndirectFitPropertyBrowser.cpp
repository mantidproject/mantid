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

  connect(this, SIGNAL(functionCleared()), this, SLOT(clearCustomFunctions()));

  initLayout(w);
}

/**
 * @return  The selected background function.
 */
IFunction_sptr IndirectFitPropertyBrowser::background() const {
  if (m_backgroundHandler != nullptr)
    return m_backgroundHandler->function()->clone();
  else
    return nullptr;
}

/**
 * @return  The function index of the selected background. -1 if no background
 *          is selected.
 */
int IndirectFitPropertyBrowser::backgroundIndex() const {
  if (m_backgroundHandler != nullptr)
    return m_backgroundHandler->functionPrefix().right(1).toInt();
  else
    return -1;
}

/**
 * @param   The function, whose function index to retrieve.
 * @return  The function index of the specified function in the browser.
 */
int IndirectFitPropertyBrowser::functionIndex(IFunction_sptr function) const {
  for (size_t i = 0u; i < compositeFunction()->nFunctions(); ++i) {
    if (compositeFunction()->getFunction(i) == function)
      return static_cast<int>(i);
  }
  return -1;
}

/**
 * @return  The selected fit type in the fit type combo box.
 */
QString IndirectFitPropertyBrowser::selectedFitType() const {
  const auto index = m_enumManager->value(m_functionsInComboBox);
  return m_enumManager->enumNames(m_functionsInComboBox)[index];
}

/**
 * @return  The name of the selected background function.
 */
QString IndirectFitPropertyBrowser::backgroundName() const {
  auto background = enumValue(m_backgroundSelection);
  if (background.isEmpty())
    return "None";
  else
    return background;
}

/**
 * @param functionName  The name of the function.
 * @return              The number of custom functions, with the specified name,
 *                      included in the selected model.
 */
size_t IndirectFitPropertyBrowser::numberOfCustomFunctions(
    const std::string &functionName) const {

  if (m_customFunctionCount.find(functionName) != m_customFunctionCount.end())
    return m_customFunctionCount.at(functionName);
  else
    return 0;
}

/**
 * @param functionName  The name of the function containing the parameter.
 * @param parameterName The name of the parameter whose value to retrieve.
 * @return              The value of the parameter with the specified name, in
 *                      the function with the specified name.
 */
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

/**
 * Sets the value of the parameter with the specified name, in the function with
 * the specified name.
 *
 * @param functionName  The name of the function containing the parameter.
 * @param parameterName The name of the parameter to set.
 * @param value         The value to set.
 */
void IndirectFitPropertyBrowser::setParameterValue(
    const std::string &functionName, const std::string &parameterName,
    double value) {
  const auto composite = compositeFunction();

  for (size_t i = 0u; i < composite->nFunctions(); ++i) {
    const auto function = composite->getFunction(i);

    if (function->name() == functionName)
      setParameterValue(function, parameterName, value);
  }

  updateParameters();
}

/**
 * Sets the value of the parameter with the specified name, in the specified
 * function.
 *
 * @param function      The function containing the parameter.
 * @param parameterName The name of the parameter to set.
 * @param value         The value to set.
 */
void IndirectFitPropertyBrowser::setParameterValue(
    IFunction_sptr function, const std::string &parameterName, double value) {
  if (function->hasParameter(parameterName)) {
    function->setParameter(parameterName, value);
    emit parameterChanged(function.get());
  }
}

/**
 * Sets the background to add to the selected model.
 *
 * @param backgroundName  The name of the background to add.
 */
void IndirectFitPropertyBrowser::setBackground(
    const std::string &backgroundName) {
  if (m_backgroundHandler != nullptr && backgroundIndex() >= 0)
    FitPropertyBrowser::removeFunction(m_backgroundHandler);

  if (backgroundName != "None")
    m_backgroundHandler = addFunction(backgroundName);
  else
    m_backgroundHandler = nullptr;
}

/**
 * Updates the values of the function parameters in this fit property browser,
 * using the specified map from parameter name to updated value.
 *
 * @param parameterValues A map from the name of a parameter to its updated
 *                        value.
 */
void IndirectFitPropertyBrowser::updateParameterValues(
    const QHash<QString, double> &parameterValues) {
  updateParameterValues(getHandler(), parameterValues);
}

/**
 * Updates the values of the parameters of the function held by the specified
 * property handler, using the specified map from parameter name to updated
 * value.
 *
 * @param functionHandler The property handler containing the function.
 * @param parameterValues A map from the name of a parameter to its updated
 *                        value.
 */
void IndirectFitPropertyBrowser::updateParameterValues(
    PropertyHandler *functionHandler,
    const QHash<QString, double> &parameterValues) {
  auto function = functionHandler->function();
  auto composite = boost::dynamic_pointer_cast<CompositeFunction>(function);

  if (composite) {
    if (composite->nFunctions() == 0)
      return;
    else if (composite->nFunctions() == 1)
      function = composite->getFunction(0);
  }

  for (const auto &parameterName : parameterValues.keys()) {
    const auto parameter = parameterName.toStdString();

    if (function->hasParameter(parameter))
      function->setParameter(parameter, parameterValues[parameterName]);
  }

  updateParameters();
}

/**
 * Sets the available background options in this indirect fit property browser.
 *
 * @param backgrounds A list of the names of available backgrouns to set.
 */
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

/**
 * Moves the functions attached to a custom function group, to the end of the
 * model.
 */
void IndirectFitPropertyBrowser::moveCustomFunctionsToEnd() {
  blockSignals(true);
  for (auto &handlerProperty : m_orderedFunctionGroups) {
    auto &handlers = m_functionHandlers[handlerProperty];

    for (int i = 0; i < handlers.size(); ++i) {
      auto &handler = handlers[i];
      const auto function = handler->function();

      if (handler->parentHandler() != nullptr) {
        handler->removeFunction();
        handler = addFunction(function->asString());
      }
    }
  }
  blockSignals(false);
}

/**
 * @param settingKey  The key of the boolean setting whose value to retrieve.
 * @return            The value of the boolean setting with the specified key.
 */
bool IndirectFitPropertyBrowser::boolSettingValue(
    const QString &settingKey) const {
  return m_boolManager->value(m_customSettings[settingKey]);
}

/**
 * @param settingKey  The key of the integer setting whose value to retrieve.
 * @return            The value of the integer setting with the specified key.
 */
int IndirectFitPropertyBrowser::intSettingValue(
    const QString &settingKey) const {
  return m_intManager->value(m_customSettings[settingKey]);
}

/**
 * @param settingKey  The key of the double setting whose value to retrieve.
 * @return            The value of the double setting with the specified key.
 */
double IndirectFitPropertyBrowser::doubleSettingValue(
    const QString &settingKey) const {
  return m_doubleManager->value(m_customSettings[settingKey]);
}

/**
 * @param settingKey  The key of the enum setting whose value to retrieve.
 * @return            The value of the enum setting with the specified key.
 */
QString
IndirectFitPropertyBrowser::enumSettingValue(const QString &settingKey) const {
  return m_enumManager->value(m_customSettings[settingKey]);
}

/**
 * Adds a boolean custom setting, with the specified key and display name.
 *
 * @param settingKey    The key of the boolean setting to add.
 * @param settingName   The display name of the boolean setting to add.
 * @param defaultValue  The default value of the boolean setting.
 */
void IndirectFitPropertyBrowser::addBoolCustomSetting(
    const QString &settingKey, const QString &settingName, bool defaultValue) {
  auto settingProperty = m_boolManager->addProperty(settingName);
  m_boolManager->setValue(settingProperty, defaultValue);
  addCustomSetting(settingKey, settingProperty);
}

/**
 * Adds an integer custom setting, with the specified key and display name.
 *
 * @param settingKey    The key of the integer setting to add.
 * @param settingName   The display name of the integer setting to add.
 * @param defaultValue  The default value of the integer setting.
 */
void IndirectFitPropertyBrowser::addIntCustomSetting(const QString &settingKey,
                                                     const QString &settingName,
                                                     int defaultValue) {
  auto settingProperty = m_intManager->addProperty(settingName);
  m_intManager->setValue(settingProperty, defaultValue);
  addCustomSetting(settingKey, settingProperty);
}

/**
 * Adds a double custom setting, with the specified key and display name.
 *
 * @param settingKey    The key of the double setting to add.
 * @param settingName   The display name of the double setting to add.
 * @param defaultValue  The default value of the double setting.
 */
void IndirectFitPropertyBrowser::addDoubleCustomSetting(
    const QString &settingKey, const QString &settingName,
    double defaultValue) {
  auto settingProperty = m_doubleManager->addProperty(settingName);
  m_doubleManager->setValue(settingProperty, defaultValue);
  addCustomSetting(settingKey, settingProperty);
}

/**
 * Adds an enum custom setting, with the specified key and display name.
 *
 * @param settingKey    The key of the enum setting to add.
 * @param settingName   The display name of the enum setting to add.
 * @param defaultValue  The default value of the enum setting.
 */
void IndirectFitPropertyBrowser::addEnumCustomSetting(
    const QString &settingKey, const QString &settingName,
    const QStringList &options) {
  auto settingProperty = m_enumManager->addProperty(settingName);
  m_enumManager->setEnumNames(settingProperty, options);
  addCustomSetting(settingKey, settingProperty);
}

/**
 * Adds a custom setting with the specified key to this indirect fit property
 * browser.
 *
 * @param settingKey      The key of the custom setting to add.
 * @param settingProperty The property to display in the fit property browser.
 */
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

/**
 * Adds an optional double custom setting, with the specified key and display
 * name.
 *
 * @param settingKey    The key of the optional double setting to add.
 * @param settingName   The display name of the optional double setting to add.
 * @param optionKey     The key of the setting specifying whether to use this
 *                      this optional setting.
 * @param optionName    The display name of the setting specifying whether to
 *                      use this optional setting.
 * @param defaultValue  The default value of the optional double setting.
 */
void IndirectFitPropertyBrowser::addOptionalDoubleSetting(
    const QString &settingKey, const QString &settingName,
    const QString &optionKey, const QString &optionName, bool enabled,
    double defaultValue) {
  auto settingProperty = m_doubleManager->addProperty(settingName);
  m_doubleManager->setValue(settingProperty, defaultValue);
  addOptionalSetting(settingKey, settingProperty, optionKey, optionName,
                     enabled);
}

/**
 * Adds an optional custom setting, with the specified key and display name.
 *
 * @param settingKey      The key of the optional double setting to add.
 * @param settingProperty The property to display in the fit property browser.
 * @param optionKey       The key of the setting specifying whether to use this
 *                        this optional setting.
 * @param optionName      The display name of the setting specifying whether to
 *                        use this optional setting.
 * @param defaultValue    The default value of the optional double setting.
 */
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

/**
 * Adds a check-box with the specified name, to this fit property browser, which
 * when checked adds the specified functions to the mode and when unchecked,
 * removes them.
 *
 * @param groupName     The name/label of the check-box to add.
 * @param functions     The functions to be added when the check-box is checked.
 * @param defaultValue  The default value of the check-box.
 */
void IndirectFitPropertyBrowser::addCheckBoxFunctionGroup(
    const QString &groupName, const std::vector<IFunction_sptr> &functions,
    bool defaultValue) {
  m_functionsAsCheckBox.insert(
      createFunctionGroupProperty(groupName, m_boolManager));
  addCustomFunctionGroup(groupName, functions);
}

/**
 * Adds a number spinner with the specified name, to this fit property browser,
 * which specifies how many multiples of the specified functions should be added
 * to the model.
 *
 * @param groupName     The name/label of the spinner to add.
 * @param functions     The functions to be added.
 * @param minimum       The minimum value of the spinner.
 * @param maximum       The maximum value of the spinner.
 * @param defaultValue  The default value of the spinner.
 */
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

/**
 * Adds an option with the specified name, to the fit type combo-box in this fit
 * property browser, which adds the specified functions to the model.
 *
 * @param groupName The name of the option to be added to the fit type
 *                  combo-box.
 * @param functions The functions added by the option.
 */
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

/**
 * Adds a custom function group to this fit property browser, with the specified
 * name and the associated specified functions.
 *
 * @param groupName The name of the function group.
 * @param functions The functions associated to the function group.
 */
void IndirectFitPropertyBrowser::addCustomFunctionGroup(
    const QString &groupName, const std::vector<IFunction_sptr> &functions) {
  m_groupToFunctionList[groupName] = functions;

  for (const auto &function : functions) {
    const auto functionName = function->name();
    if (m_customFunctionCount.find(functionName) == m_customFunctionCount.end())
      m_customFunctionCount[functionName] = 0;
  }
}

/**
 * Adds a specified multiple of the custom function group with the specified
 * name. Displays the functions in the specified property.
 *
 * @param prop      The property in which to display the functions.
 * @param groupName The name of the function group.
 * @param multiples The number of times to add the functions in the function
 *                  group.
 */
void IndirectFitPropertyBrowser::addCustomFunctions(QtProperty *prop,
                                                    const QString &groupName,
                                                    const int &multiples) {
  for (int i = 0; i < multiples; ++i)
    addCustomFunctions(prop, groupName);
}

/**
 * Adds the custom function group with the specified name. Displays the
 * functions in the specified property.
 *
 * @param prop      The property in which to display the functions.
 * @param groupName The name of the function group.
 */
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

/**
 * Clears all custom functions in this fit property browser.
 */
void IndirectFitPropertyBrowser::clearCustomFunctions() {
  for (const auto &prop : m_functionHandlers.keys()) {
    if (m_functionsAsCheckBox.contains(prop))
      m_boolManager->setValue(prop, false);
    else if (m_functionsAsSpinner.contains(prop))
      m_intManager->setValue(prop, 0);
    m_functionHandlers[prop].clear();
  }

  setBackground("None");
  m_enumManager->setValue(m_backgroundSelection, 0);
  m_enumManager->setValue(m_functionsInComboBox, 0);
}

/**
 * Clears all custom functions in the specified property.
 *
 * @param prop  The property to clear of custom functions.
 */
void IndirectFitPropertyBrowser::clearCustomFunctions(QtProperty *prop) {
  blockSignals(true);
  for (const auto &functionHandler : m_functionHandlers[prop]) {

    if (functionHandler->parentHandler() != nullptr) {
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

/**
 * Creates a custom function group property, to use for displaying an
 * option for selecting the function group.
 *
 * @param groupName       The name of the function group.
 * @param propertyManager The property manager, to be used to create
 *                        the property.
 * @param atFront         If true, moves the created function group
 *                        property to precede others in this indirect
 *                        fit property.
 */
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

/**
 * Removes the function, associated to the specified handler, from this indirect
 * fit property browser.
 *
 * @param handler The handler containing the function to be removed.
 */
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

  if (handler->parentHandler() != nullptr)
    FitPropertyBrowser::removeFunction(handler);
}

/**
 * Schedules a fit.
 */
void IndirectFitPropertyBrowser::fit() { emit fitScheduled(); }

/**
 * Schedules a sequential fit.
 */
void IndirectFitPropertyBrowser::sequentialFit() {
  emit sequentialFitScheduled();
}

/**
 * Called when an enum value changes in this indirect fit property browser.
 *
 * @param prop  The property containing the enum value which was changed.
 */
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

/**
 * Called when a boolean value changes in this indirect fit property browser.
 *
 * @param prop  The property containing the boolean value which was changed.
 */
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

/**
* Called when an integer value changes in this indirect fit property browser.
*
* @param prop  The property containing the integer value which was changed.
*/
void IndirectFitPropertyBrowser::intChanged(QtProperty *prop) {

  if (m_functionsAsSpinner.contains(prop)) {
    clearCustomFunctions(prop);
    addCustomFunctions(prop, prop->propertyName(), m_intManager->value(prop));
  } else if (m_customSettings.values().contains(prop)) {
    emit customIntChanged(prop->propertyName(), m_intManager->value(prop));
  }
  FitPropertyBrowser::intChanged(prop);
}

/**
 * @param prop  The property whose enum value to extract.
 * @return      The enum value of the specified property.
 */
QString IndirectFitPropertyBrowser::enumValue(QtProperty *prop) const {
  const auto values = m_enumManager->enumNames(prop);
  if (values.isEmpty())
    return "";
  const auto selectedIndex = m_enumManager->value(prop);
  return values[selectedIndex];
}

} // namespace MantidWidgets
} // namespace MantidQt
