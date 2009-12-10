#include "FitPropertyBrowser.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"

#include "qttreepropertybrowser.h"
#include "qtpropertymanager.h"
#include "qteditorfactory.h"

#include "../ApplicationWindow.h"
#include "MantidUI.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMenu>
#include <QInputDialog>

/**
 * Constructor
 * @param parent The parent widget - must be an ApplicationWindow
 */
FitPropertyBrowser::FitPropertyBrowser(QWidget* parent)
:QDockWidget("Fit Function",parent)/*,m_function(0)*/,m_defaultFunction("Gaussian"),m_default_width(0),
m_appWindow((ApplicationWindow*)parent),m_guessOutputName(true),m_changeSlotsEnabled(true)
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

    /* Create the top level group */

  QtProperty* fitGroup = m_groupManager->addProperty("Fit");

  connect(m_enumManager,SIGNAL(propertyChanged(QtProperty*)),this,SLOT(enumChanged(QtProperty*)));
  connect(m_boolManager,SIGNAL(propertyChanged(QtProperty*)),this,SLOT(boolChanged(QtProperty*)));
  connect(m_intManager,SIGNAL(propertyChanged(QtProperty*)),this,SLOT(intChanged(QtProperty*)));
  connect(m_doubleManager,SIGNAL(propertyChanged(QtProperty*)),this,SLOT(doubleChanged(QtProperty*)));
  connect(m_stringManager,SIGNAL(propertyChanged(QtProperty*)),this,SLOT(stringChanged(QtProperty*)));

    /* Create function group */

  m_functionsGroup = m_groupManager->addProperty("Functions");
  fitGroup->addSubProperty(m_functionsGroup);

     /* Create input - output properties */

  m_settingsGroup = m_groupManager->addProperty("Settings");
  fitGroup->addSubProperty(m_settingsGroup);

  m_workspace = m_enumManager->addProperty("Workspace");
  m_workspaceIndex = m_intManager->addProperty("Workspace Index");
  m_startX = m_doubleManager->addProperty("StartX");
  m_endX = m_doubleManager->addProperty("EndX");
  m_output = m_stringManager->addProperty("Output");

  m_settingsGroup->addSubProperty(m_workspace);
  m_settingsGroup->addSubProperty(m_workspaceIndex);
  m_settingsGroup->addSubProperty(m_startX);
  m_settingsGroup->addSubProperty(m_endX);
  m_settingsGroup->addSubProperty(m_output);

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

  m_fitGroup = m_browser->addProperty(fitGroup);

  QVBoxLayout* layout = new QVBoxLayout(w);
  QHBoxLayout* buttonsLayout = new QHBoxLayout();

  m_btnFit = new QPushButton("Fit");
  connect(m_btnFit,SIGNAL(clicked()),this,SLOT(fit()));

  m_btnUnFit = new QPushButton("Undo Fit");
  connect(m_btnUnFit,SIGNAL(clicked()),this,SLOT(undoFit()));

  QPushButton* btnClear = new QPushButton("Clear all");
  connect(btnClear,SIGNAL(clicked()),this,SLOT(clear()));

  buttonsLayout->addWidget(m_btnFit);
  buttonsLayout->addWidget(m_btnUnFit);
  buttonsLayout->addWidget(btnClear);
  buttonsLayout->addStretch();

  layout->addLayout(buttonsLayout);
  layout->addWidget(m_browser);

  setWidget(w);

  m_browser->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_browser, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(popupMenu(const QPoint &)));
  connect(m_browser, SIGNAL(currentItemChanged(QtBrowserItem*)), this, SLOT(currentItemChanged(QtBrowserItem*)));

  createCompositeFunction();

}

/// Destructor
FitPropertyBrowser::~FitPropertyBrowser()
{
  clear();
}

