#include "FitPropertyBrowser.h"
#include "PropertyHandler.h"
#include "SequentialFitDialog.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidAPI/CompositeFunctionMW.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/ConstraintFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/LibraryManager.h"

#include "MantidQtMantidWidgets/UserFunctionDialog.h"

#include "StringDialogEditorFactory.h"
#include "DoubleEditorFactory.h"

#include "qttreepropertybrowser.h"
#include "qtpropertymanager.h"
#include "qteditorfactory.h"

#include "../ApplicationWindow.h"
#include "MantidUI.h"
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

#include <algorithm>

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
  QWidget *createEditor(QtStringPropertyManager *manager, QtProperty *property,QWidget *parent)
  {
    (void) manager; //Avoid unused warning
    return new FormulaDialogEditor(property,parent);
  }
};

/**
 * Constructor
 * @param parent :: The parent widget - must be an ApplicationWindow
 */
FitPropertyBrowser::FitPropertyBrowser(QWidget* parent)
:QDockWidget("Fit Function",parent),
m_appWindow((ApplicationWindow*)parent),
m_currentHandler(0),
m_logValue(NULL),
m_compositeFunction(0),
m_defaultFunction("Gaussian"),
m_defaultPeak("Gaussian"),
m_defaultBackground("LinearBackground"),
m_guessOutputName(true),
m_changeSlotsEnabled(false),
m_peakToolOn(false),
m_auto_back(false),
m_autoBgName(QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("curvefitting.autoBackground"))),
m_autoBackground(NULL),
m_decimals(-1)
{
  // Make sure plugins are loaded
  std::string libpath = Mantid::Kernel::ConfigService::Instance().getString("plugins.directory");
  if( !libpath.empty() )
  {
    Mantid::Kernel::LibraryManager::Instance().OpenAllLibraries(libpath);
  }

  // Try to create a Gaussian. Failing will mean that CurveFitting dll is not loaded
  boost::shared_ptr<Mantid::API::IFitFunction> f = boost::shared_ptr<Mantid::API::IFitFunction>(
    Mantid::API::FunctionFactory::Instance().createFitFunction("Gaussian"));
  if (m_autoBgName.toLower() == "none")
  {
    m_autoBgName = "";
  }
  else
  {
    setAutoBackgroundName(m_autoBgName);
  }

  std::string def = Mantid::Kernel::ConfigService::Instance().getString("curvefitting.defaultPeak");
  if (!def.empty())
  {
    m_defaultPeak = def;
  }

  def = Mantid::Kernel::ConfigService::Instance().getString("curvefitting.autoBackground");
  if (!def.empty())
  {
    m_defaultBackground = def;
  }
  m_defaultFunction = m_defaultPeak;

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
  m_formulaManager = new QtStringPropertyManager(w);

    /* Create the top level group */

  /*QtProperty* fitGroup = */m_groupManager->addProperty("Fit");

  connect(m_enumManager,SIGNAL(propertyChanged(QtProperty*)),this,SLOT(enumChanged(QtProperty*)));
  connect(m_boolManager,SIGNAL(propertyChanged(QtProperty*)),this,SLOT(boolChanged(QtProperty*)));
  connect(m_intManager,SIGNAL(propertyChanged(QtProperty*)),this,SLOT(intChanged(QtProperty*)));
  connect(m_doubleManager,SIGNAL(propertyChanged(QtProperty*)),this,SLOT(doubleChanged(QtProperty*)));
  connect(m_stringManager,SIGNAL(propertyChanged(QtProperty*)),this,SLOT(stringChanged(QtProperty*)));
  connect(m_filenameManager,SIGNAL(propertyChanged(QtProperty*)),this,SLOT(stringChanged(QtProperty*)));
  connect(m_formulaManager,SIGNAL(propertyChanged(QtProperty*)),this,SLOT(stringChanged(QtProperty*)));

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

  m_costFunction = m_enumManager->addProperty("Cost function");
  m_costFunctions << "Least squares"
                  << "Ignore positive peaks";
  m_enumManager->setEnumNames(m_costFunction,m_costFunctions);

  settingsGroup->addSubProperty(m_workspace);
  settingsGroup->addSubProperty(m_workspaceIndex);
  settingsGroup->addSubProperty(m_startX);
  settingsGroup->addSubProperty(m_endX);
  settingsGroup->addSubProperty(m_output);
  settingsGroup->addSubProperty(m_minimizer);
  settingsGroup->addSubProperty(m_costFunction);

     /* Create editors and assign them to the managers */

  QtCheckBoxFactory *checkBoxFactory = new QtCheckBoxFactory(w);
  QtEnumEditorFactory *comboBoxFactory = new QtEnumEditorFactory(w);
  QtSpinBoxFactory *spinBoxFactory = new QtSpinBoxFactory(w);
  //QtDoubleSpinBoxFactory *doubleSpinBoxFactory = new QtDoubleSpinBoxFactory(w); //unused now
  DoubleEditorFactory *doubleEditorFactory = new DoubleEditorFactory(w);
  QtLineEditFactory *lineEditFactory = new QtLineEditFactory(w);
  StringDialogEditorFactory* stringDialogEditFactory = new StringDialogEditorFactory(w);
  FormulaDialogEditorFactory* formulaDialogEditFactory = new FormulaDialogEditorFactory(w);

  m_browser = new QtTreePropertyBrowser();
  m_browser->setFactoryForManager(m_enumManager, comboBoxFactory);
  m_browser->setFactoryForManager(m_boolManager, checkBoxFactory);
  m_browser->setFactoryForManager(m_intManager, spinBoxFactory);
  //m_browser->setFactoryForManager(m_doubleManager, doubleSpinBoxFactory);
  m_browser->setFactoryForManager(m_doubleManager, doubleEditorFactory);
  m_browser->setFactoryForManager(m_stringManager, lineEditFactory);
  m_browser->setFactoryForManager(m_filenameManager, stringDialogEditFactory);
  m_browser->setFactoryForManager(m_formulaManager, formulaDialogEditFactory);

  updateDecimals();

  m_functionsGroup = m_browser->addProperty(functionsGroup);
  m_settingsGroup = m_browser->addProperty(settingsGroup);

  QVBoxLayout* layout = new QVBoxLayout(w);
  QGridLayout* buttonsLayout = new QGridLayout();

  m_btnFit = new QPushButton("Fit");
  connect(m_btnFit,SIGNAL(clicked()),this,SLOT(fit()));

  m_btnUnFit = new QPushButton("Undo Fit");
  connect(m_btnUnFit,SIGNAL(clicked()),this,SLOT(undoFit()));

  QPushButton* btnClear = new QPushButton("Clear all");
  connect(btnClear,SIGNAL(clicked()),this,SLOT(clear()));

  m_btnSeqFit = new QPushButton("Sequential fit");
  connect(m_btnSeqFit,SIGNAL(clicked()),this,SLOT(sequentialFit()));

  m_btnFindPeaks = new QPushButton("Find peaks");
  connect(m_btnFindPeaks,SIGNAL(clicked()),this,SLOT(findPeaks()));

  m_btnPlotGuess = new QPushButton("Plot guess");
  connect(m_btnPlotGuess,SIGNAL(clicked()),this,SLOT(plotOrRemoveGuessAll()));
  m_btnPlotGuess->setEnabled(false);

  m_tip = new QLabel("",w);

  buttonsLayout->addWidget(m_btnFit,0,0);
  buttonsLayout->addWidget(m_btnUnFit,0,1);
  buttonsLayout->addWidget(btnClear,0,2);
  buttonsLayout->addWidget(m_btnSeqFit,1,0);
  buttonsLayout->addWidget(m_btnFindPeaks,1,1);
  buttonsLayout->addWidget(m_btnPlotGuess,1,2);

  layout->addLayout(buttonsLayout);
  layout->addWidget(m_tip);
  layout->addWidget(m_browser);

  setWidget(w);

  m_browser->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_browser, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(popupMenu(const QPoint &)));
  connect(m_browser, SIGNAL(currentItemChanged(QtBrowserItem*)), this, SLOT(currentItemChanged(QtBrowserItem*)));

  createCompositeFunction();

  m_changeSlotsEnabled = true;

}

