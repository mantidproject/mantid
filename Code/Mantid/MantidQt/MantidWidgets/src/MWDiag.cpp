#include "MantidQtMantidWidgets/MWDiag.h"
#include "MantidQtMantidWidgets/DiagResults.h"
#include "MantidQtMantidWidgets/MWRunFiles.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidQtAPI/FileDialogHandler.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FileProperty.h"
#include "MantidGeometry/Instrument.h"

#include <QSignalMapper>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QStringList>
#include <QMessageBox>
#include <QDoubleValidator>

#include <vector>
#include <string>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace MantidQt::API;
using namespace MantidQt::MantidWidgets;
using Mantid::Geometry::Instrument_const_sptr;

MWDiag::MWDiag(QWidget *parent, QString prevSettingsGr, const QComboBox * const instru):
  MantidWidget(parent),
  m_dispDialog(NULL), m_instru(instru), 
  m_TOFChanged(false), m_sTOFAutoVal(-1), m_eTOFAutoVal(-1), m_sumMono(false)
{
  // allows saving and loading the values the user entered on to the form
  m_prevSets.beginGroup(prevSettingsGr);
  // Layout the widgets
  m_designWidg.setupUi(this);

  loadSettings();
  setupToolTips();
  setUpValidators();
  connectSignals(parent);
}

/// loads default values into each control using either the previous value used when the form was run or the default value for that control
void MWDiag::loadSettings()
{
  // Want the defaults from the instrument if nothing is saved in the config
  Instrument_const_sptr instrument = getInstrument(m_instru->currentText());

  m_designWidg.maskFileFinder->setText(getSetting("input mask"));
  m_designWidg.leOFile->setText(getSetting("output file"));
  m_designWidg.leSignificance->setText(getSetting("significance", instrument, "diag_samp_sig"));
  m_designWidg.leHighAbs->setText(getSetting("high abs", instrument, "diag_huge"));
  m_designWidg.leLowAbs->setText(getSetting("low abs", instrument, "diag_tiny"));
  m_designWidg.leHighMed->setText(getSetting("high median", instrument, "diag_van_hi"));
  m_designWidg.leLowMed->setText(getSetting("low median", instrument, "diag_van_lo"));
  m_designWidg.leVariation->setText(getSetting("variation", instrument, "diag_variation"));
  m_designWidg.leStartTime->setText(getSetting("TOF start", instrument, "bkgd-range-min"));
  m_designWidg.leEndTime->setText(getSetting("TOF end", instrument, "bkgd-range-max"));
  m_designWidg.leAcceptance->setText(getSetting("back criteria", instrument, "diag_samp_hi"));
  m_designWidg.bleed_maxrate->setText(getSetting("bleed_max_framerate", instrument, "diag_bleed_maxrate"));
  m_designWidg.ignored_pixels->setText(getSetting("bleed_ignored_pixels", instrument, "diag_bleed_pixels"));

  // Boolean settings
  // Background tests
  QString value = getSetting("test background", instrument, "check_background");
  bool checked = static_cast<bool>(value.toUInt());
  m_designWidg.ckDoBack->setChecked(checked);
  // Zero removal
  value = getSetting("no zero background", instrument, "diag_samp_zero");
  checked = static_cast<bool>(value.toUInt());
  m_designWidg.ckZeroCounts->setChecked(checked);
  // Bleed test
  value = getSetting("bleed_test", instrument, "diag_bleed_test");
  checked = static_cast<bool>(value.toUInt());
  m_designWidg.bleed_group->setChecked(checked);
}

void MWDiag::setSumState(bool checked)
{
  m_sumMono = checked;
}


/**
* Get an instrument pointer for the name instrument
*/
Instrument_const_sptr MWDiag::getInstrument(const QString & name)
{
  std::string ws_name = "__empty_" + name.toStdString();

  AnalysisDataServiceImpl& dataStore = AnalysisDataService::Instance();
  if( !dataStore.doesExist(ws_name) )
  {
    QString pyInput =
      "from DirectEnergyConversion import setup_reducer\n"
      "setup_reducer('%1')";
    pyInput = pyInput.arg(QString::fromStdString(ws_name));
    runPythonCode(pyInput);
    if( !dataStore.doesExist(ws_name) )
    {
      return Instrument_const_sptr();
    }
  }
  MatrixWorkspace_sptr inst_ws = 
    boost::dynamic_pointer_cast<MatrixWorkspace>(dataStore.retrieve(ws_name));

  return inst_ws->getInstrument();
}

