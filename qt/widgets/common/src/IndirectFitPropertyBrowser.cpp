// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/IndirectFitPropertyBrowser.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
//#include "MantidQtWidgets/Common/PropertyHandler.h"
//#include "MantidQtWidgets/Common/QtPropertyBrowser/StringEditorFactory.h"
#include "MantidQtWidgets/Common/SignalBlocker.h"

// Suppress a warning coming out of code that isn't ours
//#if defined(__INTEL_COMPILER)
//#pragma warning disable 1125
//#elif defined(__GNUC__)
//#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
//#pragma GCC diagnostic push
//#endif
//#pragma GCC diagnostic ignored "-Woverloaded-virtual"
//#endif
//#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h"
//#include "MantidQtWidgets/Common/QtPropertyBrowser/qteditorfactory.h"
//#if defined(__INTEL_COMPILER)
//#pragma warning enable 1125
//#elif defined(__GNUC__)
//#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
//#pragma GCC diagnostic pop
//#endif
//#endif

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FrameworkManager.h"

#include "MantidQtWidgets/Common/FitOptionsBrowser.h"
#include "MantidQtWidgets/Common/FunctionBrowser.h"

#include <QAction>
#include <QFormLayout>
#include <QMessageBox>
#include <QSettings>

#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>

#include <QMenu>
#include <QSignalMapper>

#include <QCheckBox>

#include <iostream>

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
    : QDockWidget(parent)
{
  setFeatures(QDockWidget::DockWidgetFloatable |
              QDockWidget::DockWidgetMovable);
  setWindowTitle("Fit Function");
}

void IndirectFitPropertyBrowser::initFunctionBrowser() {
  m_functionBrowser = new FunctionBrowser(nullptr, true);
  m_functionBrowser->setObjectName("functionBrowser");
  connect(m_functionBrowser, SIGNAL(functionStructureChanged()), this,
          SIGNAL(functionChanged()));
}

void IndirectFitPropertyBrowser::iniFitOptionsBrowser() {
  m_fitOptionsBrowser = new FitOptionsBrowser(nullptr, FitOptionsBrowser::SimultaneousAndSequential);
  m_fitOptionsBrowser->setObjectName("fitOptionsBrowser");
}

void IndirectFitPropertyBrowser::init() {
  initFunctionBrowser();
  iniFitOptionsBrowser();

  auto w = new QWidget(this);
  auto layout = new QVBoxLayout(w);
  layout->setContentsMargins(0, 0, 0, 0);
  auto splitter = new QSplitter(Qt::Vertical);
  layout->addWidget(splitter);

  splitter->addWidget(m_functionBrowser);
  splitter->addWidget(m_fitOptionsBrowser);
  w->setLayout(layout);
  setWidget(w);
}

void IndirectFitPropertyBrowser::setFunction(const QString &funStr) {
  m_functionBrowser->setFunction(funStr);
}

IFunction_sptr IndirectFitPropertyBrowser::getFittingFunction() const {
  try {
    if (m_functionBrowser->getNumberOfDatasets() == 0) {
      return m_functionBrowser->getFunction();
    }
    return m_functionBrowser->getGlobalFunction();
  } catch (std::invalid_argument) {
    return IFunction_sptr(new CompositeFunction);
  }
}

IFunction_sptr IndirectFitPropertyBrowser::compositeFunction() const {
  return getFittingFunction();
}

std::string IndirectFitPropertyBrowser::minimizer(bool withProperties) const {
  return m_fitOptionsBrowser->getProperty("Minimizer").toStdString();
}

int IndirectFitPropertyBrowser::maxIterations() const {
  return m_fitOptionsBrowser->getProperty("MaxIterations").toInt();
}

int IndirectFitPropertyBrowser::getPeakRadius() const {
  return m_fitOptionsBrowser->getProperty("PeakRadius").toInt();
}

std::string IndirectFitPropertyBrowser::costFunction() const {
  return m_fitOptionsBrowser->getProperty("CostFunction").toStdString();
}

bool IndirectFitPropertyBrowser::convolveMembers() const { return false; }

bool IndirectFitPropertyBrowser::isHistogramFit() const { return false; }

bool IndirectFitPropertyBrowser::ignoreInvalidData() const { return false; }

void IndirectFitPropertyBrowser::updateParameters(
    const Mantid::API::IFunction &fun) {
  std::cerr << "Update parameters " << std::endl;
  m_functionBrowser->updateParameters(fun);
}