/// Destructor
FitPropertyBrowser::~FitPropertyBrowser()
{
  if (m_compositeFunction) delete m_compositeFunction;
}

/// Get handler to the root composite function
PropertyHandler* FitPropertyBrowser::getHandler()const
{
  return static_cast<PropertyHandler*>(m_compositeFunction->getHandler());
}

PropertyHandler* FitPropertyBrowser::addFunction(const std::string& fnName)
{
  PropertyHandler* h = getHandler()->addFunction(fnName);
  emit functionChanged();
  return h;
}

/** Slot. Called to add a new function
 */
void FitPropertyBrowser::addFunction()
{
  QtBrowserItem * ci = m_browser->currentItem();
  // Find the function which has ci as its top browser item 
  const Mantid::API::CompositeFunction* cf = getHandler()->findCompositeFunction(ci);
  if ( !cf ) return;
  int i = m_registeredFunctions.indexOf(QString::fromStdString(m_defaultFunction));
  bool ok = false;
  QString fnName = 
    QInputDialog::getItem(this, "MantidPlot - Fit", "Select function type", m_registeredFunctions,i,false,&ok);
  if (ok)
  {
    PropertyHandler* h = getHandler()->findHandler(cf);
    h->addFunction(fnName.toStdString());
  }
  emit functionChanged();
}

