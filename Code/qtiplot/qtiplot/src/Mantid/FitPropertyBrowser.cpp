#include "FitPropertyBrowser.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"

#include "qttreepropertybrowser.h"
#include "qtpropertymanager.h"
#include "qteditorfactory.h"

#include "../ApplicationWindow.h"
#include "MantidUI.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>


/**
 * Constructor
 * @param parent The parent widget - must be an ApplicationWindow
 */
FitPropertyBrowser::FitPropertyBrowser(QWidget* parent)
:QDockWidget("Fit Function",parent),m_function(0),m_defaultFunction("Gaussian"),m_appWindow((ApplicationWindow*)parent)
{
  setObjectName("FitFunction"); // this is needed for QMainWindow::restoreState()
  setMinimumHeight(150);
  setMinimumWidth(200);
  m_appWindow->addDockWidget( Qt::LeftDockWidgetArea, this );

  QWidget* w = new QWidget(parent);

    /* Create property managers: they create, own properties, get and set values  */

  m_groupManager =  new QtGroupPropertyManager(w);
  m_doubleManager = new QtDoublePropertyManager(w);
  m_stringManager = new QtStringPropertyManager(w);
  m_enumManager =   new QtEnumPropertyManager(w);
  m_intManager =    new QtIntPropertyManager(w);
  m_boolManager = new QtBoolPropertyManager(w);

  m_composite = m_boolManager->addProperty("IsComposite");
  connect(m_boolManager,SIGNAL(propertyChanged(QtProperty*)),this,SLOT(boolChanged(QtProperty*)));
  connect(m_intManager,SIGNAL(propertyChanged(QtProperty*)),this,SLOT(intChanged(QtProperty*)));
  connect(m_doubleManager,SIGNAL(propertyChanged(QtProperty*)),this,SLOT(doubleChanged(QtProperty*)));

    /* Create composite function group */

  m_compositeGroup = m_groupManager->addProperty("Composite Function");
  m_count = m_intManager->addProperty("Count");
  m_count->setEnabled(false);
  m_compositeGroup->addSubProperty(m_count);

  m_index = m_intManager->addProperty("Index");
  m_compositeGroup->addSubProperty(m_index);

    /* Create function group */

  m_functionGroup = m_groupManager->addProperty("Function");

  m_functionName = m_enumManager->addProperty("Name");
  m_functionGroup->addSubProperty(m_functionName);
  connect(m_enumManager,SIGNAL(propertyChanged(QtProperty*)),this,SLOT(functionChanged(QtProperty*)));
  
  m_peakGroup = m_groupManager->addProperty("Peak");

  m_height = m_doubleManager->addProperty("Height");
  m_peakGroup->addSubProperty(m_height);

  m_centre = m_doubleManager->addProperty("Centre");
  m_peakGroup->addSubProperty(m_centre);

  m_width = m_doubleManager->addProperty("Width");
  m_peakGroup->addSubProperty(m_width);


  m_parametersGroup = m_groupManager->addProperty("Parameters");
  m_functionGroup->addSubProperty(m_parametersGroup);

     /* Create input - output properties */

  m_workspace = m_enumManager->addProperty("Workspace");
  m_workspaceIndex = m_intManager->addProperty("Workspace Index");
  m_startX = m_doubleManager->addProperty("StartX");
  m_endX = m_doubleManager->addProperty("EndX");
  m_output = m_stringManager->addProperty("Output");

     /* Create editors and assign them to the managers */

  QtCheckBoxFactory *checkBoxFactory = new QtCheckBoxFactory(w);
  QtEnumEditorFactory *comboBoxFactory = new QtEnumEditorFactory(w);
  QtSpinBoxFactory *spinBoxFactory = new QtSpinBoxFactory(w);
  QtDoubleSpinBoxFactory *doubleSpinBoxFactory = new QtDoubleSpinBoxFactory(w);
  QtLineEditFactory *lineEditFactory = new QtLineEditFactory(w);

  m_browser = new QtTreePropertyBrowser();
  m_browser->setFactoryForManager(m_enumManager, comboBoxFactory);
  m_browser->setFactoryForManager(m_boolManager, checkBoxFactory);
  m_browser->setFactoryForManager(m_intManager, spinBoxFactory);
  m_browser->setFactoryForManager(m_doubleManager, doubleSpinBoxFactory);
  m_browser->setFactoryForManager(m_stringManager, lineEditFactory);

  m_browser->addProperty(m_composite);
  m_browser->addProperty(m_functionGroup);
  m_browser->addProperty(m_workspace);
  m_browser->addProperty(m_workspaceIndex);
  m_browser->addProperty(m_startX);
  m_browser->addProperty(m_endX);
  m_browser->addProperty(m_output);

  QVBoxLayout* layout = new QVBoxLayout(w);
  QHBoxLayout* buttonsLayout = new QHBoxLayout();
  QPushButton* btnFit = new QPushButton("Fit");
  connect(btnFit,SIGNAL(clicked()),this,SLOT(fit()));

  buttonsLayout->addWidget(btnFit);
  buttonsLayout->addStretch();

  layout->addLayout(buttonsLayout);
  layout->addWidget(m_browser);

  setWidget(w);


}

