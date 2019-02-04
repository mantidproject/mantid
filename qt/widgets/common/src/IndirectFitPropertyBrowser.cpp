// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/IndirectFitPropertyBrowser.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/PropertyHandler.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/StringEditorFactory.h"
#include "MantidQtWidgets/Common/SignalBlocker.h"

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
#include "MantidAPI/FrameworkManager.h"

#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qttreepropertybrowser.h"

#include <QAction>
#include <QFormLayout>
#include <QMessageBox>
#include <QSettings>

#include <QLabel>
#include <QLayout>
#include <QPushButton>

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

  m_workspaceIndex = m_intManager->addProperty("Workspace Index");

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

  connect(this, SIGNAL(visibilityChanged(bool)), this,
          SLOT(browserVisibilityChanged(bool)));
  connect(this, SIGNAL(customSettingChanged(QtProperty *)), this,
          SLOT(customChanged(QtProperty *)));

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
 * @return  The function index of the selected background. None if no background
 *          is selected.
 */
boost::optional<size_t> IndirectFitPropertyBrowser::backgroundIndex() const {
  if (m_backgroundHandler != nullptr) {
    const auto prefix = m_backgroundHandler->functionPrefix();

    if (!prefix.endsWith("-1"))
      return prefix.right(1).toInt();
  }
  return boost::none;
}

/**
 * @param function  The function, whose function index to retrieve.
 * @return          The function index of the specified function in the browser.
 */
