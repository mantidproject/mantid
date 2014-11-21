#include "MantidQtMantidWidgets/PropertyHandler.h"
#include "MantidQtMantidWidgets/FitPropertyBrowser.h"
//#include "../FunctionCurve.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/ConstraintFactory.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"

#include "qttreepropertybrowser.h"
#include "qtpropertymanager.h"
#include "ParameterPropertyManager.h"

#include <QMessageBox>
#include <QMenu>

using std::size_t;

namespace MantidQt
{
namespace MantidWidgets
{

// Constructor
PropertyHandler::PropertyHandler(Mantid::API::IFunction_sptr fun,
                Mantid::API::CompositeFunction_sptr parent,
                FitPropertyBrowser* browser,
                QtBrowserItem* item)
                :FunctionHandler(fun),m_browser(browser),
                m_cf(boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(fun)),
                m_pf(boost::dynamic_pointer_cast<Mantid::API::IPeakFunction>(fun)),
                m_parent(parent),
                m_type(NULL),
                m_item(item),
                m_isMultispectral(false),
                m_workspace(NULL),
                m_workspaceIndex(NULL),
                m_base(0),
                m_ci(0),
                m_hasPlot(false)
{}

/// Destructor
PropertyHandler::~PropertyHandler()
{
}

/// overrides virtual init() which is called from IFunction::setHandler(...)
void PropertyHandler::init()
{
  m_browser->m_changeSlotsEnabled = false;
  if (m_parent == NULL)
  {// the root composite function
    m_item = m_browser->m_functionsGroup;
  }
  else if (m_item == NULL)
  {
    if ( !m_parent->getHandler() )
    {
      throw std::runtime_error("Parent function handler does not exist");
    }
    //PropertyHandler* ph = parentHandler();
    QtBrowserItem* pi = parentHandler()->item();
    // Create group property with function name on it
    QtProperty* fnProp = m_browser->m_groupManager->addProperty(
      functionName()
      );
    pi->property()->addSubProperty(fnProp);
    // assign m_item
    QList<QtBrowserItem *> itList = pi->children();
    foreach(QtBrowserItem* item,itList)
    {
      if (item->property() == fnProp)
      {
        m_item = item;
        break;
      }
    }
    if (m_item == 0) 
      throw std::runtime_error("Browser item not found");

    if (!m_cf)
    {
      m_browser->m_browser->setExpanded(m_item,false);
    }
  }
  else
  {
    m_item->property()->setPropertyName(functionName());
  }

  QtProperty* fnProp = m_item->property();

  // create Type property
  if (!m_type)
  {
    m_type = m_browser->m_enumManager->addProperty("Type");

    fnProp->addSubProperty(m_type);
    if (m_parent)
    {
      m_browser->m_enumManager->setEnumNames(m_type, m_browser->m_registeredFunctions);
    }
    else
    {
      QStringList functionNames;
      functionNames << "CompositeFunction";// << "MultiBG";
      m_browser->m_enumManager->setEnumNames(m_type, functionNames);
    }
  }
  int itype = m_browser->m_enumManager->enumNames(m_type).indexOf(QString::fromStdString(m_fun->name()));
  m_browser->m_enumManager->setValue(m_type,itype);

  // create worspace and workspace index properties if parent is a MultiBG
  initWorkspace();

  // create attribute properties
  initAttributes();

  // create parameter properties
  initParameters();

  // set handlers for the child functions
  if (m_cf && m_cf->nFunctions() > 0)
  {
    for(size_t i=0;i<m_cf->nFunctions();i++)
    {
      Mantid::API::IFunction_sptr f = boost::dynamic_pointer_cast<Mantid::API::IFunction>(m_cf->getFunction(i));
      if (!f)
      {
        throw std::runtime_error("IFunction expected but func function of another type");
      }
      PropertyHandler* h = new PropertyHandler(f,m_cf,m_browser);
      f->setHandler(h);
    }
  }

  m_browser->m_changeSlotsEnabled = true;
}

/**
 * Attribute visitor to create a QtProperty. Depending on the attribute type
 * the appropriate apply method is used.
 */
class CreateAttributeProperty: public Mantid::API::IFunction::ConstAttributeVisitor<QtProperty*>
{
public:
  CreateAttributeProperty(FitPropertyBrowser* browser, PropertyHandler *handler, const QString& name)
      :m_browser(browser),m_handler(handler),m_name(name){}
protected:
  /// Create string property
  QtProperty* apply(const std::string& str)const
  {
    QtProperty* prop = NULL;
    prop = m_browser->addStringProperty(m_name);
    m_browser->setStringPropertyValue(prop,QString::fromStdString(str));
    return prop;
  }
  /// Create double property
  QtProperty* apply(const double& d)const
  {
    QtProperty* prop = m_browser->addDoubleProperty(m_name);
    m_browser->m_doubleManager->setValue(prop,d);
    return prop;
  }
  /// Create int property
  QtProperty* apply(const int& i)const
  {
    QtProperty* prop = m_browser->m_intManager->addProperty(m_name);
    m_browser->m_intManager->setValue(prop,i);
    return prop;
  }
  /// Create bool property
  QtProperty* apply(const bool& b)const
  {
    QtProperty* prop = m_browser->m_boolManager->addProperty(m_name);
    m_browser->m_boolManager->setValue(prop,b);
    return prop;
  }
  /// Create vector property
  QtProperty* apply(const std::vector<double>& b)const
  {
      //throw std::runtime_error("Vector attribute property not implememted.");
      QtProperty *prop = m_browser->m_vectorManager->addProperty(m_name);
      m_browser->m_vectorSizeManager->blockSignals(true);
      QtProperty *sizeProp = m_browser->m_vectorSizeManager->addProperty("Size");
      m_browser->m_vectorSizeManager->setValue(sizeProp, static_cast<int>(b.size()));
      prop->addSubProperty(sizeProp);
      sizeProp->setEnabled(false);
      m_browser->m_vectorSizeManager->blockSignals(false);
      m_browser->m_vectorDoubleManager->blockSignals(true);
      QString dpName = "value[%1]";
      for(size_t i = 0; i < b.size(); ++i)
      {
          QtProperty *dprop = m_browser->addDoubleProperty( dpName.arg(i), m_browser->m_vectorDoubleManager );
          m_browser->m_vectorDoubleManager->setValue(dprop, b[i]);
          prop->addSubProperty(dprop);
          m_handler->m_vectorMembers << dprop;
      }
      m_browser->m_vectorDoubleManager->blockSignals(false);
      return prop;
  }
private:
  FitPropertyBrowser* m_browser;
  PropertyHandler* m_handler;
  QString m_name;
};

/**
 * Create and attach QtProperties for function attributes. 
 */
void PropertyHandler::initAttributes()
{
  std::vector<std::string> attNames = function()->getAttributeNames();
  for(int i=0;i<m_attributes.size();i++)
  {
    m_item->property()->removeSubProperty(m_attributes[i]);
  }
  m_attributes.clear();
  m_vectorMembers.clear();
  for(size_t i=0;i<attNames.size();i++)
  {
    QString aName = QString::fromStdString(attNames[i]);
    Mantid::API::IFunction::Attribute att = function()->getAttribute(attNames[i]);
    CreateAttributeProperty tmp(m_browser, this, aName);
    QtProperty* prop = att.apply(tmp);
    m_item->property()->addSubProperty(prop);
    m_attributes << prop;
  }
}

void PropertyHandler::initParameters()
{
  for(int i=0;i<m_parameters.size();i++)
  {
    m_item->property()->removeSubProperty(m_parameters[i]);
  }
  m_parameters.clear();
  for(size_t i=0;i<function()->nParams();i++)
  {
    QString parName = QString::fromStdString(function()->parameterName(i));
    if (parName.contains('.')) continue;
    QtProperty* prop = m_browser->addDoubleProperty(parName, m_browser->m_parameterManager);

    m_browser->m_parameterManager->setDescription(prop, function()->parameterDescription(i));
    m_browser->m_parameterManager->setValue(prop,function()->getParameter(i));

    m_item->property()->addSubProperty(prop);
    m_parameters << prop;
    // add tie property if this parameter has a tie
    Mantid::API::ParameterTie* tie = m_fun->getTie(i);
    if (tie)
    {
      QStringList qtie = 
        QString::fromStdString(tie->asString(m_browser->theFunction().get())).split("=");
      if (qtie.size() > 1)
      {
        QtProperty* tieProp = m_browser->m_stringManager->addProperty("Tie");
        m_browser->m_stringManager->setValue(tieProp,qtie[1]);
        prop->addSubProperty(tieProp);
        m_ties[parName] = tieProp;
      }
    }
    // add constraint properties
    Mantid::API::IConstraint* c = m_fun->getConstraint(i);
    if (c)
    {
      QStringList qc = QString::fromStdString(c->asString()).split("<");
      bool lo = false;
      bool up = false;
      double loBound=0, upBound=0;
      if (qc.size() == 2)
      {
        if (qc[0].contains(parName))
        {
          up = true;
          upBound = qc[1].toDouble();
        }
        else
        {
          lo = true;
          loBound = qc[0].toDouble();
        }
      }
      else if (qc.size() == 3)
      {
        lo = up = true;
        loBound = qc[0].toDouble();
        upBound = qc[2].toDouble();
      }
      else
      {
        continue;
      }
      QtProperty* loProp = NULL;
      QtProperty* upProp = NULL;
      if (lo)
      {
        loProp = m_browser->addDoubleProperty("LowerBound");
        m_browser->m_doubleManager->setValue(loProp,loBound);
        prop->addSubProperty(loProp);
      }
      if (up)
      {
        upProp = m_browser->addDoubleProperty("UpperBound");
        m_browser->m_doubleManager->setValue(upProp,upBound);
        prop->addSubProperty(upProp);
      }
      m_constraints.insert(parName,std::pair<QtProperty*,QtProperty*>(loProp,upProp));
    }
  }
}

void PropertyHandler::initWorkspace()
{
  if (m_parent && m_parent->name() == "MultiBG")
  {
    //m_workspace = m_browser->m_enumManager->addProperty("Workspace");
    //QtProperty* fnProp = m_item->property();
    //fnProp->addSubProperty(m_workspace);
    //m_workspaceIndex = m_browser->m_intManager->addProperty("Workspace Index");
    //if (! m_browser->m_workspaceNames.isEmpty() )
    //{
    //  QStringList names("All");
    //  foreach(QString name,m_browser->m_workspaceNames)
    //  {
    //    names.append(name);
    //  }
    //  m_browser->m_enumManager->setEnumNames(m_workspace, names);
    //  int iWorkspace = 0;
    //  int iWorkspaceIndex = 0;
    //  if (ifun()->getWorkspace())
    //  {
    //    Mantid::API::IFunctionMW* ifmw = dynamic_cast<Mantid::API::IFunctionMW*>(ifun());
    //    if (ifmw)
    //    {
    //      std::string wsName = ifmw->getMatrixWorkspace()->getName();
    //      iWorkspace = names.indexOf(QString::fromStdString(wsName));
    //      if (iWorkspace >= 0)
    //      {
    //        iWorkspaceIndex = static_cast<int>(ifmw->getWorkspaceIndex());
    //        fnProp->addSubProperty(m_workspaceIndex);
    //      }
    //      else
    //      {
    //        iWorkspace = 0;
    //      }
    //    }
    //  }
    //  m_browser->m_enumManager->setValue(m_workspace,iWorkspace);
    //  m_browser->m_intManager->setValue(m_workspaceIndex,iWorkspaceIndex);
    //}
  }
  else
  {
    m_workspace = m_workspaceIndex = NULL;
  }
}

/**
  * Add a function to the function handled by this handler.
  * @param fnName :: A function name or full initialization string
  *   in the form name=FunctionName,param1=Value,param2=Value,...
  */
PropertyHandler* PropertyHandler::addFunction(const std::string& fnName)
{
  if (!m_cf) return NULL;
  m_browser->disableUndo();
  Mantid::API::IFunction_sptr f;
  // Create new function
  if (fnName.find("=") == std::string::npos)
  {// either from name
    f = Mantid::API::FunctionFactory::Instance().createFunction(fnName);
  }
  else
  {// of from full initialization expression
    f = Mantid::API::FunctionFactory::Instance().createInitialized(fnName);
  }

  // turn of the change slots (doubleChanged() etc) to avoid infinite loop
  m_browser->m_changeSlotsEnabled = false;

  // Check if it's a peak and set its width
  boost::shared_ptr<Mantid::API::IPeakFunction> pf = boost::dynamic_pointer_cast<Mantid::API::IPeakFunction>(f);
  if (pf)
  {
    if (!m_browser->workspaceName().empty() && 
         m_browser->workspaceIndex() >= 0 &&
         pf->centre() == 0.)
    {
      pf->setCentre( (m_browser->startX() + m_browser->endX())/2 );
    }
  }

  Mantid::API::MatrixWorkspace_sptr ws;

  try
  {
    ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
      Mantid::API::AnalysisDataService::Instance().retrieve(m_browser->workspaceName()) );
  }
  catch(...){}