void FitPropertyBrowser::popupMenu(const QPoint &pos)
{
  QtBrowserItem * ci = m_browser->currentItem();
  QMenu *menu = new QMenu(m_appWindow);
  if (ci->property() == m_functionsGroup)
  {
    QAction *action = new QAction("Add new function",this);
    connect(action,SIGNAL(triggered()),this,SLOT(addFunction()));
    menu->addAction(action);
    menu->addSeparator();
  }
  else if (m_functionItems.contains(ci))
  {
    QAction *action = new QAction("Delete",this);
    connect(action,SIGNAL(triggered()),this,SLOT(deleteFunction()));
    menu->addAction(action);
    menu->addSeparator();
  }

  if (isFitEnabled())
  {
    QAction *action = new QAction("Fit",this);
    connect(action,SIGNAL(triggered()),this,SLOT(fit()));
    menu->addAction(action);
  }

  if (isUndoEnabled())
  {
    QAction *action = new QAction("Undo Fit",this);
    connect(action,SIGNAL(triggered()),this,SLOT(undoFit()));
    menu->addAction(action);
  }

  QAction *action = new QAction("Clear all",this);
  connect(action,SIGNAL(triggered()),this,SLOT(clear()));
  menu->addAction(action);

  menu->popup(QCursor::pos());
}

/**
 * Creates a new function. 
 * @param fnName A registered function name
 */
void FitPropertyBrowser::addFunction(const std::string& fnName)
{
  disableUndo();
  Mantid::API::IFunction* f = Mantid::API::FunctionFactory::Instance().createUnwrapped(fnName);
  f->initialize();
  Mantid::API::IPeakFunction* pf = dynamic_cast<Mantid::API::IPeakFunction*>(f);
  if (pf)
  {
    if (m_default_width != 0.0)
    {
      pf->setWidth(m_default_width);
    }
    else
    {
      m_default_width = pf->width();
    }
  }
  m_compositeFunction->addFunction(f);
  setIndex(m_compositeFunction->nFunctions()-1);
  m_defaultFunction = fnName;
  setFocus();

  m_changeSlotsEnabled = false;

  // Add a group property named after the function: f<index>-<type>, where <type> is the function's class name
  QtProperty* fnProp = m_groupManager->addProperty(functionName(index()));
  m_functionsGroup->addSubProperty(fnProp);

  QtProperty* typeProp = m_enumManager->addProperty("Type");
  fnProp->addSubProperty(typeProp);
  int itype = m_registeredFunctions.indexOf(QString::fromStdString(functionType()));
  m_enumManager->setEnumNames(typeProp, m_registeredFunctions);
  m_enumManager->setValue(typeProp,itype);

  // Add properties for the function's parameters
  for(int i=0;i<f->nParams();i++)
  {
    QtProperty* parProp = m_doubleManager->addProperty(QString::fromStdString(f->parameterName(i)));
    fnProp->addSubProperty(parProp);
    m_doubleManager->setValue(parProp,f->parameter(i));
  }

  QtBrowserItem* fnGroupItem = findItem(m_fitGroup,m_functionsGroup);
  QtBrowserItem* fnItem = findItem(fnGroupItem,fnProp);
  m_browser->setExpanded(fnItem,false);
  m_functionItems.append(fnItem);
  selectFunction(index());

  m_changeSlotsEnabled = true;

  setFitEnabled(true);
}

/** Replace the current function with a new one
 * @param i The function index
 * @param fnName A registered function name
 */
void FitPropertyBrowser::replaceFunction(int i,const std::string& fnName)
{
  if (i < 0 || i >= count()) return;
  disableUndo();
  Mantid::API::IFunction* f = Mantid::API::FunctionFactory::Instance().createUnwrapped(fnName);
  f->initialize();
  Mantid::API::IPeakFunction* pf = dynamic_cast<Mantid::API::IPeakFunction*>(f);
  if (pf && peakFunction(i))
  {
    pf->setCentre(peakFunction(i)->centre());
    pf->setHeight(peakFunction(i)->height());
    pf->setWidth(peakFunction(i)->width());
  }
  m_compositeFunction->replaceFunction(i,f);
  QtBrowserItem* fnItem = m_functionItems[i];
  fnItem->property()->setPropertyName(functionName(i));
  QList<QtProperty*> subs = fnItem->property()->subProperties();
  
  if (subs.size()-1 > f->nParams())
  {
    for(int j=f->nParams()+1;j<subs.size();j++)
    {
      fnItem->property()->removeSubProperty(subs[j]);
      delete subs[j];
    }
  }
  else if (subs.size()-1 < f->nParams())
  {
    for(int j=subs.size()-1;j<f->nParams();j++)
    {
      QtProperty* parProp = m_doubleManager->addProperty(QString::fromStdString(f->parameterName(j)));
      fnItem->property()->addSubProperty(parProp);
    }
  }
  subs = fnItem->property()->subProperties();
  for(int j=0;j<f->nParams();j++)
  {
    subs[j+1]->setPropertyName(QString::fromStdString(f->parameterName(j)));
    m_doubleManager->setValue(subs[j+1],f->parameter(j));
  }

}

