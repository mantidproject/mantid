//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/IndirectDataAnalysis.h"
#include "MantidQtAPI/ManageUserDirectories.h"
#include "MantidQtMantidWidgets/RangeSelector.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/ConstraintFactory.h"
#include "MantidAPI/Expression.h"

#include <sstream>

#include <QValidator>
#include <QIntValidator>
#include <QDoubleValidator>

#include <QLineEdit>
#include <QFileInfo>
#include <QMenu>
#include <QTreeWidget>

#include <QDesktopServices>
#include <QUrl>

#include <QPalette>
#include <QColor>
#include <QApplication>

// Suppress a warning coming out of code that isn't ours
#if defined(__INTEL_COMPILER)
  #pragma warning disable 1125
#elif defined(__GNUC__)
  #if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6 )
    #pragma GCC diagnostic push
  #endif
  #pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#include "qttreepropertybrowser.h"
#include "qtpropertymanager.h"
#include "qteditorfactory.h"
#include "DoubleEditorFactory.h"
#if defined(__INTEL_COMPILER)
  #pragma warning enable 1125
#elif defined(__GNUC__)
  #if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6 )
    #pragma GCC diagnostic pop
  #endif
#endif

#include <QtCheckBoxFactory>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
// Add this class to the list of specialised dialogs in this namespace
DECLARE_SUBWINDOW(IndirectDataAnalysis);

///////////////////////////////////////////////////////////////////////
// IndirectDataAnalysis
///////////////////////////////////////////////////////////////////////

IndirectDataAnalysis::IndirectDataAnalysis(QWidget *parent) :
  UserSubWindow(parent), m_dblEdFac(NULL), m_blnEdFac(NULL), m_stringManager(NULL),
  m_changeObserver(*this, &IndirectDataAnalysis::handleDirectoryChange)
{
  // Allows us to get a handle on a tab using "m_tabs[ELWIN]", for example,
  // or even m_tabs[
  m_tabs.insert(std::make_pair(ELWIN,           new Elwin(this)));
  m_tabs.insert(std::make_pair(MSD_FIT,         new MSDFit(this)));
  m_tabs.insert(std::make_pair(FURY,            new Fury(this)));
  m_tabs.insert(std::make_pair(FURY_FIT,        new FuryFit(this)));
  m_tabs.insert(std::make_pair(CON_FIT,         new ConFit(this)));
  m_tabs.insert(std::make_pair(ABSORPTION_F2PY, new AbsorptionF2Py(this)));
  m_tabs.insert(std::make_pair(ABS_COR,         new AbsCor(this)));
}

const unsigned int IDATab::NUM_DECIMALS = 6;

void IndirectDataAnalysis::closeEvent(QCloseEvent*)
{
  Mantid::Kernel::ConfigService::Instance().removeObserver(m_changeObserver);
}

void IndirectDataAnalysis::handleDirectoryChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf)
{
  std::string key = pNf->key();
  // std::string preValue = pNf->preValue();  // Unused
  // std::string curValue = pNf->curValue();  // Unused

  if ( key == "defaultsave.directory" )
  {
    loadSettings();
  }
}

void IndirectDataAnalysis::initLayout()
{
  m_uiForm.setupUi(this);

  // Connect Poco Notification Observer
  Mantid::Kernel::ConfigService::Instance().addObserver(m_changeObserver);

  // Create Editor Factories
  m_dblEdFac = new DoubleEditorFactory(this);
  m_blnEdFac = new QtCheckBoxFactory(this);

  m_stringManager = new QtStringPropertyManager(this);

  auto tab = m_tabs.begin();
  for( ; tab != m_tabs.end(); ++tab )
    tab->second->setupTab();

  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(help()));
  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(run()));
  connect(m_uiForm.pbManageDirs, SIGNAL(clicked()), this, SLOT(openDirectoryDialog()));
}

void IndirectDataAnalysis::initLocalPython()
{
  QString pyInput = "from mantidsimple import *";
  QString pyOutput = runPythonCode(pyInput).trimmed();
  loadSettings();
}

void IndirectDataAnalysis::loadSettings()
{
  QSettings settings;
  QString settingsGroup = "CustomInterfaces/IndirectAnalysis/";
  QString saveDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory"));

  settings.beginGroup(settingsGroup + "ProcessedFiles");
  settings.setValue("last_directory", saveDir);
  
  auto tab = m_tabs.begin();
  for( ; tab != m_tabs.end(); ++tab )
    tab->second->loadTabSettings(settings);

  settings.endGroup();
}

///////////////////////////////////////////////////////////////////////
// IDATab
///////////////////////////////////////////////////////////////////////

Ui::IndirectDataAnalysis & IDATab::uiForm()
{
  return m_parent->m_uiForm;
}

DoubleEditorFactory * IDATab::doubleEditorFactory() 
{ 
  return m_parent->m_dblEdFac; 
}

QtCheckBoxFactory * IDATab::qtCheckBoxFactory() 
{ 
  return m_parent->m_blnEdFac; 
}

///////////////////////////////////////////////////////////////////////
// Elwin
///////////////////////////////////////////////////////////////////////

void Elwin::setup()
{
  // Create QtTreePropertyBrowser object
  m_elwTree = new QtTreePropertyBrowser();
  uiForm().elwin_properties->addWidget(m_elwTree);

  // Create Manager Objects
  m_elwDblMng = new QtDoublePropertyManager();
  m_elwBlnMng = new QtBoolPropertyManager();
  m_elwGrpMng = new QtGroupPropertyManager();

  // Editor Factories
  m_elwTree->setFactoryForManager(m_elwDblMng, doubleEditorFactory());
  m_elwTree->setFactoryForManager(m_elwBlnMng, qtCheckBoxFactory());

  // Create Properties
  m_elwProp["R1S"] = m_elwDblMng->addProperty("Start");
  m_elwDblMng->setDecimals(m_elwProp["R1S"], NUM_DECIMALS);
  m_elwProp["R1E"] = m_elwDblMng->addProperty("End");
  m_elwDblMng->setDecimals(m_elwProp["R1E"], NUM_DECIMALS);  
  m_elwProp["R2S"] = m_elwDblMng->addProperty("Start");
  m_elwDblMng->setDecimals(m_elwProp["R2S"], NUM_DECIMALS);
  m_elwProp["R2E"] = m_elwDblMng->addProperty("End");
  m_elwDblMng->setDecimals(m_elwProp["R2E"], NUM_DECIMALS);

  m_elwProp["UseTwoRanges"] = m_elwBlnMng->addProperty("Use Two Ranges");

  m_elwProp["Range1"] = m_elwGrpMng->addProperty("Range One");
  m_elwProp["Range1"]->addSubProperty(m_elwProp["R1S"]);
  m_elwProp["Range1"]->addSubProperty(m_elwProp["R1E"]);
  m_elwProp["Range2"] = m_elwGrpMng->addProperty("Range Two");
  m_elwProp["Range2"]->addSubProperty(m_elwProp["R2S"]);
  m_elwProp["Range2"]->addSubProperty(m_elwProp["R2E"]);

  m_elwTree->addProperty(m_elwProp["Range1"]);
  m_elwTree->addProperty(m_elwProp["UseTwoRanges"]);
  m_elwTree->addProperty(m_elwProp["Range2"]);

  // Create Slice Plot Widget for Range Selection
  m_elwPlot = new QwtPlot(this);
  m_elwPlot->setAxisFont(QwtPlot::xBottom, this->font());
  m_elwPlot->setAxisFont(QwtPlot::yLeft, this->font());
  uiForm().elwin_plot->addWidget(m_elwPlot);
  m_elwPlot->setCanvasBackground(Qt::white);
  // We always want one range selector... the second one can be controlled from
  // within the elwinTwoRanges(bool state) function
  m_elwR1 = new MantidWidgets::RangeSelector(m_elwPlot);
  connect(m_elwR1, SIGNAL(minValueChanged(double)), this, SLOT(elwinMinChanged(double)));
  connect(m_elwR1, SIGNAL(maxValueChanged(double)), this, SLOT(elwinMaxChanged(double)));
  // create the second range
  m_elwR2 = new MantidWidgets::RangeSelector(m_elwPlot);
  m_elwR2->setColour(Qt::darkGreen); // dark green for background
  connect(m_elwR1, SIGNAL(rangeChanged(double, double)), m_elwR2, SLOT(setRange(double, double)));
  connect(m_elwR2, SIGNAL(minValueChanged(double)), this, SLOT(elwinMinChanged(double)));
  connect(m_elwR2, SIGNAL(maxValueChanged(double)), this, SLOT(elwinMaxChanged(double)));
  m_elwR2->setRange(m_elwR1->getRange());
  // Refresh the plot window
  m_elwPlot->replot();
  
  connect(m_elwDblMng, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(elwinUpdateRS(QtProperty*, double)));
  connect(m_elwBlnMng, SIGNAL(valueChanged(QtProperty*, bool)), this, SLOT(elwinTwoRanges(QtProperty*, bool)));
  twoRanges(0, false);

  // m_uiForm element signals and slots
  connect(uiForm().elwin_pbPlotInput, SIGNAL(clicked()), this, SLOT(elwinPlotInput()));

  // Set any default values
  m_elwDblMng->setValue(m_elwProp["R1S"], -0.02);
  m_elwDblMng->setValue(m_elwProp["R1E"], 0.02);
}

void Elwin::loadSettings(const QSettings & settings)
{
  uiForm().elwin_inputFile->readSettings(settings.group());
}

///////////////////////////////////////////////////////////////////////
// MSDFit
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// Fury
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// FuryFit
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// ConFit
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// AbsorptionF2Py
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// AbsCor
///////////////////////////////////////////////////////////////////////

































void MSDFit::loadSettings(const QSettings & settings)
{
  uiForm().msd_inputFile->readSettings(settings.group());
}

void Fury::loadSettings(const QSettings & settings)
{
  uiForm().fury_iconFile->readSettings(settings.group());
  uiForm().fury_resFile->readSettings(settings.group());
}

void FuryFit::loadSettings(const QSettings & settings)
{
  uiForm().furyfit_inputFile->readSettings(settings.group());
}

void ConFit::loadSettings(const QSettings & settings)
{
  uiForm().confit_inputFile->readSettings(settings.group());
  uiForm().confit_resInput->readSettings(settings.group());
}

void AbsorptionF2Py::loadSettings(const QSettings & settings)
{
  uiForm().absp_inputFile->readSettings(settings.group());
}

void AbsCor::loadSettings(const QSettings & settings)
{
  uiForm().abscor_sample->readSettings(settings.group());
  uiForm().abscor_can->readSettings(settings.group());
}



void MSDFit::setup()
{
  // Tree Browser
  m_msdTree = new QtTreePropertyBrowser();
  uiForm().msd_properties->addWidget(m_msdTree);

  m_msdDblMng = new QtDoublePropertyManager();

  m_msdTree->setFactoryForManager(m_msdDblMng, doubleEditorFactory());

  m_msdProp["Start"] = m_msdDblMng->addProperty("StartX");
  m_msdDblMng->setDecimals(m_msdProp["Start"], NUM_DECIMALS);
  m_msdProp["End"] = m_msdDblMng->addProperty("EndX");
  m_msdDblMng->setDecimals(m_msdProp["End"], NUM_DECIMALS);

  m_msdTree->addProperty(m_msdProp["Start"]);
  m_msdTree->addProperty(m_msdProp["End"]);

  m_msdPlot = new QwtPlot(this);
  uiForm().msd_plot->addWidget(m_msdPlot);

  // Cosmetics
  m_msdPlot->setAxisFont(QwtPlot::xBottom, this->font());
  m_msdPlot->setAxisFont(QwtPlot::yLeft, this->font());
  m_msdPlot->setCanvasBackground(Qt::white);

  m_msdRange = new MantidWidgets::RangeSelector(m_msdPlot);

  connect(m_msdRange, SIGNAL(minValueChanged(double)), this, SLOT(msdMinChanged(double)));
  connect(m_msdRange, SIGNAL(maxValueChanged(double)), this, SLOT(msdMaxChanged(double)));
  connect(m_msdDblMng, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(msdUpdateRS(QtProperty*, double)));

  connect(uiForm().msd_pbPlotInput, SIGNAL(clicked()), this, SLOT(msdPlotInput()));
}