/** Set the current function
 * @param fun A pointer to the function which will become current
 */
void FitPropertyBrowser::setFunction(Mantid::API::IFunction* fun)
{
  try
  {
    Mantid::API::IPeakFunction* pf = dynamic_cast<Mantid::API::IPeakFunction*>(fun);
    m_function = fun;
    m_parameters.clear();
    QList<QtProperty*> props = m_parametersGroup->subProperties();
    for(int i=0;i<props.size();i++)
    {
      m_parametersGroup->removeSubProperty(props[i]);
    }
    for(int i=0;i<fun->nParams();i++)
    {
      std::string parName = fun->parameterName(i);
      double parValue = fun->parameter(i);
      QtProperty* prop = m_doubleManager->addProperty(QString::fromStdString(parName));
      m_doubleManager->setValue(prop,parValue);
      m_parameters.insert(parName,prop);
      m_parametersGroup->addSubProperty(prop);
    }
    if (pf)
    {
      setCentre(pf->centre());
      setHeight(pf->height());
      setWidth(pf->width());
      m_functionGroup->addSubProperty(m_peakGroup);
    }
    else
    {
      m_functionGroup->removeSubProperty(m_peakGroup);
    }
  }
  catch(...)
  {
    m_functionGroup->removeSubProperty(m_peakGroup);
  }
}

/**
 * Creates a new function. 
 * @param fnName A registered function name
 */
void FitPropertyBrowser::addFunction(const std::string& fnName)
{
    Mantid::API::IFunction* f = Mantid::API::FunctionFactory::Instance().createUnwrapped(fnName);
    f->initialize();
    if (isComposite())
    {
      m_compositeFunction->addFunction(f);
    }
    else if (m_function)
    {
      delete m_function;
    }
    setFunction(f);
}

/** Replace the current function with a new one
 * @param fnName A registered function name
 */
void FitPropertyBrowser::replaceFunction(const std::string& fnName)
{
  Mantid::API::IFunction* f = Mantid::API::FunctionFactory::Instance().createUnwrapped(fnName);
  f->initialize();
  if (isComposite())
  {
    Mantid::API::IPeakFunction* pf = dynamic_cast<Mantid::API::IPeakFunction*>(f);
    if (pf)
    {
      pf->setCentre(centre());
      pf->setHeight(height());
      pf->setWidth(width());
    }
    m_compositeFunction->replaceFunction(index(),f);
  }
  else if (m_function)
  {
    delete m_function;
  }
  setFunction(f);
}

/** Remove the current function
 */
void FitPropertyBrowser::removeFunction()
{
  if (isComposite() && count() > 1)
  {
    m_compositeFunction->removeFunction(index());
    setCount();
    m_function = m_compositeFunction->getFunction(index());
  }
  displayFunctionName();
  displayParameters();
  displayPeak();
  setFocus();
}


// Get the current function name
std::string FitPropertyBrowser::functionName()const
{
  if (!m_function)
  {
    return "";
  }
  return m_function->name();
}

/// Get the input workspace name
std::string FitPropertyBrowser::workspaceName()const
{
  int i = m_enumManager->value(m_workspace);
  std::string res = "";
  if (i >= 0)
  {
    res = m_workspaceNames[i].toStdString();
  }
  return res;
}

/// Get the output name
std::string FitPropertyBrowser::outputName()const
{
  return m_stringManager->value(m_output).toStdString();
}

/** Called when the function name property changed
 * @param prop A pointer to the function name property m_functionName
 */
void FitPropertyBrowser::functionChanged(QtProperty* prop)
{
  if (prop == m_functionName)
  {
    int i = m_enumManager->value(m_functionName);
    std::string fnName = m_registeredFunctions[i].toStdString();
    if (fnName == "<Delete>")
    {
      removeFunction();
    }
    else if (fnName != functionName())
    {
      if (isComposite())
      {
        replaceFunction(fnName);
      }
      else
      {
        addFunction(fnName);
      }
    }
  }
}

/** Called when a bool property changed
 * @param prop A pointer to the property 
 */
void FitPropertyBrowser::boolChanged(QtProperty* prop)
{
  if (prop == m_composite)
  {
    bool was_composite = isComposite();
    bool now_composite = m_boolManager->value(m_composite);
    if (now_composite && !was_composite)
    {
      createCompositeFunction();
    }
    else if (!now_composite && was_composite)
    {
      removeCompositeFunction();
    }
  }
}

