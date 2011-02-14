#include "MantidQtCustomInterfaces/Indirect.h"

#include "MantidQtCustomInterfaces/Background.h"

#include "MantidKernel/ConfigService.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <Poco/NObserver.h>

#include <QUrl>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QLineEdit>

#include "qttreepropertybrowser.h"
#include "qtpropertymanager.h"
#include "qteditorfactory.h"
#include "DoubleEditorFactory.h"
#include <QtCheckBoxFactory>

using namespace MantidQt::CustomInterfaces;

/**
* This is the constructor for the Indirect Instruments Interface.
* It is used primarily to ensure sane values for member variables.
*/
Indirect::Indirect(QWidget *parent, Ui::ConvertToEnergy & uiForm) : 
UserSubWindow(parent), m_uiForm(uiForm), m_backgroundDialog(NULL), m_isDirty(true),
  m_isDirtyRebin(true), m_bgRemoval(false), m_valInt(NULL), m_valDbl(NULL), 
  m_changeObserver(*this, &Indirect::handleDirectoryChange),
  // Null pointers - Calibration Tab
  m_calCalPlot(NULL), m_calResPlot(NULL),
  m_calCalR1(NULL), m_calCalR2(NULL), m_calResR1(NULL),
  m_calCalCurve(NULL), m_calResCurve(NULL),
  // Null pointers - Diagnostics Tab
  m_sltPlot(NULL), m_sltR1(NULL), m_sltR2(NULL), m_sltDataCurve(NULL)

{
  // Constructor
}
/**
* This function performs any one-time actions needed when the Inelastic interface
* is first selected, such as connecting signals to slots.
*/
void Indirect::initLayout()
{
  Mantid::Kernel::ConfigService::Instance().addObserver(m_changeObserver);

  m_settingsGroup = "CustomInterfaces/ConvertToEnergy/Indirect/";

  setupCalibration(); // setup the calibration miniplots
  setupSlice(); // setup the slice miniplot

  // "Energy Transfer" tab
  connect(m_uiForm.cbAnalyser, SIGNAL(activated(int)), this, SLOT(analyserSelected(int)));
  connect(m_uiForm.cbReflection, SIGNAL(activated(int)), this, SLOT(reflectionSelected(int)));
  connect(m_uiForm.cbMappingOptions, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(mappingOptionSelected(const QString&)));
  connect(m_uiForm.pbBack_2, SIGNAL(clicked()), this, SLOT(backgroundClicked()));
  connect(m_uiForm.pbPlotRaw, SIGNAL(clicked()), this, SLOT(plotRaw()));
  connect(m_uiForm.rebin_pbRebin, SIGNAL(clicked()), this, SLOT(rebinData()));
  connect(m_uiForm.rebin_ckDNR, SIGNAL(toggled(bool)), this, SLOT(rebinCheck(bool)));
  connect(m_uiForm.ckDetailedBalance, SIGNAL(toggled(bool)), this, SLOT(detailedBalanceCheck(bool)));

  connect(m_uiForm.ind_runFiles, SIGNAL(fileEditingFinished()), this, SLOT(setasDirty()));
  connect(m_uiForm.ind_calibFile, SIGNAL(fileEditingFinished()), this, SLOT(setasDirty()));
  connect(m_uiForm.ind_calibFile, SIGNAL(fileTextChanged(const QString &)), this, SLOT(calibFileChanged(const QString &)));
  connect(m_uiForm.leSpectraMin, SIGNAL(editingFinished()), this, SLOT(setasDirty()));
  connect(m_uiForm.leSpectraMax, SIGNAL(editingFinished()), this, SLOT(setasDirty()));
  connect(m_uiForm.ckSumFiles, SIGNAL(pressed()), this, SLOT(setasDirty()));
  connect(m_uiForm.ckUseCalib, SIGNAL(toggled(bool)), this, SLOT(useCalib(bool)));
  connect(m_uiForm.ckCleanUp, SIGNAL(pressed()), this, SLOT(setasDirtyRebin()));

  connect(m_uiForm.rebin_leELow, SIGNAL(editingFinished()), this, SLOT(setasDirtyRebin()));
  connect(m_uiForm.rebin_leEWidth, SIGNAL(editingFinished()), this, SLOT(setasDirtyRebin()));
  connect(m_uiForm.rebin_leEHigh, SIGNAL(editingFinished()), this, SLOT(setasDirtyRebin()));
  connect(m_uiForm.leDetailedBalance, SIGNAL(editingFinished()), this, SLOT(setasDirtyRebin()));
  connect(m_uiForm.ind_mapFile, SIGNAL(fileEditingFinished()), this, SLOT(setasDirtyRebin()));

  connect(m_uiForm.tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));

  // "Calibration" tab
  connect(m_uiForm.cal_pbPlot, SIGNAL(clicked()), this, SLOT(calPlotRaw()));
  connect(m_uiForm.cal_pbPlotEnergy, SIGNAL(clicked()), this, SLOT(calPlotEnergy()));
  connect(m_uiForm.cal_ckRES, SIGNAL(toggled(bool)), this, SLOT(resCheck(bool)));

  // "SofQW" tab
  connect(m_uiForm.sqw_ckRebinE, SIGNAL(toggled(bool)), this, SLOT(sOfQwRebinE(bool)));
  connect(m_uiForm.sqw_cbInput, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(sOfQwInputType(const QString&)));
  connect(m_uiForm.sqw_pbRefresh, SIGNAL(clicked()), this, SLOT(refreshWSlist()));
  connect(m_uiForm.sqw_pbPlotInput, SIGNAL(clicked()), this, SLOT(sOfQwPlotInput()));

  // "Slice" tab
  connect(m_uiForm.slice_pbPlotRaw, SIGNAL(clicked()), this, SLOT(slicePlotRaw()));
  connect(m_uiForm.slice_ckUseCalib, SIGNAL(toggled(bool)), this, SLOT(sliceCalib(bool)));

  // create validators
  m_valInt = new QIntValidator(this);
  m_valDbl = new QDoubleValidator(this);

  // apply validators
  m_uiForm.leNoGroups->setValidator(m_valInt);
  m_uiForm.leDetailedBalance->setValidator(m_valInt);
  m_uiForm.leSpectraMin->setValidator(m_valInt);
  m_uiForm.leSpectraMax->setValidator(m_valInt);
  m_uiForm.rebin_leELow->setValidator(m_valDbl);
  m_uiForm.rebin_leEWidth->setValidator(m_valDbl);
  m_uiForm.rebin_leEHigh->setValidator(m_valDbl);
  
  m_uiForm.sqw_leELow->setValidator(m_valDbl);
  m_uiForm.sqw_leEWidth->setValidator(m_valDbl);
  m_uiForm.sqw_leEHigh->setValidator(m_valDbl);
  m_uiForm.sqw_leQLow->setValidator(m_valDbl);
  m_uiForm.sqw_leQWidth->setValidator(m_valDbl);
  m_uiForm.sqw_leQHigh->setValidator(m_valDbl);

  // set default values for save formats
  m_uiForm.save_ckSPE->setChecked(false);
  m_uiForm.save_ckNexus->setChecked(true);

  loadSettings();

  refreshWSlist();
}
/**
* This function will hold any Python-dependent setup actions for the interface.
*/
void Indirect::initLocalPython()
{
  // Nothing to do here at the moment.
}
/**
* This function opens a web browser window to the Mantid Project wiki page for this
* interface ("Inelastic" subsection of ConvertToEnergy).
*/
void Indirect::helpClicked()
{
  QString tabName = m_uiForm.tabWidget->tabText(m_uiForm.tabWidget->currentIndex());
  QString url = "http://www.mantidproject.org/Indirect:";
  if ( tabName == "Energy Transfer" )
    url += "EnergyTransfer";
  else if ( tabName == "Calibration" )
    url += "Calibration";
  else if ( tabName == "Diagnostics" )
    url += "Diagnostics";
  else if ( tabName == "S(Q, w)" )
    url += "SofQW";
  QDesktopServices::openUrl(QUrl(url));
}
/**
* This function will control the actions needed for the Indirect interface when the
* "Run" button is clicked by the user.
* @param tryToSave :: whether to try and save the output. Generally true, false when user has clicked on the "Rebin" button instead of "Run"
*/
void Indirect::runClicked()
{
  QString tabName = m_uiForm.tabWidget->tabText(m_uiForm.tabWidget->currentIndex());

  if ( tabName == "Energy Transfer" )
  {
    runConvertToEnergy();
  }
  else if ( tabName == "Calibration" )
  {
    calibCreate();
  }
  else if ( tabName == "Diagnostics" )
  {
    sliceRun();
  }
  else if ( tabName == "S(Q, w)" )
  {
    sOfQwClicked();
  }
}

