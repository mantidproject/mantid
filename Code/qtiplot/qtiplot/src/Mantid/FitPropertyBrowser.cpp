#include "FitPropertyBrowser.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/ConstraintFactory.h"

#include "FilenameEditorFactory.h"

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
#include <QSettings>
#include <QFileInfo>

/**
 * Constructor
 * @param parent The parent widget - must be an ApplicationWindow
 */
FitPropertyBrowser::FitPropertyBrowser(QWidget* parent)
:QDockWidget("Fit Function",parent),m_currentFunction(0),m_compositeFunction(0),m_defaultFunction("Gaussian"),m_default_width(0),
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
  m_filenameManager = new QtStringPropertyManager(w);

    /* Create the top level group */

  /*QtProperty* fitGroup = */m_groupManager->addProperty("Fit");

  connect(m_enumManager,SIGNAL(propertyChanged(QtProperty*)),this,SLOT(enumChanged(QtProperty*)));
  connect(m_boolManager,SIGNAL(propertyChanged(QtProperty*)),this,SLOT(boolChanged(QtProperty*)));
  connect(m_intManager,SIGNAL(propertyChanged(QtProperty*)),this,SLOT(intChanged(QtProperty*)));
  connect(m_doubleManager,SIGNAL(propertyChanged(QtProperty*)),this,SLOT(doubleChanged(QtProperty*)));
  connect(m_stringManager,SIGNAL(propertyChanged(QtProperty*)),this,SLOT(stringChanged(QtProperty*)));
  connect(m_filenameManager,SIGNAL(propertyChanged(QtProperty*)),this,SLOT(filenameChanged(QtProperty*)));

    /* Create function group */

  QtProperty* functionsGroup = m_groupManager->addProperty("Functions");

     /* Create input - output properties */

  QtProperty* settingsGroup = m_groupManager->addProperty("Settings");

  m_workspace = m_enumManager->addProperty("Workspace");
  m_workspaceIndex = m_intManager->addProperty("Workspace Index");
  m_startX = addDoubleProperty("StartX");
  m_endX = addDoubleProperty("EndX");
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
  FilenameEditorFactory* filenameEditFactory = new FilenameEditorFactory(w);

  m_browser = new QtTreePropertyBrowser();
  m_browser->setFactoryForManager(m_enumManager, comboBoxFactory);
  m_browser->setFactoryForManager(m_boolManager, checkBoxFactory);
  m_browser->setFactoryForManager(m_intManager, spinBoxFactory);
  m_browser->setFactoryForManager(m_doubleManager, doubleSpinBoxFactory);
  m_browser->setFactoryForManager(m_stringManager, lineEditFactory);
  m_browser->setFactoryForManager(m_filenameManager, filenameEditFactory);

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
  if (m_compositeFunction) delete m_compositeFunction;
}