/** Remove a function
 * @param i The function index
 */
void FitPropertyBrowser::removeFunction(int i)
{
  if (i < 0 || i >= count()) return;
  disableUndo();
  QtBrowserItem* fnItem = m_functionItems[i];
  QList<QtProperty*> subs = fnItem->property()->subProperties();
  for(int j=0;j<subs.size();j++)
  {
    fnItem->property()->removeSubProperty(subs[i]);
    delete subs[j];
  }
  QtProperty* fnGroup = fnItem->parent()->property();
  fnGroup->removeSubProperty(fnItem->property());
  QList<QtProperty*> fns = fnGroup->subProperties();
  for(int j = 0;j<fns.size();j++)
  {
    fns[j]->setPropertyName(functionName(j));
  }
  m_compositeFunction->removeFunction(i);
  setIndex(index());
  setFocus();
  m_functionItems.removeAt(i);
  if (count() == 0)
  {
    setFitEnabled(false);
  }
  emit functionRemoved(i);
}


/** Get function type
 * @param i The function index
 */
std::string FitPropertyBrowser::functionType(int i)const
{
  Mantid::API::IFunction* f = function(i);
  return f!=0?f->name():m_defaultFunction;
}

/** Get function name
 * @param i The function index
 */
QString FitPropertyBrowser::functionName(int i)const
{
  if (count() <= 0 || i >= count()) return "";
  QString qname = "f" + QString::number(i)+ "-"+QString::fromStdString(functionType(i));
  return qname;
}

// Get the default function name
std::string FitPropertyBrowser::defaultFunctionType()const
{
  return m_defaultFunction;
}

// Get the default function name
void FitPropertyBrowser::setDefaultFunctionType(const std::string& fnType)
{
  m_defaultFunction = fnType;
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

/// Set the input workspace name
void FitPropertyBrowser::setWorkspaceName(const QString& wsName)
{
  int i = m_workspaceNames.indexOf(wsName);
  if (i >= 0)
  {
    m_enumManager->setValue(m_workspace,i);
  }
}

/// Get workspace index
int FitPropertyBrowser::workspaceIndex()const
{
  return m_intManager->value(m_workspaceIndex);
}

/// Set workspace index
void FitPropertyBrowser::setWorkspaceIndex(int i)
{
  m_intManager->setValue(m_workspaceIndex,i);
}

/// Get the output name
std::string FitPropertyBrowser::outputName()const
{
  return m_stringManager->value(m_output).toStdString();
}

/// Get the output name
void FitPropertyBrowser::setOutputName(const std::string& name)
{
  m_stringManager->setValue(m_output,QString::fromStdString(name));
}

/** Called when the function name property changed
 * @param prop A pointer to the function name property m_functionName
 */
void FitPropertyBrowser::enumChanged(QtProperty* prop)
{
  if ( ! m_changeSlotsEnabled ) return;

  if (prop == m_workspace)
  {
    if (m_guessOutputName)
    {
      m_stringManager->setValue(m_output,QString::fromStdString(workspaceName()));
    }
    emit workspaceNameChanged(QString::fromStdString(workspaceName()));
  }
  else if (prop->propertyName() == "Type")
  {
    for(int i=0;i<count();i++)
    {
      if (findItem(m_functionItems[i],prop))
      {
        int j = m_enumManager->value(prop);
        QString fnName = m_registeredFunctions[j];
        replaceFunction(i,fnName.toStdString());
        break;
      }
    }
  }
}

/** Called when a bool property changed
 * @param prop A pointer to the property 
 */
void FitPropertyBrowser::boolChanged(QtProperty* prop)
{
  if ( ! m_changeSlotsEnabled ) return;

}

/** Called when an int property changed
 * @param prop A pointer to the property 
 */
void FitPropertyBrowser::intChanged(QtProperty* prop)
{
  if ( ! m_changeSlotsEnabled ) return;

  if (prop == m_workspaceIndex)
  {
    Mantid::API::MatrixWorkspace_sptr ws = 
      boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
      m_appWindow->mantidUI->getWorkspace(QString::fromStdString(workspaceName()))
      );
    if (!ws)
    {
      setWorkspaceIndex(0);
      return;
    }
    int n = ws->getNumberHistograms();
    int wi = workspaceIndex();
    if (wi < 0)
    {
      setWorkspaceIndex(0);
    }
    else if (wi >= n)
    {
      setWorkspaceIndex(n-1);
    }
    emit workspaceIndexChanged(wi);
  }
}

