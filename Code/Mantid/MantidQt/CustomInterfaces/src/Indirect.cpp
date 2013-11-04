#include "MantidQtCustomInterfaces/Indirect.h"
#include "MantidQtCustomInterfaces/Transmission.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"
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

using namespace MantidQt::CustomInterfaces;
using Mantid::MantidVec;

/**
* This is the constructor for the Indirect Instruments Interface.
* It is used primarily to ensure sane values for member variables.
*/
Indirect::Indirect(QWidget *parent, Ui::ConvertToEnergy & uiForm) : 
  UserSubWindow(parent), m_uiForm(uiForm), m_backgroundDialog(NULL),
  m_changeObserver(*this, &Indirect::handleDirectoryChange),
  m_bgRemoval(false), m_valInt(NULL), m_valDbl(NULL), m_valPosDbl(NULL), 
  // Null pointers - Calibration Tab
  m_calCalPlot(NULL), m_calResPlot(NULL),
  m_calCalR1(NULL), m_calCalR2(NULL), m_calResR1(NULL),
  m_calCalCurve(NULL), m_calResCurve(NULL),
  // Null pointers - Diagnostics Tab
  m_sltPlot(NULL), m_sltR1(NULL), m_sltR2(NULL), m_sltDataCurve(NULL),
  m_tab_trans(new Transmission(m_uiForm,this))
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
  connect(m_uiForm.rebin_ckDNR, SIGNAL(toggled(bool)), this, SLOT(rebinCheck(bool)));
  connect(m_uiForm.ckDetailedBalance, SIGNAL(toggled(bool)), this, SLOT(detailedBalanceCheck(bool)));

  connect(m_uiForm.ckScaleMultiplier, SIGNAL(toggled(bool)), this, SLOT(scaleMultiplierCheck(bool)));

  connect(m_uiForm.ind_calibFile, SIGNAL(fileTextChanged(const QString &)), this, SLOT(calibFileChanged(const QString &)));
  connect(m_uiForm.ckUseCalib, SIGNAL(toggled(bool)), this, SLOT(useCalib(bool)));

  connect(m_uiForm.tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
  connect(m_uiForm.cbIndRebType, SIGNAL(currentIndexChanged(int)), m_uiForm.swIndRebin, SLOT(setCurrentIndex(int)));

  connect(m_uiForm.ind_runFiles, SIGNAL(fileTextChanged(const QString &)), this, SLOT(pbRunEditing()));
  connect(m_uiForm.ind_runFiles, SIGNAL(findingFiles()), this, SLOT(pbRunFinding()));
  connect(m_uiForm.ind_runFiles, SIGNAL(fileFindingFinished()), this, SLOT(pbRunFinished()));

  // "Calibration" tab
  connect(m_uiForm.cal_leRunNo, SIGNAL(filesFound()), this, SLOT(calPlotRaw()));
  connect(m_uiForm.cal_pbPlot, SIGNAL(clicked()), this, SLOT(calPlotRaw()));
  connect(m_uiForm.cal_ckRES, SIGNAL(toggled(bool)), this, SLOT(resCheck(bool)));
  connect(m_uiForm.cal_ckRES, SIGNAL(toggled(bool)), m_uiForm.cal_ckResScale, SLOT(setEnabled(bool)));
  connect(m_uiForm.cal_ckResScale, SIGNAL(toggled(bool)), m_uiForm.cal_leResScale, SLOT(setEnabled(bool)));
  connect(m_uiForm.cal_ckIntensityScaleMultiplier, SIGNAL(toggled(bool)), this, SLOT(intensityScaleMultiplierCheck(bool)));
  connect(m_uiForm.cal_leIntensityScaleMultiplier, SIGNAL(textChanged(const QString &)), this, SLOT(calibValidateIntensity(const QString &)));

  // "SofQW" tab
  connect(m_uiForm.sqw_ckRebinE, SIGNAL(toggled(bool)), this, SLOT(sOfQwRebinE(bool)));
  connect(m_uiForm.sqw_cbInput, SIGNAL(currentIndexChanged(int)), m_uiForm.sqw_swInput, SLOT(setCurrentIndex(int)));
  connect(m_uiForm.sqw_cbWorkspace, SIGNAL(currentIndexChanged(int)), this, SLOT(validateSofQ(int)));

  connect(m_uiForm.sqw_pbPlotInput, SIGNAL(clicked()), this, SLOT(sOfQwPlotInput()));

  // "Slice" tab
  connect(m_uiForm.slice_inputFile, SIGNAL(filesFound()), this, SLOT(slicePlotRaw()));
  connect(m_uiForm.slice_pbPlotRaw, SIGNAL(clicked()), this, SLOT(slicePlotRaw()));
  connect(m_uiForm.slice_ckUseCalib, SIGNAL(toggled(bool)), this, SLOT(sliceCalib(bool)));

  // "Transmission" tab
  connect(m_tab_trans, SIGNAL(runAsPythonScript(const QString&, bool)), this, SIGNAL(runAsPythonScript(const QString&, bool)));

  // create validators
  m_valInt = new QIntValidator(this);
  m_valDbl = new QDoubleValidator(this);
  m_valPosDbl = new QDoubleValidator(this);


  // Tolerance chosen arbitrarily. Avoids dividing by zero elsewhere.
  const double tolerance = 0.00001;
  m_valPosDbl->setBottom(tolerance);

  // apply validators
  m_uiForm.leNoGroups->setValidator(m_valInt);
  m_uiForm.leDetailedBalance->setValidator(m_valPosDbl);
  m_uiForm.leSpectraMin->setValidator(m_valInt);
  m_uiForm.leSpectraMax->setValidator(m_valInt);
  m_uiForm.rebin_leELow->setValidator(m_valDbl);
  m_uiForm.rebin_leEWidth->setValidator(m_valDbl);
  m_uiForm.rebin_leEHigh->setValidator(m_valDbl);

  m_uiForm.leScaleMultiplier->setValidator(m_valPosDbl);
  m_uiForm.cal_leIntensityScaleMultiplier->setValidator(m_valDbl);
  m_uiForm.cal_leResScale->setValidator(m_valDbl);
  
  m_uiForm.sqw_leELow->setValidator(m_valDbl);
  m_uiForm.sqw_leEWidth->setValidator(m_valDbl);
  m_uiForm.sqw_leEHigh->setValidator(m_valDbl);
  m_uiForm.sqw_leQLow->setValidator(m_valDbl);
  m_uiForm.sqw_leQWidth->setValidator(m_valDbl);
  m_uiForm.sqw_leQHigh->setValidator(m_valDbl);


  // set default values for save formats
  m_uiForm.save_ckSPE->setChecked(false);
  m_uiForm.save_ckNexus->setChecked(true);

  m_uiForm.cal_valIntensityScaleMultiplier->setText(" ");

  // nudge "Background Removal" button to display whether it is
  // set to "OFF" or "ON".
  backgroundRemoval();
  // nudge tabChanged too, so that we start off with a "Run Energy Transfer" button
  // instead of a "Run" button which then changes when clicking on a different tab.
  tabChanged(0);

  loadSettings();
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
  else if (tabName == "Transmission")
    url += "Transmission";
  QDesktopServices::openUrl(QUrl(url));
}
/**
* This function will control the actions needed for the Indirect interface when the
* "Run" button is clicked by the user.
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
  else if (tabName == "Transmission")
  {
    m_tab_trans->runTab();
  }
}

void Indirect::runConvertToEnergy()
{
  if ( ! validateInput() )
  {
    showInformationBox("Please check the input highlighted in red.");
    return;
  }

  QString pyInput =
    "import inelastic_indirect_reducer as iir\n"
    "reducer = iir.IndirectReducer()\n"
    "reducer.set_instrument_name('" + m_uiForm.cbInst->currentText() + "')\n"
    "reducer.set_detector_range(" +m_uiForm.leSpectraMin->text()+ "-1, " +m_uiForm.leSpectraMax->text()+ "-1)\n"
    "reducer.set_parameter_file('" + QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("instrumentDefinition.directory")) + m_uiForm.cbInst->currentText() + "_" + m_uiForm.cbAnalyser->currentText() + "_" + m_uiForm.cbReflection->currentText() + "_Parameters.xml')\n";

  QStringList files = m_uiForm.ind_runFiles->getFilenames();
  for ( QStringList::iterator it = files.begin(); it != files.end(); ++it )
  {
    pyInput += "reducer.append_data_file(r'" + *it + "')\n";
  }

  if ( m_uiForm.ckSumFiles->isChecked() )
  {
    pyInput += "reducer.set_sum_files(True)\n";
  }

  if ( m_bgRemoval )
  {
    QPair<double,double> background = m_backgroundDialog->getRange();
    pyInput += "reducer.set_background("+QString::number(background.first)+", "+QString::number(background.second)+")\n";
  }

  if ( m_uiForm.ckUseCalib->isChecked() )
  {
    pyInput +=
      "from IndirectCommon import loadNexus\n"
      "reducer.set_calibration_workspace(loadNexus(r'"+m_uiForm.ind_calibFile->getFirstFilename()+"'))\n";
  }

  if ( m_uiForm.ckLoadLogs->isChecked() )
  {
    pyInput += "reducer.set_load_logs(True)\n";
  }

  if ( ! m_uiForm.rebin_ckDNR->isChecked() )
  {
    QString rebin;
    if ( m_uiForm.cbIndRebType->currentIndex() == 0 )
    {
      rebin = m_uiForm.rebin_leELow->text() + "," + m_uiForm.rebin_leEWidth->text() + "," + m_uiForm.rebin_leEHigh->text();
    }
    else
    {
      rebin = m_uiForm.leRebinString->text();
    }
    pyInput += "reducer.set_rebin_string('"+rebin+"')\n";
  }

  if ( m_uiForm.ckDetailedBalance->isChecked() )
  {
    pyInput += "reducer.set_detailed_balance(" + m_uiForm.leDetailedBalance->text() + ")\n";
  }

  if ( m_uiForm.ckScaleMultiplier->isChecked() )
  {
    pyInput += "reducer.set_scale_factor(" + m_uiForm.leScaleMultiplier->text() + ")\n";
  }

  if ( m_uiForm.cbMappingOptions->currentText() != "Default" )
  {
    QString grouping = createMapFile(m_uiForm.cbMappingOptions->currentText());
    pyInput += "reducer.set_grouping_policy('" + grouping + "')\n";
  }

  if ( ! m_uiForm.ckRenameWorkspace->isChecked() )
  {
    pyInput += "reducer.set_rename(False)\n";
  }

  if ( ! m_uiForm.ckFold->isChecked() )
  {
    pyInput += "reducer.set_fold_multiple_frames(False)\n";
  }

  if( m_uiForm.ckCm1Units->isChecked() )
  {
    pyInput += "reducer.set_save_to_cm_1(True)\n";
  }
  
  if ( m_uiForm.ckCreateInfoTable->isChecked() )
  {
    pyInput += "reducer.create_info_table()\n";
  }

  pyInput += "reducer.set_save_formats([" + savePyCode() + "])\n";

  pyInput +=
    "reducer.reduce()\n"
    "ws_list = reducer.get_result_workspaces()\n";

  // Plot Output options
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

  // add sample logs to each of the workspaces
  QString calibChecked = m_uiForm.ckUseCalib->isChecked() ? "True" : "False";
  QString detailedBalance = m_uiForm.ckDetailedBalance->isChecked() ? "True" : "False";
  QString scaled = m_uiForm.ckScaleMultiplier->isChecked() ? "True" : "False";
  pyInput += "calibCheck = "+calibChecked+"\n"
             "detailedBalance = "+detailedBalance+"\n"
             "scaled = "+scaled+"\n"
             "for ws in ws_list:\n"
             "  AddSampleLog(Workspace=ws, LogName='calib_file', LogType='String', LogText=str(calibCheck))\n"
             "  if calibCheck:\n"
             "    AddSampleLog(Workspace=ws, LogName='calib_file_name', LogType='String', LogText='"+m_uiForm.ind_calibFile->getFirstFilename()+"')\n"
             "  AddSampleLog(Workspace=ws, LogName='detailed_balance', LogType='String', LogText=str(detailedBalance))\n"
             "  if detailedBalance:\n"
             "    AddSampleLog(Workspace=ws, LogName='detailed_balance_temp', LogType='Number', LogText='"+m_uiForm.leDetailedBalance->text()+"')\n"
             "  AddSampleLog(Workspace=ws, LogName='scale', LogType='String', LogText=str(scaled))\n"
             "  if scaled:\n"
             "    AddSampleLog(Workspace=ws, LogName='scale_factor', LogType='Number', LogText='"+m_uiForm.leScaleMultiplier->text()+"')\n";

  QString pyOutput = runPythonCode(pyInput).trimmed();
}

/**
* This function holds any steps that must be performed on the selection of an instrument,
* for example loading values from the Instrument Definition File (IDF).
* @param prefix :: The selected instruments prefix in Mantid.
*/
void Indirect::setIDFValues(const QString & prefix)
{
  UNUSED_ARG(prefix);
  // empty ComboBoxes, LineEdits,etc of previous values
  m_uiForm.cbAnalyser->clear();
  m_uiForm.cbReflection->clear();
  clearReflectionInfo();

  rebinCheck(m_uiForm.rebin_ckDNR->isChecked());
  detailedBalanceCheck(m_uiForm.ckDetailedBalance->isChecked());
  resCheck(m_uiForm.cal_ckRES->isChecked());

  scaleMultiplierCheck(m_uiForm.ckScaleMultiplier->isChecked());

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

/**
* This function holds any steps that must be performed on the layout that are specific
* to the currently selected instrument.
*/
void Indirect::performInstSpecific()
{
  setInstSpecificWidget("cm-1-convert-choice", m_uiForm.ckCm1Units, QCheckBox::Off);
  setInstSpecificWidget("save-aclimax-choice", m_uiForm.save_ckAclimax, QCheckBox::Off);
}

/**
* This function either shows or hides the given QCheckBox, based on the named property
* inside the instrument param file.  When hidden, the default state will be used to
* reset to the "unused" state of the checkbox.
*
* @param parameterName :: The name of the property to look for inside the current inst param file.
* @param checkBox :: The checkbox to set the state of, and to either hide or show based on the current inst.
* @param defaultState :: The state to which the checkbox will be set upon hiding it.
*/
void Indirect::setInstSpecificWidget(const std::string & parameterName, QCheckBox * checkBox, QCheckBox::ToggleState defaultState)
{
  // Get access to instrument specific parameters via the loaded empty workspace.
  std::string instName = m_uiForm.cbInst->currentText().toStdString();
  Mantid::API::MatrixWorkspace_sptr input = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve("__empty_" + instName));
  if(NULL == input)
    return;
  Mantid::Geometry::Instrument_const_sptr instr = input->getInstrument();

  // See if the instrument params file requests that the checkbox be shown to the user.
  std::vector<std::string> showParams = instr->getStringParameter(parameterName);
  
  std::string show = "";
  if(!showParams.empty())
    show = showParams[0];
  
  if(show == "Show")
    checkBox->setHidden(false);
  else
  {
    checkBox->setHidden(true);
    checkBox->setState(defaultState);
  }
}