void Indirect::runConvertToEnergy(bool tryToSave)
{
  if ( ! validateInput() )
  {
    showInformationBox("Please check the input highlighted in red.");
    return;
  }
  QString grouping = createMapFile(m_uiForm.cbMappingOptions->currentText());
  if ( grouping == "" )
  {
    return;
  }

  QString pyInput = "from mantidsimple import *\n"
    "import IndirectEnergyConversion as ind\n";

  if ( m_uiForm.ckVerbose->isChecked() ) pyInput += "verbose = True\n";
  else pyInput += "verbose = False\n";

  pyInput += "first = " +m_uiForm.leSpectraMin->text()+ "\n"
    "last = " +m_uiForm.leSpectraMax->text()+ "\n"
    "instrument = '" + m_uiForm.cbInst->currentText()+"'\n"
    "analyser = '"+m_uiForm.cbAnalyser->currentText()+"'\n"
    "reflection = '"+m_uiForm.cbReflection->currentText()+"'\n";
  
  QStringList runFiles_list = m_uiForm.ind_runFiles->getFilenames();
  QString runFiles = runFiles_list.join("', r'");

  pyInput += "rawfiles = [r'"+runFiles+"']\n"
    "Sum = ";
  if ( m_uiForm.ckSumFiles->isChecked() )
  {
    pyInput += "True\n";
  }
  else
  {
    pyInput += "False\n";
  }

  if ( m_bgRemoval )
  {
    QPair<double,double> bgRange = m_backgroundDialog->getRange();
    QString startTOF, endTOF;
    startTOF.setNum(bgRange.first, 'e');
    endTOF.setNum(bgRange.second, 'e');
    pyInput += "bgRemove = ["+startTOF+", "+endTOF+"]\n";
  }
  else
  {
    pyInput += "bgRemove = [0, 0]\n";
  }

  if ( m_uiForm.ckUseCalib->isChecked() )
  {
    QString calib = m_uiForm.ind_calibFile->getFirstFilename();
    pyInput += "calib = r'"+calib+"'\n";
  }
  else
  {
    pyInput += "calib = ''\n";
  }

  if ( ! m_uiForm.rebin_ckDNR->isChecked() )
  { 
    QString rebinParam = m_uiForm.rebin_leELow->text() + ","
      + m_uiForm.rebin_leEWidth->text() + ","
      + m_uiForm.rebin_leEHigh->text();
    pyInput += "rebinParam = '"+rebinParam+"'\n";
  }
  else
  {
    pyInput += "rebinParam = ''\n";
  }

  if ( m_uiForm.ckDetailedBalance->isChecked() )
    pyInput += "tempK = "+m_uiForm.leDetailedBalance->text()+"\n";
  else
    pyInput += "tempK = -1\n";

  pyInput += "mapfile = r'"+grouping+"'\n";

  if (tryToSave)
  {
    pyInput += savePyCode();
  }
  else
  {
    pyInput += "fileFormats = []\n";
  }

  if ( m_uiForm.ckCleanUp->isChecked() )
  {
    pyInput += "clean = False\n";
  }
  else
  {
    pyInput += "clean = True\n";
  }

  if ( isDirty() )
  {
    pyInput += "ws_list = ind.convert_to_energy(rawfiles, mapfile, first, last,"
      "instrument, analyser, reflection,"
      "SumFiles=Sum, bgremove=bgRemove, tempK=tempK, calib=calib, rebinParam=rebinParam,"
      "saveFormats=fileFormats, CleanUp=clean, Verbose=verbose)\n";
  }
  else if ( isDirtyRebin() )
  {
    pyInput += "ws_list = ind.cte_rebin(mapfile, tempK, rebinParam, analyser, reflection,"
      "fileFormats, CleanUp=clean, Verbose=verbose)\n";
  }
  else if ( tryToSave ) // where all we want to do is save and/or plot output
  {
    pyInput +=
      "import re\n"
      "wslist = mantid.getWorkspaceNames()\n"
      "save_ws = re.compile(r'_'+ana+ref+'_red')\n"
      "ws_list = []\n"
      "for workspace in wslist:\n"
      "   if save_ws.search(workspace):\n"
      "      ws_list.append(workspace)\n"
      "ind.saveItems(ws_list, fileFormats, Verbose=verbose)\n";
  }

  // Plot Output Handling
  switch ( m_uiForm.ind_cbPlotOutput->currentIndex() )
  {
  case 0: // "None"
    break;
  case 1: // "Spectra"
    {
      // Plot a spectra of the first result workspace
      pyInput += 
        "if ( len(ws_list) > 0 ):\n"
        "  nSpec = mtd[ws_list[0]].getNumberHistograms()\n"
        "  plotSpectrum(ws_list[0], range(0, nSpec))\n";
    }
    break;
  case 2: // "Contour"
    {
      // Plot a 2D Contour Lines of the first result workspace
      pyInput += 
        "if ( len(ws_list) > 0 ):\n"
        "  ws = importMatrixWorkspace(ws_list[0])\n"
        "  ws.plotGraph2D()\n";
    }
    break;
  }

  QString pyOutput = runPythonCode(pyInput).trimmed();

  if ( pyOutput != "" )
  {
    if ( pyOutput == "No intermediate workspaces were found. Run with 'Keep Intermediate Workspaces' checked." )
    {
      isDirty(true);
      runConvertToEnergy(tryToSave=tryToSave);
    }
    else
    {
      showInformationBox("The following error occurred:\n" + pyOutput + "\n\nAnalysis did not complete.");
    }
  }
  else
  {
    isDirty(false);
    isDirtyRebin(false);
  }

}
/**
* This function holds any steps that must be performed on the selection of an instrument,
* for example loading values from the Instrument Definition File (IDF).
* @param prefix :: The selected instruments prefix in Mantid.
*/
void Indirect::setIDFValues(const QString & prefix)
{
  // empty ComboBoxes, LineEdits,etc of previous values
  m_uiForm.cbAnalyser->clear();
  m_uiForm.cbReflection->clear();
  clearReflectionInfo();

  rebinCheck(m_uiForm.rebin_ckDNR->isChecked());
  detailedBalanceCheck(m_uiForm.ckDetailedBalance->isChecked());
  resCheck(m_uiForm.cal_ckRES->isChecked());

  // Get list of analysers and populate cbAnalyser
  QString pyInput = 
    "from IndirectEnergyConversion import getInstrumentDetails\n"
    "result = getInstrumentDetails('" + m_uiForm.cbInst->currentText() + "')\n"
    "print result\n";

  QString pyOutput = runPythonCode(pyInput).trimmed();

  if ( pyOutput == "" )
  {
    showInformationBox("Could not get list of analysers from Instrument Parameter file.");
  }
  else
  {
    QStringList analysers = pyOutput.split("\n", QString::SkipEmptyParts);

    for (int i = 0; i< analysers.count(); i++ )
    {
      QString text; // holds Text field of combo box (name of analyser)

      if ( text != "diffraction" ) // do not put diffraction into the analyser list
      {

        QVariant data; // holds Data field of combo box (list of reflections)

        QStringList analyser = analysers[i].split("-", QString::SkipEmptyParts);

        text = analyser[0];

        if ( analyser.count() > 1 )
        {
          QStringList reflections = analyser[1].split(",", QString::SkipEmptyParts);
          data = QVariant(reflections);
          m_uiForm.cbAnalyser->addItem(text, data);
        }
        else
        {
          m_uiForm.cbAnalyser->addItem(text);
        }
      }
    }

    analyserSelected(m_uiForm.cbAnalyser->currentIndex());
  }
}

void Indirect::closeEvent(QCloseEvent* close)
{
  (void) close;
  //saveSettings();
  Mantid::Kernel::ConfigService::Instance().removeObserver(m_changeObserver);
}

