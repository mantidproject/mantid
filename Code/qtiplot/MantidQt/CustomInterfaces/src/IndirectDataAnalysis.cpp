//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/IndirectDataAnalysis.h"
#include "MantidQtAPI/ManageUserDirectories.h"
#include "MantidQtMantidWidgets/RangeSelector.h"

#include "MantidKernel/ConfigService.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"

#include <QLineEdit>
#include <QValidator>
#include <QFileInfo>
#include <QUrl>
#include <QDesktopServices>

#include "qttreepropertybrowser.h"
#include "qtpropertymanager.h"
#include "qteditorfactory.h"

#include <Poco/NObserver.h>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
namespace CustomInterfaces
{

DECLARE_SUBWINDOW(IndirectDataAnalysis);

using namespace MantidQt::CustomInterfaces;

//----------------------
// Public member functions
//----------------------

IndirectDataAnalysis::IndirectDataAnalysis(QWidget *parent) :
UserSubWindow(parent), m_nDec(6), m_valInt(NULL), m_valDbl(NULL), m_furyResFileType(true), m_ffDataCurve(NULL), m_ffFitCurve(NULL),
  m_changeObserver(*this, &IndirectDataAnalysis::handleDirectoryChange),
  m_elwPlot(NULL), m_elwR1(NULL), m_elwR2(NULL), m_elwDataCurve(NULL),
  m_msdPlot(NULL), m_msdRange(NULL), m_msdDataCurve(NULL), m_msdTree(NULL), m_msdDblMng(NULL),
  m_cfDataCurve(NULL), m_cfCalcCurve(NULL)
{
}

void IndirectDataAnalysis::closeEvent(QCloseEvent*)
{
  Mantid::Kernel::ConfigService::Instance().removeObserver(m_changeObserver);
}

void IndirectDataAnalysis::handleDirectoryChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf)
{
  std::string key = pNf->key();
  std::string preValue = pNf->preValue();
  std::string curValue = pNf->curValue();

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

  // create validators
  m_valInt = new QIntValidator(this);
  m_valDbl = new QDoubleValidator(this);
  // Create Editor Factories
  m_dblEdFac = new DoubleEditorFactory();
  m_strEdFac = new StringDialogEditorFactory();
  m_blnEdFac = new QtCheckBoxFactory();

  setupElwin();
  setupMsd();
  // setupFury();
  setupFuryFit();
  setupConFit();
  // setupAbsorption();

  connect(m_uiForm.pbManageDirs, SIGNAL(clicked()), this, SLOT(openDirectoryDialog()));
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(help()));

  // Main "Run" event
  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(run()));

  // msd
  connect(m_uiForm.msd_pbPlotInput, SIGNAL(clicked()), this, SLOT(msdPlotInput()));
  // fury
  connect(m_uiForm.fury_cbInputType, SIGNAL(currentIndexChanged(int)), this, SLOT(furyInputType(int)));
  connect(m_uiForm.fury_pbRefresh, SIGNAL(clicked()), this, SLOT(refreshWSlist()));
  connect(m_uiForm.fury_cbResType, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(furyResType(const QString&)));
  connect(m_uiForm.fury_pbPlotInput, SIGNAL(clicked()), this, SLOT(furyPlotInput()));
  // absorption
  connect(m_uiForm.abs_cbShape, SIGNAL(activated(int)), this, SLOT(absorptionShape(int)));

  // apply validators - fury
  m_uiForm.fury_leELow->setValidator(m_valDbl);
  m_uiForm.fury_leEWidth->setValidator(m_valDbl);
  m_uiForm.fury_leEHigh->setValidator(m_valDbl);
  // apply validators - absorption
  m_uiForm.abs_leAttenuation->setValidator(m_valDbl);
  m_uiForm.abs_leScatter->setValidator(m_valDbl);
  m_uiForm.abs_leDensity->setValidator(m_valDbl);
  m_uiForm.abs_leFlatHeight->setValidator(m_valDbl);
  m_uiForm.abs_leWidth->setValidator(m_valDbl);
  m_uiForm.abs_leThickness->setValidator(m_valDbl);
  m_uiForm.abs_leElementSize->setValidator(m_valDbl);
  m_uiForm.abs_leCylHeight->setValidator(m_valDbl);
  m_uiForm.abs_leRadius->setValidator(m_valDbl);
  m_uiForm.abs_leSlices->setValidator(m_valInt);
  m_uiForm.abs_leAnnuli->setValidator(m_valInt);
  // apply validators - convolution fit
  m_uiForm.confit_leSpecNo->setValidator(m_valInt);

  refreshWSlist();
}

void IndirectDataAnalysis::initLocalPython()
{
  loadSettings();
}

void IndirectDataAnalysis::loadSettings()
{
  QSettings settings;
  QString settingsGroup = "CustomInterfaces/IndirectAnalysis/";
  QString saveDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory"));

  settings.beginGroup(settingsGroup + "ProcessedFiles");
  settings.setValue("last_directory", saveDir);
  m_uiForm.elwin_inputFile->readSettings(settings.group());
  m_uiForm.msd_inputFile->readSettings(settings.group());
  m_uiForm.fury_iconFile->readSettings(settings.group());
  m_uiForm.fury_resFile->readSettings(settings.group());
  m_uiForm.furyfit_inputFile->readSettings(settings.group());
  m_uiForm.confit_inputFile->readSettings(settings.group());
  m_uiForm.abs_inputFile->readSettings(settings.group());
  settings.endGroup();
}

void IndirectDataAnalysis::setupElwin()
{
  // Create QtTreePropertyBrowser object
  m_elwTree = new QtTreePropertyBrowser();
  m_uiForm.elwin_properties->addWidget(m_elwTree);

  // Create Manager Objects
  m_elwDblMng = new QtDoublePropertyManager();
  m_elwBlnMng = new QtBoolPropertyManager();
  m_elwGrpMng = new QtGroupPropertyManager();

  // Editor Factories
  m_elwTree->setFactoryForManager(m_elwDblMng, m_dblEdFac);
  m_elwTree->setFactoryForManager(m_elwBlnMng, m_blnEdFac);

  // Create Properties
  m_elwProp["R1S"] = m_elwDblMng->addProperty("Start");
  m_elwDblMng->setDecimals(m_elwProp["R1S"], m_nDec);
  m_elwProp["R1E"] = m_elwDblMng->addProperty("End");
  m_elwDblMng->setDecimals(m_elwProp["R1E"], m_nDec);  
  m_elwProp["R2S"] = m_elwDblMng->addProperty("Start");
  m_elwDblMng->setDecimals(m_elwProp["R2S"], m_nDec);
  m_elwProp["R2E"] = m_elwDblMng->addProperty("End");
  m_elwDblMng->setDecimals(m_elwProp["R2E"], m_nDec);

  m_elwProp["UseTwoRanges"] = m_elwBlnMng->addProperty("Use Two Ranges and Subtract");

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
  m_uiForm.elwin_plot->addWidget(m_elwPlot);
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
  elwinTwoRanges(0, false);

  // m_uiForm element signals and slots
  connect(m_uiForm.elwin_pbPlotInput, SIGNAL(clicked()), this, SLOT(elwinPlotInput()));

  // Set any default values
  m_elwDblMng->setValue(m_elwProp["R1S"], -0.02);
  m_elwDblMng->setValue(m_elwProp["R1E"], 0.02);
}

void IndirectDataAnalysis::setupMsd()
{
  // Tree Browser
  m_msdTree = new QtTreePropertyBrowser();
  m_uiForm.msd_properties->addWidget(m_msdTree);

  m_msdDblMng = new QtDoublePropertyManager();

  m_msdTree->setFactoryForManager(m_msdDblMng, m_dblEdFac);

  m_msdProp["Start"] = m_msdDblMng->addProperty("StartX");
  m_msdDblMng->setDecimals(m_msdProp["Start"], m_nDec);
  m_msdProp["End"] = m_msdDblMng->addProperty("EndX");
  m_msdDblMng->setDecimals(m_msdProp["End"], m_nDec);

  m_msdTree->addProperty(m_msdProp["Start"]);
  m_msdTree->addProperty(m_msdProp["End"]);

  m_msdPlot = new QwtPlot(this);
  m_uiForm.msd_plot->addWidget(m_msdPlot);

  // Cosmetics
  m_msdPlot->setAxisFont(QwtPlot::xBottom, this->font());
  m_msdPlot->setAxisFont(QwtPlot::yLeft, this->font());
  m_msdPlot->setCanvasBackground(Qt::white);

  m_msdRange = new MantidWidgets::RangeSelector(m_msdPlot);

  connect(m_msdRange, SIGNAL(minValueChanged(double)), this, SLOT(msdMinChanged(double)));
  connect(m_msdRange, SIGNAL(maxValueChanged(double)), this, SLOT(msdMaxChanged(double)));
  connect(m_msdDblMng, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(msdUpdateRS(QtProperty*, double)));
}

