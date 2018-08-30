#include "FunctionBrowser.h"
#include "FunctionBrowserSubscriber.h"

#include "MantidAPI/Expression.h"

#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleDialogEditor.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/FilenameDialogEditor.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/FormulaDialogEditor.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/WorkspaceEditorFactory.h"
#include "MantidQtWidgets/Common/SelectFunctionDialog.h"
#include "MantidQtWidgets/Common/UserFunctionDialog.h"

#include "MantidQtWidgets/Common/QtPropertyBrowser/CompositeEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qteditorfactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qttreepropertybrowser.h"

#include <boost/lexical_cast.hpp>

#include <QApplication>
#include <QClipboard>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>

namespace {
template <typename T, typename PropertyManager>
std::vector<T> getVectorFromProperty(QtProperty *prop,
                                     PropertyManager *manager) {
  auto const members = prop->subProperties();
  if (members.empty())
    return std::vector<T>();

  std::vector<T> values;
  values.reserve(static_cast<std::size_t>(members.size()));
  for (auto it = members.begin() + 1; it < members.end(); ++it)
    values.emplace_back(manager->value(*it));
  return values;
}
} // namespace

namespace MantidQt {
namespace MantidWidgets {
namespace Function {

FunctionBrowser::FunctionBrowser(QWidget *parent) : QWidget(parent) {}

void FunctionBrowser::subscribe(FunctionBrowserSubscriber *subscriber) {
  m_subscriber = subscriber;
}

void FunctionBrowser::createBrowser() {
  m_browser = createNewBrowser().release();

  createManagers();
  createEditorFactories();

  m_browser->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_browser, SIGNAL(customContextMenuRequested(QPoint const &)), this,
          SLOT(popupMenu(const QPoint &)));
  connect(m_browser, SIGNAL(optionChanged(QtProperty *, QString const &, bool)),
          this, SLOT(globalChanged(QtProperty *, QString const &, bool)));

  connectManagerSignals();

  connect(m_browser, SIGNAL(currentItemChanged(QtBrowserItem *)),
          SLOT(setSelectedProperty(QtBrowserItem *)));