/// Create CompositeFunction
void FitPropertyBrowser::createCompositeFunction(const QString& str)
{
 if (m_compositeFunction)
  {
    emit functionRemoved();
    delete m_compositeFunction;
    m_autoBackground = NULL;
  }
  if (str.isEmpty())
  {
    m_compositeFunction = new Mantid::API::CompositeFunctionMW();
  }
  else
  {
    Mantid::API::IFitFunction* f = Mantid::API::FunctionFactory::Instance().createInitialized(str.toStdString());
    if (!f)
    {
      createCompositeFunction();
      return;
    }
    Mantid::API::CompositeFunction* cf = dynamic_cast<Mantid::API::CompositeFunction*>(f);
    if (!cf || cf->name() != "CompositeFunctionMW")
    {
      m_compositeFunction = new Mantid::API::CompositeFunctionMW();
      m_compositeFunction->addFunction(f);
    }
    else
    {
      m_compositeFunction = cf;
    }
  }
  setWorkspace(m_compositeFunction);

  PropertyHandler* h = new PropertyHandler(m_compositeFunction,NULL,this);
  m_compositeFunction->setHandler(h);
  setCurrentFunction(h);

  if (m_auto_back)
  {
    addAutoBackground();
  }

  disableUndo();
  setFitEnabled(m_compositeFunction->nFunctions() > 0);
  emit functionChanged();
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
  bool isFunction = getHandler()->findFunction(ci) != NULL;
  bool isCompositeFunction = isFunction && getHandler()->findCompositeFunction(ci);

  //if (!isFunction)
  //{
  //  const Mantid::API::IFitFunction* h = getHandler()->findFunction(ci);
  //}

  PropertyHandler* h = getHandler()->findHandler(ci->property());

  if (isFunctionsGroup)
  {
    action = new QAction("Add function",this);
    connect(action,SIGNAL(triggered()),this,SLOT(addFunction()));
    menu->addAction(action);

    if (m_peakToolOn)
    {
      if (h && h->hasPlot())
      {
        action = new QAction("Remove plot",this);
        connect(action,SIGNAL(triggered()),this,SLOT(removeGuessAll()));
        menu->addAction(action);
      }
      else
      {
        action = new QAction("Plot",this);
        connect(action,SIGNAL(triggered()),this,SLOT(plotGuessAll()));
        menu->addAction(action);
      }
    }

    menu->addSeparator();

    action = new QAction("Save",this);
    connect(action,SIGNAL(triggered()),this,SLOT(saveFunction()));
    menu->addAction(action);

    action = new QAction("Load",this);
    connect(action,SIGNAL(triggered()),this,SLOT(loadFunction()));
    menu->addAction(action);

    action = new QAction("Copy",this);
    connect(action,SIGNAL(triggered()),this,SLOT(copy()));
    menu->addAction(action);

    //action = new QAction("Paste",this);
    //connect(action,SIGNAL(triggered()),this,SLOT(paste()));
    //menu->addAction(action);

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
      if (h && h->hasPlot())
      {
        action = new QAction("Remove plot",this);
        connect(action,SIGNAL(triggered()),this,SLOT(removeGuessCurrent()));
        menu->addAction(action);
      }
      else
      {
        action = new QAction("Plot",this);
        connect(action,SIGNAL(triggered()),this,SLOT(plotGuessCurrent()));
        menu->addAction(action);
      }
    }

    menu->addSeparator();
  }
  else if (h)
  {
    bool isParameter = h->isParameter(ci->property());
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
    else if (count() > 0 && isParameter)
    {
      bool hasTies;
      bool hasBounds;
      hasConstraints(ci->property(),hasTies,hasBounds);

      if (!hasTies && !hasBounds)
      {
        action = new QAction("Fix",this);
        connect(action,SIGNAL(triggered()),this,SLOT(addFixTie()));
        menu->addAction(action);
      }

      if (!hasTies)
      {
        QMenu *constraintMenu = menu->addMenu("Constraint");

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
        detailMenu = constraintMenu->addMenu("Upper Bound");

        action = new QAction("10%",this);
        connect(action,SIGNAL(triggered()),this,SLOT(addUpperBound10()));
        detailMenu->addAction(action);

        action = new QAction("50%",this);
        connect(action,SIGNAL(triggered()),this,SLOT(addUpperBound50()));
        detailMenu->addAction(action);

        action = new QAction("Custom",this);
        connect(action,SIGNAL(triggered()),this,SLOT(addUpperBound()));
        detailMenu->addAction(action);
        detailMenu = constraintMenu->addMenu("Both Bounds");

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

      if (hasBounds)
      {
        action = new QAction("Remove constraints",this);
        connect(action,SIGNAL(triggered()),this,SLOT(removeBounds()));
        menu->addAction(action);
      }

      if (!hasTies && !hasBounds)
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
      else if (hasTies)
      {
        action = new QAction("Remove tie",this);
        connect(action,SIGNAL(triggered()),this,SLOT(deleteTie()));
        menu->addAction(action);
      }
    }
  }

  menu->popup(QCursor::pos());
}

/** Slot. Called to remove a function
 */
void FitPropertyBrowser::deleteFunction()
{
  QtBrowserItem* ci = m_browser->currentItem();
  PropertyHandler* h = getHandler()->findHandler(ci->property());
  if (h)
  {
    getHandler()->removePlot();
    h->removeFunction();
    compositeFunction()->checkFunction();
    emit functionRemoved();
    emit functionChanged();
  }
}

//***********************************************************************************//


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

