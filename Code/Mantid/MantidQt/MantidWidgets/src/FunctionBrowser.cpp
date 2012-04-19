#include "MantidQtMantidWidgets/FunctionBrowser.h"
#include "MantidQtMantidWidgets/SelectFunctionDialog.h"
//#include "MantidQtMantidWidgets/MultifitSetupDialog.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/ConstraintFactory.h"
#include "MantidAPI/Expression.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/LibraryManager.h"

#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/ICostFunction.h"

#include "MantidQtMantidWidgets/UserFunctionDialog.h"

#include "qttreepropertybrowser.h"
#include "qtpropertymanager.h"
// Suppress a warning coming out of code that isn't ours
#if defined(__INTEL_COMPILER)
  #pragma warning disable 1125
#elif defined(__GNUC__)
  #if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6 )
    #pragma GCC diagnostic push
  #endif
  #pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#include "qteditorfactory.h"
#include "StringDialogEditorFactory.h"
#include "DoubleEditorFactory.h"
#if defined(__INTEL_COMPILER)
  #pragma warning enable 1125
#elif defined(__GNUC__)
  #if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6 )
    #pragma GCC diagnostic pop
  #endif
#endif

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QMenu>
#include <QMessageBox>
#include <QInputDialog>
#include <QSettings>
#include <QFileInfo>
#include <QApplication>
#include <QClipboard>
#include <QSignalMapper>
#include <QMetaMethod>
#include <QTreeWidget>

#include <algorithm>