void Indirect::handleDirectoryChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf)
{
  std::string key = pNf->key();
  std::string preValue = pNf->preValue();
  std::string curValue = pNf->curValue();

  if ( key == "datasearch.directories" || key == "defaultsave.directory" )
  {
    loadSettings();
  }
}
/**
* This function clears the values of the QLineEdit objec  ts used to hold Reflection-specific
* information.
*/
void Indirect::clearReflectionInfo()
{
  m_uiForm.leSpectraMin->clear();
  m_uiForm.leSpectraMax->clear();
  m_uiForm.leEfixed->clear();
  
  isDirty(true);
}
/**
* This function creates the mapping/grouping file for the data analysis.
* @param groupType :: Type of grouping (All, Group, Indiviual)
* @return path to mapping file, or an empty string if file could not be created.
*/
QString Indirect::createMapFile(const QString& groupType)
{
  QString groupFile, ngroup, nspec;
  QString ndet = "( "+m_uiForm.leSpectraMax->text()+" - "+m_uiForm.leSpectraMin->text()+") + 1";

  if ( groupType == "File" )
  {
    groupFile = m_uiForm.ind_mapFile->getFirstFilename();
    if ( groupFile == "" )
    {
      showInformationBox("You must enter a path to the .map file.");
    }
    return groupFile;
  }
  else if ( groupType == "Groups" )
  {
    ngroup = m_uiForm.leNoGroups->text();
    nspec = "( " +ndet+ " ) / " +ngroup;
  }
  else if ( groupType == "All" )
  {
    return "All";
  }
  else if ( groupType == "Individual" )
  {
    return "Individual";
  }

  groupFile = m_uiForm.cbInst->itemData(m_uiForm.cbInst->currentIndex()).toString().toLower();
  groupFile += "_" + m_uiForm.cbAnalyser->currentText() + m_uiForm.cbReflection->currentText();
  groupFile += "_" + groupType + ".map";	

  QString pyInput =
    "import IndirectEnergyConversion as ind\n"
    "mapfile = ind.createMappingFile('"+groupFile+"', %1, %2, %3)\n"
    "print mapfile\n";
  pyInput = pyInput.arg(ngroup);
  pyInput = pyInput.arg(nspec);
  pyInput = pyInput.arg(m_uiForm.leSpectraMin->text());

  QString pyOutput = runPythonCode(pyInput).trimmed();

  return pyOutput;
}
/**
* This function creates the Python script necessary to set the variables used for saving data
* in the main convert_to_energy script.
* @return python code as a string
*/
QString Indirect::savePyCode()
{
  QStringList fileFormats;
  QString fileFormatList;

  if ( m_uiForm.save_ckNexus->isChecked() )
    fileFormats << "nxs";
  if ( m_uiForm.save_ckSPE->isChecked() )
    fileFormats << "spe";
  if ( m_uiForm.save_ckNxSPE->isChecked() )
    fileFormats << "nxspe";

  if ( fileFormats.size() != 0 )
    fileFormatList = "[ '" + fileFormats.join("', '") + "']";
  else
    fileFormatList = "[]";

  QString pyInput =
    "# Save File Parameters\n"
    "fileFormats = " + fileFormatList + "\n";

  return pyInput;
}
/**
* This function is called after calib has run and creates a RES file for use in later analysis (Fury,etc)
* @param file :: the input file (WBV run.raw)
*/
void Indirect::createRESfile(const QString& file)
{
  QString pyInput =
    "from IndirectEnergyConversion import resolution\n"
    "iconOpt = { 'first': " +QString::number(m_calDblMng->value(m_calResProp["SpecMin"]))+
    ", 'last': " +QString::number(m_calDblMng->value(m_calResProp["SpecMax"]))+"}\n"

    "instrument = '" + m_uiForm.cbInst->currentText() + "'\n"
    "analyser = '" + m_uiForm.cbAnalyser->currentText() + "'\n"
    "reflection = '" + m_uiForm.cbReflection->currentText() + "'\n";

  if ( m_uiForm.cal_ckPlotResult->isChecked() ) { pyInput +=	"plot = True\n"; }
  else { pyInput += "plot = False\n"; }

  QString rebinParam = QString::number(m_calDblMng->value(m_calResProp["ELow"])) + "," +
    QString::number(m_calDblMng->value(m_calResProp["EWidth"])) + "," +
    QString::number(m_calDblMng->value(m_calResProp["EHigh"]));

  QString background = "[ " +QString::number(m_calDblMng->value(m_calResProp["Start"]))+ ", " +QString::number(m_calDblMng->value(m_calResProp["End"]))+"]";

  pyInput +=
    "background = " + background + "\n"
    "rebinParam = '" + rebinParam + "'\n"
    "file = " + file + "\n"
    "resolution(file, iconOpt, rebinParam, background, instrument, analyser, reflection, plotOpt = plot)\n";

  QString pyOutput = runPythonCode(pyInput).trimmed();

  if ( pyOutput != "" )
  {
    showInformationBox("Unable to create RES file: \n" + pyOutput);
  }
}
/**
* This function validates the input for the Convert To Energy process, highlighting invalid items.
* @return true if input is ok, false otherwise
*/
bool Indirect::validateInput()
{
  bool valid = true;
  // run files input
  if ( ! m_uiForm.ind_runFiles->isValid() )
  {
    valid = false;
  }

  // calib file input

  if ( m_uiForm.ckUseCalib->isChecked() && !m_uiForm.ind_calibFile->isValid() )
  {
    valid = false;
  }

  // mapping selection
  if (
    ( m_uiForm.cbMappingOptions->currentText() == "Groups" && m_uiForm.leNoGroups->text() == "" ) 
    ||
    ( m_uiForm.cbMappingOptions->currentText() == "File" && ! m_uiForm.ind_mapFile->isValid() )
    )
  {
    valid = false;
    m_uiForm.valNoGroups->setText("*");
  }
  else
  {
    m_uiForm.valNoGroups->setText("");
  }

  // detailed balance
  if ( m_uiForm.ckDetailedBalance->isChecked() && m_uiForm.leDetailedBalance->text() == "" )
  {
    valid = false;
    m_uiForm.valDetailedBalance->setText("*");
  }
  else
  {
    m_uiForm.valDetailedBalance->setText("");
  }

  // SpectraMin/SpectraMax
  if (
    m_uiForm.leSpectraMin->text() == ""
    ||
    m_uiForm.leSpectraMax->text() == ""
    ||
    (
    m_uiForm.leSpectraMin->text().toDouble() > m_uiForm.leSpectraMax->text().toDouble()
    )
    )
  {
    valid = false;
    m_uiForm.valSpectraMin->setText("*");
    m_uiForm.valSpectraMax->setText("*");
  }
  else
  {
    m_uiForm.valSpectraMin->setText("");
    m_uiForm.valSpectraMax->setText("");
  }

  if ( ! m_uiForm.rebin_ckDNR->isChecked() )
  {
    //
    if ( m_uiForm.rebin_leELow->text() == "" )
    {
      valid = false;
      m_uiForm.valELow->setText("*");
    }
    else
    {
      m_uiForm.valELow->setText("");
    }

    if ( m_uiForm.rebin_leEWidth->text() == "" )
    {
      valid = false;
      m_uiForm.valEWidth->setText("*");
    }
    else
    {
      m_uiForm.valEWidth->setText("");
    }

    if ( m_uiForm.rebin_leEHigh->text() == "" )
    {
      valid = false;
      m_uiForm.valEHigh->setText("*");
    }
    else
    {
      m_uiForm.valEHigh->setText("");
    }

    if ( m_uiForm.rebin_leELow->text().toDouble() > m_uiForm.rebin_leEHigh->text().toDouble() )
    {
      valid = false;
      m_uiForm.valELow->setText("*");
      m_uiForm.valEHigh->setText("*");
    }

  }
  else
  {
    m_uiForm.valELow->setText("");
    m_uiForm.valEWidth->setText("");
    m_uiForm.valEHigh->setText("");
  }


  return valid;
}
/**
* Validates user input on the calibration tab.
*/
bool Indirect::validateCalib()
{
  bool valid = true;

  // run number
  if ( ! m_uiForm.cal_leRunNo->isValid() )
  {
    valid = false;
  }

  if ( m_uiForm.cal_ckRES->isChecked() )
  {
    if ( m_calDblMng->value(m_calResProp["Start"]) > m_calDblMng->value(m_calResProp["End"]) )
    {
      valid = false;
    }

    if ( m_calDblMng->value(m_calResProp["ELow"]) > m_calDblMng->value(m_calResProp["EHigh"]) )
    {
      valid = false;
    }

  }

  return valid;
}

