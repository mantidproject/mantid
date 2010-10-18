//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/indirectAnalysis.h"
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
#include "DoubleEditorFactory.h"

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
  namespace CustomInterfaces
  {
    DECLARE_SUBWINDOW(indirectAnalysis);
  }
}

using namespace MantidQt::CustomInterfaces;

//----------------------
// Public member functions
//----------------------
///Constructor
indirectAnalysis::indirectAnalysis(QWidget *parent) :
UserSubWindow(parent), m_valInt(0), m_valDbl(0), m_furyResFileType(true), m_ffDataCurve(0), m_ffFitCurve(0)
{
}
void indirectAnalysis::closeEvent(QCloseEvent* close)
{
  (void) close;
  saveSettings();
}

/// Set up the dialog layout
void indirectAnalysis::initLayout()
{
  m_uiForm.setupUi(this);

  m_propBrowser = new QtTreePropertyBrowser();
  m_uiForm.furyfit_hlTreePlace->addWidget(m_propBrowser);
  setupTreePropertyBrowser();

  setupFFPlotArea();

  connect(m_uiForm.pbManageDirs, SIGNAL(clicked()), this, SLOT(openDirectoryDialog()));
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(help()));

  // settings
  connect(m_uiForm.set_cbInst, SIGNAL(currentIndexChanged(int)), this, SLOT(instrumentChanged(int)));
  connect(m_uiForm.set_cbAnalyser, SIGNAL(currentIndexChanged(int)), this, SLOT(analyserSelected(int)));
  connect(m_uiForm.set_cbReflection, SIGNAL(currentIndexChanged(int)), this, SLOT(reflectionSelected(int)));
  // fury
  connect(m_uiForm.fury_pbRun, SIGNAL(clicked()), this, SLOT(furyRun()));
  connect(m_uiForm.fury_cbResType, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(furyResType(const QString&)));
  connect(m_uiForm.fury_pbPlotInput, SIGNAL(clicked()), this, SLOT(furyPlotInput()));
  // fury fit
  connect(m_uiForm.furyfit_cbFitType, SIGNAL(currentIndexChanged(int)), this, SLOT(furyfit_typeSelection(int)));
  connect(m_uiForm.furyfit_pbRun, SIGNAL(clicked()), this, SLOT(runFuryFit()));
  connect(m_uiForm.furyfit_pbPlotInput, SIGNAL(clicked()), this, SLOT(furyfitPlotInput()));
  connect(m_uiForm.furyfit_leSpecNo, SIGNAL(editingFinished()), this, SLOT(furyfitPlotInput()));
  // elwin
  connect(m_uiForm.elwin_pbRun, SIGNAL(clicked()), this, SLOT(elwinRun()));
  connect(m_uiForm.elwin_pbPlotInput, SIGNAL(clicked()), this, SLOT(elwinPlotInput()));
  connect(m_uiForm.elwin_ckUseTwoRanges, SIGNAL(toggled(bool)), this, SLOT(elwinTwoRanges(bool)));
  // msd
  connect(m_uiForm.msd_pbRun, SIGNAL(clicked()), this, SLOT(msdRun()));
  connect(m_uiForm.msd_pbPlotInput, SIGNAL(clicked()), this, SLOT(msdPlotInput()));
  // absorption
  connect(m_uiForm.abs_pbRun, SIGNAL(clicked()), this, SLOT(absorptionRun()));
  connect(m_uiForm.abs_cbShape, SIGNAL(activated(int)), this, SLOT(absorptionShape(int)));
  // demon
  connect(m_uiForm.dem_pbRun, SIGNAL(clicked()), this, SLOT(demonRun()));

  m_dataDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("datasearch.directories"));
  m_saveDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory"));

  m_settingsGroup = "CustomInterfaces/IndirectAnalysis/";

  // create validators
  m_valInt = new QIntValidator(this);
  m_valDbl = new QDoubleValidator(this);

  // apply validators - settings
  m_uiForm.set_leSpecMin->setValidator(m_valInt);
  m_uiForm.set_leSpecMax->setValidator(m_valInt);
  m_uiForm.set_leEFixed->setValidator(m_valDbl);
  // apply validators - fury
  m_uiForm.fury_leELow->setValidator(m_valDbl);
  m_uiForm.fury_leEWidth->setValidator(m_valDbl);
  m_uiForm.fury_leEHigh->setValidator(m_valDbl);
  // apply validators - furyfit
  m_uiForm.furyfit_leSpecNo->setValidator(m_valInt);
  // apply validators - elwin
  m_uiForm.elwin_leEStart->setValidator(m_valDbl);
  m_uiForm.elwin_leEEnd->setValidator(m_valDbl);
  m_uiForm.elwin_leRangeTwoStart->setValidator(m_valDbl);
  m_uiForm.elwin_leRangeTwoEnd->setValidator(m_valDbl);
  // apply validators - msd
  m_uiForm.msd_leStartX->setValidator(m_valDbl);
  m_uiForm.msd_leEndX->setValidator(m_valDbl);
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
}