  m_browser->setFocusPolicy(Qt::StrongFocus);
}

std::unique_ptr<QtTreePropertyBrowser> FunctionBrowser::createNewBrowser() {
  return Mantid::Kernel::make_unique<QtTreePropertyBrowser>(nullptr);
}

/**
 * Create property managers: they create, own properties, get and set values
 */
void FunctionBrowser::createManagers() {
  m_functionManager = new QtGroupPropertyManager(this);
  m_parameterManager = new ParameterPropertyManager(this);
  m_attributeStringManager = new QtStringPropertyManager(this);
  m_attributeDoubleManager = new QtDoublePropertyManager(this);
  m_attributeIntManager = new QtIntPropertyManager(this);
  m_attributeBoolManager = new QtBoolPropertyManager(this);
  m_indexManager = new QtStringPropertyManager(this);
  m_tieManager = new QtStringPropertyManager(this);
  m_constraintManager = new QtStringPropertyManager(this);
  m_filenameManager = new QtStringPropertyManager(this);
  m_formulaManager = new QtStringPropertyManager(this);
  m_workspaceManager = new QtStringPropertyManager(this);
  m_attributeVectorManager = new QtGroupPropertyManager(this);
  m_attributeSizeManager = new QtIntPropertyManager(this);
  m_attributeVectorDoubleManager = new QtDoublePropertyManager(this);
}

/**
 * Creates the editor factories.
 */
void FunctionBrowser::createEditorFactories() {
  QtSpinBoxFactory *spinBoxFactory = new QtSpinBoxFactory(this);
  DoubleEditorFactory *doubleEditorFactory = new DoubleEditorFactory(this);

  QtLineEditFactory *lineEditFactory = new QtLineEditFactory(this);
  QtCheckBoxFactory *checkBoxFactory = new QtCheckBoxFactory(this);
  FilenameDialogEditorFactory *filenameDialogEditorFactory =
      new FilenameDialogEditorFactory(this);
  FormulaDialogEditorFactory *formulaDialogEditFactory =
      new FormulaDialogEditorFactory(this);
  WorkspaceEditorFactory *workspaceEditorFactory =
      new WorkspaceEditorFactory(this);

  // assign factories to property managers
  m_browser->setFactoryForManager(m_parameterManager,
                                  getParameterEditorFactory().release());
  m_browser->setFactoryForManager(m_attributeStringManager, lineEditFactory);
  m_browser->setFactoryForManager(m_attributeDoubleManager,
                                  doubleEditorFactory);
  m_browser->setFactoryForManager(m_attributeIntManager, spinBoxFactory);
  m_browser->setFactoryForManager(m_attributeBoolManager, checkBoxFactory);
  m_browser->setFactoryForManager(m_indexManager, lineEditFactory);
  m_browser->setFactoryForManager(m_tieManager, lineEditFactory);
  m_browser->setFactoryForManager(m_constraintManager, lineEditFactory);
  m_browser->setFactoryForManager(m_filenameManager,
                                  filenameDialogEditorFactory);
  m_browser->setFactoryForManager(m_formulaManager, formulaDialogEditFactory);
  m_browser->setFactoryForManager(m_workspaceManager, workspaceEditorFactory);
  m_browser->setFactoryForManager(m_attributeSizeManager, spinBoxFactory);
  m_browser->setFactoryForManager(m_attributeVectorDoubleManager,
                                  doubleEditorFactory);
}

void FunctionBrowser::connectManagerSignals() {
  connect(m_attributeStringManager,
          SIGNAL(valueChanged(QtProperty *, QString const &)), this,
          SLOT(stringAttributeChanged(QtProperty *, QString const &)));
  connect(m_attributeDoubleManager, SIGNAL(valueChanged(QtProperty *, double)),
          this, SLOT(doubleAttributeChanged(QtProperty *, double)));
  connect(m_attributeIntManager, SIGNAL(valueChanged(QtProperty *, int)), this,
          SLOT(intAttributeChanged(QtProperty *, int)));
  connect(m_attributeBoolManager, SIGNAL(valueChanged(QtProperty *, bool)),
          this, SLOT(boolAttributeChanged(QtProperty *, bool)));
  connect(m_formulaManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(attributeChanged(QtProperty *)));
  connect(m_filenameManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(attributeChanged(QtProperty *)));
  connect(m_workspaceManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(attributeChanged(QtProperty *)));
  connect(m_attributeVectorDoubleManager, SIGNAL(propertyChanged(QtProperty *)),
          this, SLOT(vectorDoubleAttributeChanged(QtProperty *)));
  connect(m_attributeSizeManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(attributeVectorSizeChanged(QtProperty *)));
  connect(m_tieManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(tieChanged(QtProperty *)));
  connect(m_constraintManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(constraintChanged(QtProperty *)));
  connect(m_parameterManager, SIGNAL(valueChanged(QtProperty *, double)),
          SLOT(parameterChanged(QtProperty *)));
}

void FunctionBrowser::createActions() {
  m_actionAddFunction = new QAction("Add function", this);
  connect(m_actionAddFunction, SIGNAL(triggered()), this, SLOT(addFunction()));

  m_actionRemoveFunction = new QAction("Remove function", this);
  connect(m_actionRemoveFunction, SIGNAL(triggered()), this,
          SLOT(removeFunction()));

  m_actionFixParameter = new QAction("Fix", this);
  connect(m_actionFixParameter, SIGNAL(triggered()), this,
          SLOT(fixParameter()));

  m_actionRemoveTie = new QAction("Remove tie", this);
  connect(m_actionRemoveTie, SIGNAL(triggered()), this, SLOT(removeTie()));

  m_actionAddTie = new QAction("Add tie", this);
  connect(m_actionAddTie, SIGNAL(triggered()), this, SLOT(addTie()));

  m_actionFromClipboard = new QAction("Copy from clipboard", this);
  connect(m_actionFromClipboard, SIGNAL(triggered()), this,
          SLOT(copyFromClipboard()));

  m_actionToClipboard = new QAction("Copy to clipboard", this);
  connect(m_actionToClipboard, SIGNAL(triggered()), this,
          SLOT(copyToClipboard()));

  m_actionConstraints = new QAction("Custom", this);
  connect(m_actionConstraints, SIGNAL(triggered()), this,
          SLOT(addConstraints()));

  m_actionConstraints10 = new QAction("10%", this);
  connect(m_actionConstraints10, SIGNAL(triggered()), this,
          SLOT(addConstraints10()));

  m_actionConstraints50 = new QAction("50%", this);
  connect(m_actionConstraints50, SIGNAL(triggered()), this,
          SLOT(addConstraints50()));

  m_actionRemoveConstraints = new QAction("Remove constraints", this);
  connect(m_actionRemoveConstraints, SIGNAL(triggered()), this,
          SLOT(removeConstraints()));

  m_actionRemoveConstraint = new QAction("Remove", this);
  connect(m_actionRemoveConstraint, SIGNAL(triggered()), this,
          SLOT(removeConstraint()));
}

std::unique_ptr<QtAbstractEditorFactory<ParameterPropertyManager>>
FunctionBrowser::getParameterEditorFactory() {
  return Mantid::Kernel::make_unique<ParameterEditorFactory>(this);
}

void FunctionBrowser::setParameterValue(std::string const &name, double value) {
  if (auto const parameterProperty = getParameterProperty(name))
    m_parameterManager->setValue(parameterProperty, value);
}

void FunctionBrowser::setParameterError(std::string const &name, double value) {
  if (auto const parameterProperty = getParameterProperty(name))
    m_parameterManager->setError(parameterProperty, value);
}

void FunctionBrowser::removeParameterError(std::string const &name) {
  if (auto const parameterProperty = getParameterProperty(name))
    m_parameterManager->clearError(parameterProperty);
}

void FunctionBrowser::setParameterTie(std::string const &name,
                                      std::string const &tie) {
  if (auto const parameterProperty = getParameterProperty(name)) {
    if (auto const tieProperty = getTieProperty(parameterProperty))
      m_tieManager->setValue(tieProperty, QString::fromStdString(tie));
    else
      addTieProperty(parameterProperty, QString::fromStdString(tie));
  }
}

void FunctionBrowser::removeParameterTie(std::string const &name) {
  if (auto const parameterProperty = getParameterProperty(name)) {
    if (auto const tieProperty = getTieProperty(parameterProperty))
      removeProperty(tieProperty);
  }
}

void FunctionBrowser::setParameterUpperBound(std::string const &name,
                                             double bound) {
  if (auto const parameterProperty = getParameterProperty(name))
    setUpperBoundProperty(parameterProperty, QString::number(bound));
}

void FunctionBrowser::setParameterLowerBound(std::string const &name,
                                             double bound) {
  if (auto const parameterProperty = getParameterProperty(name))
    setLowerBoundProperty(parameterProperty, QString::number(bound));
}

void FunctionBrowser::setParameterBounds(std::string const &name,
                                         double lowerBound, double upperBound) {
  if (auto const parameterProperty = getParameterProperty(name)) {
    setLowerBoundProperty(parameterProperty, QString::number(lowerBound));
    setUpperBoundProperty(parameterProperty, QString::number(upperBound));
  }
}

void FunctionBrowser::removeParameterUpperBound(std::string const &name) {
  if (auto const parameterProperty = getParameterProperty(name))
    removeUpperBoundProperty(parameterProperty);
}

void FunctionBrowser::removeParameterLowerBound(std::string const &name) {
  if (auto const parameterProperty = getParameterProperty(name))
    removeLowerBoundProperty(parameterProperty);
}

void FunctionBrowser::removeParameterConstraints(std::string const &name) {
  if (auto const parameterProperty = getParameterProperty(name)) {
    removeLowerBoundProperty(parameterProperty);
    removeUpperBoundProperty(parameterProperty);
  }
}

std::vector<std::size_t> FunctionBrowser::getSelectedFunctionPosition() const {
  return getFunctionPosition(m_selectedProperty);
}

void FunctionBrowser::selectFunctionAt(std::vector<size_t> const &position) {
  m_selectedProperty = getFunctionPropertyAt(position);
}

void FunctionBrowser::addFunctionToSelectedFunction(std::string const &name) {
  addFunctionProperty(m_selectedProperty, QString::fromStdString(name));
}

void FunctionBrowser::addFunctionToSelectedFunctionAndSelect(
    std::string const &name) {
  m_selectedProperty =
      addFunctionProperty(m_selectedProperty, QString::fromStdString(name))
          .prop;
}

void FunctionBrowser::removeSelectedFunction() {
  if (isFunction(m_selectedProperty))
    removeProperty(m_selectedProperty);
}

void FunctionBrowser::addIndexToSelectedFunction(std::string const &index) {
  addIndexProperty(m_selectedProperty, QString::fromStdString(index));
}

void FunctionBrowser::setIndicesOfFunctionsAt(
    std::vector<std::string> const &indices,
    std::vector<std::size_t> const &position) {
  if (auto const functionProperty = getFunctionPropertyAt(position))
    setIndicesOfFunctionsAt(functionProperty, indices);
}

void FunctionBrowser::addParameterToSelectedFunction(
    std::string const &name, std::string const &description, double value) {
  addParameterProperty(m_selectedProperty, QString::fromStdString(name),
                       QString::fromStdString(description), value);
}

void FunctionBrowser::addIntAttributeToSelectedFunction(std::string const &name,
                                                        int value) {
  auto prop = m_attributeIntManager->addProperty(QString::fromStdString(name));
  m_attributeIntManager->setValue(prop, value);
  addAttributeProperty(m_selectedProperty, prop);
}

void FunctionBrowser::addBoolAttributeToSelectedFunction(
    std::string const &name, bool value) {
  auto prop = m_attributeBoolManager->addProperty(QString::fromStdString(name));
  m_attributeBoolManager->setValue(prop, value);
  addAttributeProperty(m_selectedProperty, prop);
}

void FunctionBrowser::addDoubleAttributeToSelectedFunction(
    std::string const &name, double value) {
  auto prop =
      m_attributeDoubleManager->addProperty(QString::fromStdString(name));
  m_attributeDoubleManager->setValue(prop, value);
  addAttributeProperty(m_selectedProperty, prop);
}

void FunctionBrowser::addStringAttributeToSelectedFunction(
    std::string const &name, std::string const &value) {
  auto prop =
      m_attributeStringManager->addProperty(QString::fromStdString(name));
  m_attributeStringManager->setValue(prop, QString::fromStdString(value));
  addAttributeProperty(m_selectedProperty, prop);
}

void FunctionBrowser::addFileAttributeToSelectedFunction(
    std::string const &name, std::string const &value) {
  auto prop = m_filenameManager->addProperty(QString::fromStdString(name));
  m_filenameManager->setValue(prop, QString::fromStdString(value));
  addAttributeProperty(m_selectedProperty, prop);
}

void FunctionBrowser::addFormulaAttributeToSelectedFunction(
    std::string const &name, std::string const &value) {
  auto prop = m_formulaManager->addProperty(QString::fromStdString(name));
  m_formulaManager->setValue(prop, QString::fromStdString(value));
  addAttributeProperty(m_selectedProperty, prop);
}

void FunctionBrowser::addWorkspaceAttributeToSelectedFunction(
    std::string const &name, std::string const &value) {
  auto prop =
      m_attributeStringManager->addProperty(QString::fromStdString(name));
  m_attributeStringManager->setValue(prop, QString::fromStdString(value));
  addAttributeProperty(m_selectedProperty, prop);
}

void FunctionBrowser::addVectorAttributeToSelectedFunction(
    std::string const &name, std::vector<double> const &value) {
  auto const prop =
      m_attributeVectorManager->addProperty(QString::fromStdString(name));
  FunctionBrowser::AProperty aprop = addProperty(m_selectedProperty, prop);

  m_attributeSizeManager->blockSignals(true);
  auto const sizeProp = m_attributeSizeManager->addProperty("Size");
  m_attributeSizeManager->setValue(sizeProp, static_cast<int>(value.size()));
  addAttributeProperty(prop, sizeProp);
  m_attributeSizeManager->blockSignals(false);
  sizeProp->setEnabled(false);

  m_attributeVectorDoubleManager->blockSignals(true);
  QString parName = "value[%1]";
  for (auto i = 0u; i < value.size(); ++i) {
    auto const dprop =
        m_attributeVectorDoubleManager->addProperty(parName.arg(i));
    m_attributeVectorDoubleManager->setValue(dprop, value[i]);
    addProperty(prop, dprop);
  }
  m_attributeVectorDoubleManager->blockSignals(false);

  m_browser->setExpanded(aprop.item, false);
}

void FunctionBrowser::setIntAttribute(std::string const &name, int value) {
  m_attributeIntManager->setValue(
      m_attributeNameToProperty[QString::fromStdString(name)], value);
}

void FunctionBrowser::setBoolAttribute(std::string const &name, bool value) {
  m_attributeBoolManager->setValue(
      m_attributeNameToProperty[QString::fromStdString(name)], value);
}

void FunctionBrowser::setDoubleAttribute(std::string const &name,
                                         double value) {
  m_attributeDoubleManager->setValue(
      m_attributeNameToProperty[QString::fromStdString(name)], value);
}

void FunctionBrowser::setStringAttribute(std::string const &name,
                                         std::string const &value) {
  m_attributeStringManager->setValue(
      m_attributeNameToProperty[QString::fromStdString(name)],
      QString::fromStdString(value));
}

void FunctionBrowser::setFileAttribute(std::string const &name,
                                       std::string const &value) {
  m_filenameManager->setValue(
      m_attributeNameToProperty[QString::fromStdString(name)],
      QString::fromStdString(value));
}

void FunctionBrowser::setFormulaAttribute(std::string const &name,
                                          std::string const &value) {
  m_formulaManager->setValue(
      m_attributeNameToProperty[QString::fromStdString(name)],
      QString::fromStdString(value));
}

void FunctionBrowser::setWorkspaceAttribute(std::string const &name,
                                            std::string const &value) {
  m_workspaceManager->setValue(
      m_attributeNameToProperty[QString::fromStdString(name)],
      QString::fromStdString(value));
}

void FunctionBrowser::setVectorAttribute(std::string const &name,
                                         std::vector<double> const &value) {
  auto const vectorProperty =
      m_attributeNameToProperty[QString::fromStdString(name)];
}

void FunctionBrowser::popupMenu(const QPoint &) {
  if (auto const item = m_browser->currentItem())
    displayPopupMenu(item->property());
  else
    displayDefaultMenu();
}

void FunctionBrowser::addFunction() {
  if (isFunction(m_selectedProperty) || m_selectedProperty == nullptr) {
    auto const function = getFunctionFromUserDialog();
    if (!function.isEmpty()) {
      auto const position = getFunctionPosition(m_selectedProperty);
      m_subscriber->addFunction(function.toStdString(), position);
    }
  }
}

void FunctionBrowser::removeFunction() {
  if (isFunction(m_selectedProperty))
    m_subscriber->removeFunction(getFunctionPosition(m_selectedProperty));
}

void FunctionBrowser::fixParameter() {
  if (m_selectedProperty && isParameter(m_selectedProperty))
    m_subscriber->fixParameter(
        m_selectedProperty->propertyName().toStdString());
}

void FunctionBrowser::removeTie() {
  if (m_selectedProperty && isParameter(m_selectedProperty))
    m_subscriber->removeTie(getNameOfProperty(m_selectedProperty));
}

void FunctionBrowser::addTie() {
  if (m_selectedProperty && isParameter(m_selectedProperty)) {
    auto const name = getNameOfProperty(m_selectedProperty);
    if (auto tie = getTieFromDialog())
      m_subscriber->setTie(name, tie->toStdString());
  }
}

void FunctionBrowser::addConstraints() {
  if (m_selectedProperty && isParameter(m_selectedProperty))
    m_subscriber->addConstraints(getNameOfProperty(m_selectedProperty), 0, 0);
}

void FunctionBrowser::removeConstraints() {
  if (m_selectedProperty && isParameter(m_selectedProperty))
    m_subscriber->removeConstraints(getNameOfProperty(m_selectedProperty));
}

void FunctionBrowser::addConstraints10() {
  if (m_selectedProperty && isParameter(m_selectedProperty))
    m_subscriber->addConstraints10(getNameOfProperty(m_selectedProperty));
}

void FunctionBrowser::addConstraints50() {
  if (m_selectedProperty && isParameter(m_selectedProperty))
    m_subscriber->addConstraints50(getNameOfProperty(m_selectedProperty));
}

void FunctionBrowser::removeConstraint() {
  if (m_selectedProperty && isConstraint(m_selectedProperty)) {
    auto const parameter =
        getNameOfProperty(getParentParameterProperty(m_selectedProperty));
    auto const type = m_selectedProperty->propertyName().toStdString();
    m_subscriber->removeConstraint(parameter, type);
    removeProperty(m_selectedProperty);
  }
}

void FunctionBrowser::copyFromClipboard() {
  try {
    m_subscriber->setFunction(QApplication::clipboard()->text().toStdString());
  } catch (...) {
    // text in the clipboard isn't a function definition
    QMessageBox::warning(this, "MantidPlot - Warning",
                         "Text in the clipboard isn't a function definition"
                         " or contains errors.");
  }
}

void FunctionBrowser::copyToClipboard() {
  m_subscriber->copyFunctionToClipboard();
}

void FunctionBrowser::copyToClipboard(std::string const &str) {
  QApplication::clipboard()->setText(QString::fromStdString(str));
}

void FunctionBrowser::displayCompositeMenu() {
  QMenu context(this);
  context.addAction(m_actionAddFunction);
  context.addAction(m_actionRemoveFunction);
  addClipboardActionsToMenu(context);
  context.exec(QCursor::pos());
}

void FunctionBrowser::displayFunctionMenu() {
  QMenu context(this);
  context.addAction(m_actionRemoveFunction);
  addClipboardActionsToMenu(context);
  context.exec(QCursor::pos());
}

void FunctionBrowser::displayParameterMenu(bool isTied, bool isConstrained) {
  QMenu context(this);
  if (isTied)
    context.addAction(m_actionRemoveTie);
  else
    addTieActionsToMenu(context);

  if (isConstrained)
    context.addAction(m_actionRemoveConstraints);
  else
    context.addMenu(getConstraintMenu().release());
  context.exec(QCursor::pos());
}

void FunctionBrowser::setSelectedProperty(QtBrowserItem *selectedItem) {
  if (selectedItem)
    m_selectedProperty = selectedItem->property();
  m_selectedProperty = getFirstProperty();
}

/**
 * Add a sub-property to a parent property
 * @param parent :: The parent property
 * @param subproperty :: New sub-property
 */
FunctionBrowser::AProperty
FunctionBrowser::addProperty(QtProperty *parent, QtProperty *subproperty) {
  AProperty ap;
  ap.prop = subproperty;
  if (parent == nullptr) {
    ap.item = m_browser->addProperty(subproperty);
  } else {
    parent->addSubProperty(subproperty);
    auto const items = m_browser->items(subproperty);
    if (items.isEmpty())
      throw std::runtime_error("Unexpected error in FunctionBrowser [1]");
    ap.item = items[0];
  }
  ap.parent = parent;
  m_properties[subproperty] = ap;
  return ap;
}

void FunctionBrowser::addLowerBoundProperty(QtProperty *parent,
                                            QString const &value) {
  auto const lowerBoundProperty =
      addProperty(parent, m_constraintManager->addProperty("LowerBound"));
  m_constraintManager->setValue(lowerBoundProperty.prop, value);
  m_properties[lowerBoundProperty.prop] = lowerBoundProperty;
}

void FunctionBrowser::addUpperBoundProperty(QtProperty *parent,
                                            QString const &value) {
  auto const upperBoundProperty =
      addProperty(parent, m_constraintManager->addProperty("UpperBound"));
  m_constraintManager->setValue(upperBoundProperty.prop, value);
  m_properties[upperBoundProperty.prop] = upperBoundProperty;
}

void FunctionBrowser::setLowerBoundProperty(QtProperty *parent,
                                            QString const &value) {
  if (auto const boundProperty = getLowerBoundProperty(parent))
    m_constraintManager->setValue(boundProperty, value);
  else
    addLowerBoundProperty(parent, value);
}

void FunctionBrowser::setUpperBoundProperty(QtProperty *parent,
                                            QString const &value) {
  if (auto const boundProperty = getUpperBoundProperty(parent))
    m_constraintManager->setValue(boundProperty, value);
  else
    addUpperBoundProperty(parent, value);
}

void FunctionBrowser::removeLowerBoundProperty(QtProperty *parent) {
  if (auto const boundProperty = getLowerBoundProperty(parent))
    removeProperty(boundProperty);
}

void FunctionBrowser::removeUpperBoundProperty(QtProperty *parent) {
  if (auto const boundProperty = getUpperBoundProperty(parent))
    removeProperty(boundProperty);
}

/**
 * Remove and delete property
 * @param prop :: Property to remove.
 */
void FunctionBrowser::removeProperty(QtProperty *prop) {
  auto p = m_properties.find(prop);
  if (p == m_properties.end())
    return;
  AProperty ap = *p;

  // remove references to the children
  auto children = prop->subProperties();
  foreach (QtProperty *child, children) { removeProperty(child); }
  m_properties.erase(p);

  if (isParameter(prop))
    m_parameterNameToProperty.remove(getParameterName(prop));
  else if (isAttribute(prop))
    m_attributeNameToProperty.remove(getIndex(prop) + prop->propertyName());

  // remove property from Qt browser
  if (ap.parent)
    ap.parent->removeSubProperty(prop);
  else
    m_browser->removeProperty(prop);
  delete prop;
}

/**
 * Add a function property
 * @param parent :: Parent function property or NULL
 * @param funName :: Function name
 * @return :: A set AProperty struct
 */
FunctionBrowser::AProperty
FunctionBrowser::addFunctionProperty(QtProperty *parent,
                                     QString const &functionName) {
  if (!isFunction(parent))
    throw std::runtime_error("Unexpected error in FunctionBrowser [2]");
  return addProperty(parent, m_functionManager->addProperty(functionName));
}

/**
 * Add a parameter property
 * @param parent :: Parent function property
 * @param paramName :: Parameter name
 * @param paramDesc :: Parameter description
 * @param paramValue :: Parameter value
 */
FunctionBrowser::AProperty FunctionBrowser::addParameterProperty(
    QtProperty *parent, QString const &paramName, QString const &paramDesc,
    double paramValue) {
  if (!isFunction(parent))
    throw std::runtime_error("Unexpected error in FunctionBrowser [3]");
  return addParameterProperty(m_parameterManager->addProperty(paramName),
                              parent, paramName, paramDesc, paramValue);
}

FunctionBrowser::AProperty FunctionBrowser::addParameterProperty(
    QtProperty *prop, QtProperty *parent, QString const &name,
    QString const &description, double value) {
  m_parameterManager->blockSignals(true);
  m_parameterManager->setDecimals(prop, 6);
  m_parameterManager->setValue(prop, value);
  m_parameterManager->setDescription(prop, description.toStdString());
  m_parameterNameToProperty[getIndex(parent) + name] = prop;
  return addProperty(parent, prop);
}

FunctionBrowser::AProperty
FunctionBrowser::addAttributeProperty(QtProperty *parent, QtProperty *prop) {
  m_attributeNameToProperty[getIndex(parent) + prop->propertyName()] = prop;
  return addProperty(parent, prop);
}

FunctionBrowser::AProperty
FunctionBrowser::addIndexProperty(QtProperty *parent, QString const &index) {
  if (!parent || !isFunction(parent) || !m_properties[parent].parent)
    return AProperty{nullptr, nullptr, nullptr};

  auto indexProperty = m_indexManager->addProperty("Index");
  indexProperty->setEnabled(false);
  m_indexManager->setValue(indexProperty, index);
  return addProperty(parent, indexProperty);
}

void FunctionBrowser::setIndicesOfFunctionsAt(
    QtProperty *parent, std::vector<std::string> const &indices) {
  setIndicesOfFunctionsIn(parent->subProperties(), indices);
}

void FunctionBrowser::setIndicesOfFunctionsIn(
    QList<QtProperty *> const &properties,
    std::vector<std::string> const &indices) {
  auto propertyIt = properties.begin();
  auto indexIt = indices.begin();

  while (propertyIt != properties.end() && indexIt != indices.end()) {
    if (isFunction(*propertyIt)) {
      setIndexPropertyOf(*propertyIt, QString::fromStdString(*indexIt));
      ++indexIt;
    }
    ++propertyIt;
  }
}

void FunctionBrowser::setIndexPropertyOf(QtProperty *prop,
                                         QString const &index) {
  if (auto const indexProperty = getIndexProperty(prop))
    m_indexManager->setValue(indexProperty, index);
}

/**
 * Add a tie property
 * @param prop :: Parent parameter property
 * @param tie :: A tie string
 */
void FunctionBrowser::addTieProperty(QtProperty *prop, QString tie) {
  if (!prop)
    throw std::runtime_error("FunctionBrowser: null property pointer");

  if (!isParameter(prop))
    return;

  // Create and add a QtProperty for the tie.
  m_tieManager->blockSignals(true);
  QtProperty *tieProp = m_tieManager->addProperty("Tie");
  m_tieManager->setValue(tieProp, tie);
  auto const ap = addProperty(prop, tieProp);
  m_tieManager->blockSignals(false);
  m_properties[tieProp] = ap;
}

QtProperty *
FunctionBrowser::getParameterProperty(std::string const &parameterName) const {
  auto const propertyIt =
      m_parameterNameToProperty.find(QString::fromStdString(parameterName));
  if (m_parameterNameToProperty.end() != propertyIt)
    return *propertyIt;
  return nullptr;
}

QtProperty *FunctionBrowser::getTieProperty(QtProperty *prop) const {
  for (auto const subProperty : prop->subProperties()) {
    if (isTie(subProperty))
      return subProperty;
  }
  return nullptr;
}

QtProperty *FunctionBrowser::getUpperBoundProperty(QtProperty *prop) const {
  for (auto const subProperty : prop->subProperties()) {
    if (isConstraint(subProperty) &&
        subProperty->propertyName() == "UpperBound")
      return subProperty;
  }
  return nullptr;
}

QtProperty *FunctionBrowser::getLowerBoundProperty(QtProperty *prop) const {
  for (auto const subProperty : prop->subProperties()) {
    if (isConstraint(subProperty) &&
        subProperty->propertyName() == "LowerBound")
      return subProperty;
  }
  return nullptr;
}

/// Get a property for a parameter which is a parent of a given
/// property (tie or constraint).
QtProperty *
FunctionBrowser::getParentParameterProperty(QtProperty *prop) const {
  auto const parameterProperty = m_properties[prop].parent;
  if (isParameter(parameterProperty))
    return parameterProperty;

  throw std::logic_error(
      "QtProperty " + prop->propertyName().toStdString() +
      " is not a child of a property for any function parameter.");
}

void FunctionBrowser::displayPopupMenu(QtProperty *prop) {
  if (isFunction(prop))
    m_subscriber->displayFunctionMenu(getFunctionPosition(prop));
  else if (isParameter(prop))
    m_subscriber->displayParameterMenu(getParameterName(prop).toStdString());
  else if (isConstraint(prop)) {
    QMenu context(this);
    context.addAction(m_actionRemoveConstraint);
    context.exec(QCursor::pos());
  }
}

void FunctionBrowser::displayDefaultMenu() {
  QMenu context(this);
  context.addAction(m_actionAddFunction);
  addClipboardActionsToMenu(context);
  context.exec(QCursor::pos());
}

void FunctionBrowser::addTieActionsToMenu(QMenu &menu) {
  menu.addAction(m_actionFixParameter);
  menu.addAction(m_actionAddTie);
}

void FunctionBrowser::addClipboardActionsToMenu(QMenu &menu) {
  if (!QApplication::clipboard()->text().isEmpty())
    menu.addAction(m_actionFromClipboard);
  if (!m_browser->properties().isEmpty())
    menu.addAction(m_actionToClipboard);
}

std::unique_ptr<QMenu> FunctionBrowser::getConstraintMenu() {
  auto constraintMenu = Mantid::Kernel::make_unique<QMenu>("Constraints", this);
  constraintMenu->addAction(m_actionConstraints10);
  constraintMenu->addAction(m_actionConstraints50);
  constraintMenu->addAction(m_actionConstraints);
  return std::unique_ptr<QMenu>(constraintMenu.release());
}

boost::optional<QString> FunctionBrowser::getTieFromDialog() {
  bool ok;
  QString tie = QInputDialog::getText(this, "Add a tie",
                                      "Tie:", QLineEdit::Normal, "", &ok);
  return ok ? boost::optional<QString>(tie) : boost::none;
}

QString FunctionBrowser::getFunctionFromUserDialog() const {
  SelectFunctionDialog dlg(m_browser);
  if (dlg.exec() == QDialog::Accepted)
    return dlg.getFunction();
  return "";
}

QtProperty *FunctionBrowser::getRootFunctionProperty() const { return nullptr; }

/**
 * Check if property is a function group
 * @param prop :: Property to check
 */
bool FunctionBrowser::isFunction(QtProperty *prop) const {
  return prop && dynamic_cast<QtAbstractPropertyManager *>(m_functionManager) ==
                     prop->propertyManager();
}

/**
 * Check if a property is a tie
 * @param prop :: A property
 */
bool FunctionBrowser::isTie(QtProperty *prop) const {
  return prop && dynamic_cast<QtAbstractPropertyManager *>(m_tieManager) ==
                     prop->propertyManager();
}

/**
 * Check if a property is a constraint
 * @param prop :: Property to check.
 */
bool FunctionBrowser::isConstraint(QtProperty *prop) const {
  return prop && dynamic_cast<QtAbstractPropertyManager *>(
                     m_constraintManager) == prop->propertyManager();
}

/**
 * Check if property is any of the string attributes
 * @param prop :: Property to check
 */
bool FunctionBrowser::isStringAttribute(QtProperty *prop) const {
  return prop &&
         (dynamic_cast<QtAbstractPropertyManager *>(m_attributeStringManager) ==
              prop->propertyManager() ||
          dynamic_cast<QtAbstractPropertyManager *>(m_formulaManager) ==
              prop->propertyManager() ||
          dynamic_cast<QtAbstractPropertyManager *>(m_filenameManager) ==
              prop->propertyManager() ||
          dynamic_cast<QtAbstractPropertyManager *>(m_workspaceManager) ==
              prop->propertyManager());
}

/**
 * Check if property is a function attribute
 * @param prop :: Property to check
 */
bool FunctionBrowser::isDoubleAttribute(QtProperty *prop) const {
  return prop && dynamic_cast<QtAbstractPropertyManager *>(
                     m_attributeDoubleManager) == prop->propertyManager();
}

/**
 * Check if property is a function attribute
 * @param prop :: Property to check
 */
bool FunctionBrowser::isIntAttribute(QtProperty *prop) const {
  return prop && dynamic_cast<QtAbstractPropertyManager *>(
                     m_attributeIntManager) == prop->propertyManager();
}

/**
 * Check if property is a function bool attribute
 * @param prop :: Property to check
 */
bool FunctionBrowser::isBoolAttribute(QtProperty *prop) const {
  return prop && dynamic_cast<QtAbstractPropertyManager *>(
                     m_attributeBoolManager) == prop->propertyManager();
}

/**
 * Check if property is a function vector attribute
 * @param prop :: Property to check
 */
bool FunctionBrowser::isVectorAttribute(QtProperty *prop) const {
  return prop && dynamic_cast<QtAbstractPropertyManager *>(
                     m_attributeVectorManager) == prop->propertyManager();
}

/**
 * Check if property is a function attribute
 * @param prop :: Property to check
 */
bool FunctionBrowser::isAttribute(QtProperty *prop) const {
  return isStringAttribute(prop) || isDoubleAttribute(prop) ||
         isIntAttribute(prop) || isBoolAttribute(prop) ||
         isVectorAttribute(prop);
}

/**
 * Check if property is a function parameter
 * @param prop :: Property to check
 */
bool FunctionBrowser::isParameter(QtProperty *prop) const {
  return prop && dynamic_cast<QtAbstractPropertyManager *>(
                     m_parameterManager) == prop->propertyManager();
}

/**
 * Check if a property is an index
 * @param prop :: A property
 */
bool FunctionBrowser::isIndex(QtProperty *prop) const {
  return prop && dynamic_cast<QtAbstractPropertyManager *>(m_indexManager) ==
                     prop->propertyManager();
}

std::string FunctionBrowser::getNameOfProperty(QtProperty *prop) const {
  return getParameterName(prop).toStdString();
}

QtProperty *FunctionBrowser::getFirstProperty() const {
  auto const top = m_browser->properties();
  return top.isEmpty() ? nullptr : top.front();
}

/**
 * Get name of the parameter for a property
 * @param prop :: A property
 */
QString FunctionBrowser::getParameterName(QtProperty *prop) const {
  return getIndex(prop) + prop->propertyName();
}

/**
 * Get property of the overall function.
 */
FunctionBrowser::AProperty FunctionBrowser::getFunctionProperty() const {
  auto const properties = m_browser->properties();
  if (properties.isEmpty())
    return AProperty{nullptr, nullptr, nullptr};
  return m_properties[properties.front()];
}

QtProperty *
FunctionBrowser::getContainingFunctionProperty(QtProperty *prop) const {
  auto aPropIt = m_properties.find(prop);
  while (aPropIt != m_properties.end()) {
    auto const parent = aPropIt->parent;
    if (isFunction(parent))
      return parent;
    aPropIt = m_properties.find(parent);
  }
  return nullptr;
}

QtProperty *FunctionBrowser::getFunctionPropertyAt(
    std::vector<std::size_t> const &position) const {
  return getFunctionPropertyAt(getRootFunctionProperty(), position);
}

QtProperty *FunctionBrowser::getFunctionPropertyAt(
    QtProperty *start, std::vector<size_t> const &position) const {
  if (position.empty())
    return start;
  else if (start == nullptr)
    return getFunctionPropertyAt(m_browser->properties(), position);
  return getFunctionPropertyAt(start->subProperties(), position);
}

QtProperty *FunctionBrowser::getFunctionPropertyAt(
    QList<QtProperty *> properties, std::vector<size_t> const &position) const {
  for (auto it = position.begin(); it < position.end() - 1; ++it) {
    auto const prop = getFunctionPropertyAt(properties, *it);
    if (nullptr == prop)
      return nullptr;
    properties = prop->subProperties();
  }
  return getFunctionPropertyAt(properties, position.back());
}

QtProperty *
FunctionBrowser::getFunctionPropertyAt(QList<QtProperty *> const &properties,
                                       std::size_t position) const {
  auto propIt =
      std::find_if(properties.begin(), properties.end(), [&](QtProperty *prop) {
        return isFunction(prop) && position-- == 0;
      });
  return properties.end() == propIt ? nullptr : *propIt;
}

std::size_t FunctionBrowser::getLocalFunctionIndex(QtProperty *parent,
                                                   QtProperty *prop) const {
  if (nullptr == parent)
    return getLocalFunctionIndex(m_browser->properties(), prop);
  return getLocalFunctionIndex(parent->subProperties(), prop);
}

std::size_t
FunctionBrowser::getLocalFunctionIndex(QList<QtProperty *> const &subProperties,
                                       QtProperty *prop) const {
  std::size_t index = 0;
  for (auto const &subProperty : subProperties) {
    if (subProperty == prop)
      return index;
    else if (isFunction(subProperty))
      ++index;
  }
  throw std::runtime_error("Unexpected error in FunctionBrowser [1]");
}

std::vector<std::size_t>
FunctionBrowser::getFunctionPosition(QtProperty *prop) const {
  if (!prop)
    return std::vector<std::size_t>();

  if (!isFunction(prop))
    prop = getContainingFunctionProperty(prop);

  std::vector<std::size_t> position;
  while (prop) {
    auto parent = m_properties[prop].parent;
    position.emplace_back(getLocalFunctionIndex(parent, prop));
    prop = parent;
  }
  return position;
}

QtProperty *
FunctionBrowser::getParameterPropertyIn(QtProperty *prop,
                                        QString const &parameter) const {
  for (auto const &prop : prop->subProperties()) {
    if (isParameter(prop) && prop->propertyName() == parameter)
      return prop;
  }
  return nullptr;
}

/**
 * Get the function index for a property
 * @param prop :: A property
 */
QString FunctionBrowser::getIndex(QtProperty *prop) const {
  if (auto const indexProperty = getIndexProperty(prop))
    return m_indexManager->value(indexProperty);
  return "";
}

QtProperty *FunctionBrowser::getIndexProperty(QtProperty *prop) const {
  if (isFunction(prop)) {
    if (auto const indexProperty = findIndexProperty(prop->subProperties()))
      return indexProperty;
    return nullptr;
  }
  return getIndexProperty(m_properties[prop].parent);
}

QtProperty *FunctionBrowser::findIndexProperty(
    QList<QtProperty *> const &properties) const {
  for (auto it = properties.begin(); it != properties.end(); ++it) {
    if (isIndex(*it))
      return *it;
  }
  return nullptr;
}

void FunctionBrowser::stringAttributeChanged(QtProperty *prop,
                                             QString const &value) {
  auto const name = getParameterName(prop).toStdString();
  m_subscriber->stringAttributeChanged(name, value.toStdString());
}

void FunctionBrowser::intAttributeChanged(QtProperty *prop, int value) {
  auto const name = getParameterName(prop).toStdString();
  m_subscriber->intAttributeChanged(name, value);
}

void FunctionBrowser::doubleAttributeChanged(QtProperty *prop, double value) {
  auto const name = getParameterName(prop).toStdString();
  m_subscriber->doubleAttributeChanged(name, value);
}

void FunctionBrowser::boolAttributeChanged(QtProperty *prop, bool value) {
  auto const name = getParameterName(prop).toStdString();
  m_subscriber->boolAttributeChanged(name, value);
}

void FunctionBrowser::vectorDoubleAttributeChanged(QtProperty *prop) {
  auto const name = getParameterName(prop).toStdString();
  auto const value =
      getVectorFromProperty<double>(prop, m_attributeVectorDoubleManager);
  m_subscriber->vectorDoubleAttributeChanged(name, value);
}

void FunctionBrowser::vectorSizeAttributeChanged(QtProperty *prop) {
  auto const vectorProperty = m_properties[prop].parent;
  if (!vectorProperty)
    throw std::logic_error("FunctionBrowser: inconsistency in vector "
                           "properties.\nAttribute property not found.");
  auto functionProperty = m_properties[vectorProperty].parent;
  if (!functionProperty)
    throw std::logic_error("FunctionBrowser: inconsistency in vector "
                           "properties.\nFunction property not found.");

  auto const attributeName = vectorProperty->propertyName().toStdString();
  m_subscriber->vectorSizeAttributeChanged(attributeName,
                                           m_attributeSizeManager->value(prop));
}

void FunctionBrowser::tieChanged(QtProperty *prop) {
  auto parameterProperty = m_properties[prop].parent;
  if (isParameter(parameterProperty))
    m_subscriber->tieChanged(parameterProperty->propertyName().toStdString(),
                             m_tieManager->value(prop).toStdString());
}

void FunctionBrowser::connectEditorCloseToBrowser(
    QtAbstractEditorFactoryBase *editor) {
  connect(editor, SIGNAL(closeEditor()), m_browser, SLOT(closeEditor()));
}

} // namespace Function
} // namespace MantidWidgets
} // namespace MantidQt