namespace MantidQt
{
namespace MantidWidgets
{

namespace
{

class FormulaDialogEditor: public StringDialogEditor
{
public:
  FormulaDialogEditor(QtProperty *property, QWidget *parent)
    :StringDialogEditor(property,parent){}
protected slots:
  void runDialog()
  {
    MantidQt::MantidWidgets::UserFunctionDialog *dlg = new MantidQt::MantidWidgets::UserFunctionDialog((QWidget*)parent(),getText());
    if (dlg->exec() == QDialog::Accepted)
    {
      setText(dlg->getFormula());
      updateProperty();
    };
  }
};

class FormulaDialogEditorFactory: public StringDialogEditorFactory
{
public:
  FormulaDialogEditorFactory(QObject* parent):StringDialogEditorFactory(parent){}
protected:
  using QtAbstractEditorFactoryBase::createEditor; // Avoid Intel compiler warning
  QWidget *createEditor(QtStringPropertyManager *manager, QtProperty *property,QWidget *parent)
  {
    (void) manager; //Avoid unused warning
    return new FormulaDialogEditor(property,parent);
  }
};

}

/**
 * Constructor
 * @param parent :: The parent widget.
 */
FunctionBrowser::FunctionBrowser(QWidget *parent)
:QWidget(parent)
{
  // create m_browser
  createBrowser();
  createActions();

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->addWidget(m_browser);
  layout->setContentsMargins(0,0,0,0);

}

/**
 * Destructor
 */
FunctionBrowser::~FunctionBrowser()
{
}


/**
 * Create the Qt property browser and set up property managers.
 */
void FunctionBrowser::createBrowser()
{
  /* Create property managers: they create, own properties, get and set values  */
  m_functionManager = new QtGroupPropertyManager(this);
  m_parameterManager = new QtDoublePropertyManager(this);
  m_attributeStringManager = new QtStringPropertyManager(this);
  m_attributeDoubleManager = new QtDoublePropertyManager(this);
  m_attributeIntManager = new QtIntPropertyManager(this);
  m_indexManager = new QtStringPropertyManager(this);
  //m_enumManager = new QtEnumPropertyManager(this);
  //m_intManager = new QtIntPropertyManager(this);
  //m_boolManager = new QtBoolPropertyManager(this);
  //m_filenameManager = new QtStringPropertyManager(this);
  //m_formulaManager = new QtStringPropertyManager(this);

  //QtCheckBoxFactory *checkBoxFactory = new QtCheckBoxFactory(this);
  //QtEnumEditorFactory *comboBoxFactory = new QtEnumEditorFactory(this);
  QtSpinBoxFactory *spinBoxFactory = new QtSpinBoxFactory(this);
  DoubleEditorFactory *doubleEditorFactory = new DoubleEditorFactory(this);
  QtLineEditFactory *lineEditFactory = new QtLineEditFactory(this);
  //StringDialogEditorFactory* stringDialogEditFactory = new StringDialogEditorFactory(this);
  //FormulaDialogEditorFactory* formulaDialogEditFactory = new FormulaDialogEditorFactory(this);

  m_browser = new QtTreePropertyBrowser();
  m_browser->setFactoryForManager(m_parameterManager, doubleEditorFactory);
  m_browser->setFactoryForManager(m_attributeStringManager, lineEditFactory);
  m_browser->setFactoryForManager(m_attributeDoubleManager, doubleEditorFactory);
  m_browser->setFactoryForManager(m_attributeIntManager, spinBoxFactory);
  m_browser->setFactoryForManager(m_indexManager, lineEditFactory);

  //m_browser->setFactoryForManager(m_enumManager, comboBoxFactory);
  //m_browser->setFactoryForManager(m_boolManager, checkBoxFactory);
  //m_browser->setFactoryForManager(m_filenameManager, stringDialogEditFactory);
  //m_browser->setFactoryForManager(m_formulaManager, formulaDialogEditFactory);

  m_browser->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_browser, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(popupMenu(const QPoint &)));
  connect(m_browser, SIGNAL(currentItemChanged(QtBrowserItem*)), this, SLOT(currentItemChanged(QtBrowserItem*)));

}

/**
 * Create and connect actions
 */
void FunctionBrowser::createActions()
{
  m_actionAddFunction = new QAction("Add function",this);
  connect(m_actionAddFunction,SIGNAL(triggered()),this,SLOT(addFunction()));

  m_actionRemoveFunction = new QAction("Remove function",this);
  connect(m_actionRemoveFunction,SIGNAL(triggered()),this,SLOT(removeFunction()));
}

/**
 * Clear the contents
 */
void FunctionBrowser::clear()
{
  m_browser->clear();
  m_properties.clear();
}

/**
 * Set the function in the browser
 * @param funStr :: FunctionFactory function creation string
 */
void FunctionBrowser::setFunction(QString funStr)
{
  clear();
  addFunction(NULL,funStr);
}

/**
 * Add a sub-property to a parent property
 * @param parent :: The parent property
 * @param subproperty :: New sub-property
 */
FunctionBrowser::AProperty FunctionBrowser::addProperty(QtProperty* parent, QtProperty* subproperty)
{
  AProperty ap;
  ap.prop = subproperty;
  if (parent == NULL)
  {
    ap.item = m_browser->addProperty(subproperty);
  }
  else
  {
    parent->addSubProperty(subproperty);
    auto items = m_browser->items(subproperty);
    if (items.isEmpty())
    {
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
void FunctionBrowser::removeProperty(QtProperty *prop)
{
  auto p = m_properties.find(prop);
  if (p == m_properties.end()) return;
  AProperty ap = *p;

  // remove references to the children
  auto children = prop->subProperties();
  foreach(QtProperty* child,children)
  {
    m_properties.erase(child);
  }
  m_properties.erase(p);

  // remove property from Qt browser
  if (ap.parent)
  {
    ap.parent->removeSubProperty(prop);
  }
  else
  {
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
FunctionBrowser::AProperty FunctionBrowser::addFunctionProperty(QtProperty* parent, QString funName)
{
  // check that parent is a function property
  if (parent && dynamic_cast<QtAbstractPropertyManager*>(m_functionManager) != parent->propertyManager())
  {
    throw std::runtime_error("Unexpected error in FunctionBrowser [2]");
  }
  QtProperty* prop = m_functionManager->addProperty(funName);
  return addProperty(parent, prop);
}

/**
 * Add a parameter property
 * @param parent :: Parent function property
 * @param paramName :: Parameter name
 * @param paramValue :: Parameter value
 */
FunctionBrowser::AProperty FunctionBrowser::addParameterProperty(QtProperty* parent, QString paramName, double paramValue)
{
  // check that parent is a function property
  if (!parent || dynamic_cast<QtAbstractPropertyManager*>(m_functionManager) != parent->propertyManager())
  {
    throw std::runtime_error("Unexpected error in FunctionBrowser [3]");
  }
  QtProperty* prop = m_parameterManager->addProperty(paramName);
  m_parameterManager->setValue(prop,paramValue);
  return addProperty(parent,prop);
}

/**
 * Add a function.
 * @param prop :: Property of the parent composite function or NULL
 * @param funStr :: FunctionFactory function creation string
 */
void FunctionBrowser::addFunction(QtProperty* prop, QString funStr)
{
  Mantid::API::IFunction_sptr fun = Mantid::API::FunctionFactory::Instance().createInitialized(funStr.toStdString());
  AProperty funProp = addFunctionProperty(prop, QString::fromStdString(fun->name()));

  addAttributeAndParameterProperties(funProp.prop, fun);

}

namespace
{
/**
 * Attribute visitor to create a QtProperty. Depending on the attribute type
 * the appropriate apply() method is used.
 */
class CreateAttributeProperty: public Mantid::API::IFunction::ConstAttributeVisitor<FunctionBrowser::AProperty>
{
public:
  CreateAttributeProperty(FunctionBrowser* browser, QtProperty* parent,QString attName)
    :m_browser(browser),m_parent(parent),m_attName(attName)
  {
    // check that parent is a function property
    if (!m_parent || dynamic_cast<QtAbstractPropertyManager*>(m_browser->m_functionManager) != m_parent->propertyManager())
    {
      throw std::runtime_error("Unexpected error in FunctionBrowser [4]");
    }
  }
protected:
  /// Create string property
  FunctionBrowser::AProperty apply(const std::string& str)const
  {
    QtProperty* prop = m_browser->m_attributeStringManager->addProperty(m_attName);
    m_browser->m_attributeStringManager->setValue(prop, QString::fromStdString(str));
    return m_browser->addProperty(m_parent,prop);
  }
  /// Create double property
  FunctionBrowser::AProperty apply(const double& d)const
  {
    QtProperty* prop = m_browser->m_attributeDoubleManager->addProperty(m_attName);
    m_browser->m_attributeDoubleManager->setValue(prop, d);
    return m_browser->addProperty(m_parent,prop);
  }
  /// Create int property
  FunctionBrowser::AProperty apply(const int& i)const
  {
    QtProperty* prop = m_browser->m_attributeIntManager->addProperty(m_attName);
    m_browser->m_attributeIntManager->setValue(prop, i);
    return m_browser->addProperty(m_parent,prop);
  }
private:
  FunctionBrowser* m_browser;
  QtProperty* m_parent;
  QString m_attName;
};

}

/**
 * Add a attribute property
 * @param parent :: Parent function property
 * @param attName :: Attribute name
 * @param att :: Attribute value
 */
FunctionBrowser::AProperty FunctionBrowser::addAttributeProperty(QtProperty* parent, QString attName, const Mantid::API::IFunction::Attribute& att)
{
  CreateAttributeProperty cap(this,parent,attName);
  return att.apply(cap);
}

/**
 * Add attribute and parameter properties to a function property
 * @param prop :: A function property
 * @param fun :: Shared pointer to a created function
 */
void FunctionBrowser::addAttributeAndParameterProperties(QtProperty* prop, Mantid::API::IFunction_sptr fun)
{
  addIndexProperty(prop);
  auto attributeNames = fun->getAttributeNames();
  for(auto att = attributeNames.begin(); att != attributeNames.end(); ++att)
  {
    QString attName = QString::fromStdString(*att);
    AProperty ap = addAttributeProperty(prop, attName, fun->getAttribute(*att));
  }

  auto cf = boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(fun);
  if (cf)
  {
    for(size_t i = 0; i < cf->nFunctions(); ++i)
    {
      addFunction(prop,QString::fromStdString(cf->getFunction(i)->asString()));
    }
  }
  else
  {
    for(size_t i = 0; i < fun->nParams(); ++i)
    {
      QString name = QString::fromStdString(fun->parameterName(i));
      double value = fun->getParameter(i);
      addParameterProperty(prop, name, value);
    }
  }
}

/**
 * Add property showing function's index in the composite function
 * @param prop :: A function property
 * @return :: AProperty struct for added property. If all fields are NULL - property wasn't added
 *  because it is the top function
 */
FunctionBrowser::AProperty FunctionBrowser::addIndexProperty(QtProperty* prop)
{
  AProperty ap;
  ap.item = NULL;
  ap.parent = NULL;
  ap.prop = NULL;
  if (!prop) return ap;
  if (!isFunction(prop)) return ap;
  if ( !m_properties[prop].parent ) return ap;

  QString index = "fff";
  QtProperty* ip = m_indexManager->addProperty("Index");
  ip->setEnabled(false);
  m_indexManager->setValue(ip,index);
  return addProperty(prop,ip);

}

/**
 * Update function index properties 
 * @param prop :: A function property
 * @param index :: The parent function's index
 */
void FunctionBrowser::updateFunctionIndices(QtProperty* prop, QString index)
{
  if ( prop == NULL )
  {
    auto top = m_browser->properties();
    if (top.isEmpty()) return;
    prop = top[0];
  }
  auto children = prop->subProperties();
  size_t i = 0;
  foreach(QtProperty* child, children)
  {
    if (isFunction(child))
    {
      updateFunctionIndices(child,index + "f" + QString::number(i) + ".");
      ++i;
    }
    else if (isIndex(child))
    {
      m_indexManager->setValue(child,index);
    }
  }
}

/**
 * Check if property is a function group
 * @param prop :: Property to check
 */
bool FunctionBrowser::isFunction(QtProperty* prop) const
{
  return prop && dynamic_cast<QtAbstractPropertyManager*>(m_functionManager) == prop->propertyManager();
}

/**
 * Check if property is a function attribute
 * @param prop :: Property to check
 */
bool FunctionBrowser::isStringAttribute(QtProperty* prop) const
{
  return prop && dynamic_cast<QtAbstractPropertyManager*>(m_attributeStringManager) == prop->propertyManager();
}

/**
 * Check if property is a function attribute
 * @param prop :: Property to check
 */
bool FunctionBrowser::isDoubleAttribute(QtProperty* prop) const
{
  return prop && dynamic_cast<QtAbstractPropertyManager*>(m_attributeDoubleManager) == prop->propertyManager();
}

/**
 * Check if property is a function attribute
 * @param prop :: Property to check
 */
bool FunctionBrowser::isIntAttribute(QtProperty* prop) const
{
  return prop && dynamic_cast<QtAbstractPropertyManager*>(m_attributeIntManager) == prop->propertyManager();
}

/**
 * Check if property is a function attribute
 * @param prop :: Property to check
 */
bool FunctionBrowser::isAttribute(QtProperty* prop) const
{
  return isStringAttribute(prop) || isDoubleAttribute(prop) || isIntAttribute(prop);
}

/**
 * Get attribute as a string
 * @param prop :: An attribute property
 */
QString FunctionBrowser::getAttribute(QtProperty* prop) const
{
  if (isStringAttribute(prop))
  {
    return m_attributeStringManager->value(prop);
  }
  else if (isDoubleAttribute(prop))
  {
    return QString::number(m_attributeDoubleManager->value(prop));
  }
  else if (isIntAttribute(prop))
  {
    return QString::number(m_attributeIntManager->value(prop));
  }
  return "";
}

/**
 * Check if property is a function parameter
 * @param prop :: Property to check
 */
bool FunctionBrowser::isParameter(QtProperty* prop) const
{
  return prop && dynamic_cast<QtAbstractPropertyManager*>(m_parameterManager) == prop->propertyManager();
}

/**
 * Get parameter value as a string
 * @param prop :: A parameter property
 */
QString FunctionBrowser::getParameter(QtProperty* prop) const
{
  return QString::number(m_parameterManager->value(prop));
}

/**
 * Check if a property is an index
 * @param prop :: A property
 */
bool FunctionBrowser::isIndex(QtProperty* prop) const
{
  return prop && dynamic_cast<QtAbstractPropertyManager*>(m_indexManager) == prop->propertyManager();
}

/**
 * Show a pop up menu.
 */
void FunctionBrowser::popupMenu(const QPoint &)
{
  auto item = m_browser->currentItem();
  if (!item)
  {
    QMenu context(this);
    context.addAction(m_actionAddFunction);
    context.exec(QCursor::pos());
    return;
  }
  QtProperty* prop = item->property();
  if (isFunction(prop))
  {
    QMenu context(this);
    Mantid::API::IFunction_sptr fun = Mantid::API::FunctionFactory::Instance().createFunction(prop->propertyName().toStdString());
    auto cf = boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(fun);
    if (cf || m_properties[prop].parent == NULL)
    {
      context.addAction(m_actionAddFunction);
    }
    context.addAction(m_actionRemoveFunction);
    context.exec(QCursor::pos());
  }
}

/** 
 * Add a function to currently selected composite function property
 */
void FunctionBrowser::addFunction()
{
  QString newFunction;

  auto item = m_browser->currentItem();
  QtProperty* prop = NULL;
  if ( item )
  {
    prop = item->property();
    if (!isFunction(prop)) return;
  }

  // check if the browser is empty
  if ( !prop )
  {
    auto top = m_browser->properties();
    if ( !top.isEmpty() )
    {
      prop = top[0];
      if (!isFunction(prop)) return;
    }
  }

  // Get new function type 
  SelectFunctionDialog dlg(this);
  if (dlg.exec() == QDialog::Accepted)
  {
    newFunction = dlg.getFunction();
  }
  else
  {
    return;
  }

  if (newFunction.isEmpty()) return;

  std::cerr << "New fun: " << newFunction.toStdString() << std::endl;
  auto f = Mantid::API::FunctionFactory::Instance().createFunction(newFunction.toStdString());

  if (prop)
  {// there are other function defined
    Mantid::API::IFunction_sptr fun = Mantid::API::FunctionFactory::Instance().createFunction(prop->propertyName().toStdString());
    auto cf = boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(fun);
    if (cf)
    {
      addFunction(prop,QString::fromStdString(f->asString()));
    }
    else
    {
      setFunction(getFunctionString(prop) + ";" + QString::fromStdString(f->asString()));
    }
  }
  else
  {// the browser is empty - add first function
    addFunction(prop,QString::fromStdString(f->asString()));
  }
  updateFunctionIndices();
}

/**
 * Return FunctionFactory function string
 * @param prop :: Function property 
 */
QString FunctionBrowser::getFunctionString(QtProperty* prop) const
{
  if (prop == NULL)
  {
    auto props = m_browser->properties();
    if (props.isEmpty()) return "";
    prop = props[0];
  }
  if (!isFunction(prop)) return "";
  QString out;
  auto fun = Mantid::API::FunctionFactory::Instance().createFunction(prop->propertyName().toStdString());
  auto cf = boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(fun);
  if (cf)
  {
    if (cf->name() != "CompositeFunction")
    {
      out += "composite=" + QString::fromStdString(cf->name()) + ";";
    }
    auto children = prop->subProperties();
    foreach(QtProperty* child, children)
    {
      if (isFunction(child))
      {
        auto f = Mantid::API::FunctionFactory::Instance().createFunction(child->propertyName().toStdString());
        bool isComposite = dynamic_cast<Mantid::API::CompositeFunction*>(f.get());
        if (isComposite) out += "(";
        out += getFunctionString(child);
        if (isComposite) out += ")";
        out += ";";
      }
    }
    if (out.endsWith(";"))
    {
      out.remove(out.length() - 1, 1);
    }
  }
  else
  {
    out += "name=" + QString::fromStdString(fun->name()) + ",";
    auto children = prop->subProperties();
    foreach(QtProperty* child, children)
    {
      if (isAttribute(child))
      {
        out += child->propertyName() + "=" + getAttribute(child) + ",";
      }
      else if (isParameter(child))
      {
        out += child->propertyName() + "=" + getParameter(child) + ",";
      }
    }
    if (out.endsWith(","))
    {
      out.remove(out.length() - 1, 1);
    }
  }
  return out;
}

/**
 * Remove the function under currently selected property
 */
void FunctionBrowser::removeFunction()
{
  auto item = m_browser->currentItem();
  if ( !item ) return;
  QtProperty* prop = item->property();
  if (!isFunction(prop)) return;
  removeProperty(prop);
  updateFunctionIndices();
}

} // MantidWidgets
} // MantidQt