bool Indirect::validateSofQw()
{
  bool valid = true;

  if ( m_uiForm.sqw_cbInput->currentText() == "File" )
  {
    if ( ! m_uiForm.sqw_inputFile->isValid() )
    {
      valid = false;
    }
  }
  else
  {
    if ( m_uiForm.sqw_cbWorkspace->currentText().isEmpty() )
    {
      valid = false;
      m_uiForm.sqw_valWorkspace->setText("*");
    }
    else
    {
      m_uiForm.sqw_valWorkspace->setText(" ");
    }
  }

  if ( m_uiForm.sqw_ckRebinE->isChecked() )
  {
    if ( m_uiForm.sqw_leELow->text() == "" )
    {
      valid = false;
      m_uiForm.sqw_valELow->setText("*");
    }
    else
    {
      m_uiForm.sqw_valELow->setText(" ");
    }

    if ( m_uiForm.sqw_leEWidth->text() == "" )
    {
      valid = false;
      m_uiForm.sqw_valEWidth->setText("*");
    }
    else
    {
      m_uiForm.sqw_valEWidth->setText(" ");
    }
    if ( m_uiForm.sqw_leEHigh->text() == "" )
    {
      valid = false;
      m_uiForm.sqw_valEHigh->setText("*");
    }
    else
    {
      m_uiForm.sqw_valEHigh->setText(" ");
    }
  }

  if ( m_uiForm.sqw_leQLow->text() == "" )
  {
    valid = false;
    m_uiForm.sqw_valQLow->setText("*");
  }
  else
  {
    m_uiForm.sqw_valQLow->setText(" ");
  }

  if ( m_uiForm.sqw_leQWidth->text() == "" )
  {
    valid = false;
    m_uiForm.sqw_valQWidth->setText("*");
  }
  else
  {
    m_uiForm.sqw_valQWidth->setText(" ");
  }
  if ( m_uiForm.sqw_leQHigh->text() == "" )
  {
    valid = false;
    m_uiForm.sqw_valQHigh->setText("*");
  }
  else
  {
    m_uiForm.sqw_valQHigh->setText(" ");
  }
  return valid;
}

bool Indirect::validateSlice()
{
  bool valid = true;

  if ( ! m_uiForm.slice_inputFile->isValid() )
  {
    valid = false;
  }

  return valid;
}
/**
* Used to check whether any changes have been made by the user to the interface.
* @return boolean m_isDirty
*/
bool Indirect::isDirty()
{
  return m_isDirty;
}
/**
* Used to set value of m_isDirty, called from each function that signifies a change in the user interface.
* Will be set to false in functions that use the input.
* @param state :: whether to set the value to true or false.
*/
void Indirect::isDirty(bool state)
{
  m_isDirty = state;
}
/**
* Used to check whether any changes have been made by the user to the interface.
* @return boolean m_isDirtyRebin
*/
bool Indirect::isDirtyRebin()
{
  return m_isDirtyRebin;
}
/**
* Used to set value of m_isDirtyRebin, called from each function that signifies a change in the user interface.
* Will be set to false in functions that use the input.
* @param state :: whether to set the value to true or false.
*/
void Indirect::isDirtyRebin(bool state)
{
  m_isDirtyRebin = state;
}

void Indirect::loadSettings()
{
  
  // set values of m_dataDir and m_saveDir
  m_dataDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("datasearch.directories"));
  m_dataDir = m_dataDir.split(";", QString::SkipEmptyParts)[0];
  m_saveDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory"));


  QSettings settings;
  // Load settings for MWRunFile widgets
  settings.beginGroup(m_settingsGroup + "DataFiles");
  settings.setValue("last_directory", m_dataDir);
  m_uiForm.ind_runFiles->readSettings(settings.group());
  m_uiForm.cal_leRunNo->readSettings(settings.group());
  m_uiForm.slice_inputFile->readSettings(settings.group());
  settings.endGroup();

  settings.beginGroup(m_settingsGroup + "ProcessedFiles");
  settings.setValue("last_directory", m_saveDir);
  m_uiForm.ind_calibFile->readSettings(settings.group());
  m_uiForm.ind_mapFile->readSettings(settings.group());
  m_uiForm.slice_calibFile->readSettings(settings.group());
  m_uiForm.sqw_inputFile->readSettings(settings.group());
  settings.endGroup();

  // And for instrument/analyser/reflection
}

void Indirect::saveSettings()
{
  // The only settings that we want to keep are the instrument / analyser / reflection ones
  // Instrument is handled in ConvertToEnergy class
}

void Indirect::setupCalibration()
{
  int noDec = 6;
  // General
  m_calDblMng = new QtDoublePropertyManager();
  m_calGrpMng = new QtGroupPropertyManager();

  /* Calib */
  m_calCalTree = new QtTreePropertyBrowser();
  m_uiForm.cal_treeCal->addWidget(m_calCalTree);

  DoubleEditorFactory *doubleEditorFactory = new DoubleEditorFactory();
  m_calCalTree->setFactoryForManager(m_calDblMng, doubleEditorFactory);

  m_calCalProp["PeakMin"] = m_calDblMng->addProperty("Peak Min");
  m_calCalProp["PeakMax"] = m_calDblMng->addProperty("Peak Max");
  m_calCalProp["BackMin"] = m_calDblMng->addProperty("Back Min");
  m_calCalProp["BackMax"] = m_calDblMng->addProperty("Back Max");

  m_calCalTree->addProperty(m_calCalProp["PeakMin"]);
  m_calCalTree->addProperty(m_calCalProp["PeakMax"]);
  m_calCalTree->addProperty(m_calCalProp["BackMin"]);
  m_calCalTree->addProperty(m_calCalProp["BackMax"]);

  m_calCalPlot = new QwtPlot(this);
  m_calCalPlot->setAxisFont(QwtPlot::xBottom, this->font());
  m_calCalPlot->setAxisFont(QwtPlot::yLeft, this->font());
  m_uiForm.cal_plotCal->addWidget(m_calCalPlot);
  m_calCalPlot->setCanvasBackground(Qt::white);

  // R1 = Peak, R2 = Background
  m_calCalR1 = new MantidWidgets::RangeSelector(m_calCalPlot);
  connect(m_calCalR1, SIGNAL(minValueChanged(double)), this, SLOT(calMinChanged(double)));
  connect(m_calCalR1, SIGNAL(maxValueChanged(double)), this, SLOT(calMaxChanged(double)));
  m_calCalR2 = new MantidWidgets::RangeSelector(m_calCalPlot);
  m_calCalR2->setColour(Qt::darkGreen); // dark green to signify background range
  connect(m_calCalR2, SIGNAL(minValueChanged(double)), this, SLOT(calMinChanged(double)));
  connect(m_calCalR2, SIGNAL(maxValueChanged(double)), this, SLOT(calMaxChanged(double)));

  // Res
  m_calResTree = new QtTreePropertyBrowser();
  m_uiForm.cal_treeRes->addWidget(m_calResTree);

  m_calResTree->setFactoryForManager(m_calDblMng, doubleEditorFactory);

  // Res - Spectra Selection
  m_calResProp["SpecMin"] = m_calDblMng->addProperty("Spectra Min");
  m_calResProp["SpecMax"] = m_calDblMng->addProperty("Spectra Max");
  m_calResTree->addProperty(m_calResProp["SpecMin"]);
  m_calDblMng->setDecimals(m_calResProp["SpecMin"], 0);
  m_calResTree->addProperty(m_calResProp["SpecMax"]);
  m_calDblMng->setDecimals(m_calResProp["SpecMax"], 0);

  // Res - Background Properties
  QtProperty* resBG = m_calGrpMng->addProperty("Background");
  m_calResProp["Start"] = m_calDblMng->addProperty("Start");
  m_calResProp["End"] = m_calDblMng->addProperty("End");
  resBG->addSubProperty(m_calResProp["Start"]);
  resBG->addSubProperty(m_calResProp["End"]);
  m_calResTree->addProperty(resBG);

  // Res - rebinning
  QtProperty* resRB = m_calGrpMng->addProperty("Rebinning");
  m_calResProp["ELow"] = m_calDblMng->addProperty("Low");
  m_calDblMng->setDecimals(m_calResProp["ELow"], noDec);
  m_calDblMng->setValue(m_calResProp["ELow"], -0.2);
  m_calResProp["EWidth"] = m_calDblMng->addProperty("Width");
  m_calDblMng->setDecimals(m_calResProp["EWidth"], noDec);
  m_calDblMng->setValue(m_calResProp["EWidth"], 0.002);
  m_calResProp["EHigh"] = m_calDblMng->addProperty("High");
  m_calDblMng->setDecimals(m_calResProp["EHigh"], noDec);
  m_calDblMng->setValue(m_calResProp["EHigh"], 0.2);
  resRB->addSubProperty(m_calResProp["ELow"]);
  resRB->addSubProperty(m_calResProp["EWidth"]);
  resRB->addSubProperty(m_calResProp["EHigh"]);
  m_calResTree->addProperty(resRB);

  m_calResPlot = new QwtPlot(this);
  m_calResPlot->setAxisFont(QwtPlot::xBottom, this->font());
  m_calResPlot->setAxisFont(QwtPlot::yLeft, this->font());
  m_uiForm.cal_plotRes->addWidget(m_calResPlot);
  m_calResPlot->setCanvasBackground(Qt::white);

  // Create ResR2 first so ResR1 is drawn above it.
  m_calResR2 = new MantidWidgets::RangeSelector(m_calResPlot, 
    MantidQt::MantidWidgets::RangeSelector::XMINMAX, true, true);
  m_calResR2->setColour(Qt::darkGreen);
  m_calResR1 = new MantidWidgets::RangeSelector(m_calResPlot);

  connect(m_calResR1, SIGNAL(minValueChanged(double)), this, SLOT(calMinChanged(double)));
  connect(m_calResR1, SIGNAL(maxValueChanged(double)), this, SLOT(calMaxChanged(double)));
  connect(m_calResR1, SIGNAL(rangeChanged(double, double)), m_calResR2, SLOT(setRange(double, double)));
  connect(m_calDblMng, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(calUpdateRS(QtProperty*, double)));
  connect(m_calDblMng, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(calUpdateRS(QtProperty*, double)));
  connect(m_calDblMng, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(calUpdateRS(QtProperty*, double)));
}

