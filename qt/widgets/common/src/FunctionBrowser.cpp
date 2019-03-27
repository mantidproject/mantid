// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FunctionBrowser.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/ParameterTie.h"

#include "MantidKernel/Logger.h"

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

#include <QApplication>
#include <QClipboard>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QMetaMethod>
#include <QPushButton>
#include <QSettings>
#include <QSignalMapper>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <boost/lexical_cast.hpp>

namespace {
const char *globalOptionName = "Global";
Mantid::Kernel::Logger g_log("Function Browser");
} // namespace

namespace MantidQt {
namespace MantidWidgets {

/**
 * Constructor
 * @param parent :: The parent widget.
 * @param multi  :: Option to use the browser for multi-dataset fitting.
 */
FunctionBrowser::FunctionBrowser(QWidget *parent, bool multi)
    : QWidget(parent), m_multiDataset(multi), m_numberOfDatasets(0),
      m_currentDataset(0)

{
  // create m_browser
  createBrowser();
  createActions();

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->addWidget(m_browser);
  layout->setContentsMargins(0, 0, 0, 0);
}

/**
 * Destructor
 */
FunctionBrowser::~FunctionBrowser() {}

/**
 * Create the Qt property browser and set up property managers.
 */
void FunctionBrowser::createBrowser() {
  QStringList options;
  if (m_multiDataset) {
    options << globalOptionName;
  }
  m_browser = new QtTreePropertyBrowser(nullptr, options);

  /* Create property managers: they create, own properties, get and set values
   */
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

  // create editor factories
  QtSpinBoxFactory *spinBoxFactory = new QtSpinBoxFactory(this);
  DoubleEditorFactory *doubleEditorFactory = new DoubleEditorFactory(this);
  ParameterEditorFactory *paramEditorFactory = new ParameterEditorFactory(this);

  QtAbstractEditorFactory<ParameterPropertyManager> *parameterEditorFactory(
      nullptr);
  if (m_multiDataset) {
    auto buttonFactory = new DoubleDialogEditorFactory(this);
    auto compositeFactory =
        new CompositeEditorFactory<ParameterPropertyManager>(this,
                                                             buttonFactory);
    compositeFactory->setSecondaryFactory(globalOptionName, paramEditorFactory);
    parameterEditorFactory = compositeFactory;
    connect(buttonFactory, SIGNAL(buttonClicked(QtProperty *)), this,
            SLOT(parameterButtonClicked(QtProperty *)));
    connect(buttonFactory, SIGNAL(closeEditor()), m_browser,
            SLOT(closeEditor()));
  } else {
    parameterEditorFactory = paramEditorFactory;
  }

  QtLineEditFactory *lineEditFactory = new QtLineEditFactory(this);
  QtCheckBoxFactory *checkBoxFactory = new QtCheckBoxFactory(this);
  FilenameDialogEditorFactory *filenameDialogEditorFactory =
      new FilenameDialogEditorFactory(this);
  FormulaDialogEditorFactory *formulaDialogEditFactory =
      new FormulaDialogEditorFactory(this);
  WorkspaceEditorFactory *workspaceEditorFactory =
      new WorkspaceEditorFactory(this);

  // assign factories to property managers
  m_browser->setFactoryForManager(m_parameterManager, parameterEditorFactory);
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

  m_browser->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_browser, SIGNAL(customContextMenuRequested(const QPoint &)), this,
          SLOT(popupMenu(const QPoint &)));
  connect(m_browser, SIGNAL(optionChanged(QtProperty *, const QString &, bool)),
          this, SLOT(globalChanged(QtProperty *, const QString &, bool)));

  connect(m_attributeStringManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(attributeChanged(QtProperty *)));
  connect(m_attributeDoubleManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(attributeChanged(QtProperty *)));
  connect(m_attributeIntManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(attributeChanged(QtProperty *)));
  connect(m_attributeBoolManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(attributeChanged(QtProperty *)));
  connect(m_formulaManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(attributeChanged(QtProperty *)));
  connect(m_filenameManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(attributeChanged(QtProperty *)));
  connect(m_workspaceManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(attributeChanged(QtProperty *)));
  connect(m_attributeVectorDoubleManager, SIGNAL(propertyChanged(QtProperty *)),
          this, SLOT(attributeVectorDoubleChanged(QtProperty *)));
  connect(m_attributeSizeManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(attributeVectorSizeChanged(QtProperty *)));
  connect(m_tieManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(tieChanged(QtProperty *)));
  connect(m_constraintManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(constraintChanged(QtProperty *)));
  connect(m_parameterManager, SIGNAL(valueChanged(QtProperty *, double)),
          SLOT(parameterChanged(QtProperty *)));

  connect(m_browser, SIGNAL(currentItemChanged(QtBrowserItem *)),
          SLOT(updateCurrentFunctionIndex()));

  m_browser->setFocusPolicy(Qt::StrongFocus);
}

/**
 * Create and connect actions
 */
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

  setErrorsEnabled(true);
}

/**
 * Clear the contents
 */
void FunctionBrowser::clear() {
  m_browser->clear();
  m_properties.clear();
}

/**
 * Set the function in the browser
 * @param funStr :: FunctionFactory function creation string
 */
void FunctionBrowser::setFunction(const QString &funStr) {
  if (funStr.isEmpty())
    return;
  try {
    auto fun = Mantid::API::FunctionFactory::Instance().createInitialized(
        funStr.toStdString());
    if (!fun)
      return;
    this->setFunction(fun);
  } catch (...) {
    // error in the input string
  }
}

/**
 * Set the function in the browser
 * @param fun :: A function
 */
void FunctionBrowser::setFunction(Mantid::API::IFunction_sptr fun) {
  clear();
  addFunction(nullptr, fun);
  emit functionStructureChanged();
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
    auto items = m_browser->items(subproperty);
    if (items.isEmpty()) {
      throw std::runtime_error("Unexpected error in FunctionBrowser [1]");
    }
    ap.item = items[0];
  }
  ap.parent = parent;
  m_properties[subproperty] = ap;
  return ap;
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

  if (isFunction(prop)) {
    m_ties.remove(prop);
  }

  if (isTie(prop)) { //
    for (auto it = m_ties.begin(); it != m_ties.end(); ++it) {
      if (it.value().tieProp == prop) {
        m_ties.erase(it);
        break;
      }
    }
  }

  if (isConstraint(prop)) {
    for (auto it = m_constraints.begin(); it != m_constraints.end(); ++it) {
      auto &cp = it.value();
      if (cp.lower == prop) {
        if (!cp.upper) {
          m_constraints.erase(it);
        } else {
          cp.lower = nullptr;
        }
        break;
      } else if (cp.upper == prop) {
        if (!cp.lower) {
          m_constraints.erase(it);
        } else {
          cp.upper = nullptr;
        }
        break;
      }
    }
  }

  // remove property from Qt browser
  if (ap.parent) {
    ap.parent->removeSubProperty(prop);
  } else {
    m_browser->removeProperty(prop);
  }
  delete prop;
}

/**
 * Add a function property
 * @param parent :: Parent function property or NULL
 * @param funName :: Function name
 * @return :: A set AProperty struct
 */
FunctionBrowser::AProperty
FunctionBrowser::addFunctionProperty(QtProperty *parent, QString funName) {
  // check that parent is a function property
  if (parent && dynamic_cast<QtAbstractPropertyManager *>(m_functionManager) !=
                    parent->propertyManager()) {
    throw std::runtime_error("Unexpected error in FunctionBrowser [2]");
  }
  QtProperty *prop = m_functionManager->addProperty(funName);
  return addProperty(parent, prop);
}

/**
 * Add a parameter property
 * @param parent :: Parent function property
 * @param paramName :: Parameter name
 * @param paramDesc :: Parameter description
 * @param paramValue :: Parameter value
 */
FunctionBrowser::AProperty
FunctionBrowser::addParameterProperty(QtProperty *parent, QString paramName,
                                      QString paramDesc, double paramValue) {
  // check that parent is a function property
  if (!parent || dynamic_cast<QtAbstractPropertyManager *>(m_functionManager) !=
                     parent->propertyManager()) {
    throw std::runtime_error("Unexpected error in FunctionBrowser [3]");
  }
  QtProperty *prop = m_parameterManager->addProperty(paramName);
  m_parameterManager->blockSignals(true);
  m_parameterManager->setDecimals(prop, 6);
  m_parameterManager->setValue(prop, paramValue);
  m_parameterManager->setDescription(prop, paramDesc.toStdString());
  m_parameterManager->blockSignals(false);

  if (m_multiDataset) {
    prop->setOption(globalOptionName, false);
  }
  return addProperty(parent, prop);
}

/**
 * Set a function.
 * @param prop :: Property of the function or NULL
 * @param fun :: A function
 */