/// Get the default peak type
std::string FitPropertyBrowser::defaultPeakType()const
{
  return m_defaultPeak;
}
/// Set the default peak type
void FitPropertyBrowser::setDefaultPeakType(const std::string& fnType)
{
  m_defaultPeak = fnType;
  setDefaultFunctionType(fnType);
  Mantid::Kernel::ConfigService::Instance().setString("curvefitting.defaultPeak", fnType);
}
/// Get the default background type
std::string FitPropertyBrowser::defaultBackgroundType()const
{
  return m_defaultBackground;
}
/// Set the default background type
void FitPropertyBrowser::setDefaultBackgroundType(const std::string& fnType)
{
  m_defaultBackground = fnType;
  setDefaultFunctionType(fnType);
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
  if (!isWorkspaceAGroup())
  {
    m_groupMember = wsName.toStdString();
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

/// Get the cost function
std::string FitPropertyBrowser::costFunction()const
{
  int i = m_enumManager->value(m_costFunction);
  return m_costFunctions[i].toStdString();
}

/** Called when the function name property changed
 * @param prop :: A pointer to the function name property m_functionName
 */
void FitPropertyBrowser::enumChanged(QtProperty* prop)
{
  if ( ! m_changeSlotsEnabled ) return;

  if (prop == m_workspace)
  {
    if (m_guessOutputName)
    {
      if (isWorkspaceAGroup())
      {
        m_stringManager->setValue(m_output,QString::fromStdString(workspaceName()+"_params"));
      }
      else
      {
        m_stringManager->setValue(m_output,QString::fromStdString(workspaceName()));
      }
    }
    if (isWorkspaceAGroup())
    {
      setLogValue();
    }
    else
    {
      m_groupMember = workspaceName();
      removeLogValue();
    }
    emit workspaceNameChanged(QString::fromStdString(workspaceName()));
  }
  else if (prop->propertyName() == "Type")
  {
      disableUndo();
      PropertyHandler* h = getHandler()->findHandler(prop);
      if (!h) return;
      if (!h->parentHandler()) return;
      Mantid::API::IFitFunction* f = h->changeType(prop);
      if (f) setCurrentFunction(f);
      emit functionChanged();
  }
}

/** Called when a bool property changed
 * @param prop :: A pointer to the property 
 */
void FitPropertyBrowser::boolChanged(QtProperty*)
{
  if ( ! m_changeSlotsEnabled ) return;

}

/** Called when an int property changed
 * @param prop :: A pointer to the property 
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
  else
  {// it could be an attribute
    PropertyHandler* h = getHandler()->findHandler(prop);
    if (!h) return;
    h->setAttribute(prop);
  }
}

/** Called when a double property changed
 * @param prop :: A pointer to the property 
 */
void FitPropertyBrowser::doubleChanged(QtProperty* prop)
{
  if ( ! m_changeSlotsEnabled ) return;

  double value = m_doubleManager->value(prop);
  if (prop == m_startX )
  {
    // call setWorkspace to change maxX in functions
    setWorkspace(m_compositeFunction);
    getHandler()->setAttribute("StartX",value);
    emit startXChanged(startX());
    return;
  }
  else if (prop == m_endX )
  {
    // call setWorkspace to change minX in functions
    setWorkspace(m_compositeFunction);
    getHandler()->setAttribute("EndX",value);
    emit endXChanged(endX());
    return;
  }
  else if(getHandler()->setParameter(prop))
  {
    return;
  }
  else
  {// check if it is a constraint
    PropertyHandler* h = getHandler()->findHandler(prop);
    if (!h) return;

    QtProperty* parProp = h->getParameterProperty(prop);
    if (parProp)
    {
      if (prop->propertyName() == "LowerBound")
      {
        double loBound = m_doubleManager->value(prop);
        h->addConstraint(parProp,true,false,loBound,0);
      }
      else if (prop->propertyName() == "UpperBound")
      {
        double upBound = m_doubleManager->value(prop);
        h->addConstraint(parProp,false,true,0,upBound);
      }
    }
    else
    {// it could be an attribute
      h->setAttribute(prop);
    }
  }
}
/** Called when a string property changed
 * @param prop :: A pointer to the property 
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
    PropertyHandler* h = getHandler()->findHandler(prop);
    if (!h) return;

    QtProperty* parProp = h->getParameterProperty(prop);
    if (!parProp) return;

    QString parName = h->functionPrefix()+"."+parProp->propertyName();

    QString str = m_stringManager->value(prop);
    Mantid::API::ParameterTie* tie = 
      new Mantid::API::ParameterTie(compositeFunction(),parName.toStdString());
    try
    {
      tie->set(str.toStdString());
      h->addTie(parName+"="+str);
    }
    catch(...){std::cerr<<"Failed\n";}
    delete tie;
  }
  else if (getHandler()->setAttribute(prop))
  {// setting an attribute may change function parameters
    emit functionChanged();
    return;
  }
}

/** Called when a filename property changed
 * @param prop :: A pointer to the property 
 */
void FitPropertyBrowser::filenameChanged(QtProperty* prop)
{
  if ( ! m_changeSlotsEnabled ) return;

  if (getHandler()->setAttribute(prop))
  {
    return;
  }
}
// Centre of the current peak
double FitPropertyBrowser::centre()const
{
  if (m_currentHandler && m_currentHandler->pfun())
  {
    return m_currentHandler->pfun()->centre();
  }
  return 0;
}

/** Set centre of the current peak
 * @param value :: The new centre value
 */
void FitPropertyBrowser::setCentre(double value)
{
  if (m_currentHandler)
  {
    m_currentHandler->setCentre(value);
  }
}

// Height of the current peak
double FitPropertyBrowser::height()const
{
  if (m_currentHandler && m_currentHandler->pfun())
  {
    return m_currentHandler->pfun()->height();
  }
  return 0.;
}

/** Set height of the current peak
 * @param value :: The new height value
 */
void FitPropertyBrowser::setHeight(double value)
{
  if (m_currentHandler)
  {
    m_currentHandler->setHeight(value);
  }
}

// Width of the current peak
double FitPropertyBrowser::width()const
{
  if (m_currentHandler && m_currentHandler->pfun())
  {
    return m_currentHandler->pfun()->width();
  }
  return 0;
}

/** Set width of the current peak
 * @param value :: The new width value
 */
void FitPropertyBrowser::setWidth(double value)
{
  if (m_currentHandler)
  {
    m_currentHandler->setWidth(value);
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
    QString qfnName = QString::fromStdString(fnName);
    m_registeredFunctions << qfnName;
    boost::shared_ptr<Mantid::API::IFitFunction> f = boost::shared_ptr<Mantid::API::IFitFunction>(
      Mantid::API::FunctionFactory::Instance().createFitFunction(fnName));
    Mantid::API::IPeakFunction* pf = dynamic_cast<Mantid::API::IPeakFunction*>(f.get());
    //Mantid::API::CompositeFunction* cf = dynamic_cast<Mantid::API::CompositeFunction*>(f.get());
    if (pf)
    {
      m_registeredPeaks << qfnName;
    }
    else if (dynamic_cast<Mantid::API::IBackgroundFunction*>(f.get()))
    {
      m_registeredBackgrounds << qfnName;
    }
    else
    {
      m_registeredOther << qfnName;
    }
  }
}


/// Get number of functions in CompositeFunction
int FitPropertyBrowser::count()const
{
  return m_compositeFunction->nFunctions();
}

/// Get the current function
PropertyHandler* FitPropertyBrowser::currentHandler()const
{
  return m_currentHandler;
}

/** Set new current function
 * @param h :: New current function
 */
void FitPropertyBrowser::setCurrentFunction(PropertyHandler* h)const
{
  m_currentHandler = h;
  if (m_currentHandler)
  {
    m_browser->setCurrentItem(m_currentHandler->item());
    emit currentChanged();
  }
}

/** Set new current function
 * @param f :: New current function
 */
void FitPropertyBrowser::setCurrentFunction(const Mantid::API::IFitFunction* f)const
{
  setCurrentFunction(getHandler()->findHandler(f));
}

#include "../FitDialog.h"
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
      m_initialParameters[i] = compositeFunction()->getParameter(i);
    }
    m_btnUnFit->setEnabled(true);

    std::string funStr;
    if (m_compositeFunction->nFunctions() > 1)
    {
      funStr = *m_compositeFunction;
    }
    else
    {
      funStr = *(m_compositeFunction->getFunction(0));
    }

    if (isWorkspaceAGroup())
    {
      Mantid::API::IAlgorithm_sptr alg = 
        Mantid::API::AlgorithmManager::Instance().create("PlotPeakByLogValue");
      alg->initialize();
      alg->setPropertyValue("InputWorkspace",wsName);
      alg->setProperty("WorkspaceIndex",workspaceIndex());
      alg->setProperty("StartX",startX());
      alg->setProperty("EndX",endX());
      alg->setPropertyValue("OutputWorkspace",outputName());
      alg->setPropertyValue("Function",funStr);
      alg->setPropertyValue("LogValue",getLogValue());
      //alg->setPropertyValue("Minimizer",minimizer());
      //alg->setPropertyValue("CostFunction",costFunction());

      observeFinish(alg);
      alg->executeAsync();
    }
    else
    {
      Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("Fit");
      alg->initialize();
      alg->setPropertyValue("InputWorkspace",wsName);
      alg->setProperty("WorkspaceIndex",workspaceIndex());
      alg->setProperty("StartX",startX());
      alg->setProperty("EndX",endX());
      alg->setPropertyValue("Output",outputName());
      alg->setPropertyValue("Function",funStr);
      alg->setPropertyValue("Minimizer",minimizer());
      alg->setPropertyValue("CostFunction",costFunction());

      observeFinish(alg);
      alg->executeAsync();
    }
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
  if (!isWorkspaceAGroup())
  {
    emit algorithmFinished(QString::fromStdString(out));
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
  connect(m_appWindow->mantidUI,SIGNAL(workspace_added(const QString &, Mantid::API::Workspace_sptr)),
    this,SLOT(workspace_added(const QString &, Mantid::API::Workspace_sptr)));
  connect(m_appWindow->mantidUI,SIGNAL(workspace_removed(const QString &)),
    this,SLOT(workspace_removed(const QString &)));
}

/** Check if the workspace can be used in the fit. The accepted types are
  * MatrixWorkspaces same size
  * @param ws :: The workspace
  */
bool FitPropertyBrowser::isWorkspaceValid(Mantid::API::Workspace_sptr ws)const
{
  if (dynamic_cast<Mantid::API::MatrixWorkspace*>(ws.get()) != 0)
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool FitPropertyBrowser::isWorkspaceAGroup()const
{
  // MG: Disabled as there is an issue with replacing workspace groups and the browser
  return false;
}

/// Is the current function a peak?
bool FitPropertyBrowser::isPeak()const
{
  if (count() == 0)
  {
    return false;
  }
  return m_currentHandler && m_currentHandler->pfun();
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
  if (current)
  {
    m_currentHandler = getHandler()->findHandler(current->property());
  }
  else
  {
    m_currentHandler = NULL;
  }
  emit currentChanged();
}

/** Update the function parameter properties. 
 */
void FitPropertyBrowser::updateParameters()
{
  getHandler()->updateParameters();
}

/**
 * Slot. Removes all functions.
 */
void FitPropertyBrowser::clear()
{
  getHandler()->removeAllPlots();
  clearBrowser();
  createCompositeFunction();
  emit functionCleared();
}

void FitPropertyBrowser::clearBrowser()
{
  QList<QtProperty*> props = m_functionsGroup->property()->subProperties();
  QtProperty* prop;
  foreach(prop,props)
  {
    m_functionsGroup->property()->removeSubProperty(prop);
  }
}

/// Set the parameters to the fit outcome
void FitPropertyBrowser::getFitResults()
{
  if (isWorkspaceAGroup())
  {
    std::string wsName = outputName();
    Mantid::API::ITableWorkspace_sptr ws = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(
      Mantid::API::AnalysisDataService::Instance().retrieve(wsName) );
    if (ws)
    {
      if ((ws->columnCount() - 1)/2 != compositeFunction()->nParams()) return;
      Mantid::API::WorkspaceGroup_sptr wsg = boost::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(
        Mantid::API::AnalysisDataService::Instance().retrieve(workspaceName()) );
      std::vector<std::string> names = wsg->getNames();
      std::vector<std::string>::iterator it = 
        std::find(names.begin(),names.end(),m_groupMember);
      if (it == names.end()) return;
      int row = int(it - names.begin()) - 1;// take into account the group name
      if (row >= ws->rowCount()) return;
      for(int i=0;i<compositeFunction()->nParams();++i)
      {
        compositeFunction()->setParameter(i,ws->Double(row,2*i+1));
      }
      updateParameters();
      plotGuessAll();
    }
  }
  else
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
}

/**
 * Slot. Undoes the fit: restores the parameters to their initial values.
 */
void FitPropertyBrowser::undoFit()
{
  if (static_cast<int>(m_initialParameters.size()) == compositeFunction()->nParams())
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
  return m_initialParameters.size() && compositeFunction()->nParams() == static_cast<int>(m_initialParameters.size());
}

/// Enable/disable the Fit button;
void FitPropertyBrowser::setFitEnabled(bool yes)
{
  m_btnFit->setEnabled(yes);
  m_btnSeqFit->setEnabled(yes);
}

/// Returns true if the function is ready for a fit
bool FitPropertyBrowser::isFitEnabled()const
{
  return m_btnFit->isEnabled();
}

/** 
 * Slot. Adds a tie. Full expression to be entered <name>=<formula>
 */
void FitPropertyBrowser::addTie()
{
  QtBrowserItem * ci = m_browser->currentItem();
  QtProperty* paramProp = ci->property();
  PropertyHandler* h = getHandler()->findHandler(paramProp);
  if (!h->isParameter(paramProp)) return;
  if (!h) return;

  const Mantid::API::IFitFunction* f = h->function();
  if (!f) return;

  bool ok = false;
  QString tieStr = 
    QInputDialog::getText(this, "MantidPlot - Fit", "Enter tie expression", QLineEdit::Normal,"",&ok);
  if (ok)
  {
    tieStr = tieStr.trimmed();
    if (!tieStr.contains('='))
    {
      tieStr = h->functionPrefix()+"."+paramProp->propertyName() + "=" + tieStr;
    }
    h->addTie(tieStr);
  } // if (ok)
}

/** 
 * Slot. Ties a parameter to a parameter with the same name of a different function
 */
void FitPropertyBrowser::addTieToFunction()
{
  QtBrowserItem * ci = m_browser->currentItem();
  QtProperty* paramProp = ci->property();
  PropertyHandler* h = getHandler()->findHandler(paramProp);
  if (!h) return;
  if (!h->isParameter(paramProp)) return;
  std::string parName = paramProp->propertyName().toStdString();
  QStringList fnNames;

  int iPar = -1;
  for(int i=0;i<m_compositeFunction->nParams();i++)
  {
    Mantid::API::ParameterReference ref(m_compositeFunction,i);
    Mantid::API::IFitFunction* fun = ref.getFunction();
    // Pick out parameters with the same name as the one we're tying from
    if ( fun->parameterName(ref.getIndex()) == parName )
    {
      if ( iPar == -1 && fun == h->function() ) // If this is the 'tied from' parameter, remember it
      {
        iPar = i;
      }
      else  // Otherwise add it to the list of potential 'tyees'
      {
        fnNames << QString::fromStdString(m_compositeFunction->parameterName(i));
      }
    }
  }
  if (fnNames.empty() || iPar < 0)
  {
    QMessageBox::information(m_appWindow,"Mantid - information","Cannot tie this parameter to any function");
    return;
  }

  bool ok;
  QString tieName =
    QInputDialog::getItem(this, "MantidPlot - Fit", "Select function", fnNames,0,false,&ok);

  if (!ok) return;

  QString tieExpr = QString::fromStdString(m_compositeFunction->parameterName(iPar)) + "=" + tieName;

  h->addTie(tieExpr);

}

/** 
 * Slot. Adds a tie. The current item must be a function parameter
 */
void FitPropertyBrowser::addFixTie()
{
  QtBrowserItem * ci = m_browser->currentItem();
  QtProperty* paramProp = ci->property();
  PropertyHandler* h = getHandler()->findHandler(paramProp);
  if (!h) return;
  if (!h->isParameter(paramProp)) return;
  h->fix(paramProp->propertyName());
}

/** 
 * Slot. Deletes a tie. 
 */
void FitPropertyBrowser::deleteTie()
{
  QtBrowserItem * ci = m_browser->currentItem();
  QtProperty* paramProp = ci->property();
  PropertyHandler* h = getHandler()->findHandler(paramProp);

  if (ci->property()->propertyName() != "Tie") 
  {
    h->removeTie(ci->property()->propertyName());
  }
  else
  {
    h->removeTie(ci->property());
  }
}

/** Does a parameter have a tie
 * @param parProp :: The property for a function parameter
 */
void FitPropertyBrowser::hasConstraints(QtProperty* parProp,
                                        bool& hasTie,
                                        bool& hasBounds)const
{
  hasTie = false;
  hasBounds = false;
  QList<QtProperty*> subs = parProp->subProperties();
  for(int i=0;i<subs.size();i++)
  {
    if (subs[i]->propertyName() == "Tie")
    {
      hasTie = true;
    }
    if (subs[i]->propertyName() == "LowerBound")
    {
      hasBounds = true;
    }
    if (subs[i]->propertyName() == "UpperBound")
    {
      hasBounds = true;
    }
  }
}

/** Returns the tie property for a parameter property, or NULL
 * @param The :: parameter property
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
 * @param txt :: The text to display
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
  PropertyHandler* h = getHandler()->findHandler(parProp);
  if (!h) return;

  double x = m_doubleManager->value(parProp);
  double loBound = x*(1-0.01*f);
  double upBound = x*(1+0.01*f);

  h->addConstraint(ci->property(),lo,up,loBound,upBound);
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
  PropertyHandler* h = getHandler()->findHandler(parProp);
  if (!h) return;

  h->removeConstraint(parProp);
}

/**
 * Slot. Sends a signal to plot the guess for the current (selected) function
 */
void FitPropertyBrowser::plotGuessCurrent()
{
  emit plotCurrentGuess();
}

/**
 * Slot. Sends a signal to plot the guess for the whole function
 */
void FitPropertyBrowser::plotGuessAll()
{
  emit plotGuess();
}

/**
 * Slot. Sends a signal to remove the guess for the current (selected) function
 */
void FitPropertyBrowser::removeGuessCurrent()
{
  emit removeCurrentGuess();
}

/**
 * Slot. Sends a signal to remove the guess for the whole function
 */
void FitPropertyBrowser::removeGuessAll()
{
  emit removeGuess();
}

void FitPropertyBrowser::plotOrRemoveGuessAll()
{
  if (getHandler()->hasPlot())
  {
    removeGuessAll();
  }
  else
  {
    plotGuessAll();
  }
}

/** Create a double property and set some settings
 * @param name :: The name of the new property
 * @return Pointer to the created property
 */
QtProperty* FitPropertyBrowser::addDoubleProperty(const QString& name)const
{
  QtProperty* prop = m_doubleManager->addProperty(name);
  m_doubleManager->setDecimals(prop,m_decimals);
  m_doubleManager->setRange(prop,-DBL_MAX,DBL_MAX);
  return prop;
}

/** Create a string property and selects a property manager for it
 * based on the property name
 * @param name :: The name of the new property
 * @return Pointer to the created property
 */
QtProperty* FitPropertyBrowser::addStringProperty(const QString& name)const
{
  QtProperty* prop;
  QString propName = name.toLower();
  if (propName == "filename")
  {
    prop = m_filenameManager->addProperty(name);
  }
  else if (propName == "formula")
  {
    //!!! dont forget to change the manager !!!
    prop = m_formulaManager->addProperty(name);
  }
  else
  {
    prop = m_stringManager->addProperty(name);
  }
  return prop;
}

/**
 * Set a value to a string property.
 * @param prop :: A pointer to the property
 * @param value :: New value for the property
 */
void FitPropertyBrowser::setStringPropertyValue(QtProperty* prop,const QString& value)const
{
  QtStringPropertyManager* manager = dynamic_cast<QtStringPropertyManager*>(prop->propertyManager());
  if (manager)
  {
    manager->setValue(prop,value);
  }
}

QString FitPropertyBrowser::getStringPropertyValue(QtProperty* prop)const
{
  QtStringPropertyManager* manager = dynamic_cast<QtStringPropertyManager*>(prop->propertyManager());
  if (manager)
    return manager->value(prop);
  else
    return QString("");
}

const Mantid::API::IFitFunction* FitPropertyBrowser::theFunction()const
{
  return dynamic_cast<Mantid::API::CompositeFunction*>(m_compositeFunction);
}

void FitPropertyBrowser::checkFunction()
{

}

void FitPropertyBrowser::saveFunction()
{
  QString fnName = QInputDialog::getText(this,"Mantid - Input","Please select a name for the function");
  QSettings settings;
  settings.beginGroup("Mantid/FitBrowser/SavedFunctions");
  QStringList names = settings.childKeys();
  if (names.contains(fnName) && QMessageBox::question(this,"Mantid - Question","Function with this name already exists.\n"
    "Would you like to replace it?",QMessageBox::Yes) != QMessageBox::Yes)
  {
    return;
  }
  settings.setValue(fnName,QString::fromStdString(*theFunction()));
}

void FitPropertyBrowser::loadFunction()
{
  QSettings settings;
  settings.beginGroup("Mantid/FitBrowser/SavedFunctions");
  QStringList names = settings.childKeys();
  if (names.isEmpty())
  {
    QMessageBox::information(this,"Mantid - Information","There are no saved functions");
    return;
  }
  QString name = QInputDialog::getItem(this,"Mantid - Input","Please select a function to load",names,0,false);
  if (!name.isEmpty())
  {
    QString str = settings.value(name).toString();
  
    getHandler()->removeAllPlots();
    clearBrowser();
    createCompositeFunction(str);
  }
}

void FitPropertyBrowser::copy()
{
  QClipboard *clipboard = QApplication::clipboard();
  clipboard->setText(QString::fromStdString(*theFunction()));
}

void FitPropertyBrowser::paste()
{
  QClipboard *clipboard = QApplication::clipboard();
  QString str = clipboard->text();
  createCompositeFunction(str);
}

void FitPropertyBrowser::reset()
{
  QString str = QString::fromStdString(*theFunction());
  //getHandler()->removeAllPlots();// this crashes mantidplot
  clearBrowser();
  createCompositeFunction(str);
}

void FitPropertyBrowser::setWorkspace(Mantid::API::IFitFunction* f)const
{
  std::string wsName = workspaceName();
  if (!wsName.empty())
  {
    try
    {
      Mantid::API::MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve(wsName));
      if (ws)
      {
        //int xMin=-1,xMax;
        //double sX = startX();
        //double eX = endX();
        //const Mantid::MantidVec& X = ws->readX(workspaceIndex());
        //for(xMax = 0;xMax < ws->blocksize(); ++xMax)
        //{
        //  if (X[xMax] < sX) continue;
        //  else if (xMin < 0)
        //  {
        //    xMin = xMax;
        //  }
        //  if (X[xMax] > eX) break;
        //}
        QString slice = "WorkspaceIndex="+QString::number(workspaceIndex())+
          ",StartX="+QString::number(startX())+",EndX="+QString::number(endX());
        f->setWorkspace(ws,slice.toStdString());
      }
    }
    catch(...){}
  }
}

void FitPropertyBrowser::addAutoBackground()
{
  if (m_autoBgName.isEmpty()) return;
  bool hasPlot = false;
  PropertyHandler* ch = currentHandler();
  if (m_autoBackground)
  {// remove old background
    if (ch == m_autoBackground)
    {
      ch = NULL;
    }
    hasPlot = m_autoBackground->hasPlot();
    m_autoBackground->removeFunction();
    m_autoBackground = NULL;
  }
  // Create the function
  PropertyHandler* h = getHandler()->addFunction(m_autoBgName.toStdString());
  if (!h) return;
  if (!m_autoBgAttributes.isEmpty())
  {// set attributes
    QStringList attList = m_autoBgAttributes.split(' ');
    foreach(QString att,attList)
    {
      QStringList name_value = att.split('=');
      if (name_value.size() == 2)
      {
        QString name  = name_value[0].trimmed();
        QString value = name_value[1].trimmed();
        if (h->function()->hasAttribute(name.toStdString()))
        {
          h->setAttribute(name,value);
        }
      }
    }
  }
  h->fit();
  m_autoBackground = h;
  getHandler()->calcBaseAll();
  if (hasPlot)
  {
    setCurrentFunction(h);
    emit plotCurrentGuess();
    if (ch)
    {
      setCurrentFunction(ch);
    }
  }
}

void FitPropertyBrowser::refitAutoBackground()
{
  if (m_autoBackground)
  {
    m_autoBackground->fit();
  }
}

/**
  * Remember a background function name to be used for creating auto-background
  * @param aName :: A name of the auto-background. The may be followed by function
  * attributes as name=value pairs separated by spaces.
  */
void FitPropertyBrowser::setAutoBackgroundName(const QString& aName)
{
  try
  {
    QStringList nameList = aName.split(' ');
    if (nameList.isEmpty()) return;
    QString name = nameList[0];
    boost::shared_ptr<Mantid::API::IFitFunction> f = boost::shared_ptr<Mantid::API::IFitFunction>(
      Mantid::API::FunctionFactory::Instance().createFitFunction(name.toStdString()));
    m_auto_back = true;
    m_autoBgName = name;
    if (nameList.size() > 1)
    {
      nameList.removeFirst();
      m_autoBgAttributes = nameList.join(" ");
    }
    Mantid::Kernel::ConfigService::Instance().setString("curvefitting.autoBackground",aName.toStdString());
  }
  catch(...)
  {
    m_auto_back = false;
  }
}

/// Set LogValue for PlotPeakByLogValue
void FitPropertyBrowser::setLogValue(const QString& lv)
{
  if (isWorkspaceAGroup())
  {
    validateGroupMember();
    if (!m_logValue)
    {
      m_logValue = m_enumManager->addProperty("LogValue");
      m_settingsGroup->property()->addSubProperty(m_logValue);
    }
    m_logs.clear();
    m_logs << "";
    if (!m_groupMember.empty())
    {
      Mantid::API::MatrixWorkspace_sptr ws = 
        boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve(m_groupMember)
        );
      if (ws)
      {
        const std::vector<Mantid::Kernel::Property*> logs = ws->run().getLogData();
        for(int i=0;i<static_cast<int>(logs.size()); ++i)
        {
          m_logs << QString::fromStdString(logs[i]->name());
        }
      }
    }
    m_enumManager->setEnumNames(m_logValue,m_logs);
    int i = m_logs.indexOf(lv);
    if (i < 0) i = 0;
    m_enumManager->setValue(m_logValue,i);
  }
}