  size_t wi = m_browser->workspaceIndex();

  // if it's a LinearBackground estimate its A0 and A1 parameters
  // from data values at the ends of the fitting interval
    if (f->name() == "LinearBackground" && !m_browser->workspaceName().empty())
    {
      if (ws && wi < ws->getNumberHistograms())
      {
        const Mantid::MantidVec& X = ws->readX(wi);
        size_t istart = 0, iend = 0;
        for(size_t i=0; i < X.size()-1; ++i)
        {
          double x = X[i];
          if (x < m_browser->startX())
          {
            istart = i;
          }
          if (x > m_browser->endX())
          {
            iend = i;
            if (iend > 0) iend--;
            break;
          }
        }
        if (iend > istart)
        {
          const Mantid::MantidVec& Y = ws->readY(wi);
          double p0 = Y[istart];
          double p1 = Y[iend];
          double A1 = (p1-p0)/(X[iend]-X[istart]);
          double A0 = p0 - A1*X[istart];
          f->setParameter("A0",A0);
          f->setParameter("A1",A1);
        }
      }
  }
  if (ws)
  {
    m_browser->setWorkspace(f);
  }

  size_t nFunctions = m_cf->nFunctions()+1;
  m_cf->addFunction(f);
  m_browser->compositeFunction()->checkFunction();