void FunctionBrowser::setFunction(QtProperty *prop,
                                  Mantid::API::IFunction_sptr fun) {
  auto children = prop->subProperties();
  foreach (QtProperty *child, children) { removeProperty(child); }
  m_localParameterValues.clear();
  addAttributeAndParameterProperties(prop, fun);
}

/**
 * Add a function.
 * @param prop :: Property of the parent composite function or NULL
 * @param fun :: FunctionFactory function creation string
 */
void FunctionBrowser::addFunction(QtProperty *prop,
                                  Mantid::API::IFunction_sptr fun) {
  if (!prop) {
    AProperty ap =
        addFunctionProperty(nullptr, QString::fromStdString(fun->name()));
    setFunction(ap.prop, fun);
  } else {
    Mantid::API::IFunction_sptr parentFun = getFunction(prop);
    if (!parentFun)
      return;
    auto cf =
        boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(parentFun);
    if (!cf) {
      throw std::runtime_error(
          "FunctionBrowser: CompositeFunction is expected for addFunction");
    }
    cf->addFunction(fun);
    setFunction(prop, cf);
  }
  updateFunctionIndices();
}

/**
 * Attribute visitor to create a QtProperty. Depending on the attribute type
 * the appropriate apply() method is used.
 */
class CreateAttributePropertyForFunctionBrowser
    : public Mantid::API::IFunction::ConstAttributeVisitor<
          FunctionBrowser::AProperty> {
public:
  CreateAttributePropertyForFunctionBrowser(FunctionBrowser *browser,
                                            QtProperty *parent, QString attName)
      : m_browser(browser), m_parent(parent), m_attName(attName) {
    // check that parent is a function property
    if (!m_parent ||
        dynamic_cast<QtAbstractPropertyManager *>(
            m_browser->m_functionManager) != m_parent->propertyManager()) {
      throw std::runtime_error("Unexpected error in FunctionBrowser [4]");
    }
  }

protected:
  /// Create string property
  FunctionBrowser::AProperty apply(const std::string &str) const override {
    QtProperty *prop = nullptr;
    if (m_attName == "FileName") {
      prop = m_browser->m_filenameManager->addProperty(m_attName);
      m_browser->m_filenameManager->setValue(prop, QString::fromStdString(str));
    } else if (m_attName == "Formula") {
      prop = m_browser->m_formulaManager->addProperty(m_attName);
      m_browser->m_formulaManager->setValue(prop, QString::fromStdString(str));
    } else if (m_attName == "Workspace") {
      prop = m_browser->m_workspaceManager->addProperty(m_attName);
      m_browser->m_workspaceManager->setValue(prop,
                                              QString::fromStdString(str));
    } else {
      prop = m_browser->m_attributeStringManager->addProperty(m_attName);
      m_browser->m_attributeStringManager->setValue(
          prop, QString::fromStdString(str));
    }
    return m_browser->addProperty(m_parent, prop);
  }
  /// Create double property
  FunctionBrowser::AProperty apply(const double &d) const override {
    QtProperty *prop =
        m_browser->m_attributeDoubleManager->addProperty(m_attName);
    m_browser->m_attributeDoubleManager->setValue(prop, d);
    return m_browser->addProperty(m_parent, prop);
  }
  /// Create int property
  FunctionBrowser::AProperty apply(const int &i) const override {
    QtProperty *prop = m_browser->m_attributeIntManager->addProperty(m_attName);
    m_browser->m_attributeIntManager->setValue(prop, i);
    return m_browser->addProperty(m_parent, prop);
  }
  /// Create bool property
  FunctionBrowser::AProperty apply(const bool &b) const override {
    QtProperty *prop =
        m_browser->m_attributeBoolManager->addProperty(m_attName);
    m_browser->m_attributeBoolManager->setValue(prop, b);
    return m_browser->addProperty(m_parent, prop);
  }
  /// Create vector property
  FunctionBrowser::AProperty
  apply(const std::vector<double> &v) const override {
    QtProperty *prop =
        m_browser->m_attributeVectorManager->addProperty(m_attName);
    FunctionBrowser::AProperty aprop = m_browser->addProperty(m_parent, prop);

    m_browser->m_attributeSizeManager->blockSignals(true);
    QtProperty *sizeProp =
        m_browser->m_attributeSizeManager->addProperty("Size");
    m_browser->m_attributeSizeManager->setValue(sizeProp,
                                                static_cast<int>(v.size()));
    m_browser->addProperty(prop, sizeProp);
    m_browser->m_attributeSizeManager->blockSignals(false);
    sizeProp->setEnabled(false);

    m_browser->m_attributeVectorDoubleManager->blockSignals(true);
    QString parName = "value[%1]";
    for (size_t i = 0; i < v.size(); ++i) {
      QtProperty *dprop =
          m_browser->m_attributeVectorDoubleManager->addProperty(
              parName.arg(i));
      m_browser->m_attributeVectorDoubleManager->setValue(dprop, v[i]);
      m_browser->addProperty(prop, dprop);
    }
    m_browser->m_attributeVectorDoubleManager->blockSignals(false);

    m_browser->m_browser->setExpanded(aprop.item, false);
    return aprop;
  }

private:
  FunctionBrowser *m_browser;
  QtProperty *m_parent;
  QString m_attName;
};

/**
 * Attribute visitor to set an attribute from a QtProperty. Depending on the
 * attribute type
 * the appropriate apply() method is used.
 */
class SetAttributeFromProperty
    : public Mantid::API::IFunction::AttributeVisitor<> {
public:
  SetAttributeFromProperty(FunctionBrowser *browser, QtProperty *prop)
      : m_browser(browser), m_prop(prop) {}

protected:
  /// Set string attribute
  void apply(std::string &str) const override {
    QString attName = m_prop->propertyName();
    if (attName == "FileName") {
      str = m_browser->m_filenameManager->value(m_prop).toStdString();
    } else if (attName == "Formula") {
      str = m_browser->m_formulaManager->value(m_prop).toStdString();
    } else if (attName == "Workspace") {
      str = m_browser->m_workspaceManager->value(m_prop).toStdString();
    } else {
      str = m_browser->m_attributeStringManager->value(m_prop).toStdString();
    }
  }
  /// Set double attribute
  void apply(double &d) const override {
    d = m_browser->m_attributeDoubleManager->value(m_prop);
  }
  /// Set int attribute
  void apply(int &i) const override {
    i = m_browser->m_attributeIntManager->value(m_prop);
  }
  /// Set bool attribute
  void apply(bool &b) const override {
    b = m_browser->m_attributeBoolManager->value(m_prop);
  }
  /// Set vector attribute
  void apply(std::vector<double> &v) const override {
    // throw std::runtime_error("Vector setter not implemented.");
    QList<QtProperty *> members = m_prop->subProperties();
    if (members.empty())
      throw std::runtime_error(
          "FunctionBrowser: empty vector attribute group.");
    int n = members.size() - 1;
    if (n == 0) {
      v.clear();
      return;
    }
    v.resize(n);
    for (int i = 0; i < n; ++i) {
      v[i] = m_browser->m_attributeVectorDoubleManager->value(members[i + 1]);
    }
  }

private:
  FunctionBrowser *m_browser;
  QtProperty *m_prop;
};

/**
 * Add a attribute property
 * @param parent :: Parent function property
 * @param attName :: Attribute name
 * @param att :: Attribute value
 */
FunctionBrowser::AProperty FunctionBrowser::addAttributeProperty(
    QtProperty *parent, QString attName,
    const Mantid::API::IFunction::Attribute &att) {
  CreateAttributePropertyForFunctionBrowser cap(this, parent, attName);
  return att.apply(cap);
}

/**
 * Add attribute and parameter properties to a function property. For a
 * composite function
 *  adds all member functions' properties
 * @param prop :: A function property
 * @param fun :: Shared pointer to a created function
 */
void FunctionBrowser::addAttributeAndParameterProperties(
    QtProperty *prop, Mantid::API::IFunction_sptr fun) {
  // add the function index property
  addIndexProperty(prop);

  // add attribute properties
  auto attributeNames = fun->getAttributeNames();
  for (auto att = attributeNames.begin(); att != attributeNames.end(); ++att) {
    QString attName = QString::fromStdString(*att);
    addAttributeProperty(prop, attName, fun->getAttribute(*att));
  }

  auto cf = boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(fun);
  if (cf) { // if composite add members
    for (size_t i = 0; i < cf->nFunctions(); ++i) {
      AProperty ap = addFunctionProperty(
          prop, QString::fromStdString(cf->getFunction(i)->name()));
      addAttributeAndParameterProperties(ap.prop, cf->getFunction(i));
    }
  } else { // if simple add parameters
    for (size_t i = 0; i < fun->nParams(); ++i) {
      QString name = QString::fromStdString(fun->parameterName(i));
      QString desc = QString::fromStdString(fun->parameterDescription(i));
      double value = fun->getParameter(i);
      AProperty ap = addParameterProperty(prop, name, desc, value);
      // if parameter has a tie
      if (!fun->isActive(i)) {
        auto tie = fun->getTie(i);
        if (tie) {
          addTieProperty(ap.prop, QString::fromStdString(tie->asString()));
        } else {
          addTieProperty(ap.prop, QString::number(fun->getParameter(i)));
        }
      }
      auto c = fun->getConstraint(i);
      if (c) {
        addConstraintProperties(ap.prop, QString::fromStdString(c->asString()));
      }
    }
  }
}

