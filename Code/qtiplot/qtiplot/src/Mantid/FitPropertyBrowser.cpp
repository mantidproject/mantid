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
#include <QMessageBox>
#include <QInputDialog>

/**
 * Constructor
 * @param parent The parent widget - must be an ApplicationWindow
 */
FitPropertyBrowser::FitPropertyBrowser(QWidget* parent)
:QDockWidget("Fit Function",parent)/*,m_function(0)*/,m_defaultFunction("Gaussian"),m_default_width(0),
m_guessOutputName(true),m_changeSlotsEnabled(true),m_peakToolOn(false),m_appWindow((ApplicationWindow*)parent)
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

  QtProperty* functionsGroup = m_groupManager->addProperty("Functions");

     /* Create input - output properties */

  QtProperty* settingsGroup = m_groupManager->addProperty("Settings");

  m_workspace = m_enumManager->addProperty("Workspace");
  m_workspaceIndex = m_intManager->addProperty("Workspace Index");
  m_startX = m_doubleManager->addProperty("StartX");
  m_endX = m_doubleManager->addProperty("EndX");
  m_output = m_stringManager->addProperty("Output");
  m_minimizer = m_enumManager->addProperty("Minimizer");

  m_minimizers << "Levenberg-Marquardt"
               << "Simplex"
               << "Conjugate gradient (Fletcher-Reeves imp.)"
               << "Conjugate gradient (Polak-Ribiere imp.)"
               << "BFGS";
  m_enumManager->setEnumNames(m_minimizer, m_minimizers);

  settingsGroup->addSubProperty(m_workspace);
  settingsGroup->addSubProperty(m_workspaceIndex);
  settingsGroup->addSubProperty(m_startX);
  settingsGroup->addSubProperty(m_endX);
  settingsGroup->addSubProperty(m_output);
  settingsGroup->addSubProperty(m_minimizer);

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

  m_functionsGroup = m_browser->addProperty(functionsGroup);
  m_settingsGroup = m_browser->addProperty(settingsGroup);

  QVBoxLayout* layout = new QVBoxLayout(w);
  QHBoxLayout* buttonsLayout = new QHBoxLayout();

  m_btnFit = new QPushButton("Fit");
  connect(m_btnFit,SIGNAL(clicked()),this,SLOT(fit()));

  m_btnUnFit = new QPushButton("Undo Fit");
  connect(m_btnUnFit,SIGNAL(clicked()),this,SLOT(undoFit()));

  QPushButton* btnClear = new QPushButton("Clear all");
  connect(btnClear,SIGNAL(clicked()),this,SLOT(clear()));

  m_tip = new QLabel("",w);

  buttonsLayout->addWidget(m_btnFit);
  buttonsLayout->addWidget(m_btnUnFit);
  buttonsLayout->addWidget(btnClear);
  buttonsLayout->addStretch();

  layout->addLayout(buttonsLayout);
  layout->addWidget(m_tip);
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
  QAction *action;

  bool isFunctionsGroup = ci == m_functionsGroup;
  bool isSettingsGroup = ci == m_settingsGroup;
  bool isASetting = ci->parent() == m_settingsGroup;
  bool isFunction = m_functionItems.contains(ci);

  if (isFunctionsGroup)
  {
    action = new QAction("Add function",this);
    connect(action,SIGNAL(triggered()),this,SLOT(addFunction()));
    menu->addAction(action);
    menu->addSeparator();
  }
  else if (isFunctionsGroup || isSettingsGroup || isASetting)
  {
    if (isFitEnabled())
    {
      action = new QAction("Fit",this);
      connect(action,SIGNAL(triggered()),this,SLOT(fit()));
      menu->addAction(action);
    }

    if (isUndoEnabled())
    {
      action = new QAction("Undo Fit",this);
      connect(action,SIGNAL(triggered()),this,SLOT(undoFit()));
      menu->addAction(action);
    }

    action = new QAction("Clear all",this);
    connect(action,SIGNAL(triggered()),this,SLOT(clear()));
    menu->addAction(action);

  }
  else if (isFunction)
  {
    action = new QAction("Remove",this);
    connect(action,SIGNAL(triggered()),this,SLOT(deleteFunction()));
    menu->addAction(action);

    if (m_peakToolOn)
    {
      action = new QAction("Plot",this);
      connect(action,SIGNAL(triggered()),this,SLOT(plotGuessCurrent()));
      menu->addAction(action);
    }

    menu->addSeparator();
  }
  else
  {

    bool isParameter = m_functionItems.contains(ci->parent());
    bool isTie = !isParameter && ci->property()->propertyName() == "Tie";
    bool isLowerBound = !isParameter && ci->property()->propertyName() == "Lower Bound";
    bool isUpperBound = !isParameter && ci->property()->propertyName() == "Upper Bound";
    bool isType = isParameter && ci->property()->propertyName() == "Type";
    if (isType)
    {
      isParameter = false;
    }

    if (isTie)
    {
      //menu->addSeparator();
      action = new QAction("Remove",this);
      connect(action,SIGNAL(triggered()),this,SLOT(deleteTie()));
      menu->addAction(action);
    }
    else if (isLowerBound)
    {
      action = new QAction("Remove",this);
      connect(action,SIGNAL(triggered()),this,SLOT(removeLowerBound()));
      menu->addAction(action);
    }
    else if (isUpperBound)
    {
      action = new QAction("Remove",this);
      connect(action,SIGNAL(triggered()),this,SLOT(removeUpperBound()));
      menu->addAction(action);
    }
    else if (count() > 0 && isParameter)
    {
      bool noTies =  !hasTie(ci->property());
      bool hasLower = false;
      bool hasUpper = false;

      QMap<QtProperty*,std::pair<QtProperty*,QtProperty*> >::iterator c = m_constraints.find(ci->property());
      if (c != m_constraints.end())
      {
        hasLower = c.value().first != 0;
        hasUpper = c.value().second != 0;
      }
      bool hasBounds = hasLower || hasUpper;

      if (noTies && !hasBounds)
      {
        action = new QAction("Fix",this);
        connect(action,SIGNAL(triggered()),this,SLOT(addFixTie()));
        menu->addAction(action);
      }

      if (noTies && (!hasLower || !hasUpper))
      {
        QMenu *constraintMenu = menu->addMenu("Constraint");

        if (!hasLower)
        {
          QMenu* detailMenu = constraintMenu->addMenu("Lower Bound");

          action = new QAction("10%",this);
          connect(action,SIGNAL(triggered()),this,SLOT(addLowerBound10()));
          detailMenu->addAction(action);

          action = new QAction("50%",this);
          connect(action,SIGNAL(triggered()),this,SLOT(addLowerBound50()));
          detailMenu->addAction(action);

          action = new QAction("Custom",this);
          connect(action,SIGNAL(triggered()),this,SLOT(addLowerBound()));
          detailMenu->addAction(action);
        }

        if (!hasUpper)
        {
          QMenu* detailMenu = constraintMenu->addMenu("Upper Bound");

          action = new QAction("10%",this);
          connect(action,SIGNAL(triggered()),this,SLOT(addUpperBound10()));
          detailMenu->addAction(action);

          action = new QAction("50%",this);
          connect(action,SIGNAL(triggered()),this,SLOT(addUpperBound50()));
          detailMenu->addAction(action);

          action = new QAction("Custom",this);
          connect(action,SIGNAL(triggered()),this,SLOT(addUpperBound()));
          detailMenu->addAction(action);
        }

        if (!hasLower && !hasUpper)
        {
          QMenu* detailMenu = constraintMenu->addMenu("Both Bounds");

          action = new QAction("10%",this);
          connect(action,SIGNAL(triggered()),this,SLOT(addBothBounds10()));
          detailMenu->addAction(action);

          action = new QAction("50%",this);
          connect(action,SIGNAL(triggered()),this,SLOT(addBothBounds50()));
          detailMenu->addAction(action);

          action = new QAction("Custom",this);
          connect(action,SIGNAL(triggered()),this,SLOT(addBothBounds()));
          detailMenu->addAction(action);
        }
      }

      if (hasBounds)
      {
        action = new QAction("Remove constraints",this);
        connect(action,SIGNAL(triggered()),this,SLOT(removeBounds()));
        menu->addAction(action);
      }

      if (noTies && !hasBounds)
      {
        if (count() == 1)
        {
          action = new QAction("Tie",this);
          connect(action,SIGNAL(triggered()),this,SLOT(addTie()));
          menu->addAction(action);
        }
        else
        {
          QMenu* detail = menu->addMenu("Tie");

          action = new QAction("To function",this);
          connect(action,SIGNAL(triggered()),this,SLOT(addTieToFunction()));
          detail->addAction(action);

          action = new QAction("Custom Tie",this);
          connect(action,SIGNAL(triggered()),this,SLOT(addTie()));
          detail->addAction(action);
        }
      }
      else if (!noTies)
      {
        action = new QAction("Remove tie",this);
        connect(action,SIGNAL(triggered()),this,SLOT(deleteTie()));
        menu->addAction(action);
      }
    }
  }

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
  m_functionsGroup->property()->addSubProperty(fnProp);

  QtBrowserItem* fnItem = findItem(m_functionsGroup,fnProp);
  m_browser->setExpanded(fnItem,false);
  m_functionItems.append(fnItem);
  selectFunction(index());

  addFunProperties(f,fnProp);

  m_changeSlotsEnabled = true;

  setFitEnabled(true);
}