void Fury::setup()
{
  m_furTree = new QtTreePropertyBrowser();
  uiForm().fury_TreeSpace->addWidget(m_furTree);

  m_furDblMng = new QtDoublePropertyManager();

  m_furPlot = new QwtPlot(this);
  uiForm().fury_PlotSpace->addWidget(m_furPlot);
  m_furPlot->setCanvasBackground(Qt::white);
  m_furPlot->setAxisFont(QwtPlot::xBottom, this->font());
  m_furPlot->setAxisFont(QwtPlot::yLeft, this->font());

  m_furProp["ELow"] = m_furDblMng->addProperty("ELow");
  m_furDblMng->setDecimals(m_furProp["ELow"], NUM_DECIMALS);
  m_furProp["EWidth"] = m_furDblMng->addProperty("EWidth");
  m_furDblMng->setDecimals(m_furProp["EWidth"], NUM_DECIMALS);
  m_furProp["EHigh"] = m_furDblMng->addProperty("EHigh");
  m_furDblMng->setDecimals(m_furProp["EHigh"], NUM_DECIMALS);

  m_furTree->addProperty(m_furProp["ELow"]);
  m_furTree->addProperty(m_furProp["EWidth"]);
  m_furTree->addProperty(m_furProp["EHigh"]);

  m_furTree->setFactoryForManager(m_furDblMng, doubleEditorFactory());

  m_furRange = new MantidQt::MantidWidgets::RangeSelector(m_furPlot);

  // signals / slots & validators
  connect(m_furRange, SIGNAL(minValueChanged(double)), this, SLOT(furyMinChanged(double)));
  connect(m_furRange, SIGNAL(maxValueChanged(double)), this, SLOT(furyMaxChanged(double)));
  connect(m_furDblMng, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(furyUpdateRS(QtProperty*, double)));
  
  connect(uiForm().fury_cbInputType, SIGNAL(currentIndexChanged(int)), uiForm().fury_swInput, SLOT(setCurrentIndex(int)));  
  connect(uiForm().fury_cbResType, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(furyResType(const QString&)));
  connect(uiForm().fury_pbPlotInput, SIGNAL(clicked()), this, SLOT(furyPlotInput()));
}

void FuryFit::setup()
{
  m_ffTree = new QtTreePropertyBrowser();
  uiForm().furyfit_properties->addWidget(m_ffTree);
  
  // Setup FuryFit Plot Window
  m_ffPlot = new QwtPlot(this);
  m_ffPlot->setAxisFont(QwtPlot::xBottom, this->font());
  m_ffPlot->setAxisFont(QwtPlot::yLeft, this->font());
  uiForm().furyfit_vlPlot->addWidget(m_ffPlot);
  m_ffPlot->setCanvasBackground(QColor(255,255,255));
  
  m_ffRangeS = new MantidQt::MantidWidgets::RangeSelector(m_ffPlot);
  connect(m_ffRangeS, SIGNAL(minValueChanged(double)), this, SLOT(furyfitXMinSelected(double)));
  connect(m_ffRangeS, SIGNAL(maxValueChanged(double)), this, SLOT(furyfitXMaxSelected(double)));

  m_ffBackRangeS = new MantidQt::MantidWidgets::RangeSelector(m_ffPlot,
    MantidQt::MantidWidgets::RangeSelector::YSINGLE);
  m_ffBackRangeS->setRange(0.0,1.0);
  m_ffBackRangeS->setColour(Qt::darkGreen);
  connect(m_ffBackRangeS, SIGNAL(minValueChanged(double)), this, SLOT(furyfitBackgroundSelected(double)));

  // setupTreePropertyBrowser
  m_groupManager = new QtGroupPropertyManager();
  m_ffDblMng = new QtDoublePropertyManager();
  m_ffRangeManager = new QtDoublePropertyManager();
  
  m_ffTree->setFactoryForManager(m_ffDblMng, doubleEditorFactory());
  m_ffTree->setFactoryForManager(m_ffRangeManager, doubleEditorFactory());

  m_ffProp["StartX"] = m_ffRangeManager->addProperty("StartX");
  m_ffRangeManager->setDecimals(m_ffProp["StartX"], NUM_DECIMALS);
  m_ffProp["EndX"] = m_ffRangeManager->addProperty("EndX");
  m_ffRangeManager->setDecimals(m_ffProp["EndX"], NUM_DECIMALS);

  connect(m_ffRangeManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(furyfitRangePropChanged(QtProperty*, double)));

  m_ffProp["LinearBackground"] = m_groupManager->addProperty("LinearBackground");
  m_ffProp["BackgroundA0"] = m_ffRangeManager->addProperty("A0");
  m_ffRangeManager->setDecimals(m_ffProp["BackgroundA0"], NUM_DECIMALS);
  m_ffProp["LinearBackground"]->addSubProperty(m_ffProp["BackgroundA0"]);

  m_ffProp["Exponential1"] = createExponential("Exponential 1");
  m_ffProp["Exponential2"] = createExponential("Exponential 2");
  
  m_ffProp["StretchedExp"] = createStretchedExp("Stretched Exponential");

  typeSelection(uiForm().furyfit_cbFitType->currentIndex());


  // Connect to PlotGuess checkbox
  connect(m_ffDblMng, SIGNAL(propertyChanged(QtProperty*)), this, SLOT(furyfitPlotGuess(QtProperty*)));

  // Signal/slot ui connections
  connect(uiForm().furyfit_inputFile, SIGNAL(fileEditingFinished()), this, SLOT(furyfitPlotInput()));
  connect(uiForm().furyfit_cbFitType, SIGNAL(currentIndexChanged(int)), this, SLOT(furyfitTypeSelection(int)));
  connect(uiForm().furyfit_pbPlotInput, SIGNAL(clicked()), this, SLOT(furyfitPlotInput()));
  connect(uiForm().furyfit_leSpecNo, SIGNAL(editingFinished()), this, SLOT(furyfitPlotInput()));
  connect(uiForm().furyfit_cbInputType, SIGNAL(currentIndexChanged(int)), uiForm().furyfit_swInput, SLOT(setCurrentIndex(int)));  
  connect(uiForm().furyfit_pbSeqFit, SIGNAL(clicked()), this, SLOT(furyfitSequential()));
  // apply validators - furyfit
  uiForm().furyfit_leSpecNo->setValidator(m_intVal);

  // Set a custom handler for the QTreePropertyBrowser's ContextMenu event
  m_ffTree->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_ffTree, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(fitContextMenu(const QPoint &)));
}

void ConFit::setup()
{
  // Create Property Managers
  m_cfGrpMng = new QtGroupPropertyManager();
  m_cfBlnMng = new QtBoolPropertyManager();
  m_cfDblMng = new QtDoublePropertyManager();

  // Create TreeProperty Widget
  m_cfTree = new QtTreePropertyBrowser();
  uiForm().confit_properties->addWidget(m_cfTree);

  // add factories to managers
  m_cfTree->setFactoryForManager(m_cfBlnMng, qtCheckBoxFactory());
  m_cfTree->setFactoryForManager(m_cfDblMng, doubleEditorFactory());

  // Create Plot Widget
  m_cfPlot = new QwtPlot(this);
  m_cfPlot->setAxisFont(QwtPlot::xBottom, this->font());
  m_cfPlot->setAxisFont(QwtPlot::yLeft, this->font());
  m_cfPlot->setCanvasBackground(Qt::white);
  uiForm().confit_plot->addWidget(m_cfPlot);

  // Create Range Selectors
  m_cfRangeS = new MantidQt::MantidWidgets::RangeSelector(m_cfPlot);
  m_cfBackgS = new MantidQt::MantidWidgets::RangeSelector(m_cfPlot, 
    MantidQt::MantidWidgets::RangeSelector::YSINGLE);
  m_cfBackgS->setColour(Qt::darkGreen);
  m_cfBackgS->setRange(0.0, 1.0);
  m_cfHwhmRange = new MantidQt::MantidWidgets::RangeSelector(m_cfPlot);
  m_cfHwhmRange->setColour(Qt::red);

  // Populate Property Widget
  m_cfProp["FitRange"] = m_cfGrpMng->addProperty("Fitting Range");
  m_cfProp["StartX"] = m_cfDblMng->addProperty("StartX");
  m_cfDblMng->setDecimals(m_cfProp["StartX"], NUM_DECIMALS);
  m_cfProp["EndX"] = m_cfDblMng->addProperty("EndX");
  m_cfDblMng->setDecimals(m_cfProp["EndX"], NUM_DECIMALS);
  m_cfProp["FitRange"]->addSubProperty(m_cfProp["StartX"]);
  m_cfProp["FitRange"]->addSubProperty(m_cfProp["EndX"]);
  m_cfTree->addProperty(m_cfProp["FitRange"]);

  m_cfProp["LinearBackground"] = m_cfGrpMng->addProperty("Background");
  m_cfProp["BGA0"] = m_cfDblMng->addProperty("A0");
  m_cfDblMng->setDecimals(m_cfProp["BGA0"], NUM_DECIMALS);
  m_cfProp["BGA1"] = m_cfDblMng->addProperty("A1");
  m_cfDblMng->setDecimals(m_cfProp["BGA1"], NUM_DECIMALS);
  m_cfProp["LinearBackground"]->addSubProperty(m_cfProp["BGA0"]);
  m_cfProp["LinearBackground"]->addSubProperty(m_cfProp["BGA1"]);
  m_cfTree->addProperty(m_cfProp["LinearBackground"]);

  // Delta Function
  m_cfProp["DeltaFunction"] = m_cfGrpMng->addProperty("Delta Function");
  m_cfProp["UseDeltaFunc"] = m_cfBlnMng->addProperty("Use");
  m_cfProp["DeltaHeight"] = m_cfDblMng->addProperty("Height");
  m_cfDblMng->setDecimals(m_cfProp["DeltaHeight"], NUM_DECIMALS);
  m_cfProp["DeltaFunction"]->addSubProperty(m_cfProp["UseDeltaFunc"]);
  m_cfTree->addProperty(m_cfProp["DeltaFunction"]);

  m_cfProp["Lorentzian1"] = createLorentzian("Lorentzian 1");
  m_cfProp["Lorentzian2"] = createLorentzian("Lorentzian 2");

  // Connections
  connect(m_cfRangeS, SIGNAL(minValueChanged(double)), this, SLOT(confitMinChanged(double)));
  connect(m_cfRangeS, SIGNAL(maxValueChanged(double)), this, SLOT(confitMaxChanged(double)));
  connect(m_cfBackgS, SIGNAL(minValueChanged(double)), this, SLOT(confitBackgLevel(double)));
  connect(m_cfHwhmRange, SIGNAL(minValueChanged(double)), this, SLOT(confitHwhmChanged(double)));
  connect(m_cfHwhmRange, SIGNAL(maxValueChanged(double)), this, SLOT(confitHwhmChanged(double)));
  connect(m_cfDblMng, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(confitUpdateRS(QtProperty*, double)));
  connect(m_cfBlnMng, SIGNAL(valueChanged(QtProperty*, bool)), this, SLOT(confitCheckBoxUpdate(QtProperty*, bool)));

  connect(m_cfDblMng, SIGNAL(propertyChanged(QtProperty*)), this, SLOT(confitPlotGuess(QtProperty*)));

  // Have HWHM Range linked to Fit Start/End Range
  connect(m_cfRangeS, SIGNAL(rangeChanged(double, double)), m_cfHwhmRange, SLOT(setRange(double, double)));
  m_cfHwhmRange->setRange(-1.0,1.0);
  hwhmUpdateRS(0.02);

  typeSelection(uiForm().confit_cbFitType->currentIndex());
  bgTypeSelection(uiForm().confit_cbBackground->currentIndex());

  // Replot input automatically when file / spec no changes
  connect(uiForm().confit_leSpecNo, SIGNAL(editingFinished()), this, SLOT(confitPlotInput()));
  connect(uiForm().confit_inputFile, SIGNAL(fileEditingFinished()), this, SLOT(confitPlotInput()));
  
  connect(uiForm().confit_cbInputType, SIGNAL(currentIndexChanged(int)), uiForm().confit_swInput, SLOT(setCurrentIndex(int)));
  connect(uiForm().confit_cbFitType, SIGNAL(currentIndexChanged(int)), this, SLOT(confitTypeSelection(int)));
  connect(uiForm().confit_cbBackground, SIGNAL(currentIndexChanged(int)), this, SLOT(confitBgTypeSelection(int)));
  connect(uiForm().confit_pbPlotInput, SIGNAL(clicked()), this, SLOT(confitPlotInput()));
  connect(uiForm().confit_pbSequential, SIGNAL(clicked()), this, SLOT(confitSequential()));

  uiForm().confit_leSpecNo->setValidator(m_intVal);
  uiForm().confit_leSpecMax->setValidator(m_intVal);

  // Context menu
  m_cfTree->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_cfTree, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(fitContextMenu(const QPoint &)));
}