/** Called when a double property changed
 * @param prop A pointer to the property 
 */
void FitPropertyBrowser::doubleChanged(QtProperty* prop)
{
  if ( ! m_changeSlotsEnabled ) return;

  double value = m_doubleManager->value(prop);
  if (prop == m_startX )
  {
    emit startXChanged(startX());
  }
  else if (prop == m_endX )
  {
    emit endXChanged(endX());
  }
  else if(m_functionItems.size() > 0)
  {
    for(int i=0;i<m_functionItems.size();i++)
    {
      QtBrowserItem* fnItem = m_functionItems[i];
      QList<QtProperty*> subs = fnItem->property()->subProperties();
      Mantid::API::IFunction* f = m_compositeFunction->getFunction(i);
      bool done = false;
      for(int j=1;j<subs.size();j++)
      {
        if (subs[j] == prop)
        {
          f->parameter(j-1) = value;
          done = true;
          break;
        }
      }
      if (done) break;
    }
    emit parameterChanged();
  }
}
/** Called when a string property changed
 * @param prop A pointer to the property 
 */
void FitPropertyBrowser::stringChanged(QtProperty* prop)
{
  if ( ! m_changeSlotsEnabled ) return;

  if (prop == m_output)
  {
    std::string oName = outputName();
    if (oName.find_first_not_of(' ') == std::string::npos)
    {
      setOutputName("");
    }
    else if (workspaceName() == oName || oName.empty())
    {
      m_guessOutputName = true;
    }
    else
    {
      m_guessOutputName = false;
    }
  }
}

// Centre of the current peak
double FitPropertyBrowser::centre()const
{
  if (isPeak())
  {
    return peakFunction()->centre();
  }
  return 0;
}

/** Set centre of the current peak
 * @param value The new centre value
 */
void FitPropertyBrowser::setCentre(double value)
{
  if (isPeak())
  {
    peakFunction()->setCentre(value);
  }
}

// Height of the current peak
double FitPropertyBrowser::height()const
{
  if (isPeak())
  {
    return peakFunction()->height();
  }
  return 0.;
}

/** Set height of the current peak
 * @param value The new height value
 */
void FitPropertyBrowser::setHeight(double value)
{
  if (isPeak())
  {
    peakFunction()->setHeight(value);
  }
}

// Width of the current peak
double FitPropertyBrowser::width()const
{
  if (isPeak())
  {
    return peakFunction()->width();
  }
  return 0;
}

/** Set width of the current peak
 * @param value The new width value
 */
void FitPropertyBrowser::setWidth(double value)
{
  if (isPeak())
  {
    peakFunction()->setWidth(value);
  }
}

/// Get the registered function names
void FitPropertyBrowser::populateFunctionNames()
{
  const std::vector<std::string> names = Mantid::API::FunctionFactory::Instance().getKeys();
  m_registeredFunctions.clear();
  for(size_t i=0;i<names.size();i++)
  {
    std::string fnName = names[i];
    if (fnName != "CompositeFunction")
    {
      QString qfnName = QString::fromStdString(fnName);
      m_registeredFunctions << qfnName;
      boost::shared_ptr<Mantid::API::IFunction> f = Mantid::API::FunctionFactory::Instance().create(fnName);
      f->initialize();
      Mantid::API::IPeakFunction* pf = dynamic_cast<Mantid::API::IPeakFunction*>(f.get());
      if (pf)
      {
        m_registeredPeaks << qfnName;
      }
      else
      {
        m_registeredBackgrounds << qfnName;
      }
    }
  }
}

