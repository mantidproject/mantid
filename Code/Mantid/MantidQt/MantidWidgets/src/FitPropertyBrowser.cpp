#include "MantidQtMantidWidgets/FitPropertyBrowser.h"
#include "MantidQtMantidWidgets/PropertyHandler.h"
#include "MantidQtMantidWidgets/SequentialFitDialog.h"
#include "MantidQtMantidWidgets/MultifitSetupDialog.h"

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

#include <algorithm>


namespace MantidQt
{
namespace MantidWidgets
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

/**
 * Constructor
 * @param parent :: The parent widget - must be an ApplicationWindow
 * @param mantidui :: The UI form for MantidPlot
 * @param customFittings :: If true, use custom fittings
 */
FitPropertyBrowser::FitPropertyBrowser(QWidget *parent, QObject* mantidui, bool customFittings)
:QDockWidget("Fit Function",parent),
m_customFittings(customFittings),
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
m_decimals(-1),
m_mantidui(mantidui)
{
  QSettings settings;
  settings.beginGroup("Mantid/FitBrowser");

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

  QWidget* w = new QWidget(this);

    /* Create property managers: they create, own properties, get and set values  */

  m_groupManager =  new QtGroupPropertyManager(w);
  m_doubleManager = new QtDoublePropertyManager(w);
  m_stringManager = new QtStringPropertyManager(w);
  m_enumManager =   new QtEnumPropertyManager(w);
  m_intManager =    new QtIntPropertyManager(w);
  m_boolManager = new QtBoolPropertyManager(w);
  m_filenameManager = new QtStringPropertyManager(w);
  m_formulaManager = new QtStringPropertyManager(w);

  // to be able to change windows title from tread
  connect(this,SIGNAL(changeWindowTitle(const QString&)),this,SLOT(setWindowTitle(const QString&)));

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
  QtProperty* settingsGroup(NULL);

  if (!m_customFittings)
  {  
    connect(this,SIGNAL(xRangeChanged(double, double)), m_mantidui, SLOT(x_range_from_picker(double, double)));
    /* Create input - output properties */  
    settingsGroup = m_groupManager->addProperty("Settings");    
    m_startX = addDoubleProperty("StartX");
    m_endX = addDoubleProperty("EndX");
  }
  else
  {
    // Seperates the data and the settings into two seperate categories
    settingsGroup = m_groupManager->addProperty("Data");
    
    // Have slightly different names as requested by the muon scientists. 
    m_startX = addDoubleProperty(QString("Start (%1s)" ).arg(QChar(0x03BC))); //(mu); 
    m_endX = addDoubleProperty(QString("End (%1s)" ).arg(QChar(0x03BC)));
  }

  m_workspace = m_enumManager->addProperty("Workspace");
  m_workspaceIndex = m_intManager->addProperty("Workspace Index");
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

  m_plotDiff = m_boolManager->addProperty("Plot Difference");
  bool plotDiff = settings.value("Plot Difference",QVariant(true)).toBool();
  m_boolManager->setValue(m_plotDiff,plotDiff);
  
  settingsGroup->addSubProperty(m_workspace);
  settingsGroup->addSubProperty(m_workspaceIndex);
  settingsGroup->addSubProperty(m_startX);
  settingsGroup->addSubProperty(m_endX);
  settingsGroup->addSubProperty(m_output);
  
  // Only include the cost function when in the dock widget inside mantid plot, not on muon analysis widget
  // Include minimiser and plot difference under a different settings section.
  if (!customFittings)
  {
  settingsGroup->addSubProperty(m_minimizer);
  settingsGroup->addSubProperty(m_costFunction);
  settingsGroup->addSubProperty(m_plotDiff);
  }
  
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

  // Custom settings that are specific and asked for by the muon scientists.
  if (m_customFittings)
  {
    QtProperty* customSettingsGroup = m_groupManager->addProperty("Settings");
    m_rawData = m_boolManager->addProperty("Fit To Raw Data");
    bool data = settings.value("Fit To Raw Data",QVariant(false)).toBool();
    m_boolManager->setValue(m_rawData,data);
    customSettingsGroup->addSubProperty(m_minimizer);
    customSettingsGroup->addSubProperty(m_plotDiff);
    customSettingsGroup->addSubProperty(m_rawData);
    m_customSettingsGroup = m_browser->addProperty(customSettingsGroup);
  }

  QVBoxLayout* layout = new QVBoxLayout(w);
  QGridLayout* buttonsLayout = new QGridLayout();

  QPushButton* btnFit = new QPushButton("Fit");

  m_tip = new QLabel("",w);

  m_fitMenu = new QMenu(this);
  m_fitActionFit = new QAction("Fit",this);
  m_fitActionSeqFit = new QAction("Sequential Fit",this);
  m_fitActionUndoFit = new QAction("Undo Fit",this);
  m_fitMapper = new QSignalMapper(this);
  m_fitMapper->setMapping(m_fitActionFit,"Fit");
  m_fitMapper->setMapping(m_fitActionSeqFit,"SeqFit");
  m_fitMapper->setMapping(m_fitActionUndoFit,"UndoFit");
  connect(m_fitActionFit,SIGNAL(activated()), m_fitMapper, SLOT(map()));
  connect(m_fitActionSeqFit,SIGNAL(activated()), m_fitMapper, SLOT(map()));
  connect(m_fitActionUndoFit,SIGNAL(activated()), m_fitMapper, SLOT(map()));
  connect(m_fitMapper, SIGNAL(mapped(const QString &)), this, SLOT(executeFitMenu(const QString&)));
  m_fitMenu->addAction(m_fitActionFit);
  m_fitMenu->addAction(m_fitActionSeqFit);
  m_fitMenu->addSeparator();
  m_fitMenu->addAction(m_fitActionUndoFit);
  btnFit->setMenu(m_fitMenu);

  QPushButton* btnDisplay = new QPushButton("Display");
  QMenu* displayMenu = new QMenu(this);
  m_displayActionPlotGuess = new QAction("Plot Guess",this);
  m_displayActionPlotGuess->setEnabled(false);
  m_displayActionQuality = new QAction("Quality",this);
  m_displayActionQuality->setCheckable(true);
  m_displayActionQuality->setChecked(true);
  m_displayActionClearAll = new QAction("Clear fit curves",this);
  QSignalMapper* displayMapper = new QSignalMapper(this);

  displayMapper->setMapping(m_displayActionPlotGuess,"PlotGuess");
  displayMapper->setMapping(m_displayActionQuality,"Quality");
  displayMapper->setMapping(m_displayActionClearAll,"ClearAll");
  connect(m_displayActionPlotGuess,SIGNAL(activated()), displayMapper, SLOT(map()));
  connect(m_displayActionQuality,SIGNAL(activated()), displayMapper, SLOT(map()));
  connect(m_displayActionClearAll,SIGNAL(activated()), displayMapper, SLOT(map()));
  connect(displayMapper, SIGNAL(mapped(const QString &)), this, SLOT(executeDisplayMenu(const QString&)));
  displayMenu->addAction(m_displayActionPlotGuess);
  displayMenu->addAction(m_displayActionClearAll);
  displayMenu->addAction(m_displayActionQuality);
  btnDisplay->setMenu(displayMenu);

  QPushButton* btnSetup = new QPushButton("Setup");
  QMenu* setupMenu = new QMenu(this);

  m_setupActionCustomSetup = new QAction("Custom Setup",this);
  QAction* setupActionManageSetup = new QAction("Manage Setup",this);
  QAction* setupActionFindPeaks = new QAction("Find Peaks",this);
  QAction* setupActionClearFit = new QAction("Clear Fit",this);

  QMenu* setupSubMenuCustom = new QMenu(this);
  m_setupActionCustomSetup->setMenu(setupSubMenuCustom);

  QMenu* setupSubMenuManage = new QMenu(this);
  QAction* setupActionSave = new QAction("Save Setup",this);
  m_setupActionRemove = new QAction("Remove Setup",this);
  QAction* setupActionCopyToClipboard = new QAction("Copy To Clipboard",this);
  QAction* setupActionLoadFromString = new QAction("Load From String",this);
  QSignalMapper* setupManageMapper = new QSignalMapper(this);
  setupManageMapper->setMapping(setupActionSave,"SaveSetup");
  setupManageMapper->setMapping(setupActionCopyToClipboard,"CopyToClipboard");
  setupManageMapper->setMapping(setupActionLoadFromString,"LoadFromString");
  connect(setupActionSave,SIGNAL(activated()), setupManageMapper, SLOT(map()));
  connect(setupActionCopyToClipboard,SIGNAL(activated()), setupManageMapper, SLOT(map()));
  connect(setupActionLoadFromString,SIGNAL(activated()), setupManageMapper, SLOT(map()));
  connect(setupManageMapper, SIGNAL(mapped(const QString &)), this, SLOT(executeSetupManageMenu(const QString&)));
  setupSubMenuManage->addAction(setupActionSave);
  setupSubMenuManage->addAction(m_setupActionRemove);
  setupSubMenuManage->addAction(setupActionCopyToClipboard);
  setupSubMenuManage->addAction(setupActionLoadFromString);
  setupActionManageSetup->setMenu(setupSubMenuManage); 

  QMenu* setupSubMenuRemove = new QMenu(this);
  m_setupActionRemove->setMenu(setupSubMenuRemove); 

  QSignalMapper* setupMapper = new QSignalMapper(this);
  setupMapper->setMapping(setupActionClearFit,"ClearFit");
  setupMapper->setMapping(setupActionFindPeaks,"FindPeaks");
  connect(setupActionClearFit,SIGNAL(activated()), setupMapper, SLOT(map()));
  connect(setupActionFindPeaks,SIGNAL(activated()), setupMapper, SLOT(map()));
  connect(setupMapper, SIGNAL(mapped(const QString &)), this, SLOT(executeSetupMenu(const QString&)));

  setupMenu->addAction(m_setupActionCustomSetup);
  setupMenu->addAction(setupActionManageSetup);
  setupMenu->addSeparator();
  setupMenu->addAction(setupActionFindPeaks);
  setupMenu->addSeparator();
  setupMenu->addAction(setupActionClearFit);
  btnSetup->setMenu(setupMenu);

  updateSetupMenus();

  buttonsLayout->addWidget(btnFit,0,0);
  buttonsLayout->addWidget(btnDisplay,0,1);
  buttonsLayout->addWidget(btnSetup,0,2);

  layout->addLayout(buttonsLayout);
  layout->addWidget(m_tip);
  layout->addWidget(m_browser);

  setWidget(w);

  m_browser->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_browser, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(popupMenu(const QPoint &)));
  connect(m_browser, SIGNAL(currentItemChanged(QtBrowserItem*)), this, SLOT(currentItemChanged(QtBrowserItem*)));
  connect(this,SIGNAL(multifitFinished()),this,SLOT(processMultiBGResults()));