/** Called when an int property changed
 * @param prop A pointer to the property 
 */
void FitPropertyBrowser::intChanged(QtProperty* prop)
{
  if (prop == m_index)
  {
    if (!isComposite()) return;
    int i = index();
    if (i == count())// add new function
    {
      addFunction(functionName());
      setCount();
    }
    else if (i >= 0 && i < count())
    {
      setFunction(m_compositeFunction->getFunction(i));
    }
    else if (i > count())
    {
      setIndex(count()-1);
    }
    else if (i < 0)
    {
      setIndex(0);
    }
    displayFunctionName();
  }
}

/** Called when a double property changed
 * @param prop A pointer to the property 
 */
void FitPropertyBrowser::doubleChanged(QtProperty* prop)
{
  if (!m_function) return;
  Mantid::API::IPeakFunction* pf = dynamic_cast<Mantid::API::IPeakFunction*>(m_function);
  double value = m_doubleManager->value(prop);
  if (pf && prop == m_centre)
  {
    pf->setCentre(value);
    displayParameters();
  }
  else if (pf && prop == m_height)
  {
    pf->setHeight(value);
    displayParameters();
  }
  else if (pf && prop == m_width)
  {
    pf->setWidth(value);
    displayParameters();
  }
  else
  {
    std::string parName = prop->propertyName().toStdString();
    m_function->getParameter(parName) = value;
    if (pf)
    {
      displayPeak();
    }
  }
}
// Centre of the current peak
double FitPropertyBrowser::centre()const
{
  return m_doubleManager->value(m_centre);
}

/** Set centre of the current peak
 * @param value The new centre value
 */
void FitPropertyBrowser::setCentre(double value)
{
  m_doubleManager->setValue(m_centre,value);
}

// Height of the current peak
double FitPropertyBrowser::height()const
{
  return m_doubleManager->value(m_height);
}

/** Set height of the current peak
 * @param value The new height value
 */
void FitPropertyBrowser::setHeight(double value)
{
  m_doubleManager->setValue(m_height,value);
}

// Width of the current peak
double FitPropertyBrowser::width()const
{
  return m_doubleManager->value(m_width);
}

/** Set width of the current peak
 * @param value The new width value
 */
void FitPropertyBrowser::setWidth(double value)
{
  m_doubleManager->setValue(m_width,value);
}

/// Get the registered function names
void FitPropertyBrowser::populateFunctionNames()
{
  const std::vector<std::string> names = Mantid::API::FunctionFactory::Instance().getKeys();
  m_registeredFunctions.clear();
  for(size_t i=0;i<names.size();i++)
  {
    if (names[i] != "CompositeFunction")
    {
      m_registeredFunctions << QString::fromStdString(names[i]);
    }
  }
  m_registeredFunctions.append("<Delete>");
  m_enumManager->setEnumNames(m_functionName, m_registeredFunctions);
  addFunction(m_defaultFunction);
  int j = m_registeredFunctions.indexOf(QString::fromStdString(m_defaultFunction));
  if (j >= 0)
  {
    m_enumManager->setValue(m_functionName,j);
  }
}

/// Is this function composite
bool FitPropertyBrowser::isComposite()const
{
  return m_compositeFunction.get() != 0;
}

/// Index of member function
int FitPropertyBrowser::functionIndex()const
{
  if (isComposite())
  {
    return m_intManager->value(m_index);
  }
  return -1;
}

/// Create CompositeFunction
void FitPropertyBrowser::createCompositeFunction()
{
  m_compositeFunction.reset(new Mantid::API::CompositeFunction());
  if (m_function)
  {
    m_compositeFunction->addFunction(m_function);
  }
  m_browser->insertProperty(m_compositeGroup,m_composite);
  displayFunctionName();
  setCount();
  setIndex(0);
}

/// Remove CompositeFunction
void FitPropertyBrowser::removeCompositeFunction()
{
  int i = functionIndex();
  if (m_function && i >= 0 && i < m_compositeFunction->nFunctions())
  {
    m_compositeFunction->removeFunction(i,false);
  }
  m_compositeFunction.reset();
  m_browser->removeProperty(m_compositeGroup);
  displayFunctionName();
  setCount();
  setIndex(0);
}

/// Makes sure m_functionName property shows the right function name
void FitPropertyBrowser::displayFunctionName()
{
  int i = m_registeredFunctions.indexOf(QString::fromStdString(functionName()));
  if (i >= 0)
  {
    m_enumManager->setValue(m_functionName,i);
  }
}

/// Get number of functions in CompositeFunction
int FitPropertyBrowser::count()const
{
  return m_intManager->value(m_count);
}

/** Set number of functions in CompositeFunction
 */