void AbsorptionF2Py::setup()
{
  // set signals and slot connections for F2Py Absorption routine
  connect(uiForm().absp_cbInputType, SIGNAL(currentIndexChanged(int)), uiForm().absp_swInput, SLOT(setCurrentIndex(int)));
  connect(uiForm().absp_cbShape, SIGNAL(currentIndexChanged(int)), this, SLOT(absf2pShape(int)));
  connect(uiForm().absp_ckUseCan, SIGNAL(toggled(bool)), this, SLOT(absf2pUseCanChecked(bool)));
  connect(uiForm().absp_letc1, SIGNAL(editingFinished()), this, SLOT(absf2pTCSync()));
  // apply QValidators to items.
  uiForm().absp_lewidth->setValidator(m_dblVal);
  uiForm().absp_leavar->setValidator(m_dblVal);
  // sample
  uiForm().absp_lesamden->setValidator(m_dblVal);
  uiForm().absp_lesamsigs->setValidator(m_dblVal);
  uiForm().absp_lesamsiga->setValidator(m_dblVal);
  // can
  uiForm().absp_lecanden->setValidator(m_dblVal);
  uiForm().absp_lecansigs->setValidator(m_dblVal);
  uiForm().absp_lecansiga->setValidator(m_dblVal);
  // flat shape
  uiForm().absp_lets->setValidator(m_dblVal);
  uiForm().absp_letc1->setValidator(m_dblVal);
  uiForm().absp_letc2->setValidator(m_dblVal);
  // cylinder shape
  uiForm().absp_ler1->setValidator(m_dblVal);
  uiForm().absp_ler2->setValidator(m_dblVal);
  uiForm().absp_ler3->setValidator(m_dblVal);

  // "Nudge" color of title of QGroupBox to change.
  useCanChecked(uiForm().absp_ckUseCan->isChecked());
}

void AbsCor::setup()
{
  // Disable Container inputs is "Use Container" is not checked
  connect(uiForm().abscor_ckUseCan, SIGNAL(toggled(bool)), uiForm().abscor_lbContainerInputType, SLOT(setEnabled(bool)));
  connect(uiForm().abscor_ckUseCan, SIGNAL(toggled(bool)), uiForm().abscor_cbContainerInputType, SLOT(setEnabled(bool)));
  connect(uiForm().abscor_ckUseCan, SIGNAL(toggled(bool)), uiForm().abscor_swContainerInput, SLOT(setEnabled(bool)));

  connect(uiForm().abscor_cbSampleInputType, SIGNAL(currentIndexChanged(int)), uiForm().abscor_swSampleInput, SLOT(setCurrentIndex(int)));
  connect(uiForm().abscor_cbContainerInputType, SIGNAL(currentIndexChanged(int)), uiForm().abscor_swContainerInput, SLOT(setCurrentIndex(int)));
}

QString Elwin::validate()
{
  if ( ! uiForm().elwin_inputFile->isValid() )
    return uiForm().elwin_inputFile->getFileProblem();

  return "";
}

QString MSDFit::validate()
{
  if ( ! uiForm().msd_inputFile->isValid() )
    return uiForm().msd_inputFile->getFileProblem();

  return "";
}

QString Fury::validate()
{
  switch ( uiForm().fury_cbInputType->currentIndex() )
  {
  case 0: // File
    {
      if ( ! uiForm().fury_iconFile->isValid() )
        return "Empty or otherwise invalid reduction file field.";
    }
    break;
  case 1: // Workspace
    {
      if ( uiForm().fury_wsSample->currentText() == "" )
        return "No workspace selected.";
    }
    break;
  }

  if ( ! uiForm().fury_resFile->isValid()  )
    return "Invalid or empty resolution file field.";

  return "";
}

QString FuryFit::validate()
{
  return "";
}

/**
 * Validates the user's inputs in the ConvFit tab.
 *
 * @returns an string containing an error message if invalid input detected, else an empty string.
 */
QString ConFit::validate()
{
  if ( uiForm().confit_cbInputType->currentIndex() == 0 ) // File
  {
    if ( ! uiForm().confit_inputFile->isValid() )
      return "Empty or otherwise invalid file field.";
  }
  else // Workspace
  {
    if ( uiForm().confit_wsSample->currentText() == "" )
      return "No workspace selected.";
  }

  if( ! uiForm().confit_resInput->isValid() )
    return "Invalid or empty resolution file field.";

  // Enforce the rule that at least one fit is needed; either a delta function, one or two lorentzian functions,
  // or both.  (The resolution function must be convolved with a model.)
  if ( uiForm().confit_cbFitType->currentIndex() == 0 && ! m_cfBlnMng->value(m_cfProp["UseDeltaFunc"]) )
    return "No fit function has been selected.";

  return "";
}

QString AbsorptionF2Py::validate()
{
  QStringList invalidInputs;
  
  // Input (file or workspace)
  if ( uiForm().absp_cbInputType->currentText() == "File" )
  {
    if ( ! uiForm().absp_inputFile->isValid() )
      invalidInputs.append("Input File");
  }
  else
  {
    if ( uiForm().absp_wsInput->currentText() == "" )
      invalidInputs.append("Input Workspace");
  }

  if ( uiForm().absp_cbShape->currentText() == "Flat" )
  {
    // Flat Geometry
    if ( uiForm().absp_lets->text() != "" )
    {
      uiForm().absp_valts->setText(" ");
    }
    else
    {
      uiForm().absp_valts->setText("*");
      invalidInputs.append("Thickness");
    }

    if ( uiForm().absp_ckUseCan->isChecked() )
    {
      if ( uiForm().absp_letc1->text() != "" )
      {
        uiForm().absp_valtc1->setText(" ");
      }
      else
      {
        uiForm().absp_valtc1->setText("*");
        invalidInputs.append("Front Thickness");
      }

      if ( uiForm().absp_letc2->text() != "" )
      {
        uiForm().absp_valtc2->setText(" ");
      }
      else
      {
        uiForm().absp_valtc2->setText("*");
        invalidInputs.append("Back Thickness");
      }
    }
  }

  if ( uiForm().absp_cbShape->currentText() == "Cylinder" )
  {
    // Cylinder geometry
    if ( uiForm().absp_ler1->text() != "" )
    {
      uiForm().absp_valR1->setText(" ");
    }
    else
    {
      uiForm().absp_valR1->setText("*");
      invalidInputs.append("Radius 1");
    }

    if ( uiForm().absp_ler2->text() != "" )
    {
      uiForm().absp_valR2->setText(" ");
    }
    else
    {
      uiForm().absp_valR2->setText("*");
      invalidInputs.append("Radius 2");
    }
    
    // R3  only relevant when using can
    if ( uiForm().absp_ckUseCan->isChecked() )
    {
      if ( uiForm().absp_ler3->text() != "" )
      {
        uiForm().absp_valR3->setText(" ");
      }
      else
      {
        uiForm().absp_valR3->setText("*");
        invalidInputs.append("Radius 3");
      }
    }
  }

  // Can angle to beam || Step size
  if ( uiForm().absp_leavar->text() != "" )
  {
    uiForm().absp_valAvar->setText(" ");
  }
  else
  {
    uiForm().absp_valAvar->setText("*");
    invalidInputs.append("Can Angle to Beam");
  }

  // Beam Width
  if ( uiForm().absp_lewidth->text() != "" )
  {
    uiForm().absp_valWidth->setText(" ");
  }
  else
  {
    uiForm().absp_valWidth->setText("*");
    invalidInputs.append("Beam Width");
  }

  // Sample details
  if ( uiForm().absp_lesamden->text() != "" )
  {
    uiForm().absp_valSamden->setText(" ");
  }
  else
  {
    uiForm().absp_valSamden->setText("*");
    invalidInputs.append("Sample Number Density");
  }

  if ( uiForm().absp_lesamsigs->text() != "" )
  {
    uiForm().absp_valSamsigs->setText(" ");
  }
  else
  {
    uiForm().absp_valSamsigs->setText("*");
    invalidInputs.append("Sample Scattering Cross-Section");
  }

  if ( uiForm().absp_lesamsiga->text() != "" )
  {
    uiForm().absp_valSamsiga->setText(" ");
  }
  else
  {
    uiForm().absp_valSamsiga->setText("*");
    invalidInputs.append("Sample Absorption Cross-Section");
  }

  // Can details (only test if "Use Can" is checked)
  if ( uiForm().absp_ckUseCan->isChecked() )
  {
    if ( uiForm().absp_lecanden->text() != "" )
    {
      uiForm().absp_valCanden->setText(" ");
    }
    else
    {
      uiForm().absp_valCanden->setText("*");
      invalidInputs.append("Can Number Density");
    }

    if ( uiForm().absp_lecansigs->text() != "" )
    {
      uiForm().absp_valCansigs->setText(" ");
    }
    else
    {
      uiForm().absp_valCansigs->setText("*");
      invalidInputs.append("Can Scattering Cross-Section");
    }

    if ( uiForm().absp_lecansiga->text() != "" )
    {
      uiForm().absp_valCansiga->setText(" ");
    }
    else
    {
      uiForm().absp_valCansiga->setText("*");
      invalidInputs.append("Can Absorption Cross-Section");
    }
  }

  QString error = "Please check the following inputs: \n" + invalidInputs.join("\n");
  return error;
}

QString AbsCor::validate()
{
  return "";
}

Mantid::API::CompositeFunction_sptr FuryFit::createFunction(bool tie)
{
  Mantid::API::CompositeFunction_sptr result( new Mantid::API::CompositeFunction );
  QString fname;
  const int fitType = uiForm().furyfit_cbFitType->currentIndex();

  Mantid::API::IFunction_sptr func = Mantid::API::FunctionFactory::Instance().createFunction("LinearBackground");
  func->setParameter("A0", m_ffDblMng->value(m_ffProp["BackgroundA0"]));
  result->addFunction(func);
  result->tie("f0.A1", "0");
  if ( tie ) { result->tie("f0.A0", m_ffProp["BackgroundA0"]->valueText().toStdString()); }
  
  if ( fitType == 2 ) { fname = "Stretched Exponential"; }
  else { fname = "Exponential 1"; }

  result->addFunction(createUserFunction(fname, tie));

  if ( fitType == 1 || fitType == 3 )
  {
    if ( fitType == 1 ) { fname = "Exponential 2"; }
    else { fname = "Stretched Exponential"; }
    result->addFunction(createUserFunction(fname, tie));
  }

  // Return CompositeFunction object to caller.
  result->applyTies();
  return result;
}