/// Create CompositeFunction
void FitPropertyBrowser::createCompositeFunction()
{
  m_compositeFunction.reset(new Mantid::API::CompositeFunction());
  disableUndo();
  setFitEnabled(false);
}

/// Get number of functions in CompositeFunction
int FitPropertyBrowser::count()const
{
  return m_compositeFunction->nFunctions();
}

/// Get index
int FitPropertyBrowser::index()const
{
  return m_index;
}

/** Set index of the current function in the CompositeFunction
 * @param i The new index
 */
void FitPropertyBrowser::setIndex(int i)const
{
  if (count() == 0)
  {
    m_index = -1;
    return;
  }
  if (i < 0 || i >= count()) return;
  m_index = i;
  emit indexChanged(i);
}

/**
 * Select i-th function item in the browser
 * int i The function index
 */
void FitPropertyBrowser::selectFunction(int i)const
{
  setIndex(i);
  m_browser->setCurrentItem(m_functionItems[index()]);
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
    m_initialParameters.resize(compositeFunction()->nParams());
    for(int i=0;i<compositeFunction()->nParams();i++)
    {
      m_initialParameters[i] = compositeFunction()->parameter(i);
    }
    m_btnUnFit->setEnabled(true);

    Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("Fit");
    alg->initialize();
    alg->setPropertyValue("InputWorkspace",wsName);
    alg->setProperty("WorkspaceIndex",workspaceIndex());
    alg->setProperty("StartX",startX());
    alg->setProperty("EndX",endX());
    alg->setPropertyValue("Output",outputName());
    alg->setPropertyValue("Function",*m_compositeFunction);
    observeFinish(alg);
    alg->executeAsync();
  }
  catch(std::exception& e)
  {
    QString msg = "Fit algorithm failed.\n\n"+QString(e.what())+"\n";
    m_appWindow->mantidUI->showCritical(msg);
  }

}