void Indirect::setupSlice()
{
  // Property Tree
  m_sltTree = new QtTreePropertyBrowser();
  m_uiForm.slice_properties->addWidget(m_sltTree);

  // Create Manager Objects
  m_sltDblMng = new QtDoublePropertyManager();
  m_sltBlnMng = new QtBoolPropertyManager();
  m_sltGrpMng = new QtGroupPropertyManager();

  // Editor Factories
  DoubleEditorFactory *doubleEditorFactory = new DoubleEditorFactory();
  QtCheckBoxFactory *checkboxFactory = new QtCheckBoxFactory();
  m_sltTree->setFactoryForManager(m_sltDblMng, doubleEditorFactory);
  m_sltTree->setFactoryForManager(m_sltBlnMng, checkboxFactory);

  // Create Properties
  m_sltProp["SpecMin"] = m_sltDblMng->addProperty("Spectra Min");
  m_sltProp["SpecMax"] = m_sltDblMng->addProperty("Spectra Max");
  m_sltDblMng->setDecimals(m_sltProp["SpecMin"], 0);
  m_sltDblMng->setDecimals(m_sltProp["SpecMax"], 0);

  m_sltProp["R1S"] = m_sltDblMng->addProperty("Start");
  m_sltProp["R1E"] = m_sltDblMng->addProperty("End");
  m_sltProp["R2S"] = m_sltDblMng->addProperty("Start");
  m_sltProp["R2E"] = m_sltDblMng->addProperty("End");

  m_sltProp["UseTwoRanges"] = m_sltBlnMng->addProperty("Use Two Ranges");

  m_sltProp["Range1"] = m_sltGrpMng->addProperty("Range One");
  m_sltProp["Range1"]->addSubProperty(m_sltProp["R1S"]);
  m_sltProp["Range1"]->addSubProperty(m_sltProp["R1E"]);
  m_sltProp["Range2"] = m_sltGrpMng->addProperty("Range Two");
  m_sltProp["Range2"]->addSubProperty(m_sltProp["R2S"]);
  m_sltProp["Range2"]->addSubProperty(m_sltProp["R2E"]);

  m_sltTree->addProperty(m_sltProp["SpecMin"]);
  m_sltTree->addProperty(m_sltProp["SpecMax"]);
  m_sltTree->addProperty(m_sltProp["Range1"]);
  m_sltTree->addProperty(m_sltProp["UseTwoRanges"]);
  m_sltTree->addProperty(m_sltProp["Range2"]);

  // Create Slice Plot Widget for Range Selection
  m_sltPlot = new QwtPlot(this);
  m_sltPlot->setAxisFont(QwtPlot::xBottom, this->font());
  m_sltPlot->setAxisFont(QwtPlot::yLeft, this->font());
  m_uiForm.slice_plot->addWidget(m_sltPlot);
  m_sltPlot->setCanvasBackground(Qt::white);
  // We always want one range selector... the second one can be controlled from
  // within the sliceTwoRanges(bool state) function
  m_sltR1 = new MantidWidgets::RangeSelector(m_sltPlot);
  connect(m_sltR1, SIGNAL(minValueChanged(double)), this, SLOT(sliceMinChanged(double)));
  connect(m_sltR1, SIGNAL(maxValueChanged(double)), this, SLOT(sliceMaxChanged(double)));

  // second range
  // create the second range
  m_sltR2 = new MantidWidgets::RangeSelector(m_sltPlot);
  m_sltR2->setColour(Qt::darkGreen); // dark green for background
  connect(m_sltR1, SIGNAL(rangeChanged(double, double)), m_sltR2, SLOT(setRange(double, double)));
  connect(m_sltR2, SIGNAL(minValueChanged(double)), this, SLOT(sliceMinChanged(double)));
  connect(m_sltR2, SIGNAL(maxValueChanged(double)), this, SLOT(sliceMaxChanged(double)));
  m_sltR2->setRange(m_sltR1->getRange());

  // Refresh the plot window
  m_sltPlot->replot();

  connect(m_sltDblMng, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(sliceUpdateRS(QtProperty*, double)));
  connect(m_sltBlnMng, SIGNAL(valueChanged(QtProperty*, bool)), this, SLOT(sliceTwoRanges(QtProperty*, bool)));

  sliceTwoRanges(0, false); // set default value
}

void Indirect::refreshWSlist()
{
  m_uiForm.sqw_cbWorkspace->clear();
  std::set<std::string> workspaceList = Mantid::API::AnalysisDataService::Instance().getObjectNames();
  if ( ! workspaceList.empty() )
  {
    std::set<std::string>::const_iterator wsIt;
    for ( wsIt=workspaceList.begin(); wsIt != workspaceList.end(); ++wsIt )
    {
      m_uiForm.sqw_cbWorkspace->addItem(QString::fromStdString(*wsIt));
    }
  }
}
/**
* This function is called when the user selects an analyser from the cbAnalyser QComboBox
* object. It's main purpose is to initialise the values for the Reflection ComboBox.
* @param index :: Index of the value selected in the combo box.
*/
void Indirect::analyserSelected(int index)
{
  // populate Reflection combobox with correct values for Analyser selected.
  m_uiForm.cbReflection->clear();
  clearReflectionInfo();


  QVariant currentData = m_uiForm.cbAnalyser->itemData(index);
  if ( currentData == QVariant::Invalid )
  {
    m_uiForm.lbReflection->setEnabled(false);
    m_uiForm.cbReflection->setEnabled(false);
    return;
  }
  else
  {
    m_uiForm.lbReflection->setEnabled(true);
    m_uiForm.cbReflection->setEnabled(true);
    QStringList reflections = currentData.toStringList();
    for ( int i = 0; i < reflections.count(); i++ )
    {
      m_uiForm.cbReflection->addItem(reflections[i]);
    }
  }

  reflectionSelected(m_uiForm.cbReflection->currentIndex());
}
/**
* This function is called when the user selects a reflection from the cbReflection QComboBox
* object.
* @param index :: Index of the value selected in the combo box.
*/
void Indirect::reflectionSelected(int index)
{
  // first, clear values in assosciated boxes:
  clearReflectionInfo();

  QString pyInput =
    "from IndirectEnergyConversion import getReflectionDetails\n"
    "instrument = '" + m_uiForm.cbInst->currentText() + "'\n"
    "analyser = '" + m_uiForm.cbAnalyser->currentText() + "'\n"
    "reflection = '" + m_uiForm.cbReflection->currentText() + "'\n"
    "print getReflectionDetails(instrument, analyser, reflection)\n";

  QString pyOutput = runPythonCode(pyInput).trimmed();

  QStringList values = pyOutput.split("\n", QString::SkipEmptyParts);

  if ( values.count() < 3 )
  {
    showInformationBox("Could not gather necessary data from parameter file.");
    return;
  }
  else
  {
    QString analysisType = values[0];
    m_uiForm.leSpectraMin->setText(values[1]);
    m_uiForm.leSpectraMax->setText(values[2]);
    m_calDblMng->setValue(m_calResProp["SpecMin"], values[1].toDouble());
    m_calDblMng->setValue(m_calResProp["SpecMax"], values[2].toDouble());
    m_sltDblMng->setValue(m_sltProp["SpecMin"], values[1].toDouble());
    m_sltDblMng->setValue(m_sltProp["SpecMax"], values[2].toDouble());
    
    if ( values.count() >= 8 )
    {
      m_uiForm.leEfixed->setText(values[3]);
      
      m_calDblMng->setValue(m_calCalProp["PeakMin"], values[4].toDouble());
      m_calDblMng->setValue(m_calCalProp["PeakMax"], values[5].toDouble());
      m_calDblMng->setValue(m_calCalProp["BackMin"], values[6].toDouble());
      m_calDblMng->setValue(m_calCalProp["BackMax"], values[7].toDouble());

      m_sltDblMng->setValue(m_sltProp["R1S"], values[4].toDouble());
      m_sltDblMng->setValue(m_sltProp["R1E"], values[5].toDouble());
      m_sltDblMng->setValue(m_sltProp["R2S"], values[6].toDouble());
      m_sltDblMng->setValue(m_sltProp["R2E"], values[7].toDouble());
    }
    else
    {
      m_uiForm.leEfixed->clear();
    }
    // Default rebinning parameters can be set in instrument parameter file
    if ( values.count() == 9 )
    {
      QStringList rbp = values[8].split(",", QString::SkipEmptyParts);
      m_uiForm.rebin_ckDNR->setChecked(false);
      m_uiForm.rebin_leELow->setText(rbp[0]);
      m_uiForm.rebin_leEWidth->setText(rbp[1]);
      m_uiForm.rebin_leEHigh->setText(rbp[2]);
    }
    else
    {
      m_uiForm.rebin_ckDNR->setChecked(true);
      m_uiForm.rebin_leELow->setText("");
      m_uiForm.rebin_leEWidth->setText("");
      m_uiForm.rebin_leEHigh->setText("");
    }
  }

  // clear validation markers
  validateInput();
  validateCalib();
  validateSlice();
}
/**
* This function runs when the user makes a selection on the cbMappingOptions QComboBox.
* @param groupType :: Value of selection made by user.
*/
void Indirect::mappingOptionSelected(const QString& groupType)
{
  if ( groupType == "File" )
  {
    m_uiForm.swMapping->setCurrentIndex(0);
  }
  else if ( groupType == "All" )
  {
    m_uiForm.swMapping->setCurrentIndex(2);
  }
  else if ( groupType == "Individual" )
  {
    m_uiForm.swMapping->setCurrentIndex(2);
  }
  else if ( groupType == "Groups" )
  {
    m_uiForm.swMapping->setCurrentIndex(1);
  }

  isDirtyRebin(true);
}