void indirectAnalysis::initLocalPython()
{
  instrumentChanged(m_uiForm.set_cbInst->currentIndex());
  loadSettings();
}

void indirectAnalysis::loadSettings()
{
  QSettings settings;

  settings.beginGroup(m_settingsGroup + "ProcessedFiles");
  settings.setValue("last_directory", m_saveDir);
  m_uiForm.fury_iconFile->readSettings(settings.group());
  m_uiForm.fury_resFile->readSettings(settings.group());
  m_uiForm.furyfit_inputFile->readSettings(settings.group());
  m_uiForm.elwin_inputFile->readSettings(settings.group());
  m_uiForm.elwin_inputFile->readSettings(settings.group());
  m_uiForm.msd_inputFile->readSettings(settings.group());
  m_uiForm.abs_inputFile->readSettings(settings.group());
  settings.endGroup();

  settings.beginGroup(m_settingsGroup + "InstrumentOptions");
  QString instrument = settings.value("instrument", "").toString();
  QString analyser = settings.value("analyser", "").toString();
  QString reflection = settings.value("reflection", "").toString();

  if ( instrument != "" )
  {
    int index = m_uiForm.set_cbInst->findText(instrument);
    if ( index != -1 )
    {
      m_uiForm.set_cbInst->setCurrentIndex(index);
    }
  }
  if ( analyser != "" )
  {
    int index = m_uiForm.set_cbAnalyser->findText(analyser);
    if ( index != -1 )
    {
      m_uiForm.set_cbAnalyser->setCurrentIndex(index);
    }
  }
  if ( reflection != "" )
  {
    int index = m_uiForm.set_cbReflection->findText(reflection);
    if ( index != -1 )
    {
      m_uiForm.set_cbReflection->setCurrentIndex(index);
    }
  }

  settings.endGroup();

}
void indirectAnalysis::saveSettings()
{ // The only settings we want to preserve are the instrument selection.
  QSettings settings;
  settings.beginGroup(m_settingsGroup + "InstrumentOptions");
  QString instrument = m_uiForm.set_cbInst->currentText();
  QString analyser = m_uiForm.set_cbAnalyser->currentText();
  QString reflection = m_uiForm.set_cbReflection->currentText();

  settings.setValue("instrument", instrument);
  settings.setValue("analyser", analyser);
  settings.setValue("reflection", reflection);
}

void indirectAnalysis::setupTreePropertyBrowser()
{
  m_groupManager = new QtGroupPropertyManager();
  m_doubleManager = new QtDoublePropertyManager();
  m_ffRangeManager = new QtDoublePropertyManager();
  DoubleEditorFactory *doubleEditorFactory = new DoubleEditorFactory();
  m_propBrowser->setFactoryForManager(m_doubleManager, doubleEditorFactory);
  m_propBrowser->setFactoryForManager(m_ffRangeManager, doubleEditorFactory);

  m_fitProperties["StartX"] = m_ffRangeManager->addProperty("StartX");
  m_ffRangeManager->setDecimals(m_fitProperties["StartX"], 10);
  m_fitProperties["EndX"] = m_ffRangeManager->addProperty("EndX");
  m_ffRangeManager->setDecimals(m_fitProperties["EndX"], 10);

  connect(m_ffRangeManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(furyfitRangePropChanged(QtProperty*, double)));

  m_fitProperties["LinearBackground"] = m_groupManager->addProperty("LinearBackground");
  QtProperty* bgA0 = m_ffRangeManager->addProperty("A0");
  QtProperty* bgA1 = m_doubleManager->addProperty("A1");
  m_ffRangeManager->setDecimals(bgA0, 10);
  m_doubleManager->setDecimals(bgA1, 1);
  m_doubleManager->setRange(bgA1, 0.0, 0.0);
  m_fitProperties["LinearBackground"]->addSubProperty(bgA0);
  m_fitProperties["LinearBackground"]->addSubProperty(bgA1);
  m_fitProperties["BackgroundA0"] = bgA0;
  m_fitProperties["Lorentzian1"] = createLorentzian();
  m_fitProperties["Lorentzian2"] = createLorentzian();

  m_fitProperties["Exponential1"] = createExponential();
  m_fitProperties["Exponential2"] = createExponential();
  
  m_fitProperties["StretchedExp"] = createStretchedExp();

  furyfit_typeSelection(m_uiForm.furyfit_cbFitType->currentIndex());
}