Mantid::API::IFunction_sptr FuryFit::createUserFunction(const QString & name, bool tie)
{
  Mantid::API::IFunction_sptr result = Mantid::API::FunctionFactory::Instance().createFunction("UserFunction");  
  std::string formula;

  if ( name.startsWith("Exp") ) { formula = "Intensity*exp(-(x/Tau))"; }
  else { formula = "Intensity*exp(-(x/Tau)^Beta)"; }

  Mantid::API::IFunction::Attribute att(formula);  
  result->setAttribute("Formula", att);

  result->setParameter("Intensity", m_ffDblMng->value(m_ffProp[name+".Intensity"]));

  if ( tie || ! m_ffProp[name+".Intensity"]->subProperties().isEmpty() )
  {
    result->tie("Intensity", m_ffProp[name+".Intensity"]->valueText().toStdString());
  }
  result->setParameter("Tau", m_ffDblMng->value(m_ffProp[name+".Tau"]));
  if ( tie || ! m_ffProp[name+".Tau"]->subProperties().isEmpty() )
  {
    result->tie("Tau", m_ffProp[name+".Tau"]->valueText().toStdString());
  }
  if ( name.startsWith("Str") )
  {
    result->setParameter("Beta", m_ffDblMng->value(m_ffProp[name+".Beta"]));
    if ( tie || ! m_ffProp[name+".Beta"]->subProperties().isEmpty() )
    {
      result->tie("Beta", m_ffProp[name+".Beta"]->valueText().toStdString());
    }
  }

  return result;
}

namespace
{
  ////////////////////////////
  // Anon Helper functions. //
  ////////////////////////////

  /**
   * Takes an index and a name, and constructs a single level parameter name
   * for use with function ties, etc.
   *
   * @param index :: the index of the function in the first level.
   * @param name  :: the name of the parameter inside the function.
   *
   * @returns the constructed function parameter name.
   */
  std::string createParName(size_t index, const std::string & name = "")
  {
    std::stringstream prefix;
    prefix << "f" << index << "." << name;
    return prefix.str();
  }

  /**
   * Takes an index, a sub index and a name, and constructs a double level 
   * (nested) parameter name for use with function ties, etc.
   *
   * @param index    :: the index of the function in the first level.
   * @param subIndex :: the index of the function in the second level.
   * @param name     :: the name of the parameter inside the function.
   *
   * @returns the constructed function parameter name.
   */
  std::string createParName(size_t index, size_t subIndex, const std::string & name = "")
  {
    std::stringstream prefix;
    prefix << "f" << index << ".f" << subIndex << "." << name;
    return prefix.str();
  }
}

/**
 * Creates a function to carry out the fitting in the "ConvFit" tab.  The function consists
 * of various sub functions, with the following structure:
 *
 * Composite
 *  |
 *  +-- LinearBackground
 *  +-- Convolution
 *      |
 *      +-- Resolution
 *      +-- Model (AT LEAST one of the following. Composite if more than one.)
 *          |
 *          +-- DeltaFunction (yes/no)
 *          +-- Lorentzian 1 (yes/no)
 *          +-- Lorentzian 2 (yes/no)
 *
 * @param tie :: whether to tie parameters.
 *
 * @returns the composite fitting function.
 */
Mantid::API::CompositeFunction_sptr ConFit::createFunction(bool tie)
{
  auto conv = boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(Mantid::API::FunctionFactory::Instance().createFunction("Convolution"));
  Mantid::API::CompositeFunction_sptr comp( new Mantid::API::CompositeFunction );

  Mantid::API::IFunction_sptr func;
  size_t index = 0;

  // -------------------------------------
  // --- Composite / Linear Background ---
  // -------------------------------------
  func = Mantid::API::FunctionFactory::Instance().createFunction("LinearBackground");
  index = comp->addFunction(func); 

  const int bgType = uiForm().confit_cbBackground->currentIndex(); // 0 = Fixed Flat, 1 = Fit Flat, 2 = Fit all
  
  if ( tie  || bgType == 0 || ! m_cfProp["BGA0"]->subProperties().isEmpty() )
  {
    comp->tie("f0.A0", m_cfProp["BGA0"]->valueText().toStdString() );
  }
  else
  {
    func->setParameter("A0", m_cfProp["BGA0"]->valueText().toDouble());
  }

  if ( bgType != 2 )
  {
    comp->tie("f0.A1", "0.0");
  }
  else
  {
    if ( tie || ! m_cfProp["BGA1"]->subProperties().isEmpty() )
    {
      comp->tie("f0.A1", m_cfProp["BGA1"]->valueText().toStdString() );
    }
    else { func->setParameter("A1", m_cfProp["BGA1"]->valueText().toDouble()); }
  }

  // --------------------------------------------
  // --- Composite / Convolution / Resolution ---
  // --------------------------------------------
  func = Mantid::API::FunctionFactory::Instance().createFunction("Resolution");
  index = conv->addFunction(func);
  std::string resfilename = uiForm().confit_resInput->getFirstFilename().toStdString();
  Mantid::API::IFunction::Attribute attr(resfilename);
  func->setAttribute("FileName", attr);

  // --------------------------------------------------------
  // --- Composite / Convolution / Model / Delta Function ---
  // --------------------------------------------------------
  size_t subIndex = 0;

  if ( m_cfBlnMng->value(m_cfProp["UseDeltaFunc"]) )
  {
    func = Mantid::API::FunctionFactory::Instance().createFunction("DeltaFunction");
    index = conv->addFunction(func);

    if ( tie || ! m_cfProp["DeltaHeight"]->subProperties().isEmpty() )
    {
      std::string parName = createParName(index, "Height");
      conv->tie(parName, m_cfProp["DeltaHeight"]->valueText().toStdString() );
    }

    else { func->setParameter("Height", m_cfProp["DeltaHeight"]->valueText().toDouble()); }
    subIndex++;
  }
  
  // -----------------------------------------------------
  // --- Composite / Convolution / Model / Lorentzians ---
  // -----------------------------------------------------
  std::string prefix1;
  std::string prefix2;
  switch ( uiForm().confit_cbFitType->currentIndex() )
  {
  case 0: // No Lorentzians

    break;

  case 1: // 1 Lorentzian

    func = Mantid::API::FunctionFactory::Instance().createFunction("Lorentzian");
    index = conv->addFunction(func);

    // If it's the first "sub" function of model, then it wont be nested inside Convolution ...
    if( subIndex == 0 ) { prefix1 = createParName(index); }
    // ... else it's part of a composite function inside Convolution.
    else { prefix1 = createParName(index, subIndex); }

    populateFunction(func, conv, m_cfProp["Lorentzian1"], prefix1, tie);
    subIndex++;
    break;

  case 2: // 2 Lorentzians

    func = Mantid::API::FunctionFactory::Instance().createFunction("Lorentzian");
    index = conv->addFunction(func);

    // If it's the first "sub" function of model, then it wont be nested inside Convolution ...
    if( subIndex == 0 ) { prefix1 = createParName(index); }
    // ... else it's part of a composite function inside Convolution.
    else { prefix1 = createParName(index, subIndex); }

    populateFunction(func, conv, m_cfProp["Lorentzian1"], prefix1, tie);
    subIndex++;

    func = Mantid::API::FunctionFactory::Instance().createFunction("Lorentzian");
    index = conv->addFunction(func);

    prefix2 = createParName(index, subIndex); // (Part of a composite.)
    populateFunction(func, conv, m_cfProp["Lorentzian2"], prefix2, tie);

    // Now prefix1 should be changed to reflect the fact that it is now part of a composite function inside Convolution.
    prefix1 = createParName(index, subIndex-1);

    // Tie PeakCentres together
    if ( ! tie )
    {
      QString tieL = QString::fromStdString(prefix1 + "PeakCentre");
      QString tieR = QString::fromStdString(prefix2 + "PeakCentre");
      conv->tie(tieL.toStdString(), tieR.toStdString());
    }
    break;
  }

  comp->addFunction(conv);

  comp->applyTies();

  return comp;
}

QtProperty* ConFit::createLorentzian(const QString & name)
{
  QtProperty* lorentzGroup = m_cfGrpMng->addProperty(name);
  m_cfProp[name+".Height"] = m_cfDblMng->addProperty("Height");
  // m_cfDblMng->setRange(m_cfProp[name+".Height"], 0.0, 1.0); // 0 < Height < 1
  m_cfProp[name+".PeakCentre"] = m_cfDblMng->addProperty("PeakCentre");
  m_cfProp[name+".HWHM"] = m_cfDblMng->addProperty("HWHM");
  m_cfDblMng->setDecimals(m_cfProp[name+".Height"], NUM_DECIMALS);
  m_cfDblMng->setDecimals(m_cfProp[name+".PeakCentre"], NUM_DECIMALS);
  m_cfDblMng->setDecimals(m_cfProp[name+".HWHM"], NUM_DECIMALS);
  m_cfDblMng->setValue(m_cfProp[name+".HWHM"], 0.02);
  lorentzGroup->addSubProperty(m_cfProp[name+".Height"]);
  lorentzGroup->addSubProperty(m_cfProp[name+".PeakCentre"]);
  lorentzGroup->addSubProperty(m_cfProp[name+".HWHM"]);
  return lorentzGroup;
}

QtProperty* FuryFit::createExponential(const QString & name)
{
  QtProperty* expGroup = m_groupManager->addProperty(name);
  m_ffProp[name+".Intensity"] = m_ffDblMng->addProperty("Intensity");
  m_ffDblMng->setDecimals(m_ffProp[name+".Intensity"], NUM_DECIMALS);
  m_ffProp[name+".Tau"] = m_ffDblMng->addProperty("Tau");
  m_ffDblMng->setDecimals(m_ffProp[name+".Tau"], NUM_DECIMALS);
  expGroup->addSubProperty(m_ffProp[name+".Intensity"]);
  expGroup->addSubProperty(m_ffProp[name+".Tau"]);
  return expGroup;
}

QtProperty* FuryFit::createStretchedExp(const QString & name)
{
  QtProperty* prop = m_groupManager->addProperty(name);
  m_ffProp[name+".Intensity"] = m_ffDblMng->addProperty("Intensity");
  m_ffProp[name+".Tau"] = m_ffDblMng->addProperty("Tau");
  m_ffProp[name+".Beta"] = m_ffDblMng->addProperty("Beta");
  m_ffDblMng->setDecimals(m_ffProp[name+".Intensity"], NUM_DECIMALS);
  m_ffDblMng->setDecimals(m_ffProp[name+".Tau"], NUM_DECIMALS);
  m_ffDblMng->setDecimals(m_ffProp[name+".Beta"], NUM_DECIMALS);
  prop->addSubProperty(m_ffProp[name+".Intensity"]);
  prop->addSubProperty(m_ffProp[name+".Tau"]);
  prop->addSubProperty(m_ffProp[name+".Beta"]);
  return prop;
}

void ConFit::populateFunction(Mantid::API::IFunction_sptr func, Mantid::API::IFunction_sptr comp, QtProperty* group, const std::string & pref, bool tie)
{
  // Get subproperties of group and apply them as parameters on the function object
  QList<QtProperty*> props = group->subProperties();

  for ( int i = 0; i < props.size(); i++ )
  {
    if ( tie || ! props[i]->subProperties().isEmpty() )
    {
      std::string name = pref + props[i]->propertyName().toStdString();
      std::string value = props[i]->valueText().toStdString();
      comp->tie(name, value );
    }
    else
    {
      std::string propName = props[i]->propertyName().toStdString();
      double propValue = props[i]->valueText().toDouble();
      func->setParameter(propName, propValue);
    }
  }
}

QwtPlotCurve* IDATab::plotMiniplot(QwtPlot* plot, QwtPlotCurve* curve, const std::string & workspace, size_t index)
{
  if ( curve != NULL )
  {
    curve->attach(0);
    delete curve;
    curve = 0;
  }

  Mantid::API::MatrixWorkspace_const_sptr ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(workspace));

  size_t nhist = ws->getNumberHistograms();
  if ( index >= nhist )
  {
    showInformationBox("Error: Workspace index out of range.");
    return NULL;
  }

  using Mantid::MantidVec;
  const MantidVec & dataX = ws->readX(index);
  const MantidVec & dataY = ws->readY(index);

  curve = new QwtPlotCurve();
  curve->setData(&dataX[0], &dataY[0], static_cast<int>(ws->blocksize()));
  curve->attach(plot);

  plot->replot();

  return curve;
}

/**
 * Returns the range of the given curve data.
 * @param curve :: A Qwt plot curve
 * @returns A pair of doubles indicating the range
 * @throws std::invalid_argument If the curve has too few points (<2) or is NULL
 */