/**
 * @return  The selected background function.
 */
IFunction_sptr IndirectFitPropertyBrowser::background() const {
  // if (m_backgroundHandler != nullptr)
  //  return m_backgroundHandler->function()->clone();
  // else
  return nullptr;
}

/**
 * @return  The function index of the selected background. None if no background
 *          is selected.
 */
boost::optional<size_t> IndirectFitPropertyBrowser::backgroundIndex() const {
  // if (m_backgroundHandler != nullptr) {
  //  const auto prefix = m_backgroundHandler->functionPrefix();

  //  if (!prefix.endsWith("-1"))
  //    return prefix.right(1).toInt();
  //}
  return boost::none;
}

/**
 * @param function  The function, whose function index to retrieve.
 * @return          The function index of the specified function in the browser.
 */
boost::optional<size_t>
IndirectFitPropertyBrowser::functionIndex(IFunction_sptr function) const {
  // for (size_t i = 0u; i < compositeFunction()->nFunctions(); ++i) {
  //  if (compositeFunction()->getFunction(i) == function)
  //    return i;
  //}
  return boost::none;
}

/**
 * @return  The selected fit type in the fit type combo box.
 */
QString IndirectFitPropertyBrowser::selectedFitType() const {
  // const auto index = m_enumManager->value(m_functionsInComboBox);
  // return m_enumManager->enumNames(m_functionsInComboBox)[index];
  return "Sequential";
}

/**
 * @return  The name of the selected background function.
 */
QString IndirectFitPropertyBrowser::backgroundName() const {
  // auto background = enumValue(m_backgroundSelection);
  // if (background.isEmpty())
  return "None";
  // else
  //  return background;
}

/**
 * @return  A map from parameter name to a tie expression.
 */
QHash<QString, QString> IndirectFitPropertyBrowser::getTies() const {
  QHash<QString, QString> ties;
  // getCompositeTies(getHandler(), ties);
  return ties;
}


/**
 * @param functionName  The name of the function.
 * @return              The number of custom functions, with the specified name,
 *                      included in the selected model.
 */
size_t IndirectFitPropertyBrowser::numberOfCustomFunctions(
    const std::string &functionName) const {
  // auto count = m_customFunctionCount.find(functionName);
  // if (count != m_customFunctionCount.end())
  //  return count->second;
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
  // const auto composite = compositeFunction();

  // for (size_t i = 0u; i < composite->nFunctions(); ++i) {
  //  const auto function = composite->getFunction(i);

  //  if (function->name() == functionName)
  //    values.emplace_back(function->getParameter(parameterName));
  //}
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
  // const auto composite = compositeFunction();

  // for (size_t i = 0u; i < composite->nFunctions(); ++i) {
  //  const auto function = composite->getFunction(i);

  //  if (function->name() == functionName)
  //    setParameterValue(function, parameterName, value);
  //}

  // updateParameters();
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
    // emit parameterChanged(function.get());
  }
}

/**
 * Sets the background to add to the selected model.
 *
 * @param backgroundName  The name of the background to add.
 */
void IndirectFitPropertyBrowser::setBackground(
    const std::string &backgroundName) {
  // if (m_backgroundHandler != nullptr && backgroundIndex()) {
  //  MantidQt::API::SignalBlocker<QObject> blocker(this);
  //  FitPropertyBrowser::removeFunction(m_backgroundHandler);
  //}

  // if (backgroundName != "None") {
  //  MantidQt::API::SignalBlocker<QObject> blocker(this);
  //  m_backgroundHandler = addFunction(backgroundName);
  //} else
  //  m_backgroundHandler = nullptr;
  // emit functionChanged();
}

/**
 * Sets whether fit members should be convolved with the resolution after a fit.
 *
 * @param convolveMembers If true, members are to be convolved.
 */
void IndirectFitPropertyBrowser::setConvolveMembers(bool convolveMembers) {
  // m_boolManager->setValue(m_convolveMembers, convolveMembers);
}

/**
 * Sets whether the custom setting with the specified name is enabled.
 *
 * @param settingName The name of the custom setting.
 * @param enabled     True if custom setting should be enabled, false otherwise.
 */
void IndirectFitPropertyBrowser::setCustomSettingEnabled(
    const QString &settingName, bool enabled) {
  // m_customSettings[settingName]->setEnabled(enabled);
}