void Indirect::closeEvent(QCloseEvent* close)
{
  (void) close;
  Mantid::Kernel::ConfigService::Instance().removeObserver(m_changeObserver);
}

void Indirect::handleDirectoryChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf)
{
  std::string key = pNf->key();
  // std::string preValue = pNf->preValue(); // Unused
  // std::string curValue = pNf->curValue(); // Unused

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
  if ( m_uiForm.save_ckAscii->isChecked() )
    fileFormats << "ascii";
  if ( m_uiForm.save_ckAclimax->isChecked() )
    fileFormats << "aclimax";

  if ( fileFormats.size() != 0 )
    fileFormatList = "'" + fileFormats.join("', '") + "'";
  else
    fileFormatList = "";

  return fileFormatList;
}
/**
* This function is called after calib has run and creates a RES file for use in later analysis (Fury,etc)
* @param file :: the input file (WBV run.raw)
*/
void Indirect::createRESfile(const QString& file)
{
  QString scaleFactor("1.0");
  if(m_uiForm.cal_ckResScale->isChecked())
  {
    if(!m_uiForm.cal_leResScale->text().isEmpty())
    {
      scaleFactor = m_uiForm.cal_leResScale->text();
    }
  }

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

  QString scaled = m_uiForm.cal_ckIntensityScaleMultiplier->isChecked() ? "True" : "False";
  pyInput +=
    "background = " + background + "\n"
    "rebinParam = '" + rebinParam + "'\n"
    "file = " + file + "\n"
    "ws = resolution(file, iconOpt, rebinParam, background, instrument, analyser, reflection, plotOpt = plot, factor="+scaleFactor+")\n"
    "scaled = "+ scaled +"\n"
    "scaleFactor = "+m_uiForm.cal_leIntensityScaleMultiplier->text()+"\n"
    "backStart = "+QString::number(m_calDblMng->value(m_calCalProp["BackMin"]))+"\n"
    "backEnd = "+QString::number(m_calDblMng->value(m_calCalProp["BackMax"]))+"\n"
    "rebinLow = "+QString::number(m_calDblMng->value(m_calResProp["ELow"]))+"\n"
    "rebinWidth = "+QString::number(m_calDblMng->value(m_calResProp["EWidth"]))+"\n"
    "rebinHigh = "+QString::number(m_calDblMng->value(m_calResProp["EHigh"]))+"\n"
    "AddSampleLog(Workspace=ws, LogName='scale', LogType='String', LogText=str(scaled))\n"
    "if scaled:"
    "  AddSampleLog(Workspace=ws, LogName='scale_factor', LogType='Number', LogText=str(scaleFactor))\n"
    "AddSampleLog(Workspace=ws, LogName='back_start', LogType='Number', LogText=str(backStart))\n"
    "AddSampleLog(Workspace=ws, LogName='back_end', LogType='Number', LogText=str(backEnd))\n"
    "AddSampleLog(Workspace=ws, LogName='rebin_low', LogType='Number', LogText=str(rebinLow))\n"
    "AddSampleLog(Workspace=ws, LogName='rebin_width', LogType='Number', LogText=str(rebinWidth))\n"
    "AddSampleLog(Workspace=ws, LogName='rebin_high', LogType='Number', LogText=str(rebinHigh))\n";

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

  int dummyPos = 0;

  QString text = m_uiForm.leDetailedBalance->text();
  QValidator::State fieldState = m_uiForm.leDetailedBalance->validator()->validate(text, dummyPos);

  // detailed balance
  if ( m_uiForm.ckDetailedBalance->isChecked() && fieldState != QValidator::Acceptable )
  {
    valid = false;
    m_uiForm.valDetailedBalance->setText("*");
  }
  else
  {
    m_uiForm.valDetailedBalance->setText("");
  }

  int dummyPos2 = 0;

  // scale multiplier
  QString scaleMultiplierText = m_uiForm.leScaleMultiplier->text();
  QValidator::State fieldState2 = m_uiForm.leScaleMultiplier->validator()->validate(scaleMultiplierText, dummyPos2);

  if ( m_uiForm.ckScaleMultiplier->isChecked() && fieldState2 != QValidator::Acceptable )
  {
    valid = false;
    m_uiForm.valScaleMultiplier->setText("*");
  }
  else
  {
    m_uiForm.valScaleMultiplier->setText("");
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
    if ( m_uiForm.cbIndRebType->currentIndex() == 0 )
    {
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
      if ( m_uiForm.leRebinString->text() == "" )
      {
        valid = false;
      }
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
QString Indirect::validateCalib()
{
  UserInputValidator uiv;
  
  uiv.checkMWRunFilesIsValid("Run", m_uiForm.cal_leRunNo);

  auto peakRange = std::make_pair(m_calDblMng->value(m_calCalProp["PeakMin"]), m_calDblMng->value(m_calCalProp["PeakMax"]));
  auto backRange = std::make_pair(m_calDblMng->value(m_calCalProp["BackMin"]), m_calDblMng->value(m_calCalProp["BackMax"]));

  uiv.checkValidRange("Peak Range", peakRange);
  uiv.checkValidRange("Back Range", backRange);
  uiv.checkRangesDontOverlap(peakRange, backRange);

  if ( m_uiForm.cal_ckRES->isChecked() )
  {
    auto backgroundRange = std::make_pair(m_calDblMng->value(m_calResProp["Start"]), m_calDblMng->value(m_calResProp["End"]));
    uiv.checkValidRange("Background", backgroundRange);

    double eLow   = m_calDblMng->value(m_calResProp["ELow"]);
    double eHigh  = m_calDblMng->value(m_calResProp["EHigh"]);
    double eWidth = m_calDblMng->value(m_calResProp["EWidth"]);

    uiv.checkBins(eLow, eWidth, eHigh);
  }

  if( m_uiForm.cal_ckIntensityScaleMultiplier->isChecked()
    && m_uiForm.cal_leIntensityScaleMultiplier->text().isEmpty() )
  {
    uiv.addErrorMessage("You must enter a scale for the calibration file");
  }

  if( m_uiForm.cal_ckResScale->isChecked() && m_uiForm.cal_leResScale->text().isEmpty() )
  {
    uiv.addErrorMessage("You must enter a scale for the resolution file");
  }

  return uiv.generateErrorMessage();
}

void Indirect::validateSofQ(int)
{
  validateSofQw();
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

QString Indirect::validateSlice()
{
  UserInputValidator uiv;

  uiv.checkMWRunFilesIsValid("Input", m_uiForm.slice_inputFile);
  if( m_uiForm.slice_ckUseCalib->isChecked() )
    uiv.checkMWRunFilesIsValid("Calibration", m_uiForm.slice_inputFile);

  auto rangeOne = std::make_pair(m_sltDblMng->value(m_sltProp["R1S"]), m_sltDblMng->value(m_sltProp["R1E"]));
  uiv.checkValidRange("Range One", rangeOne);

  bool useTwoRanges = m_sltBlnMng->value(m_sltProp["UseTwoRanges"]);
  if( useTwoRanges )
  {
    auto rangeTwo = std::make_pair(m_sltDblMng->value(m_sltProp["R2S"]), m_sltDblMng->value(m_sltProp["R2E"]));
    uiv.checkValidRange("Range Two", rangeTwo);

    uiv.checkRangesDontOverlap(rangeOne, rangeTwo);
  }

  return uiv.generateErrorMessage();
}

void Indirect::loadSettings()
{  
  // set values of m_dataDir and m_saveDir
  m_dataDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("datasearch.directories"));
  m_dataDir.replace(" ","");
  if(m_dataDir.length() > 0)
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

/**
 * Called when a user starts to type / edit the runs to load.
 */
void Indirect::pbRunEditing()
{
  m_uiForm.pbRun->setEnabled(false);
  m_uiForm.pbRun->setText("Editing...");
}

/**
 * Called when the FileFinder starts finding the files.
 */
void Indirect::pbRunFinding()
{
  m_uiForm.pbRun->setText("Finding files...");
  m_uiForm.ind_runFiles->setEnabled(false);
}

/**
 * Called when the FileFinder has finished finding the files.
 */
void Indirect::pbRunFinished()
{
  m_uiForm.pbRun->setEnabled(true);
  m_uiForm.ind_runFiles->setEnabled(true);
  tabChanged(m_uiForm.tabWidget->currentIndex());
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
  UNUSED_ARG(index);
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
      m_uiForm.leRebinString->setText(values[8]);
      m_uiForm.rebin_ckDNR->setChecked(false);
      QStringList rbp = values[8].split(",", QString::SkipEmptyParts);
      if ( rbp.size() == 3 )
      {
        m_uiForm.rebin_leELow->setText(rbp[0]);
        m_uiForm.rebin_leEWidth->setText(rbp[1]);
        m_uiForm.rebin_leEHigh->setText(rbp[2]);
        m_uiForm.cbIndRebType->setCurrentIndex(0);
      }
      else
      {
        m_uiForm.cbIndRebType->setCurrentIndex(1);
      }
    }
    else
    {
      m_uiForm.rebin_ckDNR->setChecked(true);
      m_uiForm.rebin_leELow->setText("");
      m_uiForm.rebin_leEWidth->setText("");
      m_uiForm.rebin_leEHigh->setText("");
      m_uiForm.leRebinString->setText("");
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
* Can be called before m_backgroundDialog even exists, for the purposes of setting the button to
* it's initial (default) value.
*/
void Indirect::backgroundRemoval()
{
  if ( NULL != m_backgroundDialog )
    m_bgRemoval = m_backgroundDialog->removeBackground();
  
  if (m_bgRemoval)
    m_uiForm.pbBack_2->setText("Background Removal (On)");
  else
    m_uiForm.pbBack_2->setText("Background Removal (Off)");
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
      "from mantid.simpleapi import CalculateFlatBackground,GroupDetectors,Load\n"
      "from mantidplot import plotSpectrum\n"
      "import os.path as op\n"
      "file = r'" + rawFile + "'\n"
      "name = op.splitext( op.split(file)[1] )[0]\n"
      "bgrange = " + bgrange + "\n"
      "Load(Filename=file, OutputWorkspace=name, SpectrumMin="+specList[0]+", SpectrumMax="+specList[1]+")\n"
      "if ( bgrange != [-1, -1] ):\n"
      "    #Remove background\n"
      "    CalculateFlatBackground(InputWorkspace=name, OutputWorkspace=name+'_bg', StartX=bgrange[0], EndX=bgrange[1], Mode='Mean')\n"
      "    GroupDetectors(InputWorkspace=name+'_bg', OutputWorkspace=name+'_grp', DetectorList=range("+specList[0]+","+specList[1]+"+1))\n"
      "    GroupDetectors(InputWorkspace=name, OutputWorkspace=name+'_grp_raw', DetectorList=range("+specList[0]+","+specList[1]+"+1))\n"
      "else: # Just group detectors as they are\n"
      "    GroupDetectors(InputWorkspace=name, OutputWorkspace=name+'_grp', DetectorList=range("+specList[0]+","+specList[1]+"+1))\n"
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
* Disables/enables the relevant parts of the UI when user checks/unchecks the 'Multiplication Factor (Scale):'
* ckScaleMultiplier checkbox.
* @param state :: state of the checkbox
*/
void Indirect::scaleMultiplierCheck(bool state)
{
  m_uiForm.leScaleMultiplier->setEnabled(state);
}

void Indirect::intensityScaleMultiplierCheck(bool state)
{
  m_uiForm.cal_leIntensityScaleMultiplier->setEnabled(state);
}

void Indirect::calibValidateIntensity(const QString & text)
{
  if(!text.isEmpty())
  {
    m_uiForm.cal_valIntensityScaleMultiplier->setText(" ");
  }
  else
  {
    m_uiForm.cal_valIntensityScaleMultiplier->setText("*");
  }
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
  QString error = validateCalib();
  if ( ! error.isEmpty() )
  {
    showInformationBox(error);
  }
  else
  {
    QString filenames = "[r'"+m_uiForm.cal_leRunNo->getFilenames().join("', r'")+"']";

    QString reducer = "from mantid.simpleapi import SaveNexus\n"
      "from inelastic_indirect_reduction_steps import CreateCalibrationWorkspace\n"
      "calib = CreateCalibrationWorkspace()\n"
      "calib.set_files(" + filenames + ")\n"
      "calib.set_detector_range(" + m_uiForm.leSpectraMin->text() + "-1, " + m_uiForm.leSpectraMax->text() + "-1)\n"
      "calib.set_parameters(" + m_calCalProp["BackMin"]->valueText() + "," 
        + m_calCalProp["BackMax"]->valueText() + ","
        + m_calCalProp["PeakMin"]->valueText() + ","
        + m_calCalProp["PeakMax"]->valueText() + ")\n"
      "calib.set_analyser('" + m_uiForm.cbAnalyser->currentText() + "')\n"
      "calib.set_reflection('" + m_uiForm.cbReflection->currentText() + "')\n";

    //scale values by arbitrary scalar if requested
    if(m_uiForm.cal_ckIntensityScaleMultiplier->isChecked())
    {
      QString scale = m_uiForm.cal_leIntensityScaleMultiplier->text(); 
      if(scale.isEmpty())
      {
        scale = "1.0";
      }
      reducer += "calib.set_intensity_scale("+scale+")\n";
    }

    reducer += "calib.execute(None, None)\n"
      "result = calib.result_workspace()\n"
      "print result\n"
      "SaveNexus(InputWorkspace=result, Filename=result+'.nxs')\n";

    if ( m_uiForm.cal_ckPlotResult->isChecked() )
    {
      reducer += "from mantidplot import plotTimeBin\n"
        "plotTimeBin(result, 0)\n";
    }

    QString pyOutput = runPythonCode(reducer).trimmed();

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
      m_uiForm.ind_calibFile->setFileTextWithSearch(pyOutput + ".nxs");
      m_uiForm.ckUseCalib->setChecked(true);
    }
  }
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
  
  if ( filename.isEmpty() )
  {
    return;
  }
    
  QFileInfo fi(filename);
  QString wsname = fi.baseName();
  QString pyInput = "Load(Filename=r'" + filename + "', OutputWorkspace='" + wsname + "', SpectrumMin="
    + m_uiForm.leSpectraMin->text() + ", SpectrumMax="
    + m_uiForm.leSpectraMax->text() + ")\n";

  pyInput = "try:\n  " +
               pyInput +
            "except ValueError as ve:" +
            "  print str(ve)";

  QString pyOutput = runPythonCode(pyInput);
  
  if( ! pyOutput.isEmpty() )
  {
    showInformationBox("Unable to load file.  Error: \n\n" + pyOutput + "\nCheck whether your file exists and matches the selected instrument in the Energy Transfer tab.");
    return;
  }

  Mantid::API::MatrixWorkspace_sptr input = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(wsname.toStdString()));

  const MantidVec & dataX = input->readX(0);
  const MantidVec & dataY = input->readY(0);

  if ( m_calCalCurve != NULL )
  {
    m_calCalCurve->attach(0);
    delete m_calCalCurve;
    m_calCalCurve = 0;
  }

  m_calCalCurve = new QwtPlotCurve();
  m_calCalCurve->setData(&dataX[0], &dataY[0], static_cast<int>(input->blocksize()));
  m_calCalCurve->attach(m_calCalPlot);
  
  m_calCalPlot->setAxisScale(QwtPlot::xBottom, dataX.front(), dataX.back());

  m_calCalR1->setRange(dataX.front(), dataX.back());
  m_calCalR2->setRange(dataX.front(), dataX.back());

  // Replot
  m_calCalPlot->replot();

  // also replot the energy
  calPlotEnergy();
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

  const MantidVec & dataX = input->readX(0);
  const MantidVec & dataY = input->readY(0);

  if ( m_calResCurve != NULL )
  {
    m_calResCurve->attach(0);
    delete m_calResCurve;
    m_calResCurve = 0;
  }

  m_calResCurve = new QwtPlotCurve();
  m_calResCurve->setData(&dataX[0], &dataY[0], static_cast<int>(input->blocksize()));
  m_calResCurve->attach(m_calResPlot);
  
  m_calResPlot->setAxisScale(QwtPlot::xBottom, dataX.front(), dataX.back());
  m_calResR1->setRange(dataX.front(), dataX.back());

  m_calResR2->setMinimum(m_calDblMng->value(m_calResProp["ELow"]));
  m_calResR2->setMaximum(m_calDblMng->value(m_calResProp["EHigh"]));

  calSetDefaultResolution(input);

  // Replot
  m_calResPlot->replot();
}

void Indirect::calSetDefaultResolution(Mantid::API::MatrixWorkspace_const_sptr ws)
{
  auto inst = ws->getInstrument();
  auto analyser = inst->getStringParameter("analyser");

  if(analyser.size() > 0)
  {
    auto comp = inst->getComponentByName(analyser[0]);
    auto params = comp->getNumberParameter("resolution", true);

    //set the default instrument resolution
    if(params.size() > 0)
    {
      double res = params[0];
      m_calDblMng->setValue(m_calResProp["ELow"], -res*10);
      m_calDblMng->setValue(m_calResProp["EHigh"], res*10);

      m_calDblMng->setValue(m_calResProp["Start"], -res*9);
      m_calDblMng->setValue(m_calResProp["End"], -res*8);
    }

  }
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
    QString pyInput = "from mantid.simpleapi import *\n";

    if ( m_uiForm.sqw_cbInput->currentText() == "File" )
    {
      pyInput +=
        "filename = r'" +m_uiForm.sqw_inputFile->getFirstFilename() + "'\n"
        "(dir, file) = os.path.split(filename)\n"
        "(sqwInput, ext) = os.path.splitext(file)\n"
        "LoadNexus(Filename=filename, OutputWorkspace=sqwInput)\n";
    }
    else
    {
      pyInput +=
        "sqwInput = '" + m_uiForm.sqw_cbWorkspace->currentText() + "'\n";
    }

    // Create output name before rebinning
    pyInput += "sqwOutput = sqwInput[:-3] + 'sqw'\n";

    if ( m_uiForm.sqw_ckRebinE->isChecked() )
    {
      QString eRebinString = m_uiForm.sqw_leELow->text()+","+m_uiForm.sqw_leEWidth->text()+","+m_uiForm.sqw_leEHigh->text();
      pyInput += "Rebin(InputWorkspace=sqwInput, OutputWorkspace=sqwInput+'_r', Params='" + eRebinString + "')\n"
        "sqwInput += '_r'\n";
    }
    pyInput +=
      "efixed = " + m_uiForm.leEfixed->text() + "\n"
      "rebin = '" + rebinString + "'\n";

    QString rebinType = m_uiForm.sqw_cbRebinType->currentText();
    if(rebinType == "Centre (SofQW)")
      pyInput += "SofQW(InputWorkspace=sqwInput, OutputWorkspace=sqwOutput, QAxisBinning=rebin, EMode='Indirect', EFixed=efixed)\n";
    else if(rebinType == "Parallelepiped (SofQW2)")
      pyInput += "SofQW2(InputWorkspace=sqwInput, OutputWorkspace=sqwOutput, QAxisBinning=rebin, EMode='Indirect', EFixed=efixed)\n";
    else if(rebinType == "Parallelepiped/Fractional Area (SofQW3)")
      pyInput += "SofQW3(InputWorkspace=sqwInput, OutputWorkspace=sqwOutput, QAxisBinning=rebin, EMode='Indirect', EFixed=efixed)\n";

    pyInput += "AddSampleLog(Workspace=sqwOutput, LogName='rebin_type', LogType='String', LogText='"+rebinType+"')\n";

    if ( m_uiForm.sqw_ckSave->isChecked() )
    {
      pyInput += "SaveNexus(InputWorkspace=sqwOutput, Filename=sqwOutput+'.nxs')\n";
    }

    if ( m_uiForm.sqw_cbPlotType->currentText() == "Contour" )
    {
      pyInput += "importMatrixWorkspace(sqwOutput).plotGraph2D()\n";
    }
    else if ( m_uiForm.sqw_cbPlotType->currentText() == "Spectra" )
    {
      pyInput +=
        "nspec = mtd[sqwOutput].getNumberHistograms()\n"
        "plotSpectrum(sqwOutput, range(0, nspec))\n";
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

void Indirect::sOfQwPlotInput()
{
  QString pyInput = "from mantid.simpleapi import *\n"
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
        "LoadNexus(Filename=filename, OutputWorkspace=input)\n";
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

  pyInput += "ConvertSpectrumAxis(InputWorkspace=input, OutputWorkspace=input[:-4]+'_rqw', Target='ElasticQ', EMode='Indirect')\n"
    "ws = importMatrixWorkspace(input[:-4]+'_rqw')\n"
    "ws.plotGraph2D()\n";

  QString pyOutput = runPythonCode(pyInput).trimmed();

}

// SLICE
void Indirect::sliceRun()
{
  QString error = validateSlice();
  if ( ! error.isEmpty() )
  {
    showInformationBox(error);
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

    QString pyInput = "Load(Filename=r'" + filename + "', OutputWorkspace='" + wsname + "', SpectrumMin="
      + m_uiForm.leSpectraMin->text() + ", SpectrumMax="
      + m_uiForm.leSpectraMax->text() + ")\n";
    
    pyInput = "try:\n  " +
                pyInput +
              "except ValueError as ve:" +
              "  print str(ve)";

    QString pyOutput = runPythonCode(pyInput);
    
    if( ! pyOutput.isEmpty() )
    {
      showInformationBox("Unable to load file: \n\n\"" + pyOutput + "\".\n\nCheck whether your file exists and matches the selected instrument in the EnergyTransfer tab.");
      return;
    }

    Mantid::API::MatrixWorkspace_sptr input = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(wsname.toStdString()));

    const MantidVec & dataX = input->readX(0);
    const MantidVec & dataY = input->readY(0);

    if ( m_sltDataCurve != NULL )
    {
      m_sltDataCurve->attach(0);
      delete m_sltDataCurve;
      m_sltDataCurve = 0;
    }

    m_sltDataCurve = new QwtPlotCurve();
    m_sltDataCurve->setData(&dataX[0], &dataY[0], static_cast<int>(input->blocksize()));
    m_sltDataCurve->attach(m_sltPlot);

    m_sltPlot->setAxisScale(QwtPlot::xBottom, dataX.front(), dataX.back());

    m_sltR1->setRange(dataX.front(), dataX.back());

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