std::pair<double,double> IDATab::getCurveRange(QwtPlotCurve* curve)
{
  if( !curve )
  {
    throw std::invalid_argument("Invalid curve as argument to getCurveRange");
  }  
  size_t npts = curve->data().size();
  if( npts < 2 )
  {
    throw std::invalid_argument("Too few points on data curve to determine range.");
  }
  return std::make_pair(curve->data().x(0), curve->data().x(npts-1));
}


void IndirectDataAnalysis::run()
{
  const unsigned int TAB = m_uiForm.tabWidget->currentIndex();

  m_tabs[TAB]->runTab();
}

void IDATab::fitContextMenu(const QPoint &)
{
  /*
  QtBrowserItem* item(NULL);
  QtDoublePropertyManager* dblMng(NULL);

  int pageNo = uiForm().tabWidget->currentIndex();
  if ( pageNo == 3 )
  { // FuryFit
    item = m_ffTree->currentItem();
    dblMng = m_ffDblMng;  
  }
  else if ( pageNo == 4 )
  { // Convolution Fit
    item = m_cfTree->currentItem();
    dblMng = m_cfDblMng;
  }

  if ( ! item )
  {
    return;
  }

  // is it a fit property ?
  QtProperty* prop = item->property();

  if ( pageNo == 4 && ( prop == m_cfProp["StartX"] || prop == m_cfProp["EndX"] ) )
  {
    return;
  }

  // is it already fixed?
  bool fixed = prop->propertyManager() != dblMng;

  if ( fixed && prop->propertyManager() != m_stringManager ) { return; }

  // Create the menu
  QMenu* menu = new QMenu("FuryFit", m_ffTree);
  QAction* action;

  if ( ! fixed )
  {
    action = new QAction("Fix", this);
    connect(action, SIGNAL(triggered()), this, SLOT(fixItem()));
  }
  else
  {
    action = new QAction("Remove Fix", this);
    connect(action, SIGNAL(triggered()), this, SLOT(unFixItem()));
  }

  menu->addAction(action);

  // Show the menu
  menu->popup(QCursor::pos());
  */
}

void IDATab::fixItem()
{
  /*
  int pageNo = uiForm().tabWidget->currentIndex();

  QtBrowserItem* item(NULL);
  if ( pageNo == 3 )
  { // FuryFit
    item = m_ffTree->currentItem();
  }
  else if ( pageNo == 4 )
  { // Convolution Fit
    item = m_cfTree->currentItem();
  }

  // Determine what the property is.
  QtProperty* prop = item->property();

  QtProperty* fixedProp = m_stringManager->addProperty( prop->propertyName() );
  QtProperty* fprlbl = m_stringManager->addProperty("Fixed");
  fixedProp->addSubProperty(fprlbl);
  m_stringManager->setValue(fixedProp, prop->valueText());

  item->parent()->property()->addSubProperty(fixedProp);

  m_fixedProps[fixedProp] = prop;

  item->parent()->property()->removeSubProperty(prop);
  */
}

void IDATab::unFixItem()
{
  /*
  QtBrowserItem* item(NULL);
  
  int pageNo = uiForm().tabWidget->currentIndex();
  if ( pageNo == 3 )
  { // FuryFit
    item = m_ffTree->currentItem();
  }
  else if ( pageNo == 4 )
  { // Convolution Fit
    item = m_cfTree->currentItem();
  }

  QtProperty* prop = item->property();
  if ( prop->subProperties().empty() )
  { 
    item = item->parent();
    prop = item->property();
  }

  item->parent()->property()->addSubProperty(m_fixedProps[prop]);
  item->parent()->property()->removeSubProperty(prop);
  m_fixedProps.remove(prop);
  QtProperty* proplbl = prop->subProperties()[0];
  delete proplbl;
  delete prop;
  */
}

void Elwin::run()
{
  QString pyInput =
    "from IndirectDataAnalysis import elwin\n"
    "input = [r'" + uiForm().elwin_inputFile->getFilenames().join("', r'") + "']\n"
    "eRange = [ " + QString::number(m_elwDblMng->value(m_elwProp["R1S"])) +","+ QString::number(m_elwDblMng->value(m_elwProp["R1E"]));

  if ( m_elwBlnMng->value(m_elwProp["UseTwoRanges"]) )
  {
    pyInput += ", " + QString::number(m_elwDblMng->value(m_elwProp["R2S"])) + ", " + QString::number(m_elwDblMng->value(m_elwProp["R2E"]));
  }

  pyInput+= "]\n";

  if ( uiForm().elwin_ckVerbose->isChecked() ) pyInput += "verbose = True\n";
  else pyInput += "verbose = False\n";

  if ( uiForm().elwin_ckPlot->isChecked() ) pyInput += "plot = True\n";
  else pyInput += "plot = False\n";

  if ( uiForm().elwin_ckSave->isChecked() ) pyInput += "save = True\n";
  else pyInput += "save = False\n";

  pyInput +=
    "eq1_ws, eq2_ws = elwin(input, eRange, Save=save, Verbose=verbose, Plot=plot)\n";

  if ( uiForm().elwin_ckConcat->isChecked() )
  {
    pyInput += "from IndirectDataAnalysis import concatWSs\n"
      "concatWSs(eq1_ws, 'MomentumTransfer', 'ElwinQResults')\n"
      "concatWSs(eq2_ws, 'QSquared', 'ElwinQSqResults')\n";
  }

  QString pyOutput = runPythonCode(pyInput).trimmed();

}

void Elwin::plotInput()
{
  if ( uiForm().elwin_inputFile->isValid() )
  {
    QString filename = uiForm().elwin_inputFile->getFirstFilename();
    QFileInfo fi(filename);
    QString wsname = fi.baseName();

    QString pyInput = "LoadNexus(r'" + filename + "', '" + wsname + "')\n";
    QString pyOutput = runPythonCode(pyInput);

    std::string workspace = wsname.toStdString();

    m_elwDataCurve = plotMiniplot(m_elwPlot, m_elwDataCurve, workspace, 0);
    try
    {
      const std::pair<double, double> range = getCurveRange(m_elwDataCurve);
      m_elwR1->setRange(range.first, range.second);
      // Replot
      m_elwPlot->replot();
    }
    catch(std::invalid_argument & exc)
    {
      showInformationBox(exc.what());
    }

  }
  else
  {
    showInformationBox("Selected input files are invalid.");
  }
}

void Elwin::twoRanges(QtProperty*, bool val)
{
  m_elwR2->setVisible(val);
}

void Elwin::minChanged(double val)
{
  MantidWidgets::RangeSelector* from = qobject_cast<MantidWidgets::RangeSelector*>(sender());
  if ( from == m_elwR1 )
  {
    m_elwDblMng->setValue(m_elwProp["R1S"], val);
  }
  else if ( from == m_elwR2 )
  {
    m_elwDblMng->setValue(m_elwProp["R2S"], val);
  }
}

void Elwin::maxChanged(double val)
{
  MantidWidgets::RangeSelector* from = qobject_cast<MantidWidgets::RangeSelector*>(sender());
  if ( from == m_elwR1 )
  {
    m_elwDblMng->setValue(m_elwProp["R1E"], val);
  }
  else if ( from == m_elwR2 )
  {
    m_elwDblMng->setValue(m_elwProp["R2E"], val);
  }
}

void Elwin::updateRS(QtProperty* prop, double val)
{
  if ( prop == m_elwProp["R1S"] ) m_elwR1->setMinimum(val);
  else if ( prop == m_elwProp["R1E"] ) m_elwR1->setMaximum(val);
  else if ( prop == m_elwProp["R2S"] ) m_elwR2->setMinimum(val);
  else if ( prop == m_elwProp["R2E"] ) m_elwR2->setMaximum(val);
}

void MSDFit::run()
{
  QString pyInput =
    "from IndirectDataAnalysis import msdfit\n"
    "startX = " + QString::number(m_msdDblMng->value(m_msdProp["Start"])) +"\n"
    "endX = " + QString::number(m_msdDblMng->value(m_msdProp["End"])) +"\n"
    "inputs = [r'" + uiForm().msd_inputFile->getFilenames().join("', r'") + "']\n";

  if ( uiForm().msd_ckVerbose->isChecked() ) pyInput += "verbose = True\n";
  else pyInput += "verbose = False\n";

  if ( uiForm().msd_ckPlot->isChecked() ) pyInput += "plot = True\n";
  else pyInput += "plot = False\n";

  if ( uiForm().msd_ckSave->isChecked() ) pyInput += "save = True\n";
  else pyInput += "save = False\n";

  pyInput +=
    "msdfit(inputs, startX, endX, Save=save, Verbose=verbose, Plot=plot)\n";

  QString pyOutput = runPythonCode(pyInput).trimmed();
}

void MSDFit::plotInput()
{
  if ( uiForm().msd_inputFile->isValid() )
  {
    QString filename = uiForm().msd_inputFile->getFirstFilename();
    QFileInfo fi(filename);
    QString wsname = fi.baseName();

    QString pyInput = "LoadNexus(r'" + filename + "', '" + wsname + "')\n";
    QString pyOutput = runPythonCode(pyInput);

    std::string workspace = wsname.toStdString();

    m_msdDataCurve = plotMiniplot(m_msdPlot, m_msdDataCurve, workspace, 0);
    try
    {
      const std::pair<double, double> range = getCurveRange(m_msdDataCurve);    
      m_msdRange->setRange(range.first, range.second);
      // Replot
      m_msdPlot->replot();
    }
    catch(std::invalid_argument & exc)
    {
      showInformationBox(exc.what());
    }
  }
  else
  {
    showInformationBox("Selected input files are invalid.");
  }
}

void MSDFit::minChanged(double val)
{
  m_msdDblMng->setValue(m_msdProp["Start"], val);
}

void MSDFit::maxChanged(double val)
{
  m_msdDblMng->setValue(m_msdProp["End"], val);
}

void MSDFit::updateRS(QtProperty* prop, double val)
{
  if ( prop == m_msdProp["Start"] ) m_msdRange->setMinimum(val);
  else if ( prop == m_msdProp["End"] ) m_msdRange->setMaximum(val);
}

void Fury::run()
{
  QString filenames;
  switch ( uiForm().fury_cbInputType->currentIndex() )
  {
  case 0:
    filenames = uiForm().fury_iconFile->getFilenames().join("', r'");
    break;
  case 1:
    filenames = uiForm().fury_wsSample->currentText();
    break;
  }

  QString pyInput =
    "from IndirectDataAnalysis import fury\n"
    "samples = [r'" + filenames + "']\n"
    "resolution = r'" + uiForm().fury_resFile->getFirstFilename() + "'\n"
    "rebin = '" + m_furProp["ELow"]->valueText() +","+ m_furProp["EWidth"]->valueText() +","+m_furProp["EHigh"]->valueText()+"'\n";

  if ( uiForm().fury_ckVerbose->isChecked() ) pyInput += "verbose = True\n";
  else pyInput += "verbose = False\n";

  if ( uiForm().fury_ckPlot->isChecked() ) pyInput += "plot = True\n";
  else pyInput += "plot = False\n";

  if ( uiForm().fury_ckSave->isChecked() ) pyInput += "save = True\n";
  else pyInput += "save = False\n";

  pyInput +=
    "fury_ws = fury(samples, resolution, rebin, Save=save, Verbose=verbose, Plot=plot)\n";
  QString pyOutput = runPythonCode(pyInput).trimmed();
}

void Fury::resType(const QString& type)
{
  QStringList exts;
  if ( type == "RES File" )
  {
    exts.append("_res.nxs");
    m_furyResFileType = true;
  }
  else
  {
    exts.append("_red.nxs");
    m_furyResFileType = false;
  }
  uiForm().fury_resFile->setFileExtensions(exts);
}