void FitPropertyBrowser::popupMenu(const QPoint &)
{
  QtBrowserItem * ci = m_browser->currentItem();
  if (!ci) return;
  QMenu *menu = new QMenu(m_appWindow);
  QAction *action;

  bool isFunctionsGroup = ci == m_functionsGroup;
  bool isSettingsGroup = ci == m_settingsGroup;
  bool isASetting = ci->parent() == m_settingsGroup;
  bool isFunction = m_functionItems.contains(ci);
  bool isCompositeFunction = isFunction && dynamic_cast<Mantid::API::CompositeFunction*>(m_functionItems[ci]);

  if (isFunctionsGroup)
  {
    action = new QAction("Add function",this);
    connect(action,SIGNAL(triggered()),this,SLOT(addFunction()));
    menu->addAction(action);

    action = new QAction("Plot",this);
    connect(action,SIGNAL(triggered()),this,SLOT(plotGuessAll()));
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
    if (isCompositeFunction)
    {
      action = new QAction("Add function",this);
      connect(action,SIGNAL(triggered()),this,SLOT(addFunction()));
      menu->addAction(action);
    }

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
    else if (isLowerBound || isUpperBound)
    {
      action = new QAction("Remove",this);
      connect(action,SIGNAL(triggered()),this,SLOT(removeBounds()));
      menu->addAction(action);
    }
    //else if (isUpperBound)
    //{
    //  action = new QAction("Remove",this);
    //  connect(action,SIGNAL(triggered()),this,SLOT(removeUpperBound()));
    //  menu->addAction(action);
    //}
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
void FitPropertyBrowser::addFunction(const std::string& fnName, Mantid::API::CompositeFunction* cfun)
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
  Mantid::API::CompositeFunction* cf = cfun ? cfun : m_compositeFunction;
  cf->addFunction(f);

  m_changeSlotsEnabled = false;

  // Add a group property named after the function: f<index>-<type>, where <type> is the function's class name
  QtProperty* fnProp = m_groupManager->addProperty(functionName(f));
  QtBrowserItem* cfunItem = m_functionItems.key(cf,m_functionsGroup);
  cfunItem->property()->addSubProperty(fnProp);

  QtBrowserItem* fnItem = findItem(m_functionsGroup,fnProp);
  m_browser->setExpanded(fnItem,false);
  m_functionItems[fnItem] = f;
  setCurrentFunction(f);

  addFunProperties(f);

  checkFunction();

  m_changeSlotsEnabled = true;

  setFitEnabled(true);
  m_defaultFunction = fnName;
  setFocus();
}

/** Replace the current function with a new one
 * @param i The function index
 * @param fnName A registered function name
 */
void FitPropertyBrowser::replaceFunction(Mantid::API::IFunction* f_old,const std::string& fnName)
{
  disableUndo();
  Mantid::API::IFunction* f = Mantid::API::FunctionFactory::Instance().createUnwrapped(fnName);
  f->initialize();
  replaceFunction(f_old,f);
}

/** Replace the current function with a new one
 * @param i The function index
 * @param f A pointer to a new function
 */
void FitPropertyBrowser::replaceFunction(Mantid::API::IFunction* f_old,Mantid::API::IFunction* f_new)
{
  QtBrowserItem* fItem = m_functionItems.key(f_old,NULL);
  if (fItem == NULL) return;
  
  QtBrowserItem* fParent = fItem->parent();
  if (fParent == NULL) return;
  Mantid::API::CompositeFunction* cf = dynamic_cast<Mantid::API::CompositeFunction*>(m_functionItems[fParent]);
  if (!cf) return;
  int iFun = -1;
  for (int i=0;i<cf->nFunctions();i++)
  {
    if (f_old == cf->getFunction(i))
    {
      iFun = i;
      break;
    }
  }
  if (iFun < 0) return;
  removeFunctionItems(fItem);

  Mantid::API::IPeakFunction* pf_new = dynamic_cast<Mantid::API::IPeakFunction*>(f_new);
  Mantid::API::IPeakFunction* pf_old = dynamic_cast<Mantid::API::IPeakFunction*>(f_old);
  if (pf_new && pf_old)
  {
    pf_new->setCentre(pf_old->centre());
    pf_new->setHeight(pf_old->height());
    pf_new->setWidth(pf_old->width());
  }
  //removeTiesWithFunction(i);// do it before replaceFunction
  cf->replaceFunction(iFun,f_new);
  fItem->property()->setPropertyName(functionName(f_new));

  m_functionItems[fItem] = f_new;
  removeFunProperties(fItem->property());
  addFunProperties(f_new);
  checkFunction();
}

/** Remove a function
 * @param i The function index
 */
void FitPropertyBrowser::removeFunction(Mantid::API::IFunction* f)
{
  QtBrowserItem* fnItem = m_functionItems.key(f,NULL);
  if (fnItem == NULL) return;
  
  QtBrowserItem* fnParent = fnItem->parent();
  if (fnParent == NULL) return;
  Mantid::API::CompositeFunction* cf = dynamic_cast<Mantid::API::CompositeFunction*>(m_functionItems[fnParent]);
  if (!cf) return;
  int iFun = -1;
  for (int i=0;i<cf->nFunctions();i++)
  {
    if (f == cf->getFunction(i))
    {
      iFun = i;
      break;
    }
  }
  if (iFun < 0) return;

  removeFunctionItems(fnItem);
  //removeTiesWithFunction(i);// do it before removeFunction
  QList<QtProperty*> subs = fnItem->property()->subProperties();
  for(int j=0;j<subs.size();j++)
  {
    fnItem->property()->removeSubProperty(subs[j]);
    delete subs[j]; // ?
  }
  QtProperty* fnGroup = fnItem->parent()->property();
  fnGroup->removeSubProperty(fnItem->property());
  cf->removeFunction(iFun);
  checkFunction();
  if (count() == 0)
  {
    setFitEnabled(false);
  }
  updateParameters();
  updateNames();
  disableUndo();
  setFocus();
  emit functionRemoved(f);
}


/** Get function name
 * @param f The function's address
 */
QString FitPropertyBrowser::functionName(Mantid::API::IFunction* f,Mantid::API::CompositeFunction* cf)const
{
  if (f == m_compositeFunction) return "Functions";
  if (!cf) cf = m_compositeFunction;
  QString outName = "f";
  for(int iFun = 0;iFun < cf->nFunctions(); iFun++)
  {
    Mantid::API::IFunction* fun = cf->getFunction(iFun);
    if (fun == f)
    {
      return "f" + QString::number(iFun) + "-" + QString::fromStdString(fun->name());
    }
    Mantid::API::CompositeFunction* cf1 = dynamic_cast<Mantid::API::CompositeFunction*>(fun);
    if (cf1)
    {
      QString fn = functionName(f,cf1);
      if (!fn.isEmpty())
      {
        return "f" + QString::number(iFun) + "." + fn;
      }
    }
  }
  return "";
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
    QtBrowserItem* typeItem = m_paramItems[prop];
    if (typeItem)
    {
      Mantid::API::IFunction* fun = 0;
      int j = m_enumManager->value(prop);
      QString fnName = m_registeredFunctions[j];
      QtBrowserItem* fnItem = typeItem->parent();
      if (m_functionItems.contains(fnItem))
      {
        fun = m_functionItems[fnItem];
      }
      if (fun)
      {
        replaceFunction(fun,fnName.toStdString());
      }
    }
  }
}