void FitPropertyBrowser::setCount()
{
  int n = 0;
  if (m_compositeFunction)
  {
    n = m_compositeFunction->nFunctions();
  }
  m_intManager->setValue(m_count,n);
  if (index() > n - 1) 
  {
    setIndex(n-1);
  }
}

/// Get index
int FitPropertyBrowser::index()const
{
  return m_intManager->value(m_index);
}

/** Set index of the current function in the CompositeFunction
 * @param i The new index
 */
void FitPropertyBrowser::setIndex(int i)
{
  m_intManager->setValue(m_index,i);
}

/// Makes sure the parameters are displayed correctly
void FitPropertyBrowser::displayParameters()
{
  QMap<std::string,QtProperty*>::iterator it=m_parameters.begin();
  for(;it!=m_parameters.end();it++)
  {
    try
    {
      double value = m_function->getParameter(it.key());
      m_doubleManager->setValue(it.value(),value);
    }
    catch(...)
    {
      // Parameters don't match the function
    }
  }
}

/// Makes sure the peak parameters (centre,height,width) are displayed correctly
void FitPropertyBrowser::displayPeak()
{
  Mantid::API::IPeakFunction* pf = dynamic_cast<Mantid::API::IPeakFunction*>(m_function);
  if (pf)
  {
    setCentre(pf->centre());
    setHeight(pf->height());
    setWidth(pf->width());
  }
}

/**
 * Creates an instance of Fit algorithm, sets its properties and launches it.
 */
void FitPropertyBrowser::fit()
{
  std::string wsName = workspaceName();
  if (wsName.empty())
  {
    m_appWindow->mantidUI->showCritical("Workspace name is not set");
    return;
  }
  try
  {
  Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("Fit");
  alg->initialize();
  alg->setPropertyValue("InputWorkspace",wsName);
  alg->setProperty("WorkspaceIndex",m_intManager->value(m_workspaceIndex));
  alg->setPropertyValue("Output",outputName());
  if (isComposite())
  {
    alg->setPropertyValue("Function",*m_compositeFunction);
  }
  else
  {
    alg->setPropertyValue("Function",*m_function);
  }
  alg->executeAsync();
  }
  catch(std::exception& e)
  {
    QString msg = "Fit algorithm failed.\n\n"+QString(e.what())+"\n";
    m_appWindow->mantidUI->showCritical(msg);
  }

}

/// Get and store available workspace names
void FitPropertyBrowser::populateWorkspaceNames()
{
  m_workspaceNames.clear();
  QStringList tmp = m_appWindow->mantidUI->getWorkspaceNames();
  for(int i=0;i<tmp.size();i++)
  {
    Mantid::API::Workspace_sptr ws = m_appWindow->mantidUI->getWorkspace(tmp[i]);
    if (isWorkspaceValid(ws))
    {
      m_workspaceNames.append(tmp[i]);
    }
  }
  m_enumManager->setEnumNames(m_workspace, m_workspaceNames);
}

void FitPropertyBrowser::workspace_added(const QString &wsName, Mantid::API::Workspace_sptr ws)
{
  if ( !isWorkspaceValid(ws) ) return;
  QString oldName = QString::fromStdString(workspaceName());
  int i = m_workspaceNames.indexOf(wsName);
  if (i < 0)
  {
    m_workspaceNames.append(wsName);
    m_workspaceNames.sort();
  }
  m_enumManager->setEnumNames(m_workspace, m_workspaceNames);
  i = m_workspaceNames.indexOf(oldName);
  if (i >= 0)
  {
    m_enumManager->setValue(m_workspace,i);
  }
}

void FitPropertyBrowser::workspace_removed(const QString &wsName)
{
  QString oldName = QString::fromStdString(workspaceName());
  int i = m_workspaceNames.indexOf(wsName);
  if (i >= 0)
  {
    m_workspaceNames.removeAt(i);
  }
  m_enumManager->setEnumNames(m_workspace, m_workspaceNames);
  i = m_workspaceNames.indexOf(oldName);
  if (i >= 0)
  {
    m_enumManager->setValue(m_workspace,i);
  }
}

void FitPropertyBrowser::init()
{
  populateFunctionNames();
  populateWorkspaceNames();
  connect(m_appWindow->mantidUI,SIGNAL(workspace_replaced(const QString &, Mantid::API::Workspace_sptr)),
    this,SLOT(workspace_added(const QString &, Mantid::API::Workspace_sptr)));
  connect(m_appWindow->mantidUI,SIGNAL(workspace_removed(const QString &)),
    this,SLOT(workspace_removed(const QString &)));
}

/** Check if the workspace can be used in the fit
 * @param ws The workspace
 */
bool FitPropertyBrowser::isWorkspaceValid(Mantid::API::Workspace_sptr ws)const
{
  return dynamic_cast<Mantid::API::MatrixWorkspace*>(ws.get()) != 0;
}