  createCompositeFunction();

  m_changeSlotsEnabled = true;
    
  populateFunctionNames();

  // Should only be done for the fitBrowser which is part of MantidPlot
  if (!m_customFittings)
  {
    if (m_mantidui->metaObject()->indexOfMethod("executeAlgorithm(QString,QMap<QString,QString>,Mantid::API::AlgorithmObserver*)") >= 0)
    {
      // this make the progress bar work with Fit algorithm running form the fit browser
      connect(this,SIGNAL(executeFit(QString,QMap<QString,QString>,Mantid::API::AlgorithmObserver*)),
        m_mantidui,SLOT(executeAlgorithm(QString,QMap<QString,QString>,Mantid::API::AlgorithmObserver*)));
    }
  }
}

/// Update setup menus according to how these are set in
/// settings
void FitPropertyBrowser::updateSetupMenus()
{
  QMenu* menuLoad = m_setupActionCustomSetup->menu();
  menuLoad->clear();
  QMenu* menuRemove = m_setupActionRemove->menu();
  menuRemove->clear();

  QSettings settings;
  settings.beginGroup("Mantid/FitBrowser/SavedFunctions");
  QStringList names = settings.childKeys();

  QSignalMapper* mapperLoad = new QSignalMapper(this);
  QSignalMapper* mapperRemove = new QSignalMapper(this);
  for (int i = 0; i < names.size(); i++)
  {
    QAction* itemLoad = new QAction(names.at(i), this);
    QAction* itemRemove = new QAction(names.at(i), this);
    mapperLoad->setMapping(itemLoad,names.at(i));
    mapperRemove->setMapping(itemRemove,names.at(i));
    connect(itemLoad,SIGNAL(activated()), mapperLoad, SLOT(map()));
    connect(itemRemove,SIGNAL(activated()), mapperRemove, SLOT(map()));
    menuLoad->addAction( itemLoad );
    menuRemove->addAction( itemRemove );
  }
  connect(mapperLoad, SIGNAL(mapped(const QString &)), this, SLOT(executeCustomSetupLoad(const QString&)));
  connect(mapperRemove, SIGNAL(mapped(const QString &)), this, SLOT(executeCustomSetupRemove(const QString&)));
}

