// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FunctionTreeView.h"
#include "MantidQtWidgets/Common/FunctionBrowser/FunctionBrowserUtils.h"

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
#include "MantidQtWidgets/Common/EditLocalParameterDialog.h"
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
FunctionTreeView::FunctionTreeView(QWidget *parent, bool multi, const std::vector<std::string>& categories)
    : IFunctionView(parent), m_multiDataset(multi), m_allowedCategories(categories), m_selectFunctionDialog(nullptr)

{
  // create m_browser
  createBrowser();
  createActions();

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(m_browser);
}

/**
 * Destructor
 */
FunctionTreeView::~FunctionTreeView() {}

/**
 * Create the Qt property browser and set up property managers.
 */
void FunctionTreeView::createBrowser() {
  QStringList options;
  if (m_multiDataset) {
    options << globalOptionName;
  }
  m_browser = new QtTreePropertyBrowser(this, options);

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
          SLOT(parameterPropertyChanged(QtProperty *)));

  connect(m_browser, SIGNAL(currentItemChanged(QtBrowserItem *)),
          SLOT(updateCurrentFunctionIndex()));

  m_browser->setFocusPolicy(Qt::StrongFocus);
}

/**
 * Create and connect actions
 */
void FunctionTreeView::createActions() {
  m_actionAddFunction = new QAction("Add function", this);
  m_actionAddFunction->setObjectName("add_function");
  connect(m_actionAddFunction, SIGNAL(triggered()), this, SLOT(addFunctionBegin()));

  m_actionRemoveFunction = new QAction("Remove function", this);
  m_actionRemoveFunction->setObjectName("remove_function");
  connect(m_actionRemoveFunction, SIGNAL(triggered()), this,
          SLOT(removeFunction()));

  m_actionFixParameter = new QAction("Fix", this);
  connect(m_actionFixParameter, SIGNAL(triggered()), this,
          SLOT(fixParameter()));

  m_actionRemoveTie = new QAction("Remove tie", this);
  connect(m_actionRemoveTie, SIGNAL(triggered()), this, SLOT(removeTie()));

  m_actionAddTie = new QAction("Add tie", this);
  connect(m_actionAddTie, SIGNAL(triggered()), this, SLOT(addTie()));

  m_actionFromClipboard = new QAction("Paste from clipboard", this);
  m_actionFromClipboard->setObjectName("paste_from_clipboard");
  connect(m_actionFromClipboard, SIGNAL(triggered()), this,
          SLOT(pasteFromClipboard()));

  m_actionToClipboard = new QAction("Copy to clipboard", this);
  m_actionToClipboard->setObjectName("copy_to_clipboard");
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
void FunctionTreeView::clear() {
  m_browser->clear();
  m_properties.clear();
}

/**
 * Set the function in the browser
 * @param fun :: A function
 */
void FunctionTreeView::setFunction(Mantid::API::IFunction_sptr fun) {
  clear();
  addFunction(nullptr, fun);
}

/**
 * Add a sub-property to a parent property
 * @param parent :: The parent property
 * @param subproperty :: New sub-property
 */
FunctionTreeView::AProperty
FunctionTreeView::addProperty(QtProperty *parent, QtProperty *subproperty) {
  AProperty ap;
  ap.prop = subproperty;
  if (parent == nullptr) {
    ap.item = m_browser->addProperty(subproperty);
  } else {
    parent->addSubProperty(subproperty);
    auto items = m_browser->items(subproperty);
    if (items.isEmpty()) {
      throw std::runtime_error("Unexpected error in FunctionTreeView [1]");
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
void FunctionTreeView::removeProperty(QtProperty *prop) {
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
FunctionTreeView::AProperty
FunctionTreeView::addFunctionProperty(QtProperty *parent, QString funName) {
  // check that parent is a function property
  if (parent && dynamic_cast<QtAbstractPropertyManager *>(m_functionManager) !=
                    parent->propertyManager()) {
    throw std::runtime_error("Unexpected error in FunctionTreeView [2]");
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
FunctionTreeView::AProperty
FunctionTreeView::addParameterProperty(QtProperty *parent, QString paramName,
                                      QString paramDesc, double paramValue) {
  // check that parent is a function property
  if (!parent || dynamic_cast<QtAbstractPropertyManager *>(m_functionManager) !=
                     parent->propertyManager()) {
    throw std::runtime_error("Unexpected error in FunctionTreeView [3]");
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
void FunctionTreeView::setFunction(QtProperty *prop,
                                  Mantid::API::IFunction_sptr fun) {
  auto children = prop->subProperties();
  foreach (QtProperty *child, children) { removeProperty(child); }
  //m_localParameterValues.clear();
  addAttributeAndParameterProperties(prop, fun);
}

/**
 * Add a function.
 * @param prop :: Property of the parent composite function or NULL
 * @param fun :: FunctionFactory function creation string
 */
void FunctionTreeView::addFunction(QtProperty *prop,
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
          "FunctionTreeView: CompositeFunction is expected for addFunction");
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
class CreateAttributePropertyForFunctionTreeView
    : public Mantid::API::IFunction::ConstAttributeVisitor<
          FunctionTreeView::AProperty> {
public:
  CreateAttributePropertyForFunctionTreeView(FunctionTreeView *browser,
                                            QtProperty *parent, QString attName)
      : m_browser(browser), m_parent(parent), m_attName(attName) {
    // check that parent is a function property
    if (!m_parent ||
        dynamic_cast<QtAbstractPropertyManager *>(
            m_browser->m_functionManager) != m_parent->propertyManager()) {
      throw std::runtime_error("Unexpected error in FunctionTreeView [4]");
    }
  }

protected:
  /// Create string property
  FunctionTreeView::AProperty apply(const std::string &str) const override {
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
  FunctionTreeView::AProperty apply(const double &d) const override {
    QtProperty *prop =
        m_browser->m_attributeDoubleManager->addProperty(m_attName);
    m_browser->m_attributeDoubleManager->setValue(prop, d);
    return m_browser->addProperty(m_parent, prop);
  }
  /// Create int property
  FunctionTreeView::AProperty apply(const int &i) const override {
    QtProperty *prop = m_browser->m_attributeIntManager->addProperty(m_attName);
    m_browser->m_attributeIntManager->setValue(prop, i);
    return m_browser->addProperty(m_parent, prop);
  }
  /// Create bool property
  FunctionTreeView::AProperty apply(const bool &b) const override {
    QtProperty *prop =
        m_browser->m_attributeBoolManager->addProperty(m_attName);
    m_browser->m_attributeBoolManager->setValue(prop, b);
    return m_browser->addProperty(m_parent, prop);
  }
  /// Create vector property
  FunctionTreeView::AProperty
  apply(const std::vector<double> &v) const override {
    QtProperty *prop =
        m_browser->m_attributeVectorManager->addProperty(m_attName);
    FunctionTreeView::AProperty aprop = m_browser->addProperty(m_parent, prop);

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
  FunctionTreeView *m_browser;
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
  SetAttributeFromProperty(FunctionTreeView *browser, QtProperty *prop)
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
          "FunctionTreeView: empty vector attribute group.");
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
  FunctionTreeView *m_browser;
  QtProperty *m_prop;
};

/**
 * Add a attribute property
 * @param parent :: Parent function property
 * @param attName :: Attribute name
 * @param att :: Attribute value
 */
FunctionTreeView::AProperty FunctionTreeView::addAttributeProperty(
    QtProperty *parent, QString attName,
    const Mantid::API::IFunction::Attribute &att) {
  CreateAttributePropertyForFunctionTreeView cap(this, parent, attName);
  return att.apply(cap);
}

/**
 * Add attribute and parameter properties to a function property. For a
 * composite function
 *  adds all member functions' properties
 * @param prop :: A function property
 * @param fun :: Shared pointer to a created function
 */
void FunctionTreeView::addAttributeAndParameterProperties(
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
FunctionTreeView::AProperty FunctionTreeView::addIndexProperty(QtProperty *prop) {
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
void FunctionTreeView::updateFunctionIndices(QtProperty *prop, QString index) {
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
FunctionTreeView::AProperty FunctionTreeView::getFunctionProperty() const {
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
 * Check if property is a function group
 * @param prop :: Property to check
 */
bool FunctionTreeView::isFunction(QtProperty *prop) const {
  return prop && dynamic_cast<QtAbstractPropertyManager *>(m_functionManager) ==
                     prop->propertyManager();
}

/**
 * Check if property is any of the string attributes
 * @param prop :: Property to check
 */
bool FunctionTreeView::isStringAttribute(QtProperty *prop) const {
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
bool FunctionTreeView::isDoubleAttribute(QtProperty *prop) const {
  return prop && dynamic_cast<QtAbstractPropertyManager *>(
                     m_attributeDoubleManager) == prop->propertyManager();
}

/**
 * Check if property is a function attribute
 * @param prop :: Property to check
 */
bool FunctionTreeView::isIntAttribute(QtProperty *prop) const {
  return prop && dynamic_cast<QtAbstractPropertyManager *>(
                     m_attributeIntManager) == prop->propertyManager();
}

/**
 * Check if property is a function bool attribute
 * @param prop :: Property to check
 */
bool FunctionTreeView::isBoolAttribute(QtProperty *prop) const {
  return prop && dynamic_cast<QtAbstractPropertyManager *>(
                     m_attributeBoolManager) == prop->propertyManager();
}

/**
 * Check if property is a function vector attribute
 * @param prop :: Property to check
 */
bool FunctionTreeView::isVectorAttribute(QtProperty *prop) const {
  return prop && dynamic_cast<QtAbstractPropertyManager *>(
                     m_attributeVectorManager) == prop->propertyManager();
}

/**
 * Check if property is a function attribute
 * @param prop :: Property to check
 */
bool FunctionTreeView::isAttribute(QtProperty *prop) const {
  return isStringAttribute(prop) || isDoubleAttribute(prop) ||
         isIntAttribute(prop) || isBoolAttribute(prop) ||
         isVectorAttribute(prop);
}

/**
 * Check if property is a function parameter
 * @param prop :: Property to check
 */
bool FunctionTreeView::isParameter(QtProperty *prop) const {
  return prop && dynamic_cast<QtAbstractPropertyManager *>(
                     m_parameterManager) == prop->propertyManager();
}

/**
 * Get parameter value as a string
 * @param prop :: A parameter property
 */
double FunctionTreeView::getParameter(QtProperty *prop) const {
  return m_parameterManager->value(prop);
}

/**
 * Check if a property is an index
 * @param prop :: A property
 */
bool FunctionTreeView::isIndex(QtProperty *prop) const {
  return prop && dynamic_cast<QtAbstractPropertyManager *>(m_indexManager) ==
                     prop->propertyManager();
}

/**
 * Get the function index for a property
 * @param prop :: A property
 */
QString FunctionTreeView::getIndex(QtProperty *prop) const {
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
QString FunctionTreeView::getParameterName(QtProperty *prop) {
  return getIndex(prop) + prop->propertyName();
}

/**
 * Return function property for a function with given index.
 * @param index :: Function index to search, or empty string for top-level
 * function
 * @return Function property, or NULL if not found
 */
QtProperty *FunctionTreeView::getFunctionProperty(const QString &index) const {
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
void FunctionTreeView::addTieProperty(QtProperty *prop, QString tie) {
  if (!prop) {
    throw std::runtime_error("FunctionTreeView: null property pointer");
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
}

/**
 * Check if a parameter property has a tie
 * @param prop :: A parameter property
 */
bool FunctionTreeView::hasTie(QtProperty *prop) const {
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
bool FunctionTreeView::isTie(QtProperty *prop) const {
  return prop && dynamic_cast<QtAbstractPropertyManager *>(m_tieManager) ==
                     prop->propertyManager();
}

/**
 * Get a tie for a parameter
 * @param prop :: A parameter property
 */
std::string FunctionTreeView::getTie(QtProperty *prop) const {
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
QList<FunctionTreeView::AProperty>
FunctionTreeView::addConstraintProperties(QtProperty *prop, QString constraint) {
  if (!isParameter(prop))
    return QList<FunctionTreeView::AProperty>();
  QString lowerBoundStr = "";
  QString upperBoundStr = "";
  Mantid::API::Expression expr;
  expr.parse(constraint.toStdString());
  if (expr.name() != "==")
    return QList<FunctionTreeView::AProperty>();
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
      return QList<FunctionTreeView::AProperty>();
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
  QList<FunctionTreeView::AProperty> plist;
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
  return plist;
}

/**
 * Check if a property is a constraint
 * @param prop :: Property to check.
 */
bool FunctionTreeView::isConstraint(QtProperty *prop) const {
  return prop && dynamic_cast<QtAbstractPropertyManager *>(
                     m_constraintManager) == prop->propertyManager();
}

/**
 * Check if a parameter property has a constraint
 * @param prop :: A parameter property.
 */
bool FunctionTreeView::hasConstraint(QtProperty *prop) const {
  return hasLowerBound(prop) || hasUpperBound(prop);
}

/**
 * Check if a parameter property has a lower bound
 * @param prop :: A parameter property.
 */
bool FunctionTreeView::hasLowerBound(QtProperty *prop) const {
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
bool FunctionTreeView::hasUpperBound(QtProperty *prop) const {
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
QString FunctionTreeView::getConstraint(const QString &paramName,
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
void FunctionTreeView::popupMenu(const QPoint &) {
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
void FunctionTreeView::addFunctionBegin() {

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

  // Start the dialog
  if (!m_selectFunctionDialog) {
    m_selectFunctionDialog = new SelectFunctionDialog(this, m_allowedCategories);
    connect(m_selectFunctionDialog, SIGNAL(finished(int)), this, SLOT(addFunctionEnd(int)));
  }
  m_selectedFunctionProperty = prop;
  m_selectFunctionDialog->open();
}

void FunctionTreeView::addFunctionEnd(int result) {
  if (result != QDialog::Accepted) {
    return;
  }

  QString newFunction = m_selectFunctionDialog->getFunction();
  if (newFunction.isEmpty())
    return;

  // create new function
  auto f = Mantid::API::FunctionFactory::Instance().createFunction(
      newFunction.toStdString());

  auto prop = m_selectedFunctionProperty;
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
  emit functionAdded(QString::fromStdString(f->asString()));
}

/**
 * Set value of an attribute (as a property) to a function.
 * @param fun :: Function to which attribute is set.
 * @param prop :: A property with the name and value of the attribute.
 */
void FunctionTreeView::setAttributeToFunction(Mantid::API::IFunction &fun,
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
Mantid::API::IFunction_sptr FunctionTreeView::getFunction(QtProperty *prop,
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
 * Updates the function parameter value
 * @param funcIndex :: Index of the function
 * @param paramName :: Parameter name
 * @param value :: New value
 */
void FunctionTreeView::setParameter(const QString &paramName, double value) {
  auto prop = getParameterProperty(paramName);
  m_parameterManager->setValue(prop, value);
}

/**
 * Updates the function parameter error
 * @param funcIndex :: Index of the function
 * @param paramName :: Parameter name
 * @param error :: New error
 */
void FunctionTreeView::setParamError(const QString &paramName, double error) {
  QString index, name;
  std::tie(index, name) = splitParameterName(paramName);
  if (auto prop = getFunctionProperty(index)) {
    auto children = prop->subProperties();
    foreach (QtProperty *child, children) {
      if (isParameter(child) && child->propertyName() == name) {
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
double FunctionTreeView::getParameter(const QString &paramName) const {
  auto prop = getParameterProperty(paramName);
  return m_parameterManager->value(prop);
}

/// Get a property for a parameter
QtProperty *
FunctionTreeView::getParameterProperty(const QString &paramName) const {
  QString index, name;
  std::tie(index, name) = splitParameterName(paramName);
  if (auto prop = getFunctionProperty(index)) {
    auto children = prop->subProperties();
    foreach (QtProperty *child, children) {
      if (isParameter(child) && child->propertyName() == name) {
        return child;
      }
    }
  }
  std::string message = "Unknown function parameter " +
                        paramName.toStdString() +
                        "\n\n This may happen if there is a CompositeFunction "
                        "containing only one function.";
  throw std::runtime_error(message);
}

/// Get a property for a parameter which is a parent of a given
/// property (tie or constraint).
QtProperty *
FunctionTreeView::getParentParameterProperty(QtProperty *prop) const {
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
 * Remove the function under currently selected property
 */
void FunctionTreeView::removeFunction() {
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
}

/**
 * Fix currently selected parameter
 */
void FunctionTreeView::fixParameter() {
  auto item = m_browser->currentItem();
  if (!item)
    return;
  QtProperty *prop = item->property();
  if (!isParameter(prop))
    return;
  QString tie = QString::number(getParameter(prop));
  addTieProperty(prop, tie);
  emit tiesChanged();
}

/// Get a tie property attached to a parameter property
QtProperty *FunctionTreeView::getTieProperty(QtProperty *prop) const {
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
void FunctionTreeView::removeTie() {
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
  emit tiesChanged();
}

/**
 * Add a custom tie to currently selected parameter
 */
void FunctionTreeView::addTie() {
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
  emit tiesChanged();
}

/**
 * Copy function from the clipboard
 */
void FunctionTreeView::pasteFromClipboard() {
  QString funStr = QApplication::clipboard()->text();
  if (funStr.isEmpty())
    return;
  try {
    auto fun = Mantid::API::FunctionFactory::Instance().createInitialized(
        funStr.toStdString());
    if (!fun)
      return;
    this->setFunction(fun);
    emit functionReplaced(funStr);
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
void FunctionTreeView::copyToClipboard() {
  auto fun = getFunction();
  if (fun) {
    QApplication::clipboard()->setText(QString::fromStdString(fun->asString()));
  }
}

/**
 * Add both constraints to current parameter
 */
void FunctionTreeView::addConstraints() {
  auto item = m_browser->currentItem();
  if (!item)
    return;
  QtProperty *prop = item->property();
  if (!isParameter(prop))
    return;
  addConstraintProperties(prop, "0<" + prop->propertyName() + "<0");
  emit constraintsChanged();
}

/**
 * Add both constraints to current parameter
 */
void FunctionTreeView::addConstraints10() {
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
  emit constraintsChanged();
}

/**
 * Add both constraints to current parameter
 */
void FunctionTreeView::addConstraints50() {
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
  emit constraintsChanged();
}

/**
 * Remove both constraints from current parameter
 */
void FunctionTreeView::removeConstraints() {
  auto item = m_browser->currentItem();
  if (!item)
    return;
  QtProperty *prop = item->property();
  if (!isParameter(prop))
    return;
  //const bool isLocal = isLocalParameterProperty(prop);
  auto props = prop->subProperties();
  foreach (QtProperty *p, props) {
    if (isConstraint(p)) {
      removeProperty(p);
    }
  }
  emit constraintsChanged();
}

/**
 * Remove one constraint from current parameter
 */
void FunctionTreeView::removeConstraint() {
  auto item = m_browser->currentItem();
  if (!item)
    return;
  QtProperty *prop = item->property();
  if (!isConstraint(prop))
    return;
  auto paramProp = getParentParameterProperty(prop);
  removeProperty(prop);
  emit constraintsChanged();
}

/**
 * Slot connected to all function attribute managers. Update the corresponding
 * function.
 * @param prop :: An attribute property that was changed
 */
void FunctionTreeView::attributeChanged(QtProperty *prop) {
  auto funProp = m_properties[prop].parent;
  if (!funProp)
    return;
  // get function with the changed attribute (it is set from prop's value)
  auto fun = getFunction(funProp, true);

  // delete and recreate all function's properties (attributes, parameters, etc)
  setFunction(funProp, fun);
  updateFunctionIndices();
}

/** Called when the size of a vector attribute is changed
 * @param prop :: A property that was changed.
 */
void FunctionTreeView::attributeVectorSizeChanged(QtProperty *prop) {
  QtProperty *vectorProp = m_properties[prop].parent;
  if (!vectorProp)
    throw std::logic_error("FunctionTreeView: inconsistency in vector "
                           "properties.\nAttribute property not found.");
  auto funProp = m_properties[vectorProp].parent;
  if (!funProp)
    throw std::logic_error("FunctionTreeView: inconsistency in vector "
                           "properties.\nFunction property not found.");
  auto fun = getFunction(funProp, true);
  if (!fun)
    throw std::logic_error("FunctionTreeView: inconsistency in vector "
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
  }
}

/**
 * Slot connected to a property displaying the value of a member of a vector
 * attribute.
 * @param prop :: A property that was changed.
 */
void FunctionTreeView::attributeVectorDoubleChanged(QtProperty *prop) {
  QtProperty *vectorProp = m_properties[prop].parent;
  if (!vectorProp)
    throw std::runtime_error(
        "FunctionTreeView: inconsistency in vector properties.");
  attributeChanged(vectorProp);
}

void FunctionTreeView::parameterPropertyChanged(QtProperty *prop) {
  bool isGlobal = true;
  QString newTie;

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
  emit parameterChanged(getParameterName(prop));
}

/// Called when a tie property changes
void FunctionTreeView::tieChanged(QtProperty *prop) {
  for (const auto &atie : m_ties) {
    if (atie.tieProp == prop) {
      emit tiesChanged();
    }
  }
}

/// Called when a constraint property changes
void FunctionTreeView::constraintChanged(QtProperty *prop) {
  for (const auto &constraint : m_constraints) {
    const bool isLower = constraint.lower == prop;
    const bool isUpper = constraint.upper == prop;
    if (isLower || isUpper) {
      emit constraintsChanged();
    }
  }
}

void FunctionTreeView::parameterButtonClicked(QtProperty *prop) {
  emit localParameterButtonClicked(getIndex(prop) + prop->propertyName());
}

bool FunctionTreeView::hasFunction() const {
  return !m_functionManager->properties().isEmpty();
}

/// Resize the browser's columns
/// @param s0 :: New size for the first column (Parameter).
/// @param s1 :: New size for the second column (Value).
/// @param s2 :: New size for the third optional column (Global).
void FunctionTreeView::setColumnSizes(int s0, int s1, int s2) {
  m_browser->setColumnSizes(s0, s1, s2);
}

/**
 * Emit a signal when any of the Global options change.
 */
void FunctionTreeView::globalChanged(QtProperty *, const QString &, bool) {
  emit globalsChanged();
}

/**
 * Set display of parameter errors on/off
 * @param enabled :: [input] On/off display of errors
 */
void FunctionTreeView::setErrorsEnabled(bool enabled) {
  m_parameterManager->setErrorsEnabled(enabled);
}

/**
 * Clear all errors, if they are set
 */
void FunctionTreeView::clearErrors() { m_parameterManager->clearErrors(); }

boost::optional<QString> FunctionTreeView::currentFunctionIndex() const {
  return m_currentFunctionIndex;
}

void FunctionTreeView::updateCurrentFunctionIndex() {
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

void FunctionTreeView::setParameterTie(const QString &paramName, const QString &tie) {
  auto paramProp = getParameterProperty(paramName);
  addTieProperty(paramProp, tie);
}

QTreeWidgetItem *FunctionTreeView::getPropertyWidgetItem(QtProperty *prop) const
{
  return m_browser->getItemWidget(m_properties.find(prop)->item);
}

QRect FunctionTreeView::visualItemRect(QtProperty *prop) const {
  if (!prop) return QRect();
  auto item = getPropertyWidgetItem(prop);
  return item->treeWidget()->visualItemRect(item);
}

QRect FunctionTreeView::getVisualRectFunctionProperty(const QString &index) const {
  QRect rect;
  QtProperty *prop{nullptr};
  try {
    prop = getFunctionProperty(index);
    rect = visualItemRect(prop);
  } catch (std::exception &) {
  }
  return rect;
}

QRect FunctionTreeView::getVisualRectParameterProperty(const QString & paramName) const
{
  QRect rect;
  QtProperty *prop{ nullptr };
  try {
    prop = getParameterProperty(paramName);
    rect = visualItemRect(prop);
  }
  catch (std::exception &) {
  }
  return rect;
}

QWidget * FunctionTreeView::getParamWidget(const QString & paramName) const
{
  QtProperty *prop{ nullptr };
  try {
    prop = getParameterProperty(paramName);
    auto item = getPropertyWidgetItem(prop);
    return item->treeWidget()->itemWidget(item, 1);
  }
  catch (std::exception &) {
  }
  return nullptr;
}

QTreeWidget *FunctionTreeView::treeWidget() const {
  return m_browser->treeWidget();
}

} // namespace MantidWidgets
} // namespace MantidQt
