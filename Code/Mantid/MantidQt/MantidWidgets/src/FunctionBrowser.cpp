#include "MantidQtMantidWidgets/FunctionBrowser.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/ParameterTie.h"

#include "MantidKernel/Logger.h"

#include "MantidQtMantidWidgets/FilenameDialogEditor.h"
#include "MantidQtMantidWidgets/FormulaDialogEditor.h"
#include "MantidQtMantidWidgets/SelectFunctionDialog.h"
#include "MantidQtMantidWidgets/UserFunctionDialog.h"
#include "MantidQtMantidWidgets/WorkspaceEditorFactory.h"

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
#include "DoubleEditorFactory.h"
#include "CompositeEditorFactory.h"
#include "ButtonEditorFactory.h"
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

namespace{
  const char * globalOptionName = "Global";
  Mantid::Kernel::Logger g_log("Function Browser");
}

namespace MantidQt
{
namespace MantidWidgets
{

/**
 * Constructor
 * @param parent :: The parent widget.
 * @param multi  :: Option to use the browser for multi-dataset fitting.
 */
FunctionBrowser::FunctionBrowser(QWidget *parent, bool multi)
  :QWidget(parent),m_multiDataset(multi),
  m_numberOfDatasets(0),
  m_currentDataset(0)

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
  QStringList options;
  if ( m_multiDataset )
  {
    options << globalOptionName;
  }

  /* Create property managers: they create, own properties, get and set values  */
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

  QtAbstractEditorFactory<ParameterPropertyManager> *parameterEditorFactory(NULL);
  if ( m_multiDataset )
  {
    auto buttonFactory = new DoubleButtonEditorFactory(this);
    auto compositeFactory = new CompositeEditorFactory<ParameterPropertyManager>(this,buttonFactory);
    compositeFactory->setSecondaryFactory(globalOptionName, paramEditorFactory);
    parameterEditorFactory = compositeFactory;
    connect(buttonFactory,SIGNAL(buttonClicked(QtProperty*)), this,SLOT(parameterButtonClicked(QtProperty*)));
  }
  else
  {
    parameterEditorFactory = paramEditorFactory;
  }
  
  QtLineEditFactory *lineEditFactory = new QtLineEditFactory(this);
  QtCheckBoxFactory *checkBoxFactory = new QtCheckBoxFactory(this);
  FilenameDialogEditorFactory* filenameDialogEditorFactory = new FilenameDialogEditorFactory(this);
  FormulaDialogEditorFactory* formulaDialogEditFactory = new FormulaDialogEditorFactory(this);
  WorkspaceEditorFactory* workspaceEditorFactory = new WorkspaceEditorFactory(this);

  m_browser = new QtTreePropertyBrowser(NULL,options);
  // assign factories to property managers
  m_browser->setFactoryForManager(m_parameterManager, parameterEditorFactory);
  m_browser->setFactoryForManager(m_attributeStringManager, lineEditFactory);
  m_browser->setFactoryForManager(m_attributeDoubleManager, doubleEditorFactory);
  m_browser->setFactoryForManager(m_attributeIntManager, spinBoxFactory);
  m_browser->setFactoryForManager(m_attributeBoolManager, checkBoxFactory);
  m_browser->setFactoryForManager(m_indexManager, lineEditFactory);
  m_browser->setFactoryForManager(m_tieManager, lineEditFactory);
  m_browser->setFactoryForManager(m_constraintManager, lineEditFactory);
  m_browser->setFactoryForManager(m_filenameManager, filenameDialogEditorFactory);
  m_browser->setFactoryForManager(m_formulaManager, formulaDialogEditFactory);
  m_browser->setFactoryForManager(m_workspaceManager, workspaceEditorFactory);
  m_browser->setFactoryForManager(m_attributeSizeManager, spinBoxFactory);
  m_browser->setFactoryForManager(m_attributeVectorDoubleManager, doubleEditorFactory);

  m_browser->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_browser, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(popupMenu(const QPoint &)));

  connect(m_attributeStringManager,SIGNAL(propertyChanged(QtProperty*)),this,SLOT(attributeChanged(QtProperty*)));
  connect(m_attributeDoubleManager,SIGNAL(propertyChanged(QtProperty*)),this,SLOT(attributeChanged(QtProperty*)));
  connect(m_attributeIntManager,SIGNAL(propertyChanged(QtProperty*)),this,SLOT(attributeChanged(QtProperty*)));
  connect(m_attributeBoolManager,SIGNAL(propertyChanged(QtProperty*)),this,SLOT(attributeChanged(QtProperty*)));
  connect(m_formulaManager,SIGNAL(propertyChanged(QtProperty*)),this,SLOT(attributeChanged(QtProperty*)));
  connect(m_filenameManager,SIGNAL(propertyChanged(QtProperty*)),this,SLOT(attributeChanged(QtProperty*)));
  connect(m_attributeVectorDoubleManager,SIGNAL(propertyChanged(QtProperty*)),this,SLOT(attributeVectorDoubleChanged(QtProperty*)));

  connect(m_parameterManager, SIGNAL(valueChanged(QtProperty*,double)),
          SLOT(parameterChanged(QtProperty*)));

  connect(m_browser, SIGNAL(currentItemChanged(QtBrowserItem*)), SLOT(updateCurrentFunctionIndex()));

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

  m_actionFixParameter = new QAction("Fix",this);
  connect(m_actionFixParameter,SIGNAL(triggered()),this,SLOT(fixParameter()));

  m_actionRemoveTie = new QAction("Remove tie",this);
  connect(m_actionRemoveTie,SIGNAL(triggered()),this,SLOT(removeTie()));

  m_actionAddTie = new QAction("Add tie",this);
  connect(m_actionAddTie,SIGNAL(triggered()),this,SLOT(addTie()));

  m_actionFromClipboard = new QAction("Copy from clipboard",this);
  connect(m_actionFromClipboard,SIGNAL(triggered()),this,SLOT(copyFromClipboard()));

  m_actionToClipboard = new QAction("Copy to clipboard",this);
  connect(m_actionToClipboard,SIGNAL(triggered()),this,SLOT(copyToClipboard()));

  m_actionConstraints = new QAction("Custom",this);
  connect(m_actionConstraints,SIGNAL(triggered()),this,SLOT(addConstraints()));

  m_actionConstraints10 = new QAction("10%",this);
  connect(m_actionConstraints10,SIGNAL(triggered()),this,SLOT(addConstraints10()));

  m_actionConstraints50 = new QAction("50%",this);
  connect(m_actionConstraints50,SIGNAL(triggered()),this,SLOT(addConstraints50()));

  m_actionRemoveConstraints = new QAction("Remove constraints",this);
  connect(m_actionRemoveConstraints,SIGNAL(triggered()),this,SLOT(removeConstraints()));