QString MWDiag::getSetting(const QString & settingName, boost::shared_ptr<const Mantid::Geometry::Instrument> instrument,
                           const QString & idfName) const
{
  QString value;
  if( m_prevSets.contains(settingName) )
  {
    value = m_prevSets.value(settingName).toString();
  }
  else if( instrument && !idfName.isEmpty() )
  {
    std::string parName = idfName.toStdString();
    std::string parType = instrument->getParameterType(parName);
    switch(parType[0])
    {
    case('b'):
      {
        std::vector<bool> vl = instrument->getBoolParameter(parName);
        if (vl[0])
          value = "1";
        else
          value = "0";
        break;
      }
    case('i'):
      {
        std::vector<int> params = instrument->getIntParameter(parName);
        if( params.size() == 1 )
        {
          value = QString::number(params.front());
        }
        else value = QString();
        break;
      }
    default:
      {
        std::vector<double> params = instrument->getNumberParameter(parName);
        if( params.size() == 1 )
        {
          value = QString::number(params.front());
        }
        else value = QString();
      }


    }
  }
  else
  {
    value = QString();
  }
  // Convert strings of true/false to 1/0
  if( value.trimmed().toLower()  == "true" ) value = "1";
  else if( value.trimmed().toLower() == "false" ) value = "0";

  return value;
}