void IndirectDataAnalysis::setupFuryFit()
{
  m_ffTree = new QtTreePropertyBrowser();
  m_uiForm.furyfit_properties->addWidget(m_ffTree);
  
  // Setup FuryFit Plot Window
  m_ffPlot = new QwtPlot(this);
  m_ffPlot->setAxisFont(QwtPlot::xBottom, this->font());
  m_ffPlot->setAxisFont(QwtPlot::yLeft, this->font());
  m_uiForm.furyfit_vlPlot->addWidget(m_ffPlot);
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
  m_doubleManager = new QtDoublePropertyManager();
  m_ffRangeManager = new QtDoublePropertyManager();

  m_ffTree->setFactoryForManager(m_doubleManager, m_dblEdFac);
  m_ffTree->setFactoryForManager(m_ffRangeManager, m_dblEdFac);

  m_ffProp["StartX"] = m_ffRangeManager->addProperty("StartX");
  m_ffRangeManager->setDecimals(m_ffProp["StartX"], 10);
  m_ffProp["EndX"] = m_ffRangeManager->addProperty("EndX");
  m_ffRangeManager->setDecimals(m_ffProp["EndX"], 10);

  connect(m_ffRangeManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(furyfitRangePropChanged(QtProperty*, double)));

  m_ffProp["LinearBackground"] = m_groupManager->addProperty("LinearBackground");
  QtProperty* bgA0 = m_ffRangeManager->addProperty("A0");
  QtProperty* bgA1 = m_doubleManager->addProperty("A1");
  m_ffRangeManager->setDecimals(bgA0, 10);
  m_doubleManager->setDecimals(bgA1, 1);
  m_doubleManager->setRange(bgA1, 0.0, 0.0);
  m_ffProp["LinearBackground"]->addSubProperty(bgA0);
  m_ffProp["LinearBackground"]->addSubProperty(bgA1);
  m_ffProp["BackgroundA0"] = bgA0;

  m_ffProp["Exponential1"] = createExponential();
  m_ffProp["Exponential2"] = createExponential();
  
  m_ffProp["StretchedExp"] = createStretchedExp();

  furyfitTypeSelection(m_uiForm.furyfit_cbFitType->currentIndex());

  // Connect to PlotGuess checkbox
  connect(m_doubleManager, SIGNAL(propertyChanged(QtProperty*)), this, SLOT(furyfitPlotGuess(QtProperty*)));

  // Signal/slot ui connections
  connect(m_uiForm.furyfit_cbFitType, SIGNAL(currentIndexChanged(int)), this, SLOT(furyfitTypeSelection(int)));
  connect(m_uiForm.furyfit_pbPlotInput, SIGNAL(clicked()), this, SLOT(furyfitPlotInput()));
  connect(m_uiForm.furyfit_leSpecNo, SIGNAL(editingFinished()), this, SLOT(furyfitPlotInput()));
  connect(m_uiForm.furyfit_cbInputType, SIGNAL(currentIndexChanged(int)), this, SLOT(furyfitInputType(int)));
  connect(m_uiForm.furyfit_pbRefreshWSList, SIGNAL(clicked()), this, SLOT(refreshWSlist()));
  connect(m_uiForm.furyfit_pbPlotOutput, SIGNAL(clicked()), this, SLOT(furyfitPlotOutput()));
  connect(m_uiForm.furyfit_pbSeqFit, SIGNAL(clicked()), this, SLOT(furyfitSequential()));
  // apply validators - furyfit
  m_uiForm.furyfit_leSpecNo->setValidator(m_valInt);
}

void IndirectDataAnalysis::setupConFit()
{
  // Create Property Managers
  m_cfGrpMng = new QtGroupPropertyManager();
  m_cfBlnMng = new QtBoolPropertyManager();
  m_cfStrMng = new QtStringPropertyManager();
  m_cfDblMng = new QtDoublePropertyManager();

  // Create TreeProperty Widget
  m_cfTree = new QtTreePropertyBrowser();
  m_uiForm.confit_properties->addWidget(m_cfTree);

  // add factories to managers
  m_cfTree->setFactoryForManager(m_cfBlnMng, m_blnEdFac);
  m_cfTree->setFactoryForManager(m_cfDblMng, m_dblEdFac);
  m_cfTree->setFactoryForManager(m_cfStrMng, m_strEdFac);

  // Create Plot Widget
  m_cfPlot = new QwtPlot(this);
  m_cfPlot->setAxisFont(QwtPlot::xBottom, this->font());
  m_cfPlot->setAxisFont(QwtPlot::yLeft, this->font());
  m_cfPlot->setCanvasBackground(Qt::white);
  m_uiForm.confit_plot->addWidget(m_cfPlot);

  // Create Range Selectors
  m_cfRangeS = new MantidQt::MantidWidgets::RangeSelector(m_cfPlot);
  m_cfBackgS = new MantidQt::MantidWidgets::RangeSelector(m_cfPlot, 
    MantidQt::MantidWidgets::RangeSelector::YSINGLE);
  m_cfBackgS->setColour(Qt::darkGreen);
  m_cfBackgS->setRange(0.0, 1.0);

  // Populate Property Widget

  m_cfProp["FitRange"] = m_cfGrpMng->addProperty("Fitting Range");
  m_cfProp["StartX"] = m_cfDblMng->addProperty("StartX");
  m_cfDblMng->setDecimals(m_cfProp["StartX"], m_nDec);
  m_cfProp["EndX"] = m_cfDblMng->addProperty("EndX");
  m_cfDblMng->setDecimals(m_cfProp["EndX"], m_nDec);
  m_cfProp["FitRange"]->addSubProperty(m_cfProp["StartX"]);
  m_cfProp["FitRange"]->addSubProperty(m_cfProp["EndX"]);
  m_cfTree->addProperty(m_cfProp["FitRange"]);

  m_cfProp["LinearBackground"] = m_cfGrpMng->addProperty("Background");
  m_cfProp["BGA0"] = m_cfDblMng->addProperty("A0");
  m_cfProp["BGConstant"] = m_cfBlnMng->addProperty("Constant");
  m_cfProp["BGA1"] = m_cfDblMng->addProperty("A1");
  m_cfProp["LinearBackground"]->addSubProperty(m_cfProp["BGA0"]);
  m_cfProp["LinearBackground"]->addSubProperty(m_cfProp["BGConstant"]);
  m_cfProp["LinearBackground"]->addSubProperty(m_cfProp["BGA1"]);
  m_cfTree->addProperty(m_cfProp["LinearBackground"]);

  // Delta Function
  m_cfProp["DeltaFunction"] = m_cfGrpMng->addProperty("Delta Function");
  m_cfProp["UseDeltaFunc"] = m_cfBlnMng->addProperty("Use");
  m_cfProp["DeltaHeight"] = m_cfDblMng->addProperty("Height");
  m_cfDblMng->setDecimals(m_cfProp["DeltaHeight"], m_nDec);
  m_cfProp["DeltaFunction"]->addSubProperty(m_cfProp["UseDeltaFunc"]);
  // m_cfProp["DeltaFunction"]->addSubProperty(m_cfProp["DeltaHeight"]); // < Not by default
  m_cfTree->addProperty(m_cfProp["DeltaFunction"]);

  // Resolution Function
  m_cfProp["ResolutionFunction"] = m_cfGrpMng->addProperty("Resolution Function");
  m_cfProp["ResFuncFile"] = m_cfStrMng->addProperty("File");
  m_cfProp["ResolutionFunction"]->addSubProperty(m_cfProp["ResFuncFile"]);
  m_cfTree->addProperty(m_cfProp["ResolutionFunction"]);

  m_cfProp["Lorentzian1"] = createLorentzian("Lorentzian 1");
  m_cfProp["Lorentzian2"] = createLorentzian("Lorentzian 2");

  // Connections
  connect(m_cfRangeS, SIGNAL(minValueChanged(double)), this, SLOT(confitMinChanged(double)));
  connect(m_cfRangeS, SIGNAL(maxValueChanged(double)), this, SLOT(confitMaxChanged(double)));
  connect(m_cfBackgS, SIGNAL(minValueChanged(double)), this, SLOT(confitBackgLevel(double)));
  connect(m_cfDblMng, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(confitUpdateRS(QtProperty*, double)));
  connect(m_cfBlnMng, SIGNAL(valueChanged(QtProperty*, bool)), this, SLOT(confitCheckBoxUpdate(QtProperty*, bool)));

  connect(m_cfDblMng, SIGNAL(propertyChanged(QtProperty*)), this, SLOT(confitPlotGuess(QtProperty*)));

  confitTypeSelection(m_uiForm.confit_cbFitType->currentIndex());

  // Replot input automatically when file / spec no changes
  connect(m_uiForm.confit_leSpecNo, SIGNAL(editingFinished()), this, SLOT(confitPlotInput()));
  connect(m_uiForm.confit_inputFile, SIGNAL(fileEditingFinished()), this, SLOT(confitPlotInput()));
  // other signal/slot connections
  connect(m_uiForm.confit_pbRefresh, SIGNAL(clicked()), this, SLOT(refreshWSlist()));
  connect(m_uiForm.confit_cbInputType, SIGNAL(currentIndexChanged(int)), this, SLOT(confitInputType(int)));
  connect(m_uiForm.confit_cbFitType, SIGNAL(currentIndexChanged(int)), this, SLOT(confitTypeSelection(int)));
  connect(m_uiForm.confit_pbPlotInput, SIGNAL(clicked()), this, SLOT(confitPlotInput()));
}

