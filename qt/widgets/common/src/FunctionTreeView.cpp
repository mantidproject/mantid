// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
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

#include "MantidQtWidgets/Common/EditLocalParameterDialog.h"
#include "MantidQtWidgets/Common/InterfaceManager.h"
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
#include <regex>
#include <utility>

using namespace Mantid::API;

namespace {
const char *globalOptionName = "Global";
Mantid::Kernel::Logger g_log("Function Browser");
const std::regex PREFIX_REGEX("(^[f][0-9](.*))");
inline bool variableIsPrefixed(std::string const &name) { return std::regex_match(name, PREFIX_REGEX); }

std::string insertPrefix(std::string const &param) {
  auto const parameterName = QString::fromStdString(param);
  return parameterName.left(parameterName.indexOf(".") + 1).toStdString() + "f0." +
         parameterName.right(parameterName.size() - parameterName.indexOf(".") - 1).toStdString();
}

std::string addPrefix(std::string const &param) { return "f0." + param; }

std::string removeEmbeddedPrefix(std::string const &param) {
  if (variableIsPrefixed(param)) {
    const auto paramSplit = QString::fromStdString(param).split(".");
    return paramSplit[0].toStdString() + "." + paramSplit[paramSplit.size() - 1].toStdString();
  } else {
    return param;
  }
}

std::string removePrefix(std::string const &param) {
  if (variableIsPrefixed(param)) {
    const auto paramSplit = QString::fromStdString(param).split(".");
    return paramSplit[paramSplit.size() - 1].toStdString();
  } else {
    return param;
  }
}

bool containsOneOf(std::string const &str, std::string const &delimiters) {
  return !str.empty() && str.find_first_of(delimiters) != std::string::npos;
}

// These attributes require the function to be fully reconstructed, as a
// different number of properties will be required
const std::vector<std::string> REQUIRESRECONSTRUCTIONATTRIBUTES = {"n", "Formula", "Workspace"};
} // namespace

namespace MantidQt::MantidWidgets {

/**
 * Constructor
 * @param parent :: The parent widget.
 * @param multi  :: Option to use the browser for multi-dataset fitting.
 * @param categories :: Function categories to be included to the Add Function
 * dialog. An empty vector means include all available categories.
 */
FunctionTreeView::FunctionTreeView(QWidget *parent, bool multi, std::vector<std::string> categories)
    : IFunctionView(parent), m_multiDataset(multi), m_multiDomainFunctionPrefix(),
      m_allowedCategories(std::move(categories)), m_selectFunctionDialog(nullptr)

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
FunctionTreeView::~FunctionTreeView() {
  m_browser->unsetFactoryForManager(m_parameterManager);
  m_browser->unsetFactoryForManager(m_attributeStringManager);
  m_browser->unsetFactoryForManager(m_attributeDoubleManager);
  m_browser->unsetFactoryForManager(m_attributeIntManager);
  m_browser->unsetFactoryForManager(m_attributeBoolManager);
  m_browser->unsetFactoryForManager(m_indexManager);
  m_browser->unsetFactoryForManager(m_tieManager);
  m_browser->unsetFactoryForManager(m_constraintManager);
  m_browser->unsetFactoryForManager(m_filenameManager);
  m_browser->unsetFactoryForManager(m_formulaManager);
  m_browser->unsetFactoryForManager(m_workspaceManager);
  m_browser->unsetFactoryForManager(m_attributeSizeManager);
  m_browser->unsetFactoryForManager(m_attributeVectorDoubleManager);
}

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
  m_constraintManager = new QtDoublePropertyManager(this);
  m_filenameManager = new QtStringPropertyManager(this);
  m_formulaManager = new QtStringPropertyManager(this);
  m_workspaceManager = new QtStringPropertyManager(this);
  m_attributeVectorManager = new QtGroupPropertyManager(this);
  m_attributeSizeManager = new QtIntPropertyManager(this);
  m_attributeVectorDoubleManager = new QtDoublePropertyManager(this);

  // create editor factories
  QtSpinBoxFactory *spinBoxFactory = new QtSpinBoxFactory(this);
  DoubleEditorFactory *dblEditorFactory = new DoubleEditorFactory(this);
  ParameterEditorFactory *paramEditorFactory = new ParameterEditorFactory(this);

  QtAbstractEditorFactory<ParameterPropertyManager> *parameterEditorFactory(nullptr);
  if (m_multiDataset) {
    m_doubleEditorFactory = new DoubleDialogEditorFactory(this);
    auto compositeFactory = new CompositeEditorFactory<ParameterPropertyManager>(this, m_doubleEditorFactory);
    compositeFactory->setSecondaryFactory(globalOptionName, paramEditorFactory);
    parameterEditorFactory = compositeFactory;
    connect(m_doubleEditorFactory, SIGNAL(buttonClicked(QtProperty *)), this,
            SLOT(parameterButtonClicked(QtProperty *)));
    connect(m_doubleEditorFactory, SIGNAL(closeEditor()), m_browser, SLOT(closeEditor()));
  } else {
    parameterEditorFactory = paramEditorFactory;
  }

  QtLineEditFactory *lineEditFactory = new QtLineEditFactory(this);
  QtCheckBoxFactory *checkBoxFactory = new QtCheckBoxFactory(this);
  FilenameDialogEditorFactory *filenameDialogEditorFactory = new FilenameDialogEditorFactory(this);
  FormulaDialogEditorFactory *formulaDialogEditFactory = new FormulaDialogEditorFactory(this);
  WorkspaceEditorFactory *workspaceEditorFactory = new WorkspaceEditorFactory(this);

  // assign factories to property managers
  m_browser->setFactoryForManager(m_parameterManager, parameterEditorFactory);
  m_browser->setFactoryForManager(m_attributeStringManager, lineEditFactory);
  m_browser->setFactoryForManager(m_attributeDoubleManager, dblEditorFactory);
  m_browser->setFactoryForManager(m_attributeIntManager, spinBoxFactory);
  m_browser->setFactoryForManager(m_attributeBoolManager, checkBoxFactory);
  m_browser->setFactoryForManager(m_indexManager, lineEditFactory);
  m_browser->setFactoryForManager(m_tieManager, lineEditFactory);
  m_browser->setFactoryForManager(m_constraintManager, dblEditorFactory);
  m_browser->setFactoryForManager(m_filenameManager, filenameDialogEditorFactory);
  m_browser->setFactoryForManager(m_formulaManager, formulaDialogEditFactory);
  m_browser->setFactoryForManager(m_workspaceManager, workspaceEditorFactory);
  m_browser->setFactoryForManager(m_attributeSizeManager, spinBoxFactory);
  m_browser->setFactoryForManager(m_attributeVectorDoubleManager, dblEditorFactory);