boost::optional<size_t>
IndirectFitPropertyBrowser::functionIndex(IFunction_sptr function) const {
  for (size_t i = 0u; i < compositeFunction()->nFunctions(); ++i) {
    if (compositeFunction()->getFunction(i) == function)
      return i;
  }
  return boost::none;
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
 * @return  A map from parameter name to a tie expression.
 */
QHash<QString, QString> IndirectFitPropertyBrowser::getTies() const {
  QHash<QString, QString> ties;
  getCompositeTies(getHandler(), ties);
  return ties;
}

/**
 * @param handler A property handler, holding a composite function whose ties to
 *                retrieve.
 * @param ties    The map in which to store the tie expression under the tied
 *                parameter.
 */
void IndirectFitPropertyBrowser::getCompositeTies(
    PropertyHandler *handler, QHash<QString, QString> &ties) const {
  for (size_t i = 0u; i < handler->cfun()->nFunctions(); ++i) {
    auto nextHandler = handler->getHandler(i);
    if (nextHandler->cfun())
      getCompositeTies(nextHandler, ties);
    else
      getTies(nextHandler, ties);
  }
}

/**
 * @param handler A property handler, holding a function whose ties to
 *                retrieve.
 * @param ties    The map in which to store the tie expression under the tied
 *                parameter.
 */
void IndirectFitPropertyBrowser::getTies(PropertyHandler *handler,
                                         QHash<QString, QString> &ties) const {
  const auto prefix = handler->functionPrefix() + ".";
  auto tieProperties = handler->getTies();
  for (const auto parameter : tieProperties.keys())
    ties[prefix + parameter] = m_stringManager->value(tieProperties[parameter]);
}

/**
 * @param functionName  The name of the function.
 * @return              The number of custom functions, with the specified name,
 *                      included in the selected model.
 */
size_t IndirectFitPropertyBrowser::numberOfCustomFunctions(
    const std::string &functionName) const {
  auto count = m_customFunctionCount.find(functionName);
  if (count != m_customFunctionCount.end())
    return count->second;
  return 0;
}

/**
 * @param functionName  The name of the function containing the parameter.
 * @param parameterName The name of the parameter whose value to retrieve.
 * @return              All values of the parameter with the specified name, in
 *                      the function with the specified name.
 */
std::vector<double> IndirectFitPropertyBrowser::parameterValue(
    const std::string &functionName, const std::string &parameterName) const {
  std::vector<double> values;
  const auto composite = compositeFunction();

  for (size_t i = 0u; i < composite->nFunctions(); ++i) {
    const auto function = composite->getFunction(i);

    if (function->name() == functionName)
      values.emplace_back(function->getParameter(parameterName));
  }
  return values;
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
  if (m_backgroundHandler != nullptr && backgroundIndex()) {
    MantidQt::API::SignalBlocker<QObject> blocker(this);
    FitPropertyBrowser::removeFunction(m_backgroundHandler);
  }

  if (backgroundName != "None") {
    MantidQt::API::SignalBlocker<QObject> blocker(this);
    m_backgroundHandler = addFunction(backgroundName);
  } else
    m_backgroundHandler = nullptr;
  emit functionChanged();
}

/**
 * Sets whether fit members should be convolved with the resolution after a fit.
 *
 * @param convolveMembers If true, members are to be convolved.
 */
void IndirectFitPropertyBrowser::setConvolveMembers(bool convolveMembers) {
  m_boolManager->setValue(m_convolveMembers, convolveMembers);
}

/**
 * Sets whether the custom setting with the specified name is enabled.
 *
 * @param settingName The name of the custom setting.
 * @param enabled     True if custom setting should be enabled, false otherwise.
 */
void IndirectFitPropertyBrowser::setCustomSettingEnabled(
    const QString &settingName, bool enabled) {
  m_customSettings[settingName]->setEnabled(enabled);
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

void IndirectFitPropertyBrowser::updateErrors() {
  getHandler()->updateErrors();
}

void IndirectFitPropertyBrowser::updateTies() {
  for (auto i = 0u; i < compositeFunction()->nParams(); ++i)
    updateTie(i);
}

void IndirectFitPropertyBrowser::updateTie(std::size_t index) {
  const auto function = compositeFunction();
  const auto tie = function->getTie(index);
  const auto tieString = tie ? tie->asString() : "";
  removeTie(QString::fromStdString(function->parameterName(index)));

  if (!tieString.empty())
    addTie(QString::fromStdString(tieString));
}

void IndirectFitPropertyBrowser::addTie(const QString &tieString) {
  const auto index = tieString.split(".").first().right(1).toInt();
  const auto handler = getHandler()->getHandler(index);

  if (handler)
    handler->addTie(tieString);
}

void IndirectFitPropertyBrowser::removeTie(const QString &parameterName) {
  const auto parts = parameterName.split(".");
  const auto index = parts.first().right(1).toInt();
  const auto name = parts.last();
  const auto handler = getHandler()->getHandler(index);

  if (handler) {
    const auto tieProperty = handler->getTies()[name];
    handler->removeTie(tieProperty, parameterName.toStdString());
  }
}

void IndirectFitPropertyBrowser::clearErrors() { getHandler()->clearErrors(); }

/**
 * @param settingKey  The key of the boolean setting whose value to retrieve.
 * @return            The value of the boolean setting with the specified key.
 */
bool IndirectFitPropertyBrowser::boolSettingValue(
    const QString &settingKey) const {
  return m_boolManager->value(m_customSettings[settingKey]);
}

/**
 * Sets the value of the custom boolean setting, with the specified key, to the
 * specified value.
 *
 * @param settingKey  The key of the custom boolean setting.
 * @param value       The value to set the boolean custom setting to.
 */
void IndirectFitPropertyBrowser::setCustomBoolSetting(const QString &settingKey,
                                                      bool value) {
  m_boolManager->setValue(m_customSettings[settingKey], value);
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
  return enumValue(m_customSettings[settingKey]);
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
 * @param options       The available options in the enum setting.
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
 * @param enabled       True if the setting should start enabled, false
 *                      otherwise.
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
 * @param enabled         True if the setting should start enabled, false
 *                        otherwise.
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
 * Sets whether a setting with a specified key affects the fitting function.
 *
 * @param settingKey      The key of the setting.
 * @param changesFunction Boolean specifying whether the setting affects the
 *                        fitting function.
 */
void IndirectFitPropertyBrowser::setCustomSettingChangesFunction(
    const QString &settingKey, bool changesFunction) {
  if (changesFunction)
    m_functionChangingSettings.insert(m_customSettings[settingKey]);
  else
    m_functionChangingSettings.remove(m_customSettings[settingKey]);
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
  auto boolProperty = createFunctionGroupProperty(groupName, m_boolManager);
  m_functionsAsCheckBox.insert(boolProperty);
  addCustomFunctionGroup(groupName, functions);
  m_boolManager->setValue(boolProperty, defaultValue);
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
 * Removes all current Fit Type options from the fit type combo-box in this
 * property browser.
 */
void IndirectFitPropertyBrowser::clearFitTypeComboBox() {
  m_enumManager->setEnumNames(m_functionsInComboBox, {"None"});
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
  for (int i = 0; i < multiples; ++i) {
    if (!m_functionHandlers.contains(prop))
      m_functionHandlers.insert(prop, QVector<PropertyHandler *>());
    addCustomFunctions(prop, m_groupToFunctionList[groupName]);
  }

  if (multiples > 0)
    emit functionChanged();
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

  addCustomFunctions(prop, m_groupToFunctionList[groupName]);
  emit functionChanged();
}

/**
 * Adds the functions with the specified name. Displays the functions in the
 * specified property.
 *
 * @param prop      The property in which to display the functions.
 * @param functions The functions to add.
 */
void IndirectFitPropertyBrowser::addCustomFunctions(
    QtProperty *prop, const std::vector<IFunction_sptr> &functions) {
  MantidQt::API::SignalBlocker<QObject> blocker(this);
  for (const auto &function : functions) {
    m_functionHandlers[prop] << addFunction(function->asString());
    m_customFunctionCount[function->name()] += 1;
  }
}

/**
 * Clears the functions in this indirect fit property browser.
 */
void IndirectFitPropertyBrowser::clear() {
  clearAllCustomFunctions();
  FitPropertyBrowser::clear();
}

/**
 * Clears all custom functions in this fit property browser.
 */
void IndirectFitPropertyBrowser::clearAllCustomFunctions() {
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
 * Updates the plot guess feature in this indirect fit property browser.
 * @param sampleWorkspace :: The workspace loaded as sample
 */
void IndirectFitPropertyBrowser::updatePlotGuess(
    MatrixWorkspace_const_sptr sampleWorkspace) {
  if (sampleWorkspace && compositeFunction()->nFunctions() > 0)
    setPeakToolOn(true);
  else
    setPeakToolOn(false);
}

/**
 * Clears all custom functions in the specified property.
 *
 * @param prop        The property to clear of custom functions.
 * @param emitSignals If True, will emit Qt signals.
 */
void IndirectFitPropertyBrowser::clearCustomFunctions(QtProperty *prop,
                                                      bool emitSignals) {
  clearCustomFunctions(prop);
  m_functionHandlers[prop].clear();

  if (emitSignals) {
    emit removePlotSignal(getHandler());
    emit functionRemoved();
    emit functionChanged();
  }
}

/**
 * Clears all custom functions in the specified property.
 *
 * @param prop        The property to clear of custom functions.
 */
void IndirectFitPropertyBrowser::clearCustomFunctions(QtProperty *prop) {
  MantidQt::API::SignalBlocker<QObject> blocker(this);
  for (const auto &functionHandler : m_functionHandlers[prop]) {

    if (functionHandler->parentHandler() != nullptr) {
      FitPropertyBrowser::removeFunction(functionHandler);
      m_customFunctionCount[functionHandler->function()->name()] -= 1;
    }
  }
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
      customFunctionRemoved(prop);
    }
  }

  if (handler->parentHandler() != nullptr)
    FitPropertyBrowser::removeFunction(handler);

  if (handler == m_backgroundHandler)
    m_enumManager->setValue(m_backgroundSelection, 0);
}

/**
 * Called when a custom function has been removed from this fit property
 * browser.
 *
 * @param prop  The property of the removed custom function.
 */
void IndirectFitPropertyBrowser::customFunctionRemoved(QtProperty *prop) {

  if (m_functionsAsSpinner.contains(prop)) {
    disconnect(m_intManager, SIGNAL(propertyChanged(QtProperty *)), this,
               SLOT(intChanged(QtProperty *)));
    m_intManager->setValue(prop, m_intManager->value(prop) - 1);
    connect(m_intManager, SIGNAL(propertyChanged(QtProperty *)), this,
            SLOT(intChanged(QtProperty *)));
    FitPropertyBrowser::intChanged(prop);
  } else if (m_functionsAsCheckBox.contains(prop)) {
    disconnect(m_boolManager, SIGNAL(propertyChanged(QtProperty *)), this,
               SLOT(boolChanged(QtProperty *)));
    m_boolManager->setValue(prop, false);
    connect(m_boolManager, SIGNAL(propertyChanged(QtProperty *)), this,
            SLOT(boolChanged(QtProperty *)));
    FitPropertyBrowser::boolChanged(prop);
  } else if (prop == m_functionsInComboBox) {
    disconnect(m_enumManager, SIGNAL(propertyChanged(QtProperty *)), this,
               SLOT(enumChanged(QtProperty *)));
    m_enumManager->setValue(m_functionsInComboBox, 0);
    connect(m_enumManager, SIGNAL(propertyChanged(QtProperty *)), this,
            SLOT(enumChanged(QtProperty *)));
    FitPropertyBrowser::enumChanged(prop);
  }
}

void IndirectFitPropertyBrowser::setWorkspaceIndex(int i) {
  FitPropertyBrowser::setWorkspaceIndex(i);
}

void IndirectFitPropertyBrowser::setFitEnabled(bool enable) {
  FitPropertyBrowser::setFitEnabled(enable);
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
    clearCustomFunctions(prop, false);
    addCustomFunctions(prop, enumValue(prop));
  } else if (prop == m_backgroundSelection) {
    setBackground(enumValue(prop).toStdString());
  } else if (m_customSettings.values().contains(prop)) {
    emit customEnumChanged(prop->propertyName(), enumValue(prop));
    emit customSettingChanged(prop);
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
      clearCustomFunctions(prop, true);
  } else if (m_customSettings.values().contains(prop)) {
    emit customBoolChanged(propertyName, m_boolManager->value(prop));
    emit customSettingChanged(prop);
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
    auto multiples = m_intManager->value(prop);
    clearCustomFunctions(prop, multiples == 0);
    addCustomFunctions(prop, prop->propertyName(), multiples);
  } else if (m_customSettings.values().contains(prop)) {
    emit customIntChanged(prop->propertyName(), m_intManager->value(prop));
    emit customSettingChanged(prop);
  }
  FitPropertyBrowser::intChanged(prop);
}

/**
 * Called when a double value changes in this indirect fit property browser.
 *
 * @param prop  The property containing the double value which was changed.
 */
void IndirectFitPropertyBrowser::doubleChanged(QtProperty *prop) {
  if (m_customSettings.values().contains(prop)) {
    emit customDoubleChanged(prop->propertyName(),
                             m_doubleManager->value(prop));
    emit customSettingChanged(prop);
  }
  FitPropertyBrowser::doubleChanged(prop);
}

/**
 * Called when a custom setting changes in this indirect fit property browser.
 *
 * @param prop The custom setting property which was changed.
 */
void IndirectFitPropertyBrowser::customChanged(QtProperty *prop) {
  if (m_functionChangingSettings.contains(prop))
    emit functionChanged();
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

/**
 * Called when the browser visibility has changed.
 *
 * @param isVisible True if the browser is visible, false otherwise.
 */
void IndirectFitPropertyBrowser::browserVisibilityChanged(bool isVisible) {
  if (!isVisible)
    emit browserClosed();
}

} // namespace MantidWidgets
} // namespace MantidQt