bool IndirectDataAnalysis::validateElwin()
{
  bool valid = true;

  if ( ! m_uiForm.elwin_inputFile->isValid() )
  {
    valid = false;
  }

  return valid;
}

bool IndirectDataAnalysis::validateMsd()
{
  bool valid = true;

  if ( ! m_uiForm.msd_inputFile->isValid() )
  {
    valid = false;
  }

  return valid;
}

bool IndirectDataAnalysis::validateFury()
{
  bool valid = true;

  switch ( m_uiForm.fury_cbInputType->currentIndex() )
  {
  case 0:
    {
      if ( ! m_uiForm.fury_iconFile->isValid() )
      {
        valid = false;
      }
    }
    break;
  case 1:
    {
      if ( m_uiForm.fury_cbWorkspace->currentText() == "" )
      {
        valid = false;
      }
    }
    break;
  }

  if ( ! m_uiForm.fury_resFile->isValid()  )
  {
    valid = false;
  }

  if ( m_uiForm.fury_leELow->text() == "" )
  {
    m_uiForm.fury_valELow->setText("*");
    valid = false;
  }
  else
  {
    m_uiForm.fury_valELow->setText(" ");
  }
  if ( m_uiForm.fury_leEWidth->text() == "" )
  {
    m_uiForm.fury_valEWidth->setText("*");
    valid = false;
  }
  else
  {
    m_uiForm.fury_valEWidth->setText(" ");
  }
  if ( m_uiForm.fury_leEHigh->text() == "" )
  {
    m_uiForm.fury_valEHigh->setText("*");
    valid = false;
  }
  else
  {
    m_uiForm.fury_valEHigh->setText(" ");
  }

  return valid;
}

bool IndirectDataAnalysis::validateAbsorption()
{
  bool valid = true;

  if ( ! m_uiForm.abs_inputFile->isValid() )
  {
    valid = false;
  }

  if ( m_uiForm.abs_leAttenuation->text() == "" )
  {
    m_uiForm.abs_valAttenuation->setText("*");
    valid = false;
  }
  else
  {
    m_uiForm.abs_valAttenuation->setText(" ");
  }

  if ( m_uiForm.abs_leScatter->text() == "" )
  {
    m_uiForm.abs_valScatter->setText("*");
    valid = false;
  }
  else
  {
    m_uiForm.abs_valScatter->setText(" ");
  }

  if ( m_uiForm.abs_leDensity->text() == "" )
  {
    m_uiForm.abs_valDensity->setText("*");
    valid = false;
  }
  else
  {
    m_uiForm.abs_valDensity->setText(" ");
  }

  if ( m_uiForm.abs_cbShape->currentText() == "Flat Plate" )
  {
    // ... FLAT PLATE
    if ( m_uiForm.abs_leFlatHeight->text() == "" )
    {
      m_uiForm.abs_valFlatHeight->setText("*");
      valid = false;
    }
    else
    {
      m_uiForm.abs_valFlatHeight->setText(" ");
    }

    if ( m_uiForm.abs_leWidth->text() == "" )
    {
      m_uiForm.abs_valWidth->setText("*");
      valid = false;
    }
    else
    {
      m_uiForm.abs_valWidth->setText(" ");
    }

    if ( m_uiForm.abs_leThickness->text() == "" )
    {
      m_uiForm.abs_valThickness->setText("*");
      valid = false;
    }
    else
    {
      m_uiForm.abs_valThickness->setText(" ");
    }

    if ( m_uiForm.abs_leElementSize->text() == "" )
    {
      m_uiForm.abs_valElementSize->setText("*");
      valid = false;
    }
    else
    {
      m_uiForm.abs_valElementSize->setText(" ");
    }
  }
  else
  {
    // ... CYLINDER
    if ( m_uiForm.abs_leCylHeight->text() == "" )
    {
      m_uiForm.abs_valCylHeight->setText("*");
      valid = false;
    }
    else
    {
      m_uiForm.abs_valCylHeight->setText(" ");
    }

    if ( m_uiForm.abs_leRadius->text() == "" )
    {
      m_uiForm.abs_valRadius->setText("*");
      valid = false;
    }
    else
    {
      m_uiForm.abs_valRadius->setText(" ");
    }

    if ( m_uiForm.abs_leSlices->text() == "" )
    {
      m_uiForm.abs_valSlices->setText("*");
      valid = false;
    }
    else
    {
      m_uiForm.abs_valSlices->setText(" ");
    }

    if ( m_uiForm.abs_leAnnuli->text() == "" )
    {
      m_uiForm.abs_valAnnuli->setText("*");
      valid = false;
    }
    else
    {
      m_uiForm.abs_valAnnuli->setText(" ");
    }
  }

  return valid;
}

Mantid::API::CompositeFunction* IndirectDataAnalysis::createFunction(QtTreePropertyBrowser* propertyBrowser)
{
  Mantid::API::CompositeFunction* result = new Mantid::API::CompositeFunction();

  QList<QtBrowserItem*> items = propertyBrowser->topLevelItems();
  m_furyfitConstraints = "";
  int funcIndex = 0;

  for ( int i = 0; i < items.size(); i++ )
  {
    QtProperty* item = items[i]->property(); 
    QList<QtProperty*> sub = item->subProperties();

    if ( sub.size() > 0 )
    {
      Mantid::API::IFunction* func;
      std::string name = item->propertyName().toStdString();
      if ( name == "Stretched Exponential" )
      {
        // create user function
        func = Mantid::API::FunctionFactory::Instance().createFunction("UserFunction");
        // set the necessary properties
        std::string formula = "Intensity*exp(-Exponent*(x^Beta))";
        Mantid::API::IFunction::Attribute att(formula);
        func->setAttribute("Formula", att);
        if ( m_furyfitConstraints != "" ) m_furyfitConstraints += ",";
        m_furyfitConstraints += "0 <= f%1.Beta <= 1";
        m_furyfitConstraints = m_furyfitConstraints.arg(funcIndex);
      }
      else if ( name == "Exponential" )
      {
        // create user function
        func = Mantid::API::FunctionFactory::Instance().createFunction("UserFunction");
        // set the necessary properties
        std::string formula = "Intensity*exp(-(x*Exponent))";
        Mantid::API::IFunction::Attribute att(formula);
        func->setAttribute("Formula", att);
        m_furyfitConstraints = m_furyfitConstraints.arg(funcIndex);
      }
      else
      {
        func = Mantid::API::FunctionFactory::Instance().createFunction(name);
      }
      for ( int j = 0; j < sub.size(); j++ ) // set initial parameter values
      {
        func->setParameter(sub[j]->propertyName().toStdString(), sub[j]->valueText().toDouble());
      }
      result->addFunction(func);

      funcIndex++;
    }
  }
  return result;
}