void Indirect::tabChanged(int index)
{
  QString tabName = m_uiForm.tabWidget->tabText(index);
  m_uiForm.pbRun->setText("Run " + tabName);
}
/**
* This function is called when the user clicks on the Background Removal button. It
* displays the Background Removal dialog, initialising it if it hasn't been already.
*/
void Indirect::backgroundClicked()
{
  if ( m_backgroundDialog == NULL )
  {
    m_backgroundDialog = new Background(this);
    connect(m_backgroundDialog, SIGNAL(accepted()), this, SLOT(backgroundRemoval()));
    connect(m_backgroundDialog, SIGNAL(rejected()), this, SLOT(backgroundRemoval()));
    m_backgroundDialog->show();
  }
  else
  {
    m_backgroundDialog->show();
  }
}
/**
* Slot called when m_backgroundDialog is closed. Assesses whether user desires background removal.
*/
void Indirect::backgroundRemoval()
{
  if ( m_backgroundDialog->removeBackground() )
  {
    m_bgRemoval = true;
    m_uiForm.pbBack_2->setText("Background Removal (On)");
  }
  else
  {
    m_bgRemoval = false;
    m_uiForm.pbBack_2->setText("Background Removal (Off)");
  }
  isDirty(true);
}
/**
* Plots raw time data from .raw file before any data conversion has been performed.
*/
void Indirect::plotRaw()
{
  if ( m_uiForm.ind_runFiles->isValid() )
  {
    bool ok;
    QString spectraRange = QInputDialog::getText(this, "Insert Spectra Ranges", "Range: ", QLineEdit::Normal, m_uiForm.leSpectraMin->text() +"-"+ m_uiForm.leSpectraMax->text(), &ok);

    if ( !ok || spectraRange.isEmpty() )
    {
      return;
    }
    QStringList specList = spectraRange.split("-");

    QString rawFile = m_uiForm.ind_runFiles->getFirstFilename();
    if ( (specList.size() > 2) || ( specList.size() < 1) )
    {
      showInformationBox("Invalid input. Must be of form <SpecMin>-<SpecMax>");
      return;
    }
    if ( specList.size() == 1 )
    {
      specList.append(specList[0]);
    }

    QString bgrange;

    if ( m_bgRemoval )
    {
      QPair<double, double> range = m_backgroundDialog->getRange();
      bgrange = "[ " + QString::number(range.first) + "," + QString::number(range.second) + " ]";
    }
    else
    {
      bgrange = "[-1, -1]";
    }

    QString pyInput =
      "from mantidsimple import *\n"
      "from mantidplot import *\n"
      "import os.path as op\n"
      "file = r'" + rawFile + "'\n"
      "name = op.splitext( op.split(file)[1] )[0]\n"
      "bgrange = " + bgrange + "\n"
      "LoadRaw(file, name, SpectrumMin="+specList[0]+", SpectrumMax="+specList[1]+")\n"
      "if ( bgrange != [-1, -1] ):\n"
      "    #Remove background\n"
      "    FlatBackground(name, name+'_bg', bgrange[0], bgrange[1], Mode='Mean')\n"
      "    GroupDetectors(name+'_bg', name+'_grp', DetectorList=range("+specList[0]+","+specList[1]+"+1))\n"
      "    GroupDetectors(name, name+'_grp_raw', DetectorList=range("+specList[0]+","+specList[1]+"+1))\n"
      "else: # Just group detectors as they are\n"
      "    GroupDetectors(name, name+'_grp', DetectorList=range("+specList[0]+","+specList[1]+"+1))\n"
      "graph = plotSpectrum(name+'_grp', 0)\n";

    QString pyOutput = runPythonCode(pyInput).trimmed();
    
    if ( pyOutput != "" )
    {
      showInformationBox(pyOutput);
    }

  }
  else
  {
    showInformationBox("You must select a run file.");
  }
}
/**
* This function will disable the necessary elements of the interface when the user selects "Do Not Rebin"
* and enable them again when this is de-selected.
* @param state :: whether the "Do Not Rebin" checkbox is checked
*/
void Indirect::rebinCheck(bool state) 
{
  QString val;
  if ( state ) val = " ";
  else val = "*";
  m_uiForm.rebin_pbRebin->setEnabled( !state );
  m_uiForm.rebin_lbLow->setEnabled( !state );
  m_uiForm.rebin_lbWidth->setEnabled( !state );
  m_uiForm.rebin_lbHigh->setEnabled( !state );
  m_uiForm.rebin_leELow->setEnabled( !state );
  m_uiForm.rebin_leEWidth->setEnabled( !state );
  m_uiForm.rebin_leEHigh->setEnabled( !state );
  m_uiForm.valELow->setEnabled(!state);
  m_uiForm.valELow->setText(val);
  m_uiForm.valEWidth->setEnabled(!state);
  m_uiForm.valEWidth->setText(val);
  m_uiForm.valEHigh->setEnabled(!state);
  m_uiForm.valEHigh->setText(val);
  isDirtyRebin(true);
}
/**
* Disables/enables the relevant parts of the UI when user checks/unchecks the Detailed Balance
* ckDetailedBalance checkbox.
* @param state :: state of the checkbox
*/
void Indirect::detailedBalanceCheck(bool state)
{
  m_uiForm.leDetailedBalance->setEnabled(state);
  m_uiForm.lbDBKelvin->setEnabled(state);

  isDirtyRebin(true);
}
/**
* This function enables/disables the display of the options involved in creating the RES file.
* @param state :: whether checkbox is checked or unchecked
*/
void Indirect::resCheck(bool state)
{
  m_calResR1->setVisible(state);
  m_calResR2->setVisible(state);
}
/**
* This function just calls the runClicked slot, but with tryToSave being 'false'
*/
void Indirect::rebinData()
{
  runConvertToEnergy(false);
}