void Fury::plotInput()
{
  std::string workspace;
  if ( uiForm().fury_cbInputType->currentIndex() == 0 )
  {
    if ( uiForm().fury_iconFile->isValid() )
    {
      QString filename = uiForm().fury_iconFile->getFirstFilename();
      QFileInfo fi(filename);
      QString wsname = fi.baseName();

      QString pyInput = "LoadNexus(r'" + filename + "', '" + wsname + "')\n";
      QString pyOutput = runPythonCode(pyInput);

      workspace = wsname.toStdString();
    }
    else
    {
      showInformationBox("Selected input files are invalid.");
      return;
    }
  }
  else if ( uiForm().fury_cbInputType->currentIndex() == 1 )
  {
    workspace = uiForm().fury_wsSample->currentText().toStdString();
    if ( workspace.empty() )
    {
      showInformationBox("No workspace selected.");
      return;
    }
  }

  m_furCurve = plotMiniplot(m_furPlot, m_furCurve, workspace, 0);
  try
  {
    const std::pair<double, double> range = getCurveRange(m_furCurve);    
    m_furRange->setRange(range.first, range.second);
    m_furPlot->replot();
  }
  catch(std::invalid_argument & exc)
  {
    showInformationBox(exc.what());
  }
}

void Fury::maxChanged(double val)
{
  m_furDblMng->setValue(m_furProp["EHigh"], val);
}

void Fury::minChanged(double val)
{
  m_furDblMng->setValue(m_furProp["ELow"], val);
}

void Fury::updateRS(QtProperty* prop, double val)
{
  if ( prop == m_furProp["ELow"] )
    m_furRange->setMinimum(val);
  else if ( prop == m_furProp["EHigh"] )
    m_furRange->setMaximum(val);
}

void FuryFit::run()
{
  // First create the function
  auto function = createFunction();

  uiForm().furyfit_ckPlotGuess->setChecked(false);
  
  const int fitType = uiForm().furyfit_cbFitType->currentIndex();

  if ( uiForm().furyfit_ckConstrainIntensities->isChecked() )
  {
    switch ( fitType )
    {
    case 0: // 1 Exp
    case 2: // 1 Str
      m_furyfitTies = "f1.Intensity = 1-f0.A0";
      break;
    case 1: // 2 Exp
    case 3: // 1 Exp & 1 Str
      m_furyfitTies = "f1.Intensity=1-f2.Intensity-f0.A0";
      break;
    default:
      break;
    }
  }
  QString ftype;
  switch ( fitType )
  {
  case 0:
    ftype = "1E_s"; break;
  case 1:
    ftype = "2E_s"; break;
  case 2:
    ftype = "1S_s"; break;
  case 3:
    ftype = "1E1S_s"; break;
  default:
    ftype = "s"; break;
  }

  plotInput();
  if ( m_ffInputWS == NULL )
  {
    return;
  }

  QString pyInput = "from IndirectCommon import getWSprefix\nprint getWSprefix('%1')\n";
  pyInput = pyInput.arg(QString::fromStdString(m_ffInputWSName));
  QString outputNm = runPythonCode(pyInput).trimmed();
  outputNm += QString("fury_") + ftype + uiForm().furyfit_leSpecNo->text();
  std::string output = outputNm.toStdString();

  // Create the Fit Algorithm
  Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("Fit");
  alg->initialize();
  alg->setPropertyValue("Function", function->asString());
  alg->setPropertyValue("InputWorkspace", m_ffInputWSName);
  alg->setProperty("WorkspaceIndex", uiForm().furyfit_leSpecNo->text().toInt());
  alg->setProperty("StartX", m_ffRangeManager->value(m_ffProp["StartX"]));
  alg->setProperty("EndX", m_ffRangeManager->value(m_ffProp["EndX"]));
  alg->setProperty("Ties", m_furyfitTies.toStdString());
  alg->setPropertyValue("Output", output);
  alg->execute();

  if ( ! alg->isExecuted() )
  {
    QString msg = "There was an error executing the fitting algorithm. Please see the "
      "Results Log pane for more details.";
    showInformationBox(msg);
    return;
  }

  // Now show the fitted curve of the mini plot
  m_ffFitCurve = plotMiniplot(m_ffPlot, m_ffFitCurve, output+"_Workspace", 1);
  QPen fitPen(Qt::red, Qt::SolidLine);
  m_ffFitCurve->setPen(fitPen);
  m_ffPlot->replot();

  // Get params.
  QMap<QString,double> parameters;
  std::vector<std::string> parNames = function->getParameterNames();
  std::vector<double> parVals;

  for( size_t i = 0; i < parNames.size(); ++i )
    parVals.push_back(function->getParameter(parNames[i]));

  for ( size_t i = 0; i < parNames.size(); ++i )
    parameters[QString(parNames[i].c_str())] = parVals[i];

  m_ffRangeManager->setValue(m_ffProp["BackgroundA0"], parameters["f0.A0"]);
  
  if ( fitType != 2 )
  {
    // Exp 1
    m_ffDblMng->setValue(m_ffProp["Exponential 1.Intensity"], parameters["f1.Intensity"]);
    m_ffDblMng->setValue(m_ffProp["Exponential 1.Tau"], parameters["f1.Tau"]);
    
    if ( fitType == 1 )
    {
      // Exp 2
      m_ffDblMng->setValue(m_ffProp["Exponential 2.Intensity"], parameters["f2.Intensity"]);
      m_ffDblMng->setValue(m_ffProp["Exponential 2.Tau"], parameters["f2.Tau"]);
    }
  }
  
  if ( fitType > 1 )
  {
    // Str
    QString fval;
    if ( fitType == 2 ) { fval = "f1."; }
    else { fval = "f2."; }
    
    m_ffDblMng->setValue(m_ffProp["Stretched Exponential.Intensity"], parameters[fval+"Intensity"]);
    m_ffDblMng->setValue(m_ffProp["Stretched Exponential.Tau"], parameters[fval+"Tau"]);
    m_ffDblMng->setValue(m_ffProp["Stretched Exponential.Beta"], parameters[fval+"Beta"]);
  }

  if ( uiForm().furyfit_ckPlotOutput->isChecked() )
  {
    QString pyInput = "from mantidplot import *\n"
      "plotSpectrum('" + QString::fromStdString(output) + "_Workspace', [0,1,2])\n";
    QString pyOutput = runPythonCode(pyInput);
  }
}

void FuryFit::typeSelection(int index)
{
  m_ffTree->clear();

  m_ffTree->addProperty(m_ffProp["StartX"]);
  m_ffTree->addProperty(m_ffProp["EndX"]);

  m_ffTree->addProperty(m_ffProp["LinearBackground"]);

  switch ( index )
  {
  case 0:
    m_ffTree->addProperty(m_ffProp["Exponential1"]);
    break;
  case 1:
    m_ffTree->addProperty(m_ffProp["Exponential1"]);
    m_ffTree->addProperty(m_ffProp["Exponential2"]);
    break;
  case 2:
    m_ffTree->addProperty(m_ffProp["StretchedExp"]);
    break;
  case 3:
    m_ffTree->addProperty(m_ffProp["Exponential1"]);
    m_ffTree->addProperty(m_ffProp["StretchedExp"]);
    break;
  }
}

void FuryFit::plotInput()
{
  std::string wsname;

  switch ( uiForm().furyfit_cbInputType->currentIndex() )
  {
  case 0: // "File"
    {
      if ( ! uiForm().furyfit_inputFile->isValid() )
      {
        return;
      }
      else
      {
      QFileInfo fi(uiForm().furyfit_inputFile->getFirstFilename());
      wsname = fi.baseName().toStdString();
      if ( (m_ffInputWS == NULL) || ( wsname != m_ffInputWSName ) )
      {
        std::string filename = uiForm().furyfit_inputFile->getFirstFilename().toStdString();
        // LoadNexus
        Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("LoadNexus");
        alg->initialize();
        alg->setPropertyValue("Filename", filename);
        alg->setPropertyValue("OutputWorkspace",wsname);
        alg->execute();
        // get the output workspace
        m_ffInputWS = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(wsname));
      }
      }
    }
    break;
  case 1: // Workspace
    {
      wsname = uiForm().furyfit_wsIqt->currentText().toStdString();
      try
      {
        m_ffInputWS = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(wsname));
      }
      catch ( Mantid::Kernel::Exception::NotFoundError & )
      {
        QString msg = "Workspace: '" + QString::fromStdString(wsname) + "' could not be "
          "found in the Analysis Data Service.";
        showInformationBox(msg);
        return;
      }
    }
    break;
  }
  m_ffInputWSName = wsname;

  int specNo = uiForm().furyfit_leSpecNo->text().toInt();

  m_ffDataCurve = plotMiniplot(m_ffPlot, m_ffDataCurve, m_ffInputWSName, specNo);
  try
  {
    const std::pair<double, double> range = getCurveRange(m_ffDataCurve);
    m_ffRangeS->setRange(range.first, range.second);
    m_ffRangeManager->setRange(m_ffProp["StartX"], range.first, range.second);
    m_ffRangeManager->setRange(m_ffProp["EndX"], range.first, range.second);
    
    m_ffPlot->setAxisScale(QwtPlot::xBottom, range.first, range.second);
    m_ffPlot->setAxisScale(QwtPlot::yLeft, 0.0, 1.0);
    m_ffPlot->replot();
  }
  catch(std::invalid_argument & exc)
  {
    showInformationBox(exc.what());
  }
}

void FuryFit::xMinSelected(double val)
{
  m_ffRangeManager->setValue(m_ffProp["StartX"], val);
}

void FuryFit::xMaxSelected(double val)
{
  m_ffRangeManager->setValue(m_ffProp["EndX"], val);
}

void FuryFit::backgroundSelected(double val)
{
  m_ffRangeManager->setValue(m_ffProp["BackgroundA0"], val);
}

void FuryFit::rangePropChanged(QtProperty* prop, double val)
{
  if ( prop == m_ffProp["StartX"] )
  {
    m_ffRangeS->setMinimum(val);
  }
  else if ( prop == m_ffProp["EndX"] )
  {
    m_ffRangeS->setMaximum(val);
  }
  else if ( prop == m_ffProp["BackgroundA0"] )
  {
    m_ffBackRangeS->setMinimum(val);
  }
}

void FuryFit::sequential()
{
  plotInput();
  if ( m_ffInputWS == NULL )
  {
    return;
  }

  Mantid::API::CompositeFunction_sptr func = createFunction();

  // Function Ties
  func->tie("f0.A1", "0");
  if ( uiForm().furyfit_ckConstrainIntensities->isChecked() )
  {
    switch ( uiForm().furyfit_cbFitType->currentIndex() )
    {
    case 0: // 1 Exp
    case 2: // 1 Str
      func->tie("f1.Intensity","1-f0.A0");
      break;
    case 1: // 2 Exp
    case 3: // 1 Exp & 1 Str
      func->tie("f1.Intensity","1-f2.Intensity-f0.A0");
      break;
    }
  }

  std::string function = std::string(func->asString());
  
  QString pyInput = "from IndirectDataAnalysis import furyfitSeq\n"
    "input = '" + QString::fromStdString(m_ffInputWSName) + "'\n"
    "func = r'" + QString::fromStdString(function) + "'\n"
    "startx = " + m_ffProp["StartX"]->valueText() + "\n"
    "endx = " + m_ffProp["EndX"]->valueText() + "\n"
    "plot = '" + uiForm().furyfit_cbPlotOutput->currentText() + "'\n"
    "save = ";
  pyInput += uiForm().furyfit_ckSaveSeq->isChecked() ? "True\n" : "False\n";
  pyInput += "furyfitSeq(input, func, startx, endx, save, plot)\n";
  
  QString pyOutput = runPythonCode(pyInput);
}