QtProperty* IndirectDataAnalysis::createLorentzian(QString name)
{
  QtProperty* lorentzGroup = m_cfGrpMng->addProperty(name);
  m_cfProp[name+".Height"] = m_cfDblMng->addProperty("Height");
  // m_cfDblMng->setRange(m_cfProp[name+".Height"], 0.0, 1.0); // 0 < Height < 1
  m_cfProp[name+".PeakCentre"] = m_cfDblMng->addProperty("PeakCentre");
  m_cfProp[name+".HWHM"] = m_cfDblMng->addProperty("HWHM");
  m_cfDblMng->setDecimals(m_cfProp[name+".Height"], m_nDec);
  m_cfDblMng->setDecimals(m_cfProp[name+".PeakCentre"], m_nDec);
  m_cfDblMng->setDecimals(m_cfProp[name+".HWHM"], m_nDec);
  lorentzGroup->addSubProperty(m_cfProp[name+".Height"]);
  lorentzGroup->addSubProperty(m_cfProp[name+".PeakCentre"]);
  lorentzGroup->addSubProperty(m_cfProp[name+".HWHM"]);
  return lorentzGroup;
}

QtProperty* IndirectDataAnalysis::createExponential()
{
  QtProperty* expGroup = m_groupManager->addProperty("Exponential");
  QtProperty* expA0 = m_doubleManager->addProperty("Intensity");
  m_doubleManager->setRange(expA0, 0.0, 1.0); // 0 < Height < 1
  m_doubleManager->setDecimals(expA0, m_nDec);
  QtProperty* expA1 = m_doubleManager->addProperty("Exponent");
  m_doubleManager->setDecimals(expA1, m_nDec);
  expGroup->addSubProperty(expA0);
  expGroup->addSubProperty(expA1);
  return expGroup;
}

QtProperty* IndirectDataAnalysis::createStretchedExp()
{
  QtProperty* prop = m_groupManager->addProperty("Stretched Exponential");
  QtProperty* stA0 = m_doubleManager->addProperty("Intensity");
  m_doubleManager->setRange(stA0, 0.0, 1.0);  // 0 < Height < 1
  QtProperty* stA1 = m_doubleManager->addProperty("Exponent");
  QtProperty* stA2 = m_doubleManager->addProperty("Beta");
  m_doubleManager->setDecimals(stA0, m_nDec);
  m_doubleManager->setDecimals(stA1, m_nDec);
  m_doubleManager->setDecimals(stA2, m_nDec);
  m_doubleManager->setRange(stA2, 0.0, 1.0);
  prop->addSubProperty(stA0);
  prop->addSubProperty(stA1);
  prop->addSubProperty(stA2);
  return prop;
}

void IndirectDataAnalysis::refreshWSlist()
{
  // Get object list from ADS
  std::set<std::string> workspaceList = Mantid::API::AnalysisDataService::Instance().getObjectNames();
  // Clear Workspace Lists
  m_uiForm.fury_cbWorkspace->clear();
  m_uiForm.furyfit_cbWorkspace->clear();
  m_uiForm.confit_cbWorkspace->clear();
  
  if ( ! workspaceList.empty() )
  {
    std::set<std::string>::const_iterator it;
    for ( it=workspaceList.begin(); it != workspaceList.end(); ++it )
    {
      Mantid::API::Workspace_sptr workspace = boost::dynamic_pointer_cast<Mantid::API::Workspace>(Mantid::API::AnalysisDataService::Instance().retrieve(*it));
      
      if ( workspace->id() != "TableWorkspace" )
      {
      QString ws = QString::fromStdString(*it);
      m_uiForm.fury_cbWorkspace->addItem(ws);
      m_uiForm.furyfit_cbWorkspace->addItem(ws);
      m_uiForm.confit_cbWorkspace->addItem(ws);
      }
    }
  }
}

void IndirectDataAnalysis::run()
{
  QString tabName = m_uiForm.tabWidget->tabText(m_uiForm.tabWidget->currentIndex());

  if ( tabName == "Elwin" )  { elwinRun(); }
  else if ( tabName == "MSD Fit" ) { msdRun(); }
  else if ( tabName == "Fury" ) { furyRun(); }
  else if ( tabName == "FuryFit" ) { furyfitRun(); }
  else if ( tabName == "ConvFit" ) { confitRun(); }
  else if ( tabName == "Absorption" ) { absorptionRun(); }
  else { showInformationBox("This tab does not have a 'Run' action."); }
}

void IndirectDataAnalysis::elwinRun()
{
  if ( ! validateElwin() )
  {
    showInformationBox("Please check your input.");
    return;
  }

  QString pyInput =
    "from IndirectDataAnalysis import elwin\n"
    "input = [r'" + m_uiForm.elwin_inputFile->getFilenames().join("', r'") + "']\n"
    "eRange = [ " + QString::number(m_elwDblMng->value(m_elwProp["R1S"])) +","+ QString::number(m_elwDblMng->value(m_elwProp["R1E"]));

  if ( m_elwBlnMng->value(m_elwProp["UseTwoRanges"]) )
  {
    pyInput += ", " + QString::number(m_elwDblMng->value(m_elwProp["R2S"])) + ", " + QString::number(m_elwDblMng->value(m_elwProp["R2E"]));
  }

  pyInput+= "]\n";

  if ( m_uiForm.elwin_ckVerbose->isChecked() ) pyInput += "verbose = True\n";
  else pyInput += "verbose = False\n";

  if ( m_uiForm.elwin_ckPlot->isChecked() ) pyInput += "plot = True\n";
  else pyInput += "plot = False\n";

  if ( m_uiForm.elwin_ckSave->isChecked() ) pyInput += "save = True\n";
  else pyInput += "save = False\n";

  pyInput +=
    "eq1_ws, eq2_ws = elwin(input, eRange, Save=save, Verbose=verbose, Plot=plot)\n";

  if ( m_uiForm.elwin_ckConcat->isChecked() )
  {
    pyInput += "from IndirectDataAnalysis import concatWSs\n"
      "concatWSs(eq1_ws, 'MomentumTransfer', 'ElwinQResults')\n"
      "concatWSs(eq2_ws, 'QSquared', 'ElwinQSqResults')\n";
  }

  QString pyOutput = runPythonCode(pyInput).trimmed();

}

void IndirectDataAnalysis::elwinPlotInput()
{
  if ( m_uiForm.elwin_inputFile->isValid() )
  {
    QString filename = m_uiForm.elwin_inputFile->getFirstFilename();
    QFileInfo fi(filename);
    QString wsname = fi.baseName();

    QString pyInput = "LoadNexus(r'" + filename + "', '" + wsname + "')\n";
    QString pyOutput = runPythonCode(pyInput);

    std::string workspace = wsname.toStdString();

    m_elwDataCurve = plotMiniplot(m_elwPlot, m_elwDataCurve, workspace, 0);
    
    int npts = m_elwDataCurve->data().size();
    double lower = m_elwDataCurve->data().x(0);
    double upper = m_elwDataCurve->data().x(npts-1);
    
    m_elwR1->setRange(lower, upper);

    // Replot
    m_elwPlot->replot();
  }
  else
  {
    showInformationBox("Selected input files are invalid.");
  }
}

void IndirectDataAnalysis::elwinTwoRanges(QtProperty*, bool val)
{
  m_elwR2->setVisible(val);
}

void IndirectDataAnalysis::elwinMinChanged(double val)
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

void IndirectDataAnalysis::elwinMaxChanged(double val)
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

void IndirectDataAnalysis::elwinUpdateRS(QtProperty* prop, double val)
{
  if ( prop == m_elwProp["R1S"] ) m_elwR1->setMinimum(val);
  else if ( prop == m_elwProp["R1E"] ) m_elwR1->setMaximum(val);
  else if ( prop == m_elwProp["R2S"] ) m_elwR2->setMinimum(val);
  else if ( prop == m_elwProp["R2E"] ) m_elwR2->setMaximum(val);
}