/**
 * Add property showing function's index in the composite function
 * @param prop :: A function property
 * @return :: AProperty struct for added property. If all fields are NULL -
 * property wasn't added
 *  because it is the top function
 */
FunctionBrowser::AProperty FunctionBrowser::addIndexProperty(QtProperty *prop) {
  AProperty ap;
  ap.item = nullptr;
  ap.parent = nullptr;
  ap.prop = nullptr;
  if (!prop)
    return ap;
  if (!isFunction(prop))
    return ap;
  if (!m_properties[prop].parent)
    return ap;

  QString index = "fff";
  QtProperty *ip = m_indexManager->addProperty("Index");
  ip->setEnabled(false);
  m_indexManager->setValue(ip, index);
  auto retval = addProperty(prop, ip);
  updateFunctionIndices();
  return retval;
}

/**
 * Update function index properties
 * @param prop :: A function property
 * @param index :: The parent function's index
 */
void FunctionBrowser::updateFunctionIndices(QtProperty *prop, QString index) {
  if (prop == nullptr) {
    auto top = m_browser->properties();
    if (top.isEmpty())
      return;
    prop = top[0];
  }
  auto children = prop->subProperties();
  size_t i = 0;
  foreach (QtProperty *child, children) {
    if (isFunction(child)) {
      updateFunctionIndices(child, index + "f" + QString::number(i) + ".");
      ++i;
    } else if (isIndex(child)) {
      m_indexManager->setValue(child, index);
    }
  }
}

/**
 * Get property of the overall function.
 */
FunctionBrowser::AProperty FunctionBrowser::getFunctionProperty() const {
  auto props = m_browser->properties();
  if (props.isEmpty()) {
    AProperty ap;
    ap.item = nullptr;
    ap.parent = nullptr;
    ap.prop = nullptr;
    return ap;
  }
  QtProperty *prop = props[0];
  return m_properties[prop];
}

/**
 * Get a list of names of global parameters
 */
QStringList FunctionBrowser::getGlobalParameters() const {
  QStringList out;
  for (auto propIt = m_properties.begin(); propIt != m_properties.end();
       ++propIt) {
    QtProperty *prop = propIt->prop;
    if (isGlobalParameterProperty(prop)) {
      out << getIndex(prop) + prop->propertyName();
    }
  }
  return out;
}
/**
 * Get a list of names of global parameters
 */
void FunctionBrowser::setGlobalParameters(QStringList &globals) {
  for (auto propIt = m_properties.begin(); propIt != m_properties.end();
       ++propIt) {
    QtProperty *prop = propIt->prop;
    QString tmp = getIndex(prop) + prop->propertyName();
    for (auto &global : globals) {
      if (tmp == global) {
        prop->setOption(globalOptionName, true);
      }
    }
  }
}
/**
 * Get a list of names of local parameters
 */
QStringList FunctionBrowser::getLocalParameters() const {
  QStringList out;
  for (auto propIt = m_properties.begin(); propIt != m_properties.end();
       ++propIt) {
    QtProperty *prop = propIt->prop;
    if (isLocalParameterProperty(prop)) {
      out << getIndex(prop) + prop->propertyName();
    }
  }
  return out;
}

/**
 * Check if property is a function group
 * @param prop :: Property to check
 */