/** Replace the current function with a new one
 * @param i The function index
 * @param fnName A registered function name
 */
void FitPropertyBrowser::replaceFunction(int i,const std::string& fnName)
{
  disableUndo();
  Mantid::API::IFunction* f = Mantid::API::FunctionFactory::Instance().createUnwrapped(fnName);
  f->initialize();
  replaceFunction(i,f);
}

/** Replace the current function with a new one
 * @param i The function index
 * @param f A pointer to a new function
 */
void FitPropertyBrowser::replaceFunction(int i,Mantid::API::IFunction* f)
{
  if (i < 0 || i >= count()) return;
  Mantid::API::IPeakFunction* pf = dynamic_cast<Mantid::API::IPeakFunction*>(f);
  if (pf && peakFunction(i))
  {
    pf->setCentre(peakFunction(i)->centre());
    pf->setHeight(peakFunction(i)->height());
    pf->setWidth(peakFunction(i)->width());
  }
  removeTiesWithFunction(i);// do it before replaceFunction
  m_compositeFunction->replaceFunction(i,f);
  QtBrowserItem* fnItem = m_functionItems[i];
  fnItem->property()->setPropertyName(functionName(i));

  removeFunProperties(fnItem->property());
  addFunProperties(f,fnItem->property());
}

/** Remove a function
 * @param i The function index
 */