/**
 * Sets the available background options in this indirect fit property browser.
 *
 * @param backgrounds A list of the names of available backgrouns to set.
 */
void IndirectFitPropertyBrowser::setBackgroundOptions(
    const QStringList &backgrounds) {
  // const auto currentlyHidden =
  //    m_enumManager->enumNames(m_backgroundSelection).isEmpty();
  // const auto doHide = backgrounds.isEmpty();

  // if (doHide && !currentlyHidden)
  //  m_browser->removeProperty(m_backgroundGroup);
  // else if (!doHide && currentlyHidden)
  //  m_browser->insertProperty(m_backgroundGroup, m_customFunctionGroups);

  // m_enumManager->setEnumNames(m_backgroundSelection, backgrounds);
}

void IndirectFitPropertyBrowser::updateErrors() {
  // getHandler()->updateErrors();
}

void IndirectFitPropertyBrowser::updateTies() {
  // for (auto i = 0u; i < compositeFunction()->nParams(); ++i)
  //  updateTie(i);
}

void IndirectFitPropertyBrowser::updateTie(std::size_t index) {
  // const auto function = compositeFunction();
  // const auto tie = function->getTie(index);
  // const auto tieString = tie ? tie->asString() : "";
  // removeTie(QString::fromStdString(function->parameterName(index)));

  // if (!tieString.empty())
  //  addTie(QString::fromStdString(tieString));
}

void IndirectFitPropertyBrowser::addTie(const QString &tieString) {
  // const auto index = tieString.split(".").first().right(1).toInt();
  // const auto handler = getHandler()->getHandler(index);

  // if (handler)
  //  handler->addTie(tieString);
}

void IndirectFitPropertyBrowser::removeTie(const QString &parameterName) {
  // const auto parts = parameterName.split(".");
  // const auto index = parts.first().right(1).toInt();
  // const auto name = parts.last();
  // const auto handler = getHandler()->getHandler(index);

  // if (handler) {
  //  const auto tieProperty = handler->getTies()[name];
  //  handler->removeTie(tieProperty, parameterName.toStdString());
  //}
}

void IndirectFitPropertyBrowser::clearErrors() {
  //  getHandler()->clearErrors();
}

/**
 * @param settingKey  The key of the boolean setting whose value to retrieve.
 * @return            The value of the boolean setting with the specified key.
 */
bool IndirectFitPropertyBrowser::boolSettingValue(
    const QString &settingKey) const {
  return false; // m_boolManager->value(m_customSettings[settingKey]);
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
  // m_boolManager->setValue(m_customSettings[settingKey], value);
}

/**
 * @param settingKey  The key of the integer setting whose value to retrieve.
 * @return            The value of the integer setting with the specified key.
 */
int IndirectFitPropertyBrowser::intSettingValue(
    const QString &settingKey) const {
  return -1; // m_intManager->value(m_customSettings[settingKey]);
}

/**
 * @param settingKey  The key of the double setting whose value to retrieve.
 * @return            The value of the double setting with the specified key.
 */
double IndirectFitPropertyBrowser::doubleSettingValue(
    const QString &settingKey) const {
  return 0.0; // m_doubleManager->value(m_customSettings[settingKey]);
}

/**
 * @param settingKey  The key of the enum setting whose value to retrieve.
 * @return            The value of the enum setting with the specified key.
 */