void Indirect::useCalib(bool state)
{
  m_uiForm.ind_calibFile->isOptional(!state);
  m_uiForm.ind_calibFile->setEnabled(state);
}
/**
* This function is called when the user clicks on the "Create Calibration File" button.
* Pretty much does what it says on the tin.
*/
void Indirect::calibCreate()
{
  QString file = m_uiForm.cal_leRunNo->getFirstFilename();
  if ( ! validateCalib() || file == "" )
  {
    showInformationBox("Please check your input.");
  }
  else
  {
    QString suffix = "_" + m_uiForm.cbAnalyser->currentText() + m_uiForm.cbReflection->currentText() + "_calib";

    QString filenames = "[r'"+m_uiForm.cal_leRunNo->getFilenames().join("', r'")+"']";

    QString pyInput =
      "from IndirectEnergyConversion import createCalibFile\n"
      "plot = ";

    if ( m_uiForm.cal_ckPlotResult->isChecked() )
      pyInput +=	"True\n";
    else
      pyInput += "False\n";

    pyInput +=
      "file = createCalibFile("+filenames+", '"+suffix+"', %1, %2, %3, %4, %5, %6, PlotOpt=plot)\n"
      "print file\n";

    pyInput = pyInput.arg(QString::number(m_calDblMng->value(m_calCalProp["PeakMin"])));
    pyInput = pyInput.arg(QString::number(m_calDblMng->value(m_calCalProp["PeakMax"])));
    pyInput = pyInput.arg(QString::number(m_calDblMng->value(m_calCalProp["BackMin"])));
    pyInput = pyInput.arg(QString::number(m_calDblMng->value(m_calCalProp["BackMax"])));
    pyInput = pyInput.arg(m_uiForm.leSpectraMin->text());
    pyInput = pyInput.arg(m_uiForm.leSpectraMax->text());

    QString pyOutput = runPythonCode(pyInput).trimmed();

    if ( pyOutput == "" )
    {
      showInformationBox("An error occurred creating the calib file.\n");
    }
    else
    {
      if ( m_uiForm.cal_ckRES->isChecked() )
      {
        createRESfile(filenames);
      }
      m_uiForm.ind_calibFile->setFileText(pyOutput);
      m_uiForm.ckUseCalib->setChecked(true);
    }
  }
}
/**
* Sets interface as "Dirty" - catches all relevant user changes that don't need special action
*/
void Indirect::setasDirty()
{
  isDirty(true);
}
/*
* Sets interface as "Dirty" - catches all relevant user changes that don't need special action
*/
void Indirect::setasDirtyRebin()
{
  isDirtyRebin(true);
}
/**
* Controls the ckUseCalib checkbox to automatically check it when a user inputs a file from clicking on 'browse'.
* @param calib :: path to calib file
*/
void Indirect::calibFileChanged(const QString & calib)
{
  if ( calib.isEmpty() )
  {
    m_uiForm.ckUseCalib->setChecked(false);
  }
  else
  {
    m_uiForm.ckUseCalib->setChecked(true);
  }
}
// CALIBRATION TAB
void Indirect::calPlotRaw()
{
  QString filename = m_uiForm.cal_leRunNo->getFirstFilename();
  
  if ( filename == "" )
  {
    showInformationBox("Please enter a run number.");
    return;
  }
    
  QFileInfo fi(filename);
  QString wsname = fi.baseName();

  QString pyInput = "LoadRaw(r'" + filename + "', '" + wsname + "', SpectrumMin=" 
    + m_uiForm.leSpectraMin->text() + ", SpectrumMax="
    + m_uiForm.leSpectraMax->text() + ")\n";
  QString pyOutput = runPythonCode(pyInput);
    
  Mantid::API::MatrixWorkspace_sptr input = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(wsname.toStdString()));

  QVector<double> dataX = QVector<double>::fromStdVector(input->readX(0));
  QVector<double> dataY = QVector<double>::fromStdVector(input->readY(0));

  if ( m_calCalCurve != NULL )
  {
    m_calCalCurve->attach(0);
    delete m_calCalCurve;
    m_calCalCurve = 0;
  }

  m_calCalCurve = new QwtPlotCurve();
  m_calCalCurve->setData(dataX, dataY);
  m_calCalCurve->attach(m_calCalPlot);
  
  m_calCalPlot->setAxisScale(QwtPlot::xBottom, dataX.first(), dataX.last());

  m_calCalR1->setRange(dataX.first(), dataX.last());
  m_calCalR2->setRange(dataX.first(), dataX.last());

  // Replot
  m_calCalPlot->replot();

}

void Indirect::calPlotEnergy()
{
  if ( ! m_uiForm.cal_leRunNo->isValid() )
  {
    showInformationBox("Run number not valid.");
    return;
  }
  QString files = "[r'" + m_uiForm.cal_leRunNo->getFilenames().join("', r'") + "']";
  QString pyInput =
    "from IndirectEnergyConversion import resolution\n"
    "iconOpt = { 'first': " +QString::number(m_calDblMng->value(m_calResProp["SpecMin"]))+
    ", 'last': " +QString::number(m_calDblMng->value(m_calResProp["SpecMax"]))+ "}\n"
    "instrument = '" + m_uiForm.cbInst->currentText() + "'\n"
    "analyser = '" + m_uiForm.cbAnalyser->currentText() + "'\n"
    "reflection = '" + m_uiForm.cbReflection->currentText() + "'\n"
    "files = " + files + "\n"
    "outWS = resolution(files, iconOpt, '', '', instrument, analyser, reflection, Res=False)\n"
    "print outWS\n";
  QString pyOutput = runPythonCode(pyInput).trimmed();
  
  Mantid::API::MatrixWorkspace_sptr input = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(pyOutput.toStdString()));

  QVector<double> dataX = QVector<double>::fromStdVector(input->readX(0));
  QVector<double> dataY = QVector<double>::fromStdVector(input->readY(0));

  if ( m_calResCurve != NULL )
  {
    m_calResCurve->attach(0);
    delete m_calResCurve;
    m_calResCurve = 0;
  }

  m_calResCurve = new QwtPlotCurve();
  m_calResCurve->setData(dataX, dataY);
  m_calResCurve->attach(m_calResPlot);
  
  m_calResPlot->setAxisScale(QwtPlot::xBottom, dataX.first(), dataX.last());
  m_calResR1->setRange(dataX.first(), dataX.last());

  m_calResR2->setMinimum(m_calDblMng->value(m_calResProp["ELow"]));
  m_calResR2->setMaximum(m_calDblMng->value(m_calResProp["EHigh"]));

  // Replot
  m_calResPlot->replot();
}

void Indirect::calMinChanged(double val)
{
  MantidWidgets::RangeSelector* from = qobject_cast<MantidWidgets::RangeSelector*>(sender());
  if ( from == m_calCalR1 )
  {
    m_calDblMng->setValue(m_calCalProp["PeakMin"], val);
  }
  else if ( from == m_calCalR2 )
  {
    m_calDblMng->setValue(m_calCalProp["BackMin"], val);
  }
  else if ( from == m_calResR1 )
  {
    m_calDblMng->setValue(m_calResProp["Start"], val);
  }
}

void Indirect::calMaxChanged(double val)
{
  MantidWidgets::RangeSelector* from = qobject_cast<MantidWidgets::RangeSelector*>(sender());
  if ( from == m_calCalR1 )
  {
    m_calDblMng->setValue(m_calCalProp["PeakMax"], val);
  }
  else if ( from == m_calCalR2 )
  {
    m_calDblMng->setValue(m_calCalProp["BackMax"], val);
  }
  else if ( from == m_calResR1 )
  {
    m_calDblMng->setValue(m_calResProp["End"], val);
  }
}

void Indirect::calUpdateRS(QtProperty* prop, double val)
{
  if ( prop == m_calCalProp["PeakMin"] ) m_calCalR1->setMinimum(val);
  else if ( prop == m_calCalProp["PeakMax"] ) m_calCalR1->setMaximum(val);
  else if ( prop == m_calCalProp["BackMin"] ) m_calCalR2->setMinimum(val);
  else if ( prop == m_calCalProp["BackMax"] ) m_calCalR2->setMaximum(val);
  else if ( prop == m_calResProp["Start"] ) m_calResR1->setMinimum(val);
  else if ( prop == m_calResProp["End"] ) m_calResR1->setMaximum(val);
  else if ( prop == m_calResProp["ELow"] ) m_calResR2->setMinimum(val);
  else if ( prop == m_calResProp["EHigh"] ) m_calResR2->setMaximum(val);
}

void Indirect::sOfQwClicked()
{
  if ( validateSofQw() )
  {
    QString rebinString = m_uiForm.sqw_leQLow->text()+","+m_uiForm.sqw_leQWidth->text()+","+m_uiForm.sqw_leQHigh->text();
    QString pyInput = "from mantidsimple import *\n";

    if ( m_uiForm.sqw_cbInput->currentText() == "File" )
    {
      pyInput +=
        "filename = r'" +m_uiForm.sqw_inputFile->getFirstFilename() + "'\n"
        "(dir, file) = os.path.split(filename)\n"
        "(sqwInput, ext) = os.path.splitext(file)\n"
        "LoadNexus(filename, sqwInput)\n"
        "cleanup = True\n"; 
    }
    else
    {
      pyInput +=
        "sqwInput = '" + m_uiForm.sqw_cbWorkspace->currentText() + "'\n"
        "cleanup = False\n";
    }

    // Create output name before rebinning
    pyInput += "sqwOutput = sqwInput[:-3] + 'sqw'\n";

    if ( m_uiForm.sqw_ckRebinE->isChecked() )
    {
      QString eRebinString = m_uiForm.sqw_leELow->text()+","+m_uiForm.sqw_leEWidth->text()+","+m_uiForm.sqw_leEHigh->text();
      pyInput += "Rebin(sqwInput, sqwInput+'_r', '" + eRebinString + "')\n"
        "if cleanup:\n"
        "    mantid.deleteWorkspace(sqwInput)\n"
        "sqwInput += '_r'\n"
        "cleanup = True\n";
    }
    pyInput +=
      "efixed = " + m_uiForm.leEfixed->text() + "\n"
      "rebin = '" + rebinString + "'\n"      
      "SofQW(sqwInput, sqwOutput, rebin, 'Indirect', EFixed=efixed)\n"
      "if cleanup:\n"
      "    mantid.deleteWorkspace(sqwInput)\n";

    if ( m_uiForm.sqw_ckSave->isChecked() )
    {
      pyInput += "SaveNexus(sqwOutput, sqwOutput+'.nxs')\n";
    }

    if ( m_uiForm.sqw_cbPlotType->currentText() == "Contour" )
    {
      pyInput += "importMatrixWorkspace(sqwOutput).plotGraph2D()\n";
    }
    else if ( m_uiForm.sqw_cbPlotType->currentText() == "Specta" )
    {
      pyInput +=
        "nspec = mtd[sqwOuput].getNumberHistograms()\n"
        "plotSpectra(sqwOutput, range(0, nspec)\n";
    }
        
    QString pyOutput = runPythonCode(pyInput).trimmed();
  }
  else
  {
    showInformationBox("Some of your input is invalid. Please check the input highlighted.");
  }
}