  m_browser->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_browser, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(popupMenu(const QPoint &)));
  connect(m_browser, SIGNAL(optionChanged(QtProperty *, const QString &, bool)), this,
          SLOT(globalChanged(QtProperty *, const QString &, bool)));

  connect(m_attributeStringManager, SIGNAL(propertyChanged(QtProperty *)), this, SLOT(attributeChanged(QtProperty *)));
  connect(m_attributeDoubleManager, SIGNAL(propertyChanged(QtProperty *)), this, SLOT(attributeChanged(QtProperty *)));
  connect(m_attributeIntManager, SIGNAL(propertyChanged(QtProperty *)), this, SLOT(attributeChanged(QtProperty *)));
  connect(m_attributeBoolManager, SIGNAL(propertyChanged(QtProperty *)), this, SLOT(attributeChanged(QtProperty *)));
  connect(m_formulaManager, SIGNAL(propertyChanged(QtProperty *)), this, SLOT(attributeChanged(QtProperty *)));
  connect(m_filenameManager, SIGNAL(propertyChanged(QtProperty *)), this, SLOT(attributeChanged(QtProperty *)));
  connect(m_workspaceManager, SIGNAL(propertyChanged(QtProperty *)), this, SLOT(attributeChanged(QtProperty *)));
  connect(m_attributeVectorDoubleManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(attributeVectorDoubleChanged(QtProperty *)));
  connect(m_attributeSizeManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(attributeVectorSizeChanged(QtProperty *)));
  connect(m_tieManager, SIGNAL(propertyChanged(QtProperty *)), this, SLOT(tieChanged(QtProperty *)));
  connect(m_constraintManager, SIGNAL(propertyChanged(QtProperty *)), this, SLOT(constraintChanged(QtProperty *)));
  connect(m_parameterManager, SIGNAL(valueChanged(QtProperty *, double)), SLOT(parameterPropertyChanged(QtProperty *)));

  connect(m_browser, SIGNAL(currentItemChanged(QtBrowserItem *)), SLOT(updateCurrentFunctionIndex()));

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
  connect(m_actionRemoveFunction, SIGNAL(triggered()), this, SLOT(removeFunction()));

  m_actionFixParameter = new QAction("Fix", this);
  connect(m_actionFixParameter, SIGNAL(triggered()), this, SLOT(fixParameter()));

  m_actionRemoveTie = new QAction("Remove tie", this);
  connect(m_actionRemoveTie, SIGNAL(triggered()), this, SLOT(removeTie()));

  m_actionAddTie = new QAction("Add tie", this);
  connect(m_actionAddTie, SIGNAL(triggered()), this, SLOT(addTie()));

  m_actionFromClipboard = new QAction("Paste from clipboard", this);
  m_actionFromClipboard->setObjectName("paste_from_clipboard");
  connect(m_actionFromClipboard, SIGNAL(triggered()), this, SLOT(pasteFromClipboard()));

  m_actionToClipboard = new QAction("Copy to clipboard", this);
  m_actionToClipboard->setObjectName("copy_to_clipboard");
  connect(m_actionToClipboard, SIGNAL(triggered()), this, SLOT(copyToClipboard()));

  m_actionConstraints = new QAction("Custom", this);
  connect(m_actionConstraints, SIGNAL(triggered()), this, SLOT(addConstraints()));

  m_actionConstraints10 = new QAction("10%", this);
  connect(m_actionConstraints10, SIGNAL(triggered()), this, SLOT(addConstraints10()));

  m_actionConstraints50 = new QAction("50%", this);
  connect(m_actionConstraints50, SIGNAL(triggered()), this, SLOT(addConstraints50()));

  m_actionRemoveConstraints = new QAction("Remove constraints", this);
  connect(m_actionRemoveConstraints, SIGNAL(triggered()), this, SLOT(removeConstraints()));

  m_actionRemoveConstraint = new QAction("Remove", this);
  connect(m_actionRemoveConstraint, SIGNAL(triggered()), this, SLOT(removeConstraint()));

  m_actionFunctionHelp = new QAction("Help", this);
  connect(m_actionFunctionHelp, SIGNAL(triggered()), this, SIGNAL(functionHelpRequest()));

  setErrorsEnabled(false);
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
  if (fun) {
    addFunction(nullptr, fun);
  }
}

/**
 * Add a sub-property to a parent property
 * @param parent :: The parent property
 * @param subproperty :: New sub-property
 */