QString
IndirectFitPropertyBrowser::enumSettingValue(const QString &settingKey) const {
  // return enumValue(m_customSettings[settingKey]);
  return "";
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
  // auto settingProperty = m_boolManager->addProperty(settingName);
  // m_boolManager->setValue(settingProperty, defaultValue);
  // addCustomSetting(settingKey, settingProperty);
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
  // auto settingProperty = m_intManager->addProperty(settingName);
  // m_intManager->setValue(settingProperty, defaultValue);
  // addCustomSetting(settingKey, settingProperty);
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
  // auto settingProperty = m_doubleManager->addProperty(settingName);
  // m_doubleManager->setValue(settingProperty, defaultValue);
  // addCustomSetting(settingKey, settingProperty);
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
  // auto settingProperty = m_enumManager->addProperty(settingName);
  // m_enumManager->setEnumNames(settingProperty, options);
  // addCustomSetting(settingKey, settingProperty);
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
  // auto settingProperty = m_doubleManager->addProperty(settingName);
  // m_doubleManager->setValue(settingProperty, defaultValue);
  // addOptionalSetting(settingKey, settingProperty, optionKey, optionName,
  //                   enabled);
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
  // if (changesFunction)
  //  m_functionChangingSettings.insert(m_customSettings[settingKey]);
  // else
  //  m_functionChangingSettings.remove(m_customSettings[settingKey]);
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
  // auto boolProperty = createFunctionGroupProperty(groupName, m_boolManager);
  // m_functionsAsCheckBox.insert(boolProperty);
  // addCustomFunctionGroup(groupName, functions);
  // m_boolManager->setValue(boolProperty, defaultValue);
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
  // auto intProperty = createFunctionGroupProperty(groupName, m_intManager);
  // m_intManager->setMinimum(intProperty, minimum);
  // m_intManager->setMaximum(intProperty, maximum);
  // m_intManager->setValue(intProperty, defaultValue);
  // m_functionsAsSpinner.insert(intProperty);
  // addCustomFunctionGroup(groupName, functions);
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
  // if (m_functionsInComboBox == nullptr) {
  //  m_functionsInComboBox =
  //      createFunctionGroupProperty("Fit Type", m_enumManager, true);
  //  m_groupToFunctionList["None"] = {};
  //  m_enumManager->setEnumNames(m_functionsInComboBox, {"None"});
  //}

  // auto groupNames = m_enumManager->enumNames(m_functionsInComboBox)
  //                  << groupName;
  // m_enumManager->setEnumNames(m_functionsInComboBox, groupNames);
  // addCustomFunctionGroup(groupName, functions);
}

/**
 * Removes all current Fit Type options from the fit type combo-box in this
 * property browser.
 */
void IndirectFitPropertyBrowser::clearFitTypeComboBox() {
  // m_enumManager->setEnumNames(m_functionsInComboBox, {"None"});
}

/**
 * Clears the functions in this indirect fit property browser.
 */
void IndirectFitPropertyBrowser::clear() {
  // clearAllCustomFunctions();
  // FitPropertyBrowser::clear();
}

/**
 * Clears all custom functions in this fit property browser.
 */
void IndirectFitPropertyBrowser::clearAllCustomFunctions() {
  // for (const auto &prop : m_functionHandlers.keys()) {
  //  if (m_functionsAsCheckBox.contains(prop))
  //    m_boolManager->setValue(prop, false);
  //  else if (m_functionsAsSpinner.contains(prop))
  //    m_intManager->setValue(prop, 0);
  //  m_functionHandlers[prop].clear();
  //}

  // setBackground("None");
  // m_enumManager->setValue(m_backgroundSelection, 0);
  // m_enumManager->setValue(m_functionsInComboBox, 0);
}

/**
 * Updates the plot guess feature in this indirect fit property browser.
 * @param sampleWorkspace :: The workspace loaded as sample
 */
void IndirectFitPropertyBrowser::updatePlotGuess(
    MatrixWorkspace_const_sptr sampleWorkspace) {
  // if (sampleWorkspace && compositeFunction()->nFunctions() > 0)
  //  setPeakToolOn(true);
  // else
  //  setPeakToolOn(false);
}


void IndirectFitPropertyBrowser::setWorkspaceIndex(int i) {
  // FitPropertyBrowser::setWorkspaceIndex(i);
}

int IndirectFitPropertyBrowser::workspaceIndex() const { return 0; }

void IndirectFitPropertyBrowser::updateAttributes() {}

int IndirectFitPropertyBrowser::count() const { return 0; }

IFunction_sptr IndirectFitPropertyBrowser::getFunctionAtIndex(int) const {
  return IFunction_sptr();
}

void IndirectFitPropertyBrowser::setDefaultPeakType(
    const std::string &function) {}

void IndirectFitPropertyBrowser::setWorkspaceName(const QString &function) {}

void IndirectFitPropertyBrowser::setStartX(double) {}

void IndirectFitPropertyBrowser::setEndX(double) {}

void IndirectFitPropertyBrowser::updateFunctionBrowserData(size_t nData) {
  m_functionBrowser->setNumberOfDatasets(static_cast<int>(nData));
}

void IndirectFitPropertyBrowser::setFitEnabled(bool enable) {
  // FitPropertyBrowser::setFitEnabled(enable);
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