std::string FitPropertyBrowser::getLogValue()const
{
  if (isWorkspaceAGroup() && m_logValue)
  {
    int i = m_enumManager->value(m_logValue);
    if (i < m_logs.size()) return m_logs[i].toStdString();
  }
  return "";
}

/// Remove LogValue from the browser
void FitPropertyBrowser::removeLogValue()
{
  if (isWorkspaceAGroup()) return;
  m_settingsGroup->property()->removeSubProperty(m_logValue);
  m_logValue = NULL;
}

void FitPropertyBrowser::validateGroupMember()
{
  std::string wsName = workspaceName();
  Mantid::API::WorkspaceGroup_sptr wsg = boost::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(
    Mantid::API::AnalysisDataService::Instance().retrieve(wsName) );
  if (!wsg)
  {
    m_groupMember = workspaceName();
    return;
  }
  std::vector<std::string> names = wsg->getNames();
  if (names.empty())
  {
    m_groupMember = "";
    return;
  }
  if (std::find(names.begin(),names.end(),m_groupMember) != names.end())
  {
    return;
  }
  if (names[0] == wsName)
  {
    if (names.size() > 1)
    {
      m_groupMember = names[1];
    }
    else
    {
      m_groupMember = "";
    }
  }
  else
  {
    m_groupMember = names[0];
  }
}