  m_actionRemoveConstraint = new QAction("Remove",this);
  connect(m_actionRemoveConstraint,SIGNAL(triggered()),this,SLOT(removeConstraint()));

  m_parameterManager->setErrorsEnabled(true);
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
void FunctionBrowser::setFunction(const QString& funStr)
{
  if ( funStr.isEmpty() ) return;
  try
  {
    auto fun = Mantid::API::FunctionFactory::Instance().createInitialized( funStr.toStdString() );
    if ( !fun ) return;
    this->setFunction(fun);
  }
  catch(...)
  {
    // error in the input string
  }
}

/**
 * Set the function in the browser
 * @param fun :: A function
 */
void FunctionBrowser::setFunction(Mantid::API::IFunction_sptr fun)
{
  clear();
  addFunction(NULL,fun);
  emit functionStructureChanged();
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

  if ( isFunction(prop) )
  {
    m_ties.erase(prop);
  }

  if ( isTie(prop) )
  {// 
    for(auto it = m_ties.begin(); it != m_ties.end(); ++it)
    {
      if (it.value().tieProp == prop)
      {
        m_ties.erase(it);
        break;
      }
    }
  }

  if ( isConstraint(prop) )
  {
    for(auto it = m_constraints.begin(); it != m_constraints.end(); ++it)
    {
      auto& cp = it.value();
      if ( cp.lower == prop )
      {
        if ( !cp.upper )
        {
          m_constraints.erase(it);
        }
        else
        {
          cp.lower = NULL;
        }
        break;
      }
      else if ( cp.upper == prop )
      {
        if ( !cp.lower )
        {
          m_constraints.erase(it);
        }
        else
        {
          cp.upper = NULL;
        }
        break;
      }
    }
  }

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
 * @param paramDesc :: Parameter description
 * @param paramValue :: Parameter value
 */
FunctionBrowser::AProperty FunctionBrowser::addParameterProperty(QtProperty* parent, QString paramName, QString paramDesc, double paramValue)
{
  // check that parent is a function property
  if (!parent || dynamic_cast<QtAbstractPropertyManager*>(m_functionManager) != parent->propertyManager())
  {
    throw std::runtime_error("Unexpected error in FunctionBrowser [3]");
  }
  QtProperty* prop = m_parameterManager->addProperty(paramName);
  m_parameterManager->setDecimals(prop,6);
  m_parameterManager->setValue(prop,paramValue);
  m_parameterManager->setDescription(prop,paramDesc.toStdString());

  if ( m_multiDataset )
  {
    prop->setOption(globalOptionName,false);
  }
  return addProperty(parent,prop);
}

/**
 * Set a function.
 * @param prop :: Property of the function or NULL
 * @param fun :: A function
 */
void FunctionBrowser::setFunction(QtProperty* prop, Mantid::API::IFunction_sptr fun)
{
  auto children = prop->subProperties();
  foreach(QtProperty* child, children)
  {
    removeProperty(child);
  }
  addAttributeAndParameterProperties(prop, fun);
}

/**
 * Add a function.
 * @param prop :: Property of the parent composite function or NULL
 * @param fun :: FunctionFactory function creation string
 */
void FunctionBrowser::addFunction(QtProperty* prop, Mantid::API::IFunction_sptr fun)
{
  if ( !prop )
  {
    AProperty ap = addFunctionProperty(NULL,QString::fromStdString(fun->name()));
    setFunction(ap.prop, fun);
  }
  else
  {
    Mantid::API::IFunction_sptr parentFun =  getFunction(prop);
    if ( !parentFun ) return;
    auto cf = boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(parentFun);
    if ( !cf )
    {
      throw std::runtime_error("FunctionBrowser: CompositeFunction is expected for addFunction");
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
class CreateAttributePropertyForFunctionBrowser: public Mantid::API::IFunction::ConstAttributeVisitor<FunctionBrowser::AProperty>
{
public:
  CreateAttributePropertyForFunctionBrowser(FunctionBrowser* browser, QtProperty* parent,QString attName)
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
    QtProperty* prop = NULL;
    if (m_attName == "FileName")
    {
      prop = m_browser->m_filenameManager->addProperty(m_attName);
      m_browser->m_filenameManager->setValue(prop, QString::fromStdString(str));
    }
    else if ( m_attName == "Formula" )
    {
      prop = m_browser->m_formulaManager->addProperty(m_attName);
      m_browser->m_formulaManager->setValue(prop, QString::fromStdString(str));
    }
    else if ( m_attName == "Workspace" )
    {
      prop = m_browser->m_workspaceManager->addProperty(m_attName);
      m_browser->m_workspaceManager->setValue(prop, QString::fromStdString(str));
    }
    else
    {
      prop = m_browser->m_attributeStringManager->addProperty(m_attName);
      m_browser->m_attributeStringManager->setValue(prop, QString::fromStdString(str));
    }
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
  /// Create bool property
  FunctionBrowser::AProperty apply(const bool& b)const
  {
    QtProperty* prop = m_browser->m_attributeBoolManager->addProperty(m_attName);
    m_browser->m_attributeBoolManager->setValue(prop, b);
    return m_browser->addProperty(m_parent,prop);
  }
  /// Create vector property
  FunctionBrowser::AProperty apply(const std::vector<double>& v)const
  {
    QtProperty* prop = m_browser->m_attributeVectorManager->addProperty(m_attName);
    FunctionBrowser::AProperty aprop = m_browser->addProperty(m_parent,prop);

    QtProperty* sizeProp = m_browser->m_attributeSizeManager->addProperty("Size");
    m_browser->m_attributeSizeManager->setValue( sizeProp, static_cast<int>(v.size()) );
    m_browser->addProperty( prop, sizeProp );
    sizeProp->setEnabled(false);

    m_browser->m_attributeVectorDoubleManager->blockSignals(true);
    QString parName = "value[%1]";
    for(size_t i = 0; i < v.size(); ++i)
    {
        QtProperty *dprop = m_browser->m_attributeVectorDoubleManager->addProperty( parName.arg(i) );
        m_browser->m_attributeVectorDoubleManager->setValue( dprop, v[i] );
        m_browser->addProperty( prop, dprop );
    }
    m_browser->m_attributeVectorDoubleManager->blockSignals(false);

    m_browser->m_browser->setExpanded( aprop.item, false );
    return aprop;
  }
private:
  FunctionBrowser* m_browser;
  QtProperty* m_parent;
  QString m_attName;
};

/**
 * Attribute visitor to set an attribute from a QtProperty. Depending on the attribute type
 * the appropriate apply() method is used.
 */
class SetAttributeFromProperty: public Mantid::API::IFunction::AttributeVisitor<>
{
public:
  SetAttributeFromProperty(FunctionBrowser* browser, QtProperty* prop)
    :m_browser(browser),m_prop(prop)
  {
  }
protected:
  /// Set string attribute
  void apply(std::string& str)const
  {
    QString attName = m_prop->propertyName();
    if ( attName == "FileName" )
    {
      str = m_browser->m_filenameManager->value(m_prop).toStdString();
    }
    else if ( attName == "Formula" )
    {
      str = m_browser->m_formulaManager->value(m_prop).toStdString();
    }
    else if ( attName == "Workspace" )
    {
      str = m_browser->m_workspaceManager->value(m_prop).toStdString();
    }
    else
    {
      str = m_browser->m_attributeStringManager->value(m_prop).toStdString();
    }
  }
  /// Set double attribute
  void apply(double& d)const
  {
    d = m_browser->m_attributeDoubleManager->value(m_prop);
  }
  /// Set int attribute
  void apply(int& i)const
  {
    i = m_browser->m_attributeIntManager->value(m_prop);
  }
  /// Set bool attribute
  void apply(bool& b)const
  {
    b = m_browser->m_attributeBoolManager->value(m_prop);
  }
  /// Set vector attribute
  void apply(std::vector<double>& v)const
  {
      //throw std::runtime_error("Vector setter not implemented.");
      QList<QtProperty*> members = m_prop->subProperties();
      if ( members.empty() ) throw std::runtime_error("FunctionBrowser: empty vector attribute group.");
      int n = members.size() - 1;
      if ( n == 0 )
      {
          v.clear();
          return;
      }
      v.resize( n );
      for(int i = 0; i < n; ++i)
      {
          v[i] = m_browser->m_attributeVectorDoubleManager->value( members[i+1] );
      }
  }
private:
  FunctionBrowser* m_browser;
  QtProperty* m_prop;
};

/**
 * Add a attribute property
 * @param parent :: Parent function property
 * @param attName :: Attribute name
 * @param att :: Attribute value
 */
FunctionBrowser::AProperty FunctionBrowser::addAttributeProperty(QtProperty* parent, QString attName, const Mantid::API::IFunction::Attribute& att)
{
  CreateAttributePropertyForFunctionBrowser cap(this,parent,attName);
  return att.apply(cap);
}

/**
 * Add attribute and parameter properties to a function property. For a composite function
 *  adds all member functions' properties
 * @param prop :: A function property
 * @param fun :: Shared pointer to a created function
 */
void FunctionBrowser::addAttributeAndParameterProperties(QtProperty* prop, Mantid::API::IFunction_sptr fun)
{
  // add the function index property
  addIndexProperty(prop);

  // add attribute properties
  auto attributeNames = fun->getAttributeNames();
  for(auto att = attributeNames.begin(); att != attributeNames.end(); ++att)
  {
    QString attName = QString::fromStdString(*att);
    addAttributeProperty(prop, attName, fun->getAttribute(*att));
  }

  auto cf = boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(fun);
  if (cf)
  {// if composite add members
    for(size_t i = 0; i < cf->nFunctions(); ++i)
    {
      AProperty ap = addFunctionProperty(prop,QString::fromStdString(cf->getFunction(i)->name()));
      addAttributeAndParameterProperties(ap.prop,cf->getFunction(i));
    }
  }
  else
  {// if simple add parameters
    for(size_t i = 0; i < fun->nParams(); ++i)
    {
      QString name = QString::fromStdString(fun->parameterName(i));
      QString desc = QString::fromStdString(fun->parameterDescription(i));
      double value = fun->getParameter(i);
      AProperty ap = addParameterProperty(prop, name, desc, value);
      // if parameter has a tie
      if (fun->isFixed(i))
      {
        auto tie = fun->getTie(i);
        if (tie)
        {
          addTieProperty(ap.prop, QString::fromStdString(tie->asString()));
        }
      }
      auto c = fun->getConstraint(i);
      if ( c )
      {
        addConstraintProperties( ap.prop, QString::fromStdString( c->asString() ) );
      }
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
 * Get property of the overall function.
 */
FunctionBrowser::AProperty FunctionBrowser::getFunctionProperty() const
{
  auto props = m_browser->properties();
  if ( props.isEmpty() )
  {
    AProperty ap;
    ap.item = NULL;
    ap.parent = NULL;
    ap.prop = NULL;
    return ap;
  }
  QtProperty* prop = props[0];
  return m_properties[prop];
}

/**
 * Get a list of names of global parameters
 */
QStringList FunctionBrowser::getGlobalParameters() const
{
  QStringList out;
  for(auto propIt = m_properties.begin(); propIt != m_properties.end(); ++propIt)
  {
    QtProperty *prop = propIt->prop;
    if ( prop->hasOption(globalOptionName) && prop->checkOption(globalOptionName) )
    {
      out << getIndex(prop) + prop->propertyName();
    }
  }
  return out;
}

/**
 * Get a list of names of local parameters
 */
QStringList FunctionBrowser::getLocalParameters() const
{
  QStringList out;
  for(auto propIt = m_properties.begin(); propIt != m_properties.end(); ++propIt)
  {
    QtProperty *prop = propIt->prop;
    if ( prop->hasOption(globalOptionName) && !prop->checkOption(globalOptionName) )
    {
      out << getIndex(prop) + prop->propertyName();
    }
  }
  return out;
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
 * Check if property is any of the string attributes
 * @param prop :: Property to check
 */
bool FunctionBrowser::isStringAttribute(QtProperty* prop) const
{
  return prop && (
    dynamic_cast<QtAbstractPropertyManager*>(m_attributeStringManager) == prop->propertyManager() ||
    dynamic_cast<QtAbstractPropertyManager*>(m_formulaManager) == prop->propertyManager() ||
    dynamic_cast<QtAbstractPropertyManager*>(m_filenameManager) == prop->propertyManager() ||
    dynamic_cast<QtAbstractPropertyManager*>(m_workspaceManager) == prop->propertyManager()
    );
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
 * Check if property is a function bool attribute
 * @param prop :: Property to check
 */
bool FunctionBrowser::isBoolAttribute(QtProperty *prop) const
{
    return prop && dynamic_cast<QtAbstractPropertyManager*>(m_attributeBoolManager) == prop->propertyManager();
}

/**
 * Check if property is a function vector attribute
 * @param prop :: Property to check
 */
bool FunctionBrowser::isVectorAttribute(QtProperty *prop) const
{
    return prop && dynamic_cast<QtAbstractPropertyManager*>(m_attributeVectorManager) == prop->propertyManager();
}

/**
 * Check if property is a function attribute
 * @param prop :: Property to check
 */
bool FunctionBrowser::isAttribute(QtProperty* prop) const
{
  return isStringAttribute(prop) || isDoubleAttribute(prop) || isIntAttribute(prop) ||
          isBoolAttribute(prop) || isVectorAttribute(prop);
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
double FunctionBrowser::getParameter(QtProperty* prop) const
{
  return m_parameterManager->value(prop);
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
 * Get the function index for a property
 * @param prop :: A property
 */
QString FunctionBrowser::getIndex(QtProperty* prop) const
{
  if ( !prop ) return "";
  if (isFunction(prop))
  {
    auto props = prop->subProperties();
    if (props.isEmpty()) return "";
    for(auto it = props.begin(); it != props.end(); ++it)
    {
      if ( isIndex(*it) )
      {
        return m_indexManager->value(*it);
      }
    }
    return "";
  }

  auto ap = m_properties[prop];
  return getIndex(ap.parent);
}

/**
 * Return function property for a function with given index.
 * @param index :: Function index to search, or empty string for top-level function
 * @return Function property, or NULL if not found
 */
QtProperty* FunctionBrowser::getFunctionProperty(const QString& index) const
{
  // Might not be the most efficient way to do it. m_functionManager might be searched instead,
  // but it is not being kept up-to-date at the moment (is not cleared).
  foreach (auto property, m_properties.keys())
  {
    if(isFunction(property) && getIndex(property) == index)
    {
      return property;
    }
  }

  // No function with such index
  return NULL;
}


/**
 * Add a tie property
 * @param prop :: Parent parameter property
 * @param tie :: A tie string
 */
FunctionBrowser::AProperty FunctionBrowser::addTieProperty(QtProperty* prop, QString tie)
{
  if ( !prop )
  {
    throw std::runtime_error("FunctionBrowser: null property pointer");
  }
  AProperty ap;
  ap.item = NULL;
  ap.prop = NULL;
  ap.parent = NULL;

  if ( !isParameter(prop) ) return ap;

  Mantid::API::Expression expr;
  expr.parse(tie.toStdString());
  // Do parameter names include composite function index
  bool isComposite = false;
  auto vars = expr.getVariables();
  for(auto var = vars.begin(); var != vars.end(); ++var)
  {
    // nesting level of a particular variable
    int n = static_cast<int>(std::count(var->begin(),var->end(),'.'));
    if ( n != 0 )
    {
      isComposite = true;
    }
  }

  // check that tie has form <paramName>=<expression>
  if ( expr.name() != "=" )
  {// prepend "<paramName>="
    if ( !isComposite )
    {
      tie.prepend(prop->propertyName() + "=");
    }
    else
    {
      QString index = getIndex(prop);
      tie.prepend(index + prop->propertyName() + "=");
    }
  }

  // find the property of the function
  QtProperty *funProp = isComposite? getFunctionProperty().prop : m_properties[prop].parent;

  QtProperty* tieProp = m_tieManager->addProperty("Tie");
  m_tieManager->setValue(tieProp, tie);
  ap = addProperty(prop,tieProp);

  ATie atie;
  atie.paramProp = prop;
  atie.tieProp = tieProp;
  //atie.tie = tie;
  m_ties.insert(funProp,atie);

  return ap;
}

/**
 * Check if a parameter property has a tie
 * @param prop :: A parameter property
 */
bool FunctionBrowser::hasTie(QtProperty* prop) const
{
  if ( !prop ) return false;
  auto children = prop->subProperties();
  foreach(QtProperty* child, children)
  {
    if ( child->propertyName() == "Tie" )
    {
      return true;
    }
  }
  return false;
}

/**
 * Check if a property is a tie
 * @param prop :: A property
 */
bool FunctionBrowser::isTie(QtProperty* prop) const
{
  return prop && dynamic_cast<QtAbstractPropertyManager*>(m_tieManager) == prop->propertyManager();
}


/**
 * Get a tie for a parameter
 * @param prop :: A parameter property
 */
std::string FunctionBrowser::getTie(QtProperty* prop) const
{
  if ( !prop ) return "";
  auto children = prop->subProperties();
  foreach(QtProperty* child, children)
  {
    if ( child->propertyName() == "Tie" )
    {
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
QList<FunctionBrowser::AProperty> FunctionBrowser::addConstraintProperties(QtProperty* prop, QString constraint)
{
  if ( !isParameter(prop) ) return QList<FunctionBrowser::AProperty>();
  QString lowerBoundStr = "";
  QString upperBoundStr = "";
  Mantid::API::Expression expr;
  expr.parse(constraint.toStdString());
  if ( expr.name() != "==" ) return QList<FunctionBrowser::AProperty>();
  if ( expr.size() == 3 )
  {// lower < param < upper
    try
    {
      // check that the first and third terms are numbers
      double d1 = boost::lexical_cast<double>( expr[0].name() ); (void)d1;
      double d2 = boost::lexical_cast<double>( expr[2].name() ); (void)d2;
      if ( expr[1].operator_name() == "<" && expr[2].operator_name() == "<" )
      {
        lowerBoundStr = QString::fromStdString( expr[0].name() );
        upperBoundStr = QString::fromStdString( expr[2].name() );
      }
      else // assume that the operators are ">"
      {
        lowerBoundStr = QString::fromStdString( expr[2].name() );
        upperBoundStr = QString::fromStdString( expr[0].name() );
      }
    }
    catch(...)
    {// error in constraint
      return QList<FunctionBrowser::AProperty>();
    }
  }
  else if ( expr.size() == 2 )
  {// lower < param or param > lower etc
    size_t paramPos = 0;
    try // find position of the parameter name in expression
    {
      double d = boost::lexical_cast<double>( expr[1].name() );(void)d;
    }
    catch(...)
    {
      paramPos = 1;
    }
    std::string op = expr[1].operator_name();
    if ( paramPos == 0 )
    {// parameter goes first
      if ( op == "<" )
      {// param < number
        upperBoundStr = QString::fromStdString( expr[1].name() );
      }
      else
      {// param > number
        lowerBoundStr = QString::fromStdString( expr[1].name() );
      }
    }
    else
    {// parameter is second
      if ( op == "<" )
      {// number < param
        lowerBoundStr = QString::fromStdString( expr[0].name() );
      }
      else
      {// number > param
        upperBoundStr = QString::fromStdString( expr[0].name() );
      }
    }
  }

  // add properties
  QList<FunctionBrowser::AProperty> plist;
  AConstraint ac;
  //ac.constraint = constraint;
  ac.paramProp = prop;
  ac.lower = ac.upper = NULL;
  if ( !lowerBoundStr.isEmpty() )
  {
    auto ap = addProperty( prop, m_constraintManager->addProperty("LowerBound") );
    plist << ap;
    ac.lower = ap.prop;
    m_constraintManager->setValue( ac.lower, lowerBoundStr );
  }
  if ( !upperBoundStr.isEmpty() )
  {
    auto ap = addProperty( prop, m_constraintManager->addProperty("UpperBound") );
    plist << ap;
    ac.upper = ap.prop;
    m_constraintManager->setValue( ac.upper, upperBoundStr );
  }
  if ( ac.lower || ac.upper )
  {
    m_constraints.insert(m_properties[prop].parent,ac);
  }

  return plist;
}

/**
 * Check if a property is a constraint
 * @param prop :: Property to check.
 */
bool FunctionBrowser::isConstraint(QtProperty* prop) const
{
  return prop && dynamic_cast<QtAbstractPropertyManager*>(m_constraintManager) == prop->propertyManager();
}

/**
 * Check if a parameter property has a constraint
 * @param prop :: A parameter property.
 */
bool FunctionBrowser::hasConstraint(QtProperty* prop) const
{
  return hasLowerBound(prop) || hasUpperBound(prop);
}

/**
 * Check if a parameter property has a lower bound
 * @param prop :: A parameter property.
 */
bool FunctionBrowser::hasLowerBound(QtProperty* prop) const
{
  if ( !isParameter(prop) ) return false;
  auto props = prop->subProperties();
  if ( props.isEmpty() ) return false;
  foreach(QtProperty* p, props)
  {
    if ( dynamic_cast<QtAbstractPropertyManager*>(m_constraintManager) == p->propertyManager() &&
      p->propertyName() == "LowerBound" ) return true;
  }
  return false;
}

/**
 * Check if a parameter property has a upper bound
 * @param prop :: A parameter property.
 */
bool FunctionBrowser::hasUpperBound(QtProperty* prop) const
{
  if ( !isParameter(prop) ) return false;
  auto props = prop->subProperties();
  if ( props.isEmpty() ) return false;
  foreach(QtProperty* p, props)
  {
    if ( dynamic_cast<QtAbstractPropertyManager*>(m_constraintManager) == p->propertyManager() &&
      p->propertyName() == "UpperBound" ) return true;
  }
  return false;
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
    if ( !QApplication::clipboard()->text().isEmpty() )
    {
      context.addAction(m_actionFromClipboard);
    }
    if ( !m_browser->properties().isEmpty() )
    {
      context.addAction(m_actionToClipboard);
    }
    context.exec(QCursor::pos());
    return;
  }
  QtProperty* prop = item->property();
  if (isFunction(prop))
  {// functions
    QMenu context(this);
    Mantid::API::IFunction_sptr fun = Mantid::API::FunctionFactory::Instance().createFunction(prop->propertyName().toStdString());
    auto cf = boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(fun);
    if (cf || m_properties[prop].parent == NULL)
    {
      context.addAction(m_actionAddFunction);
    }
    context.addAction(m_actionRemoveFunction);
    if ( !QApplication::clipboard()->text().isEmpty() )
    {
      context.addAction(m_actionFromClipboard);
    }
    if ( !m_browser->properties().isEmpty() )
    {
      context.addAction(m_actionToClipboard);
    }
    context.exec(QCursor::pos());
  }
  else if (isParameter(prop))
  {// parameters
    QMenu context(this);
    if ( hasTie(prop) )
    {
      context.addAction(m_actionRemoveTie);
    }
    else
    {
      context.addAction(m_actionFixParameter);
      context.addAction(m_actionAddTie);
    }
    bool hasLower = hasLowerBound(prop);
    bool hasUpper = hasUpperBound(prop);
    if ( !hasLower && !hasUpper )
    {
      QMenu *constraintMenu = new QMenu("Constraints",this);
      constraintMenu->addAction(m_actionConstraints10);
      constraintMenu->addAction(m_actionConstraints50);
      constraintMenu->addAction(m_actionConstraints);
      context.addMenu(constraintMenu);
    }
    else
    {
      context.addAction(m_actionRemoveConstraints);
    }
    context.exec(QCursor::pos());
  }
  else if ( isConstraint(prop) )
  {// constraints
    QMenu context(this);
    context.addAction(m_actionRemoveConstraint);
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

  // create new function
  auto f = Mantid::API::FunctionFactory::Instance().createFunction(newFunction.toStdString());
  //newFunction = QString::fromStdString(f->asString());

  if (prop)
  {// there are other functions defined
    Mantid::API::IFunction_sptr fun = Mantid::API::FunctionFactory::Instance().createFunction(prop->propertyName().toStdString());
    auto cf = boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(fun);
    if (cf)
    {
      addFunction(prop,f);
    }
    else
    {
      cf.reset(new Mantid::API::CompositeFunction);
      auto f0 = getFunction(prop);
      if ( f0 )
      {
        cf->addFunction(f0);
      }
      cf->addFunction(f);
      setFunction(cf);
    }
  }
  else
  {// the browser is empty - add first function
    addFunction(NULL,f);
  }
  emit functionStructureChanged();
}

/**
 * Return the function 
 * @param prop :: Function property 
 * @param attributesOnly :: Only set attributes
 */
Mantid::API::IFunction_sptr FunctionBrowser::getFunction(QtProperty* prop, bool attributesOnly)
{
  if (prop == NULL)
  {// get overall function
    auto props = m_browser->properties();
    if (props.isEmpty()) return Mantid::API::IFunction_sptr();
    prop = props[0];
  }
  if (!isFunction(prop)) return Mantid::API::IFunction_sptr();

  // construct the function 
  auto fun = Mantid::API::FunctionFactory::Instance().createFunction(prop->propertyName().toStdString());
  auto cf = boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(fun);
  if (cf)
  {
    auto children = prop->subProperties();
    foreach(QtProperty* child, children)
    {
      if (isFunction(child))
      {
        auto f = getFunction(child);
        // if f is null ignore that function
        if ( f )
        {
          cf->addFunction(f);
        }
      }
    }
  }
  else
  {
    // loop over the children properties and set parameters and attributes
    auto children = prop->subProperties();
    foreach(QtProperty* child, children)
    {
      if (isAttribute(child))
      {
        std::string attName = child->propertyName().toStdString();
        SetAttributeFromProperty setter(this,child);
        Mantid::API::IFunction::Attribute attr = fun->getAttribute(attName);
        attr.apply(setter);
        try
        {
          fun->setAttribute(attName,attr);
        }
        catch(std::exception& expt)
        {
          QMessageBox::critical(this,"MantidPlot - Error", "Cannot set attribute " + QString::fromStdString(attName) + 
            " of function " + prop->propertyName() + ":\n\n" + QString::fromStdString(expt.what()));
        }
      }
      else if (!attributesOnly && isParameter(child))
      {
          fun->setParameter(child->propertyName().toStdString(), getParameter(child));
      }
    }

  }

  // if this flag is set the function requires attributes only
  // attempts to set other properties may result in exceptions
  if ( attributesOnly ) return fun;

  // add ties
  {
    auto from = m_ties.lowerBound(prop);
    auto to = m_ties.upperBound(prop);
    QList<QtProperty*> filedTies; // ties can become invalid after some editing
    for(auto it = from; it != to; ++it)
    {
      try
      {
        QString tie = m_tieManager->value(it.value().tieProp);
        fun->addTies(tie.toStdString());
      }
      catch(...)
      {
        //std::cerr << "tie failed " << it.value().tie.toStdString() << std::endl;
        filedTies << it.value().tieProp;
      }
    }
    // remove failed ties from the browser
    foreach(QtProperty* p, filedTies)
    {
      removeProperty(p);
    }
  }

  // add constraints
  {
    auto from = m_constraints.lowerBound(prop);
    auto to = m_constraints.upperBound(prop);
    for(auto it = from; it != to; ++it)
    {
      try
      {
        QString constraint;
        auto cp = it.value();
        if ( cp.lower )
        {
          constraint += m_constraintManager->value(cp.lower) + "<" + cp.paramProp->propertyName();
        }
        else
        {
          constraint += cp.paramProp->propertyName();
        }
        if ( cp.upper )
        {
          constraint += "<" + m_constraintManager->value(cp.upper) ;
        }
        fun->addConstraints(constraint.toStdString());
      }
      catch(...)
      {
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
Mantid::API::IFunction_sptr FunctionBrowser::getFunctionByIndex(const QString& index)
{
  if(auto prop = getFunctionProperty(index))
  {
    return getFunction(prop);
  }
  else
  {
    return Mantid::API::IFunction_sptr();
  }
}

/**
 * Updates the function parameter value
 * @param funcIndex :: Index of the function
 * @param paramName :: Parameter name
 * @param value :: New value
 */
void FunctionBrowser::setParameter(const QString& funcIndex, const QString& paramName, double value)
{
  auto prop = getParameterProperty(funcIndex, paramName);
  m_parameterManager->setValue(prop,value);
}

/**
 * Updates the function parameter error
 * @param funcIndex :: Index of the function
 * @param paramName :: Parameter name
 * @param error :: New error
 */
void FunctionBrowser::setParamError(const QString& funcIndex, const QString& paramName, double error)
{
  if (auto prop = getFunctionProperty(funcIndex))
  {
    auto children = prop->subProperties();
    foreach(QtProperty* child, children)
    {
      if (isParameter(child) && child->propertyName() == paramName)
      {
//        m_parameterManager->setDescription(child,"");
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
double FunctionBrowser::getParameter(const QString& funcIndex, const QString& paramName) const
{
  auto prop = getParameterProperty(funcIndex, paramName);
  return m_parameterManager->value(prop);
}

/**
 * Split a qualified parameter name into function index and local parameter name.
 * @param paramName :: Fully qualified parameter name (includes function index)
 * @return :: A string list with the first item is the function index and the second
 *   item is the param local name.
 */
QStringList FunctionBrowser::splitParameterName(const QString& paramName) const
{
  QString functionIndex;
  QString parameterName = paramName;
  int j = paramName.lastIndexOf('.');
  if ( j > 0 )
  {
    ++j;
    functionIndex = paramName.mid(0,j);
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
void FunctionBrowser::setParameter(const QString& paramName, double value)
{
  QStringList name = splitParameterName(paramName);
  setParameter(name[0],name[1],value);
}

/**
 * Updates the function parameter error
 * @param paramName :: Fully qualified parameter name (includes function index)
 * @param error :: New error
 */
void FunctionBrowser::setParamError(const QString& paramName, double error)
{
  QStringList name = splitParameterName(paramName);
  setParamError(name[0],name[1],error);
}

/**
 * Get a value of a parameter
 * @param paramName :: Fully qualified parameter name (includes function index)
 */
double FunctionBrowser::getParameter(const QString& paramName) const
{
  QStringList name = splitParameterName(paramName);
  return getParameter(name[0],name[1]);
}

/// Get a property for a parameter
QtProperty* FunctionBrowser::getParameterProperty(const QString& paramName) const
{
  QStringList name = splitParameterName(paramName);
  return getParameterProperty(name[0],name[1]);
}

/// Get a property for a parameter
QtProperty* FunctionBrowser::getParameterProperty(const QString& funcIndex, const QString& paramName) const
{
  if (auto prop = getFunctionProperty(funcIndex))
  {
    auto children = prop->subProperties();
    foreach(QtProperty* child, children)
    {
      if (isParameter(child) && child->propertyName() == paramName)
      {
        return child;
      }
    }
  }
  throw std::runtime_error("Unknown function parameter " + (funcIndex + paramName).toStdString());
}

/**
 * Update parameter values in the browser to match those of a function.
 * @param fun :: A function to copy the values from. It must have the same
 *   type (composition) as the function in the browser.
 */
void FunctionBrowser::updateParameters(const Mantid::API::IFunction& fun)
{
  auto paramNames = fun.getParameterNames();
  for(auto par = paramNames.begin(); par != paramNames.end(); ++par)
  {
    setParameter( QString::fromStdString(*par), fun.getParameter(*par) );
  }
}


/**
 * Return FunctionFactory function string
 */
QString FunctionBrowser::getFunctionString()
{
  auto fun = getFunction();
  if ( !fun ) return "";
  return QString::fromStdString( fun->asString() );
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
  emit functionStructureChanged();
}

/**
 * Fix currently selected parameter
 */
void FunctionBrowser::fixParameter()
{
  auto item = m_browser->currentItem();
  if ( !item ) return;
  QtProperty* prop = item->property();
  if (!isParameter(prop)) return;
  QString tie = QString::number(getParameter(prop));
  auto ap = addTieProperty(prop, tie);
  if (ap.prop)
  {
    ap.prop->setEnabled(false);
  }
}

/// Get a tie property attached to a parameter property
QtProperty* FunctionBrowser::getTieProperty(QtProperty* prop) const
{
  auto children = prop->subProperties();
  foreach(QtProperty* child, children)
  {
    if ( child->propertyName() == "Tie" )
    {
      return child; 
    }
  }
  return NULL;
}

/**
 * Unfix currently selected parameter
 */
void FunctionBrowser::removeTie()
{
  auto item = m_browser->currentItem();
  if ( !item ) return;
  QtProperty* prop = item->property();
  if (!isParameter(prop)) return;
  auto tieProp = getTieProperty(prop);
  if ( tieProp )
  {
    removeProperty(tieProp);
  }
}

/**
 * Add a custom tie to currently selected parameter
 */
void FunctionBrowser::addTie()
{
  auto item = m_browser->currentItem();
  if ( !item ) return;
  QtProperty* prop = item->property();
  if (!isParameter(prop)) return;

  bool ok;
  QString tie = QInputDialog::getText(this, "Add a tie", "Tie:", QLineEdit::Normal, "", &ok);
  if ( ok && !tie.isEmpty() )
  {
    addTieProperty(prop, tie);
  }
}

/**
 * Copy function from the clipboard
 */
void FunctionBrowser::copyFromClipboard()
{
  QString funStr = QApplication::clipboard()->text();
  if ( funStr.isEmpty() ) return;
  try
  {
    auto fun = Mantid::API::FunctionFactory::Instance().createInitialized( funStr.toStdString() );
    if ( !fun ) return;
    this->setFunction(fun);
  }
  catch(...)
  {
    // text in the clipboard isn't a function definition
    QMessageBox::warning(this,"MantidPlot - Warning", "Text in the clipboard isn't a function definition"
      " or contains errors.");
  }
}

/**
 * Copy function to the clipboard
 */
void FunctionBrowser::copyToClipboard()
{
  auto fun = getFunction();
  if ( fun ) 
  {
    QApplication::clipboard()->setText( QString::fromStdString(fun->asString()) );
  }
}

/**
 * Add both constraints to current parameter
 */
void FunctionBrowser::addConstraints()
{
  auto item = m_browser->currentItem();
  if ( !item ) return;
  QtProperty* prop = item->property();
  if (!isParameter(prop)) return;
  addConstraintProperties(prop,"0<"+prop->propertyName()+"<0");
}

/**
 * Add both constraints to current parameter
 */
void FunctionBrowser::addConstraints10()
{
  auto item = m_browser->currentItem();
  if ( !item ) return;
  QtProperty* prop = item->property();
  if (!isParameter(prop)) return;
  double val = getParameter(prop);
  addConstraintProperties(prop,QString::number(val*0.9)+"<"+prop->propertyName()+"<"+QString::number(val*1.1));
}

/**
 * Add both constraints to current parameter
 */
void FunctionBrowser::addConstraints50()
{
  auto item = m_browser->currentItem();
  if ( !item ) return;
  QtProperty* prop = item->property();
  if (!isParameter(prop)) return;
  double val = getParameter(prop);
  addConstraintProperties(prop,QString::number(val*0.5)+"<"+prop->propertyName()+"<"+QString::number(val*1.5));
}

/**
 * Remove both constraints from current parameter
 */
void FunctionBrowser::removeConstraints()
{
  auto item = m_browser->currentItem();
  if ( !item ) return;
  QtProperty* prop = item->property();
  if (!isParameter(prop)) return;
  auto props = prop->subProperties();
  foreach(QtProperty* p, props)
  {
    if ( isConstraint(p) )
    {
      removeProperty( p );
    }
  }
}

/**
 * Remove one constraint from current parameter
 */
void FunctionBrowser::removeConstraint()
{
  auto item = m_browser->currentItem();
  if ( !item ) return;
  QtProperty* prop = item->property();
  if (!isConstraint(prop)) return;
  removeProperty( prop );
}

void FunctionBrowser::updateCurrentFunctionIndex()
{
  boost::optional<QString> newIndex;

  if (auto item = m_browser->currentItem())
  {
    auto prop = item->property();
    newIndex = getIndex(prop);
  }

  if (m_currentFunctionIndex != newIndex)
  {
    m_currentFunctionIndex = newIndex;
    emit currentFunctionChanged();
  }
}

/**
 * Slot connected to all function attribute managers. Update the corresponding function.
 * @param prop :: An attribute property that was changed
 */
void FunctionBrowser::attributeChanged(QtProperty* prop)
{
  auto funProp = m_properties[prop].parent;
  if ( !funProp ) return;
  // get function with the changed attribute (it is set from prop's value)
  auto fun = getFunction( funProp, true );

  // delete and recreate all function's properties (attributes, parameters, etc)
  setFunction(funProp, fun);
  updateFunctionIndices();
}

/**
 * Slot connected to a property displaying the value of a member of a vector attribute.
 * @param prop :: A property that was changed.
 */
void FunctionBrowser::attributeVectorDoubleChanged(QtProperty *prop)
{
    QtProperty *vectorProp = m_properties[prop].parent;
    if ( !vectorProp ) throw std::runtime_error("FunctionBrowser: inconsistency in vector properties.");
    attributeChanged( vectorProp );
}

void FunctionBrowser::parameterChanged(QtProperty* prop)
{
  emit parameterChanged(getIndex(prop), prop->propertyName());
}

void FunctionBrowser::parameterButtonClicked(QtProperty *prop)
{
  emit localParameterButtonClicked(getIndex(prop) + prop->propertyName());
}

bool FunctionBrowser::hasFunction() const
{
  return ! m_functionManager->properties().isEmpty();
}

/// Get the number of datasets
int FunctionBrowser::getNumberOfDatasets() const
{
  return m_numberOfDatasets;
}

/// Set new number of the datasets
/// @param n :: New value for the number of datasets.
void FunctionBrowser::setNumberOfDatasets(int n)
{
  if ( !m_multiDataset )
  {
    throw std::runtime_error("Function browser wasn't set up for multi-dataset fitting.");
  }
  m_numberOfDatasets = n;
}

/**
 * Get value of a local parameter
 * @param parName :: Name of a parameter.
 * @param i :: Data set index.
 */
double FunctionBrowser::getLocalParameterValue(const QString& parName, int i) const
{
  checkLocalParameter(parName);
  return m_localParameterValues[parName][i].value;
}

void FunctionBrowser::setLocalParameterValue(const QString& parName, int i, double value)
{
  checkLocalParameter(parName);
  m_localParameterValues[parName][i].value = value;
  if ( i == m_currentDataset )
  {
    setParameter( parName, value );
  }
}

/**
 * Init a local parameter. Define initial values for all datasets.
 * @param parName :: Name of parametere to init.
 */
void FunctionBrowser::initLocalParameter(const QString& parName)const
{
  double value = getParameter(parName);
  QVector<LocalParameterData> values( getNumberOfDatasets(), LocalParameterData(value) );
  m_localParameterValues[parName] = values;
}

/// Make sure that the parameter is initialized
/// @param parName :: Name of a parameter to check
void FunctionBrowser::checkLocalParameter(const QString& parName)const
{
  if ( !m_localParameterValues.contains(parName) || m_localParameterValues[parName].size() != getNumberOfDatasets() )
  {
    initLocalParameter(parName);
  }
}

void FunctionBrowser::resetLocalParameters()
{
  m_localParameterValues.clear();
}

/// Set current dataset.
void FunctionBrowser::setCurrentDataset(int i)
{
  m_currentDataset = i;
  if ( m_currentDataset >= m_numberOfDatasets )
  {
    throw std::runtime_error("Dataset index is outside the range");
  }
  auto localParameters = getLocalParameters();
  foreach(QString par, localParameters)
  {
    setParameter( par, getLocalParameterValue( par, m_currentDataset ) );
    updateLocalTie(par);
  }
}

/// Remove local parameter values for a number of datasets.
/// @param indices :: A list of indices of datasets to remove.
void FunctionBrowser::removeDatasets(QList<int> indices)
{
  int newSize = m_numberOfDatasets;
  qSort(indices);
  for(auto par = m_localParameterValues.begin(); par != m_localParameterValues.end(); ++par)
  {
    for(int i = indices.size() - 1; i >= 0; --i)
    {
      int index = indices[i];
      if ( index < m_numberOfDatasets )
      {
        par.value().remove(index);
      }
    }
    newSize = par.value().size();
  }
  setNumberOfDatasets( newSize );
}

/// Add local parameters for additional datasets.
/// @param n :: Number of datasets added.
void FunctionBrowser::addDatasets(int n)
{
  if ( getNumberOfDatasets() == 0 )
  {
    setNumberOfDatasets( n );
    return;
  }
  int newSize = m_numberOfDatasets;
  for(auto par = m_localParameterValues.begin(); par != m_localParameterValues.end(); ++par)
  {
    auto &values = par.value();
    double value = values.back().value;
    values.insert(values.end(),n,LocalParameterData(value));
    newSize = values.size();
  }
  setNumberOfDatasets(newSize);
}

/// Return the multidomain function for multi-dataset fitting
Mantid::API::IFunction_sptr FunctionBrowser::getGlobalFunction()
{
  if ( !m_multiDataset )
  {
    throw std::runtime_error("Function browser wasn't set up for multi-dataset fitting.");
  }
  // number of spectra to fit == size of the multi-domain function
  int nOfDataSets = getNumberOfDatasets();
  if ( nOfDataSets == 0 )
  {
    throw std::runtime_error("There are no data sets specified.");
  }

  // description of a single function
  QString funStr = getFunctionString();

  if ( nOfDataSets == 1 )
  {
    return Mantid::API::FunctionFactory::Instance().createInitialized( funStr.toStdString() );
  }

  bool isComposite = (std::find(funStr.begin(),funStr.end(),';') != funStr.end());
  if ( isComposite )
  {
    funStr = ";(" + funStr + ")";
  }
  else
  {
    funStr = ";" + funStr;
  }

  QString multiFunStr = "composite=MultiDomainFunction,NumDeriv=1";
  for(int i = 0; i < nOfDataSets; ++i)
  {
    multiFunStr += funStr;
  }

  // add the global ties
  QStringList globals = getGlobalParameters();
  QString globalTies;
  if ( !globals.isEmpty() )
  {
    globalTies = "ties=(";
    bool isFirst = true;
    foreach(QString par, globals)
    {
      if ( !isFirst ) globalTies += ",";
      else
        isFirst = false;

      for(int i = 1; i < nOfDataSets; ++i)
      {
        globalTies += QString("f%1.").arg(i) + par + "=";
      }
      globalTies += QString("f0.%1").arg(par);
    }
    globalTies += ")";
    multiFunStr += ";" + globalTies;
  }

  // create the multi-domain function
  auto fun = Mantid::API::FunctionFactory::Instance().createInitialized( multiFunStr.toStdString() );
  boost::shared_ptr<Mantid::API::MultiDomainFunction> multiFun = boost::dynamic_pointer_cast<Mantid::API::MultiDomainFunction>( fun );
  if ( !multiFun )
  {
    throw std::runtime_error("Failed to create the MultiDomainFunction");
  }
  
  auto globalParams = getGlobalParameters();

  // set the domain indices, initial local parameter values and ties
  for(int i = 0; i < nOfDataSets; ++i)
  {
    multiFun->setDomainIndex(i,i);
    auto fun1 = multiFun->getFunction(i);
    for(size_t j = 0; j < fun1->nParams(); ++j)
    {
      QString parName = QString::fromStdString(fun1->parameterName(j));
      if ( globalParams.contains(parName) ) continue;
      auto tie = fun1->getTie(j);
      if ( tie )
      {
        // If parameter has a tie at this stage then it gets it form the currently
        // displayed function. But the i-th local parameters may not have this tie,
        // so remove it
        fun1->removeTie(j);
      }
      if ( isLocalParameterFixed(parName,i) )
      {
        // Fix this particular local parameter
        fun1->tie(parName.toStdString(),boost::lexical_cast<std::string>( getLocalParameterValue(parName,i) ));
      }
      else
      {
        fun1->setParameter(j, getLocalParameterValue(parName,i));
      }
    }
  }
  assert( multiFun->nFunctions() == static_cast<size_t>(nOfDataSets) );

  return fun;
}

/// Make sure that properties are in sync with the cached ties
/// @param parName :: A parameter to check.
void FunctionBrowser::updateLocalTie(const QString& parName)
{
    auto prop = getParameterProperty(parName);
    if ( hasTie(prop) )
    {
      auto tieProp = getTieProperty(prop);
      removeProperty(tieProp);
    }
    if ( m_localParameterValues[parName][m_currentDataset].fixed )
    {
      auto ap = addTieProperty(prop, QString::number(m_localParameterValues[parName][m_currentDataset].value));
      if (ap.prop)
      {
        ap.prop->setEnabled(false);
      }
    }
}


/// Fix/unfix a local parameter
/// @param parName :: Parameter name
/// @param i :: Index of a dataset.
/// @param fixed :: Make it fixed (true) or free (false)
void FunctionBrowser::setLocalParameterFixed(const QString& parName, int i, bool fixed)
{
  checkLocalParameter(parName);
  m_localParameterValues[parName][i].fixed = fixed;
  if ( i == m_currentDataset )
  {
    updateLocalTie(parName);
  }
}

/// Check if a local parameter is fixed
/// @param parName :: Parameter name
/// @param i :: Index of a dataset.
bool FunctionBrowser::isLocalParameterFixed(const QString& parName, int i) const
{
  checkLocalParameter(parName);
  return m_localParameterValues[parName][i].fixed;
}

/// Update the interface to have the same parameter values as in a function.
/// @param fun :: A function to get parameter values from.
void FunctionBrowser::updateMultiDatasetParameters(const Mantid::API::IFunction& fun)
{
  auto cfun = dynamic_cast<const Mantid::API::CompositeFunction*>( &fun );
  if ( cfun && cfun->nFunctions() > 0 )
  {
    auto qLocalParameters = getLocalParameters();
    std::vector<std::string> localParameters;
    foreach(QString par, qLocalParameters)
    {
      localParameters.push_back( par.toStdString() );
    }
    size_t currentIndex = static_cast<size_t>( m_currentDataset );
    for(size_t i = 0; i < cfun->nFunctions(); ++i)
    {
      auto sfun = cfun->getFunction(i);
      if ( i == currentIndex )
      {
        updateParameters( *sfun );
      }
      for(int j = 0; j < qLocalParameters.size(); ++j)
      {
        setLocalParameterValue( qLocalParameters[j], static_cast<int>(i), sfun->getParameter(localParameters[j]) );
      }
    }
  }
  else
  {
    updateParameters( fun );
  }
}

} // MantidWidgets
} // MantidQt