FunctionTreeView::AProperty FunctionTreeView::addProperty(QtProperty *parent, QtProperty *subproperty) {
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
  foreach (QtProperty *child, children) {
    removeProperty(child);
  }
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
FunctionTreeView::AProperty FunctionTreeView::addFunctionProperty(QtProperty *parent, std::string const &funName) {
  // check that parent is a function property
  if (parent && dynamic_cast<QtAbstractPropertyManager *>(m_functionManager) != parent->propertyManager()) {
    throw std::runtime_error("Unexpected error in FunctionTreeView [2]");
  }
  QtProperty *prop = m_functionManager->addProperty(QString::fromStdString(funName));
  return addProperty(parent, prop);
}

/**
 * Add a parameter property
 * @param parent :: Parent function property
 * @param parameterName :: Parameter name
 * @param paramDesc :: Parameter description
 * @param paramValue :: Parameter value
 */
FunctionTreeView::AProperty FunctionTreeView::addParameterProperty(QtProperty *parent, std::string const &parameterName,
                                                                   std::string const &paramDesc, double paramValue) {
  // check that parent is a function property
  if (!parent || dynamic_cast<QtAbstractPropertyManager *>(m_functionManager) != parent->propertyManager()) {
    throw std::runtime_error("Unexpected error in FunctionTreeView [3]");
  }
  QtProperty *prop = m_parameterManager->addProperty(QString::fromStdString(parameterName));
  m_parameterManager->blockSignals(true);
  m_parameterManager->setDecimals(prop, 6);
  m_parameterManager->setValue(prop, paramValue);
  m_parameterManager->setDescription(prop, paramDesc);
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
void FunctionTreeView::setFunction(QtProperty *prop, const Mantid::API::IFunction_sptr &fun) {
  auto children = prop->subProperties();
  foreach (QtProperty *child, children) {
    removeProperty(child);
  }
  // m_localParameterValues.clear();
  if (!m_multiDomainFunctionPrefix.empty())
    addMultiDomainIndexProperty(prop);
  addAttributeAndParameterProperties(prop, fun);
}

/**
 * Add a function.
 * @param prop :: Property of the parent composite function or NULL
 * @param fun :: A function to add
 */
bool FunctionTreeView::addFunction(QtProperty *prop, const Mantid::API::IFunction_sptr &fun) {
  if (!fun)
    return false;
  if (!prop) {
    AProperty ap = addFunctionProperty(nullptr, fun->name());
    setFunction(ap.prop, fun);
  } else {
    Mantid::API::IFunction_sptr parentFun = getFunction(prop);
    if (!parentFun)
      return false;
    auto cf = std::dynamic_pointer_cast<Mantid::API::CompositeFunction>(parentFun);
    if (!cf) {
      throw std::logic_error("FunctionTreeView: CompositeFunction is expected for addFunction");
    }
    try {
      cf->addFunction(fun);
    } catch (const std::exception &e) {
      QMessageBox::warning(this, "Mantid - Warning", QString("Cannot Add function:\n\n%1").arg(e.what()));
      return false;
    }
    setFunction(prop, cf);
  }
  updateFunctionIndices();
  return true;
}

/**
 * Attribute visitor to create a QtProperty. Depending on the attribute type
 * the appropriate apply() method is used.
 */
class CreateAttributePropertyForFunctionTreeView
    : public Mantid::API::IFunction::ConstAttributeVisitor<FunctionTreeView::AProperty> {
public:
  CreateAttributePropertyForFunctionTreeView(FunctionTreeView *browser, QtProperty *parent, std::string const &attName)
      : m_browser(browser), m_parent(parent), m_attName(QString::fromStdString(attName)) {
    // check that parent is a function property
    if (!m_parent ||
        dynamic_cast<QtAbstractPropertyManager *>(m_browser->m_functionManager) != m_parent->propertyManager()) {
      throw std::runtime_error("Unexpected error in FunctionTreeView [4]");
    }
  }

protected:
  /// Create string property
  FunctionTreeView::AProperty apply(const std::string &str) const override {
    QtProperty *prop = nullptr;
    if (m_attName.indexOf("FileName") != -1) {
      m_browser->m_filenameManager->blockSignals(true);
      prop = m_browser->m_filenameManager->addProperty(m_attName);
      m_browser->m_filenameManager->setValue(prop, QString::fromStdString(str));
      m_browser->m_filenameManager->blockSignals(false);
    } else if (m_attName.indexOf("Formula") != -1) {
      m_browser->m_formulaManager->blockSignals(true);
      prop = m_browser->m_formulaManager->addProperty(m_attName);
      m_browser->m_formulaManager->setValue(prop, QString::fromStdString(str));
      m_browser->m_formulaManager->blockSignals(false);
    } else if (m_attName.indexOf("Workspace") != -1) {
      m_browser->m_workspaceManager->blockSignals(true);
      prop = m_browser->m_workspaceManager->addProperty(m_attName);
      m_browser->m_workspaceManager->setValue(prop, QString::fromStdString(str));
      m_browser->m_workspaceManager->blockSignals(false);
    } else {
      m_browser->m_attributeStringManager->blockSignals(true);
      prop = m_browser->m_attributeStringManager->addProperty(m_attName);
      m_browser->m_attributeStringManager->setValue(prop, QString::fromStdString(str));
      m_browser->m_attributeStringManager->blockSignals(false);
    }
    return m_browser->addProperty(m_parent, prop);
  }
  /// Create double property
  FunctionTreeView::AProperty apply(const double &d) const override {
    m_browser->m_attributeDoubleManager->blockSignals(true);
    QtProperty *prop = m_browser->m_attributeDoubleManager->addProperty(m_attName);
    m_browser->m_attributeDoubleManager->setValue(prop, d);
    m_browser->m_attributeDoubleManager->blockSignals(false);
    return m_browser->addProperty(m_parent, prop);
  }
  /// Create int property
  FunctionTreeView::AProperty apply(const int &i) const override {
    m_browser->m_attributeIntManager->blockSignals(true);
    QtProperty *prop = m_browser->m_attributeIntManager->addProperty(m_attName);
    prop->setEnabled(i < std::numeric_limits<int>::max());
    m_browser->m_attributeIntManager->setValue(prop, i);
    m_browser->m_attributeIntManager->blockSignals(false);
    return m_browser->addProperty(m_parent, prop);
  }
  /// Create bool property
  FunctionTreeView::AProperty apply(const bool &b) const override {
    m_browser->m_attributeBoolManager->blockSignals(true);
    QtProperty *prop = m_browser->m_attributeBoolManager->addProperty(m_attName);
    m_browser->m_attributeBoolManager->setValue(prop, b);
    m_browser->m_attributeBoolManager->blockSignals(false);
    return m_browser->addProperty(m_parent, prop);
  }
  /// Create vector property
  FunctionTreeView::AProperty apply(const std::vector<double> &v) const override {
    QtProperty *prop = m_browser->m_attributeVectorManager->addProperty(m_attName);
    FunctionTreeView::AProperty aprop = m_browser->addProperty(m_parent, prop);

    m_browser->m_attributeSizeManager->blockSignals(true);
    QtProperty *sizeProp = m_browser->m_attributeSizeManager->addProperty("Size");
    m_browser->m_attributeSizeManager->setValue(sizeProp, static_cast<int>(v.size()));
    m_browser->addProperty(prop, sizeProp);
    m_browser->m_attributeSizeManager->blockSignals(false);

    m_browser->m_attributeVectorDoubleManager->blockSignals(true);
    QString parName = "value[%1]";
    for (size_t i = 0; i < v.size(); ++i) {
      QtProperty *dprop = m_browser->m_attributeVectorDoubleManager->addProperty(parName.arg(i));
      m_browser->m_attributeVectorDoubleManager->setValue(dprop, v[i]);
      m_browser->addProperty(prop, dprop);
    }
    m_browser->m_attributeVectorDoubleManager->blockSignals(false);

    sizeProp->setEnabled(false);
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
class SetAttributeFromProperty : public Mantid::API::IFunction::AttributeVisitor<> {
public:
  SetAttributeFromProperty(FunctionTreeView *browser, QtProperty *prop) : m_browser(browser), m_prop(prop) {}

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
  void apply(double &d) const override { d = m_browser->m_attributeDoubleManager->value(m_prop); }
  /// Set int attribute
  void apply(int &i) const override { i = m_browser->m_attributeIntManager->value(m_prop); }
  /// Set bool attribute
  void apply(bool &b) const override { b = m_browser->m_attributeBoolManager->value(m_prop); }
  /// Set vector attribute
  void apply(std::vector<double> &v) const override {
    QList<QtProperty *> members = m_prop->subProperties();
    if (members.empty())
      throw std::runtime_error("FunctionTreeView: empty vector attribute group.");
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
FunctionTreeView::AProperty FunctionTreeView::addAttributeProperty(QtProperty *parent, std::string const &attName,
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
 * @param parentComposite :: If relevant, the composite the function is part of.
 * @param parentIndex :: If relevant, the index of the function within its
 * composite.
 */
void FunctionTreeView::addAttributeAndParameterProperties(QtProperty *prop, const IFunction_sptr &fun,
                                                          const CompositeFunction_sptr &parentComposite,
                                                          const std::size_t &parentIndex) {
  // add the function index property
  addIndexProperty(prop);

  // add attribute properties
  auto attributeNames = fun->getAttributeNames();
  for (const auto &att : attributeNames) {
    if (!variableIsPrefixed(att)) {
      addAttributeProperty(prop, att, fun->getAttribute(att));
    }
  }

  auto cf = std::dynamic_pointer_cast<Mantid::API::CompositeFunction>(fun);
  if (cf) {
    // if composite add members
    for (size_t i = 0; i < cf->nFunctions(); ++i) {
      AProperty ap = addFunctionProperty(prop, cf->getFunction(i)->name());
      addAttributeAndParameterProperties(ap.prop, cf->getFunction(i), cf, i);
    }
  } else { // if simple add parameters
    for (size_t i = 0; i < fun->nParams(); ++i) {
      auto name = fun->parameterName(i);
      auto desc = fun->parameterDescription(i);
      double value = fun->getParameter(i);
      AProperty ap = addParameterProperty(prop, name, desc, value);
      // if parameter has a tie
      if (!fun->isActive(i))
        addParameterTie(ap.prop, fun, name, i, parentComposite, parentIndex);
      else
        addGlobalParameterTie(ap.prop, name, parentComposite, parentIndex);

      if (const auto constraint = fun->getConstraint(i)) {
        addConstraintProperties(ap.prop, constraint->asString());
      }
    }
  }
}

/**
 * Add a tie to a function property. If the tie is stored within the wider
 * composite function, it will find this tie and apply it.
 * @param property :: A function property.
 * @param function :: Shared pointer to the function.
 * @param parameterName :: The name of the parameter to be tied.
 * @param parameterIndex :: The index of the parameter within its function.
 * @param parentComposite :: If relevant, the composite the function is part of.
 * @param parentIndex :: If relevant, the index of the function within its
 * composite.
 */
void FunctionTreeView::addParameterTie(QtProperty *property, const IFunction_sptr &function,
                                       const std::string &parameterName, const std::size_t &parameterIndex,
                                       const CompositeFunction_sptr &parentComposite, const std::size_t &parentIndex) {
  if (const auto tie = function->getTie(parameterIndex)) {
    addTieProperty(property, tie->asString());
  } else {
    auto tieAdded = false;
    if (parentComposite)
      tieAdded = addParameterTieInComposite(property, parameterName, parentComposite, parentIndex);

    if (!tieAdded)
      addTieProperty(property, QString::number(function->getParameter(parameterIndex)).toStdString());
  }
}

/**
 * Add a tie to a function property. Used if a tie is stored within the wider
 * composite function.
 * @param property :: A function property.
 * @param parameterName :: The name of the parameter to be tied.
 * @param composite :: The composite function containing the tie.
 * @param index :: The index of the function within the composite function.
 * @returns true if a tie was found, and a tie property was added.
 */
bool FunctionTreeView::addParameterTieInComposite(QtProperty *property, const std::string &parameterName,
                                                  const CompositeFunction_sptr &composite, const std::size_t &index) {
  for (auto i = 0u; i < composite->nParams(); ++i) {
    const auto fullName = "f" + std::to_string(index) + "." + parameterName;
    if (fullName == composite->parameterName(i)) {
      if (const auto tie = composite->getTie(i)) {
        const auto tieStr = QString::fromStdString(tie->asString());
        addTieProperty(property, tieStr.mid(tieStr.indexOf('=') + 1).toStdString());
        return true;
      }
      return false;
    }
  }
  return false;
}

/**
 * Add a global tie to a function property if one exists for the specified
 * parameter.
 * @param property :: A function property.
 * @param parameterName :: The name of the parameter to check for a global tie.
 * @param parentComposite :: The composite function the parameter is in. This is
 * a nullptr if the parameter is not in a composite function.
 * @param parentIndex :: The index of the parameter within its composite.
 */
void FunctionTreeView::addGlobalParameterTie(QtProperty *property, const std::string &parameterName,
                                             const CompositeFunction_sptr &parentComposite,
                                             const std::size_t &parentIndex) {
  if (m_multiDomainFunctionPrefix.empty() || m_globalTies.empty())
    return;

  auto const fullName = getFullParameterName(parameterName, parentComposite ? static_cast<int>(parentIndex) : -1);

  const auto it = std::find_if(m_globalTies.cbegin(), m_globalTies.cend(),
                               [&fullName](const auto &globalTie) { return fullName == globalTie.m_parameter; });
  if (it != m_globalTies.cend())
    addTieProperty(property, (*it).m_tie, true);
}

/**
 * Adds a property showing the function index of a domain within a
 * MultiDomainFunction. It is used when we don't want to display an entire
 * MultiDomainFunction, just a specific function within it.
 * @param prop :: The property to add the function index on to.
 */
void FunctionTreeView::addMultiDomainIndexProperty(QtProperty *prop) {
  QtProperty *indexProperty = m_indexManager->addProperty("Index");
  indexProperty->setEnabled(false);
  m_indexManager->setValue(indexProperty, QString::fromStdString(m_multiDomainFunctionPrefix));
  (void)addProperty(prop, indexProperty);
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
void FunctionTreeView::updateFunctionIndices(QtProperty *prop, std::string const &index) {
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
      updateFunctionIndices(child, index + "f" + std::to_string(i) + ".");
      ++i;
    } else if (isIndex(child)) {
      m_indexManager->setValue(child, QString::fromStdString(m_multiDomainFunctionPrefix + index));
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
  return prop && dynamic_cast<QtAbstractPropertyManager *>(m_functionManager) == prop->propertyManager();
}

/**
 * Check if property is any of the string attributes
 * @param prop :: Property to check
 */
bool FunctionTreeView::isStringAttribute(QtProperty *prop) const {
  return prop && (dynamic_cast<QtAbstractPropertyManager *>(m_attributeStringManager) == prop->propertyManager() ||
                  dynamic_cast<QtAbstractPropertyManager *>(m_formulaManager) == prop->propertyManager() ||
                  dynamic_cast<QtAbstractPropertyManager *>(m_filenameManager) == prop->propertyManager() ||
                  dynamic_cast<QtAbstractPropertyManager *>(m_workspaceManager) == prop->propertyManager());
}

/**
 * Check if property is a function attribute
 * @param prop :: Property to check
 */
bool FunctionTreeView::isDoubleAttribute(QtProperty *prop) const {
  return prop && dynamic_cast<QtAbstractPropertyManager *>(m_attributeDoubleManager) == prop->propertyManager();
}

/**
 * Check if property is a function attribute
 * @param prop :: Property to check
 */
bool FunctionTreeView::isIntAttribute(QtProperty *prop) const {
  return prop && dynamic_cast<QtAbstractPropertyManager *>(m_attributeIntManager) == prop->propertyManager();
}

/**
 * Check if property is a function bool attribute
 * @param prop :: Property to check
 */
bool FunctionTreeView::isBoolAttribute(QtProperty *prop) const {
  return prop && dynamic_cast<QtAbstractPropertyManager *>(m_attributeBoolManager) == prop->propertyManager();
}

/**
 * Check if property is a function vector attribute
 * @param prop :: Property to check
 */
bool FunctionTreeView::isVectorAttribute(QtProperty *prop) const {
  return prop && dynamic_cast<QtAbstractPropertyManager *>(m_attributeVectorManager) == prop->propertyManager();
}

/**
 * Check if property is a function attribute
 * @param prop :: Property to check
 */
bool FunctionTreeView::isAttribute(QtProperty *prop) const {
  return isStringAttribute(prop) || isDoubleAttribute(prop) || isIntAttribute(prop) || isBoolAttribute(prop) ||
         isVectorAttribute(prop);
}

/**
 * Check if property is a function parameter
 * @param prop :: Property to check
 */
bool FunctionTreeView::isParameter(QtProperty *prop) const {
  return prop && dynamic_cast<QtAbstractPropertyManager *>(m_parameterManager) == prop->propertyManager();
}

/**
 * Get parameter value as a string
 * @param prop :: A parameter property
 */
double FunctionTreeView::getParameter(QtProperty *prop) const { return m_parameterManager->value(prop); }
/**
 * Check if a property is an index
 * @param prop :: A property
 */
bool FunctionTreeView::isIndex(QtProperty *prop) const {
  return prop && dynamic_cast<QtAbstractPropertyManager *>(m_indexManager) == prop->propertyManager();
}

/**
 * Get the function index for a property
 * @param prop :: A property
 */
std::string FunctionTreeView::getIndex(QtProperty *prop) const {
  if (!prop)
    return "";
  if (isFunction(prop)) {
    auto props = prop->subProperties();
    if (props.isEmpty())
      return "";
    const auto it = std::find_if(props.cbegin(), props.cend(), [&](const auto &prop) { return isIndex(prop); });
    if (it != props.cend()) {
      return m_indexManager->value(*it).toStdString();
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
std::string FunctionTreeView::getParameterName(QtProperty *prop) const {
  if (isParameter(prop)) {
    return getIndex(prop) + prop->propertyName().toStdString();
  } else {
    auto const *parent = getParentParameterProperty(prop);
    return getIndex(prop) + parent->propertyName().toStdString();
  }
}
/**
 * Get name of the attribute for a property
 * @param prop :: A property
 */
std::string FunctionTreeView::getAttributeName(QtProperty *prop) const {
  if (isAttribute(prop)) {
    return getIndex(prop) + prop->propertyName().toStdString();
  }
  throw std::logic_error("QtProperty " + prop->propertyName().toStdString() + " is not an attribute property.");
}

/**
 * Return function property for a function with given index.
 * @param index :: Function index to search, or empty string for top-level
 * function
 * @return Function property, or NULL if not found
 */
QtProperty *FunctionTreeView::getFunctionProperty(std::string const &index) const {
  // Might not be the most efficient way to do it. m_functionManager might be
  // searched instead,
  // but it is not being kept up-to-date at the moment (is not cleared).
  for (const auto &property : m_properties.keys()) {
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
 * @param globalTie :: true if the tie is a global tie.
 */
void FunctionTreeView::addTieProperty(QtProperty *prop, std::string const &tie, bool globalTie) {
  if (!prop) {
    throw std::runtime_error("FunctionTreeView: null property pointer");
  }

  if (!isParameter(prop))
    return;

  QtProperty *funProp = getFunctionProperty().prop;

  // Create and add a QtProperty for the tie.
  m_tieManager->blockSignals(true);
  QtProperty *tieProp = m_tieManager->addProperty("Tie");
  m_tieManager->setValue(tieProp, QString::fromStdString(globalTie ? tie : getFullTie(tie)));
  addProperty(prop, tieProp);
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
 * Returns the full tie to add as a Tie property. This will add the
 * m_multiDomainFunctionPrefix to the start if we are using multiple datasets.
 * @param tie :: The original tie.
 * @returns The full tie to use as a tie property.
 */
std::string FunctionTreeView::getFullTie(std::string const &tie) const {
  if (!isNumber(tie) && !containsOneOf(tie, "="))
    return m_multiDomainFunctionPrefix + tie;
  return tie;
}

/**
 * Returns the full parameter name. This will add the
 * m_multiDomainFunctionPrefix to the start if we are using multiple datasets,
 * and will add the composite index if it is within a composite.
 * @param parameter :: The original parameter.
 * @param compositeIndex :: The index of the function within the composite that
 * the parameter is in.
 * @returns The full parameter name.
 */
std::string FunctionTreeView::getFullParameterName(const std::string &parameter, int compositeIndex) const {
  auto fullParameterName = m_multiDomainFunctionPrefix;
  if (compositeIndex != -1)
    fullParameterName += "f" + std::to_string(compositeIndex) + ".";
  fullParameterName += parameter;
  return fullParameterName;
}

/**
 * Check if a parameter property has a tie
 * @param prop :: A parameter property
 */
bool FunctionTreeView::hasTie(QtProperty *prop) const {
  if (!prop)
    return false;
  const auto children = prop->subProperties();
  return std::any_of(children.cbegin(), children.cend(),
                     [](const auto child) { return child->propertyName() == "Tie"; });
}

/**
 * Check if a property is a tie
 * @param prop :: A property
 */
bool FunctionTreeView::isTie(QtProperty *prop) const {
  return prop && dynamic_cast<QtAbstractPropertyManager *>(m_tieManager) == prop->propertyManager();
}

/**
 * Get a tie for a parameter
 * @param prop :: A parameter property
 */
QString FunctionTreeView::getTie(QtProperty *prop) const {
  if (!prop)
    return "";
  if (prop->propertyName() == "Tie") {
    return m_tieManager->value(prop);
  }
  const auto children = prop->subProperties();
  const auto it =
      std::find_if(children.cbegin(), children.cend(), [](const auto child) { return child->propertyName() == "Tie"; });
  if (it != children.cend()) {
    return m_tieManager->value(*it);
  }
  return "";
}

/**
 * Add a constraint property
 * @param prop :: Parent parameter property
 * @param constraint :: A constraint string
 */
QList<FunctionTreeView::AProperty> FunctionTreeView::addConstraintProperties(QtProperty *prop,
                                                                             std::string const &constraint) {
  if (!isParameter(prop))
    return QList<FunctionTreeView::AProperty>();
  auto const parts = splitConstraintString(constraint);
  if (parts.first.empty())
    return QList<FunctionTreeView::AProperty>();
  auto lowerBound = parts.second.first;
  auto upperBound = parts.second.second;

  // add properties
  QList<FunctionTreeView::AProperty> plist;
  AConstraint ac;
  ac.paramProp = prop;
  ac.lower = ac.upper = nullptr;

  if (!lowerBound.empty()) {
    auto apLower = addProperty(prop, m_constraintManager->addProperty("LowerBound"));
    plist << apLower;
    ac.lower = apLower.prop;
    m_constraintManager->setValue(ac.lower, QString::fromStdString(lowerBound).toDouble());
  }

  if (!upperBound.empty()) {
    auto apUpper = addProperty(prop, m_constraintManager->addProperty("UpperBound"));
    plist << apUpper;
    ac.upper = apUpper.prop;
    m_constraintManager->setValue(ac.upper, QString::fromStdString(upperBound).toDouble());
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
  return prop && dynamic_cast<QtAbstractPropertyManager *>(m_constraintManager) == prop->propertyManager();
}

/**
 * Check if a parameter property has a constraint
 * @param prop :: A parameter property.
 */
bool FunctionTreeView::hasConstraint(QtProperty *prop) const { return hasLowerBound(prop) || hasUpperBound(prop); }

/**
 * Check if a parameter property has a lower bound
 * @param prop :: A parameter property.
 */
bool FunctionTreeView::hasLowerBound(QtProperty *prop) const {
  if (!isParameter(prop))
    return false;
  const auto props = prop->subProperties();
  return std::any_of(props.cbegin(), props.cend(), [&](const auto &p) {
    return dynamic_cast<QtAbstractPropertyManager *>(m_constraintManager) == p->propertyManager() &&
           p->propertyName() == "LowerBound";
  });
}

/**
 * Check if a parameter property has a upper bound
 * @param prop :: A parameter property.
 */
bool FunctionTreeView::hasUpperBound(QtProperty *prop) const {
  if (!isParameter(prop))
    return false;
  const auto props = prop->subProperties();
  return std::any_of(props.cbegin(), props.cend(), [&](const auto &p) {
    return dynamic_cast<QtAbstractPropertyManager *>(m_constraintManager) == p->propertyManager() &&
           p->propertyName() == "UpperBound";
  });
}

/// Get a constraint string
std::string FunctionTreeView::getConstraint(std::string const &parameterName, const double &lowerBound,
                                            const double &upperBound) const {

  std::string constraint;
  if (lowerBound != Mantid::EMPTY_DBL())
    constraint += QString::number(lowerBound).toStdString() + "<";

  constraint += parameterName;
  if (upperBound != Mantid::EMPTY_DBL())
    constraint += "<" + QString::number(upperBound).toStdString();

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
        Mantid::API::FunctionFactory::Instance().createFunction(prop->propertyName().toStdString());
    auto cf = std::dynamic_pointer_cast<Mantid::API::CompositeFunction>(fun);
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
    context.addAction(m_actionFunctionHelp);
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
  m_selectFunctionDialog->clearSearchBoxText();
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
  auto f = Mantid::API::FunctionFactory::Instance().createFunction(newFunction.toStdString());

  // get previous global parameters
  auto globalParameters = getGlobalParameters();

  auto prop = m_selectedFunctionProperty;
  if (prop) { // there are other functions defined
    Mantid::API::IFunction_sptr fun =
        Mantid::API::FunctionFactory::Instance().createFunction(prop->propertyName().toStdString());
    auto cf = std::dynamic_pointer_cast<Mantid::API::CompositeFunction>(fun);
    if (cf) {
      auto const isAdded = addFunction(prop, f);
      if (!isAdded)
        return;
    } else {
      cf.reset(new Mantid::API::CompositeFunction);
      auto f0 = getFunction(prop);
      if (f0) {
        // Modify the previous globals so they have a function prefix
        std::transform(globalParameters.begin(), globalParameters.end(), globalParameters.begin(),
                       m_multiDomainFunctionPrefix.empty() ? addPrefix : insertPrefix);
        cf->addFunction(f0);
      }
      cf->addFunction(f);
      setFunction(cf);
    }
  } else { // the browser is empty - add first function
    auto const isAdded = addFunction(nullptr, f);
    if (!isAdded)
      return;
  }
  emit functionAdded(f->asString());
  setGlobalParameters(globalParameters);
  emit globalsChanged(globalParameters);
}

/**
 * Set value of an attribute (as a property) to a function.
 * @param fun :: Function to which attribute is set.
 * @param prop :: A property with the name and value of the attribute.
 */
void FunctionTreeView::setAttributeToFunction(Mantid::API::IFunction &fun, QtProperty *prop) {
  std::string attName = prop->propertyName().toStdString();
  SetAttributeFromProperty setter(this, prop);
  Mantid::API::IFunction::Attribute attr = fun.getAttribute(attName);
  attr.apply(setter);
  try {
    fun.setAttribute(attName, attr);
  } catch (std::exception &expt) {
    QMessageBox::critical(this, "Mantid - Error",
                          "Cannot set attribute " + QString::fromStdString(attName) + " of function " +
                              prop->propertyName() + ":\n\n" + QString::fromStdString(expt.what()));
  }
}

/**
 * Return the function
 * @param prop :: Function property
 * @param attributesOnly :: Only set attributes
 */
Mantid::API::IFunction_sptr FunctionTreeView::getFunction(QtProperty *prop, bool attributesOnly) {
  if (prop == nullptr) { // get overall function
    auto props = m_browser->properties();
    if (props.isEmpty())
      return Mantid::API::IFunction_sptr();
    prop = props[0];
  }
  if (!isFunction(prop))
    return Mantid::API::IFunction_sptr();

  // construct the function
  auto fun = Mantid::API::FunctionFactory::Instance().createFunction(prop->propertyName().toStdString());
  auto cf = std::dynamic_pointer_cast<Mantid::API::CompositeFunction>(fun);
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
        fun->setParameter(child->propertyName().toStdString(), getParameter(child));
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
      auto const tie = (it->paramName + "=" + m_tieManager->value(it.value().tieProp).toStdString());
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
        const auto constraint = getConstraint(localParam.paramProp->propertyName().toStdString(), lower, upper);
        fun->addConstraints(constraint);
      } catch (...) {
      }
    }
  }

  return fun;
}

/**
 * Updates the function parameter value
 * @param parameterName :: Parameter name
 * @param value :: New value
 */
void FunctionTreeView::setParameter(std::string const &parameterName, double value) {
  auto prop = getParameterProperty(parameterName);
  ScopedFalse _false(m_emitParameterValueChange);
  m_parameterManager->setValue(prop, value);
}

/**
 * Updates the function parameter error
 * @param parameterName :: Parameter name
 * @param error :: New error
 */
void FunctionTreeView::setParameterError(std::string const &parameterName, double error) {
  std::string index, name;
  std::tie(index, name) = splitParameterName(parameterName);
  if (auto const *prop = getFunctionProperty(index)) {
    auto children = prop->subProperties();
    const auto it = std::find_if(children.cbegin(), children.cend(), [&](auto &child) {
      return isParameter(child) && child->propertyName().toStdString() == name;
    });
    if (it != children.cend()) {
      m_parameterManager->setError(*it, error);
    }
  }
}
/**
 * Updates the value of a double attribute
 * @param attrName :: Attribute name
 * @param value :: New value
 */
void FunctionTreeView::setDoubleAttribute(std::string const &attrName, double value) {
  auto prop = getAttributeProperty(attrName);
  ScopedFalse _false(m_emitAttributeValueChange);
  m_attributeDoubleManager->setValue(prop, value);
}
/**
 * Updates the value of a integer attribute
 * @param attrName :: Attribute name
 * @param value :: New value
 */
void FunctionTreeView::setIntAttribute(std::string const &attrName, int value) {
  auto prop = getAttributeProperty(attrName);
  ScopedFalse _false(m_emitAttributeValueChange);
  m_attributeIntManager->setValue(prop, value);
}
/**
 * Updates the value of a string attribute
 * @param attrName :: Attribute name
 * @param valueAsStdStr :: New value
 */
void FunctionTreeView::setStringAttribute(std::string const &attrName, std::string &valueAsStdStr) {
  QString value = QString::fromStdString(valueAsStdStr);
  auto prop = getAttributeProperty(attrName);
  ScopedFalse _false(m_emitAttributeValueChange);
  // attName is the un-prefixed attribute name
  auto attName = prop->propertyName();
  if (attName == "FileName") {
    m_filenameManager->setValue(prop, value);
  } else if (attName == "Formula") {
    m_formulaManager->setValue(prop, value);
  } else if (attName == "Workspace") {
    m_workspaceManager->setValue(prop, value);
  } else {
    m_attributeStringManager->setValue(prop, value);
  }
}
/**
 * Updates the value of a boolean attribute
 * @param attrName :: Attribute name
 * @param value :: New value
 */
void FunctionTreeView::setBooleanAttribute(std::string const &attrName, bool value) {
  auto prop = getAttributeProperty(attrName);
  ScopedFalse _false(m_emitAttributeValueChange);
  m_attributeBoolManager->setValue(prop, value);
}
/**
 * Updates the value of a vector attribute
 * NOTE: This is currently not implemented as there is no need for it
 * as the use of vector attributes is limited to tabulated functions (e.g
 * resolution) which create their attributes 'on-the-fly' when the fit is
 * performed
 * @param attrName :: Attribute name
 * @param value :: New value
 */
void FunctionTreeView::setVectorAttribute(std::string const &attrName, std::vector<double> &value) {
  UNUSED_ARG(attrName);
  UNUSED_ARG(value);
}
/**
 * Get a value of a parameter
 * @param parameterName :: Parameter name
 */
double FunctionTreeView::getParameter(std::string const &parameterName) const {
  auto const *prop = getParameterProperty(parameterName);
  return m_parameterManager->value(prop);
}
/**
 * Get a value of a attribute
 * @param attrName :: Attribute name
 */
IFunction::Attribute FunctionTreeView::getAttribute(std::string const &attrName) const {
  auto prop = getAttributeProperty(attrName);
  if (isDoubleAttribute(prop)) {
    return IFunction::Attribute(m_attributeDoubleManager->value(prop));
  } else if (isIntAttribute(prop)) {
    return IFunction::Attribute(m_attributeIntManager->value(prop));
  } else if (isStringAttribute(prop)) {
    return IFunction::Attribute(m_attributeStringManager->value(prop).toStdString());
  } else if (isBoolAttribute(prop)) {
    return IFunction::Attribute(m_attributeBoolManager->value(prop));
  } else if (isVectorAttribute(prop)) {
    return IFunction::Attribute(m_attributeVectorDoubleManager->value(prop));
  } else {
    throw std::runtime_error("Unknown function attribute " + attrName);
  }
}
/// Get a property for a parameter
QtProperty *FunctionTreeView::getParameterProperty(std::string const &parameterName) const {
  std::string index, name;
  std::tie(index, name) = splitParameterName(parameterName);
  if (auto const *prop = getFunctionProperty(index)) {
    const auto children = prop->subProperties();
    const auto it = std::find_if(children.cbegin(), children.cend(), [&](const auto &child) {
      return isParameter(child) && child->propertyName().toStdString() == name;
    });
    if (it != children.cend()) {
      return *it;
    }
  }
  std::string message = "Unknown function parameter " + parameterName +
                        "\n\n This may happen if there is a CompositeFunction "
                        "containing only one function.";
  throw std::runtime_error(message);
}
/// Get a property for an attribute
QtProperty *FunctionTreeView::getAttributeProperty(std::string const &attributeName) const {
  std::string index, name;
  const QtProperty *prop;
  std::tie(index, name) = splitParameterName(attributeName);
  if (!variableIsPrefixed(attributeName)) {
    // If variable is unprefixed then we are on the top level composite
    // function so grab first property from the tree
    prop = m_browser->properties()[0];
  } else {
    prop = getFunctionProperty(index);
  }
  const auto children = prop->subProperties();
  const auto it = std::find_if(children.cbegin(), children.cend(), [&](const auto &child) {
    return isAttribute(child) && child->propertyName().toStdString() == name;
  });
  if (it != children.cend()) {
    return *it;
  }
  std::string message = "Unknown function attribute " + attributeName;
  throw std::runtime_error(message);
}

/// Get a property for a parameter which is a parent of a given
/// property (tie or constraint).
QtProperty *FunctionTreeView::getParentParameterProperty(QtProperty *prop) const {
  const auto itProp =
      std::find_if(m_ties.cbegin(), m_ties.cend(), [&prop](const auto &tie) { return tie.tieProp == prop; });
  if (itProp != m_ties.cend())
    return (*itProp).paramProp;

  const auto itConstr = std::find_if(m_constraints.cbegin(), m_constraints.cend(), [&prop](const auto &constraint) {
    return (constraint.lower == prop || constraint.upper == prop);
  });
  if (itConstr != m_constraints.cend())
    return (*itConstr).paramProp;
  throw std::logic_error("QtProperty " + prop->propertyName().toStdString() +
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
  auto const functionString = getFunction(prop)->asString();
  auto const functionIndex = getIndex(prop);
  removeProperty(prop);
  updateFunctionIndices();

  // After removing a function we could end up with
  // a CompositeFunction with only one function
  // In this case, the function should be kept but
  // the composite function should be removed
  auto props = m_browser->properties();
  auto globalParameters = getGlobalParameters();
  if (!props.isEmpty()) {
    // The function browser is not empty

    // Check if the current function in the browser is a
    // composite function
    auto topProp = props[0];
    auto cf = std::dynamic_pointer_cast<Mantid::API::CompositeFunction>(getFunction(topProp));
    if (cf) {
      // If it is a composite function
      // check that there are more than one function
      // which means more than two subproperties

      if (cf->nFunctions() == 1 && cf->name() == "CompositeFunction") {
        // If only one function remains, remove the composite function:
        // Temporary copy the remaining function
        auto func = getFunction(props[0]->subProperties()[1]);
        std::transform(globalParameters.begin(), globalParameters.end(), globalParameters.begin(),
                       m_multiDomainFunctionPrefix.empty() ? removePrefix : removeEmbeddedPrefix);
        // Remove the composite function
        m_browser->removeProperty(topProp);
        // Add the temporary stored function
        setFunction(func);
      }
    }
  }
  emit functionRemovedString(functionString);
  emit functionRemoved(functionIndex);
  setGlobalParameters(globalParameters);
  emit globalsChanged(globalParameters);
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
  auto tie = QString::number(getParameter(prop)).toStdString();
  addTieProperty(prop, tie);
  emit parameterTieChanged(getParameterName(prop), tie);
}

/// Get a tie property attached to a parameter property
QtProperty *FunctionTreeView::getTieProperty(QtProperty *prop) const {
  const auto children = prop->subProperties();
  const auto it = std::find_if(children.cbegin(), children.cend(),
                               [](const auto &child) { return child->propertyName() == "Tie"; });
  if (it != children.cend()) {
    return *it;
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
  emit parameterTieChanged(getParameterName(prop), "");
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
  auto tie = QInputDialog::getText(this, "Add a tie", "Tie:", QLineEdit::Normal, "", &ok).toStdString();
  if (ok && !tie.empty()) {
    try {
      addTieProperty(prop, tie);
      emit parameterTieChanged(getParameterName(prop), tie);
    } catch (Mantid::API::Expression::ParsingError &) {
      QMessageBox::critical(this, "Mantid - Error", "Syntax errors found in tie: " + QString::fromStdString(tie));
    } catch (std::exception &e) {
      QMessageBox::critical(this, "Mantid - Error", QString::fromLatin1("Errors found in tie: ") + e.what());
    }
  }
}

/**
 * Copy function from the clipboard
 */
void FunctionTreeView::pasteFromClipboard() {
  auto funStr = QApplication::clipboard()->text().toStdString();
  if (funStr.empty())
    return;
  try {
    auto fun = Mantid::API::FunctionFactory::Instance().createInitialized(funStr);
    if (!fun)
      return;
    this->setFunction(fun);
    emit functionReplaced(funStr);
  } catch (...) {
    // text in the clipboard isn't a function definition
    QMessageBox::warning(this, "Mantid - Warning",
                         "Text in the clipboard isn't a function definition"
                         " or contains errors.");
  }
}

/**
 * Copy function to the clipboard
 */
void FunctionTreeView::copyToClipboard() { emit copyToClipboardRequest(); }

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
  std::string functionIndex, name;
  std::tie(functionIndex, name) = splitParameterName(getParameterName(prop));
  auto const value = QString::number(getParameter(prop)).toStdString();
  auto const constraint = value + "<" + name + "<" + value;
  addConstraintProperties(prop, constraint);
  emit parameterConstraintAdded(functionIndex, constraint);
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
  std::string functionIndex, name;
  std::tie(functionIndex, name) = splitParameterName(getParameterName(prop));
  auto const constraint =
      QString::number(val * 0.9).toStdString() + "<" + name + "<" + QString::number(val * 1.1).toStdString();
  addConstraintProperties(prop, constraint);
  emit parameterConstraintAdded(functionIndex, constraint);
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
  std::string functionIndex, name;
  std::tie(functionIndex, name) = splitParameterName(getParameterName(prop));
  auto const constraint =
      QString::number(val * 0.5).toStdString() + "<" + name + "<" + QString::number(val * 1.5).toStdString();
  addConstraintProperties(prop, constraint);
  emit parameterConstraintAdded(functionIndex, constraint);
}

void FunctionTreeView::removeConstraintsQuiet(QtProperty *prop) {
  auto props = prop->subProperties();
  foreach (QtProperty *p, props) {
    if (isConstraint(p)) {
      removeProperty(p);
    }
  }
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
  removeConstraintsQuiet(prop);
  emit parameterConstraintRemoved(getParameterName(prop));
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
  auto const parName = getParameterName(paramProp);
  emit parameterConstraintRemoved(parName);
  std::string functionIndex, constraint;
  std::tie(functionIndex, constraint) = getFunctionAndConstraint(paramProp);
  if (!constraint.empty()) {
    emit parameterConstraintAdded(functionIndex, constraint);
  }
}
/**
 * Get user selected function
 */
IFunction_sptr FunctionTreeView::getSelectedFunction() {
  auto item = m_browser->currentItem();
  if (!item)
    return IFunction_sptr();
  QtProperty *prop = item->property();
  if (!isFunction(prop))
    return IFunction_sptr();
  return getFunction(prop);
}
/**
 * Show function help page for input functionName
 */
void FunctionTreeView::showFunctionHelp(std::string const &functionName) const {
  API::InterfaceManager().showFitFunctionHelp(QString::fromStdString(functionName));
}

std::pair<std::string, std::string> FunctionTreeView::getFunctionAndConstraint(QtProperty *prop) const {
  auto const parName = getParameterName(prop);
  double lower = Mantid::EMPTY_DBL();
  double upper = Mantid::EMPTY_DBL();
  for (auto const *p : prop->subProperties()) {
    if (p->propertyName() == "LowerBound")
      lower = m_constraintManager->value(p);
    if (p->propertyName() == "UpperBound")
      upper = m_constraintManager->value(p);
  }
  if (lower != Mantid::EMPTY_DBL() || upper != Mantid::EMPTY_DBL()) {
    std::string functionIndex, name;
    std::tie(functionIndex, name) = splitParameterName(parName);
    return std::make_pair(functionIndex, getConstraint(name, lower, upper));
  }
  return std::make_pair("", "");
}

/**
 * Slot connected to all function attribute managers. Update the corresponding
 * function.
 * @param prop :: An attribute property that was changed
 */
void FunctionTreeView::attributeChanged(QtProperty *prop) {
  auto attributeName = getAttributeName(prop);
  // Some attributes require the function to be fully reconstructed, in this
  // case we'd need to emit a function replaced signal. If its not one of these
  // attributes emit an attributeValueChanged signal.
  try {
    if (std::find(REQUIRESRECONSTRUCTIONATTRIBUTES.begin(), REQUIRESRECONSTRUCTIONATTRIBUTES.end(),
                  removePrefix(attributeName)) != REQUIRESRECONSTRUCTIONATTRIBUTES.end()) {
      auto funProp = m_properties[prop].parent;
      if (!funProp)
        return;
      auto fun = getFunction(funProp, true);

      // delete and recreate all function's properties (attributes, parameters,
      // etc)
      setFunction(funProp, fun);
      updateFunctionIndices();
      if (m_emitAttributeValueChange) {
        emit functionReplaced(getFunction()->asString());
      }
    } else {
      if (m_emitAttributeValueChange) {
        emit attributePropertyChanged(attributeName);
      }
    }
  } catch (std::exception &expt) {
    QMessageBox::critical(this, "Mantid - Error", QString::fromStdString(expt.what()));
  }
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
    throw std::runtime_error("FunctionTreeView: inconsistency in vector properties.");
  attributeChanged(vectorProp);
}

void FunctionTreeView::parameterPropertyChanged(QtProperty *prop) {
  auto tieProp = getTieProperty(prop);
  if (tieProp && !tieProp->isEnabled()) {
    // it is a fixed tie
    QString newTie = QString("%1=%2").arg(prop->propertyName()).arg(m_parameterManager->value(prop));
    if (!newTie.isEmpty()) {
      m_tieManager->setValue(tieProp, newTie);
    }
  }
  if (m_emitParameterValueChange) {
    setErrorsEnabled(false);
    emit parameterChanged(getParameterName(prop));
  }
}

/// Called when a tie property changes
void FunctionTreeView::tieChanged(QtProperty *prop) {
  for (const auto &atie : m_ties) {
    if (atie.tieProp == prop) {
      emit parameterTieChanged(getParameterName(prop), getTie(prop).toStdString());
    }
  }
}

/// Called when a constraint property changes
void FunctionTreeView::constraintChanged(QtProperty *prop) {
  for (const auto &constr : m_constraints) {
    const bool isLower = constr.lower == prop;
    const bool isUpper = constr.upper == prop;
    if (isLower || isUpper) {
      auto paramProp = getParentParameterProperty(prop);
      std::string functionIndex, constraint;
      std::tie(functionIndex, constraint) = getFunctionAndConstraint(paramProp);
      if (!constraint.empty()) {
        emit parameterConstraintAdded(functionIndex, constraint);
        return; // No need to keep looping as found constraint changed
      }
    }
  }
}

void FunctionTreeView::parameterButtonClicked(QtProperty *prop) {
  emit localParameterButtonClicked(getIndex(prop) + prop->propertyName().toStdString());
}

bool FunctionTreeView::hasFunction() const { return !m_functionManager->properties().isEmpty(); }

/// Resize the browser's columns
/// @param s0 :: New size for the first column (Parameter).
/// @param s1 :: New size for the second column (Value).
/// @param s2 :: New size for the third optional column (Global).
void FunctionTreeView::setColumnSizes(int s0, int s1, int s2) { m_browser->setColumnSizes(s0, s1, s2); }

void FunctionTreeView::setStretchLastColumn(bool stretch) { m_browser->setStretchLastColumn(stretch); }

/// Show global column
void FunctionTreeView::hideGlobals() { m_browser->hideColumn(2); }
// Hide global column
void FunctionTreeView::showGlobals() { m_browser->showColumn(2); }

/**
 * The function index displayed as a multi-domain function index at the top of
 * the FunctionTreeView. It is used as an offset for the other function
 * prefixes. It is used when we don't want to display an entire
 * MultiDomainFunction, just a specific function within it.
 * @param functionPrefix :: The function prefix of the domain currently being
 * displayed in the FunctionTreeView.
 */
void FunctionTreeView::setMultiDomainFunctionPrefix(std::string const &functionPrefix) {
  m_multiDomainFunctionPrefix = functionPrefix;
}

/**
 * The global ties within a multi-domain function. It is used when we don't want
 * to display an entire MultiDomainFunction, and so we have to store the
 * GlobalTies manually in this vector.
 * @param globalTies :: A vector of global ties to be displayed within the
 * function tree.
 */
void FunctionTreeView::setGlobalTies(std::vector<GlobalTie> const &globalTies) { m_globalTies = globalTies; }

/**
 * Emit a signal when any of the Global options change.
 */
void FunctionTreeView::globalChanged(QtProperty *, const QString &, bool) {
  emit globalsChanged(getGlobalParameters());
}

/**
 * Set display of parameter errors on/off
 * @param enabled :: [input] On/off display of errors
 */
void FunctionTreeView::setErrorsEnabled(bool enabled) { m_parameterManager->setErrorsEnabled(enabled); }

/**
 * Clear all errors, if they are set
 */
void FunctionTreeView::clearErrors() { m_parameterManager->clearErrors(); }

std::optional<std::string> FunctionTreeView::currentFunctionIndex() const { return m_currentFunctionIndex; }

void FunctionTreeView::updateCurrentFunctionIndex() {
  std::optional<std::string> newIndex;

  if (auto item = m_browser->currentItem()) {
    auto prop = item->property();
    newIndex = getIndex(prop);
  }

  if (m_currentFunctionIndex != newIndex) {
    m_currentFunctionIndex = newIndex;
    emit currentFunctionChanged();
  }
}

void FunctionTreeView::setParameterTie(std::string const &parameterName, std::string const &tie) {
  auto paramProp = getParameterProperty(parameterName);
  auto tieProp = getTieProperty(paramProp);
  if (!tie.empty()) {
    if (tieProp) {
      m_tieManager->setValue(tieProp, QString::fromStdString(tie));
    } else {
      addTieProperty(paramProp, tie);
    }
  } else if (tieProp) {
    removeProperty(tieProp);
  }
}

void FunctionTreeView::setParameterConstraint(std::string const &parameterName, std::string const &constraint) {
  auto paramProp = getParameterProperty(parameterName);
  if (hasConstraint(paramProp)) {
    removeConstraintsQuiet(paramProp);
  }
  addConstraintProperties(paramProp, constraint);
}

std::vector<std::string> FunctionTreeView::getGlobalParameters() const {
  std::vector<std::string> globals;
  for (const auto &prop : m_properties) {
    if (prop.prop->checkOption(globalOptionName)) {
      auto const name = getParameterName(prop.prop);
      globals.emplace_back(name);
    }
  }
  return globals;
}

void FunctionTreeView::setGlobalParameters(const std::vector<std::string> &globals) {
  for (const auto &ap : m_properties) {
    auto prop = ap.prop;
    if (!prop->hasOption(globalOptionName))
      continue;

    auto const parameterName = getParameterName(prop);
    auto const isGlobal = std::any_of(globals.cbegin(), globals.cend(), [&](std::string const &global) {
      return m_multiDomainFunctionPrefix + global == parameterName;
    });
    prop->setOption(globalOptionName, isGlobal);
  }
}

QTreeWidgetItem *FunctionTreeView::getPropertyWidgetItem(QtProperty *prop) const {
  return m_browser->getItemWidget(m_properties.find(prop)->item);
}

QRect FunctionTreeView::visualItemRect(QtProperty *prop) const {
  if (!prop)
    return QRect();
  auto item = getPropertyWidgetItem(prop);
  return item->treeWidget()->visualItemRect(item);
}

QRect FunctionTreeView::getVisualRectFunctionProperty(std::string const &index) const {
  QRect rect;
  try {
    rect = visualItemRect(getFunctionProperty(index));
  } catch (std::exception &) {
  }
  return rect;
}

QRect FunctionTreeView::getVisualRectParameterProperty(std::string const &index) const {
  QRect rect;
  try {
    rect = visualItemRect(getParameterProperty(index));
  } catch (std::exception &) {
  }
  return rect;
}

QWidget *FunctionTreeView::getParamWidget(std::string const &parameterName) const {
  try {
    auto item = getPropertyWidgetItem(getParameterProperty(parameterName));
    return item->treeWidget()->itemWidget(item, 1);
  } catch (std::exception &) {
  }
  return nullptr;
}

QTreeWidget *FunctionTreeView::treeWidget() const { return m_browser->treeWidget(); }

QtTreePropertyBrowser *FunctionTreeView::treeBrowser() { return m_browser; }

DoubleDialogEditorFactory *FunctionTreeView::doubleEditorFactory() { return m_doubleEditorFactory; }

} // namespace MantidQt::MantidWidgets