void FitPropertyBrowser::removeFunction(int i)
{
  if (i < 0 || i >= count()) return;
  disableUndo();
  removeTiesWithFunction(i);// do it before removeFunction
  QtBrowserItem* fnItem = m_functionItems[i];
  QList<QtProperty*> subs = fnItem->property()->subProperties();
  for(int j=0;j<subs.size();j++)
  {
    fnItem->property()->removeSubProperty(subs[j]);
    delete subs[j]; // ?
  }
  QtProperty* fnGroup = fnItem->parent()->property();
  fnGroup->removeSubProperty(fnItem->property());
  m_compositeFunction->removeFunction(i);
  setIndex(index());
  setFocus();
  m_functionItems.removeAt(i);
  if (count() == 0)
  {
    setFitEnabled(false);
  }
  updateParameters();
  QList<QtProperty*> fns = fnGroup->subProperties();
  for(int j = 0;j<fns.size();j++)
  {
    fns[j]->setPropertyName(functionName(j));
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

/// Get the minimizer
std::string FitPropertyBrowser::minimizer()const
{
  int i = m_enumManager->value(m_minimizer);
  return m_minimizers[i].toStdString();
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
      int j0 = 1 + f->nAttributes();
      for(int j=j0;j<subs.size();j++)
      {
        if (subs[j] == prop)
        {
          f->parameter(j-j0) = value;
          done = true;
          break;
        }
      }
      if (done) 
      {
        emit parameterChanged(i);
        break;
      }
    }
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
  else if (prop->propertyName() == "Tie")
  {
    for(int i=0;i<m_ties.size();i++)
    {
      if (prop == m_ties[i].getProperty())
      {
        QString estr = m_stringManager->value(prop);
        // Make sure the tied parameter is right and the property keeps only the right-hand side formula
        int j = estr.indexOf('=');
        if (j == estr.size())
        {
          m_appWindow->mantidUI->showCritical("Tie expression is missing");
          m_stringManager->setValue(prop,"");
          return;
        }
        if (j >= 0)
        {
          estr.remove(0,j+1);
          m_stringManager->setValue(prop,estr);
          return;
        }
        try
        {
          estr.prepend(m_ties[i].parName()+"=");
          m_ties[i].set(estr);
        }
        catch(std::exception& e)
        {
          //QString msg = "Error in tie \""+estr+"\":\n\n"+QString::fromAscii(e.what());
          //m_stringManager->setValue(prop,"");
        }
      }
    }
  }
  else
  {// Check if it is a function attribute
    for(int fi=0;fi<m_functionItems.size();fi++)
    {
      QtProperty* fnProp = m_functionItems[fi]->property();
      QList<QtProperty*> funProps = fnProp->subProperties();
      // If a string is found in funProps - it is an attribute
      int ia = funProps.indexOf(prop);
      if (ia >= 0)
      {
        std::string attrName = prop->propertyName().toStdString();
        std::string attrValue = m_stringManager->value(prop).toStdString();
        try
        {
          function(fi)->setAttribute(attrName,attrValue);
        }
        catch(...)
        {
          break;
        }
        m_compositeFunction->checkFunction();
        removeFunProperties(fnProp);
        addFunProperties(function(fi),fnProp);
        break;
      }
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
  m_registeredPeaks.clear();
  m_registeredBackgrounds.clear();
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
  if (i < -1 || i >= count()) return;
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
    alg->setPropertyValue("Minimizer",minimizer());
    QString tiesStr;
    for(int i=0;i<m_ties.size();i++)
    {
      tiesStr += m_ties[i].expr();
      if (i!=m_ties.size()-1)
      {
        tiesStr += ",";
      }
    }
    alg->setPropertyValue("Ties",tiesStr.toStdString());

    QString constraintsStr;
    for(int i=0;i<m_functionItems.size();i++)
    {
      QList<QtProperty*> parProps = m_functionItems[i]->property()->subProperties();
      for(int j=1;j<parProps.size();j++)
      {
        QMap<QtProperty*,std::pair<QtProperty*,QtProperty*> >::iterator it = m_constraints.find(parProps[j]);
        if (it != m_constraints.end())
        {
          QString lowerStr;
          QString upperStr;
          QList<QtProperty*> subs = parProps[j]->subProperties();
          for(int k = 0;k != subs.size(); k++)
          {
            QtProperty* sub = subs[k];
            if (sub->propertyName() == "Lower Bound")
            {
              lowerStr = QString::number(m_doubleManager->value(sub));
            }
            else if (sub->propertyName() == "Upper Bound")
            {
              upperStr = QString::number(m_doubleManager->value(sub));
            }
          }
          if (!lowerStr.isEmpty() || !upperStr.isEmpty())
          {
            if (m_functionItems.size() > 1)
            {
              constraintsStr += "f" + QString::number(i) + ".";
            }
            constraintsStr += parProps[j]->propertyName() + "=" + lowerStr + ":" + upperStr + ",";
          }
        }
      }

      alg->setPropertyValue("Constraints",constraintsStr.toStdString());

    }

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
  if (!current) return;
  bool ok = false;
  int i = m_functionItems.indexOf(current);
  if (i >= 0)
  {
    ok = true;
  }
  else if (current == m_functionsGroup || current == m_settingsGroup || current->parent() == m_settingsGroup)
  {
  }
  else
  {
    i = m_functionItems.indexOf(current->parent());
    if (i >= 0)
    {
      ok = true;
    }
    else
    {
      i = m_functionItems.indexOf(current->parent()->parent());
      if (i >= 0)
      {
        ok = true;
      }
    }
  }

  if (ok)
  {
    setIndex(i);
  }
  else
  {
    setIndex(-1);
  }
}

/** Update the function parameter properties. 
 */
void FitPropertyBrowser::updateParameters()
{
  for(int i=0;i<m_functionItems.size();i++)
  {
    QtBrowserItem* fnItem = m_functionItems[i];
    QList<QtProperty*> paramProps = fnItem->property()->subProperties();
    Mantid::API::IFunction* f = m_compositeFunction->getFunction(i);
    // Parameter properties start after the "Type" field and all attributes
    int j0 = 1 + f->nAttributes();
    int nParamProps = paramProps.size()-j0;
    // If new parameters appeared (e.g. in a UserFunction)
    if (nParamProps != f->nParams())
    {
      for(int ip=0;ip< f->nParams();ip++)
      {
        QString parName = QString::fromStdString(f->parameterName(ip));
        QtProperty* parProp;
        if (ip >= nParamProps)
        {
          parProp = m_doubleManager->addProperty(parName);
        }
        else
        {
          parProp = paramProps[j0 + ip];
          parProp->setPropertyName(parName);
        }
        fnItem->property()->addSubProperty(parProp);
        double v = f->parameter(ip);
        m_doubleManager->setValue(parProp,v);
      }
      if (nParamProps > f->nParams())
      {// Remove the extra properties along with its ties and constraints
        for(int ip = f->nParams(); ip < nParamProps; ip++)
        {
          QtProperty* parProp = paramProps[ip+j0];
          for(int t=0;t<m_ties.size();)
          {
            if (m_ties[t].getProperty() == parProp)
            {
              m_ties.removeAt(t);
            }
            else
            {
              ++t;
            }
          }
          QMap<QtProperty*,std::pair<QtProperty*,QtProperty*> >::iterator cit = m_constraints.find(parProp);
          if (cit != m_constraints.end())
          {
            m_constraints.remove(parProp);
          }
          fnItem->property()->removeSubProperty(parProp);
        }// for(ip)
      }// if (nParamProps > f->nParams())
    }
    else
    {
      for(int j=j0;j<paramProps.size();j++)
      {
        int ip = j - j0;
        QString parName = QString::fromStdString(f->parameterName(ip));
        paramProps[j]->setPropertyName(parName);
        double v = f->parameter(ip);
        m_doubleManager->setValue(paramProps[j],v);
        QList<QtProperty*> tieProps = paramProps[j]->subProperties();
        for(int k=0;k<tieProps.size();k++)
        {
          if (tieProps[k]->propertyName() == "Tie")
          {
            int it = indexOfTie(tieProps[k]);
            if (it >= 0)
            {
              m_stringManager->setValue(tieProps[k],m_ties[it].exprRHS());
            }
          }
        }// for(k)
      }// for(j)
    }
  }// for(i)
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
      delete subs[j]; // ?
    }
    m_functionsGroup->property()->removeSubProperty(fnProp);
    delete fnProp; // ?
  }
  m_functionItems.clear();
  m_ties.clear();
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

/** Adds a tie
 * @param tstr The expression, e.g. "f1.Sigma= f0.Height/2"
 */
void FitPropertyBrowser::addTie(const QString& tstr)
{
  m_ties.push_back(FitParameterTie(m_compositeFunction));
  FitParameterTie& tie = m_ties.back();
  try
  {
    tie.set(tstr);
    int iPar = compositeFunction()->parameterIndex(tie.parName().toStdString());
    int iFun = compositeFunction()->functionIndex(iPar);
    QtBrowserItem* fnItem = m_functionItems[iFun];
    QtProperty* fnProp = fnItem->property();
    std::string parName = compositeFunction()->parameterLocalName(iPar);
    QtProperty* tieProp = m_stringManager->addProperty( "Tie" );
    tie.setProperty(tieProp);
    m_stringManager->setValue(tieProp,tie.expr());
    int iPar1 = function(iFun)->parameterIndex(parName)+1;
    QtProperty* parProp = fnProp->subProperties()[iPar1];
    parProp->addSubProperty(tieProp);
    m_browser->setExpanded(fnItem->children()[iPar1],false);
  }
  catch(std::exception& e)
  {
    QString msg = "Error in a tie:\n\n"+QString(e.what())+"\n";
    m_appWindow->mantidUI->showCritical(msg);
  }
}

/** Adds a tie
 * @param i The function index
 * @param parProp The property of the tied parameter
 */
void FitPropertyBrowser::addTie(int i,QtProperty* parProp,const QString& tieExpr)
{
  m_ties.push_back(FitParameterTie(m_compositeFunction));
  FitParameterTie& tie = m_ties.back();
  QtProperty* tieProp = m_stringManager->addProperty( "Tie" );
  tie.setProperty(tieProp);
  double value = m_doubleManager->value(parProp);
  tie.set(tieExpr);
  parProp->addSubProperty(tieProp);
  m_stringManager->setValue(tieProp,tie.exprRHS());
  //m_browser->setBackgroundColor(findItem(m_functionItems[i],parProp),QColor(Qt::yellow));
}


/** 
 * Slot. Adds a tie. Full expression to be entered <name>=<formula>
 */
void FitPropertyBrowser::addTie()
{
  QtBrowserItem * ci = m_browser->currentItem();
  int i = m_functionItems.indexOf(ci->parent());
  if (i >= 0 && ci->property()->propertyName() != "Type")
  {
    QtProperty* parProp = ci->property();
    double value = m_doubleManager->value(parProp);
    addTie(i,parProp,"f"+QString::number(i)+"."+parProp->propertyName()+"="+QString::number(value));
  }
  else
  {
    bool ok = false;
    QString tieStr = 
      QInputDialog::getText(this, "MantidPlot - Fit", "Enter tie expression", QLineEdit::Normal,"",&ok);
    if (ok)
    {
      addTie(tieStr);
    }
  }
}

/** 
 * Slot. Ties a parameter to a parameter with the same name of a different function
 */
void FitPropertyBrowser::addTieToFunction()
{
  QtBrowserItem * ci = m_browser->currentItem();
  std::string parName = ci->property()->propertyName().toStdString();
  bool ok;
  QStringList fnNames;
  for(int i=0;i<count();i++)
  {
    if (i == index()) continue;
    Mantid::API::IFunction* fun = m_compositeFunction->getFunction(i);
    for(int j=0;j<fun->nParams();j++)
    {
      if (fun->parameterName(j) == parName)
      {
        fnNames << m_functionItems[i]->property()->propertyName();
      }
    }
  }
  if (fnNames.empty())
  {
    QMessageBox::information(m_appWindow,"Mantid - information","Cannot tie this parameter to any function");
    return;
  }

  QString fnName = 
    QInputDialog::getItem(this, "MantidPlot - Fit", "Select function", fnNames,0,false,&ok);

  if (!ok) return;

  for(int i=0;i<count();i++)
  {
    if (m_functionItems[i]->property()->propertyName() == fnName)
    {
      QString expr = "f"+QString::number(index())+"."+QString::fromStdString(parName)
        +"=f"+QString::number(i)+"."+QString::fromStdString(parName);
      addTie(index(),ci->property(),expr);
    }
  }
}

/** 
 * Slot. Adds a tie. Full expression to be entered <name>=<formula>
 */
void FitPropertyBrowser::addFixTie()
{
  QtBrowserItem * ci = m_browser->currentItem();
  int i = m_functionItems.indexOf(ci->parent());
  if (i >= 0 && ci->property()->propertyName() != "Type")
  {
    QtProperty* parProp = ci->property();
    double value = m_doubleManager->value(parProp);
    addTie("f"+QString::number(i)+"."+parProp->propertyName()+"="+QString::number(value));
    parProp->setEnabled(false);
  }
}

/** 
 * Slot. Deletes a tie. 
 */
void FitPropertyBrowser::deleteTie()
{
  QtBrowserItem * ci = m_browser->currentItem();
  QtProperty* parProp;
  QtProperty* tieProp;
  int iFun;
  if (ci->property()->propertyName() != "Tie") 
  {
    parProp = ci->property();
    tieProp = getTieProperty(parProp);
    if (!tieProp) return;
    iFun = m_functionItems.indexOf(ci->parent());
  }
  else
  {
    tieProp = ci->property();
    parProp = ci->parent()->property();
    iFun = m_functionItems.indexOf(ci->parent()->parent());
  }
  QString parName = "f"+QString::number(iFun)+"."+parProp->propertyName();
  for(int i=0;i<m_ties.size();i++)
  {
    if (m_ties[i].parName() == parName)
    {
      m_ties.removeAt(i);
      parProp->removeSubProperty(tieProp);
      break;
    }
  }
  parProp->setEnabled(true);
}

/** Check ties' validity. Removes invalid ties.
 * @param iFun The deleted function index
 */
void FitPropertyBrowser::removeTiesWithFunction(int iFun)
{
  for(int i=0;i<m_ties.size();)
  {
    std::string parName = m_ties[i].parName().toStdString();
    if (!m_ties[i].functionDeleted(iFun))// the tie is invalid
    {
      int iPar = compositeFunction()->parameterIndex(parName);
      int iFun1 = compositeFunction()->functionIndex(iPar);
      QtBrowserItem* fnItem = m_functionItems[iFun1];
      QList<QtBrowserItem*> subs = fnItem->children();
      for(int j=1;j!=subs.size();j++)
      {
        QList<QtProperty*> ts = subs[j]->property()->subProperties();
        int k = ts.indexOf(m_ties[i].getProperty());
        if (k >= 0)
        {
          subs[j]->property()->removeSubProperty(ts[k]);
        }
      }
      m_ties.removeAt(i);
    }
    else
    {
      i++;
    }
  }
}

/** Find the tie index for a property. 
 * @param tieProp The property which displays and sets a tie.
 * @return The index of the tie if successful or -1 if failed.
 */
int FitPropertyBrowser::indexOfTie(QtProperty* tieProp)
{
  for(int i=0;i<m_ties.size();i++)
  {
    if (m_ties[i].getProperty() == tieProp)
    {
      return i;
    }
  }
  return -1;
}

/** Does a parameter have a tie
 * @param parProp The property for a function parameter
 */
bool FitPropertyBrowser::hasTie(QtProperty* parProp)const
{
  QList<QtProperty*> subs = parProp->subProperties();
  for(int i=0;i<subs.size();i++)
  {
    if (subs[i]->propertyName() == "Tie")
    {
      return true;
    }
  }
  return false;
}

/** Returns the tie property for a parameter property, or NULL
 * @param The parameter property
 */
QtProperty* FitPropertyBrowser::getTieProperty(QtProperty* parProp)const
{
  QList<QtProperty*> subs = parProp->subProperties();
  for(int i=0;i<subs.size();i++)
  {
    if (subs[i]->propertyName() == "Tie")
    {
      return subs[i];
    }
  }
  return NULL;
}

/** Display a tip
 * @param txt The text to display
 */
void FitPropertyBrowser::setTip(const QString& txt)
{
  m_tip->setText(txt);
}

/**
 * Slot. Adds lower bound to the selected parameter property
 * and sets it f % below parameter's current value
 */
void FitPropertyBrowser::addLowerBound(int f)
{
  QtBrowserItem * ci = m_browser->currentItem();
  QtProperty* parProp = ci->property();
  std::pair<QtProperty*,QtProperty*>& c = m_constraints[parProp];
  QtProperty* lower = c.first;
  if (lower != NULL) return;
  lower = m_doubleManager->addProperty("Lower Bound");
  parProp->addSubProperty(lower);
  c.first = lower;
  double value = m_doubleManager->value(parProp)*(1.-0.01*f);
  m_doubleManager->setValue(lower,value);
}

/**
 * Slot. Adds lower bound to the selected parameter property
 */
void FitPropertyBrowser::addLowerBound()
{
  addLowerBound(0);
}

/**
 * Slot. Adds lower bound to the selected parameter property
 */
void FitPropertyBrowser::addLowerBound10()
{
  addLowerBound(10);
}

/**
 * Slot. Adds lower bound to the selected parameter property
 */
void FitPropertyBrowser::addLowerBound50()
{
  addLowerBound(50);
}

/**
 * Slot.Adds upper bound to the selected parameter property
 */
void FitPropertyBrowser::addUpperBound(int f)
{
  QtBrowserItem * ci = m_browser->currentItem();
  QtProperty* parProp = ci->property();
  std::pair<QtProperty*,QtProperty*>& c = m_constraints[parProp];
  QtProperty* upper = c.second;
  if (upper != NULL) return;
  upper = m_doubleManager->addProperty("Upper Bound");
  parProp->addSubProperty(upper);
  c.second = upper;
  double value = m_doubleManager->value(parProp)*(1.+0.01*f);
  m_doubleManager->setValue(upper,value);
}

/**
 * Slot.Adds upper bound to the selected parameter property
 */
void FitPropertyBrowser::addUpperBound10()
{
  addUpperBound(10);
}

/**
 * Slot.Adds upper bound to the selected parameter property
 */
void FitPropertyBrowser::addUpperBound50()
{
  addUpperBound(50);
}

/**
 * Slot.Adds upper bound to the selected parameter property
 */
void FitPropertyBrowser::addUpperBound()
{
  addUpperBound(0);
}

/**
 * Slot.Sets the lower and upper bounds of the selected parameter to 10% of its value
 */
void FitPropertyBrowser::addBothBounds10()
{
  addLowerBound10();
  addUpperBound10();
}

/**
 * Slot.Sets the lower and upper bounds of the selected parameter to 50% of its value
 */
void FitPropertyBrowser::addBothBounds50()
{
  addLowerBound50();
  addUpperBound50();
}

/**
 * Slot.Adds lower and upper bounds to the selected parameter property
 */
void FitPropertyBrowser::addBothBounds()
{
  addLowerBound();
  addUpperBound();
}

/**
 * Slot.Removes lower bound from the selected parameter property
 */
void FitPropertyBrowser::removeLowerBound()
{
  QtBrowserItem * ci = m_browser->currentItem();
  QtProperty* parProp = ci->parent()->property();
  QMap<QtProperty*,std::pair<QtProperty*,QtProperty*> >::iterator c = m_constraints.find(parProp);
  if (c != m_constraints.end())
  {
    if (c.value().first)
    {
      parProp->removeSubProperty(c.value().first);
    }
    if (c.value().second)
    {
      c.value().first = 0;
    }
    else
    {
      m_constraints.erase(c);
    }
  }
}

/**
 * Slot.Removes upper bound from the selected parameter property
 */
void FitPropertyBrowser::removeUpperBound()
{
  QtBrowserItem * ci = m_browser->currentItem();
  QtProperty* parProp = ci->parent()->property();
  QMap<QtProperty*,std::pair<QtProperty*,QtProperty*> >::iterator c = m_constraints.find(parProp);
  if (c != m_constraints.end())
  {
    if (c.value().second)
    {
      parProp->removeSubProperty(c.value().second);
    }
    if (c.value().first)
    {
      c.value().second = 0;
    }
    else
    {
      m_constraints.erase(c);
    }
  }
}

/**
 * Slot.Removes lower and upper bounds from the selected parameter property
 */
void FitPropertyBrowser::removeBounds()
{
  QtBrowserItem * ci = m_browser->currentItem();
  QtProperty* parProp = ci->property();
  QMap<QtProperty*,std::pair<QtProperty*,QtProperty*> >::iterator c = m_constraints.find(parProp);
  if (c != m_constraints.end())
  {
    if (c.value().first)
    {
      parProp->removeSubProperty(c.value().first);
    }
    if (c.value().second)
    {
      parProp->removeSubProperty(c.value().second);
    }
    m_constraints.erase(c);
  }
}

/**
 * Slot. Sends a signal to plot the guess for the current (selected) function
 */
void FitPropertyBrowser::plotGuessCurrent()
{
  emit plotGuess(index());
}

/// Remove all properties associated with a function
void FitPropertyBrowser::removeFunProperties(QtProperty* fnProp)
{
  QList<QtProperty*> subs = fnProp->subProperties();
  for(int i = 0; i < subs.size(); i++)
  {
    QtProperty* parProp = subs[i];
    for(int t=0;t<m_ties.size();)
    {
      if (m_ties[t].getProperty() == parProp)
      {
        m_ties.removeAt(t);
      }
      else
      {
        ++t;
      }
    }
    QMap<QtProperty*,std::pair<QtProperty*,QtProperty*> >::iterator cit = m_constraints.find(parProp);
    if (cit != m_constraints.end())
    {
      m_constraints.remove(parProp);
    }
    fnProp->removeSubProperty(parProp);
  }
}
/** Add properties associated with a function: type, attributes, parameters
 * @param f A pointer to the function
 * @param fnProp The group property for the function f
 */
void FitPropertyBrowser::addFunProperties(Mantid::API::IFunction* f,QtProperty* fnProp)
{
  m_changeSlotsEnabled = false;

  QtProperty* typeProp = m_enumManager->addProperty("Type");
  fnProp->addSubProperty(typeProp);
  int itype = m_registeredFunctions.indexOf(QString::fromStdString(f->name()));
  m_enumManager->setEnumNames(typeProp, m_registeredFunctions);
  m_enumManager->setValue(typeProp,itype);

  // Add attributes for the function's parameters
  std::vector<std::string> attr = f->getAttributeNames();
  for(size_t i=0;i<attr.size();i++)
  {
    QtProperty* parProp = m_stringManager->addProperty(QString::fromStdString(attr[i]));
    fnProp->addSubProperty(parProp);
    m_stringManager->setValue(parProp,QString::fromStdString(f->getAttribute(attr[i])));
  }

  // Add properties for the function's parameters
  for(int i=0;i<f->nParams();i++)
  {
    QtProperty* parProp = m_doubleManager->addProperty(QString::fromStdString(f->parameterName(i)));
    fnProp->addSubProperty(parProp);
    m_doubleManager->setValue(parProp,f->parameter(i));
  }

  m_changeSlotsEnabled = true;
}