/// loads default values into each control using either the previous value used when 
/// the form was run or the default value for that control
void MWDiag::saveDefaults()
{
  m_prevSets.setValue("input mask", m_designWidg.maskFileFinder->getText());
  m_prevSets.setValue("output file", m_designWidg.leOFile->text());

  m_prevSets.setValue("significance", m_designWidg.leSignificance->text());
  m_prevSets.setValue("no solid", m_designWidg.ckAngles->isChecked());

  m_prevSets.setValue("high abs", m_designWidg.leHighAbs->text());
  m_prevSets.setValue("low abs", m_designWidg.leLowAbs->text());
  m_prevSets.setValue("high median", m_designWidg.leHighMed->text());
  m_prevSets.setValue("low median", m_designWidg.leLowMed->text());

  m_prevSets.setValue("variation", m_designWidg.leVariation->text());

  m_prevSets.setValue("test background", m_designWidg.ckDoBack->isChecked());
  m_prevSets.setValue("back criteria", m_designWidg.leAcceptance->text());
  m_prevSets.setValue("no zero background",
    m_designWidg.ckZeroCounts->isChecked());
  m_prevSets.setValue("TOF start", m_designWidg.leStartTime->text());
  m_prevSets.setValue("TOF end", m_designWidg.leEndTime->text());
}
/// runs setToolTip() on each of the controls on the form  
void MWDiag::setupToolTips()
{  
  QString oFileToolTip =
    "The name of a file to write the spectra numbers of those that fail a test";
  m_designWidg.lbOFile->setToolTip(oFileToolTip);
  m_designWidg.leOFile->setToolTip(oFileToolTip);
  m_designWidg.pbOFile->setToolTip(oFileToolTip);

  QString significanceToolTip =
    "Spectra with integrated counts within this number of standard deviations from\n"
    "the median will not be labeled bad (sets property SignificanceTest when\n"
    "MedianDetectorTest is run)";
  m_designWidg.leSignificance->setToolTip(significanceToolTip);
  m_designWidg.lbSignificance->setToolTip(significanceToolTip);
  m_designWidg.ckAngles->setToolTip("Not yet implemented");
  //-------------------------------------------------------------------------------------------------
  QString highAbsSetTool =
    "Reject any spectrum that contains more than this number of counts in total\n"
    "(sets property HighThreshold when FindDetectorsOutsideLimits is run)";
  m_designWidg.leHighAbs->setToolTip(highAbsSetTool);
  m_designWidg.lbHighAbs->setToolTip(highAbsSetTool);

  QString lowAbsSetTool =
    "Reject any spectrum that contains less than this number of counts in total\n"
    "(sets property LowThreshold when FindDetectorsOutsideLimits is run)";
  m_designWidg.leLowAbs->setToolTip(lowAbsSetTool);
  m_designWidg.lbLowAbs->setToolTip(lowAbsSetTool);

  QString highMedToolTip =
    "Reject any spectrum whose total number of counts is more than this number of\n"
    "times the median total for spectra (sets property HighThreshold when\n"
    "MedianDetectorTest is run)";
  m_designWidg.leHighMed->setToolTip(highMedToolTip);
  m_designWidg.lbHighMed->setToolTip(highMedToolTip);

  QString lowMedToolTip =
    "Reject any spectrum whose total number of counts is less than this number of\n"
    "times the median total for spectra (sets property LowThreshold when\n"
    "MedianDetectorTest is run)";
  m_designWidg.leLowMed->setToolTip(lowMedToolTip);
  m_designWidg.lbLowMed->setToolTip(lowMedToolTip);

  QString variationToolTip = 
    "When comparing equivalent spectra in the two white beam vanadiums reject any\n"
    "whose the total number of counts varies by more than this multiple of the\n"
    "median variation (sets property Variation when DetectorEfficiencyVariation is\n"
    "is run)";
  m_designWidg.leVariation->setToolTip(variationToolTip);
  m_designWidg.lbVariation->setToolTip(variationToolTip);

  QString acceptToolTip =
    "Spectra whose total number of counts in the background region is this number\n"
    "of times the median number of counts would be marked bad (sets property\n"
    "HighThreshold when MedianDetectorTest is run)";
  m_designWidg.lbAcceptance->setToolTip(acceptToolTip);
  m_designWidg.leAcceptance->setToolTip(acceptToolTip);

  QString startTToolTip =
    "An x-value in the bin marking the start of the background region, the\n"
    "selection is exclusive (RangeLower in MedianDetectorTest)";
  m_designWidg.lbStartTime->setToolTip(startTToolTip);
  m_designWidg.leStartTime->setToolTip(startTToolTip);
  QString endTToolTip =
    "An x-value in the bin marking the the background region's end, the selection\n"
    "is exclusive (RangeUpper in MedianDetectorTest)";
  m_designWidg.lbEndTime->setToolTip(endTToolTip);
  m_designWidg.leEndTime->setToolTip(endTToolTip);
  m_designWidg.ckZeroCounts->setToolTip(
    "Check this and spectra with zero counts in the background region will be"
    "considered bad");
}
void MWDiag::connectSignals(const QWidget * const parentInterface)
{// connect all the open file buttons to an open file dialog connected to it's line edit box
  QSignalMapper *signalMapper = new QSignalMapper(this);
  signalMapper->setMapping(m_designWidg.pbOFile, QString("OutputFile"));
  connect(m_designWidg.pbOFile, SIGNAL(clicked()), signalMapper, SLOT(map()));  
  connect(signalMapper, SIGNAL(mapped(const QString &)),
    this, SLOT(browseClicked(const QString &)));

  // signals connected to the interface that this form is on
  if ( parentInterface != NULL )
  {
    // controls that copy the text from other controls
    connect(parentInterface, SIGNAL(MWDiag_updateWBV(const QString&)),
      m_designWidg.white_file, SLOT(setFileTextWithSearch(const QString&)));
    connect(parentInterface, SIGNAL(MWDiag_updateTOFs(const double &, const double &)),
      this, SLOT(updateTOFs(const double &, const double &)));
    connect(m_designWidg.leStartTime, SIGNAL(editingFinished()), this, SLOT(TOFUpd()));
    connect(m_designWidg.leEndTime, SIGNAL(editingFinished()), this, SLOT(TOFUpd()));

    connect(parentInterface, SIGNAL(MWDiag_sendRuns(const QStringList&)),
      this, SLOT(specifyRuns(const QStringList &)));
  }
}
void MWDiag::setUpValidators()
{
  // Attach number validators to everything that will only accept a number
  m_designWidg.leSignificance->setValidator(new QDoubleValidator(this));
  m_designWidg.leHighAbs->setValidator(new QDoubleValidator(this));
  m_designWidg.leLowAbs->setValidator(new QDoubleValidator(this));
  m_designWidg.leHighMed->setValidator(new QDoubleValidator(this));
  m_designWidg.leLowMed->setValidator(new QDoubleValidator(this));
  m_designWidg.leVariation->setValidator(new QDoubleValidator(this));
  m_designWidg.leAcceptance->setValidator(new QDoubleValidator(this));
  m_designWidg.leStartTime->setValidator(new QDoubleValidator(this));
  m_designWidg.leEndTime->setValidator(new QDoubleValidator(this));
}

