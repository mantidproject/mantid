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
    : FitPropertyBrowser(parent, mantidui) {}

void IndirectFitPropertyBrowser::init() {
  QWidget *w = new QWidget(this);

  QSettings settings;
  settings.beginGroup("Mantid/FitBrowser");

  m_customFunctionGroups =
      m_groupManager->addProperty("Custom Function Groups");

  /* Create background selection */
  m_backgroundSelection = m_enumManager->addProperty("Background");

  /* Create function group */
  QtProperty *functionsGroup = m_groupManager->addProperty("Functions");

  /* Create fitting range group */
  QtProperty *fittingRangeGroup = m_groupManager->addProperty("Fitting Range");

  /* Create input - output properties */
  QtProperty *settingsGroup = m_groupManager->addProperty("Settings");
  m_startX = addDoubleProperty("StartX");
  m_endX = addDoubleProperty("EndX");

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
  bool convolveCompositeItems =
      settings.value(m_plotCompositeMembers->propertyName(), QVariant(false))
          .toBool();
  m_boolManager->setValue(m_convolveMembers, convolveCompositeItems);

  m_showParamErrors = m_boolManager->addProperty("Show Parameter Errors");
  bool showParamErrors =
      settings.value(m_showParamErrors->propertyName(), false).toBool();
  m_boolManager->setValue(m_showParamErrors, showParamErrors);
  m_parameterManager->setErrorsEnabled(showParamErrors);

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

  /* Create editors and assign them to the managers */
  createEditors(w);

  updateDecimals();

  m_browser->addProperty(m_customFunctionGroups);
  m_browser->addProperty(m_backgroundSelection);
  m_functionsGroup = m_browser->addProperty(functionsGroup);
  m_settingsGroup = m_browser->addProperty(settingsGroup);

  initLayout(w);

  QStringList backgrounds = {"None"};
  backgrounds.append(registeredBackgrounds());
  m_enumManager->setEnumNames(m_backgroundSelection, backgrounds);
}

Mantid::API::IFunction_sptr IndirectFitPropertyBrowser::backgroundFunction() {
  return m_backgroundHandler->function();
}

Mantid::API::IFunction_sptr IndirectFitPropertyBrowser::modelFunction() {
  removeFunction(m_backgroundHandler);
  const auto model = getFittingFunction();
  const auto backgroundName = enumValue(m_backgroundSelection).toStdString();
  m_backgroundHandler = addFunction(backgroundName);
  return model;
}

void IndirectFitPropertyBrowser::addCustomFunctionGroup(
    const QString &groupName, std::vector<std::string> functionNames,
    CustomGroupMode mode) {
  if (mode == CustomGroupMode::COMBOBOX) {
    addCustomFunctionGroupToComboBox(groupName);
  } else {
    const auto functionProperty = m_boolManager->addProperty(groupName);
    m_customFunctionGroups->addSubProperty(functionProperty);
    m_functionsAsCheckBox.insert(functionProperty);
  }
  m_groupToFunctionList[groupName] = functionNames;
}

QtBrowserItem IndirectFitPropertyBrowser::addCustomFunctionGroupToComboBox(
    const QString &groupName) {
  if (!m_functionsInComboBox) {
    m_functionsInComboBox = m_enumManager->addProperty("Function Group");
    m_customFunctionGroups->addSubProperty(m_functionsInComboBox);
  }

  const auto groupNames = m_enumManager->enumNames(m_functionsInComboBox)
                          << groupName;
  m_enumManager->setEnumNames(m_functionsInComboBox, groupNames);
}

void IndirectFitPropertyBrowser::enumChanged(QtProperty *prop) {

  if (prop == m_functionsInComboBox) {
    clearCustomFunctions(prop);
    addCustomFunctions(prop, enumValue(prop));
  } else if (prop == m_backgroundSelection) {

    if (m_backgroundHandler)
      removeFunction(m_backgroundHandler);

    const auto backgroundName = enumValue(prop).toStdString();
    if (backgroundName != "None")
      m_backgroundHandler = addFunction(backgroundName);
    else
      m_backgroundHandler = nullptr;
  }
  FitPropertyBrowser::enumChanged(prop);
}

void IndirectFitPropertyBrowser::boolChanged(QtProperty *prop) {

  if (m_functionsAsCheckBox.contains(prop)) {
    clearCustomFunctions(prop);
    addCustomFunctions(prop, prop->propertyName());
  }
  FitPropertyBrowser::boolChanged(prop);
}

void IndirectFitPropertyBrowser::addCustomFunctions(QtProperty *prop,
                                                    const QString &groupName) {
  if (!m_functionHandlers.contains(prop))
    m_functionHandlers.insert(prop, QVector<PropertyHandler *>());

  for (const auto &functionName : m_groupToFunctionList[groupName])
    m_functionHandlers[prop] << addFunction(functionName);
}

void IndirectFitPropertyBrowser::clearCustomFunctions(QtProperty *prop) {
  for (const auto &functionHandler : m_functionHandlers[prop])
    removeFunction(functionHandler);
  m_functionHandlers.clear();
}

QString IndirectFitPropertyBrowser::enumValue(QtProperty *prop) const {
  const auto values = m_enumManager->enumNames(prop);
  const auto selectedIndex = m_enumManager->value(prop);
  return values[selectedIndex];
}

} // namespace MantidWidgets
} // namespace MantidQt