bool FunctionBrowser::isFunction(QtProperty *prop) const {
  return prop && dynamic_cast<QtAbstractPropertyManager *>(m_functionManager) ==
                     prop->propertyManager();
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
 * Get parameter value as a string
 * @param prop :: A parameter property
 */
double FunctionBrowser::getParameter(QtProperty *prop) const {
  return m_parameterManager->value(prop);
}

/**
 * Check if a property is an index
 * @param prop :: A property
 */
bool FunctionBrowser::isIndex(QtProperty *prop) const {
  return prop && dynamic_cast<QtAbstractPropertyManager *>(m_indexManager) ==
                     prop->propertyManager();
}

/**
 * Get the function index for a property
 * @param prop :: A property
 */
QString FunctionBrowser::getIndex(QtProperty *prop) const {
  if (!prop)
    return "";
  if (isFunction(prop)) {
    auto props = prop->subProperties();
    if (props.isEmpty())
      return "";
    for (auto it = props.begin(); it != props.end(); ++it) {
      if (isIndex(*it)) {
        return m_indexManager->value(*it);
      }
    }
    return "";
  }

  auto ap = m_properties[prop];
  return getIndex(ap.parent);
}

/**
 * Get name of the parameter for a property
 * @param prop :: A property
 */
QString FunctionBrowser::getParameterName(QtProperty *prop) {
  return getIndex(prop) + prop->propertyName();
}

/**
 * Return function property for a function with given index.
 * @param index :: Function index to search, or empty string for top-level
 * function
 * @return Function property, or NULL if not found
 */
QtProperty *FunctionBrowser::getFunctionProperty(const QString &index) const {
  // Might not be the most efficient way to do it. m_functionManager might be
  // searched instead,
  // but it is not being kept up-to-date at the moment (is not cleared).
  foreach (auto property, m_properties.keys()) {
    if (isFunction(property) && getIndex(property) == index) {
      return property;
    }
  }

  // No function with such index
  return nullptr;
}

/**
 * Add a tie property
 * @param prop :: Parent parameter property
 * @param tie :: A tie string
 */
void FunctionBrowser::addTieProperty(QtProperty *prop, QString tie) {
  if (!prop) {
    throw std::runtime_error("FunctionBrowser: null property pointer");
  }
  AProperty ap;
  ap.item = nullptr;
  ap.prop = nullptr;
  ap.parent = nullptr;

  if (!isParameter(prop))
    return;

  QtProperty *funProp = getFunctionProperty().prop;

  // Create and add a QtProperty for the tie.
  m_tieManager->blockSignals(true);
  QtProperty *tieProp = m_tieManager->addProperty("Tie");
  m_tieManager->setValue(tieProp, tie);
  ap = addProperty(prop, tieProp);
  m_tieManager->blockSignals(false);

  const auto parName = getParameterName(prop);
  // Store tie information for easier access
  ATie atie;
  atie.paramProp = prop;
  atie.paramName = parName;
  atie.tieProp = tieProp;
  m_ties.insert(funProp, atie);

  // In case of multi-dataset fitting store the tie for a local parameter
  if (isLocalParameterProperty(prop)) {
    auto &localValues = m_localParameterValues[parName];
    if (m_currentDataset >= localValues.size()) {
      initLocalParameter(parName);
    }
    localValues[m_currentDataset].tie = tie;
  }
}

/**
 * Check if a parameter property has a tie
 * @param prop :: A parameter property
 */
bool FunctionBrowser::hasTie(QtProperty *prop) const {
  if (!prop)
    return false;
  auto children = prop->subProperties();
  foreach (QtProperty *child, children) {
    if (child->propertyName() == "Tie") {
      return true;
    }
  }
  return false;
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
 * Get a tie for a parameter
 * @param prop :: A parameter property
 */
std::string FunctionBrowser::getTie(QtProperty *prop) const {
  if (!prop)
    return "";
  auto children = prop->subProperties();
  foreach (QtProperty *child, children) {
    if (child->propertyName() == "Tie") {
      return m_tieManager->value(child).toStdString();
    }
  }
  return "";
}

/**
 * Add a constraint property
 * @param prop :: Parent parameter property
 * @param constraint :: A constraint string
 */
QList<FunctionBrowser::AProperty>
FunctionBrowser::addConstraintProperties(QtProperty *prop, QString constraint) {
  if (!isParameter(prop))
    return QList<FunctionBrowser::AProperty>();
  QString lowerBoundStr = "";
  QString upperBoundStr = "";
  Mantid::API::Expression expr;
  expr.parse(constraint.toStdString());
  if (expr.name() != "==")
    return QList<FunctionBrowser::AProperty>();
  if (expr.size() == 3) { // lower < param < upper
    try {
      // check that the first and third terms are numbers
      double d1 = boost::lexical_cast<double>(expr[0].name());
      (void)d1;
      double d2 = boost::lexical_cast<double>(expr[2].name());
      (void)d2;
      if (expr[1].operator_name() == "<" && expr[2].operator_name() == "<") {
        lowerBoundStr = QString::fromStdString(expr[0].name());
        upperBoundStr = QString::fromStdString(expr[2].name());
      } else // assume that the operators are ">"
      {
        lowerBoundStr = QString::fromStdString(expr[2].name());
        upperBoundStr = QString::fromStdString(expr[0].name());
      }
    } catch (...) { // error in constraint
      return QList<FunctionBrowser::AProperty>();
    }
  } else if (expr.size() == 2) { // lower < param or param > lower etc
    size_t paramPos = 0;
    try // find position of the parameter name in expression
    {
      double d = boost::lexical_cast<double>(expr[1].name());
      (void)d;
    } catch (...) {
      paramPos = 1;
    }
    std::string op = expr[1].operator_name();
    if (paramPos == 0) { // parameter goes first
      if (op == "<") {   // param < number
        upperBoundStr = QString::fromStdString(expr[1].name());
      } else { // param > number
        lowerBoundStr = QString::fromStdString(expr[1].name());
      }
    } else {           // parameter is second
      if (op == "<") { // number < param
        lowerBoundStr = QString::fromStdString(expr[0].name());
      } else { // number > param
        upperBoundStr = QString::fromStdString(expr[0].name());
      }
    }
  }

  // add properties
  QList<FunctionBrowser::AProperty> plist;
  AConstraint ac;
  ac.paramProp = prop;
  ac.lower = ac.upper = nullptr;
  if (!lowerBoundStr.isEmpty()) {
    auto ap = addProperty(prop, m_constraintManager->addProperty("LowerBound"));
    plist << ap;
    ac.lower = ap.prop;
    m_constraintManager->setValue(ac.lower, lowerBoundStr);
  }
  if (!upperBoundStr.isEmpty()) {
    auto ap = addProperty(prop, m_constraintManager->addProperty("UpperBound"));
    plist << ap;
    ac.upper = ap.prop;
    m_constraintManager->setValue(ac.upper, upperBoundStr);
  }
  if (ac.lower || ac.upper) {
    m_constraints.insert(m_properties[prop].parent, ac);
  }

  // In case of multi-dataset fitting store the tie for a local parameter
  if (isLocalParameterProperty(prop)) {
    const auto parName = getParameterName(prop);
    checkLocalParameter(parName);
    auto &localValues = m_localParameterValues[parName][m_currentDataset];
    localValues.lowerBound = lowerBoundStr;
    localValues.upperBound = upperBoundStr;
  }

  return plist;
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
 * Check if a parameter property has a constraint
 * @param prop :: A parameter property.
 */
bool FunctionBrowser::hasConstraint(QtProperty *prop) const {
  return hasLowerBound(prop) || hasUpperBound(prop);
}

/**
 * Check if a parameter property has a lower bound
 * @param prop :: A parameter property.
 */
bool FunctionBrowser::hasLowerBound(QtProperty *prop) const {
  if (!isParameter(prop))
    return false;
  auto props = prop->subProperties();
  if (props.isEmpty())
    return false;
  foreach (QtProperty *p, props) {
    if (dynamic_cast<QtAbstractPropertyManager *>(m_constraintManager) ==
            p->propertyManager() &&
        p->propertyName() == "LowerBound")
      return true;
  }
  return false;
}

/**
 * Check if a parameter property has a upper bound
 * @param prop :: A parameter property.
 */
bool FunctionBrowser::hasUpperBound(QtProperty *prop) const {
  if (!isParameter(prop))
    return false;
  auto props = prop->subProperties();
  if (props.isEmpty())
    return false;
  foreach (QtProperty *p, props) {
    if (dynamic_cast<QtAbstractPropertyManager *>(m_constraintManager) ==
            p->propertyManager() &&
        p->propertyName() == "UpperBound")
      return true;
  }
  return false;
}

/// Get a constraint string
QString FunctionBrowser::getConstraint(const QString &paramName,
                                       const QString &lowerBound,
                                       const QString &upperBound) const {
  QString constraint;
  if (!lowerBound.isEmpty()) {
    constraint += lowerBound + "<";
  }
  constraint += paramName;
  if (!upperBound.isEmpty()) {
    constraint += "<" + upperBound;
  }
  return constraint;
}

/**
 * Show a pop up menu.
 */
void FunctionBrowser::popupMenu(const QPoint & /*unused*/) {
  auto item = m_browser->currentItem();
  if (!item) {
    QMenu context(this);
    context.addAction(m_actionAddFunction);
    if (!QApplication::clipboard()->text().isEmpty()) {
      context.addAction(m_actionFromClipboard);
    }
    if (!m_browser->properties().isEmpty()) {
      context.addAction(m_actionToClipboard);
    }
    context.exec(QCursor::pos());
    return;
  }
  QtProperty *prop = item->property();
  if (isFunction(prop)) { // functions
    QMenu context(this);
    Mantid::API::IFunction_sptr fun =
        Mantid::API::FunctionFactory::Instance().createFunction(
            prop->propertyName().toStdString());
    auto cf = boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(fun);
    if (cf || m_properties[prop].parent == nullptr) {
      context.addAction(m_actionAddFunction);
    }
    context.addAction(m_actionRemoveFunction);
    if (!QApplication::clipboard()->text().isEmpty()) {
      context.addAction(m_actionFromClipboard);
    }
    if (!m_browser->properties().isEmpty()) {
      context.addAction(m_actionToClipboard);
    }
    context.exec(QCursor::pos());
  } else if (isParameter(prop)) { // parameters
    QMenu context(this);
    if (hasTie(prop)) {
      context.addAction(m_actionRemoveTie);
    } else {
      context.addAction(m_actionFixParameter);
      context.addAction(m_actionAddTie);
    }
    bool hasLower = hasLowerBound(prop);
    bool hasUpper = hasUpperBound(prop);
    if (!hasLower && !hasUpper) {
      QMenu *constraintMenu = new QMenu("Constraints", this);
      constraintMenu->addAction(m_actionConstraints10);
      constraintMenu->addAction(m_actionConstraints50);
      constraintMenu->addAction(m_actionConstraints);
      context.addMenu(constraintMenu);
    } else {
      context.addAction(m_actionRemoveConstraints);
    }
    context.exec(QCursor::pos());
  } else if (isConstraint(prop)) { // constraints
    QMenu context(this);
    context.addAction(m_actionRemoveConstraint);
    context.exec(QCursor::pos());
  }
}

/**
 * Add a function to currently selected composite function property
 */
void FunctionBrowser::addFunction() {
  QString newFunction;

  auto item = m_browser->currentItem();
  QtProperty *prop = nullptr;
  if (item) {
    prop = item->property();
    if (!isFunction(prop))
      return;
  }

  // check if the browser is empty
  if (!prop) {
    auto top = m_browser->properties();
    if (!top.isEmpty()) {
      prop = top[0];
      if (!isFunction(prop))
        return;
    }
  }

  // Get new function type
  newFunction = getUserFunctionFromDialog();
  if (newFunction.isEmpty())
    return;

  // create new function
  auto f = Mantid::API::FunctionFactory::Instance().createFunction(
      newFunction.toStdString());

  if (prop) { // there are other functions defined
    Mantid::API::IFunction_sptr fun =
        Mantid::API::FunctionFactory::Instance().createFunction(
            prop->propertyName().toStdString());
    auto cf = boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(fun);
    if (cf) {
      addFunction(prop, f);
    } else {
      cf.reset(new Mantid::API::CompositeFunction);
      auto f0 = getFunction(prop);
      if (f0) {
        cf->addFunction(f0);
      }
      cf->addFunction(f);
      setFunction(cf);
    }
  } else { // the browser is empty - add first function
    addFunction(nullptr, f);
  }
  emit functionStructureChanged();
}

/**
 * Ask user to select a function and return it
 * @returns :: function string
 */
QString FunctionBrowser::getUserFunctionFromDialog() {
  SelectFunctionDialog dlg(this);
  if (dlg.exec() == QDialog::Accepted) {
    return dlg.getFunction();
  } else {
    return QString();
  }
}

/**
 * Set value of an attribute (as a property) to a function.
 * @param fun :: Function to which attribute is set.
 * @param prop :: A property with the name and value of the attribute.
 */
void FunctionBrowser::setAttributeToFunction(Mantid::API::IFunction &fun,
                                             QtProperty *prop) {
  std::string attName = prop->propertyName().toStdString();
  SetAttributeFromProperty setter(this, prop);
  Mantid::API::IFunction::Attribute attr = fun.getAttribute(attName);
  attr.apply(setter);
  try {
    fun.setAttribute(attName, attr);
  } catch (std::exception &expt) {
    QMessageBox::critical(this, "MantidPlot - Error",
                          "Cannot set attribute " +
                              QString::fromStdString(attName) +
                              " of function " + prop->propertyName() + ":\n\n" +
                              QString::fromStdString(expt.what()));
  }
}

/**
 * Return the function
 * @param prop :: Function property
 * @param attributesOnly :: Only set attributes
 */
Mantid::API::IFunction_sptr FunctionBrowser::getFunction(QtProperty *prop,
                                                         bool attributesOnly) {
  if (prop == nullptr) { // get overall function
    auto props = m_browser->properties();
    if (props.isEmpty())
      return Mantid::API::IFunction_sptr();
    prop = props[0];
  }
  if (!isFunction(prop))
    return Mantid::API::IFunction_sptr();

  // construct the function
  auto fun = Mantid::API::FunctionFactory::Instance().createFunction(
      prop->propertyName().toStdString());
  auto cf = boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(fun);
  if (cf) {
    auto children = prop->subProperties();
    foreach (QtProperty *child, children) {
      if (isFunction(child)) {
        auto f = getFunction(child);
        // if f is null ignore that function
        if (f) {
          cf->addFunction(f);
        }
      } else if (isAttribute(child)) {
        setAttributeToFunction(*fun, child);
      }
    }
  } else {
    // loop over the children properties and set parameters and attributes
    auto children = prop->subProperties();
    foreach (QtProperty *child, children) {
      if (isAttribute(child)) {
        setAttributeToFunction(*fun, child);
      } else if (!attributesOnly && isParameter(child)) {
        fun->setParameter(child->propertyName().toStdString(),
                          getParameter(child));
      }
    }
  }

  // if this flag is set the function requires attributes only
  // attempts to set other properties may result in exceptions
  if (attributesOnly)
    return fun;

  // add ties
  {
    auto from = m_ties.lowerBound(prop);
    auto to = m_ties.upperBound(prop);
    // ties can become invalid after some editing
    QList<QtProperty *> failedTies;
    for (auto it = from; it != to; ++it) {
      auto const tie =
          (it->paramName + "=" + m_tieManager->value(it.value().tieProp))
              .toStdString();
      try {
        fun->addTies(tie);
      } catch (...) {
        failedTies << it.value().tieProp;
        g_log.warning() << "Invalid tie has been removed: " << tie << std::endl;
      }
    }
    // remove failed ties from the browser
    foreach (QtProperty *p, failedTies) {
      auto paramProp = getParentParameterProperty(p);
      if (isLocalParameterProperty(paramProp)) {
        auto const paramName = paramProp->propertyName();
        auto const index = getIndex(paramProp);
        setLocalParameterTie(index + paramName, m_currentDataset, "");
      }
      removeProperty(p);
    }
  }

  // add constraints
  {
    auto from = m_constraints.lowerBound(prop);
    auto to = m_constraints.upperBound(prop);
    for (auto it = from; it != to; ++it) {
      try {
        const auto &localParam = it.value();
        const auto lower = m_constraintManager->value(localParam.lower);
        const auto upper = m_constraintManager->value(localParam.upper);
        const auto constraint =
            getConstraint(localParam.paramProp->propertyName(), lower, upper);
        fun->addConstraints(constraint.toStdString());
      } catch (...) {
      }
    }
  }

  return fun;
}

/**
 * Return function at specified function index (e.g. f0.)
 * @param index :: Index of the function, or empty string for top-level function
 * @return Function at index, or null pointer if not found
 */
Mantid::API::IFunction_sptr
FunctionBrowser::getFunctionByIndex(const QString &index) {
  if (auto prop = getFunctionProperty(index)) {
    return getFunction(prop);
  } else {
    return Mantid::API::IFunction_sptr();
  }
}

/**
 * Updates the function parameter value
 * @param funcIndex :: Index of the function
 * @param paramName :: Parameter name
 * @param value :: New value
 */
void FunctionBrowser::setParameter(const QString &funcIndex,
                                   const QString &paramName, double value) {
  auto prop = getParameterProperty(funcIndex, paramName);
  m_parameterManager->setValue(prop, value);
}

/**
 * Updates the function parameter error
 * @param funcIndex :: Index of the function
 * @param paramName :: Parameter name
 * @param error :: New error
 */
void FunctionBrowser::setParamError(const QString &funcIndex,
                                    const QString &paramName, double error) {
  if (auto prop = getFunctionProperty(funcIndex)) {
    auto children = prop->subProperties();
    foreach (QtProperty *child, children) {
      if (isParameter(child) && child->propertyName() == paramName) {
        m_parameterManager->setError(child, error);
        break;
      }
    }
  }
}

/**
 * Get a value of a parameter
 * @param funcIndex :: Index of the function
 * @param paramName :: Parameter name
 */
double FunctionBrowser::getParameter(const QString &funcIndex,
                                     const QString &paramName) const {
  auto prop = getParameterProperty(funcIndex, paramName);
  return m_parameterManager->value(prop);
}

/**
 * Split a qualified parameter name into function index and local parameter
 * name.
 * @param paramName :: Fully qualified parameter name (includes function index)
 * @return :: A string list with the first item is the function index and the
 * second
 *   item is the param local name.
 */
QStringList
FunctionBrowser::splitParameterName(const QString &paramName) const {
  QString functionIndex;
  QString parameterName = paramName;
  int j = paramName.lastIndexOf('.');
  if (j > 0) {
    ++j;
    functionIndex = paramName.mid(0, j);
    parameterName = paramName.mid(j);
  }
  QStringList res;
  res << functionIndex << parameterName;
  return res;
}

/**
 * Updates the function parameter value
 * @param paramName :: Fully qualified parameter name (includes function index)
 * @param value :: New value
 */
void FunctionBrowser::setParameter(const QString &paramName, double value) {
  QStringList name = splitParameterName(paramName);
  setParameter(name[0], name[1], value);
}

/**
 * Updates the function parameter error
 * @param paramName :: Fully qualified parameter name (includes function index)
 * @param error :: New error
 */
void FunctionBrowser::setParamError(const QString &paramName, double error) {
  QStringList name = splitParameterName(paramName);
  setParamError(name[0], name[1], error);
}

/**
 * Get a value of a parameter
 * @param paramName :: Fully qualified parameter name (includes function index)
 */
double FunctionBrowser::getParameter(const QString &paramName) const {
  QStringList name = splitParameterName(paramName);
  return getParameter(name[0], name[1]);
}

/// Get a property for a parameter
QtProperty *
FunctionBrowser::getParameterProperty(const QString &paramName) const {
  QStringList name = splitParameterName(paramName);
  return getParameterProperty(name[0], name[1]);
}

/// Get a property for a parameter
QtProperty *
FunctionBrowser::getParameterProperty(const QString &funcIndex,
                                      const QString &paramName) const {
  if (auto prop = getFunctionProperty(funcIndex)) {
    auto children = prop->subProperties();
    foreach (QtProperty *child, children) {
      if (isParameter(child) && child->propertyName() == paramName) {
        return child;
      }
    }
  }
  std::string message = "Unknown function parameter " +
                        (funcIndex + paramName).toStdString() +
                        "\n\n This may happen if there is a CompositeFunction "
                        "containing only one function.";
  throw std::runtime_error(message);
}

/// Get a property for a parameter which is a parent of a given
/// property (tie or constraint).
QtProperty *
FunctionBrowser::getParentParameterProperty(QtProperty *prop) const {
  for (auto &tie : m_ties) {
    if (tie.tieProp == prop) {
      return tie.paramProp;
    }
  }

  for (auto &constraint : m_constraints) {
    if (constraint.lower == prop || constraint.upper == prop) {
      return constraint.paramProp;
    }
  }

  throw std::logic_error(
      "QtProperty " + prop->propertyName().toStdString() +
      " is not a child of a property for any function parameter.");
}

/**
 * Update parameter values in the browser to match those of a function.
 * @param fun :: A function to copy the values from. It must have the same
 *   type (composition) as the function in the browser.
 */
void FunctionBrowser::updateParameters(const Mantid::API::IFunction &fun) {
  const auto paramNames = fun.getParameterNames();
  for (const auto &parameter : paramNames) {
    const QString qName = QString::fromStdString(parameter);
    setParameter(qName, fun.getParameter(parameter));
    const size_t index = fun.parameterIndex(parameter);
    setParamError(qName, fun.getError(index));
  }
}

/**
 * Return FunctionFactory function string
 */
QString FunctionBrowser::getFunctionString() {
  auto fun = getFunction();
  if (!fun)
    return "";
  return QString::fromStdString(fun->asString());
}

/**
 * Remove the function under currently selected property
 */
void FunctionBrowser::removeFunction() {
  auto item = m_browser->currentItem();
  if (!item)
    return;
  QtProperty *prop = item->property();
  if (!isFunction(prop))
    return;

  removeProperty(prop);
  updateFunctionIndices();

  // After removing a function we could end up with
  // a CompositeFunction with only one function
  // In this case, the function should be kept but
  // the composite function should be removed
  auto props = m_browser->properties();
  if (!props.isEmpty()) {
    // The function browser is not empty

    // Check if the current function in the browser is a
    // composite function
    auto topProp = props[0];
    auto fun = Mantid::API::FunctionFactory::Instance().createFunction(
        topProp->propertyName().toStdString());
    auto cf = boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(fun);
    if (cf) {
      // If it is a composite function
      // check that there are more than one function
      // which means more than two subproperties
      size_t nFunctions = props[0]->subProperties().size() - 1;

      if (nFunctions == 1) {
        // If only one function remains, remove the composite function:
        // Temporary copy the remaining function
        auto func = getFunction(m_browser->properties()[0]->subProperties()[1]);
        // Remove the composite function
        m_browser->removeProperty(topProp);
        // Add the temporary stored function
        setFunction(func);
      }
    }
  }

  auto fun = getFunction();
  if (fun) {
    // Remove local parameters that were deleted with the function
    // or renamed due to change in the structure of the composite
    // function
    for (auto iter = m_localParameterValues.begin();
         iter != m_localParameterValues.end();) {
      auto param = iter.key();
      if (!fun->hasParameter(param.toStdString())) {
        iter = m_localParameterValues.erase(iter);
      } else {
        ++iter;
      }
    }
  } else {
    m_localParameterValues.clear();
  }

  emit functionStructureChanged();
}

/**
 * Fix currently selected parameter
 */
void FunctionBrowser::fixParameter() {
  auto item = m_browser->currentItem();
  if (!item)
    return;
  QtProperty *prop = item->property();
  if (!isParameter(prop))
    return;
  QString tie = QString::number(getParameter(prop));
  addTieProperty(prop, tie);
}

/// Get a tie property attached to a parameter property
QtProperty *FunctionBrowser::getTieProperty(QtProperty *prop) const {
  auto children = prop->subProperties();
  foreach (QtProperty *child, children) {
    if (child->propertyName() == "Tie") {
      return child;
    }
  }
  return nullptr;
}

/**
 * Unfix currently selected parameter
 */
void FunctionBrowser::removeTie() {
  auto item = m_browser->currentItem();
  if (!item)
    return;
  QtProperty *prop = item->property();
  if (!isParameter(prop))
    return;
  auto tieProp = getTieProperty(prop);
  if (tieProp) {
    removeProperty(tieProp);
  }
  if (isLocalParameterProperty(prop)) {
    auto parName = getParameterName(prop);
    auto &localValues = m_localParameterValues[parName];
    if (m_currentDataset < localValues.size()) {
      localValues[m_currentDataset].tie = "";
      localValues[m_currentDataset].fixed = false;
    }
  }
}

/**
 * Add a custom tie to currently selected parameter
 */
void FunctionBrowser::addTie() {
  auto item = m_browser->currentItem();
  if (!item)
    return;
  QtProperty *prop = item->property();
  if (!isParameter(prop))
    return;

  bool ok;
  QString tie = QInputDialog::getText(this, "Add a tie",
                                      "Tie:", QLineEdit::Normal, "", &ok);
  if (ok && !tie.isEmpty()) {
    try {
      addTieProperty(prop, tie);
    } catch (Mantid::API::Expression::ParsingError &) {
      QMessageBox::critical(this, "MantidPlot - Error",
                            "Syntax errors found in tie: " + tie);
    }
  }
}

/**
 * Copy function from the clipboard
 */
void FunctionBrowser::copyFromClipboard() {
  QString funStr = QApplication::clipboard()->text();
  if (funStr.isEmpty())
    return;
  try {
    auto fun = Mantid::API::FunctionFactory::Instance().createInitialized(
        funStr.toStdString());
    if (!fun)
      return;
    this->setFunction(fun);
  } catch (...) {
    // text in the clipboard isn't a function definition
    QMessageBox::warning(this, "MantidPlot - Warning",
                         "Text in the clipboard isn't a function definition"
                         " or contains errors.");
  }
}

/**
 * Copy function to the clipboard
 */
void FunctionBrowser::copyToClipboard() {
  auto fun = getFunction();
  if (fun) {
    QApplication::clipboard()->setText(QString::fromStdString(fun->asString()));
  }
}

/**
 * Add both constraints to current parameter
 */
void FunctionBrowser::addConstraints() {
  auto item = m_browser->currentItem();
  if (!item)
    return;
  QtProperty *prop = item->property();
  if (!isParameter(prop))
    return;
  addConstraintProperties(prop, "0<" + prop->propertyName() + "<0");
}

/**
 * Add both constraints to current parameter
 */
void FunctionBrowser::addConstraints10() {
  auto item = m_browser->currentItem();
  if (!item)
    return;
  QtProperty *prop = item->property();
  if (!isParameter(prop))
    return;
  double val = getParameter(prop);
  addConstraintProperties(prop, QString::number(val * 0.9) + "<" +
                                    prop->propertyName() + "<" +
                                    QString::number(val * 1.1));
}

/**
 * Add both constraints to current parameter
 */
void FunctionBrowser::addConstraints50() {
  auto item = m_browser->currentItem();
  if (!item)
    return;
  QtProperty *prop = item->property();
  if (!isParameter(prop))
    return;
  double val = getParameter(prop);
  addConstraintProperties(prop, QString::number(val * 0.5) + "<" +
                                    prop->propertyName() + "<" +
                                    QString::number(val * 1.5));
}

/**
 * Remove both constraints from current parameter
 */
void FunctionBrowser::removeConstraints() {
  auto item = m_browser->currentItem();
  if (!item)
    return;
  QtProperty *prop = item->property();
  if (!isParameter(prop))
    return;
  const bool isLocal = isLocalParameterProperty(prop);
  auto props = prop->subProperties();
  foreach (QtProperty *p, props) {
    if (isConstraint(p)) {
      removeProperty(p);
      if (isLocal) {
        auto parName = getParameterName(prop);
        checkLocalParameter(parName);
        auto &localValue = m_localParameterValues[parName][m_currentDataset];
        localValue.lowerBound = "";
        localValue.upperBound = "";
      }
    }
  }
}

/**
 * Remove one constraint from current parameter
 */
void FunctionBrowser::removeConstraint() {
  auto item = m_browser->currentItem();
  if (!item)
    return;
  QtProperty *prop = item->property();
  if (!isConstraint(prop))
    return;
  auto paramProp = getParentParameterProperty(prop);
  if (isLocalParameterProperty(paramProp)) {
    auto parName = getParameterName(paramProp);
    checkLocalParameter(parName);
    auto &localValue = m_localParameterValues[parName][m_currentDataset];
    localValue.lowerBound = "";
    localValue.upperBound = "";
  }
  removeProperty(prop);
}

void FunctionBrowser::updateCurrentFunctionIndex() {
  boost::optional<QString> newIndex;

  if (auto item = m_browser->currentItem()) {
    auto prop = item->property();
    newIndex = getIndex(prop);
  }

  if (m_currentFunctionIndex != newIndex) {
    m_currentFunctionIndex = newIndex;
    emit currentFunctionChanged();
  }
}

/**
 * Slot connected to all function attribute managers. Update the corresponding
 * function.
 * @param prop :: An attribute property that was changed
 */
void FunctionBrowser::attributeChanged(QtProperty *prop) {
  auto funProp = m_properties[prop].parent;
  if (!funProp)
    return;
  // get function with the changed attribute (it is set from prop's value)
  auto fun = getFunction(funProp, true);

  // delete and recreate all function's properties (attributes, parameters, etc)
  setFunction(funProp, fun);
  updateFunctionIndices();
  emit functionStructureChanged();
}

/** Called when the size of a vector attribute is changed
 * @param prop :: A property that was changed.
 */
void FunctionBrowser::attributeVectorSizeChanged(QtProperty *prop) {
  QtProperty *vectorProp = m_properties[prop].parent;
  if (!vectorProp)
    throw std::logic_error("FunctionBrowser: inconsistency in vector "
                           "properties.\nAttribute property not found.");
  auto funProp = m_properties[vectorProp].parent;
  if (!funProp)
    throw std::logic_error("FunctionBrowser: inconsistency in vector "
                           "properties.\nFunction property not found.");
  auto fun = getFunction(funProp, true);
  if (!fun)
    throw std::logic_error("FunctionBrowser: inconsistency in vector "
                           "properties.\nFunction undefined.");
  auto attName = vectorProp->propertyName().toStdString();
  auto attribute = fun->getAttribute(attName).asVector();
  auto newSize = m_attributeSizeManager->value(prop);
  if (newSize < 0)
    newSize = 0;
  if (attribute.size() != static_cast<size_t>(newSize)) {
    if (newSize == 0) {
      attribute.clear();
    } else {
      attribute.resize(newSize);
    }
    fun->setAttributeValue(attName, attribute);
    setFunction(funProp, fun);
    updateFunctionIndices();
    emit functionStructureChanged();
  }
}

/**
 * Slot connected to a property displaying the value of a member of a vector
 * attribute.
 * @param prop :: A property that was changed.
 */
void FunctionBrowser::attributeVectorDoubleChanged(QtProperty *prop) {
  QtProperty *vectorProp = m_properties[prop].parent;
  if (!vectorProp)
    throw std::runtime_error(
        "FunctionBrowser: inconsistency in vector properties.");
  attributeChanged(vectorProp);
}

void FunctionBrowser::parameterChanged(QtProperty *prop) {
  bool isGlobal = true;
  QString newTie;
  if (isLocalParameterProperty(prop)) {
    setLocalParameterValue(getParameterName(prop), m_currentDataset,
                           m_parameterManager->value(prop));
    isGlobal = false;
    newTie = getLocalParameterTie(getParameterName(prop), m_currentDataset);
  }

  auto tieProp = getTieProperty(prop);
  if (tieProp && !tieProp->isEnabled()) {
    if (isGlobal) {
      // it is a fixed tie
      newTie = QString("%1=%2")
                   .arg(prop->propertyName())
                   .arg(m_parameterManager->value(prop));
    }
    if (!newTie.isEmpty()) {
      m_tieManager->setValue(tieProp, newTie);
    }
  }
  emit parameterChanged(getIndex(prop), prop->propertyName());
}

/// Called when a tie property changes
void FunctionBrowser::tieChanged(QtProperty *prop) {
  for (const auto &atie : m_ties) {
    if (atie.tieProp == prop) {
      const auto parProp = atie.paramProp;
      if (isLocalParameterProperty(parProp)) {
        const auto parName = getParameterName(parProp);
        checkLocalParameter(parName);
        auto &paramValue = m_localParameterValues[parName][m_currentDataset];
        if (paramValue.fixed)
          break;
        auto tie = QString::fromStdString(getTie(parProp));
        auto tieExpr = tie.split('=');
        if (tieExpr.size() == 2) {
          tie = tieExpr[1];
        }
        paramValue.tie = tie;
        break;
      }
    }
  }
}

/// Called when a constraint property changes
void FunctionBrowser::constraintChanged(QtProperty *prop) {
  for (const auto &constraint : m_constraints) {
    const bool isLower = constraint.lower == prop;
    const bool isUpper = constraint.upper == prop;
    if (isLower || isUpper) {
      auto paramProp = constraint.paramProp;
      if (isLocalParameterProperty(paramProp)) {
        const auto parName = getParameterName(paramProp);
        checkLocalParameter(parName);
        auto &paramValue = m_localParameterValues[parName][m_currentDataset];
        if (isLower) {
          paramValue.lowerBound = m_constraintManager->value(prop);
        } else {
          paramValue.upperBound = m_constraintManager->value(prop);
        }
        break;
      }
    }
  }
}

void FunctionBrowser::parameterButtonClicked(QtProperty *prop) {
  emit localParameterButtonClicked(getIndex(prop) + prop->propertyName());
}

bool FunctionBrowser::hasFunction() const {
  return !m_functionManager->properties().isEmpty();
}

/// Get the number of datasets
int FunctionBrowser::getNumberOfDatasets() const { return m_numberOfDatasets; }

/// Set new number of the datasets
/// @param n :: New value for the number of datasets.
void FunctionBrowser::setNumberOfDatasets(int n) {
  if (!m_multiDataset) {
    throw std::runtime_error(
        "Function browser wasn't set up for multi-dataset fitting.");
  }
  m_numberOfDatasets = n;
}

/**
 * Get value of a local parameter
 * @param parName :: Name of a parameter.
 * @param i :: Data set index.
 */
double FunctionBrowser::getLocalParameterValue(const QString &parName,
                                               int i) const {
  checkLocalParameter(parName);
  return m_localParameterValues[parName][i].value;
}

void FunctionBrowser::setLocalParameterValue(const QString &parName, int i,
                                             double value) {
  checkLocalParameter(parName);
  m_localParameterValues[parName][i].value = value;
  if (i == m_currentDataset) {
    setParameter(parName, value);
  }
}

void FunctionBrowser::setLocalParameterValue(const QString &parName, int i,
                                             double value, double error) {
  checkLocalParameter(parName);
  m_localParameterValues[parName][i].value = value;
  m_localParameterValues[parName][i].error = error;
  if (i == m_currentDataset) {
    setParameter(parName, value);
    setParamError(parName, error);
  }
}
/// Get error of a local parameter
double FunctionBrowser::getLocalParameterError(const QString &parName,
                                               int i) const {
  checkLocalParameter(parName);
  return m_localParameterValues[parName][i].error;
}

/**
 * Init a local parameter. Define initial values for all datasets.
 * @param parName :: Name of parametere to init.
 */
void FunctionBrowser::initLocalParameter(const QString &parName) const {
  auto nData = getNumberOfDatasets();
  if (nData == 0) {
    nData = 1;
  }
  auto oldValues = m_localParameterValues.find(parName);
  if (oldValues != m_localParameterValues.end() && !oldValues->isEmpty()) {
    auto nOldData = oldValues->size();
    if (nOldData > nData) {
      oldValues->erase(oldValues->begin() + nData, oldValues->end());
    } else if (nOldData < nData) {
      oldValues->insert(oldValues->end(), nData - nOldData, oldValues->back());
    }
  } else {
    double value = getParameter(parName);
    QVector<LocalParameterData> values(nData, LocalParameterData(value));
    m_localParameterValues[parName] = values;
  }
}

/// Make sure that the parameter is initialized
/// @param parName :: Name of a parameter to check
void FunctionBrowser::checkLocalParameter(const QString &parName) const {
  if (!m_localParameterValues.contains(parName) ||
      m_localParameterValues[parName].size() != getNumberOfDatasets()) {
    initLocalParameter(parName);
  }
}

/// Check that a property contains a local parameter
bool FunctionBrowser::isLocalParameterProperty(QtProperty *prop) const {
  return m_currentDataset < m_numberOfDatasets &&
         prop->hasOption(globalOptionName) &&
         !prop->checkOption(globalOptionName);
}

/// Check that a property contains a global parameter
bool FunctionBrowser::isGlobalParameterProperty(QtProperty *prop) const {
  return prop->hasOption(globalOptionName) &&
         prop->checkOption(globalOptionName);
}

void FunctionBrowser::resetLocalParameters() { m_localParameterValues.clear(); }

/// Set current dataset.
void FunctionBrowser::setCurrentDataset(int i) {
  m_currentDataset = i;
  if (m_currentDataset >= m_numberOfDatasets) {
    throw std::runtime_error("Dataset index is outside the range");
  }
  auto localParameters = getLocalParameters();
  foreach (QString par, localParameters) {
    setParameter(par, getLocalParameterValue(par, m_currentDataset));
    setParamError(par, getLocalParameterError(par, m_currentDataset));
    updateLocalTie(par);
    updateLocalConstraint(par);
  }
}

/// Remove local parameter values for a number of datasets.
/// @param indices :: A list of indices of datasets to remove.
void FunctionBrowser::removeDatasets(QList<int> indices) {
  if (indices.size() > m_numberOfDatasets) {
    throw std::runtime_error(
        "FunctionBrowser asked to removed too many datasets");
  }
  qSort(indices);
  for (auto par = m_localParameterValues.begin();
       par != m_localParameterValues.end(); ++par) {
    for (int i = indices.size() - 1; i >= 0; --i) {
      int index = indices[i];
      if (index < 0) {
        throw std::runtime_error(
            "Index of a dataset in FunctionBrowser cannot be negative.");
      }
      if (index < m_numberOfDatasets) {
        auto &v = par.value();
        // value may not have been populated
        if (index < v.size()) {
          v.remove(index);
        }
      } else {
        throw std::runtime_error(
            "Index of a dataset in FunctionBrowser is out of range.");
      }
    }
  }
  setNumberOfDatasets(m_numberOfDatasets - indices.size());
}

/// Add local parameters for additional datasets.
/// @param n :: Number of datasets added.
void FunctionBrowser::addDatasets(int n) {
  setNumberOfDatasets(m_numberOfDatasets + n);
}

/// Return the multidomain function for multi-dataset fitting
Mantid::API::IFunction_sptr FunctionBrowser::getGlobalFunction() {
  if (!m_multiDataset) {
    throw std::runtime_error(
        "Function browser wasn't set up for multi-dataset fitting.");
  }
  // number of spectra to fit == size of the multi-domain function
  int nOfDataSets = getNumberOfDatasets();
  if (nOfDataSets == 0) {
    throw std::runtime_error("There are no data sets specified.");
  }

  // description of a single function
  QString funStr = getFunctionString();

  if (nOfDataSets == 1) {
    return Mantid::API::FunctionFactory::Instance().createInitialized(
        funStr.toStdString());
  }

  bool isComposite =
      (std::find(funStr.begin(), funStr.end(), ';') != funStr.end());
  if (isComposite) {
    funStr = ";(" + funStr + ")";
  } else {
    funStr = ";" + funStr;
  }

  QString multiFunStr = "composite=MultiDomainFunction,NumDeriv=1";
  for (int i = 0; i < nOfDataSets; ++i) {
    multiFunStr += funStr;
  }

  // add the global ties
  QStringList globals = getGlobalParameters();
  QString globalTies;
  if (!globals.isEmpty()) {
    globalTies = "ties=(";
    bool isFirst = true;
    foreach (QString par, globals) {
      if (!isFirst)
        globalTies += ",";
      else
        isFirst = false;

      for (int i = 1; i < nOfDataSets; ++i) {
        globalTies += QString("f%1.").arg(i) + par + "=";
      }
      globalTies += QString("f0.%1").arg(par);
    }
    globalTies += ")";
    multiFunStr += ";" + globalTies;
  }

  // create the multi-domain function
  auto fun = Mantid::API::FunctionFactory::Instance().createInitialized(
      multiFunStr.toStdString());
  boost::shared_ptr<Mantid::API::MultiDomainFunction> multiFun =
      boost::dynamic_pointer_cast<Mantid::API::MultiDomainFunction>(fun);
  if (!multiFun) {
    throw std::runtime_error("Failed to create the MultiDomainFunction");
  }

  auto globalParams = getGlobalParameters();

  // set the domain indices, initial local parameter values and ties
  for (int i = 0; i < nOfDataSets; ++i) {
    multiFun->setDomainIndex(i, i);
    auto fun1 = multiFun->getFunction(i);
    for (size_t j = 0; j < fun1->nParams(); ++j) {
      const auto parameterName = fun1->parameterName(j);
      const auto parName = QString::fromStdString(parameterName);
      if (globalParams.contains(parName))
        continue;
      auto tie = fun1->getTie(j);
      if (tie) {
        // If parameter has a tie at this stage then it gets it from the
        // currently displayed function. But the i-th local parameters may not
        // have this tie, so remove it
        fun1->removeTie(j);
      }
      if (fun1->isFixed(j)) {
        fun1->unfix(j);
      }
      if (fun1->getConstraint(j)) {
        fun1->removeConstraint(parameterName);
      }
      checkLocalParameter(parName);
      const auto &localParam = m_localParameterValues[parName][i];
      if (localParam.fixed) {
        // Fix this particular local parameter
        fun1->setParameter(j, localParam.value);
        fun1->fix(j);
      } else {
        const auto &tie = localParam.tie;
        if (!tie.isEmpty()) {
          fun1->tie(parameterName, tie.toStdString());
        } else {
          fun1->setParameter(j, localParam.value);
        }
      }
      if (!localParam.lowerBound.isEmpty() &&
          !localParam.upperBound.isEmpty()) {
        auto constraint = getConstraint(parName, localParam.lowerBound,
                                        localParam.upperBound);
        fun1->addConstraints(constraint.toStdString());
      }
    }
  }
  assert(multiFun->nFunctions() == static_cast<size_t>(nOfDataSets));

  return fun;
}

/// Make sure that properties are in sync with the cached ties
/// @param parName :: A parameter to check.
void FunctionBrowser::updateLocalTie(const QString &parName) {
  auto prop = getParameterProperty(parName);
  if (hasTie(prop)) {
    auto tieProp = getTieProperty(prop);
    removeProperty(tieProp);
  }
  auto &localParam = m_localParameterValues[parName][m_currentDataset];
  if (localParam.fixed) {
    addTieProperty(
        prop, QString::number(
                  m_localParameterValues[parName][m_currentDataset].value));
  } else if (!localParam.tie.isEmpty()) {
    addTieProperty(prop, localParam.tie);
  }
}

/// Make sure that properties are in sync with the cached constraints
/// @param parName :: A parameter to check.
void FunctionBrowser::updateLocalConstraint(const QString &parName) {
  auto prop = getParameterProperty(parName);
  if (hasConstraint(prop)) {
    auto props = prop->subProperties();
    foreach (QtProperty *p, props) {
      if (isConstraint(p)) {
        removeProperty(p);
      }
    }
  }
  auto &localParam = m_localParameterValues[parName][m_currentDataset];
  if (!localParam.lowerBound.isEmpty() && !localParam.upperBound.isEmpty()) {
    auto constraint =
        getConstraint(parName, localParam.lowerBound, localParam.upperBound);
    addConstraintProperties(prop, constraint);
  }
}

/// Fix/unfix a local parameter
/// @param parName :: Parameter name
/// @param i :: Index of a dataset.
/// @param fixed :: Make it fixed (true) or free (false)
void FunctionBrowser::setLocalParameterFixed(const QString &parName, int i,
                                             bool fixed) {
  checkLocalParameter(parName);
  m_localParameterValues[parName][i].fixed = fixed;
  if (i == m_currentDataset) {
    updateLocalTie(parName);
  }
}

/// Check if a local parameter is fixed
/// @param parName :: Parameter name
/// @param i :: Index of a dataset.
bool FunctionBrowser::isLocalParameterFixed(const QString &parName,
                                            int i) const {
  checkLocalParameter(parName);
  return m_localParameterValues[parName][i].fixed;
}

/// Get the tie for a local parameter.
/// @param parName :: Parameter name
/// @param i :: Index of a dataset.
QString FunctionBrowser::getLocalParameterTie(const QString &parName,
                                              int i) const {
  checkLocalParameter(parName);
  return m_localParameterValues[parName][i].tie;
}

/// Set a tie for a local parameter.
/// @param parName :: Parameter name
/// @param i :: Index of a dataset.
/// @param tie :: A tie string.
void FunctionBrowser::setLocalParameterTie(const QString &parName, int i,
                                           QString tie) {
  checkLocalParameter(parName);
  m_localParameterValues[parName][i].tie = tie;
  if (i == m_currentDataset) {
    updateLocalTie(parName);
  }
}

/// Update the interface to have the same parameter values as in a function.
/// @param fun :: A function to get parameter values from.
void FunctionBrowser::updateMultiDatasetParameters(
    const Mantid::API::IFunction &fun) {
  auto cfun = dynamic_cast<const Mantid::API::CompositeFunction *>(&fun);
  if (cfun && cfun->nFunctions() > 0) {

    // Multiple datasets
    if (const auto *multiFun =
            dynamic_cast<const Mantid::API::MultiDomainFunction *>(cfun)) {
      // Check the function has the correct number of domains
      if (multiFun->getNumberDomains() !=
          static_cast<size_t>(m_numberOfDatasets)) {
        throw std::invalid_argument("Function has incorrect number of domains");
      }
      // update function
      {
        auto sfun = multiFun->getFunction(0);
        const auto globalParameters = getGlobalParameters();
        for (int j = 0; j < globalParameters.size(); ++j) {
          auto const &paramName = globalParameters[j];
          auto paramIndex = sfun->parameterIndex(paramName.toStdString());
          setParameter(paramName, sfun->getParameter(paramIndex));
          setParamError(paramName, sfun->getError(paramIndex));
        }
      }
      size_t currentIndex = static_cast<size_t>(m_currentDataset);
      const auto localParameters = getLocalParameters();
      for (size_t i = 0; i < multiFun->nFunctions(); ++i) {
        auto sfun = multiFun->getFunction(i);
        for (int j = 0; j < localParameters.size(); ++j) {
          auto const &paramName = localParameters[j];
          auto paramIndex = sfun->parameterIndex(paramName.toStdString());
          auto value = sfun->getParameter(paramIndex);
          auto error = sfun->getError(paramIndex);
          setLocalParameterValue(localParameters[j], static_cast<int>(i), value,
                                 error);
          if (i == currentIndex) {
            setParameter(paramName, value);
            setParamError(paramName, error);
          }
        }
      }
    } else { // composite function, 1 domain only
      if (m_numberOfDatasets != 1) {
        throw std::invalid_argument(
            "Multiple datasets, but function is single-domain");
      }
      updateParameters(*cfun);
    }
  } else {
    updateParameters(fun);
  }
}

/// Resize the browser's columns
/// @param s0 :: New size for the first column (Parameter).
/// @param s1 :: New size for the second column (Value).
/// @param s2 :: New size for the third optional column (Global).
void FunctionBrowser::setColumnSizes(int s0, int s1, int s2) {
  m_browser->setColumnSizes(s0, s1, s2);
}

/**
 * Emit a signal when any of the Global options change.
 */
void FunctionBrowser::globalChanged(QtProperty * /*unused*/, const QString & /*unused*/, bool /*unused*/) {
  emit globalsChanged();
}

/**
 * Set display of parameter errors on/off
 * @param enabled :: [input] On/off display of errors
 */
void FunctionBrowser::setErrorsEnabled(bool enabled) {
  m_parameterManager->setErrorsEnabled(enabled);
}

/**
 * Clear all errors, if they are set
 */
void FunctionBrowser::clearErrors() { m_parameterManager->clearErrors(); }

} // namespace MantidWidgets
} // namespace MantidQt