/**
* Returns true if the input on the form is valid, false otherwise
*/
bool MWDiag::isInputValid() const
{
  bool valid(m_designWidg.maskFileFinder->isValid());

  valid &= m_designWidg.white_file->isValid();
  valid &= m_designWidg.white_file_2->isValid();

  if(m_designWidg.ckDoBack->isChecked() && m_monoFiles.isEmpty() )
  {
    valid = false;
  }

  return valid;
}

//this function will be replaced a function in a widget
void MWDiag::browseClicked(const QString &buttonDis)
{
  QLineEdit *editBox(NULL);
  QStringList extensions;
  bool toSave = false;
  if ( buttonDis == "OutputFile")
  {
    editBox = m_designWidg.leOFile;
    extensions << "msk";
    toSave = true;
  }

  QString filepath = openFileDialog(toSave, extensions);
  if( filepath.isEmpty() ) return;
  QWidget *focus = QApplication::focusWidget();
  editBox->setFocus();
  editBox->setText(filepath);
  if( focus )
  {
    focus->setFocus();
  }
  else
  {
    this->setFocus();
  }
}

/**
* Create a diagnostic script from the given
*/
QString MWDiag::createDiagnosticScript() const
{
  // Be nice and explicit so that this is as easy as possible to read later
  // Pull out the for data first
  QString sampleRun;
  if( m_designWidg.ckDoBack )
  {
    if( m_monoFiles.count() == 1 )
    {
      sampleRun = "r'" + m_monoFiles[0] + "'";
    }
    else
    {
      if( m_sumMono )
      {
        sampleRun = "[r'";
        sampleRun += m_monoFiles.join("',r'");
        sampleRun += "']";
      }
      else
      {
        throw std::runtime_error("Diagnostic interface does not support multiple mono files without summing. ");
      }
    }
  }
  else
  {
    sampleRun = "None";
  }
  QString whiteBeam = "r'" + m_designWidg.white_file->getFirstFilename() + "'";
  QString whiteBeam2 = "r'" + m_designWidg.white_file_2->getFirstFilename() + "'";
  if( whiteBeam2 == "r''" ) whiteBeam2 = "None";
  QString removeZeroes = m_designWidg.ckZeroCounts->isChecked() ? "True" : "False";
  QString lowCounts = m_designWidg.leLowAbs->text();
  QString highCounts = m_designWidg.leHighAbs->text();
  QString lowMedian = m_designWidg.leLowMed->text();
  QString highMedian = m_designWidg.leHighMed->text();
  QString significance = m_designWidg.leSignificance->text();
  QString acceptance = m_designWidg.leAcceptance->text();
  QString bkgdRange = QString("[%1,%2]").arg(m_designWidg.leStartTime->text(),m_designWidg.leEndTime->text());
  QString variation = m_designWidg.leVariation->text();
  QString hard_mask_file = "r'" + m_designWidg.maskFileFinder->getFirstFilename() + "'";
  if( hard_mask_file == "r''" ) hard_mask_file = "None";
  QString bleed_maxrate = m_designWidg.bleed_maxrate->text();
  QString bleed_pixels = m_designWidg.ignored_pixels->text();

  QString diagCall =
    "from DirectEnergyConversion import setup_reducer\n"
    "from mantid import config\n"
    "reducer = setup_reducer(config['default.instrument'])\n"
    "reducer.prop_man.log_to_mantid = True\n"
    "diag_total_mask = reducer.diagnose(";

  if( m_designWidg.ckDoBack->isChecked() )
  {
    // Do the background check so we need all fields
    diagCall += 
      whiteBeam + "," + sampleRun + ","
      "samp_zero=" + removeZeroes + ","
      "tiny=" + lowCounts + ","
      "huge=" + highCounts + ","
      "van_lo=" + lowMedian + ","
      "van_hi=" + highMedian + ","
      "samp_sig=" + significance + ","
      "samp_hi=" + acceptance + ","
      "bkgd_range=" + bkgdRange + ","
      "variation=" + variation + ","
      "hard_mask=" + hard_mask_file;
  }
  else
  {
    // No background check so don't need all of the fields
    diagCall += 
      whiteBeam + ","
      "tiny=" + lowCounts + ","
      "huge=" + highCounts + ","
      "van_lo=" + lowMedian + ","
      "van_hi=" + highMedian + ","
      "samp_sig=" + significance + ","
      "hard_mask=" + hard_mask_file;
  }

  // Bleed correction
  if( m_designWidg.bleed_group->isChecked() )
  {
    diagCall += 
      ",bleed_test=True,"
      "bleed_maxrate=" + bleed_maxrate + ","
      "bleed_pixels=" + bleed_pixels;
  }
  else
  {
    diagCall += ",bleed_test=False";
  }

  // Print results argument and Closing  argument bracket
  diagCall += ", print_diag_results=True)\n";

  return diagCall;
}