void indirectAnalysis::setupFFPlotArea()
{
  m_furyFitPlotWindow = new QwtPlot(this);
  m_furyFitPlotWindow->setAxisFont(QwtPlot::xBottom, this->font());
  m_furyFitPlotWindow->setAxisFont(QwtPlot::yLeft, this->font());
  m_uiForm.furyfit_vlPlot->addWidget(m_furyFitPlotWindow);
  m_furyFitPlotWindow->setCanvasBackground(QColor(255,255,255));
  
  m_ffRangeS = new MantidQt::MantidWidgets::RangeSelector(m_furyFitPlotWindow);
  connect(m_ffRangeS, SIGNAL(minValueChanged(double)), this, SLOT(furyfitXMinSelected(double)));
  connect(m_ffRangeS, SIGNAL(maxValueChanged(double)), this, SLOT(furyfitXMaxSelected(double)));

  m_ffBackRangeS = new MantidQt::MantidWidgets::RangeSelector(m_furyFitPlotWindow,
    MantidQt::MantidWidgets::RangeSelector::YSINGLE);
  m_ffBackRangeS->setRange(0.0,1.0);
  m_ffBackRangeS->setColour(Qt::darkGreen);
  connect(m_ffBackRangeS, SIGNAL(minValueChanged(double)), this, SLOT(furyfitBackgroundSelected(double)));

}