  if (m_cf->nFunctions() != nFunctions)
  {// this may happen
    m_browser->reset();
    return NULL;
  }

  PropertyHandler* h = new PropertyHandler(f,m_cf,m_browser);
  f->setHandler(h);
  h->setAttribute("StartX",m_browser->startX());
  h->setAttribute("EndX",m_browser->endX());

  // enable the change slots
  m_browser->m_changeSlotsEnabled = true;
  m_browser->setFitEnabled(true);
  if (pf)
  {
    m_browser->setDefaultPeakType(f->name());
  }
  else
  {
    m_browser->setDefaultBackgroundType(f->name());
  }
  m_browser->setFocus();
  m_browser->setCurrentFunction(h);

  return h;
}

// Removes handled function from its parent function and 
// properties from the browser
void PropertyHandler::removeFunction()
{
  PropertyHandler* ph = parentHandler();
  if (ph)
  {
    if (this == m_browser->m_autoBackground)
    {
      m_browser->m_autoBackground = NULL;
    }
    ph->item()->property()->removeSubProperty(m_item->property());
    Mantid::API::CompositeFunction_sptr cf = ph->cfun();
    for(int i=0;i<static_cast<int>(cf->nFunctions());i++)
    {
      if (cf->getFunction(i) == function())
      {
        emit m_browser->removePlotSignal(this);
        cf->removeFunction(i);
        break;
      }
    }
    ph->renameChildren();
  }
}

void PropertyHandler::renameChildren()const
{
  m_browser->m_changeSlotsEnabled = false;
  // update tie properties, as the parameter names may change
  QMap<QString,QtProperty*>::const_iterator it = m_ties.begin();
  for(;it!=m_ties.end();++it)
  {
    QtProperty* prop = it.value();
    Mantid::API::ParameterTie* tie = 
      m_fun->getTie(m_fun->parameterIndex(it.key().toStdString()));
    if (!tie) continue;
    QStringList qtie = QString::fromStdString(tie->asString()).split("=");
    if (qtie.size() < 2) continue;
    m_browser->m_stringManager->setValue(prop,qtie[1]);
  }
  if (!m_cf) return;
  // rename children
  for(size_t i=0;i<m_cf->nFunctions();i++)
  {
    PropertyHandler* h = getHandler(i);
    if (!h) continue;
    QtProperty* nameProp = h->item()->property();
    nameProp->setPropertyName(h->functionName());
    h->renameChildren();
  }
  m_browser->m_changeSlotsEnabled = true;
}

/// Creates name for this function to be displayed
/// in the browser
QString PropertyHandler::functionName()const
{
  QString name = functionPrefix();
  if (!name.isEmpty())
  {
    name += "-";
  }
  name += QString::fromStdString(function()->name());
  return name;
}

QString PropertyHandler::functionPrefix()const
{
  PropertyHandler* ph = parentHandler();
  if (ph)
  {
    int iFun = -1;
    Mantid::API::CompositeFunction_sptr cf = ph->cfun();
    for(int i=0;i<static_cast<int>(cf->nFunctions());i++)
    {
      if (cf->getFunction(i) == function())
      {
        iFun = i;
        break;
      }
    }
    QString pref = ph->functionPrefix();
    if (!pref.isEmpty()) pref += ".";
    return pref + "f" + QString::number(iFun);
  }
  return "";
}

// Return the parent handler
PropertyHandler* PropertyHandler::parentHandler()const
{
  if (!m_parent) return 0;
  PropertyHandler* ph = static_cast<PropertyHandler*>(m_parent->getHandler());
  return ph;
}
// Return the child's handler
PropertyHandler* PropertyHandler::getHandler(std::size_t i)const
{
  if (!m_cf || i >= m_cf->nFunctions()) return 0;
  PropertyHandler* ph = static_cast<PropertyHandler*>(m_cf->getFunction(i)->getHandler());
  return ph;
}
/** Returns 'this' if item == m_item and this is a composite function or
* calls findCompositeFunction recursively with all its children or
* zero
*/
Mantid::API::CompositeFunction_const_sptr PropertyHandler::findCompositeFunction(QtBrowserItem* item)const
{
  if (!m_cf) return Mantid::API::CompositeFunction_sptr();
  if (item == m_item) return m_cf;
  for(size_t i=0;i<m_cf->nFunctions();i++)
  {
    Mantid::API::CompositeFunction_const_sptr res = getHandler(i)->findCompositeFunction(item);
    if (res != NULL) return res;
  }
  return Mantid::API::CompositeFunction_sptr();
}
/** Returns 'this' if item == m_item or
* calls findFunction recursively with all its children or
* zero
*/
Mantid::API::IFunction_const_sptr PropertyHandler::findFunction(QtBrowserItem* item)const
{
  if (item == m_item) return function();
  if (!m_cf) return Mantid::API::IFunction_sptr();
  for(size_t i=0;i<m_cf->nFunctions();i++)
  {
    Mantid::API::IFunction_const_sptr res = getHandler(i)->findFunction(item);
    if (res != NULL) return res;
  }
  return Mantid::API::IFunction_sptr();
}

PropertyHandler* PropertyHandler::findHandler(QtProperty* prop)
{
  if (prop == NULL) return NULL;
  if (prop == m_item->property()) return this;
  if (prop == m_type) return this;
  if (prop == m_workspace) return this;
  if (prop == m_workspaceIndex) return this;
  if (m_attributes.contains(prop)) return this;
  if (m_parameters.contains(prop)) return this;
  if (m_vectorMembers.contains(prop)) return this;
  if (!m_ties.key(prop,"").isEmpty()) return this;
  QMap<QString,std::pair<QtProperty*,QtProperty*> >::iterator it = m_constraints.begin();
  for(;it!=m_constraints.end();++it)
  {
    if (it.value().first == prop || it.value().second == prop)
    {
      return this;
    }
  }
  if (!m_cf) return 0;
  for(size_t i=0;i<m_cf->nFunctions();i++)
  {
    PropertyHandler* h = getHandler(i)->findHandler(prop);
    if (h != NULL) return h;
  }
  return NULL;
}

PropertyHandler* PropertyHandler::findHandler(Mantid::API::IFunction_const_sptr fun)
{
  if (fun == function()) return this;
  if (m_cf)
  {
    for(size_t i=0;i<m_cf->nFunctions();i++)
    {
      PropertyHandler* h = getHandler(i)->findHandler(fun);
      if (h) return h;
    }
  }
  return NULL;
}

PropertyHandler* PropertyHandler::findHandler(const Mantid::API::IFunction* fun)
{
  if (fun == function().get()) return this;
  if (m_cf)
  {
    for(size_t i=0;i<m_cf->nFunctions();i++)
    {
      PropertyHandler* h = getHandler(i)->findHandler(fun);
      if (h) return h;
    }
  }
  return NULL;
}

/**
* Set function parameter value read from a QtProperty
* @param prop :: The (double) property with the new parameter value
* @return true if successfull
*/
bool PropertyHandler::setParameter(QtProperty* prop)
{
  if (m_parameters.contains(prop))
  {
    std::string parName = prop->propertyName().toStdString();
    double parValue = m_browser->m_parameterManager->value(prop);
    m_fun->setParameter(parName,parValue);
    m_browser->sendParameterChanged(m_fun.get());
    return true;
  }
  if (m_cf)
  {
    for(size_t i=0;i<m_cf->nFunctions();i++)
    {
      bool res = getHandler(i)->setParameter(prop);
      if (res) return true;
    }
  }
  return false;
}

/**
 * Visitor setting new attribute value. Depending on the attribute type
 * the appropriate apply method is used.
 */
class SetAttribute: public Mantid::API::IFunction::AttributeVisitor<>
{
public:
  SetAttribute(FitPropertyBrowser* browser,QtProperty* prop)
    :m_browser(browser),m_prop(prop){}
protected:
  /// Create string property
  void apply(std::string& str)const
  {
    QString attName = m_prop->propertyName();
    str = m_browser->getStringPropertyValue(m_prop).toStdString();
  }
  /// Create double property
  void apply(double& d)const
  {
    d = m_browser->m_doubleManager->value(m_prop);
  }
  /// Create int property
  void apply(int& i)const
  {
    i = m_browser->m_intManager->value(m_prop);
  }
  /// Create bool property
  void apply(bool& b)const
  {
    b = m_browser->m_boolManager->value(m_prop);
  }
  /// Create vector property
  void apply(std::vector<double>& v)const
  {
      QList<QtProperty*> members = m_prop->subProperties();
      if ( members.size() <= 1 )
      {
          v.clear();
          return;
      }
      v.resize( members.size() - 1 );
      for(int i = 1; i < members.size(); ++i)
      {
          v[i-1] = m_browser->m_vectorDoubleManager->value(members[i]);
      }
  }
private:
  FitPropertyBrowser* m_browser;
  QtProperty* m_prop;
};

/**
 * Visitor setting new attribute value. Depending on the attribute type
 * the appropriate apply method is used.
 */
class SetAttributeProperty: public Mantid::API::IFunction::ConstAttributeVisitor<>
{
public:
  SetAttributeProperty(FitPropertyBrowser* browser,QtProperty* prop)
    :m_browser(browser),m_prop(prop){}
protected:
  /// Set string property
  void apply(const std::string& str)const
  {
    m_browser->m_changeSlotsEnabled = false;
    QString attName = m_prop->propertyName();
    m_browser->setStringPropertyValue(m_prop,QString::fromStdString(str));
    m_browser->m_changeSlotsEnabled = true;
  }
  /// Set double property
  void apply(const double& d)const
  {
    m_browser->m_changeSlotsEnabled = false;
    m_browser->m_doubleManager->setValue(m_prop,d);
    m_browser->m_changeSlotsEnabled = true;
  }
  /// Set int property
  void apply(const int& i)const
  {
    m_browser->m_changeSlotsEnabled = false;
    m_browser->m_intManager->setValue(m_prop,i);
    m_browser->m_changeSlotsEnabled = true;
  }
  /// Set bool property
  void apply(const bool& b)const
  {
    m_browser->m_changeSlotsEnabled = false;
    m_browser->m_boolManager->setValue(m_prop,b);
    m_browser->m_changeSlotsEnabled = true;
  }
  /// Set vector property
  void apply(const std::vector<double>&)const
  {
      // this method is supposed to be called when corresponding
      // property value changes but it doesn't have a value because
      // it's a group property
      throw std::runtime_error("Vector attribute not implemented.");
  }
private:
  FitPropertyBrowser* m_browser;
  QtProperty* m_prop;
};

/**
* Set function attribute value read from a QtProperty
* @param prop :: The (string) property with the new attribute value
* @return true if successfull
*/
bool PropertyHandler::setAttribute(QtProperty* prop)
{
  if (m_attributes.contains(prop))
  {
    QString attName = prop->propertyName();
    try
    {
      Mantid::API::IFunction::Attribute att = 
        m_fun->getAttribute(attName.toStdString());
      SetAttribute tmp(m_browser,prop);
      att.apply(tmp);
      m_fun->setAttribute(attName.toStdString(),att);
      m_browser->compositeFunction()->checkFunction();
      initAttributes();
      initParameters();
      if (this == m_browser->m_autoBackground)
      {
        fit();
      }
    }
    catch(std::exception& e)
    {
      initParameters();
      QMessageBox::critical(m_browser,"Mantid - Error",e.what());
      return false;
    }
    return true;
  }
  if (m_cf)
  {
    for(size_t i=0;i<m_cf->nFunctions();i++)
    {
      bool res = getHandler(i)->setAttribute(prop);
      if (res) return true;
    }
  }
  return false;
}

void PropertyHandler::setAttribute(const QString& attName, const double& attValue)
{
  if (m_fun->hasAttribute(attName.toStdString()))
  {
    try
    {
      m_fun->setAttribute(attName.toStdString(),Mantid::API::IFunction::Attribute(attValue));
      m_browser->compositeFunction()->checkFunction();
      foreach(QtProperty* prop,m_attributes)
      {
        if (prop->propertyName() == attName)
        {
            // re-insert the attribute and parameter properties as they may
            // depend on the value of the attribute being set
            initAttributes();
            initParameters();
        }
      }
    }
    catch(...){}
  }
  if (cfun())
  {
    for(size_t i=0;i<cfun()->nFunctions();++i)
    {
      PropertyHandler* h = getHandler(i);
      h->setAttribute(attName,attValue);
    }
  }
}



void PropertyHandler::setAttribute(const QString& attName, const QString& attValue)
{
  const std::string name = attName.toStdString();
  if (m_fun->hasAttribute(name))
  {
    Mantid::API::IFunction::Attribute att = m_fun->getAttribute(name);
    att.fromString(attValue.toStdString());
    m_fun->setAttribute(name,att);
    m_browser->compositeFunction()->checkFunction();
    foreach(QtProperty* prop,m_attributes)
    {
      if (prop->propertyName() == attName)
      {
        SetAttributeProperty tmp(m_browser,prop);
        att.apply(tmp);
      }
    }
    // re-insert the attribute and parameter properties as they may
    // depend on the value of the attribute being set
    initAttributes();
    initParameters();
  }
}

/**
 * Set function vector attribute value
 * @param prop :: A property for a member of a vector attribute.
 */
void PropertyHandler::setVectorAttribute(QtProperty *prop)
{
    foreach (QtProperty *att, m_attributes)
    {
        QList<QtProperty*> subProps = att->subProperties();
        if ( subProps.contains(prop) )
        {
            setAttribute(att);
            return;
        }
    }
}

/**
 * Applies given function to all the parameter properties recursively, within this context.
 * @param func :: Function to apply
 */
void PropertyHandler::applyToAllParameters(void (PropertyHandler::*func)(QtProperty*))
{
  for(int i=0;i<m_parameters.size();i++)
  {
    QtProperty* prop = m_parameters[i];
    (this->*(func))(prop);
  }

  if (m_cf)
  {
    for(size_t i=0;i<m_cf->nFunctions();i++)
    {
      getHandler(i)->applyToAllParameters(func);
    }
  }
}

void PropertyHandler::updateParameters()
{
  applyToAllParameters(&PropertyHandler::updateParameter);
}

void PropertyHandler::updateErrors()
{
  applyToAllParameters(&PropertyHandler::updateError);
}

void PropertyHandler::clearErrors()
{
  applyToAllParameters(&PropertyHandler::clearError);
}

/**
 * @param prop :: Property of the parameter
 */
void PropertyHandler::updateParameter(QtProperty* prop)
{
  double parValue = function()->getParameter(prop->propertyName().toStdString());
  m_browser->m_parameterManager->setValue(prop, parValue);
}

/**
 * @param prop :: Property of the parameter
 */
void PropertyHandler::updateError(QtProperty* prop)
{
  size_t index = function()->parameterIndex(prop->propertyName().toStdString());
  double error = function()->getError(index);
  m_browser->m_parameterManager->setError(prop, error);
}

/**
 * @param prop :: Property of the parameter
 */
void PropertyHandler::clearError(QtProperty* prop)
{
  m_browser->m_parameterManager->clearError(prop);
}

/**
* Change the type of the function (replace the function)
* @param prop :: The "Type" property with new value
*/
Mantid::API::IFunction_sptr PropertyHandler::changeType(QtProperty* prop)
{
  if (prop == m_type)
  {
    //if (!m_parent) return m_browser->compositeFunction();// dont replace the root composite function

    // Create new function
    int i = m_browser->m_enumManager->value(prop);
    QStringList functionNames = m_browser->m_enumManager->enumNames(prop);
    const QString& fnName = functionNames[i];
    Mantid::API::IFunction_sptr f;
    try
    {
      f = Mantid::API::FunctionFactory::Instance().
        createFunction(fnName.toStdString());
    }
    catch(std::exception& e)
    {
      QMessageBox::critical(NULL,"Mantid - Error","Cannot create function "+fnName+
        "\n"+e.what());
      return Mantid::API::IFunction_sptr();
    }

    // turn of the change slots (doubleChanged() etc) to avoid infinite loop
    m_browser->m_changeSlotsEnabled = false;

    // Check if it's a peak and set its width
    Mantid::API::IPeakFunction* pf = dynamic_cast<Mantid::API::IPeakFunction*>(f.get());
    if (pf)
    {
      if (!m_pf)
      {
        if (!m_browser->workspaceName().empty() && m_browser->workspaceIndex() >= 0)
        {
          pf->setCentre( (m_browser->startX() + m_browser->endX())/2 );
        }
      }
      else
      {
        pf->setCentre(m_pf->centre());
        pf->setHeight(m_pf->height());
        pf->setFwhm(m_pf->fwhm());
      }
    }

    if (pf)
    {
      m_browser->setDefaultPeakType(fnName.toStdString());
    }
    else
    {
      m_browser->setDefaultBackgroundType(fnName.toStdString());
    }

    QList<QtProperty*> subs = m_item->property()->subProperties();
    foreach(QtProperty* sub, subs)
    {
      m_item->property()->removeSubProperty(sub);
    }

    m_browser->m_changeSlotsEnabled = true;

    emit m_browser->removePlotSignal(this);

    Mantid::API::IFunction_sptr f_old = function();
    PropertyHandler* h = new PropertyHandler(f,m_parent,m_browser,m_item);
    if (this == m_browser->m_autoBackground)
    {
      if (dynamic_cast<Mantid::API::IBackgroundFunction*>(f.get()))
      {
        m_browser->m_autoBackground = h;
        h->fit();
      }
      else
      {
        m_browser->m_autoBackground = NULL;
      }
    }
    if (m_parent)
    {
      m_parent->replaceFunctionPtr(f_old,f);
    }
    f->setHandler(h);
    // calculate the baseline
    if (h->pfun())
    {
      h->setCentre(h->centre()); // this sets m_ci
      h->calcBase();
    }
    // at this point this handler does not exist any more. only return is possible
    return f;

  }
  else if (m_cf)
  {
    for(size_t i=0;i<m_cf->nFunctions();i++)
    {
      Mantid::API::IFunction_sptr f = getHandler(i)->changeType(prop);
      if (f) return f;
    }
  }
  return Mantid::API::IFunction_sptr();
}

bool PropertyHandler::isParameter(QtProperty* prop)
{
  return m_parameters.contains(prop);
}

QtProperty* PropertyHandler::getParameterProperty(const QString& parName)const
{
  foreach(QtProperty* parProp,m_parameters)
  {
    if (parProp->propertyName() == parName)
    {
      return parProp;
    }
  }
  return NULL;
}

QtProperty* PropertyHandler::getParameterProperty(QtProperty* prop)const
{
  foreach(QtProperty* parProp,m_parameters)
  {
    QList<QtProperty*> subs = parProp->subProperties();
    if (subs.contains(prop))
    {
      return parProp;
    }
  }
  return NULL;
}

void PropertyHandler::addTie(const QString& tieStr)
{
  QStringList parts = tieStr.split("=");
  if (parts.size() != 2) return;
  std::string name = parts[0].trimmed().toStdString();
  std::string expr = parts[1].trimmed().toStdString();
  try
  {
    Mantid::API::ParameterTie* tie = 
      m_browser->compositeFunction()->tie(name,expr);
    if (tie == NULL) return;
    QString parName = QString::fromStdString(
      tie->getFunction()->parameterName(static_cast<int>(tie->getIndex()))
     );
    foreach(QtProperty* parProp,m_parameters)
    {
      if (parProp->propertyName() == parName)
      {
        m_browser->m_changeSlotsEnabled = false;
        QtProperty* tieProp = m_ties[parName];
        if (!tieProp)
        {
          tieProp = m_browser->m_stringManager->addProperty( "Tie" );
          m_ties[parName] = tieProp;
        }
        m_browser->m_stringManager->setValue(tieProp,QString::fromStdString(expr));
        m_browser->m_changeSlotsEnabled = true;
        parProp->addSubProperty(tieProp);
        return;
      }
    }
  }
  catch(...){}
}

void PropertyHandler::fix(const QString& parName)
{
  QtProperty* parProp = getParameterProperty(parName);
  if (!parProp) return;
  QString parValue = QString::number(m_browser->m_parameterManager->value(parProp));
  try
  {
    m_fun->tie(parName.toStdString(),parValue.toStdString());
    m_browser->m_changeSlotsEnabled = false;
    QtProperty* tieProp = m_ties[parName];
    if (!tieProp)
    {
      tieProp = m_browser->m_stringManager->addProperty( "Tie" );
      m_ties[parName] = tieProp;
    }
    m_browser->m_stringManager->setValue(tieProp,parValue);
    m_browser->m_changeSlotsEnabled = true;
    parProp->addSubProperty(tieProp);
    tieProp->setEnabled(false);
  }
  catch(...){}

}

/**
 * Remove the tie.
 * @param prop :: The tie property to remove
 */
void PropertyHandler::removeTie(QtProperty* prop)
{
  QString parName = m_ties.key(prop,"");
  if (parName.isEmpty()) return;
  
  QtProperty* parProp = getParameterProperty(parName);
  if (parProp)
  {
    m_browser->m_changeSlotsEnabled = false;
    m_fun->removeTie(parName.toStdString());
    parProp->removeSubProperty(prop);
    m_ties.remove(parName);
    m_browser->m_changeSlotsEnabled = true;
    parProp->setEnabled(true);
  }
}

/**
 * Remove the tie.
 * @param parName :: The name of the parameter
 */
void PropertyHandler::removeTie(const QString& parName)
{
  QtProperty* prop = m_ties[parName];
  if (prop) removeTie(prop);
}

/**
 * Calculate m_base: the baseline level under the peak (if this function is a peak and auto background is on)
 */
void PropertyHandler::calcBase()
{
  if (!m_browser->m_autoBackground) return;

  auto ws = boost::dynamic_pointer_cast<const Mantid::API::MatrixWorkspace>(m_browser->getWorkspace());
  if (ws)
  {
    size_t wi = m_browser->workspaceIndex();
    const Mantid::MantidVec& X = ws->readX(wi);
    const Mantid::MantidVec& Y = ws->readY(wi);
    int n = static_cast<int>(Y.size()) - 1;
    if (m_ci < 0 || m_ci > n || !m_browser->m_autoBackground)
    {
      m_base = 0.;
    }
    else
    {
      Mantid::API::FunctionDomain1DVector x(X[m_ci]);
      Mantid::API::FunctionValues y(x);
      m_browser->m_autoBackground->function()->function(x,y);
      m_base = y[0];
    }
  }
  else
  {
    m_base = 0.;
  }
}

/**
 * If the handled function is composite calculate the peak baselines for all members.
 * If auto background is off does nothing.
 */
void PropertyHandler::calcBaseAll()
{
  if (!m_browser->m_autoBackground) return;
  if (!m_cf) return;
  for(size_t i=0;i<m_cf->nFunctions();++i)
  {
    PropertyHandler* h = getHandler(i);
    if (h->pfun())
    {
      h->calcBase();
    }
    else if (h->cfun())
    {
      h->calcBaseAll();
    }
  }
}

/**
 * Set the height of the handled peak function.
 */
void PropertyHandler::setHeight(const double& h)
{
  if (m_pf)
  {
    m_pf->setHeight(h - m_base);
  }
}

/**
 * Set the centre of the handled peak function.
 * Find m_ci: x-index of the peakcentre.
 */
void PropertyHandler::setCentre(const double& c)
{
  if (m_pf)
  {
    m_pf->setCentre(c);

    //  find m_ci: x-index of the peakcentre
    auto ws = boost::dynamic_pointer_cast<const Mantid::API::MatrixWorkspace>(m_browser->getWorkspace());
    if (ws)
    {
      size_t wi = m_browser->workspaceIndex();
      const Mantid::MantidVec& X = ws->readX(wi);
      int n = static_cast<int>(X.size()) - 2;
      if (m_ci < 0) m_ci = 0;
      if (m_ci > n) m_ci = n;
      double x = X[m_ci];
      if (x < c)
      {
        for(;m_ci<=n;++m_ci)
        {
          x = X[m_ci];
          if (x > c) break;
        }
      }
      else
      {
        for(;m_ci>=0;--m_ci)
        {
          x = X[m_ci];
          if (x < c) break;
        }
      }
    }
  }
}

void PropertyHandler::setFwhm(const double& w)
{
  if (m_pf)
  {
    m_pf->setFwhm(w);
  }
}

double PropertyHandler::height()const
{
  if (m_pf)
  {
    return m_pf->height();
  }
  return 0;
}

double PropertyHandler::centre()const
{
  if (m_pf)
  {
    return m_pf->centre();
  }
  return (m_browser->endX() + m_browser->startX())/2;
}

double PropertyHandler::fwhm()const
{
  if (m_pf)
  {
    return m_pf->fwhm();
  }
  return 0;
}

/**
 * Add constraint to parameter property parProp
 */
void PropertyHandler::addConstraint(QtProperty* parProp,bool lo,bool up,double loBound,double upBound)
{
  QMap<QString,std::pair<QtProperty*,QtProperty*> >::iterator old = 
    m_constraints.find(parProp->propertyName());

  bool hasLo = false;
  bool hasUp = false;

  if (old != m_constraints.end())
  {
    hasLo = old.value().first != NULL;
    hasUp = old.value().second != NULL;
    if (hasLo && !lo)
    {
      lo = true;
      loBound = m_browser->m_doubleManager->value(old.value().first);
    }
    if (hasUp && !up)
    {
      up = true;
      upBound = m_browser->m_doubleManager->value(old.value().second);
    }
  }

  m_browser->m_changeSlotsEnabled = false;
  std::pair<QtProperty*,QtProperty*> cnew;//(nullptr,nullptr); - Can't do this in constructor in C++11
  // Don't know if these 2 lines are necessary, but this code is hard to understand - it could really use some comments!
  cnew.first = NULL;
  cnew.second = NULL;
  std::ostringstream ostr;
  if (lo) 
  {
    ostr << loBound << "<";
    if (!hasLo)
    {
      cnew.first = m_browser->addDoubleProperty("LowerBound");
      parProp->addSubProperty(cnew.first);
    }
    else
    {
      cnew.first = old.value().first;
    }
    m_browser->m_doubleManager->setValue(cnew.first,loBound);
  }
  ostr << parProp->propertyName().toStdString();
  if (up)
  {
    ostr << "<" << upBound;
    if (!hasUp)
    {
      cnew.second = m_browser->addDoubleProperty("UpperBound");
      parProp->addSubProperty(cnew.second);
    }
    else
    {
      cnew.second = old.value().second;
    }
    m_browser->m_doubleManager->setValue(cnew.second,upBound);
  }

  if (old != m_constraints.end())
  {
    m_constraints.erase(old);
  }

  m_constraints.insert(parProp->propertyName(),cnew);

  Mantid::API::IConstraint* c = 
    Mantid::API::ConstraintFactory::Instance().createInitialized(m_fun.get(),ostr.str());
  m_fun->addConstraint(c);
  m_browser->m_changeSlotsEnabled = true;
}

void PropertyHandler::removeConstraint(QtProperty* parProp)
{
  QMap<QString,std::pair<QtProperty*,QtProperty*> >::iterator it = 
    m_constraints.find(parProp->propertyName());

  if (it != m_constraints.end())
  {
    if (it.value().first)
    {
      parProp->removeSubProperty(it.value().first);
    }
    if (it.value().second)
    {
      parProp->removeSubProperty(it.value().second);
    }
    m_fun->removeConstraint(parProp->propertyName().toStdString());
    m_constraints.erase( it );
  }
}

/**
 * Make a list of all peaks in this function
 */
QList<PropertyHandler*> PropertyHandler::getPeakList()
{
  QList<PropertyHandler*> res;
  if (m_pf)
  {
    res << this;
  }
  if (m_cf)
  {
    for(size_t i=0;i<m_cf->nFunctions();++i)
    {
      PropertyHandler* h = getHandler(i);
      if ( !h )  continue;
      if (h->pfun())
      {
        res << h;
      }
      else if (h->cfun())
      {
        res << h->getPeakList();
      }
    }
  }
  return res;
}


/**
 * Remove the reference to the function curve as it has been deleted
 */
void PropertyHandler::plotRemoved()
{
  m_hasPlot = false;
}

/**
 * Updates the high-level structure tooltip of this handler's property, updating those of
 * sub-properties recursively first.
 *
 * For non-empty composite functions: something like ((Gaussian * Lorentzian) + FlatBackground)
 *
 * For non-composite functions: function()->name().
 *
 * @return The new tooltip
 */
QString PropertyHandler::updateStructureTooltip()
{
  QString newTooltip;

  if ( m_cf && (m_cf->name() == "CompositeFunction" || m_cf->name() == "ProductFunction") )
  {
    QStringList childrenTooltips;

    // Update tooltips for all the children first, and use them to build this tooltip
    for (size_t i = 0; i < m_cf->nFunctions(); ++i)
    {
      if (auto childHandler = getHandler(i))
      {
        childrenTooltips << childHandler->updateStructureTooltip();
      }
      else
      {
        throw std::runtime_error("Error while building structure tooltip: no handler for child");
      }
    }

    if ( childrenTooltips.empty() )
    {
      newTooltip = QString::fromStdString("Empty " + m_cf->name());
    }
    else
    {
      QChar op('+');

      if (m_cf->name() == "ProductFunction")
      {
        op = '*';
      }

      newTooltip = QString("(%1)").arg(childrenTooltips.join(' ' + op + ' '));
    }
  }
  else
  {
    newTooltip = QString::fromStdString(function()->name());
  }

  m_item->property()->setToolTip(newTooltip);
  return newTooltip;
}

/**
 * Remove all plots including children's
 */
void PropertyHandler::removeAllPlots()
{
  emit m_browser->removePlotSignal(this);
  if (m_cf)
  {
    for(size_t i=0;i<m_cf->nFunctions();++i)
    {
      getHandler(i)->removeAllPlots();
    }
  }
}

void PropertyHandler::fit()
{
  try
  {
    if (m_browser->workspaceName().empty()) return;

    Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("Fit");
    alg->initialize();
    alg->setProperty("Function",m_fun);
    alg->setPropertyValue("InputWorkspace",m_browser->workspaceName());
    alg->setProperty("WorkspaceIndex",m_browser->workspaceIndex());
    alg->setProperty("StartX",m_browser->startX());
    alg->setProperty("EndX",m_browser->endX());
    alg->execute();
    Mantid::API::IFunction_sptr f = alg->getProperty("Function");
    if (f != m_fun)
    {// this should never happen, just in case...
      for(size_t i=0;i<f->nParams();++i)
      {
        m_fun->setParameter(i,f->getParameter(i));
      }
    }
    m_browser->getHandler()->calcBaseAll();
    updateParameters();
  }
  catch(...)
  {
  }
}

void PropertyHandler::updateWorkspaces(QStringList oldWorkspaces)
{
  if (m_workspace)
  {
    int index = m_browser->m_enumManager->value(m_workspace) - 1;
    QString wsName;
    if (index >= 0 && index < oldWorkspaces.size())
    {
      wsName = oldWorkspaces[index];
    }
    QStringList names("All");
    foreach(QString name,m_browser->m_workspaceNames)
    {
      names.append(name);
    }
    m_browser->m_enumManager->setEnumNames(m_workspace, names);
    if (m_browser->m_workspaceNames.contains(wsName))
    {
      m_browser->m_enumManager->setValue(m_workspace,m_browser->m_workspaceNames.indexOf(wsName) + 1);
    }
  }
  if (cfun())
  {
    for(size_t i = 0; i < cfun()->nFunctions(); ++i)
    {
      getHandler(i)->updateWorkspaces(oldWorkspaces);
    }
  }
}

void PropertyHandler::setFunctionWorkspace()
{
  if (m_workspace)
  {
    int index = m_browser->m_enumManager->value(m_workspace) - 1;
    if (index >= 0 && index < m_browser->m_workspaceNames.size())
    {
      std::string wsName = m_browser->m_workspaceNames[index].toStdString();
      Mantid::API::Workspace_sptr ws = Mantid::API::AnalysisDataService::Instance().retrieve(wsName);
      int wsIndex = m_browser->m_intManager->value(m_workspaceIndex);
      auto mws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws);
      if (mws)
      {
        ifun()->setMatrixWorkspace(mws,size_t(wsIndex),m_browser->startX(),m_browser->endX());
      }
      else
      {
        ifun()->setWorkspace(ws);
      }
      m_item->property()->insertSubProperty(m_workspaceIndex,m_workspace);
    }
    else
    {
      ifun()->setWorkspace(Mantid::API::Workspace_sptr());
      m_item->property()->removeSubProperty(m_workspaceIndex);
    }
  }
  else
  {
    ifun()->setWorkspace(Mantid::API::Workspace_sptr());
  }
}

} // MantidQt
} // API