void FuryFit::plotGuess(QtProperty*)
{
  if ( ! uiForm().furyfit_ckPlotGuess->isChecked() || m_ffDataCurve == NULL )
  {
    return;
  }

  Mantid::API::CompositeFunction_sptr function = createFunction(true);

  // Create the double* array from the input workspace
  const size_t binIndxLow = m_ffInputWS->binIndexOf(m_ffRangeManager->value(m_ffProp["StartX"]));
  const size_t binIndxHigh = m_ffInputWS->binIndexOf(m_ffRangeManager->value(m_ffProp["EndX"]));
  const size_t nData = binIndxHigh - binIndxLow;

  std::vector<double> inputXData(nData);

  const Mantid::MantidVec& XValues = m_ffInputWS->readX(0);

  const bool isHistogram = m_ffInputWS->isHistogramData();

  for ( size_t i = 0; i < nData ; i++ )
  {
    if ( isHistogram )
      inputXData[i] = 0.5*(XValues[binIndxLow+i]+XValues[binIndxLow+i+1]);
    else
      inputXData[i] = XValues[binIndxLow+i];
  }

  Mantid::API::FunctionDomain1DVector domain(inputXData);
  Mantid::API::FunctionValues outputData(domain);
  function->function(domain, outputData);

  QVector<double> dataX;
  QVector<double> dataY;

  for ( size_t i = 0; i < nData; i++ )
  {
    dataX.append(inputXData[i]);
    dataY.append(outputData.getCalculated(i));
  }

  // Create the curve
  if ( m_ffFitCurve != NULL )
  {
    m_ffFitCurve->attach(0);
    delete m_ffFitCurve;
    m_ffFitCurve = 0;
  }

  m_ffFitCurve = new QwtPlotCurve();
  m_ffFitCurve->setData(dataX, dataY);
  m_ffFitCurve->attach(m_ffPlot);
  QPen fitPen(Qt::red, Qt::SolidLine);
  m_ffFitCurve->setPen(fitPen);
  m_ffPlot->replot();
}

void ConFit::run()
{
  plotInput();

  if ( m_cfDataCurve == NULL )
  {
    showInformationBox("There was an error reading the data file.");
    return;
  }

  uiForm().confit_ckPlotGuess->setChecked(false);

  Mantid::API::CompositeFunction_sptr function = createFunction();

  // get output name
  QString ftype = "";
  switch ( uiForm().confit_cbFitType->currentIndex() )
  {
  case 0:
    ftype += "Delta"; break;
  case 1:
    ftype += "1L"; break;
  case 2:
    ftype += "2L"; break;
  default:
    break;
  }
  switch ( uiForm().confit_cbBackground->currentIndex() )
  {
  case 0:
    ftype += "FixF_s"; break;
  case 1:
    ftype += "FitF_s"; break;
  case 2:
    ftype += "FitL_s"; break;
  }

  QString outputNm = runPythonCode(QString("from IndirectCommon import getWSprefix\nprint getWSprefix('") + QString::fromStdString(m_cfInputWSName) + QString("')\n")).trimmed();
  outputNm += QString("conv_") + ftype + uiForm().confit_leSpecNo->text();  
  std::string output = outputNm.toStdString();

  Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("Fit");
  alg->initialize();
  alg->setPropertyValue("Function", function->asString());
  alg->setPropertyValue("InputWorkspace", m_cfInputWSName);
  alg->setProperty<int>("WorkspaceIndex", uiForm().confit_leSpecNo->text().toInt());
  alg->setProperty<double>("StartX", m_cfDblMng->value(m_cfProp["StartX"]));
  alg->setProperty<double>("EndX", m_cfDblMng->value(m_cfProp["EndX"]));
  alg->setPropertyValue("Output", output);
  alg->execute();

  if ( ! alg->isExecuted() )
  {
    showInformationBox("Fit algorithm failed.");
    return;
  }

  // Plot the line on the mini plot
  m_cfCalcCurve = plotMiniplot(m_cfPlot, m_cfCalcCurve, output+"_Workspace", 1);
  QPen fitPen(Qt::red, Qt::SolidLine);
  m_cfCalcCurve->setPen(fitPen);
  m_cfPlot->replot();

  // Get params.
  QMap<QString,double> parameters;
  std::vector<std::string> parNames = function->getParameterNames();
  std::vector<double> parVals;

  for( size_t i = 0; i < parNames.size(); ++i )
    parVals.push_back(function->getParameter(parNames[i]));

  for ( size_t i = 0; i < parNames.size(); ++i )
    parameters[QString(parNames[i].c_str())] = parVals[i];

  // Populate Tree widget with values
  // Background should always be f0
  m_cfDblMng->setValue(m_cfProp["BGA0"], parameters["f0.A0"]);
  m_cfDblMng->setValue(m_cfProp["BGA1"], parameters["f0.A1"]);

  int noLorentz = uiForm().confit_cbFitType->currentIndex();

  int funcIndex = 1;
  QString prefBase = "f1.f";
  if ( noLorentz > 1 || ( noLorentz > 0 && m_cfBlnMng->value(m_cfProp["UseDeltaFunc"]) ) )
  {
    prefBase += "1.f";
    funcIndex--;
  }

  if ( m_cfBlnMng->value(m_cfProp["UseDeltaFunc"]) )
  {
    QString key = prefBase+QString::number(funcIndex)+".Height";
    m_cfDblMng->setValue(m_cfProp["DeltaHeight"], parameters[key]);
    funcIndex++;
  }

  if ( noLorentz > 0 )
  {
    // One Lorentz
    QString pref = prefBase + QString::number(funcIndex) + ".";
    m_cfDblMng->setValue(m_cfProp["Lorentzian 1.Height"], parameters[pref+"Height"]);
    m_cfDblMng->setValue(m_cfProp["Lorentzian 1.PeakCentre"], parameters[pref+"PeakCentre"]);
    m_cfDblMng->setValue(m_cfProp["Lorentzian 1.HWHM"], parameters[pref+"HWHM"]);
    funcIndex++;
  }

  if ( noLorentz > 1 )
  {
    // Two Lorentz
    QString pref = prefBase + QString::number(funcIndex) + ".";
    m_cfDblMng->setValue(m_cfProp["Lorentzian 2.Height"], parameters[pref+"Height"]);
    m_cfDblMng->setValue(m_cfProp["Lorentzian 2.PeakCentre"], parameters[pref+"PeakCentre"]);
    m_cfDblMng->setValue(m_cfProp["Lorentzian 2.HWHM"], parameters[pref+"HWHM"]);
  }

  // Plot Output
  if ( uiForm().confit_ckPlotOutput->isChecked() )
  {
    QString pyInput =
      "plotSpectrum('" + QString::fromStdString(output) + "_Workspace', [0,1,2])\n";
    QString pyOutput = runPythonCode(pyInput);
  }

}

void ConFit::typeSelection(int index)
{
  m_cfTree->removeProperty(m_cfProp["Lorentzian1"]);
  m_cfTree->removeProperty(m_cfProp["Lorentzian2"]);
  
  switch ( index )
  {
  case 0:
    m_cfHwhmRange->setVisible(false);
    break;
  case 1:
    m_cfTree->addProperty(m_cfProp["Lorentzian1"]);
    m_cfHwhmRange->setVisible(true);
    break;
  case 2:
    m_cfTree->addProperty(m_cfProp["Lorentzian1"]);
    m_cfTree->addProperty(m_cfProp["Lorentzian2"]);
    m_cfHwhmRange->setVisible(true);
    break;
  }    
}

void ConFit::bgTypeSelection(int index)
{
  if ( index == 2 )
  {
    m_cfProp["LinearBackground"]->addSubProperty(m_cfProp["BGA1"]);
  }
  else
  {
    m_cfProp["LinearBackground"]->removeSubProperty(m_cfProp["BGA1"]);
  }
}

void ConFit::plotInput()
{
  std::string wsname;
  const bool plotGuess = uiForm().confit_ckPlotGuess->isChecked();
  uiForm().confit_ckPlotGuess->setChecked(false);

  // Find wsname and set m_cfInputWS to point to that workspace.
  switch ( uiForm().confit_cbInputType->currentIndex() )
  {
  case 0: // "File"
    {
      if ( uiForm().confit_inputFile->isValid() )
      {
        QFileInfo fi(uiForm().confit_inputFile->getFirstFilename());
        wsname = fi.baseName().toStdString();

        // Load the file if it has not already been loaded.
        if ( (m_cfInputWS == NULL) || ( wsname != m_cfInputWSName )
          )
        {
          std::string filename = uiForm().confit_inputFile->getFirstFilename().toStdString();
          Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("LoadNexus");
          alg->initialize();
          alg->setPropertyValue("Filename", filename);
          alg->setPropertyValue("OutputWorkspace",wsname);
          alg->execute();
          m_cfInputWS = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(wsname));
        }
      }
      else
      {
        return;
      }
    }
    break;
  case 1: // Workspace
    {
      wsname = uiForm().confit_wsSample->currentText().toStdString();
      try
      {
        m_cfInputWS = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(wsname));
      }
      catch ( Mantid::Kernel::Exception::NotFoundError & )
      {
        QString msg = "Workspace: '" + QString::fromStdString(wsname) + "' could not be "
          "found in the Analysis Data Service.";
        showInformationBox(msg);
        return;
      }
    }
    break;
  }
  m_cfInputWSName = wsname;

  int specNo = uiForm().confit_leSpecNo->text().toInt();
  // Set spectra max value
  size_t specMax = m_cfInputWS->getNumberHistograms();
  if( specMax > 0 ) specMax -= 1;
  if ( specNo < 0 || static_cast<size_t>(specNo) > specMax ) //cast is okay as the first check is for less-than-zero
  {
    uiForm().confit_leSpecNo->setText("0");
    specNo = 0;
  }
  int smCurrent = uiForm().confit_leSpecMax->text().toInt();
  if ( smCurrent < 0 || static_cast<size_t>(smCurrent) > specMax )
  {
    uiForm().confit_leSpecMax->setText(QString::number(specMax));
  }

  m_cfDataCurve = plotMiniplot(m_cfPlot, m_cfDataCurve, wsname, specNo);
  try
  {
    const std::pair<double, double> range = getCurveRange(m_cfDataCurve);    
    m_cfRangeS->setRange(range.first, range.second);
    uiForm().confit_ckPlotGuess->setChecked(plotGuess);
  }
  catch(std::invalid_argument & exc)
  {
    showInformationBox(exc.what());
  }
}

void ConFit::plotGuess(QtProperty*)
{

  if ( ! uiForm().confit_ckPlotGuess->isChecked() || m_cfDataCurve == NULL )
  {
    return;
  }

  Mantid::API::CompositeFunction_sptr function = createFunction(true);

  if ( m_cfInputWS == NULL )
  {
    plotInput();
  }

  // std::string inputName = m_cfInputWS->getName();  // Unused

  const size_t binIndexLow = m_cfInputWS->binIndexOf(m_cfDblMng->value(m_cfProp["StartX"]));
  const size_t binIndexHigh = m_cfInputWS->binIndexOf(m_cfDblMng->value(m_cfProp["EndX"]));
  const size_t nData = binIndexHigh - binIndexLow;

  std::vector<double> inputXData(nData);
  //double* outputData = new double[nData];

  const Mantid::MantidVec& XValues = m_cfInputWS->readX(0);
  const bool isHistogram = m_cfInputWS->isHistogramData();

  for ( size_t i = 0; i < nData; i++ )
  {
    if ( isHistogram )
    {
      inputXData[i] = 0.5 * ( XValues[binIndexLow+i] + XValues[binIndexLow+i+1] );
    }
    else
    {
      inputXData[i] = XValues[binIndexLow+i];
    }
  }

  Mantid::API::FunctionDomain1DVector domain(inputXData);
  Mantid::API::FunctionValues outputData(domain);
  function->function(domain, outputData);

  QVector<double> dataX, dataY;

  for ( size_t i = 0; i < nData; i++ )
  {
    dataX.append(inputXData[i]);
    dataY.append(outputData.getCalculated(i));
  }

  if ( m_cfCalcCurve != NULL )
  {
    m_cfCalcCurve->attach(0);
    delete m_cfCalcCurve;
    m_cfCalcCurve = 0;
  }

  m_cfCalcCurve = new QwtPlotCurve();
  m_cfCalcCurve->setData(dataX, dataY);
  QPen fitPen(Qt::red, Qt::SolidLine);
  m_cfCalcCurve->setPen(fitPen);
  m_cfCalcCurve->attach(m_cfPlot);
  m_cfPlot->replot();
}