/**
* Show the test result dialog
*/
void MWDiag::showTestResults(const QString & testSummary) const
{
  if( !m_dispDialog )
  {
    m_dispDialog = new DiagResults(this->parentWidget());
    connect(m_dispDialog, SIGNAL(runAsPythonScript(const QString&, bool)), this,
      SIGNAL(runAsPythonScript(const QString&, bool)));
  }

  m_dispDialog->updateResults(testSummary);
  m_dispDialog->show();
}

/** close the results window, if there is one open
*/
void MWDiag::closeDialog()
{
  if (m_dispDialog)
  {
    m_dispDialog->close();
  }
}

/**
*
*/
QString MWDiag::openFileDialog(const bool save, const QStringList &exts)
{
  QString filter;
  if ( !exts.empty() )
  {
    filter = "";
    for ( int i = 0; i < exts.size(); i ++ )
    {
      filter.append("*." + exts[i] + " ");
    }
    filter = filter.trimmed();
  }
  filter.append(";;All Files (*.*)");

  QString filename;
  if( save )
  {
    filename = FileDialogHandler::getSaveFileName(this, "Save file",
      m_prevSets.value("save file dir", "").toString(), filter);
    if( ! filename.isEmpty() )
    {
      m_prevSets.setValue("save file dir", QFileInfo(filename).absoluteDir().path());
    }
  }
  else
  {
    filename = QFileDialog::getOpenFileName(this, "Open file",
      m_prevSets.value("load file dir", "").toString(), filter);
    if( ! filename.isEmpty() )
    {
      m_prevSets.setValue("load file dir", QFileInfo(filename).absoluteDir().path());
    }
  }
  return filename;
} 

/**raises the window containing the results summary, run the Python scripts that
*  have been created and, optionally on success, save the values on the form 
*  @param outWS :: the workspace for placing the information in
*  @param saveSettings :: if the Python executes successfully and this parameter is true the settings are saved
*  @return this method catches most exceptions and this return is main way that errors are reported
*/
QString MWDiag::run(const QString &, const bool)
{
  // close any result window that is still there from a previous run, there might be nothing
  closeDialog();
  // prepare to remove any intermediate workspaces used only during the calculations
  QString prob1;

  if( !isInputValid() )
  {
    throw std::invalid_argument("Invalid input detected. Errors are marked with a red star.");
  }
  QString diagCode = createDiagnosticScript();
  // The results of the diag code execution are captured in the string return of runPythonCode
  QString scriptResults = runPythonCode(diagCode);

  // Now display them to the user if all went well
  // but bail out if not
  if( scriptResults.startsWith("Exception:") )
  {
    return scriptResults;
  }
  // Send the results to the the non-modal dialog
  showTestResults(scriptResults);
  return "";

}

/** Called when the user identifies the background region in a different form, it copies the values over
*  @param start :: the TOF value of the start of the background region
*  @param end :: the TOF value of the end of the background region
*/
void MWDiag::updateTOFs(const double &start, const double &end)
{// if the user added their own value don't change it
  m_sTOFAutoVal = start;
  m_eTOFAutoVal = end;
  if ( ! m_TOFChanged ) 
  {
    m_designWidg.leStartTime->setText(QString::number(start));
    m_designWidg.leEndTime->setText(QString::number(end));
  }
}
/** This slot sets m_monoFiles based on the array that is
*  passed to it
*  @param runFileNames :: names of the files that will be used in the background test
*/
void MWDiag::specifyRuns(const QStringList & runFileNames)
{
  m_monoFiles = runFileNames;
}
/// if the user has changed either of the time of flight values running this method stops the setting from being replaced by the default
void MWDiag::TOFUpd()
{// if the user had already altered the contents of the box it has been noted that the save name is under user control so do nothing
  if (m_TOFChanged) return;
  m_TOFChanged = (m_designWidg.leStartTime->text().toDouble() != m_sTOFAutoVal)
    || (m_designWidg.leEndTime->text().toDouble() != m_eTOFAutoVal);
}