void IndirectDataAnalysis::msdRun()
{
  if ( ! validateMsd() )
  {
    showInformationBox("Please check your input.");
    return;
  }

  QString pyInput =
    "from IndirectDataAnalysis import msdfit\n"
    "startX = " + QString::number(m_msdDblMng->value(m_msdProp["Start"])) +"\n"
    "endX = " + QString::number(m_msdDblMng->value(m_msdProp["End"])) +"\n"
    "inputs = [r'" + m_uiForm.msd_inputFile->getFilenames().join("', r'") + "']\n";

  if ( m_uiForm.msd_ckVerbose->isChecked() ) pyInput += "verbose = True\n";
  else pyInput += "verbose = False\n";

  if ( m_uiForm.msd_ckPlot->isChecked() ) pyInput += "plot = True\n";
  else pyInput += "plot = False\n";

  if ( m_uiForm.msd_ckSave->isChecked() ) pyInput += "save = True\n";
  else pyInput += "save = False\n";

  pyInput +=
    "msdfit(inputs, startX, endX, Save=save, Verbose=verbose, Plot=plot)\n";

  QString pyOutput = runPythonCode(pyInput).trimmed();


}

void IndirectDataAnalysis::msdPlotInput()
{
  if ( m_uiForm.msd_inputFile->isValid() )
  {
    QString filename = m_uiForm.msd_inputFile->getFirstFilename();
    QFileInfo fi(filename);
    QString wsname = fi.baseName();

    QString pyInput = "LoadNexus(r'" + filename + "', '" + wsname + "')\n";
    QString pyOutput = runPythonCode(pyInput);

    std::string workspace = wsname.toStdString();

    m_msdDataCurve = plotMiniplot(m_msdPlot, m_msdDataCurve, workspace, 0);
    int npnts = m_msdDataCurve->data().size();
    double lower = m_msdDataCurve->data().x(0);
    double upper = m_msdDataCurve->data().x(npnts-1);

    m_msdRange->setRange(lower, upper);

    // Replot
    m_msdPlot->replot();
  }
  else
  {
    showInformationBox("Selected input files are invalid.");
  }
}

void IndirectDataAnalysis::msdMinChanged(double val)
{
  m_msdDblMng->setValue(m_msdProp["Start"], val);
}

void IndirectDataAnalysis::msdMaxChanged(double val)
{
  m_msdDblMng->setValue(m_msdProp["End"], val);
}

void IndirectDataAnalysis::msdUpdateRS(QtProperty* prop, double val)
{
  if ( prop == m_msdProp["Start"] ) m_msdRange->setMinimum(val);
  else if ( prop == m_msdProp["End"] ) m_msdRange->setMaximum(val);
}

void IndirectDataAnalysis::furyRun()
{
  if ( !validateFury() )
  {
    showInformationBox("Please check your input.");
    return;
  }

  QString filenames;
  switch ( m_uiForm.fury_cbInputType->currentIndex() )
  {
  case 0:
    filenames = m_uiForm.fury_iconFile->getFilenames().join("', r'");
    break;
  case 1:
    filenames = m_uiForm.fury_cbWorkspace->currentText();
    break;
  }

  QString pyInput =
    "from IndirectDataAnalysis import fury\n"
    "samples = [r'" + filenames + "']\n"
    "resolution = r'" + m_uiForm.fury_resFile->getFirstFilename() + "'\n"
    "rebin = '" + m_uiForm.fury_leELow->text()+","+m_uiForm.fury_leEWidth->text()+","+ m_uiForm.fury_leEHigh->text()+"'\n";

  if ( m_uiForm.fury_ckVerbose->isChecked() ) pyInput += "verbose = True\n";
  else pyInput += "verbose = False\n";

  if ( m_uiForm.fury_ckPlot->isChecked() ) pyInput += "plot = True\n";
  else pyInput += "plot = False\n";

  if ( m_uiForm.fury_ckSave->isChecked() ) pyInput += "save = True\n";
  else pyInput += "save = False\n";

  pyInput +=
    "fury_ws = fury(samples, resolution, rebin, Save=save, Verbose=verbose, Plot=plot)\n";
  QString pyOutput = runPythonCode(pyInput).trimmed();
}

void IndirectDataAnalysis::furyInputType(int index)
{
  m_uiForm.fury_swInput->setCurrentIndex(index);
  refreshWSlist();
}

void IndirectDataAnalysis::furyResType(const QString& type)
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
  m_uiForm.fury_resFile->setFileExtensions(exts);
}

void IndirectDataAnalysis::furyPlotInput()
{
  if ( m_uiForm.fury_iconFile->isValid() )
  {
    QString pyInput = "from IndirectDataAnalysis import plotInput\n"
      "inputfiles = [r'" + m_uiForm.fury_iconFile->getFilenames().join("', r'") + "']\n"
      "spec = [0]\n"
      "plotInput(inputfiles, spectra=spec)\n";
    QString pyOutput = runPythonCode(pyInput).trimmed();
  }
}

