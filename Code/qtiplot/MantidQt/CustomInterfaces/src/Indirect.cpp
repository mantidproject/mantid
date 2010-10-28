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
  connect(m_uiForm.leEfixed, SIGNAL(editingFinished()), this, SLOT(setasDirty()));
  connect(m_uiForm.leSpectraMin, SIGNAL(editingFinished()), this, SLOT(setasDirty()));
  connect(m_uiForm.leSpectraMax, SIGNAL(editingFinished()), this, SLOT(setasDirty()));
  connect(m_uiForm.ckSumFiles, SIGNAL(pressed()), this, SLOT(setasDirty()));
  connect(m_uiForm.ckUseCalib, SIGNAL(toggled(bool)), this, SLOT(useCalib(bool)));
  connect(m_uiForm.ckCleanUp, SIGNAL(pressed()), this, SLOT(setasDirty()));

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
  connect(m_uiForm.slice_ckUseTwoRanges, SIGNAL(toggled(bool)), this, SLOT(sliceTwoRanges(bool)));
  connect(m_uiForm.slice_ckUseCalib, SIGNAL(toggled(bool)), this, SLOT(sliceCalib(bool)));

  // create validators
  m_valInt = new QIntValidator(this);
  m_valDbl = new QDoubleValidator(this);

  // apply validators
  m_uiForm.leNoGroups->setValidator(m_valInt);
  m_uiForm.leDetailedBalance->setValidator(m_valInt);
  m_uiForm.leEfixed->setValidator(m_valDbl);
  m_uiForm.leSpectraMin->setValidator(m_valInt);
  m_uiForm.leSpectraMax->setValidator(m_valInt);
  m_uiForm.rebin_leELow->setValidator(m_valDbl);
  m_uiForm.rebin_leEWidth->setValidator(m_valDbl);
  m_uiForm.rebin_leEHigh->setValidator(m_valDbl);

  m_uiForm.cal_lePeakMin->setValidator(m_valInt);
  m_uiForm.cal_lePeakMax->setValidator(m_valInt);
  m_uiForm.cal_leBackMin->setValidator(m_valInt);
  m_uiForm.cal_leBackMax->setValidator(m_valInt);
  m_uiForm.cal_leResSpecMin->setValidator(m_valInt);
  m_uiForm.cal_leResSpecMax->setValidator(m_valInt);
  m_uiForm.cal_leStartX->setValidator(m_valDbl);
  m_uiForm.cal_leEndX->setValidator(m_valDbl);
  m_uiForm.cal_leELow->setValidator(m_valDbl);
  m_uiForm.cal_leEWidth->setValidator(m_valDbl);
  m_uiForm.cal_leEHigh->setValidator(m_valDbl);

  m_uiForm.sqw_leELow->setValidator(m_valDbl);
  m_uiForm.sqw_leEWidth->setValidator(m_valDbl);
  m_uiForm.sqw_leEHigh->setValidator(m_valDbl);
  m_uiForm.sqw_leQLow->setValidator(m_valDbl);
  m_uiForm.sqw_leQWidth->setValidator(m_valDbl);
  m_uiForm.sqw_leQHigh->setValidator(m_valDbl);

  m_uiForm.slice_leRange0->setValidator(m_valInt);
  m_uiForm.slice_leRange1->setValidator(m_valInt);
  m_uiForm.slice_leRange2->setValidator(m_valInt);
  m_uiForm.slice_leRange3->setValidator(m_valInt);

  // set default values for save formats
  m_uiForm.save_ckSPE->setChecked(false);
  m_uiForm.save_ckNexus->setChecked(true);

  loadSettings();

  refreshWSlist();

  sliceTwoRanges(m_uiForm.slice_ckUseTwoRanges->isChecked());
}
/**
* This function will hold any Python-dependent setup actions for the interface.
*/
void Indirect::initLocalPython()
{
  // Nothing to do at this stage.
}
/**
* This function opens a web browser window to the Mantid Project wiki page for this
* interface ("Inelastic" subsection of ConvertToEnergy).
*/
void Indirect::helpClicked()
{
  QString tabName = m_uiForm.tabWidget->tabText(m_uiForm.tabWidget->currentIndex());
  QString url = "http://www.mantidproject.org/ConvertToEnergy#";
  if ( tabName == "Energy Transfer" )
    url += "Energy Transfer";
  else if ( tabName == "Calibration" )
    url += "Calibration";
  else if ( tabName == "Diagnostics" )
    url += "Time Slice";
  else if ( tabName == "S(Q, w)" )
    url += "SofQW";
  QDesktopServices::openUrl(QUrl(url));
}
/**
* This function will control the actions needed for the Indirect interface when the
* "Run" button is clicked by the user.
* @param tryToSave whether to try and save the output. Generally true, false when user has clicked on the "Rebin" button instead of "Run"
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
  QString groupFile = createMapFile(m_uiForm.cbMappingOptions->currentText());
  if ( groupFile == "" )
  {
    return;
  }

  QString filePrefix = m_uiForm.cbInst->itemData(m_uiForm.cbInst->currentIndex()).toString().toLower();
  filePrefix += "_" + m_uiForm.cbAnalyser->currentText() + m_uiForm.cbReflection->currentText() + "_";

  QString pyInput = "from mantidsimple import *\n"
    "import IndirectEnergyConversion as ind\n";

  pyInput += "first = " +m_uiForm.leSpectraMin->text()+ "\n";
  pyInput += "last = " +m_uiForm.leSpectraMax->text()+ "\n";
  pyInput += "ana = '"+m_uiForm.cbAnalyser->currentText()+"'\n";
  pyInput += "ref = '"+m_uiForm.cbReflection->currentText()+"'\n";

  QStringList runFiles_list = m_uiForm.ind_runFiles->getFilenames();
  QString runFiles = runFiles_list.join("', r'");

  pyInput += "rawfiles = [r'"+runFiles+"']\n"
    "Sum=";
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

  pyInput += "efixed = "+m_uiForm.leEfixed->text()+"\n";

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

  pyInput += "mapfile = r'"+groupFile+"'\n";

  if (tryToSave)
  {
    pyInput += savePyCode();
  }
  else
  {
    pyInput +=
      "fileFormats = []\n"
      "ins = ''\n"
      "directory = ''\n"
      "suffix = ''\n";
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
    pyInput += "ind.convert_to_energy(rawfiles, mapfile, "
      "first, last, efixed, SumFiles=Sum, bgremove = bgRemove, tempK = tempK, calib = calib, "
      "rebinParam = rebinParam, instrument = ins, savesuffix = suffix, saveFormats = fileFormats,"
      "analyser = ana, reflection = ref, CleanUp=clean)\n";
  }
  else if ( isDirtyRebin() )
  {
    pyInput += "ind.cte_rebin(mapfile, tempK, rebinParam, ana, ref, ins, suffix,"
      "fileFormats, directory, CleanUp = clean)\n";
  }
  else if ( tryToSave )
  {
    pyInput +=
      "import re\n"
      "wslist = mantid.getWorkspaceNames()\n"
      "save_ws = re.compile(r'_'+ana+ref+'$')\n"
      "ws_list = []\n"
      "runNos = []\n"
      "for workspace in wslist:\n"
      "   if save_ws.search(workspace):\n"
      "      ws_list.append(workspace)\n"
      "      runNos.append(mantid.getMatrixWorkspace(workspace).getRun().getLogData('run_number').value())\n"
      "ind.saveItems(ws_list, runNos, fileFormats, ins, suffix)\n";
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
* @param prefix The selected instruments prefix in Mantid.
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
* This function clears the values of the QLineEdit objects used to hold Reflection-specific
* information.
*/
void Indirect::clearReflectionInfo()
{
  m_uiForm.leSpectraMin->clear();
  m_uiForm.leSpectraMax->clear();
  m_uiForm.leEfixed->clear();
  m_uiForm.cal_lePeakMin->clear();
  m_uiForm.cal_lePeakMax->clear();
  m_uiForm.cal_leBackMin->clear();
  m_uiForm.cal_leBackMax->clear();

  m_uiForm.cal_leResSpecMin->clear();
  m_uiForm.cal_leResSpecMax->clear();

  m_uiForm.slice_leSpecMin->clear();
  m_uiForm.slice_leSpecMax->clear();
  m_uiForm.slice_leRange0->clear();
  m_uiForm.slice_leRange1->clear();
  m_uiForm.slice_leRange2->clear();
  m_uiForm.slice_leRange3->clear();

  isDirty(true);
}
/**
* This function creates the mapping/grouping file for the data analysis.
* @param groupType Type of grouping (All, Group, Indiviual)
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
    ngroup = "1";
    nspec = ndet;
  }
  else if ( groupType == "Individual" )
  {
    ngroup = ndet;
    nspec = "1";
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
  QString analyser = m_uiForm.cbAnalyser->currentText();

  QString ins = m_uiForm.cbInst->itemData(m_uiForm.cbInst->currentIndex()).toString().toLower();
  QString suffix = analyser + m_uiForm.cbReflection->currentText() + "_red";

  QStringList fileFormats;
  QString fileFormatList;

  if ( m_uiForm.save_ckNexus->isChecked() )
    fileFormats << "nxs";
  if ( m_uiForm.save_ckSPE->isChecked() )
    fileFormats << "spe";

  if ( fileFormats.size() != 0 )
    fileFormatList = "[ '" + fileFormats.join("', '") + "']";
  else
    fileFormatList = "[]";

  QString pyInput =
    "# Save File Parameters\n"
    "ins = '" + ins + "'\n"
    "suffix = '" + suffix + "'\n"
    "fileFormats = " + fileFormatList + "\n";

  return pyInput;
}
/**
* This function is called after calib has run and creates a RES file for use in later analysis (Fury,etc)
* @param file the input file (WBV run.raw)
*/
void Indirect::createRESfile(const QString& file)
{
  QString pyInput =
    "import IndirectEnergyConversion as ind\n"
    "iconOpt = { 'first': " +m_uiForm.cal_leResSpecMin->text()+
    ", 'last': " +m_uiForm.cal_leResSpecMax->text()+
    ", 'efixed': " +m_uiForm.leEfixed->text()+ "}\n"
    "plot = ";

  if ( m_uiForm.cal_ckPlotResult->isChecked() )
  {
    pyInput +=	"True\n";
  }
  else
  {
    pyInput += "False\n";
  }
  QString rebinParam = m_uiForm.cal_leELow->text() + "," +
    m_uiForm.cal_leEWidth->text() + "," +
    m_uiForm.cal_leEHigh->text();
  QString background = "[ " +m_uiForm.cal_leStartX->text()+ ", " +m_uiForm.cal_leEndX->text()+"]";

  pyInput +=
    "background = " + background + "\n"
    "rebinParam = '" + rebinParam + "'\n"
    "file = r'" + file + "'\n"
    "outWS = ind.res(file, iconOpt, rebinParam, background, plotOpt = plot)\n";

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

  // efixed
  if ( m_uiForm.leEfixed->text() == "" )
  {
    valid = false;
    m_uiForm.valEFixed->setText("*");
  }
  else
  {
    m_uiForm.valEFixed->setText("");
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

  // peak min/max, back min/max (calib)
  if ( m_uiForm.cal_lePeakMin->text() == "" )
  {
    valid = false;
    m_uiForm.valPeakMin->setText("*");
  }
  else
  {
    m_uiForm.valPeakMin->setText("");
  }

  if ( m_uiForm.cal_lePeakMax->text() == "" )
  {
    valid = false;
    m_uiForm.valPeakMax->setText("*");
  }
  else
  {
    m_uiForm.valPeakMax->setText("");
  }

  if ( m_uiForm.cal_leBackMin->text() == "" )
  {
    valid = false;
    m_uiForm.valBackMin->setText("*");
  }
  else
  {
    m_uiForm.valBackMin->setText("");
  }

  if ( m_uiForm.cal_leBackMax->text() == "" )
  {
    valid = false;
    m_uiForm.valBackMax->setText("*");
  }
  else
  {
    m_uiForm.valBackMax->setText("");
  }

  if ( m_uiForm.cal_lePeakMin->text().toInt() > m_uiForm.cal_lePeakMax->text().toInt() )
  {
    valid = false;
    m_uiForm.valPeakMin->setText("*");
    m_uiForm.valPeakMax->setText("*");
  }

  if ( m_uiForm.cal_leBackMin->text().toInt() > m_uiForm.cal_leBackMax->text().toInt() )
  {
    valid = false;
    m_uiForm.valBackMin->setText("*");
    m_uiForm.valBackMax->setText("*");
  }


  if ( m_uiForm.cal_ckRES->isChecked() )
  {
    // SpectraSelect min/max
    if ( m_uiForm.cal_leResSpecMin->text() == "" )
    {
      valid = false;
      m_uiForm.valResSpecMin->setText("*");
    }
    else
    {
      m_uiForm.valResSpecMin->setText("");
    }
    if ( m_uiForm.cal_leResSpecMax->text() == "" )
    {
      valid = false;
      m_uiForm.valResSpecMax->setText("*");
    }
    else
    {
      m_uiForm.valResSpecMax->setText("");
    }

    if ( m_uiForm.cal_leResSpecMin->text().toInt() > m_uiForm.cal_leResSpecMax->text().toInt() )
    {
      valid = false;
      m_uiForm.valResSpecMin->setText("*");
      m_uiForm.valResSpecMax->setText("*");
    }

    // start/end x
    if ( m_uiForm.cal_leStartX->text() == "" )
    {
      valid = false;
      m_uiForm.valStartX->setText("*");
    }
    else
    {
      m_uiForm.valStartX->setText(" ");
    }
    if ( m_uiForm.cal_leEndX->text() == "" )
    {
      valid = false;
      m_uiForm.valEndX->setText("*");
    }
    else
    {
      m_uiForm.valEndX->setText(" ");
    }

    if ( m_uiForm.cal_leStartX->text().toInt() > m_uiForm.cal_leEndX->text().toInt() )
    {
      valid = false;
      m_uiForm.valStartX->setText("*");
      m_uiForm.valEndX->setText("*");
    }

    // rebinning (res)
    if ( m_uiForm.cal_leEWidth->text() == "" )
    {
      valid = false;
      m_uiForm.valResEWidth->setText("*");
    }
    else
    {
      m_uiForm.valResEWidth->setText(" ");
    }

    if ( m_uiForm.cal_leELow->text() == "" )
    {
      valid = false;
      m_uiForm.valResELow->setText("*");
    }
    else
    {
      m_uiForm.valResELow->setText(" ");
    }

    if ( m_uiForm.cal_leEHigh->text() == "" )
    {
      valid = false;
      m_uiForm.valResEHigh->setText("*");
    }
    else
    {
      m_uiForm.valResEHigh->setText(" ");
    }

    if ( m_uiForm.cal_leELow->text().toInt() > m_uiForm.cal_leEHigh->text().toInt() )
    {
      valid = false;
      m_uiForm.valResELow->setText("*");
      m_uiForm.valResEHigh->setText("*");
    }

  }
  else
  {
    m_uiForm.valResSpecMin->setText(" ");
    m_uiForm.valResSpecMax->setText(" ");
    m_uiForm.valStartX->setText(" ");
    m_uiForm.valEndX->setText(" ");
    m_uiForm.valResELow->setText(" ");
    m_uiForm.valResEWidth->setText(" ");
    m_uiForm.valResEHigh->setText(" ");
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

  if ( m_uiForm.slice_ckUseCalib->isChecked() && ! m_uiForm.slice_calibFile->isValid() )
  {
    valid = false;
  }

  if ( m_uiForm.slice_leSpecMin->text() == "" )
  {
    m_uiForm.slice_valSpecMin->setText("*");
    valid = false;
  }
  else
  {
    m_uiForm.slice_valSpecMin->setText(" ");
  }

  if ( m_uiForm.slice_leSpecMax->text() == "" )
  {
    m_uiForm.slice_valSpecMax->setText("*");
    valid = false;
  }
  else
  {
    m_uiForm.slice_valSpecMax->setText(" ");
  }

  if ( m_uiForm.slice_leRange0->text() == "" )
  {
    m_uiForm.slice_valRange0->setText("*");
    valid = false;
  }
  else
  {
    m_uiForm.slice_valRange0->setText(" ");
  }

  if ( m_uiForm.slice_leRange1->text() == "" )
  {
    m_uiForm.slice_valRange1->setText("*");
    valid = false;
  }
  else
  {
    m_uiForm.slice_valRange1->setText(" ");
  }

  if ( m_uiForm.slice_ckUseTwoRanges->isChecked() )
  {
    if ( m_uiForm.slice_leRange2->text() == "" )
    {
      m_uiForm.slice_valRange2->setText("*");
      valid = false;
    }
    else
    {
      m_uiForm.slice_valRange2->setText(" ");
    }

    if ( m_uiForm.slice_leRange3->text() == "" )
    {
      m_uiForm.slice_valRange3->setText("*");
      valid = false;
    }
    else
    {
      m_uiForm.slice_valRange3->setText(" ");
    }
  }
  else
  {
    m_uiForm.slice_valRange2->setText(" ");
    m_uiForm.slice_valRange3->setText(" ");
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
* @param state whether to set the value to true or false.
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
* @param state whether to set the value to true or false.
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
  // Calib
  m_calCalPlot = new QwtPlot(this);
  m_calCalPlot->setAxisFont(QwtPlot::xBottom, this->font());
  m_calCalPlot->setAxisFont(QwtPlot::yLeft, this->font());
  m_uiForm.cal_plotCal->addWidget(m_calCalPlot);
  m_calCalPlot->setCanvasBackground(Qt::white);
  // R1 = Peak, R2 = Background
  m_calCalR1 = new MantidWidgets::RangeSelector(m_calCalPlot);
  m_calCalR1->setMinimum(m_uiForm.cal_lePeakMin->text().toDouble());
  m_calCalR1->setMaximum(m_uiForm.cal_lePeakMax->text().toDouble());
  connect(m_calCalR1, SIGNAL(minValueChanged(double)), this, SLOT(calMinChanged(double)));
  connect(m_calCalR1, SIGNAL(maxValueChanged(double)), this, SLOT(calMaxChanged(double)));
  m_calCalR2 = new MantidWidgets::RangeSelector(m_calCalPlot);
  m_calCalR2->setColour(Qt::darkGreen); // dark green to signify background range
  m_calCalR2->setMinimum(m_uiForm.cal_leBackMin->text().toDouble());
  m_calCalR2->setMaximum(m_uiForm.cal_leBackMax->text().toDouble());
  connect(m_calCalR2, SIGNAL(minValueChanged(double)), this, SLOT(calMinChanged(double)));
  connect(m_calCalR2, SIGNAL(maxValueChanged(double)), this, SLOT(calMaxChanged(double)));

  // Res
  m_calResPlot = new QwtPlot(this);
  m_calResPlot->setAxisFont(QwtPlot::xBottom, this->font());
  m_calResPlot->setAxisFont(QwtPlot::yLeft, this->font());
  m_uiForm.cal_plotRes->addWidget(m_calResPlot);
  m_calResPlot->setCanvasBackground(Qt::white);
  // Only one range selector for Res (background)
  m_calResR1 = new MantidWidgets::RangeSelector(m_calResPlot);
  m_calResR1->setMinimum(m_uiForm.cal_lePeakMin->text().toDouble());
  m_calResR1->setMaximum(m_uiForm.cal_lePeakMax->text().toDouble());
  connect(m_calResR1, SIGNAL(minValueChanged(double)), this, SLOT(calMinChanged(double)));
  connect(m_calResR1, SIGNAL(maxValueChanged(double)), this, SLOT(calMaxChanged(double)));

  connect(m_uiForm.cal_lePeakMin, SIGNAL(editingFinished()), this, SLOT(calUpdateRS()));
  connect(m_uiForm.cal_lePeakMax, SIGNAL(editingFinished()), this, SLOT(calUpdateRS()));
  connect(m_uiForm.cal_leBackMin, SIGNAL(editingFinished()), this, SLOT(calUpdateRS()));
  connect(m_uiForm.cal_leBackMax, SIGNAL(editingFinished()), this, SLOT(calUpdateRS()));
  connect(m_uiForm.cal_leStartX, SIGNAL(editingFinished()), this, SLOT(calUpdateRS()));
  connect(m_uiForm.cal_leEndX, SIGNAL(editingFinished()), this, SLOT(calUpdateRS()));
}

void Indirect::setupSlice()
{
  // Create Slice Plot Widget for Range Selection
  m_sltPlot = new QwtPlot(this);
  m_sltPlot->setAxisFont(QwtPlot::xBottom, this->font());
  m_sltPlot->setAxisFont(QwtPlot::yLeft, this->font());
  m_uiForm.slice_plot->addWidget(m_sltPlot);
  m_sltPlot->setCanvasBackground(Qt::white);
  // We always want one range selector... the second one can be controlled from
  // within the sliceTwoRanges(bool state) function
  m_sltR1 = new MantidWidgets::RangeSelector(m_sltPlot);
  m_sltR1->setMinimum(m_uiForm.slice_leRange0->text().toDouble());
  m_sltR1->setMaximum(m_uiForm.slice_leRange1->text().toDouble());
  connect(m_sltR1, SIGNAL(minValueChanged(double)), this, SLOT(sliceMinChanged(double)));
  connect(m_sltR1, SIGNAL(maxValueChanged(double)), this, SLOT(sliceMaxChanged(double)));

  // second range
  // create the second range
  m_sltR2 = new MantidWidgets::RangeSelector(m_sltPlot);
  m_sltR2->setColour(Qt::darkGreen); // dark green for background
  m_sltR2->setMinimum(m_uiForm.slice_leRange2->text().toDouble()); // m_uiForm.slice_leRange2->text().toDouble()
  m_sltR2->setMaximum(m_uiForm.slice_leRange3->text().toDouble());
  connect(m_sltR1, SIGNAL(rangeChanged(double, double)), m_sltR2, SLOT(setRange(double, double)));
  connect(m_sltR2, SIGNAL(minValueChanged(double)), this, SLOT(sliceMinChanged(double)));
  connect(m_sltR2, SIGNAL(maxValueChanged(double)), this, SLOT(sliceMaxChanged(double)));
  m_sltR2->setRange(m_sltR1->getRange());

  // Refresh the plot window
  m_sltPlot->replot();

  connect(m_uiForm.slice_leRange0, SIGNAL(editingFinished()), this, SLOT(sliceUpdateRS()));
  connect(m_uiForm.slice_leRange1, SIGNAL(editingFinished()), this, SLOT(sliceUpdateRS()));
  connect(m_uiForm.slice_leRange2, SIGNAL(editingFinished()), this, SLOT(sliceUpdateRS()));
  connect(m_uiForm.slice_leRange3, SIGNAL(editingFinished()), this, SLOT(sliceUpdateRS()));
}

/* QT SLOT FUNCTIONS */

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
* @param index Index of the value selected in the combo box.
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
* @param index Index of the value selected in the combo box.
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
    m_uiForm.cal_leResSpecMin->setText(values[1]);
    m_uiForm.cal_leResSpecMax->setText(values[2]);
    m_uiForm.slice_leSpecMin->setText(values[1]);
    m_uiForm.slice_leSpecMax->setText(values[2]);
    if ( values.count() == 8 )
    {
      m_uiForm.leEfixed->setText(values[3]);
      m_uiForm.cal_lePeakMin->setText(values[4]);
      m_uiForm.cal_lePeakMax->setText(values[5]);
      m_uiForm.cal_leBackMin->setText(values[6]);
      m_uiForm.cal_leBackMax->setText(values[7]);

      m_uiForm.slice_leRange0->setText(values[4]);
      m_uiForm.slice_leRange1->setText(values[5]);
      m_uiForm.slice_leRange2->setText(values[6]);
      m_uiForm.slice_leRange3->setText(values[7]);
      m_sltR1->setMinimum(values[4].toDouble());
      m_sltR1->setMaximum(values[5].toDouble());
      m_sltR2->setMinimum(values[6].toDouble());
      m_sltR2->setMaximum(values[7].toDouble());
    }
    else
    {
      m_uiForm.leEfixed->clear();
      m_uiForm.cal_lePeakMin->clear();
      m_uiForm.cal_lePeakMax->clear();
      m_uiForm.cal_leBackMin->clear();
      m_uiForm.cal_leBackMax->clear();
      m_uiForm.slice_leRange0->clear();
      m_uiForm.slice_leRange1->clear();
      m_uiForm.slice_leRange2->clear();
      m_uiForm.slice_leRange3->clear();
    }
  }

  // clear validation markers
  validateInput();
  validateCalib();
  validateSlice();
}
/**
* This function runs when the user makes a selection on the cbMappingOptions QComboBox.
* @param groupType Value of selection made by user.
*/
void Indirect::mappingOptionSelected(const QString& groupType)
{
  if ( groupType == "File" )
  {
    m_uiForm.swMapping->setCurrentIndex(0);
    m_uiForm.swMapping->setEnabled(true);
  }
  else if ( groupType == "All" )
  {
    m_uiForm.swMapping->setCurrentIndex(2);
    m_uiForm.swMapping->setEnabled(false);
  }
  else if ( groupType == "Individual" )
  {
    m_uiForm.swMapping->setCurrentIndex(2);
    m_uiForm.swMapping->setEnabled(false);
  }
  else if ( groupType == "Groups" )
  {
    m_uiForm.swMapping->setCurrentIndex(1);
    m_uiForm.swMapping->setEnabled(true);
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
    }
    else
    {
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

      QString pyInput =
        "from mantidsimple import *\n"
        "from mantidplot import *\n"
        "LoadRaw(r'"+rawFile+"', 'RawTime', SpectrumMin="+specList[0]+", SpectrumMax="+specList[1]+")\n"
        "GroupDetectors('RawTime', 'RawTime', DetectorList=range("+specList[0]+","+specList[1]+"+1))\n"
        "graph = plotSpectrum('RawTime', 0)\n";

      QString pyOutput = runPythonCode(pyInput).trimmed();

      if ( pyOutput != "" )
      {
        showInformationBox(pyOutput);
      }
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
* @param state whether the "Do Not Rebin" checkbox is checked
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
* @param state state of the checkbox
*/
void Indirect::detailedBalanceCheck(bool state)
{
  m_uiForm.leDetailedBalance->setEnabled(state);
  m_uiForm.lbDBKelvin->setEnabled(state);

  isDirtyRebin(true);
}
/**
* This function enables/disables the display of the options involved in creating the RES file.
* @param state whether checkbox is checked or unchecked
*/
void Indirect::resCheck(bool state)
{
  m_uiForm.cal_pbPlotEnergy->setEnabled(state);
  m_uiForm.cal_gbRES->setEnabled(state);
  m_calResR1->setVisible(state);
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
    QString suffix = "_" + m_uiForm.cbAnalyser->currentText() + m_uiForm.cbReflection->currentText() + "_calib.nxs";
    QString pyInput =
      "from IndirectEnergyConversion import createCalibFile\n"
      "plot = ";

    if ( m_uiForm.cal_ckPlotResult->isChecked() )
      pyInput +=	"True\n";
    else
      pyInput += "False\n";

    pyInput +=
      "file = createCalibFile(r'"+file+"', '"+suffix+"', %1, %2, %3, %4, %5, %6, PlotOpt=plot)\n"
      "print file\n";

    pyInput = pyInput.arg(m_uiForm.cal_lePeakMin->text());
    pyInput = pyInput.arg(m_uiForm.cal_lePeakMax->text());
    pyInput = pyInput.arg(m_uiForm.cal_leBackMin->text());
    pyInput = pyInput.arg(m_uiForm.cal_leBackMax->text());
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
        createRESfile(file);
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
* @param calib path to calib file
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

  m_calCalR1->setMinimum(m_uiForm.cal_lePeakMin->text().toDouble());
  m_calCalR1->setMaximum(m_uiForm.cal_lePeakMax->text().toDouble());
  m_calCalR1->setRange(dataX.first(), dataX.last());

  m_calCalR2->setMinimum(m_uiForm.cal_leBackMin->text().toDouble());
  m_calCalR2->setMaximum(m_uiForm.cal_leBackMax->text().toDouble());
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
  QString file = m_uiForm.cal_leRunNo->getFirstFilename();
  QString pyInput =
    "from IndirectEnergyConversion import res\n"
    "iconOpt = { 'first': " +m_uiForm.cal_leResSpecMin->text()+
    ", 'last': " +m_uiForm.cal_leResSpecMax->text()+
    ", 'efixed': " +m_uiForm.leEfixed->text()+ "}\n"
    "file = r'" + file + "'\n"
    "outWS = res(file, iconOpt, '', '', Res=False)\n"
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

  m_calResR1->setMinimum(m_uiForm.cal_leStartX->text().toDouble());
  m_calResR1->setMaximum(m_uiForm.cal_leEndX->text().toDouble());
  m_calResR1->setRange(dataX.first(), dataX.last());

  // Replot
  m_calResPlot->replot();
}

void Indirect::calMinChanged(double val)
{
  MantidWidgets::RangeSelector* from = qobject_cast<MantidWidgets::RangeSelector*>(sender());
  if ( from == m_calCalR1 )
  {
    m_uiForm.cal_lePeakMin->setText(QString::number(val));
  }
  else if ( from == m_calCalR2 )
  {
    m_uiForm.cal_leBackMin->setText(QString::number(val));
  }
  else if ( from == m_calResR1 )
  {
    m_uiForm.cal_leStartX->setText(QString::number(val));
  }
}

void Indirect::calMaxChanged(double val)
{
  MantidWidgets::RangeSelector* from = qobject_cast<MantidWidgets::RangeSelector*>(sender());
  if ( from == m_calCalR1 )
  {
    m_uiForm.cal_lePeakMax->setText(QString::number(val));
  }
  else if ( from == m_calCalR2 )
  {
    m_uiForm.cal_leBackMax->setText(QString::number(val));
  }
  else if ( from == m_calResR1 )
  {
    m_uiForm.cal_leEndX->setText(QString::number(val));
  }
}

void Indirect::calUpdateRS()
{
  QLineEdit* from = qobject_cast<QLineEdit*>(sender());
  double val = from->text().toDouble();

  if ( from == m_uiForm.cal_lePeakMin )
  {
    m_calCalR1->setMinimum(val);
  }
  else if ( from == m_uiForm.cal_lePeakMax )
  {
    m_calCalR1->setMaximum(val);
  }
  else if ( from == m_uiForm.cal_leBackMin )
  {
    m_calCalR2->setMinimum(val);
  }
  else if ( from == m_uiForm.cal_leBackMax )
  {
    m_calCalR2->setMaximum(val);
  }
  else if ( from == m_uiForm.cal_leStartX )
  {
    m_calResR1->setMinimum(val);
  }
  else if ( from == m_uiForm.cal_leEndX )
  {
    m_calResR1->setMaximum(val);
  }
}

void Indirect::sOfQwClicked()
{
  if ( validateSofQw() )
  {
    QString rebinString = m_uiForm.sqw_leQLow->text()+","+m_uiForm.sqw_leQWidth->text()+","+m_uiForm.sqw_leQHigh->text();
    QString pyInput = "from mantidsimple import *\n";

    if ( m_uiForm.sqw_cbInput->currentText() == "File" )
    {
      pyInput += "cleanup = True\n"
        "sqwInput = 'sqwInput'\n"
        "LoadNexusProcessed(r'"+m_uiForm.sqw_inputFile->getFirstFilename()+ "','sqwInput')\n";
    }
    else
    {
      pyInput += "cleanup = False\n"
        "sqwInput = '" + m_uiForm.sqw_cbWorkspace->currentText() + "'\n";
    }

    if ( m_uiForm.sqw_ckRebinE->isChecked() )
    {
      QString eRebinString = m_uiForm.sqw_leELow->text()+","+m_uiForm.sqw_leEWidth->text()+","+m_uiForm.sqw_leEHigh->text();
      pyInput += "Rebin(sqwInput, 'sqwInput_r', '" + eRebinString + "')\n"
        "if cleanup:\n"
        "    mantid.deleteWorkspace(sqwInput)\n"
        "sqwInput = 'sqwInput_r'\n";
    }
    pyInput +=
      "efixed = " +m_uiForm.leEfixed->text()+"\n"
      "rebin = '" + rebinString + "'\n"
      "SofQW(sqwInput,'sqwOutput',rebin,'Indirect',EFixed=efixed)\n"
      "if cleanup:\n"
      "    mantid.deleteWorkspace(sqwInput)\n";
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
      pyInput += "input = 'SofQWInput'\n"
        "filename = r'" + m_uiForm.sqw_inputFile->getFirstFilename() + "'\n"
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

  pyInput += "nHist = mtd[input].getNumberHistograms()\n"
    "plotSpectrum(input, range(0,nHist))\n";

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
    "from IndirectDataAnalysis import slice\n"
    "tofRange = [" + m_uiForm.slice_leRange0->text() + ","
    + m_uiForm.slice_leRange1->text();
  if ( m_uiForm.slice_ckUseTwoRanges->isChecked() )
  {
    pyInput +=
      "," + m_uiForm.slice_leRange2->text() + ","
      + m_uiForm.slice_leRange3->text() + "]\n";
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
  pyInput +=
    "rawfile = [r'" + filenames + "']\n"
    "spectra = ["+m_uiForm.slice_leSpecMin->text() + "," + m_uiForm.slice_leSpecMax->text() +"]\n";

  if ( m_uiForm.slice_ckVerbose->isChecked() ) pyInput += "verbose = True\n";
  else pyInput += "verbose = False\n";

  if ( m_uiForm.slice_ckPlot->isChecked() ) pyInput += "plot = True\n";
  else pyInput += "plot = False\n";

  if ( m_uiForm.slice_ckSave->isChecked() ) pyInput += "save = True\n";
  else pyInput += "save = False\n";

  pyInput +=
    "slice(rawfile, calib, tofRange, spectra, Save=save, Verbose=verbose, Plot=plot)";

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

void Indirect::sliceTwoRanges(bool state)
{
  m_uiForm.slice_lbRange2->setEnabled(state);
  m_uiForm.slice_lbTo2->setEnabled(state);
  m_uiForm.slice_leRange2->setEnabled(state);
  m_uiForm.slice_leRange3->setEnabled(state);
  m_uiForm.slice_valRange2->setEnabled(state);
  m_uiForm.slice_valRange3->setEnabled(state);

  m_sltR2->setVisible(state);

  validateSlice();
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
    m_uiForm.slice_leRange0->setText(QString::number(val));
  }
  else if ( from == m_sltR2 )
  {
    m_uiForm.slice_leRange2->setText(QString::number(val));
  }
}

void Indirect::sliceMaxChanged(double val)
{
  MantidWidgets::RangeSelector* from = qobject_cast<MantidWidgets::RangeSelector*>(sender());
  if ( from == m_sltR1 )
  {
    m_uiForm.slice_leRange1->setText(QString::number(val));
  }
  else if ( from == m_sltR2 )
  {
    m_uiForm.slice_leRange3->setText(QString::number(val));
  }
}

void Indirect::sliceUpdateRS()
{
  QLineEdit* from = qobject_cast<QLineEdit*>(sender());
  double val = from->text().toDouble();

  if ( from == m_uiForm.slice_leRange0 )
  {
    m_sltR1->setMinimum(val);
  }
  else if ( from == m_uiForm.slice_leRange1 )
  {
    m_sltR1->setMaximum(val);
  }
  else if ( from == m_uiForm.slice_leRange2 )
  {
    m_sltR2->setMinimum(val);
  }
  else if ( from == m_uiForm.slice_leRange3 )
  {
    m_sltR2->setMaximum(val);
  }
}