void FitPropertyBrowser::executeCustomSetupLoad(const QString& name)
{
  QSettings settings;
  settings.beginGroup("Mantid/FitBrowser/SavedFunctions");
  QStringList names = settings.childKeys();

  QString str = settings.value(name).toString();
  loadFunction(str);
}

void FitPropertyBrowser::executeCustomSetupRemove(const QString& name)
{
  QSettings settings;
  settings.beginGroup("Mantid/FitBrowser/SavedFunctions");
  QStringList names = settings.childKeys();

  settings.remove(name);
  updateSetupMenus();
}

void FitPropertyBrowser::executeFitMenu(const QString& item)
{
  if (item == "Fit")
    fit();
  if (item == "SeqFit")
    sequentialFit();
  if (item == "UndoFit")
    undoFit();
}

void FitPropertyBrowser::executeDisplayMenu(const QString& item)
{
  if (item == "PlotGuess")
  {
    plotOrRemoveGuessAll();
  }
  else if (item == "ClearAll")
  {
    clearAllPlots();
  }
}

void FitPropertyBrowser::executeSetupMenu(const QString& item)
{
  if (item == "ClearFit")
    clear();
  if (item == "FindPeaks")
    findPeaks();
}

void FitPropertyBrowser::executeSetupManageMenu(const QString& item)
{
  if (item == "SaveSetup")
    saveFunction();
  if (item == "CopyToClipboard")
    copy();
  if (item == "LoadFromString")
    loadFunctionFromString();
}