/* FURY FIT TAB */
void IndirectDataAnalysis::furyfitRun()
{
    // First create the function
    Mantid::API::CompositeFunction* function = createFunction(m_ffTree);

    // uncheck "plot guess"
    m_uiForm.furyfit_ckPlotGuess->setChecked(false);

    // Background level
    m_furyfitTies = "f0.A1 = 0";
    
    if ( m_uiForm.furyfit_ckConstrainIntensities->isChecked() )
    {
      switch ( m_uiForm.furyfit_cbFitType->currentIndex() )
      {
      case 0: // 1 Exp
      case 2: // 1 Str
        m_furyfitTies += ", f1.Intensity = 1-f0.A0";
        break;
      case 1: // 2 Exp
      case 3: // 1 Exp & 1 Str
        m_furyfitTies += ",f1.Intensity=1-f2.Intensity-f0.A0";
        break;
      default:
        break;
      }
    }

    // the plotInput function handles loading the workspace, no need to duplicate that code here
    furyfitPlotInput();
    // however if it doesn't a workspace we don't want to continue, so...
    if ( m_ffInputWS == NULL )
    {
      return;
    }
    
    std::string output = m_ffInputWSName + "_fit_s" + m_uiForm.furyfit_leSpecNo->text().toStdString();
    // Create the Fit Algorithm
    Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("Fit");
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", m_ffInputWSName);
    alg->setProperty("WorkspaceIndex", m_uiForm.furyfit_leSpecNo->text().toInt());
    alg->setProperty("StartX", m_ffRangeManager->value(m_ffProp["StartX"]));
    alg->setProperty("EndX", m_ffRangeManager->value(m_ffProp["EndX"]));
    alg->setProperty("Ties", m_furyfitTies.toStdString());
    alg->setProperty("Constraints", m_furyfitConstraints.toStdString());
    alg->setPropertyValue("Function", *function);
    alg->setPropertyValue("Output",output);
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

    /// Get the "*_Parameters" TableWorkspace created by the Fit function (@todo change this to use the more succint parameters?)
    Mantid::API::ITableWorkspace_sptr table = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(output+"_Parameters"));
    std::map<std::string,double> params;
    int nRow = table->rowCount();
    for ( int i = 0; i < nRow; i++ )
    {
      Mantid::API::TableRow row = table->getRow(i);
      std::string key = "";
      double value = 0.0;
      row >> key >> value;
      params[key] = value;
    }

    // Background is in all functions
    m_ffRangeManager->setValue(m_ffProp["BackgroundA0"], params["f0.A0"]);

    QMap<QString,QtProperty*> subprops;
    QtProperty* exp;
    QList<QtProperty*> subs;

    switch ( m_uiForm.furyfit_cbFitType->currentIndex() )
    {
    case 0:
    case 1:
    case 3:
      {
      exp = m_ffProp["Exponential1"];
      subs = exp->subProperties();
      for ( int i = 0; i < subs.size(); i++ )
      {
        subprops[subs[i]->propertyName()] = subs[i];
      }
      m_doubleManager->setValue(subprops["Intensity"], params["f1.Intensity"]);
      m_doubleManager->setValue(subprops["Exponent"], params["f1.Exponent"]);
      break;
      }
    case 2:
      {
      exp = m_ffProp["StretchedExp"];
      subs = exp->subProperties();
      for ( int i = 0; i < subs.size(); i++ )
      {
        subprops[subs[i]->propertyName()] = subs[i];
      }
      m_doubleManager->setValue(subprops["Intensity"], params["f1.Intensity"]);
      m_doubleManager->setValue(subprops["Exponent"], params["f1.Exponent"]);
      m_doubleManager->setValue(subprops["Beta"], params["f1.Beta"]);
      break;
      }
    }

    switch ( m_uiForm.furyfit_cbFitType->currentIndex() )
    {
    case 1: // 2 Exp
      {
      exp = m_ffProp["Exponential2"];
      subs = exp->subProperties();
      for ( int i = 0; i < subs.size(); i++ )
      {
        subprops[subs[i]->propertyName()] = subs[i];
      }
      m_doubleManager->setValue(subprops["Intensity"], params["f2.Intensity"]);
      m_doubleManager->setValue(subprops["Exponent"], params["f2.Exponent"]);
      break;
      }
    case 3: // 1 Exp & 1 Stretched Exp
      {
      exp = m_ffProp["StretchedExp"];
      subs = exp->subProperties();
      for ( int i = 0; i < subs.size(); i++ )
      {
        subprops[subs[i]->propertyName()] = subs[i];
      }
      m_doubleManager->setValue(subprops["Intensity"], params["f2.Intensity"]);
      m_doubleManager->setValue(subprops["Exponent"], params["f2.Exponent"]);
      m_doubleManager->setValue(subprops["Beta"], params["f2.Beta"]);
      break;
      }
    case 0:
      break;
    case 2:
      break;
    }
}
/** ...  */
void IndirectDataAnalysis::furyfitTypeSelection(int index)
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
/** ...  */
void IndirectDataAnalysis::furyfitPlotInput()
{
  std::string wsname;

  switch ( m_uiForm.furyfit_cbInputType->currentIndex() )
  {
  case 0: // "File"
    {
      QFileInfo fi(m_uiForm.furyfit_inputFile->getFirstFilename());
      wsname = fi.baseName().toStdString();
      if ( (m_ffInputWS == NULL) || ( wsname != m_ffInputWSName ) )
      {
        std::string filename = m_uiForm.furyfit_inputFile->getFirstFilename().toStdString();
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
    break;
  case 1: // Workspace
    {
      wsname = m_uiForm.furyfit_cbWorkspace->currentText().toStdString();
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

  int specNo = m_uiForm.furyfit_leSpecNo->text().toInt();

  m_ffDataCurve = plotMiniplot(m_ffPlot, m_ffDataCurve, m_ffInputWSName, specNo);

  int nopnts =  m_ffDataCurve->data().size();
  double lower = m_ffDataCurve->data().x(0);
  double upper = m_ffDataCurve->data().x(nopnts-1);

  m_ffRangeS->setRange(lower, upper);
  m_ffRangeManager->setRange(m_ffProp["StartX"], lower, upper);
  m_ffRangeManager->setRange(m_ffProp["EndX"], lower, upper);

  m_ffPlot->setAxisScale(QwtPlot::xBottom, lower, upper);
  m_ffPlot->setAxisScale(QwtPlot::yLeft, 0.0, 1.0);
  m_ffPlot->replot();
}
/** ...  */
void IndirectDataAnalysis::furyfitXMinSelected(double val)
{
  m_ffRangeManager->setValue(m_ffProp["StartX"], val);
}
/** ...  */
void IndirectDataAnalysis::furyfitXMaxSelected(double val)
{
  m_ffRangeManager->setValue(m_ffProp["EndX"], val);
}
/** ...  */
void IndirectDataAnalysis::furyfitBackgroundSelected(double val)
{
  m_ffRangeManager->setValue(m_ffProp["BackgroundA0"], val);
}
/** ...  */
void IndirectDataAnalysis::furyfitRangePropChanged(QtProperty* prop, double val)
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
/** ...  */
void IndirectDataAnalysis::furyfitInputType(int index)
{
  m_uiForm.furyfit_swInput->setCurrentIndex(index);
}
/** ...  */
void IndirectDataAnalysis::furyfitPlotOutput()
{
  if ( m_ffOutputWS == NULL )
  {
    showInformationBox("No output found for FuryFit");
    return;
  }

  std::string name = m_ffOutputWS->getName();

  QString pyInput = "from mantidplot import *\n"
    "plotSpectrum('" + QString::fromStdString(name) + "', [0,1,2])\n";
  QString pyOutput = runPythonCode(pyInput);
}
/** ...  */
void IndirectDataAnalysis::furyfitSequential()
{
  furyfitPlotInput();
  if ( m_ffInputWS == NULL )
  {
    return;
  }

  Mantid::API::CompositeFunction* func = createFunction(m_ffTree);

  // Function Ties
  func->tie("f0.A1", "0");
  if ( m_uiForm.furyfit_ckConstrainIntensities->isChecked() )
  {
    switch ( m_uiForm.furyfit_cbFitType->currentIndex() )
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

  std::string function = std::string(*func);
  
  QString stX = QString::number(m_ffRangeManager->value(m_ffProp["StartX"]), 'g', 10);
  QString enX = QString::number(m_ffRangeManager->value(m_ffProp["EndX"]), 'g', 10);

  QString pyInput = "from IndirectDataAnalysis import furyfitSeq\n"
    "input = '" + QString::fromStdString(m_ffInputWSName) + "'\n"
    "func = r'" + QString::fromStdString(function) + "'\n"
    "startx = " + stX + "\n"
    "endx = " + enX + "\n"
    "furyfitSeq(input, func, startx, endx)\n";
  
  QString pyOutput = runPythonCode(pyInput);

}
/** ...  */
void IndirectDataAnalysis::furyfitPlotGuess(QtProperty*)
{
  if ( ! m_uiForm.furyfit_ckPlotGuess->isChecked() )
  {
    return;
  }

  Mantid::API::CompositeFunction* function = new Mantid::API::CompositeFunction();
  QList<QtProperty*> fitItems;
  int funcIndex = 1;
  bool singleF = true;

  switch ( m_uiForm.furyfit_cbFitType->currentIndex() )
  {
  case 0: // 1 Exponential
    fitItems.append(m_ffProp["Exponential1"]);
    break;
  case 1: // 2 Exponentials
    fitItems.append(m_ffProp["Exponential1"]);
    fitItems.append(m_ffProp["Exponential2"]);
    singleF = false;
    break;
  case 2: // 1 Stretched Exponential
    fitItems.append(m_ffProp["StretchedExp"]);
    break;
  case 3: // 1 Exponential with 1 Stretched Exponential
    fitItems.append(m_ffProp["Exponential1"]);
    fitItems.append(m_ffProp["StretchedExp"]);
    singleF = false;
    break;
  default:
    return;
    break;
  }

  // Add in background
  Mantid::API::IFunction* background = Mantid::API::FunctionFactory::Instance().createFunction("LinearBackground");
  function->addFunction(background);
  function->tie("f0.A1", "0");
  function->tie("f0.A0", m_ffProp["BackgroundA0"]->valueText().toStdString());

  for ( int i = 0; i < fitItems.size(); i++ )
  {
    QList<QtProperty*> fitProps = fitItems[i]->subProperties();
    if ( fitProps.size() > 0 )
    {
      Mantid::API::IFunction* func;
      // Both Exp and StrExp are Userfunctions
      func = Mantid::API::FunctionFactory::Instance().createFunction("UserFunction");
      std::string funcName = fitItems[i]->propertyName().toStdString();
      std::string formula;
      if ( funcName == "Exponential" )
      {
        formula = "Intensity*exp(-(x*Exponent))";
      }
      else if ( funcName == "Stretched Exponential" )
      {
        formula = "Intensity*exp(-Exponent*(x^Beta))";
      }
      // Create subfunction object with specified formula
      Mantid::API::IFunction::Attribute att(formula);
      func->setAttribute("Formula", att);
      function->addFunction(func);
      // Create ties
      for ( int j = 0; j < fitProps.size(); j++ )
      {
        std::string parName;
        parName += QString("f%1.").arg(funcIndex).toStdString() + fitProps[j]->propertyName().toStdString();
        function->tie(parName, fitProps[j]->valueText().toStdString());
      }
      funcIndex++;
    }
  }
  // Run the fit routine
  if ( m_ffInputWS == NULL )
  {
    furyfitPlotInput();
  }

  std::string inputName = m_ffInputWS->getName();
  
  // Create the double* array from the input workspace
  int binIndxLow = m_ffInputWS->binIndexOf(m_ffRangeManager->value(m_ffProp["StartX"]));
  int binIndxHigh = m_ffInputWS->binIndexOf(m_ffRangeManager->value(m_ffProp["EndX"]));
  const int nData = binIndxHigh - binIndxLow;

  double* inputXData = new double[nData];
  double* outputData = new double[nData];

  const Mantid::MantidVec& XValues = m_ffInputWS->readX(0);

  const bool isHistogram = m_ffInputWS->isHistogramData();

  for ( int i = 0; i < nData ; i++ )
  {
    if ( isHistogram )
      inputXData[i] = 0.5*(XValues[binIndxLow+i]+XValues[binIndxLow+i+1]);
    else
      inputXData[i] = XValues[binIndxLow+i];
  }

  function->applyTies();
  function->function(outputData, inputXData, nData);

  // get output data into a q vector for qwt
  QVector<double> dataX;
  QVector<double> dataY;

  for ( int i = 0; i < nData; i++ )
  {
    dataX.append(inputXData[i]);
    dataY.append(outputData[i]);
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

// CONVOLUTION FIT
void IndirectDataAnalysis::confitRun()
{
  confitPlotInput();

  if ( m_cfDataCurve == NULL )
  {
    showInformationBox("Input invalid");
    return;
  }

  m_uiForm.confit_ckPlotGuess->setChecked(false);

  Mantid::API::CompositeFunction* function = confitCreateFunction();
  std::string output = m_cfInputWSName + "_convfit_s" + m_uiForm.confit_leSpecNo->text().toStdString();

  Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("Fit");
  alg->initialize();
  alg->setPropertyValue("InputWorkspace", m_cfInputWSName);
  alg->setProperty<int>("WorkspaceIndex", m_uiForm.confit_leSpecNo->text().toInt());
  alg->setProperty<double>("StartX", m_cfDblMng->value(m_cfProp["StartX"]));
  alg->setProperty<double>("EndX", m_cfDblMng->value(m_cfProp["EndX"]));
  alg->setPropertyValue("Function", *function);
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

  // Update parameter values (possibly easier from algorithm properties)
  QMap<QString,double> parameters;
  QStringList parNames = QString::fromStdString(alg->getPropertyValue("ParameterNames")).split(",", QString::SkipEmptyParts);
  QStringList parVals = QString::fromStdString(alg->getPropertyValue("Parameters")).split(",", QString::SkipEmptyParts);
  for ( int i = 0; i < parNames.size(); i++ )
  {
    parameters[parNames[i]] = parVals[i].toDouble();
  }

  // Populate Tree widget with values

  // Background should always be f0
  m_cfDblMng->setValue(m_cfProp["BGA0"], parameters["f0.A0"]);
  m_cfDblMng->setValue(m_cfProp["BGA1"], parameters["f0.A1"]);

  int funcIndex = 1;

  if ( m_cfBlnMng->value(m_cfProp["UseDeltaFunc"]) )
  {
    m_cfDblMng->setValue(m_cfProp["DeltaHeight"], parameters["f1.Height"]);
    funcIndex++;
  }

  // Increment for Resolution
  funcIndex++;

  if ( m_uiForm.confit_cbFitType->currentIndex() > 0 )
  {
    // One Lorentz
    QString pref = "f" + QString::number(funcIndex) + ".";
    m_cfDblMng->setValue(m_cfProp["Lorentzian 1.Height"], parameters[pref+"Height"]);
    m_cfDblMng->setValue(m_cfProp["Lorentzian 1.PeakCentre"], parameters[pref+"PeakCentre"]);
    m_cfDblMng->setValue(m_cfProp["Lorentzian 1.HWHM"], parameters[pref+"HWHM"]);
    funcIndex++;
  }

  if ( m_uiForm.confit_cbFitType->currentIndex() == 2 )
  {
    // Two Lorentz
    QString pref = "f" + QString::number(funcIndex) + ".";
    m_cfDblMng->setValue(m_cfProp["Lorentzian 2.Height"], parameters[pref+"Height"]);
    m_cfDblMng->setValue(m_cfProp["Lorentzian 2.PeakCentre"], parameters[pref+"PeakCentre"]);
    m_cfDblMng->setValue(m_cfProp["Lorentzian 2.HWHM"], parameters[pref+"HWHM"]);
  }

}

void IndirectDataAnalysis::confitTypeSelection(int index)
{
  m_cfTree->removeProperty(m_cfProp["Lorentzian1"]);
  m_cfTree->removeProperty(m_cfProp["Lorentzian2"]);
  
  switch ( index )
  {
  case 0:
    break;
  case 1:
    m_cfTree->addProperty(m_cfProp["Lorentzian1"]);
    break;
  case 2:
    m_cfTree->addProperty(m_cfProp["Lorentzian1"]);
    m_cfTree->addProperty(m_cfProp["Lorentzian2"]);
    break;
  }    
}

Mantid::API::CompositeFunction* IndirectDataAnalysis::confitCreateFunction(bool tie)
{
  // Mantid::API::CompositeFunction* conv = Mantid::API::FunctionFactory::Instance().createFunction("Convolution");
  Mantid::API::CompositeFunction* result = new Mantid::API::CompositeFunction();
  int index = 0;

  Mantid::API::IFunction* func;

  // Background
  func = Mantid::API::FunctionFactory::Instance().createFunction("LinearBackground");
  index = result->addFunction(func);
  if ( tie ) { result->tie("f0.A0", m_cfProp["BGA0"]->valueText().toStdString() ); }
  else { func->setParameter("A0", m_cfProp["BGA0"]->valueText().toDouble()); }
  if ( m_cfBlnMng->value(m_cfProp["BGConstant"]) )
  {
    result->tie("f0.A1", "0.0");
  }
  else
  {
    if ( tie ) { result->tie("f0.A1", m_cfProp["BGA1"]->valueText().toStdString() ); }
    else { func->setParameter("A1", m_cfProp["BGA1"]->valueText().toDouble()); }
  }

  // Delta Function
  if ( m_cfBlnMng->value(m_cfProp["UseDeltaFunc"]) )
  {
    func = Mantid::API::FunctionFactory::Instance().createFunction("DeltaFunction");
    index = result->addFunction(func);
    if ( tie ) { result->tie("f1.Height", m_cfProp["DeltaHeight"]->valueText().toStdString() ); }
    else { func->setParameter("Height", m_cfProp["DeltaHeight"]->valueText().toDouble()); }
  }

  // Resolution
  func = Mantid::API::FunctionFactory::Instance().createFunction("Resolution");
  index = result->addFunction(func);
  Mantid::API::IFunction::Attribute attr(m_cfProp["ResFuncFile"]->valueText().toStdString());
  func->setAttribute("FileName", attr);
  
  // Lorentzians
  switch ( m_uiForm.confit_cbFitType->currentIndex() )
  {
  case 0: // No Lorentzians
    break;
  case 1: // 1 Lorentzian
    func = Mantid::API::FunctionFactory::Instance().createFunction("Lorentzian");
    index = result->addFunction(func);
    populateFunction(func, result, m_cfProp["Lorentzian1"], index, tie);
    break;
  case 2: // 2 Lorentzian
    func = Mantid::API::FunctionFactory::Instance().createFunction("Lorentzian");
    index = result->addFunction(func);
    populateFunction(func, result, m_cfProp["Lorentzian1"], index, tie);
    func = Mantid::API::FunctionFactory::Instance().createFunction("Lorentzian");
    index = result->addFunction(func);
    populateFunction(func, result, m_cfProp["Lorentzian2"], index, tie);
    // Tie PeakCentres together
    if ( ! tie )
    {
      QString tieL = "f" + QString::number(index-1) + ".PeakCentre";
      QString tieR = "f" + QString::number(index) + ".PeakCentre";
      result->tie(tieL.toStdString(), tieR.toStdString());
    }
    break;
  }

  if ( tie ) { result->applyTies(); }

  return result;
  // conv->addFunction(result);
  // return conv;
}

void IndirectDataAnalysis::populateFunction(Mantid::API::IFunction* func, Mantid::API::IFunction* comp, QtProperty* group, int index, bool tie)
{
  // Get subproperties of group and apply them as parameters on the function object
  QList<QtProperty*> props = group->subProperties();
  QString pref = "f" + QString::number(index) + ".";

  for ( int i = 0; i < props.size(); i++ )
  {
    if ( tie )
    {
      QString propName = pref + props[i]->propertyName();
      comp->tie(propName.toStdString(), props[i]->valueText().toStdString() );
    }
    else { func->setParameter(props[i]->propertyName().toStdString(), props[i]->valueText().toDouble()); }
  }

}

QwtPlotCurve* IndirectDataAnalysis::plotMiniplot(QwtPlot* plot, QwtPlotCurve* curve, std::string workspace, int index)
{
  if ( curve != NULL )
  {
    curve->attach(0);
    delete curve;
    curve = 0;
  }

  Mantid::API::MatrixWorkspace_const_sptr ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(workspace));

  int nhist = ws->getNumberHistograms();
  if ( index >= nhist )
  {
    showInformationBox("Error: Workspace index out of range.");
    return 0;
  }

  const QVector<double> dataX = QVector<double>::fromStdVector(ws->readX(index));
  const QVector<double> dataY = QVector<double>::fromStdVector(ws->readY(index));

  curve = new QwtPlotCurve();
  curve->setData(dataX, dataY);
  curve->attach(plot);

  plot->replot();

  return curve;
}

void IndirectDataAnalysis::confitInputType(int index)
{
  m_uiForm.confit_swInput->setCurrentIndex(index);
}

void IndirectDataAnalysis::confitPlotInput()
{

  std::string wsname;
  switch ( m_uiForm.confit_cbInputType->currentIndex() )
  {
  case 0: // "File"
    {
      if ( m_uiForm.confit_inputFile->isValid() )
      {
        QFileInfo fi(m_uiForm.confit_inputFile->getFirstFilename());
        wsname = fi.baseName().toStdString();
        if ( (m_ffInputWS == NULL) || ( wsname != m_ffInputWSName ) )
        {
          std::string filename = m_uiForm.confit_inputFile->getFirstFilename().toStdString();
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
      wsname = m_uiForm.confit_cbWorkspace->currentText().toStdString();
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

  int specNo = m_uiForm.confit_leSpecNo->text().toInt();

  m_cfDataCurve = plotMiniplot(m_cfPlot, m_cfDataCurve, wsname, specNo);
  int npnts = m_cfDataCurve->data().size();
  const double & lower = m_cfDataCurve->data().x(0);
  const double & upper = m_cfDataCurve->data().x(npnts-1);
  m_cfRangeS->setRange(lower, upper);
}

void IndirectDataAnalysis::confitPlotGuess(QtProperty*)
{

  if ( ! m_uiForm.confit_ckPlotGuess->isChecked() )
  {
    return;
  }

  Mantid::API::CompositeFunction* function = confitCreateFunction(true);

  if ( m_cfInputWS == NULL )
  {
    confitPlotInput();
  }

  std::string inputName = m_cfInputWS->getName();

  const int binIndexLow = m_cfInputWS->binIndexOf(m_cfDblMng->value(m_cfProp["StartX"]));
  const int binIndexHigh = m_cfInputWS->binIndexOf(m_cfDblMng->value(m_cfProp["EndX"]));
  const int nData = binIndexHigh - binIndexLow;

  double* inputXData = new double[nData];
  double* outputData = new double[nData];

  const Mantid::MantidVec& XValues = m_cfInputWS->readX(0);
  const bool isHistogram = m_cfInputWS->isHistogramData();

  for ( int i = 0; i < nData; i++ )
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

  function->function(outputData, inputXData, nData);

  QVector<double> dataX, dataY;

  for ( int i = 0; i < nData; i++ )
  {
    dataX.append(inputXData[i]);
    dataY.append(outputData[i]);
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

void IndirectDataAnalysis::confitMinChanged(double val)
{
  m_cfDblMng->setValue(m_cfProp["StartX"], val);
}

void IndirectDataAnalysis::confitMaxChanged(double val)
{
  m_cfDblMng->setValue(m_cfProp["EndX"], val);
}

void IndirectDataAnalysis::confitBackgLevel(double val)
{
  m_cfDblMng->setValue(m_cfProp["BGA0"], val);
}

void IndirectDataAnalysis::confitUpdateRS(QtProperty* prop, double val)
{
  if ( prop == m_cfProp["StartX"] ) { m_cfRangeS->setMinimum(val); }
  else if ( prop == m_cfProp["EndX"] ) { m_cfRangeS->setMaximum(val); }
  else if ( prop == m_cfProp["BGA0"] ) { m_cfBackgS->setMinimum(val); }
}

void IndirectDataAnalysis::confitCheckBoxUpdate(QtProperty* prop, bool checked)
{
  // Add/remove some properties to display only relevant options
  if ( prop == m_cfProp["BGConstant"] )
  {
    if ( ! checked ) { m_cfProp["LinearBackground"]->addSubProperty(m_cfProp["BGA1"]); }
    else { m_cfProp["LinearBackground"]->removeSubProperty(m_cfProp["BGA1"]); }
  }
  else if ( prop == m_cfProp["UseDeltaFunc"] )
  {
    if ( checked ) { m_cfProp["DeltaFunction"]->addSubProperty(m_cfProp["DeltaHeight"]); }
    else { m_cfProp["DeltaFunction"]->removeSubProperty(m_cfProp["DeltaHeight"]); }
  }
}

/* ABSORPTION TAB */
void IndirectDataAnalysis::absorptionRun()
{
  if ( ! validateAbsorption() )
  {
    showInformationBox("Please check your input.");
    return;
  }

  QString pyInput =
    "from IndirectDataAnalysis import absorption\n"
    "file = r'" + m_uiForm.abs_inputFile->getFirstFilename() + "'\n"
    "mode = '" + m_uiForm.abs_cbShape->currentText() + "'\n"
    "sample = [ %1, %2, %3 ]\n"
    "can = [ %4, %5, %6, %7 ]\n";

  pyInput = pyInput.arg(m_uiForm.abs_leAttenuation->text());
  pyInput = pyInput.arg(m_uiForm.abs_leScatter->text());
  pyInput = pyInput.arg(m_uiForm.abs_leDensity->text());

  if ( m_uiForm.abs_cbShape->currentText() == "Flat Plate" )
  {
    pyInput = pyInput.arg(m_uiForm.abs_leFlatHeight->text());
    pyInput = pyInput.arg(m_uiForm.abs_leWidth->text());
    pyInput = pyInput.arg(m_uiForm.abs_leThickness->text());
    pyInput = pyInput.arg(m_uiForm.abs_leElementSize->text());
  }
  else
  {
    pyInput = pyInput.arg(m_uiForm.abs_leCylHeight->text());
    pyInput = pyInput.arg(m_uiForm.abs_leRadius->text());
    pyInput = pyInput.arg(m_uiForm.abs_leSlices->text());
    pyInput = pyInput.arg(m_uiForm.abs_leAnnuli->text());
  }

  if ( m_uiForm.abs_ckVerbose->isChecked() ) pyInput += "verbose = True\n";
  else pyInput += "verbose = False\n";

  if ( m_uiForm.abs_ckPlot->isChecked() ) pyInput += "plot = True\n";
  else pyInput += "plot = False\n";

  if ( m_uiForm.abs_ckSave->isChecked() ) pyInput += "save = True\n";
  else pyInput += "save = False\n";

  pyInput +=
    "absorption(file, mode, sample, can, Save=save, Verbose=verbose, Plot=plot)\n";
  QString pyOutput = runPythonCode(pyInput).trimmed();
}

void IndirectDataAnalysis::absorptionShape(int index)
{
  m_uiForm.abs_swDetails->setCurrentIndex(index);
}

void IndirectDataAnalysis::openDirectoryDialog()
{
  MantidQt::API::ManageUserDirectories *ad = new MantidQt::API::ManageUserDirectories(this);
  ad->setAttribute(Qt::WA_DeleteOnClose);
  ad->show();
  ad->setFocus();
}

void IndirectDataAnalysis::help()
{
  QString tabName = m_uiForm.tabWidget->tabText(m_uiForm.tabWidget->currentIndex());
  QString url = "http://www.mantidproject.org/IDA#";
  if ( tabName == "Initial Settings" )
    url += "";
  else if ( tabName == "Elwin" )
    url += "Elwin";
  else if ( tabName == "MSD Fit" )
    url += "MSD";
  else if ( tabName == "Fury" )
    url += "Fury";
  else if ( tabName == "FuryFit" )
    url += "FuryFit";
  else if ( tabName == "ConvFit" )
    url += "ConvFit";
  else if ( tabName == "Absorption" )
    url += "Absorption";
  QDesktopServices::openUrl(QUrl(url));
}

} //namespace
} //namespace