bool indirectAnalysis::validateFury()
{
  bool valid = true;

  if ( ! m_uiForm.fury_iconFile->isValid() )
  {
    valid = false;
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
bool indirectAnalysis::validateElwin()
{
  bool valid = true;

  if ( ! m_uiForm.elwin_inputFile->isValid() )
  {
    valid = false;
  }

  if ( m_uiForm.elwin_leEStart->text() == "" )
  {
    m_uiForm.elwin_valRangeStart->setText("*");
    valid = false;
  }
  else
  {
    m_uiForm.elwin_valRangeStart->setText(" ");
  }
  if ( m_uiForm.elwin_leEEnd->text() == "" )
  {
    m_uiForm.elwin_valRangeEnd->setText("*");
    valid = false;
  }
  else
  {
    m_uiForm.elwin_valRangeEnd->setText(" ");
  }

  if ( m_uiForm.elwin_ckUseTwoRanges->isChecked() )
  {
    if ( m_uiForm.elwin_leRangeTwoStart->text() == "" )
    {
      m_uiForm.elwin_valRangeTwoStart->setText("*");
      valid = false;
    }
    else
    {
      m_uiForm.elwin_valRangeTwoStart->setText(" ");
    }

    if ( m_uiForm.elwin_leRangeTwoEnd->text() == "" )
    {
      m_uiForm.elwin_valRangeTwoEnd->setText("*");
      valid = false;
    }
    else
    {
      m_uiForm.elwin_valRangeTwoEnd->setText(" ");
    }
  }

  return valid;
}
bool indirectAnalysis::validateMsd()
{
  bool valid = true;

  if ( ! m_uiForm.msd_inputFile->isValid() )
  {
    valid = false;
  }

  if ( m_uiForm.msd_leStartX->text() == "" )
  {
    m_uiForm.msd_valStartX->setText("*");
    valid = false;
  }
  else
  {
    m_uiForm.msd_valStartX->setText(" ");
  }

  if ( m_uiForm.msd_leEndX->text() == "" )
  {
    m_uiForm.msd_valEndX->setText("*");
    valid = false;
  }
  else
  {
    m_uiForm.msd_valEndX->setText(" ");
  }


  return valid;
}
bool indirectAnalysis::validateAbsorption()
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
bool indirectAnalysis::validateDemon()
{
  return m_uiForm.dem_rawFiles->isValid();
}

Mantid::API::CompositeFunction* indirectAnalysis::createFunction()
{
  Mantid::API::CompositeFunction* result = new Mantid::API::CompositeFunction();

  // QString message;
  QList<QtBrowserItem*> items = m_propBrowser->topLevelItems();
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
        m_furyfitConstraints += "0 <= f%1.Intensity <= 1,0 <= f%1.Beta <= 1";
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
        if ( m_furyfitConstraints != "" ) m_furyfitConstraints += ",";
        m_furyfitConstraints += "0 <= f%1.Intensity <= 1";
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

QtProperty* indirectAnalysis::createLorentzian()
{
  QtProperty* lorentzGroup = m_groupManager->addProperty("Lorentzian");
  QtProperty* lzA0 = m_doubleManager->addProperty("Height");
  m_doubleManager->setRange(lzA0, 0.0, 1.0); // 0 < Height < 1
  QtProperty* lzA1 = m_doubleManager->addProperty("PeakCentre");
  QtProperty* lzA2 = m_doubleManager->addProperty("HWHM");
  m_doubleManager->setDecimals(lzA0, 10);
  m_doubleManager->setDecimals(lzA1, 10);
  m_doubleManager->setDecimals(lzA2, 10);
  lorentzGroup->addSubProperty(lzA0);
  lorentzGroup->addSubProperty(lzA1);
  lorentzGroup->addSubProperty(lzA2);
  return lorentzGroup;
}

QtProperty* indirectAnalysis::createExponential()
{
  QtProperty* expGroup = m_groupManager->addProperty("Exponential");
  QtProperty* expA0 = m_doubleManager->addProperty("Intensity");
  m_doubleManager->setRange(expA0, 0.0, 1.0); // 0 < Height < 1
  m_doubleManager->setDecimals(expA0, 10);
  QtProperty* expA1 = m_doubleManager->addProperty("Exponent");
  m_doubleManager->setDecimals(expA1, 10);
  expGroup->addSubProperty(expA0);
  expGroup->addSubProperty(expA1);
  return expGroup;
}

QtProperty* indirectAnalysis::createStretchedExp()
{
  QtProperty* prop = m_groupManager->addProperty("Stretched Exponential");
  QtProperty* stA0 = m_doubleManager->addProperty("Intensity");
  m_doubleManager->setRange(stA0, 0.0, 1.0);  // 0 < Height < 1
  QtProperty* stA1 = m_doubleManager->addProperty("Exponent");
  QtProperty* stA2 = m_doubleManager->addProperty("Beta");
  m_doubleManager->setDecimals(stA0, 10);
  m_doubleManager->setDecimals(stA1, 10);
  m_doubleManager->setDecimals(stA2, 10);
  m_doubleManager->setRange(stA2, 0.0, 1.0);
  prop->addSubProperty(stA0);
  prop->addSubProperty(stA1);
  prop->addSubProperty(stA2);
  return prop;
}

void indirectAnalysis::instrumentChanged(int index)
{
  m_uiForm.set_cbAnalyser->blockSignals(true);
  m_uiForm.set_cbAnalyser->clear();
  m_uiForm.set_cbAnalyser->blockSignals(false);

  QString pyInput = 
    "from IndirectEnergyConversion import getInstrumentDetails\n"
    "result = getInstrumentDetails('" + m_uiForm.set_cbInst->currentText() + "')\n"
    "print result\n";
  QString pyOutput = runPythonCode(pyInput).trimmed();

  if ( pyOutput == "" )
  {
    showInformationBox("Could not gather required information from instrument definition.");
  }
  else
  {
    QStringList analysers = pyOutput.split("\n", QString::SkipEmptyParts);

    for (int i = 0; i< analysers.count(); i++ )
    {
      QString text; // holds Text field of combo box (name of analyser)
      QVariant data; // holds Data field of combo box (list of reflections)

      QStringList analyser = analysers[i].split("-", QString::SkipEmptyParts);

      text = analyser[0];

      if ( analyser.count() > 1 )
      {
        QStringList reflections = analyser[1].split(",", QString::SkipEmptyParts);
        data = QVariant(reflections);
        m_uiForm.set_cbAnalyser->addItem(text, data);
      }
      else
      {
        m_uiForm.set_cbAnalyser->addItem(text);
      }
    }
  }
}
void indirectAnalysis::analyserSelected(int index)
{
  // populate Reflection combobox with correct values for Analyser selected.
  m_uiForm.set_cbReflection->blockSignals(true);
  m_uiForm.set_cbReflection->clear();
  m_uiForm.set_cbReflection->blockSignals(false);

  QVariant currentData = m_uiForm.set_cbAnalyser->itemData(index);
  if ( currentData == QVariant::Invalid )
  {
    m_uiForm.set_lbReflection->setEnabled(false);
    m_uiForm.set_cbReflection->setEnabled(false);
  }
  else
  {
    m_uiForm.set_lbReflection->setEnabled(true);
    m_uiForm.set_cbReflection->setEnabled(true);
    QStringList reflections = currentData.toStringList();
    for ( int i = 0; i < reflections.count(); i++ )
    {
      m_uiForm.set_cbReflection->addItem(reflections[i]);
    }
  }
}
void indirectAnalysis::reflectionSelected(int index)
{
  QString pyInput =
    "from IndirectEnergyConversion import getReflectionDetails\n"
    "instrument = '" + m_uiForm.set_cbInst->currentText() + "'\n"
    "analyser = '" + m_uiForm.set_cbAnalyser->currentText() + "'\n"
    "reflection = '" + m_uiForm.set_cbReflection->currentText() + "'\n"
    "print getReflectionDetails(instrument, analyser, reflection)\n";
  QString pyOutput = runPythonCode(pyInput).trimmed();

  QStringList values = pyOutput.split("\n", QString::SkipEmptyParts);

  QString analysisType = values[0];
  m_uiForm.set_leSpecMin->setText(values[1]);
  m_uiForm.set_leSpecMax->setText(values[2]);
  
  if ( values.count() == 8 )
  {
    m_uiForm.set_leEFixed->setText(values[3]);
  }
  else
  {
    m_uiForm.set_leEFixed->clear();
  }
  bool state;
  if ( analysisType == "diffraction" )
  {
    state = false;
  }
  else
  {
    state = true;
  }
  m_uiForm.tabElwin->setEnabled(state);
  m_uiForm.tabMSD->setEnabled(state);
  m_uiForm.tabFury->setEnabled(state);
  m_uiForm.tabAbsorption->setEnabled(state);
  m_uiForm.tabDemon->setEnabled(!state);
}
void indirectAnalysis::furyRun()
{
  if ( !validateFury() )
  {
    showInformationBox("Please check your input.");
    return;
  }

  QString filenames = m_uiForm.fury_iconFile->getFilenames().join("', r'");

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
void indirectAnalysis::furyResType(const QString& type)
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
void indirectAnalysis::furyPlotInput()
{
  if ( m_uiForm.fury_iconFile->isValid() )
  {
    QString pyInput = "from IndirectDataAnalysis import plotInput\n"
      "inputfiles = [r'" + m_uiForm.fury_iconFile->getFilenames().join("', r'") + "']\n"
      "spec = ["+m_uiForm.set_leSpecMin->text() + "," + m_uiForm.set_leSpecMax->text() +"]\n"
      "plotInput(inputfiles, spectra=spec)\n";
    QString pyOutput = runPythonCode(pyInput).trimmed();
  }
}

// FURYFIT

void indirectAnalysis::runFuryFit()
{
  if ( m_uiForm.furyfit_inputFile->isValid() )
  {
    // First create the function
    Mantid::API::CompositeFunction* function = createFunction();

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

    // Now load up the input workspace (if not already loaded)
    QFileInfo fi(m_uiForm.furyfit_inputFile->getFirstFilename());
    std::string wsname = fi.baseName().toStdString();

    bool wsExists = Mantid::API::AnalysisDataService::Instance().doesExist(wsname);

    if ( ( wsname != m_ffInputWSName ) || ! wsExists )
    {
    QString pyInput = "from mantidsimple import *\n"
      "LoadNexus(r'" + m_uiForm.furyfit_inputFile->getFirstFilename() + "', '"+QString::fromStdString(wsname)+"')\n";
    QString pyOutput = runPythonCode(pyInput);
    m_ffInputWSName = wsname;
    }

    /// @todo some check here to make sure the spectra number isn't out of range
    
    std::string output = wsname + "_fit";
    // Create the Fit Algorithm
    Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("Fit");
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", m_ffInputWSName);
    alg->setProperty("WorkspaceIndex", m_uiForm.furyfit_leSpecNo->text().toInt());
    alg->setProperty("StartX", m_ffRangeManager->value(m_fitProperties["StartX"]));
    alg->setProperty("EndX", m_ffRangeManager->value(m_fitProperties["EndX"]));
    alg->setProperty("Ties", m_furyfitTies.toStdString());
    alg->setProperty("Constraints", m_furyfitConstraints.toStdString());
    alg->setPropertyValue("Function", *function);
    alg->setPropertyValue("Output",output);
    alg->execute();

    // Now show the fitted curve of the mini plot
    m_ffOutputWS = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(output+"_Workspace"));
    const QVector<double> dataX = QVector<double>::fromStdVector(m_ffOutputWS->readX(1));
    const QVector<double> dataY = QVector<double>::fromStdVector(m_ffOutputWS->readY(1));

    if ( m_ffFitCurve != NULL )
    {
      m_ffFitCurve->attach(0);
      delete m_ffFitCurve;
    }
    m_ffFitCurve = new QwtPlotCurve();
    m_ffFitCurve->setData(dataX, dataY);
    m_ffFitCurve->attach(m_furyFitPlotWindow);

    QPen fitPen(Qt::red, Qt::SolidLine);
    m_ffFitCurve->setPen(fitPen);
    m_furyFitPlotWindow->replot();

    // Get the "*_Parameters" TableWorkspace created by the Fit function
    Mantid::API::ITableWorkspace_sptr table = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(output+"_Parameters"));
    std::map<std::string,double> params;
    Mantid::API::TableRow row = table->getFirstRow();
    do
    {
      std::string key;
      double value, error;
      row >> key >> value >> error;
      params[key] = value;
    }
    while ( row.next() );

    // Background is in all functions
    m_ffRangeManager->setValue(m_fitProperties["BackgroundA0"], params["f0.A0"]);

    QMap<QString,QtProperty*> subprops;
    QtProperty* exp;
    QList<QtProperty*> subs;

    switch ( m_uiForm.furyfit_cbFitType->currentIndex() )
    {
    case 0:
    case 1:
    case 3:
      {
      exp = m_fitProperties["Exponential1"];
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
      exp = m_fitProperties["StretchedExp"];
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
      exp = m_fitProperties["Exponential2"];
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
      exp = m_fitProperties["StretchedExp"];
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
  else
  {
    showInformationBox("You must select an input file.");
  }
}

void indirectAnalysis::furyfit_typeSelection(int index)
{
  m_propBrowser->clear();

  // StartX and EndX
  //m_doubleManager->setDecimals(startX, 10);
  //m_doubleManager->setDecimals(endX, 10);
  m_propBrowser->addProperty(m_fitProperties["StartX"]);
  m_propBrowser->addProperty(m_fitProperties["EndX"]);

  m_propBrowser->addProperty(m_fitProperties["LinearBackground"]);

  switch ( index )
  {
  case 0:
    m_propBrowser->addProperty(m_fitProperties["Exponential1"]);
    break;
  case 1:
    m_propBrowser->addProperty(m_fitProperties["Exponential1"]);
    m_propBrowser->addProperty(m_fitProperties["Exponential2"]);
    break;
  case 2:
    m_propBrowser->addProperty(m_fitProperties["StretchedExp"]);
    break;
  case 3:
    m_propBrowser->addProperty(m_fitProperties["Exponential1"]);
    m_propBrowser->addProperty(m_fitProperties["StretchedExp"]);
    break;
  default:
    showInformationBox("Something very bad has happened.");
    return;
    break;
  }
}

void indirectAnalysis::furyfitPlotInput()
{
  if ( ! m_uiForm.furyfit_inputFile->isValid() )
  {
    return;
  }
  else
  {
    if ( m_ffDataCurve != NULL )
    {
      m_ffDataCurve->attach(0);
      delete m_ffDataCurve;
    }

    QFileInfo fi(m_uiForm.furyfit_inputFile->getFirstFilename());
    std::string wsname = fi.baseName().toStdString();

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
    m_ffInputWSName = wsname;

    /// @todo some check here to make sure the spectra number isn't out of range
    int specNo = m_uiForm.furyfit_leSpecNo->text().toInt();

    // get the data of that spectra number
    const QVector<double> dataX = QVector<double>::fromStdVector(m_ffInputWS->readX(specNo));
    const QVector<double> dataY = QVector<double>::fromStdVector(m_ffInputWS->readY(specNo));

    // get xMin and xMax range
    const double & lower = dataX.first();
    const double & upper = dataX.last();
    m_ffRangeS->setRange(lower, upper);
    m_ffRangeManager->setRange(m_fitProperties["StartX"], lower, upper);
    m_ffRangeManager->setRange(m_fitProperties["EndX"], lower, upper);
            
    // create the QwtPlotCurve
    m_ffDataCurve = new QwtPlotCurve();
    m_ffDataCurve->setData(dataX, dataY);
    m_ffDataCurve->attach(m_furyFitPlotWindow);

    m_furyFitPlotWindow->setAxisScale(QwtPlot::xBottom, lower, upper);
    m_furyFitPlotWindow->setAxisScale(QwtPlot::yLeft, 0.0, 1.0);
    m_furyFitPlotWindow->replot();
  }
}

void indirectAnalysis::furyfitXMinSelected(double val)
{
  // disconnect(m_ffRangeS, SIGNAL(minValueChanged(double)), this, SLOT(furyfitXMinSelected(double)));
  m_ffRangeManager->setValue(m_fitProperties["StartX"], val);
  // connect(m_ffRangeS, SIGNAL(minValueChanged(double)), this, SLOT(furyfitXMinSelected(double)));
}
void indirectAnalysis::furyfitXMaxSelected(double val)
{
  // disconnect(m_ffRangeS, SIGNAL(maxValueChanged(double)), this, SLOT(furyfitXMaxSelected(double)));
  m_ffRangeManager->setValue(m_fitProperties["EndX"], val);
  // connect(m_ffRangeS, SIGNAL(maxValueChanged(double)), this, SLOT(furyfitXMaxSelected(double)));
}

void indirectAnalysis::furyfitBackgroundSelected(double val)
{
  m_ffRangeManager->setValue(m_fitProperties["BackgroundA0"], val);
}

void indirectAnalysis::furyfitRangePropChanged(QtProperty* prop, double val)
{
  if ( prop == m_fitProperties["StartX"] )
  {
    m_ffRangeS->setMinimum(val);
  }
  else if ( prop == m_fitProperties["EndX"] )
  {
    m_ffRangeS->setMaximum(val);
  }
  else if ( prop == m_fitProperties["BackgroundA0"] )
  {
    m_ffBackRangeS->setMinimum(val);
  }
}

void indirectAnalysis::elwinRun()
{
  if ( ! validateElwin() )
  {
    showInformationBox("Please check your input.");
    return;
  }

  QString pyInput =
    "from IndirectDataAnalysis import elwin\n"
    "input = [r'" + m_uiForm.elwin_inputFile->getFilenames().join("', r'") + "']\n"
    "eRange = [ " + m_uiForm.elwin_leEStart->text() +","+ m_uiForm.elwin_leEEnd->text();
  if ( m_uiForm.elwin_ckUseTwoRanges->isChecked() )
  {
    pyInput += ", " + m_uiForm.elwin_leRangeTwoStart->text() + ", " + m_uiForm.elwin_leRangeTwoEnd->text();
  }

  pyInput+= "]\n"
    "eFixed = "+ m_uiForm.set_leEFixed->text() +"\n";

  if ( m_uiForm.elwin_ckVerbose->isChecked() ) pyInput += "verbose = True\n";
  else pyInput += "verbose = False\n";

  if ( m_uiForm.elwin_ckPlot->isChecked() ) pyInput += "plot = True\n";
  else pyInput += "plot = False\n";

  if ( m_uiForm.elwin_ckSave->isChecked() ) pyInput += "save = True\n";
  else pyInput += "save = False\n";

  pyInput +=
    "elwin_ws = elwin(input, eRange, eFixed, Save=save, Verbose=verbose, Plot=plot)\n";

  QString pyOutput = runPythonCode(pyInput).trimmed();
}
void indirectAnalysis::elwinPlotInput()
{
  if ( m_uiForm.elwin_inputFile->isValid() )
  {
    QString pyInput = "from IndirectDataAnalysis import plotInput\n"
      "inputfiles = [r'" + m_uiForm.elwin_inputFile->getFilenames().join("', r'") + "']\n"
      "spec = ["+m_uiForm.set_leSpecMin->text() + "," + m_uiForm.set_leSpecMax->text() +"]\n"
      "plotInput(inputfiles, spectra=spec)\n";
    QString pyOutput = runPythonCode(pyInput).trimmed();
  }
}
void indirectAnalysis::elwinTwoRanges(bool state)
{
  QString val;
  if ( state ) val = "*";
  else val = " ";
  m_uiForm.elwin_lbR2Start->setEnabled(state);
  m_uiForm.elwin_lbR2End->setEnabled(state);
  m_uiForm.elwin_leRangeTwoStart->setEnabled(state);
  m_uiForm.elwin_leRangeTwoEnd->setEnabled(state);
  m_uiForm.elwin_valRangeTwoStart->setEnabled(state);
  m_uiForm.elwin_valRangeTwoEnd->setEnabled(state);
  m_uiForm.elwin_valRangeTwoStart->setText(val);
  m_uiForm.elwin_valRangeTwoEnd->setText(val);
}
void indirectAnalysis::msdRun()
{
  if ( ! validateMsd() )
  {
    showInformationBox("Please check your input.");
    return;
  }

  QString pyInput =
    "from IndirectDataAnalysis import msdfit\n"
    "startX = " + m_uiForm.msd_leStartX->text() +"\n"
    "endX = " + m_uiForm.msd_leEndX->text() +"\n"
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
void indirectAnalysis::msdPlotInput()
{
  QString pyInput = "from mantidsimple import *\n"
    "from mantidplot import *\n"
    "LoadNexusProcessed(r'" + m_uiForm.msd_inputFile->getFirstFilename() + "', 'msd_input_plot')\n"
    "plotSpectrum('msd_input_plot', 0)\n";
  QString pyOutput = runPythonCode(pyInput).trimmed();
}
void indirectAnalysis::absorptionRun()
{
  if ( ! validateAbsorption() )
  {
    showInformationBox("Please check your input.");
    return;
  }

  QString pyInput =
    "from IndirectDataAnalysis import absorption\n"
    "efixed = " + m_uiForm.set_leEFixed->text() + "\n"
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
    "absorption(file, mode, sample, can, efixed, Save=save, Verbose=verbose, Plot=plot)\n";
  QString pyOutput = runPythonCode(pyInput).trimmed();
}
void indirectAnalysis::absorptionShape(int index)
{
  m_uiForm.abs_swDetails->setCurrentIndex(index);
}
void indirectAnalysis::demonRun()
{
  if ( validateDemon() )
  {
    QString pyInput = "from IndirectDataAnalysis import demon\n"
      "files = [r'" + m_uiForm.dem_rawFiles->getFilenames().join("',r'") + "']\n"
      "first = " +m_uiForm.set_leSpecMin->text()+"\n"
      "last = " +m_uiForm.set_leSpecMax->text()+"\n";

    if ( m_uiForm.dem_ckVerbose->isChecked() ) pyInput += "verbose = True\n";
    else pyInput += "verbose = False\n";

    if ( m_uiForm.dem_ckPlot->isChecked() ) pyInput += "plot = True\n";
    else pyInput += "plot = False\n";

    if ( m_uiForm.dem_ckSave->isChecked() ) pyInput += "save = True\n";
    else pyInput += "save = False\n";

    pyInput += "ws, rn = demon(files, first, last, Verbose=verbose, Plot=plot, Save=save)\n";

    QString pyOutput = runPythonCode(pyInput).trimmed();
  }
  else
  {
    showInformationBox("Input invalid.");
  }
}
void indirectAnalysis::openDirectoryDialog()
{
  MantidQt::API::ManageUserDirectories *ad = new MantidQt::API::ManageUserDirectories(this);
  ad->setAttribute(Qt::WA_DeleteOnClose);
  ad->show();
  ad->setFocus();
}
void indirectAnalysis::help()
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
  else if ( tabName == "DEMON" )
    url += "Demon";
  QDesktopServices::openUrl(QUrl(url));
}