void ConFit::sequential()
{
  const QString error = validate();
  if( ! error.isEmpty() )
  {
    showInformationBox(error);
    return;
  }

  if ( m_cfInputWS == NULL )
  {
    return;
  }

  QString bg = uiForm().confit_cbBackground->currentText();
  if ( bg == "Fixed Flat" )
  {
    bg = "FixF";
  }
  else if ( bg == "Fit Flat" )
  {
    bg = "FitF";
  }
  else if ( bg == "Fit Linear" )
  {
    bg = "FitL";
  }

  Mantid::API::CompositeFunction_sptr func = createFunction();
  std::string function = std::string(func->asString());
  QString stX = m_cfProp["StartX"]->valueText();
  QString enX = m_cfProp["EndX"]->valueText();

  QString pyInput =
    "from IndirectDataAnalysis import confitSeq\n"
    "input = '" + QString::fromStdString(m_cfInputWSName) + "'\n"
    "func = r'" + QString::fromStdString(function) + "'\n"
    "startx = " + stX + "\n"
    "endx = " + enX + "\n"
    "specMin = " + uiForm().confit_leSpecNo->text() + "\n"
    "specMax = " + uiForm().confit_leSpecMax->text() + "\n"
    "plot = '" + uiForm().confit_cbPlotOutput->currentText() + "'\n"
    "save = ";
  
  pyInput += uiForm().confit_ckSaveSeq->isChecked() ? "True\n" : "False\n";
  
  pyInput +=    
    "bg = '" + bg + "'\n"
    "confitSeq(input, func, startx, endx, save, plot, bg, specMin, specMax)\n";

  QString pyOutput = runPythonCode(pyInput);
}

void ConFit::minChanged(double val)
{
  m_cfDblMng->setValue(m_cfProp["StartX"], val);
}

void ConFit::maxChanged(double val)
{
  m_cfDblMng->setValue(m_cfProp["EndX"], val);
}

void ConFit::hwhmChanged(double val)
{
  const double peakCentre = m_cfDblMng->value(m_cfProp["Lorentzian 1.PeakCentre"]);
  // Always want HWHM to display as positive.
  if ( val > peakCentre )
  {
    m_cfDblMng->setValue(m_cfProp["Lorentzian 1.HWHM"], val-peakCentre);
  }
  else
  {
    m_cfDblMng->setValue(m_cfProp["Lorentzian 1.HWHM"], peakCentre-val);
  }
}

void ConFit::backgLevel(double val)
{
  m_cfDblMng->setValue(m_cfProp["BGA0"], val);
}

void ConFit::updateRS(QtProperty* prop, double val)
{
  if ( prop == m_cfProp["StartX"] ) { m_cfRangeS->setMinimum(val); }
  else if ( prop == m_cfProp["EndX"] ) { m_cfRangeS->setMaximum(val); }
  else if ( prop == m_cfProp["BGA0"] ) { m_cfBackgS->setMinimum(val); }
  else if ( prop == m_cfProp["Lorentzian 1.HWHM"] ) { hwhmUpdateRS(val); }
}

void ConFit::hwhmUpdateRS(double val)
{
  const double peakCentre = m_cfDblMng->value(m_cfProp["Lorentzian 1.PeakCentre"]);
  m_cfHwhmRange->setMinimum(peakCentre-val);
  m_cfHwhmRange->setMaximum(peakCentre+val);
}

void ConFit::checkBoxUpdate(QtProperty* prop, bool checked)
{
  // Add/remove some properties to display only relevant options
  if ( prop == m_cfProp["UseDeltaFunc"] )
  {
    if ( checked ) { m_cfProp["DeltaFunction"]->addSubProperty(m_cfProp["DeltaHeight"]); }
    else { m_cfProp["DeltaFunction"]->removeSubProperty(m_cfProp["DeltaHeight"]); }
  }
}

void IndirectDataAnalysis::openDirectoryDialog()
{
  MantidQt::API::ManageUserDirectories *ad = new MantidQt::API::ManageUserDirectories(this);
  ad->show();
  ad->setFocus();
}

void IndirectDataAnalysis::help()
{
  QString tabName = m_uiForm.tabWidget->tabText(m_uiForm.tabWidget->currentIndex());
  QString url = "http://www.mantidproject.org/IDA";
  if ( tabName == "Initial Settings" )
    url += "";
  else if ( tabName == "Elwin" )
    url += ":Elwin";
  else if ( tabName == "MSD Fit" )
    url += ":MSDFit";
  else if ( tabName == "Fury" )
    url += ":Fury";
  else if ( tabName == "FuryFit" )
    url += ":FuryFit";
  else if ( tabName == "ConvFit" )
    url += ":ConvFit";
  else if ( tabName == "Calculate Corrections" )
    url += ":CalcCor";
  else if ( tabName == "Apply Corrections" )
    url += ":AbsCor";
  QDesktopServices::openUrl(QUrl(url));
}

void AbsorptionF2Py::run()
{
  QString pyInput = "import IndirectAbsCor\n";
  
  QString geom;
  QString size;

  if ( uiForm().absp_cbShape->currentText() == "Flat" )
  {
    geom = "flt";
    if ( uiForm().absp_ckUseCan->isChecked() ) 
    {
      size = "[" + uiForm().absp_lets->text() + ", " +
      uiForm().absp_letc1->text() + ", " +
      uiForm().absp_letc2->text() + "]";
    }
    else
    {
      size = "[" + uiForm().absp_lets->text() + ", 0.0, 0.0]";
    }
  }
  else if ( uiForm().absp_cbShape->currentText() == "Cylinder" )
  {
    geom = "cyl";

    // R3 only populated when using can. R4 is fixed to 0.0
    if ( uiForm().absp_ckUseCan->isChecked() ) 
    {
      size = "[" + uiForm().absp_ler1->text() + ", " +
        uiForm().absp_ler2->text() + ", " +
        uiForm().absp_ler3->text() + ", 0.0 ]";
    }
    else
    {
      size = "[" + uiForm().absp_ler1->text() + ", " +
        uiForm().absp_ler2->text() + ", 0.0, 0.0 ]";
    }
    
  }

  QString width = uiForm().absp_lewidth->text();

  if ( uiForm().absp_cbInputType->currentText() == "File" )
  {
    QString input = uiForm().absp_inputFile->getFirstFilename();
    if ( input == "" ) { return; }
    pyInput +=
    "import os.path as op\n"
    "file = r'" + input + "'\n"
    "( dir, filename ) = op.split(file)\n"
    "( name, ext ) = op.splitext(filename)\n"
    "LoadNexusProcessed(file, name)\n"
    "inputws = name\n";
  }
  else
  {
    pyInput += "inputws = '" + uiForm().absp_wsInput->currentText() + "'\n";
  }
  
  if ( uiForm().absp_ckUseCan->isChecked() )
  {
    pyInput +=
      "ncan = 2\n"
      "density = [" + uiForm().absp_lesamden->text() + ", " + uiForm().absp_lecanden->text() + ", " + uiForm().absp_lecanden->text() + "]\n"
      "sigs = [" + uiForm().absp_lesamsigs->text() + "," + uiForm().absp_lecansigs->text() + "," + uiForm().absp_lecansigs->text() + "]\n"
      "siga = [" + uiForm().absp_lesamsiga->text() + "," + uiForm().absp_lecansiga->text() + "," + uiForm().absp_lecansiga->text() + "]\n";
  }
  else
  {
    pyInput +=
      "ncan = 1\n"
      "density = [" + uiForm().absp_lesamden->text() + ", 0.0, 0.0 ]\n"
      "sigs = [" + uiForm().absp_lesamsigs->text() + ", 0.0, 0.0]\n"
      "siga = [" + uiForm().absp_lesamsiga->text() + ", 0.0, 0.0]\n";
  }

  pyInput +=
    "geom = '" + geom + "'\n"
    "beam = [3.0, 0.5*" + width + ", -0.5*" + width + ", 2.0, -2.0, 0.0, 3.0, 0.0, 3.0]\n"
    "size = " + size + "\n"
    "avar = " + uiForm().absp_leavar->text() + "\n"
    "plotOpt = '" + uiForm().absp_cbPlotOutput->currentText() + "'\n"
    "IndirectAbsCor.AbsRunFeeder(inputws, geom, beam, ncan, size, density, sigs, siga, avar, plotOpt=plotOpt)\n";

  QString pyOutput = runPythonCode(pyInput).trimmed();
}

void AbsorptionF2Py::shape(int index)
{
  uiForm().absp_swShapeDetails->setCurrentIndex(index);
  // Meaning of the "avar" variable changes depending on shape selection
  if ( index == 0 ) { uiForm().absp_lbAvar->setText("Can Angle to Beam"); }
  else if ( index == 1 ) { uiForm().absp_lbAvar->setText("Step Size"); }
}

void AbsorptionF2Py::useCanChecked(bool checked)
{
  // Disable thickness fields/labels/asterisks.
  uiForm().absp_lbtc1->setEnabled(checked);
  uiForm().absp_lbtc2->setEnabled(checked);
  uiForm().absp_letc1->setEnabled(checked);
  uiForm().absp_letc2->setEnabled(checked);
  uiForm().absp_valtc1->setVisible(checked);
  uiForm().absp_valtc2->setVisible(checked);

  // Disable R3 field/label/asterisk.
  uiForm().absp_lbR3->setEnabled(checked);
  uiForm().absp_ler3->setEnabled(checked);
  uiForm().absp_valR3->setVisible(checked);

  // Disable "Can Details" group and asterisks.
  uiForm().absp_gbCan->setEnabled(checked);
  uiForm().absp_valCanden->setVisible(checked);
  uiForm().absp_valCansigs->setVisible(checked);
  uiForm().absp_valCansiga->setVisible(checked);
  
  // Workaround for "disabling" title of the QGroupBox.
  QPalette palette;
  if(checked)
    palette.setColor(
      QPalette::Disabled, 
      QPalette::WindowText,
      QApplication::palette().color(QPalette::Disabled, QPalette::WindowText));
  else
    palette.setColor(
      QPalette::Active, 
      QPalette::WindowText,
      QApplication::palette().color(QPalette::Active, QPalette::WindowText));

  uiForm().absp_gbCan->setPalette(palette);
}

void AbsorptionF2Py::tcSync()
{
  if ( uiForm().absp_letc2->text() == "" )
  {
    QString val = uiForm().absp_letc1->text();
    uiForm().absp_letc2->setText(val);
  }
}

void AbsCor::run()
{
  QString geom = uiForm().abscor_cbGeometry->currentText();
  if ( geom == "Flat" )
  {
    geom = "flt";
  }
  else if ( geom == "Cylinder" )
  {
    geom = "cyl";
  }

  QString pyInput = "from IndirectDataAnalysis import abscorFeeder, loadNexus\n";

  if ( uiForm().abscor_cbSampleInputType->currentText() == "File" )
  {
    pyInput +=
      "sample = loadNexus(r'" + uiForm().abscor_sample->getFirstFilename() + "')\n";
  }
  else
  {
    pyInput +=
      "sample = '" + uiForm().abscor_wsSample->currentText() + "'\n";
  }

  if ( uiForm().abscor_ckUseCan->isChecked() )
  {
    if ( uiForm().abscor_cbContainerInputType->currentText() == "File" )
    {
      pyInput +=
        "container = loadNexus(r'" + uiForm().abscor_can->getFirstFilename() + "')\n";
    }
    else
    {
      pyInput +=
        "container = '" + uiForm().abscor_wsContainer->currentText() + "'\n";
    }
  }
  else
  {
    pyInput += "container = ''\n";
  }

  pyInput += "geom = '" + geom + "'\n";


  if ( uiForm().abscor_ckUseCorrections->isChecked() )
  {
    pyInput += "useCor = True\n";
  }
  else
  {
    pyInput += "useCor = False\n";
  }

  pyInput += "abscorFeeder(sample, container, geom, useCor)\n";
  QString pyOutput = runPythonCode(pyInput).trimmed();
}

void IDATab::showInformationBox(const QString & message)
{
  m_parent->showInformationBox(message);
}

QString IDATab::runPythonCode(const QString & code, bool no_output)
{
  return m_parent->runPythonCode(code, no_output);
}



} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