/// Destructor
FitPropertyBrowser::~FitPropertyBrowser()
{
  QSettings settings;
  settings.beginGroup("Mantid/FitBrowser");
  bool plotDiff = m_boolManager->value(m_plotDiff);
  settings.setValue("Plot Difference",plotDiff);
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
    if (!cf || (cf->name() != "CompositeFunctionMW" && cf->name() != "MultiBG"))
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
  QMenu *menu = new QMenu(this);
  QAction *action;

  bool isFunctionsGroup = ci == m_functionsGroup;
  bool isSettingsGroup = ci == m_settingsGroup;
  bool isASetting = ci->parent() == m_settingsGroup;
  bool isFunction = getHandler()->findFunction(ci) != NULL;
  bool isCompositeFunction = isFunction && getHandler()->findCompositeFunction(ci);

  PropertyHandler* h = getHandler()->findHandler(ci->property());

  if (isFunctionsGroup)
  {
    action = new QAction("Add function",this);
    connect(action,SIGNAL(triggered()),this,SLOT(addFunction()));
    menu->addAction(action);

    if (m_compositeFunction->name() == "MultiBG" && m_compositeFunction->nFunctions() == 1 
      && Mantid::API::AnalysisDataService::Instance().doesExist(workspaceName()))
    {
      action = new QAction("Setup multifit",this);
      connect(action,SIGNAL(triggered()),this,SLOT(setupMultifit()));
      menu->addAction(action);
    }

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

    action = new QAction("Copy To Clipboard",this);
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
    emit removePlotSignal(getHandler());
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
  if (i < 0)
  {
    // workspace may not be found because add notification hasn't been processed yet
    populateWorkspaceNames();
    i = m_workspaceNames.indexOf(wsName);
  }
  if (i >= 0)
  {
    m_enumManager->setValue(m_workspace,i);
    if (!m_customFittings)
    {
      Mantid::API::MatrixWorkspace_sptr mws;
      try
      {
        mws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve(wsName.toStdString()));
      }
      catch(Mantid::Kernel::Exception::NotFoundError&)
      {
      }
      if (mws)
      {
        size_t wi = static_cast<size_t>(workspaceIndex());
        if (wi < mws->getNumberHistograms() && !mws->readX(wi).empty())
        {
          setStartX(mws->readX(wi).front());
          setEndX(mws->readX(wi).back());
        }
      }
    }
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

/// Set the output name
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
    workspaceChange(QString::fromStdString(workspaceName()));
  }
  else if (prop->propertyName() == "Type")
  {
      disableUndo();
      PropertyHandler* h = getHandler()->findHandler(prop);
      if (!h) return;
      //if (!h->parentHandler()) return;
      Mantid::API::IFitFunction* f = h->changeType(prop);
      if (!h->parentHandler())
      {
        m_compositeFunction = dynamic_cast<Mantid::API::CompositeFunction*>(f);
      }
      if (f) setCurrentFunction(f);
      emit functionChanged();
  }
  else if (prop->propertyName() == "Workspace")
  {
      PropertyHandler* h = getHandler()->findHandler(prop);
      if (!h) return;
      h->setFunctionWorkspace();
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
      Mantid::API::AnalysisDataService::Instance().retrieve(workspaceName()));
    if (!ws)
    {
      setWorkspaceIndex(0);
      return;
    }
    int n = static_cast<int>(ws->getNumberHistograms());
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
  else if (prop->propertyName() == "Workspace Index")
  {
    PropertyHandler* h = getHandler()->findHandler(prop);
    if (!h) return;
    h->setFunctionWorkspace();
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
    if (!m_customFittings)
    {
      getHandler()->setAttribute("StartX",value);
    }
    else
    {
      getHandler()->setAttribute(QString("Start (%1s)" ).arg(QChar(0x03BC)), value); // (mu)
    }
    emit startXChanged(startX());
    emit xRangeChanged(startX(), endX());
    return;
  }
  else if (prop == m_endX )
  {
    // call setWorkspace to change minX in functions
    setWorkspace(m_compositeFunction);
    if (!m_customFittings)
    {
      getHandler()->setAttribute("EndX",value);
    }
    else
    {
      getHandler()->setAttribute(QString("End (%1s)" ).arg(QChar(0x03BC)), value); 
    }
    emit endXChanged(endX());
    emit xRangeChanged(startX(), endX());
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
    if (qfnName == "MultiBG") continue;
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
  return static_cast<int>(m_compositeFunction->nFunctions());
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

//#include "../FitDialog.h"
/**
 * Creates an instance of Fit algorithm, sets its properties and launches it.
 */
void FitPropertyBrowser::fit()
{
  std::string wsName = workspaceName();

  if (wsName.empty())
  {
    QMessageBox::critical(this,"Mantid - Error", "Workspace name is not set");
    return;
  }
  try
  {
    m_initialParameters.resize(compositeFunction()->nParams());
    for(size_t i=0;i<compositeFunction()->nParams();i++)
    {
      m_initialParameters[i] = compositeFunction()->getParameter(i);
    }
    m_fitActionUndoFit->setEnabled(true);

    std::string funStr;
    if (m_compositeFunction->name() == "MultiBG")
    {
      std::string ties;
      Mantid::API::Expression funExpr;
      funExpr.parse(*m_compositeFunction);
      funExpr.toList(";");
      for(size_t i = 0; i < funExpr.size(); ++i)
      {
        const Mantid::API::Expression& e = funExpr[i];
        if (e.name() == "=" && e.size() == 2 && e[0].name() == "ties")
        {
          ties = e[0].name() + "=(" + e[1].str() + ")";
        }
      }
      funStr = "composite=MultiBG;";
      for(size_t i=0;i<m_compositeFunction->nFunctions();++i)
      {
        Mantid::API::IFunctionMW* f = dynamic_cast<Mantid::API::IFunctionMW*>(m_compositeFunction->getFunction(i));
        if (!f) continue;
        funStr += f->asString();
        if (f->getMatrixWorkspace() && !f->getMatrixWorkspace()->getName().empty())
        {
          funStr += ",Workspace=" + f->getMatrixWorkspace()->getName() + ",WSParam=(WorkspaceIndex="+
            boost::lexical_cast<std::string>(f->getWorkspaceIndex()) + ")";
        }
        funStr += ";";
      }
      funStr += ties;
    }
    else if (m_compositeFunction->nFunctions() > 1)
    {
      funStr = *m_compositeFunction;
    }
    else
    {
      funStr = *(m_compositeFunction->getFunction(0));
    }

    if ( Mantid::API::AnalysisDataService::Instance().doesExist(wsName+"_NormalisedCovarianceMatrix"))
    {
      Mantid::API::FrameworkManager::Instance().deleteWorkspace(wsName+"_NormalisedCovarianceMatrix");
    }
    if ( Mantid::API::AnalysisDataService::Instance().doesExist(wsName+"_Parameters"))
    {
      Mantid::API::FrameworkManager::Instance().deleteWorkspace(wsName+"_Parameters");
    }
    if ( Mantid::API::AnalysisDataService::Instance().doesExist(wsName+"_Workspace"))
    {
      Mantid::API::FrameworkManager::Instance().deleteWorkspace(wsName+"_Workspace");
    }

    // If it is in the custom fitting (muon analysis) i.e not a docked widget in mantidPlot
    if (m_customFittings)
    {
      Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("Fit");
      alg->initialize();
      if (rawData())
        alg->setPropertyValue("InputWorkspace",wsName + "_Raw");
      else
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
    else
    {
      if (m_mantidui->metaObject()->indexOfMethod("executeAlgorithm(QString,QMap<QString,QString>,Mantid::API::AlgorithmObserver*)") >= 0)
      {
        QMap<QString,QString> algParams;
        algParams["InputWorkspace"] = QString::fromStdString(wsName);
        algParams["WorkspaceIndex"] = QString::number(workspaceIndex());
        algParams["StartX"] = QString::number(startX());
        algParams["EndX"] = QString::number(endX());
        algParams["Output"] = QString::fromStdString(outputName());
        algParams["Function"] = QString::fromStdString(funStr);
        algParams["Minimizer"] = QString::fromStdString(minimizer());
        algParams["CostFunction"] = QString::fromStdString(costFunction());
        emit executeFit("Fit",algParams,this);
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
  }
  catch(std::exception& e)
  {
    QString msg = "Fit algorithm failed.\n\n"+QString(e.what())+"\n";
    QMessageBox::critical(this,"Mantid - Error", msg);
  }
}

void FitPropertyBrowser::finishHandle(const Mantid::API::IAlgorithm* alg)
{  
  // Emit a signal to show that the fitting has completed. (workspaceName that the fit has been done against is sent as a parameter)
  QString name(QString::fromStdString(alg->getProperty("InputWorkspace") ) );
  if (name.contains('_') ) // Must be fitting to raw data, need to group under name without "_Raw".
    emit fittingDone(name.left(name.find('_') ) );
  else // else fitting to current workspace, group under same name.
    emit fittingDone(name);

  getFitResults();
  if (!isWorkspaceAGroup() && alg->existsProperty("OutputWorkspace"))
  {
    std::string out = alg->getProperty("OutputWorkspace");
    emit algorithmFinished(QString::fromStdString(out));
  }
  // update Quality string
  if ( m_displayActionQuality->isChecked() )
  {
    double quality = alg->getProperty("OutputChi2overDoF");
    std::string costFunction = alg->getProperty("CostFunction");
    Mantid::API::ICostFunction* costfun 
     = Mantid::API::CostFunctionFactory::Instance().createUnwrapped(costFunction); 
    emit changeWindowTitle(QString("Fit Function (") 
      + costfun->shortName().c_str() + " = " + QString::number(quality) + ")");
  }
  else
    emit changeWindowTitle("Fit Function");
  if (m_compositeFunction->name() == "MultiBG")
  {
    emit multifitFinished();
  }
}


/// Get and store available workspace names
void FitPropertyBrowser::populateWorkspaceNames()
{
  m_workspaceNames.clear();
  //QStringList tmp = m_appWindow->mantidUI->getWorkspaceNames();

  QStringList tmp;
  std::set<std::string> sv = Mantid::API::AnalysisDataService::Instance().getObjectNames();
  for (std::set<std::string>::const_iterator it = sv.begin(); it != sv.end(); ++it)
    tmp<<QString::fromStdString(*it);

  for(int i=0;i<tmp.size();i++)
  {
    Mantid::API::Workspace_sptr ws = Mantid::API::AnalysisDataService::Instance().retrieve(tmp[i].toStdString());
    if (isWorkspaceValid(ws))
    {
      m_workspaceNames.append(tmp[i]);
    }
  }
  m_enumManager->setEnumNames(m_workspace, m_workspaceNames);
}

/**
 * Connect to the AnalysisDataServis when shown
 */
void FitPropertyBrowser::showEvent(QShowEvent* e)
{
  (void)e;
  // Observe what workspaces are added and deleted unless it's a custom fitting, all workspaces for custom fitting (eg muon analysis) 
  // should be manually added.
  if (!m_customFittings)
  {
    observeAdd();
  }
  observePostDelete();
  populateWorkspaceNames();
}

/**
 * Disconnect from the AnalysisDataServis when hiden
 */
void FitPropertyBrowser::hideEvent(QHideEvent* e)
{
  (void)e;
  observeAdd(false);
  observePostDelete(false);
}

/// workspace was added
void FitPropertyBrowser::addHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws)
{
  if ( !isWorkspaceValid(ws) ) return;
  QStringList oldWorkspaces = m_workspaceNames;
  QString oldName = QString::fromStdString(workspaceName());
  int i = m_workspaceNames.indexOf(QString(wsName.c_str()));
  // if new workspace append this workspace name
  if (i < 0)
  {
    m_workspaceNames.append(QString(wsName.c_str()));
    m_workspaceNames.sort();
    m_enumManager->setEnumNames(m_workspace, m_workspaceNames);
  }
  // get hold of index of oldName
  i = m_workspaceNames.indexOf(oldName);
  if (i >= 0)
  {
    m_enumManager->setValue(m_workspace,i);
  }
  if (m_workspaceNames.size() == 1)
  {
    setWorkspaceName(QString::fromStdString(wsName));
  }
  getHandler()->updateWorkspaces(oldWorkspaces);
}

/// workspace was removed
void FitPropertyBrowser::postDeleteHandle(const std::string& wsName)
{
  QStringList oldWorkspaces = m_workspaceNames;
  QString oldName = QString::fromStdString(workspaceName());
  int i = m_workspaceNames.indexOf(QString(wsName.c_str()));
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
  getHandler()->updateWorkspaces(oldWorkspaces);
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

/// Get whether the FitPropertyBrowser is in the Muon interface
bool FitPropertyBrowser::isCustomFittings()const
{
  return m_customFittings;
}

///
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
  std::string wsName = outputName() + "_Parameters";
  if (Mantid::API::AnalysisDataService::Instance().doesExist(wsName))
  {
    Mantid::API::ITableWorkspace_sptr ws = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(
      Mantid::API::AnalysisDataService::Instance().retrieve(wsName) );

    Mantid::API::TableRow row = ws->getFirstRow();
    do
    {
      try
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
      catch(...)
      {
        // do nothing
      }
    }
    while(row.next());
    updateParameters();
  }
}

/**
 * Slot. Undoes the fit: restores the parameters to their initial values.
 */
void FitPropertyBrowser::undoFit()
{
  if (m_initialParameters.size() == compositeFunction()->nParams())
  {
    for(size_t i=0;i<compositeFunction()->nParams();i++)
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
  m_fitActionUndoFit->setEnabled(false);
}

/// Tells if undo can be done
bool FitPropertyBrowser::isUndoEnabled()const
{
  return m_initialParameters.size() && compositeFunction()->nParams() == m_initialParameters.size();
}

/// Enable/disable the Fit button;
void FitPropertyBrowser::setFitEnabled(bool yes)
{
  m_fitActionFit->setEnabled(yes);
  if (!m_customFittings)
  {
    m_fitActionSeqFit->setEnabled(yes);
  }
  else
  {
    m_fitActionSeqFit->setEnabled(false);
  }
}

/// Returns true if the function is ready for a fit
bool FitPropertyBrowser::isFitEnabled()const
{
  return m_fitActionFit->isEnabled();
}

/** 
 * Slot. Adds a tie. Full expression to be entered \<name\>=\<formula\>
 */
void FitPropertyBrowser::addTie()
{
  QtBrowserItem * ci = m_browser->currentItem();
  QtProperty* paramProp = ci->property();
  PropertyHandler* h = getHandler()->findHandler(paramProp);
  if (!h) return;
  if (!h->isParameter(paramProp)) return;

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
  for(size_t i=0;i<m_compositeFunction->nParams();i++)
  {
    Mantid::API::ParameterReference ref(m_compositeFunction,i);
    Mantid::API::IFitFunction* fun = dynamic_cast<Mantid::API::IFitFunction*>(ref.getFunction());
    if (!fun)
    {
      throw std::runtime_error("IFitFunction expected but func function of another type");
    }
    // Pick out parameters with the same name as the one we're tying from
    if ( fun->parameterName(static_cast<int>(ref.getIndex())) == parName )
    {
      if ( iPar == -1 && fun == h->function() ) // If this is the 'tied from' parameter, remember it
      {
        iPar = (int)i;
      }
      else  // Otherwise add it to the list of potential 'tyees'
      {
        fnNames << QString::fromStdString(m_compositeFunction->parameterName(i));
      }
    }
  }
  if (fnNames.empty() || iPar < 0)
  {
    QMessageBox::information(this,"Mantid - information","Cannot tie this parameter to any function");
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
 * @param hasTie :: Parameter has a tie
 * @param hasBounds :: Parameter has bounds
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
 * @param parProp :: parameter property
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

void FitPropertyBrowser::clearAllPlots()
{
  emit removeFitCurves();
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
  bool ok(false);
  QString fnName = QInputDialog::getText(this, tr("Mantid - Input"), tr("Please select a name for the function"), QLineEdit::Normal, "", &ok);
  if (ok && !fnName.isEmpty())
  {
    saveFunction(fnName);
  }
}

void FitPropertyBrowser::saveFunction(const QString& fnName)
{
  QSettings settings;
  settings.beginGroup("Mantid/FitBrowser/SavedFunctions");
  QStringList names = settings.childKeys();
  if (names.contains(fnName) && QMessageBox::question(this,"Mantid - Question","Function with this name already exists.\n"
    "Would you like to replace it?",QMessageBox::Yes) != QMessageBox::Yes)
  {
    return;
  }
  settings.setValue(fnName,QString::fromStdString(*theFunction()));
  updateSetupMenus();
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
  
    loadFunction(str);
  }
}

void FitPropertyBrowser::loadFunctionFromString()
{
  QString str = QInputDialog::getText(this, "Mantid - Input", "Specify fit function string");

  if (!str.isEmpty())
  {
    loadFunction(str);
  }
}


void FitPropertyBrowser::loadFunction(const QString& funcString)
{
  // when loading a function from a sting initially
  // do not try to do auto background even if set 
  bool isAutoBGset = false;
  if ( m_auto_back )
  {
    isAutoBGset = true;
    m_auto_back = false;
  }

  getHandler()->removeAllPlots();
  clearBrowser();
  createCompositeFunction(funcString);

  if ( isAutoBGset )
    m_auto_back = true;
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
        QString slice = "WorkspaceIndex="+QString::number(workspaceIndex())+
          ",StartX="+QString::number(startX())+",EndX="+QString::number(endX());
        f->setWorkspace(ws,slice.toStdString(),true);
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
    //validateGroupMember();
    if (!m_logValue)
    {
      m_logValue = m_enumManager->addProperty("LogValue");
      m_settingsGroup->property()->addSubProperty(m_logValue);
    }
    m_logs.clear();
    m_logs << "";
   /* if (!m_groupMember.empty())
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
    }*/
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

void FitPropertyBrowser::sequentialFit()
{
  if (workspaceName() == outputName())
  {
    setOutputName(outputName() + "_res");
  }
  SequentialFitDialog* dlg = new SequentialFitDialog(this, m_mantidui);
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
    QMessageBox::critical(this,"Mantid - Error", "Workspace name is not set");
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
      f->setMatrixWorkspace(inputWS,workspaceIndex());
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
  m_displayActionPlotGuess->setEnabled(on);
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

bool FitPropertyBrowser::plotDiff()const
{
  return m_boolManager->value(m_plotDiff);
}

bool FitPropertyBrowser::rawData()const  	
{	  	
  return m_boolManager->value(m_rawData);
}

void FitPropertyBrowser::setTextPlotGuess(const QString text) 
{
  m_displayActionPlotGuess->setText(text);
}

/**
* Currently only called by the custom interface for the muon analysis fit browser.
* It adds the name of a loaded workspace to a drop down property box . 
*
* @param wsName :: The workspace name to be added.
*/
void FitPropertyBrowser::manualAddWorkspace(const QString& wsName)
{
  QString oldName = QString::fromStdString(workspaceName());
  int i = m_workspaceNames.indexOf(wsName);
  // if new workspace append this workspace name
  if (i < 0)
  {
    m_workspaceNames.append(wsName);
    m_workspaceNames.sort();
    m_enumManager->setEnumNames(m_workspace, m_workspaceNames);
  }
  // get hold of index of oldName
  i = m_workspaceNames.indexOf(oldName);
  if (i >= 0)
  {
    m_enumManager->setValue(m_workspace,i);
  }    
  //Set the workspace to the most recent one.
  workspaceChange(wsName);
}


/**
* Sets a new workspace
*/
void FitPropertyBrowser::workspaceChange(const QString& wsName)
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
    //m_groupMember = workspaceName();
    removeLogValue();
  }
  
  // If fit property browser is a custom fitting then emit signal that workspace has changed and to assign a new peak picker tool to it
  if (m_customFittings)
  {
    // Sets up the peak picker tool for that workspace. Might not work if not on correct tab. i.e loading data on the first tab wouldn't work.
     updatePPTool(wsName);
  }
}

/**
* Shows the correct workspace in the fit property browser and
* then updates the PeakPickerTool to another workspace.
*
* @param wsName :: The name of the workspace the PeakPickerTool is
*                   to be assigned to.
*/
void FitPropertyBrowser::updatePPTool(const QString & wsName)
{
  emit workspaceNameChanged(wsName);
  emit wsChangePPAssign(wsName);
}

/**
* Returns the list of workspace names the fit property browser is working on
*/
QStringList FitPropertyBrowser::getWorkspaceNames()
{
  return m_workspaceNames;
}


/**
 * Call MultifitSetupDialog to populate MultiBG function.
 */
void FitPropertyBrowser::setupMultifit()
{
  MultifitSetupDialog* dlg = new MultifitSetupDialog(this);
  dlg->exec();
  QStringList ties = dlg->getParameterTies();
  
  if (!ties.isEmpty())
  {
    QString wsName = QString::fromStdString(workspaceName());
    Mantid::API::MatrixWorkspace_sptr mws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
      Mantid::API::AnalysisDataService::Instance().retrieve(workspaceName()));
    if (mws)
    {
      Mantid::API::IFitFunction* fun = dynamic_cast<Mantid::API::IFitFunction*>(m_compositeFunction->getFunction(0));
      if (!fun)
      {
        throw std::runtime_error("IFitFunction expected but func function of another type");
      }
      QString fun1Ini = QString::fromStdString(*fun);
      QString funIni = "composite=MultiBG;" + fun1Ini + ",Workspace="+wsName+",WSParam=(WorkspaceIndex=0);";
      QString tieStr;
      for(size_t i = 1; i < mws->getNumberHistograms(); ++i)
      {
        QString comma = i > 1? "," : "";
        QString fi = comma + "f" + QString::number(i) + ".";
        for(int j = 0; j < static_cast<int>(fun->nParams()); ++j)
        {
          if (!ties[j].isEmpty())
          {
            tieStr +=  fi + QString::fromStdString(fun->parameterName(j)) + "=" + ties[j];
          }
        }
        QString wsParam = ",WSParam=(WorkspaceIndex="+QString::number(i);
        wsParam += ",StartX="+QString::number(startX())+",EndX="+QString::number(endX())+")";
        funIni += fun1Ini + ",Workspace="+wsName+wsParam+";";
      }
      if (!tieStr.isEmpty())
      {
        funIni += "ties=("+tieStr+")";
      }
      loadFunction(funIni);
    }
  }
}

/// Process and create some output if it is a MultiBG fit
void FitPropertyBrowser::processMultiBGResults()
{
  if (compositeFunction()->name() != "MultiBG") return;

  // if all member functions are of the same type and composition
  // create a TableWorkspace with parameter series

  // check if member functions are the same
  QStringList parNames;
  Mantid::API::IFitFunction* fun0 = dynamic_cast<Mantid::API::IFitFunction*>(compositeFunction()->getFunction(0));
  if (!fun0)
  {
    throw std::runtime_error("IFitFunction expected but func function of another type");
  }
  for(size_t i = 0; i < fun0->nParams(); ++i)
  {
    parNames << QString::fromStdString(fun0->parameterName(i));
  }

  for(size_t i = 1; i < compositeFunction()->nFunctions(); ++i)
  {
    Mantid::API::IFitFunction* fun = dynamic_cast<Mantid::API::IFitFunction*>(compositeFunction()->getFunction(i));
    if (!fun)
    {
      throw std::runtime_error("IFitFunction expected but func function of another type");
    }
    for(size_t j = 0; j < fun->nParams(); ++j)
    {
      if (parNames.indexOf(QString::fromStdString(fun->parameterName(j))) < 0)
      {
        // Functions are different, stop
        return;
      }
    }
  }

  QApplication::setOverrideCursor(Qt::WaitCursor);
  // create a TableWorkspace: first column - function index
  // other colomns - the parameters
  Mantid::API::ITableWorkspace_sptr table = Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
  table->addColumn("int","Index");
  foreach(QString par,parNames)
  {
    table->addColumn("double",par.toStdString());
  }
  // Create WorkspaceGroup with the fit results
  std::vector<std::string> worspaceNames(compositeFunction()->nFunctions());
  for(size_t i = 0; i < compositeFunction()->nFunctions(); ++i)
  {
    Mantid::API::TableRow row = table->appendRow();
    row << int(i);
    Mantid::API::IFunctionMW* fun = dynamic_cast<Mantid::API::IFunctionMW*>(compositeFunction()->getFunction(i));
    for(size_t j = 0; j < fun->nParams(); ++j)
    {
      row << fun->getParameter(j);
    }
    size_t wi = fun->getWorkspaceIndex();
    Mantid::API::MatrixWorkspace_sptr mws = fun->createCalculatedWorkspace(fun->getMatrixWorkspace(),wi);
    worspaceNames[i] = workspaceName()+"_"+QString::number(wi).toStdString()+"_Workspace";
    Mantid::API::AnalysisDataService::Instance().addOrReplace(worspaceNames[i],mws);
  }

  // Save the table
  Mantid::API::AnalysisDataService::Instance().addOrReplace(workspaceName()+"_Param_series",table);
  try
  {
    Mantid::API::IAlgorithm_sptr group = Mantid::API::AlgorithmManager::Instance().create("GroupWorkspaces");
    group->setProperty("InputWorkspaces",worspaceNames);
    group->setPropertyValue("OutputWorkspace",workspaceName()+"_Workspace");
    group->execute();
  }
  catch(...) {}
  QApplication::restoreOverrideCursor();

}


} // MantidQt
} // API