void Indirect::sOfQwRebinE(bool state)
{
  QString val;
  if ( state ) val = "*";
  else val = " ";
  m_uiForm.sqw_leELow->setEnabled(state);
  m_uiForm.sqw_leEWidth->setEnabled(state);
  m_uiForm.sqw_leEHigh->setEnabled(state);
  m_uiForm.sqw_valELow->setEnabled(state);
  m_uiForm.sqw_valELow->setText(val);
  m_uiForm.sqw_valEWidth->setEnabled(state);
  m_uiForm.sqw_valEWidth->setText(val);
  m_uiForm.sqw_valEHigh->setEnabled(state);
  m_uiForm.sqw_valEHigh->setText(val);
  m_uiForm.sqw_lbELow->setEnabled(state);
  m_uiForm.sqw_lbEWidth->setEnabled(state);
  m_uiForm.sqw_lbEHigh->setEnabled(state);
}

void Indirect::sOfQwInputType(const QString& input)
{
  if ( input == "File" )
  {
    m_uiForm.sqw_swInput->setCurrentIndex(0);
  }
  else
  {
    m_uiForm.sqw_swInput->setCurrentIndex(1);
    refreshWSlist();
  }
}

void Indirect::sOfQwPlotInput()
{
  QString pyInput = "from mantidsimple import *\n"
    "from mantidplot import *\n";

  //...
  if ( m_uiForm.sqw_cbInput->currentText() == "File" )
  {
    // get filename
    if ( m_uiForm.sqw_inputFile->isValid() )
    {
      pyInput +=
        "filename = r'" + m_uiForm.sqw_inputFile->getFirstFilename() + "'\n"
        "(dir, file) = os.path.split(filename)\n"
        "(input, ext) = os.path.splitext(file)\n"
        "LoadNexus(filename, input)\n";
    }
    else
    {
      showInformationBox("Invalid filename.");
      return;
    }
  }
  else
  {
    pyInput += "input = '" + m_uiForm.sqw_cbWorkspace->currentText() + "'\n";
  }

  pyInput += "ConvertSpectrumAxis(input, input+'_q', 'MomentumTransfer', 'Indirect')\n"
    "ws = importMatrixWorkspace(input+'_q')\n"
    "ws.plotGraph2D()\n";

  QString pyOutput = runPythonCode(pyInput).trimmed();

}

// SLICE
void Indirect::sliceRun()
{
  if ( ! validateSlice() )
  {
    showInformationBox("Please check your input.");
    return;
  }

  QString pyInput =
    "from IndirectEnergyConversion import slice\n"
    "tofRange = [" + QString::number(m_sltDblMng->value(m_sltProp["R1S"])) + ","
    + QString::number(m_sltDblMng->value(m_sltProp["R1E"]));
  if ( m_sltBlnMng->value(m_sltProp["UseTwoRanges"]) )
  {
    pyInput +=
      "," + QString::number(m_sltDblMng->value(m_sltProp["R2S"])) + ","
      + QString::number(m_sltDblMng->value(m_sltProp["R2E"])) + "]\n";
  }
  else
  {
    pyInput += "]\n";
  }
  if ( m_uiForm.slice_ckUseCalib->isChecked() )
  {
    pyInput +=
      "calib = r'" + m_uiForm.slice_calibFile->getFirstFilename() + "'\n";
  }
  else
  {
    pyInput +=
      "calib = ''\n";
  }
  QString filenames = m_uiForm.slice_inputFile->getFilenames().join("', r'");
  QString suffix = m_uiForm.cbAnalyser->currentText() + m_uiForm.cbReflection->currentText();
  pyInput +=
    "rawfile = [r'" + filenames + "']\n"
    "spectra = ["+ QString::number(m_sltDblMng->value(m_sltProp["SpecMin"])) + "," + QString::number(m_sltDblMng->value(m_sltProp["SpecMax"])) +"]\n"
    "suffix = '" + suffix + "'\n";

  if ( m_uiForm.slice_ckVerbose->isChecked() ) pyInput += "verbose = True\n";
  else pyInput += "verbose = False\n";

  if ( m_uiForm.slice_ckPlot->isChecked() ) pyInput += "plot = True\n";
  else pyInput += "plot = False\n";

  if ( m_uiForm.slice_ckSave->isChecked() ) pyInput += "save = True\n";
  else pyInput += "save = False\n";

  pyInput +=
    "slice(rawfile, calib, tofRange, spectra, suffix, Save=save, Verbose=verbose, Plot=plot)";

  QString pyOutput = runPythonCode(pyInput).trimmed();
}

void Indirect::slicePlotRaw()
{
  if ( m_uiForm.slice_inputFile->isValid() )
  {
    QString filename = m_uiForm.slice_inputFile->getFirstFilename();
    QFileInfo fi(filename);
    QString wsname = fi.baseName();

    QString pyInput = "LoadRaw(r'" + filename + "', '" + wsname + "', SpectrumMin=" 
      + m_uiForm.leSpectraMin->text() + ", SpectrumMax="
      + m_uiForm.leSpectraMax->text() + ")\n";
    QString pyOutput = runPythonCode(pyInput);

    Mantid::API::MatrixWorkspace_sptr input = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(wsname.toStdString()));

    QVector<double> dataX = QVector<double>::fromStdVector(input->readX(0));
    QVector<double> dataY = QVector<double>::fromStdVector(input->readY(0));

    if ( m_sltDataCurve != NULL )
    {
      m_sltDataCurve->attach(0);
      delete m_sltDataCurve;
      m_sltDataCurve = 0;
    }

    m_sltDataCurve = new QwtPlotCurve();
    m_sltDataCurve->setData(dataX, dataY);
    m_sltDataCurve->attach(m_sltPlot);

    m_sltPlot->setAxisScale(QwtPlot::xBottom, dataX.first(), dataX.last());

    m_sltR1->setRange(dataX.first(), dataX.last());

    // Replot
    m_sltPlot->replot();
  }
  else
  {
    showInformationBox("Selected input files are invalid.");
  }

}

void Indirect::sliceTwoRanges(QtProperty*, bool state)
{
  m_sltR2->setVisible(state);
}

void Indirect::sliceCalib(bool state)
{
  m_uiForm.slice_calibFile->setEnabled(state);
  m_uiForm.slice_calibFile->isOptional(!state);
}

void Indirect::sliceMinChanged(double val)
{
  MantidWidgets::RangeSelector* from = qobject_cast<MantidWidgets::RangeSelector*>(sender());
  if ( from == m_sltR1 )
  {
    m_sltDblMng->setValue(m_sltProp["R1S"], val);
  }
  else if ( from == m_sltR2 )
  {
    m_sltDblMng->setValue(m_sltProp["R2S"], val);
  }
}

void Indirect::sliceMaxChanged(double val)
{
  MantidWidgets::RangeSelector* from = qobject_cast<MantidWidgets::RangeSelector*>(sender());
  if ( from == m_sltR1 )
  {
    m_sltDblMng->setValue(m_sltProp["R1E"], val);
  }
  else if ( from == m_sltR2 )
  {
    m_sltDblMng->setValue(m_sltProp["R2E"], val);
  }
}

void Indirect::sliceUpdateRS(QtProperty* prop, double val)
{
  if ( prop == m_sltProp["R1S"] ) m_sltR1->setMinimum(val);
  else if ( prop == m_sltProp["R1E"] ) m_sltR1->setMaximum(val);
  else if ( prop == m_sltProp["R2S"] ) m_sltR2->setMinimum(val);
  else if ( prop == m_sltProp["R2E"] ) m_sltR2->setMaximum(val);
}