void FitPropertyBrowser::sequentialFit()
{
  if (workspaceName() == outputName())
  {
    setOutputName(outputName() + "_res");
  }
  SequentialFitDialog* dlg = new SequentialFitDialog(this);
  std::string wsName = workspaceName();
  if (!wsName.empty() && dlg->addWorkspaces(QStringList(QString::fromStdString(wsName))))
  {
    dlg->show();
  }
  
}

void FitPropertyBrowser::findPeaks()
{
  std::string wsName = workspaceName();
  if (wsName.empty())
  {
    m_appWindow->mantidUI->showCritical("Workspace name is not set");
    return;
  }

  std::string peakListName = wsName + "_PeakList_tmp";

  int FWHM,Tolerance;
  QString setting = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("curvefitting.findPeaksFWHM"));
  FWHM = setting.isEmpty() ? 7 : setting.toInt();

  setting = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("curvefitting.findPeaksTolerance"));
  Tolerance = setting.isEmpty() ? 4 : setting.toInt();

  Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("FindPeaks");
  alg->initialize();
  alg->setPropertyValue("InputWorkspace",wsName);
  alg->setProperty("WorkspaceIndex",workspaceIndex());
  alg->setPropertyValue("PeaksList",peakListName);
  alg->setProperty("FWHM",FWHM);
  alg->setProperty("Tolerance",Tolerance);

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  Mantid::API::MatrixWorkspace_sptr inputWS =
      boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
          Mantid::API::AnalysisDataService::Instance().retrieve(workspaceName()));

  try
  {
    alg->execute();
    Mantid::API::ITableWorkspace_sptr ws = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(
      Mantid::API::AnalysisDataService::Instance().retrieve(peakListName) );

    clear();
    Mantid::API::ColumnVector<double> centre = ws->getVector("centre");
    Mantid::API::ColumnVector<double> width = ws->getVector("width");
    Mantid::API::ColumnVector<double> height = ws->getVector("height");
    for(int i=0; i<centre.size(); ++i)
    {
      if (centre[i] < startX() || centre[i] > endX()) continue;
      Mantid::API::IPeakFunction* f = dynamic_cast<Mantid::API::IPeakFunction*>(
        Mantid::API::FunctionFactory::Instance().createFunction(defaultPeakType())
        );
      if (!f) break;
      f->initialize();
      f->setMatrixWorkspace(inputWS,workspaceIndex(),-1,-1);
      f->setCentre(centre[i]);
      f->setWidth(width[i]);
      f->setHeight(height[i]);
      addFunction(*f);
      delete f;
    }
  }
  catch(...)
  {
    QApplication::restoreOverrideCursor();
    throw;
  }

	QApplication::restoreOverrideCursor();
}

void FitPropertyBrowser::setPeakToolOn(bool on)
{
  m_peakToolOn = on;
  m_btnPlotGuess->setEnabled(on);
}

void FitPropertyBrowser::updateDecimals()
{
  if (m_decimals < 0)
  {
    QSettings settings;
    settings.beginGroup("Mantid/FitBrowser");
    m_decimals = settings.value("decimals",6).toInt();
  }
  QSet<QtProperty *> props = m_doubleManager->properties();
  foreach(QtProperty *prop,props)
  {
    m_doubleManager->setDecimals(prop,m_decimals);
  }
}

void FitPropertyBrowser::setDecimals(int d)
{
  m_decimals = d;
  QSettings settings;
  settings.beginGroup("Mantid/FitBrowser");
  settings.setValue("decimals",d);
  updateDecimals();
}