/** Called when a bool property changed
 * @param prop A pointer to the property 
 */
void FitPropertyBrowser::boolChanged(QtProperty*)
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
  else if(m_paramItems.contains(prop))
  {
    QtBrowserItem* parItem = m_paramItems[prop];
    QtBrowserItem* fnItem = parItem->parent();
    Mantid::API::IFunction* f = m_functionItems[fnItem];
    if (!f) return;
    QList<QtProperty*> subs = fnItem->property()->subProperties();
    int j0 = 1 + f->nAttributes();
    for(int j=j0;j<subs.size();j++)
    {
      if (subs[j] == prop)
      {
        f->setParameter(j-j0,value);
        emit parameterChanged(f);
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
  {//   -------  need to change this code for setting a tie from the property editor ---------
    //for(int i=0;i<m_ties.size();i++)
    //{
    //  if (prop == m_ties[i].getProperty())
    //  {
    //    QString estr = m_stringManager->value(prop);
    //    // Make sure the tied parameter is right and the property keeps only the right-hand side formula
    //    int j = estr.indexOf('=');
    //    if (j == estr.size())
    //    {
    //      m_appWindow->mantidUI->showCritical("Tie expression is missing");
    //      m_stringManager->setValue(prop,"");
    //      return;
    //    }
    //    if (j >= 0)
    //    {
    //      estr.remove(0,j+1);
    //      m_stringManager->setValue(prop,estr);
    //      return;
    //    }
    //    try
    //    {
    //      estr.prepend(m_ties[i].parName()+"=");
    //      m_ties[i].set(estr);
    //    }
    //    catch(std::exception& e)
    //    {
    //      //QString msg = "Error in tie \""+estr+"\":\n\n"+QString::fromAscii(e.what());
    //      //m_stringManager->setValue(prop,"");
    //    }
    //  }
    //}
  }
  else if (m_paramItems.contains(prop))
  {// Check if it is a function attribute
    QtBrowserItem* attrItem = m_paramItems[prop];
    QtBrowserItem* fnItem = attrItem->parent();
    if (fnItem && m_functionItems.contains(fnItem))
    {
      Mantid::API::IFunction* fun = m_functionItems[fnItem];
      if (fun)
      {
        std::string attrName = prop->propertyName().toStdString();
        try
        {
          fun->setAttribute(attrName,m_stringManager->value(prop).toStdString());
          m_compositeFunction->checkFunction();
          removeFunProperties(fnItem->property(),true);
          addFunProperties(fun,true);
        }
        catch(...)
        {
        }
      }
    }
  }
}

/** Called when a filename property changed
 * @param prop A pointer to the property 
 */
void FitPropertyBrowser::filenameChanged(QtProperty* prop)
{
  if ( ! m_changeSlotsEnabled ) return;

  if (m_paramItems.contains(prop))
  {// Check if it is a function attribute
    QtBrowserItem* attrItem = m_paramItems[prop];
    QtBrowserItem* fnItem = attrItem->parent();
    if (fnItem && m_functionItems.contains(fnItem))
    {
      Mantid::API::IFunction* fun = m_functionItems[fnItem];
      if (fun)
      {
        std::string attrName = prop->propertyName().toStdString();
        std::string attrValue = m_filenameManager->value(prop).toStdString();
        try
        {
          fun->setAttribute(attrName,attrValue);
          m_compositeFunction->checkFunction();
          removeFunProperties(fnItem->property(),true);
          addFunProperties(fun,true);
          QFileInfo finfo(QString::fromStdString(attrValue));
          QSettings settings;
          settings.setValue("Mantid/FitBrowser/ResolutionDir",finfo.absolutePath());
        }
        catch(std::exception& e)
        {
          std::cerr<<"Error "<<e.what()<<'\n';
          QMessageBox::critical(this,"Mantid - Error","Error in loading a resolution file.\n"
            "The file must have two or more columns of numbers.\n"
            "The first two columns are x and y-values of the resolution.");
        }
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
    //if (fnName != "Convolution")
    //{
      QString qfnName = QString::fromStdString(fnName);
      m_registeredFunctions << qfnName;
      boost::shared_ptr<Mantid::API::IFunction> f = Mantid::API::FunctionFactory::Instance().create(fnName);
      f->initialize();
      Mantid::API::IPeakFunction* pf = dynamic_cast<Mantid::API::IPeakFunction*>(f.get());
      Mantid::API::CompositeFunction* cf = dynamic_cast<Mantid::API::CompositeFunction*>(f.get());
      if (pf)
      {
        m_registeredPeaks << qfnName;
      }
      else if (!cf)
      {
        m_registeredBackgrounds << qfnName;
      }
    //}
  }
}

/// Create CompositeFunction
void FitPropertyBrowser::createCompositeFunction()
{
  emit functionRemoved(m_compositeFunction);
  if (m_compositeFunction) delete m_compositeFunction;
  m_compositeFunction = new Mantid::API::CompositeFunction();
  m_functionItems[m_functionsGroup] = m_compositeFunction;
  disableUndo();
  setFitEnabled(false);
}

/// Get number of functions in CompositeFunction
int FitPropertyBrowser::count()const
{
  return m_compositeFunction->nFunctions();
}

/** Set new current function
 * @param f New current function
 */
void FitPropertyBrowser::setCurrentFunction(Mantid::API::IFunction* f)const
{
  m_currentFunction = f;
  QtBrowserItem* fnItem = m_functionItems.key(f,0);
  if (fnItem)
  {
    m_browser->setCurrentItem(fnItem);
    emit currentChanged();
  }
}

/**
 * Creates an instance of Fit algorithm, sets its properties and launches it.
 */
void FitPropertyBrowser::fit()
{
  std::cerr << '\n' << *m_compositeFunction <<'\n';

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
      m_initialParameters[i] = compositeFunction()->getParameter(i);
    }
    m_btnUnFit->setEnabled(true);

    bool simpleFunction;
    Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("Fit");
    alg->initialize();
    alg->setPropertyValue("InputWorkspace",wsName);
    alg->setProperty("WorkspaceIndex",workspaceIndex());
    alg->setProperty("StartX",startX());
    alg->setProperty("EndX",endX());
    alg->setPropertyValue("Output",outputName());
    if (m_compositeFunction->nFunctions() > 1)
    {
      alg->setPropertyValue("Function",*m_compositeFunction);
      simpleFunction = false;
    }
    else
    {
      alg->setPropertyValue("Function",*(m_compositeFunction->getFunction(0)));
      simpleFunction = true;
    }
    alg->setPropertyValue("Minimizer",minimizer());

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

/** 
 * Get the current function if it's a peak
 */
Mantid::API::IPeakFunction* FitPropertyBrowser::peakFunction()const
{
  return dynamic_cast<Mantid::API::IPeakFunction*>(m_currentFunction);
}

/** Slot. Called to add a new function
 */
void FitPropertyBrowser::addFunction()
{
  QtBrowserItem * ci = m_browser->currentItem();
  if ( !m_functionItems.contains(ci) ) return;
  Mantid::API::CompositeFunction* cf = dynamic_cast<Mantid::API::CompositeFunction*>(m_functionItems[ci]);
  if ( !cf ) return;
  int i = m_registeredFunctions.indexOf(QString::fromStdString(m_defaultFunction));
  bool ok = false;
  QString fnName = 
    QInputDialog::getItem(this, "MantidPlot - Fit", "Select function type", m_registeredFunctions,i,false,&ok);
  if (ok)
  {
    addFunction(fnName.toStdString(),cf);
  }
}

/** Slot. Called to remove a function
 */
void FitPropertyBrowser::deleteFunction()
{
  if (m_currentFunction != NULL && m_currentFunction != m_compositeFunction)
  {
    removeFunction(m_currentFunction);
  }
}

QtBrowserItem* FitPropertyBrowser::findItem(QtBrowserItem* parent,QtProperty* prop)const
{
  QList<QtBrowserItem*> children = parent->children();
  QtBrowserItem* res = 0;
  for(int i=0;i<children.size();i++)
  {
    if (children[i]->property() == prop)
    {
      return children[i];
    }
    QList<QtBrowserItem*> grand_children = children[i]->children();
    if (grand_children.size() > 0) res = findItem(children[i],prop);
    if (res) return res;
  }
  return 0;
}

/**
 * Slot. Responds to changing the current item
 */
void FitPropertyBrowser::currentItemChanged(QtBrowserItem * current )
{
  Mantid::API::IFunction* f = 0;
  QtBrowserItem* fnItem = current;
  for(int i=0;i<100;i++)
  {
    if (f || !fnItem) break;
    if (m_functionItems.contains(fnItem))
    {
      f = m_functionItems[fnItem];
    }
    else
    {
      fnItem = fnItem->parent();
    }
  }

  m_currentFunction = f;
  emit currentChanged();
}

/** Update the function parameter properties. 
 */
void FitPropertyBrowser::updateParameters()
{
  QMap<QtProperty*,QtBrowserItem*>::iterator it = m_paramItems.begin();
  for(;it!=m_paramItems.end();it++)
  {
    if (it.key()->propertyManager() == m_doubleManager)
    {
      QtBrowserItem* fnItem = it.value()->parent();
      if (m_functionItems.contains(fnItem))
      {
        Mantid::API::IFunction* fun = m_functionItems[fnItem];
        if (fun)
        {
          m_doubleManager->setValue(it.key(),fun->getParameter(it.key()->propertyName().toStdString()));
        }
      }
    }
  }
}

/**
 * Slot. Removes all functions.
 */
void FitPropertyBrowser::clear()
{
  QList<QtProperty*> props = m_functionsGroup->property()->subProperties();
  QtProperty* prop;
  foreach(prop,props)
  {
    m_functionsGroup->property()->removeSubProperty(prop);
  }
  m_functionItems.clear();
  m_paramItems.clear();
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
        compositeFunction()->setParameter(name,value);
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
      compositeFunction()->setParameter(i,m_initialParameters[i]);
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

/** Get the property for a function's parameter.
 * @param f A pointer to a simple function
 * 
 */
QtProperty* FitPropertyBrowser::getParameterProperty(Mantid::API::IFunction* f,int i)const
{
  QtBrowserItem* fnItem = m_functionItems.key(f,0);
  if (!fnItem) return 0;
  QList<QtProperty*> props = fnItem->property()->subProperties();
  int j = 1 + f->nAttributes() + i;
  if (j > props.size()) return 0;
  return props[j];
}

/**
 * Adds a tie to a function
 * @param tieExpr The expression, e.g. "f1.Sigma= f0.Height/2"
 */
bool FitPropertyBrowser::addTie(const QString& tieExpr,Mantid::API::IFunction* f)
{
  QStringList parts = tieExpr.split("=");
  if (parts.size() != 2) return false;
  std::string name = parts[0].trimmed().toStdString();
  std::string expr = parts[1].trimmed().toStdString();
  try
  {
    Mantid::API::ParameterTie* tie = f->tie(name,expr);
    if (tie == NULL) return false;
    QtProperty* parProp = getParameterProperty(tie->getFunction(),tie->getIndex());
    if (!parProp) return false;
    QtProperty* tieProp = m_stringManager->addProperty( "Tie" );
    m_stringManager->setValue(tieProp,QString::fromStdString(expr));
    parProp->addSubProperty(tieProp);
    m_ties[tieProp] = tie;
    return true;
  }
  catch(...)
  {
    return false;
  }
  return false;
}

/** 
 * Slot. Adds a tie. Full expression to be entered <name>=<formula>
 */
void FitPropertyBrowser::addTie()
{
  QtBrowserItem * ci = m_browser->currentItem();
  QtProperty* paramProp = m_paramItems.key(ci,0);
  if (!paramProp) return;
  QtBrowserItem* fnItem = ci->parent();
  Mantid::API::IFunction* f = m_functionItems.contains(fnItem) ? m_functionItems[fnItem] : 0;
  if (!f) return;

  bool ok = false;
  QString tieStr = 
    QInputDialog::getText(this, "MantidPlot - Fit", "Enter tie expression", QLineEdit::Normal,"",&ok);
  if (ok)
  {
    tieStr = tieStr.trimmed();
    if (!tieStr.contains('='))
    {
      int iPar = f->parameterIndex(paramProp->propertyName().toStdString());
      Mantid::API::ParameterReference ref(f,iPar);
      iPar = m_compositeFunction->getParameterIndex(ref);
      tieStr = QString::fromStdString(m_compositeFunction->parameterName(iPar)) + "=" + tieStr;
    }
    ok = addTie(tieStr,m_compositeFunction);
  } // if (ok)
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

  int iPar = -1;
  for(int i=0;i<m_compositeFunction->nParams();i++)
  {
    Mantid::API::ParameterReference ref(m_compositeFunction,i);
    Mantid::API::IFunction* fun = ref.getFunction();
    QtProperty* prop = getParameterProperty(fun,ref.getIndex());
    if (prop == ci->property())
    {
      iPar = i;
      continue;
    }
    if (fun->parameterName(ref.getIndex()) == parName)
    {
      fnNames << QString::fromStdString(m_compositeFunction->parameterName(i));
    }
  }
  if (fnNames.empty() || iPar < 0)
  {
    QMessageBox::information(m_appWindow,"Mantid - information","Cannot tie this parameter to any function");
    return;
  }

  QString tieName = 
    QInputDialog::getItem(this, "MantidPlot - Fit", "Select function", fnNames,0,false,&ok);

  if (!ok) return;

  QString tieExpr = QString::fromStdString(m_compositeFunction->parameterName(iPar)) + "=" + tieName;

  addTie(tieExpr,m_compositeFunction);

}

/** 
 * Slot. Adds a tie. The current item must be a function parameter
 */
void FitPropertyBrowser::addFixTie()
{
  QtBrowserItem * ci = m_browser->currentItem();
  QtProperty* paramProp = m_paramItems.key(ci,0);
  if (!paramProp) return;
  QtBrowserItem* fnItem = ci->parent();
  Mantid::API::IFunction* f = m_functionItems.contains(fnItem) ? m_functionItems[fnItem] : 0;
  if (!f) return;
  double value = m_doubleManager->value(paramProp);
  addTie(paramProp->propertyName() + "=" + QString::number(value),f);
  paramProp->setEnabled(false);
  m_browser->setExpanded(ci,false);
}

/** 
 * Slot. Deletes a tie. 
 */
void FitPropertyBrowser::deleteTie()
{
  QtBrowserItem * ci = m_browser->currentItem();
  QtProperty* parProp;
  QtProperty* tieProp;
  if (ci->property()->propertyName() != "Tie") 
  {
    parProp = ci->property();
    tieProp = getTieProperty(parProp);
    if (!tieProp) return;
  }
  else
  {
    tieProp = ci->property();
    parProp = ci->parent()->property();
  }

  for(QMap<QtProperty*,Mantid::API::ParameterTie*>::iterator it = m_ties.begin();it!=m_ties.end();it++)
  {
    std::cerr<<it.key()->propertyName().toStdString()<<' '<<it.value()<<'\n';
  }

  QString parName = parProp->propertyName();
  Mantid::API::ParameterTie* tie = m_ties[tieProp];
  if (!tie) return;
  tie->getFunction()->removeTie(tie->getIndex());
  m_ties.remove(tieProp);
  parProp->removeSubProperty(tieProp);
  parProp->setEnabled(true);
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
void FitPropertyBrowser::addConstraint(int f,bool lo,bool up)
{
  QtBrowserItem * ci = m_browser->currentItem();
  QtProperty* parProp = ci->property();
  if (!m_paramItems.contains(parProp) || m_paramItems[parProp] != ci) return;
  QtBrowserItem* fnItem = ci->parent();
  if (!m_functionItems.contains(fnItem)) return;
  Mantid::API::IFunction* fun = m_functionItems[fnItem];
  int iPar = fun->parameterIndex(parProp->propertyName().toStdString());

  double x = m_doubleManager->value(parProp);
  double loBound = x*(1-0.01*f);
  double upBound = x*(1+0.01*f);
  Mantid::API::IConstraint* c_old = fun->firstConstraint();
  while(c_old)
  {
    if (c_old->getIndex() == iPar)
    {
      double lowerBound = 1;
      bool hasLo = false;
      double upperBound = 0;
      bool hasUp = false;
      extractLowerAndUpper(c_old->asString(),lowerBound,upperBound,hasLo,hasUp);
      if (hasLo && !lo)
      {
        lo = true;
        loBound = lowerBound;
      }
      if (hasUp && !up)
      {
        up = true;
        upBound = upperBound;
      }
    }
    c_old = fun->nextConstraint();
  }

  std::ostringstream ostr;
  if (lo) 
  {
    ostr << loBound << "<";
  }
  ostr << parProp->propertyName().toStdString();
  if (up)
  {
    ostr << "<" << upBound;
  }
  Mantid::API::IConstraint* c = Mantid::API::ConstraintFactory::Instance().createInitialized(fun,ostr.str());
  fun->addConstraint(c);
  checkFunction();
}

/**
 * Slot. Adds lower bound to the selected parameter property
 */
void FitPropertyBrowser::addLowerBound()
{
  addConstraint(0,true,false);
}

/**
 * Slot. Adds lower bound to the selected parameter property
 */
void FitPropertyBrowser::addLowerBound10()
{
  addConstraint(10,true,false);
}

/**
 * Slot. Adds lower bound to the selected parameter property
 */
void FitPropertyBrowser::addLowerBound50()
{
  addConstraint(50,true,false);
}

/**
 * Slot.Adds upper bound to the selected parameter property
 */
void FitPropertyBrowser::addUpperBound10()
{
  addConstraint(10,false,true);
}

/**
 * Slot.Adds upper bound to the selected parameter property
 */
void FitPropertyBrowser::addUpperBound50()
{
  addConstraint(50,false,true);
}

/**
 * Slot.Adds upper bound to the selected parameter property
 */
void FitPropertyBrowser::addUpperBound()
{
  addConstraint(0,false,true);
}

/**
 * Slot.Sets the lower and upper bounds of the selected parameter to 10% of its value
 */
void FitPropertyBrowser::addBothBounds10()
{
  addConstraint(10,true,true);
}

/**
 * Slot.Sets the lower and upper bounds of the selected parameter to 50% of its value
 */
void FitPropertyBrowser::addBothBounds50()
{
  addConstraint(50,true,true);
}

/**
 * Slot.Adds lower and upper bounds to the selected parameter property
 */
void FitPropertyBrowser::addBothBounds()
{
  addConstraint(0,true,true);
}


/**
 * Slot.Removes lower and upper bounds from the selected parameter property
 */
void FitPropertyBrowser::removeBounds()
{
  QtBrowserItem * ci = m_browser->currentItem();
  QtProperty* parProp = ci->property();
  QString parName = parProp->propertyName();
  if (parName == "Upper Bound" || parName == "Lower Bound" )
  {
    ci = ci->parent();
    parProp = ci->property();
  }
  QtBrowserItem* fnItem = ci->parent();
  if (!fnItem) return;
  if (!m_functionItems.contains(fnItem)) return;
  Mantid::API::IFunction* fun = m_functionItems[fnItem];
  if (!fun) return;
  fun->removeConstraint(parProp->propertyName().toStdString());
  checkFunction();
  return;
}

/**
 * Slot. Sends a signal to plot the guess for the current (selected) function
 */
void FitPropertyBrowser::plotGuessCurrent()
{
  emit plotGuess(function());
}

/**
 * Slot. Sends a signal to plot the guess for the whole function
 */
void FitPropertyBrowser::plotGuessAll()
{
  emit plotGuess(theFunction());
}

/// Remove all properties associated with a function
void FitPropertyBrowser::removeFunProperties(QtProperty* fnProp,bool doubleOnly)
{
  QList<QtProperty*> subs = fnProp->subProperties();
  for(int i = 0; i < subs.size(); i++)
  {
    QtProperty* parProp = subs[i];
    if (doubleOnly && parProp->propertyManager() != m_doubleManager) continue;
    QMap<QtProperty*,std::pair<QtProperty*,QtProperty*> >::iterator cit = m_constraints.find(parProp);
    if (cit != m_constraints.end())
    {
      m_constraints.remove(parProp);
    }
    fnProp->removeSubProperty(parProp);
    if (m_paramItems.contains(parProp))
    {
      m_paramItems.remove(parProp);
    }
  }
}
/** Add properties associated with a function: type, attributes, parameters
 * @param f A pointer to the function
 * @param doubleOnly If true only the fitting parameters will be added to the browser but not 
 *   attributes and Type. It is useful for UserFunction when its parameters have changed 
 */
void FitPropertyBrowser::addFunProperties(Mantid::API::IFunction* f,bool doubleOnly)
{
  m_changeSlotsEnabled = false;

  QtBrowserItem* fnItem = m_functionItems.key(f,NULL);
  if (fnItem == NULL) return;
  QtProperty* fnProp = fnItem->property();

  if (!doubleOnly)
  {
    QtProperty* typeProp = m_enumManager->addProperty("Type");
    fnProp->addSubProperty(typeProp);
    QtBrowserItem* typeItem = findItem(m_functionsGroup,typeProp);
    if (typeItem)
    {
      m_paramItems[typeProp] = typeItem;
    }

    int itype = m_registeredFunctions.indexOf(QString::fromStdString(f->name()));
    m_enumManager->setEnumNames(typeProp, m_registeredFunctions);
    m_enumManager->setValue(typeProp,itype);

    // Add attributes for the function's parameters
    std::vector<std::string> attr = f->getAttributeNames();
    for(size_t i=0;i<attr.size();i++)
    {
      std::string attName = attr[i];
      QtProperty* parProp = 0;

      if (attName == "FileName")
      {
        parProp = m_filenameManager->addProperty(QString::fromStdString(attName));
        fnProp->addSubProperty(parProp);
        m_filenameManager->setValue(parProp,QString::fromStdString(f->getAttribute(attName)));
      }
      else
      {
        parProp = m_stringManager->addProperty(QString::fromStdString(attName));
        fnProp->addSubProperty(parProp);
        m_stringManager->setValue(parProp,QString::fromStdString(f->getAttribute(attName)));
      }

      QtBrowserItem* attrItem = findItem(m_functionsGroup,parProp);
      if (attrItem)
      {
        m_paramItems[parProp] = attrItem;
      }
    }
  }

  // Add properties for the function's parameters
  for(int i=0;i<f->nParams();i++)
  {
    QtProperty* parProp = addDoubleProperty(QString::fromStdString(f->parameterName(i)));
    fnProp->addSubProperty(parProp);
    m_doubleManager->setValue(parProp,f->getParameter(i));
    QList<QtBrowserItem*> items = fnItem->children();
    foreach(QtBrowserItem* item,items)
    {
      if (item->property() == parProp)
      {
        m_paramItems[parProp] = item;
        break;
      }
    }
  }

  m_changeSlotsEnabled = true;
}

/** Create a double property and set some settings
 * @param name The name of the new property
 * @return Pointer to the created property
 */
QtProperty* FitPropertyBrowser::addDoubleProperty(const QString& name)const
{
  QtProperty* prop = m_doubleManager->addProperty(name);
  m_doubleManager->setDecimals(prop,6);
  return prop;
}

void FitPropertyBrowser::updateNames()
{
  QMap<QtBrowserItem*,Mantid::API::IFunction*>::iterator it = m_functionItems.begin();
  for(;it!=m_functionItems.end();it++)
  {
    it.key()->property()->setPropertyName(functionName(it.value()));
  }
}

/** Remove items from m_functionItems
 * @param fnItem The function item to remove. If it is connected to a CompositeFunction
 *   remove all its members
 */
void FitPropertyBrowser::removeFunctionItems(QtBrowserItem* fnItem)
{
  if ( !m_functionItems.contains(fnItem) ) return;
  Mantid::API::IFunction* fun = m_functionItems[fnItem];
  Mantid::API::CompositeFunction* cf = dynamic_cast<Mantid::API::CompositeFunction*>(fun);
  if (cf)
  {
    for(int i=0;i<cf->nFunctions();i++)
    {
      QtBrowserItem* fItem = m_functionItems.key(cf->getFunction(i));
      if (fItem) removeFunctionItems(fItem);
    }
  }
  QMap<QtProperty*,QtBrowserItem*>::iterator it = m_paramItems.begin();
  for(;it!=m_paramItems.end();)
  {
    if (it.value()->parent() == fnItem)
    {
      it = m_paramItems.erase(it);
      //it++;
    }
    else
    {
      it++;
    }
  }
  m_functionItems.remove(fnItem);
}

Mantid::API::IFunction* FitPropertyBrowser::theFunction()const
{
  return dynamic_cast<Mantid::API::CompositeFunction*>(m_compositeFunction);
}

void FitPropertyBrowser::checkFunction()
{
  m_compositeFunction->checkFunction();
  for(int i=0;i<m_compositeFunction->nParams();i++)
  {
    Mantid::API::ParameterReference ref(m_compositeFunction,i);
    Mantid::API::IFunction* fun = ref.getFunction();
    int iPar = ref.getIndex();
    QtProperty* parProp = getParameterProperty(fun,iPar);
    QList<QtProperty*> subs = parProp->subProperties();

    QtProperty* lowerProp = 0;
    QtProperty* upperProp = 0;
    QtProperty* tieProp = 0;
    Mantid::API::ParameterTie* tie = fun->getTie(iPar);
    Mantid::API::IConstraint* c = fun->firstConstraint();
    while(c)
    {
      if (c->getIndex() == iPar)
      {
        break;
      }
      c = fun->nextConstraint();
    }

    for(int j=0;j<subs.size();j++)
    {
      if (subs[j]->propertyName() == "Tie")
      {
        tieProp = subs[j];
      }
      if (subs[j]->propertyName() == "Lower Bound")
      {
        lowerProp = subs[j];
      }
      if (subs[j]->propertyName() == "Upper Bound")
      {
        upperProp = subs[j];
      }
    }

    if (tie)
    {
      if (!tieProp)
      {
        tieProp = m_stringManager->addProperty( "Tie" );
        parProp->addSubProperty(tieProp);
        m_ties[tieProp] = tie;
      }
      m_stringManager->setValue(tieProp,QString::fromStdString(tie->asString(m_compositeFunction)));
    }
    else
    {
      if (tieProp)
      {
        parProp->removeSubProperty(tieProp);
        m_ties.remove(tieProp);
      }
    }

    if (c)
    {
      bool hasLower = false;
      bool hasUpper = false;
      double lower,upper;
      extractLowerAndUpper(c->asString(),lower,upper,hasLower,hasUpper);
      if (hasUpper && !upperProp)
      {
        upperProp = addDoubleProperty("Upper Bound");
        parProp->addSubProperty(upperProp);
        std::pair<QtProperty*,QtProperty*>& cpair = m_constraints[parProp];
        cpair.second = upperProp;
        m_doubleManager->setValue(upperProp,upper);
      }
      if (hasLower && !lowerProp)
      {
        lowerProp = addDoubleProperty("Lower Bound");
        parProp->addSubProperty(lowerProp);
        std::pair<QtProperty*,QtProperty*>& cpair = m_constraints[parProp];
        cpair.first = lowerProp;
        m_doubleManager->setValue(lowerProp,lower);
      }
    }
    else // if !c
    {
      if (upperProp)
      {
        parProp->removeSubProperty(upperProp);
        m_constraints.remove(parProp);
      }
      if (lowerProp)
      {
        parProp->removeSubProperty(lowerProp);
        if (m_constraints.contains(parProp))
        {
          m_constraints.remove(parProp);
        }
      }
    }
  } // for i
}

/// Extracts lower and upper bounds form a string of the form 1<Sigma<3, or 1<Sigma or Sigma < 3
void FitPropertyBrowser::extractLowerAndUpper(const std::string& str,double& lo,double& up,bool& hasLo, bool& hasUp)const
{
  hasLo = false;
  hasUp = false;
  QStringList lst = QString::fromStdString(str).split("<");
  if (lst.size() == 3)
  {
    hasLo = true;
    hasUp = true;
    lo = lst[0].toDouble();
    up = lst[2].toDouble();
  }
  else if (lst.size() == 2)
  {
    lo = lst[0].toDouble(&hasLo);
    up = lst[1].toDouble(&hasUp);
  }

}