void FitPropertyBrowser::finishHandle(const Mantid::API::IAlgorithm* alg)
{
  std::string out = alg->getProperty("OutputWorkspace");
  getFitResults();
  emit algorithmFinished(QString::fromStdString(out));
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

void FitPropertyBrowser::reinit()
{
}

/** Check if the workspace can be used in the fit
 * @param ws The workspace
 */
bool FitPropertyBrowser::isWorkspaceValid(Mantid::API::Workspace_sptr ws)const
{
  return dynamic_cast<Mantid::API::MatrixWorkspace*>(ws.get()) != 0;
}

/// Is the current function a peak?
bool FitPropertyBrowser::isPeak()const
{
  if (count() == 0)
  {
    return false;
  }
  return peakFunction() != 0;
}

/// Get the start X
double FitPropertyBrowser::startX()const
{
  return m_doubleManager->value(m_startX);
}

/// Set the start X
void FitPropertyBrowser::setStartX(double value)
{
  m_doubleManager->setValue(m_startX,value);
}

/// Get the end X
double FitPropertyBrowser::endX()const
{
  return m_doubleManager->value(m_endX);
}

/// Set the end X
void FitPropertyBrowser::setEndX(double value)
{
  m_doubleManager->setValue(m_endX,value);
}

/** Get a function 
 * @param i The function index
 */
Mantid::API::IFunction* FitPropertyBrowser::function(int i)const
{
  if (count() <= 0 || i < 0 || i >= count()) return 0;
  return m_compositeFunction->getFunction(i);
}

/** Get a function if it's a peak
 * @param i The function index
 */
Mantid::API::IPeakFunction* FitPropertyBrowser::peakFunction(int i)const
{
  if (count() <= 0 || i < 0 || i >= count()) return 0;
  return dynamic_cast<Mantid::API::IPeakFunction*>(function(i));
}

/** Slot. Called to add a new function
 */
void FitPropertyBrowser::addFunction()
{
  int i = m_registeredFunctions.indexOf(QString::fromStdString(m_defaultFunction));
  bool ok = false;
  QString fnName = 
    QInputDialog::getItem(this, "MantidPlot - Fit", "Select function type", m_registeredFunctions,i,false,&ok);
  if (ok)
  {
    addFunction(fnName.toStdString());
  }
}

/** Slot. Called to remove a function
 */
void FitPropertyBrowser::deleteFunction()
{
  int i = index();
  removeFunction(i);
}

QtBrowserItem* FitPropertyBrowser::findItem(QtBrowserItem* parent,QtProperty* prop)const
{
  QList<QtBrowserItem*> children = parent->children();
  QtBrowserItem* res = 0;
  for(int i=0;i<children.size();i++)
  {
    if (children[i]->property() == prop)
    {
      res = children[i];
    }
  }
  return res;
}

/**
 * Slot. Responds to changing the current item
 */
void FitPropertyBrowser::currentItemChanged(QtBrowserItem * current )
{
  int i = m_functionItems.indexOf(current);
  if (i >= 0)
  {
    setIndex(i);
  }
}

/// Update the function parameter properties
void FitPropertyBrowser::updateParameters()
{
  for(int i=0;i<m_functionItems.size();i++)
  {
    QtBrowserItem* fnItem = m_functionItems[i];
    QList<QtProperty*> subs = fnItem->property()->subProperties();
    Mantid::API::IFunction* f = m_compositeFunction->getFunction(i);
    for(int j=1;j<subs.size();j++)
    {
      double v = f->parameter(j-1);
      m_doubleManager->setValue(subs[j],v);
    }
  }
}

/**
 * Slot. Removes all functions.
 */
void FitPropertyBrowser::clear()
{
  for(int i=0;i<m_functionItems.size();i++)
  {
    QtBrowserItem* fnItem = m_functionItems[i];
    QtProperty* fnProp = fnItem->property();
    QList<QtProperty*> subs = fnProp->subProperties();
    Mantid::API::IFunction* f = m_compositeFunction->getFunction(i);
    for(int j=0;j<subs.size();j++)
    {
      fnProp->removeSubProperty(subs[j]);
      delete subs[j];
    }
    m_functionsGroup->removeSubProperty(fnProp);
    delete fnProp;
  }
  m_functionItems.clear();
  createCompositeFunction();
  emit functionCleared();
}

/// Set the parameters to the fit outcome
void FitPropertyBrowser::getFitResults()
{
  std::string wsName = outputName() + "_Parameters";
  Mantid::API::ITableWorkspace_sptr ws = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(
    Mantid::API::AnalysisDataService::Instance().retrieve(wsName) );

  if (ws)
  {
    try
    {
      Mantid::API::TableRow row = ws->getFirstRow();
      do
      {
        std::string name;
        double value;
        row >> name >> value;
        // In case of a single function Fit doesn't create a CompositeFunction
        if (count() == 1)
        {
          name.insert(0,"f0.");
        }
        compositeFunction()->getParameter(name) = value;
      }
      while(row.next());
      updateParameters();
    }
    catch(...)
    {
      // do nothing
    }
  }
}

/**
 * Slot. Undoes the fit: restores the parameters to their initial values.
 */
void FitPropertyBrowser::undoFit()
{
  if (m_initialParameters.size() == compositeFunction()->nParams())
  {
    for(int i=0;i<compositeFunction()->nParams();i++)
    {
      compositeFunction()->parameter(i) = m_initialParameters[i];
    }
    updateParameters();
  }
  disableUndo();
}

/// disable undo when the function changes
void FitPropertyBrowser::disableUndo()
{
  m_initialParameters.clear();
  m_btnUnFit->setEnabled(false);
}

/// Tells if undo can be done
bool FitPropertyBrowser::isUndoEnabled()const
{
  return m_initialParameters.size() && compositeFunction()->nParams() == m_initialParameters.size();
}

/// Enable/disable the Fit button;
void FitPropertyBrowser::setFitEnabled(bool yes)
{
  m_btnFit->setEnabled(yes);
}

/// Returns true if the function is ready for a fit
bool FitPropertyBrowser::isFitEnabled()const
{
  return m_btnFit->isEnabled();
}
