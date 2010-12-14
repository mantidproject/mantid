#include "MantidQtMantidWidgets/MWDiag.h"
#include "MantidQtMantidWidgets/DiagResults.h"
#include "MantidQtMantidWidgets/MWRunFiles.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidQtAPI/FileDialogHandler.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FileProperty.h"

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

// default parameters writen to in the GUI
static const bool NO_SOLIDS = false;
static const char HIGH_ABSOLUTE[5] = "1e10";
static const char LOW_ABSOLUTE[2] = "0";
static const char SIGNIFIC_TEST[4] = "3.3";
static const char HIGH_MEDIAN[4] = "3.0";
static const char LOW_MEDIAN[4] = "0.1";
static const char VARIATION[4] = "1.1";
static const bool CHECK_BACK = true;
static const char BACK_CRITERIA[4] = "5.0";
static const double START_WINDOW_TOF = 18000;
static const double END_WINDOW_TOF = 19500;
static const bool NO_ZERO_BG = true;

MWDiag::MWDiag(QWidget *parent, QString prevSettingsGr, const QComboBox * const instru):
  MantidWidget(parent),
  m_dispDialog(NULL), m_instru(instru), m_WBV1(NULL), m_WBV2(NULL),
  m_TOFChanged(false), m_sTOFAutoVal(-1), m_eTOFAutoVal(-1)
{
  // allows saving and loading the values the user entered on to the form
  m_prevSets.beginGroup(prevSettingsGr);
  
  m_designWidg.setupUi(this);
  insertFileWidgs();
  
  loadDefaults();
  setupToolTips();
  setUpValidators();
  connectSignals(parent);
}

void MWDiag::insertFileWidgs()
{
  m_WBV1 = new MWRunFiles(this);
  m_designWidg.indivTestWhiteLay->addWidget(m_WBV1);
  m_WBV1->setLabelText("White Beam Van 1");
  m_WBV1->isOptional(false);
  m_WBV1->allowMultipleFiles(false);

  m_WBV2 = new MWRunFiles(this);
  m_designWidg.effVarTestWhiteLay->addWidget(m_WBV2);
  m_WBV2->setLabelText("White Beam Van 2");
  m_WBV2->isOptional(true);
  m_WBV2->allowMultipleFiles(false);

  connect(m_designWidg.leIFile, SIGNAL(editingFinished()), this, SLOT(validateHardMaskFile()));
  validateHardMaskFile();
}

/// loads default values into each control using either the previous value used when the form was run or the default value for that control
void MWDiag::loadDefaults()
{  
  m_designWidg.leIFile->setText(
    m_prevSets.value("input mask", "").toString());
  m_designWidg.leOFile->setText(
    m_prevSets.value("output file", "").toString());
  m_designWidg.leSignificance->setText(
    m_prevSets.value("significance", SIGNIFIC_TEST).toString());
  m_designWidg.ckAngles->setChecked(
    m_prevSets.value("no solid", NO_SOLIDS).toBool());

  m_designWidg.leHighAbs->setText(
    m_prevSets.value("high abs", HIGH_ABSOLUTE).toString());
  m_designWidg.leLowAbs->setText(
    m_prevSets.value("low abs", LOW_ABSOLUTE).toString());
  m_designWidg.leHighMed->setText(
    m_prevSets.value("high median", HIGH_MEDIAN).toString());
  m_designWidg.leLowMed->setText(
    m_prevSets.value("low median", LOW_MEDIAN).toString());

  m_designWidg.leVariation->setText(
    m_prevSets.value("variation", VARIATION).toString());

  m_designWidg.ckDoBack->setChecked(
    m_prevSets.value("test background", CHECK_BACK).toBool());
  m_designWidg.leAcceptance->setText(
    m_prevSets.value("back criteria", BACK_CRITERIA).toString());
  m_designWidg.ckZeroCounts->setChecked(
    m_prevSets.value("no zero background", NO_ZERO_BG).toBool());
  m_designWidg.leStartTime->setText(
    m_prevSets.value("TOF start", START_WINDOW_TOF).toString());
  m_designWidg.leEndTime->setText(
    m_prevSets.value("TOF end", END_WINDOW_TOF).toString());
}
/// loads default values into each control using either the previous value used when 
/// the form was run or the default value for that control
void MWDiag::saveDefaults()
{
  m_prevSets.setValue("input mask", m_designWidg.leIFile->text());
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
  QString iFileToolTip = "A file containing a list of spectra numbers which we aleady know should be masked";
  m_designWidg.lbIFile->setToolTip(iFileToolTip);
  m_designWidg.leIFile->setToolTip(iFileToolTip);
  m_designWidg.pbIFile->setToolTip(iFileToolTip);
  
  QString oFileToolTip =
    "The name of a file to write the spectra numbers of those that fail a test";
  m_designWidg.lbOFile->setToolTip(oFileToolTip);
  m_designWidg.leOFile->setToolTip(oFileToolTip);
  m_designWidg.pbOFile->setToolTip(oFileToolTip);
  
  QString significanceToolTip =
    "Spectra with integrated counts within this number of standard deviations from\n"
    "the median will not be labelled bad (sets property SignificanceTest when\n"
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
    "When comparing equilivient spectra in the two white beam vanadiums reject any\n"
    "whose the total number of counts varies by more than this multiple of the\n"
    "medain variation (sets property Variation when DetectorEfficiencyVariation is\n"
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
  signalMapper->setMapping(m_designWidg.pbIFile, QString("InputFile"));
  signalMapper->setMapping(m_designWidg.pbOFile, QString("OutputFile"));
  connect(m_designWidg.pbIFile, SIGNAL(clicked()), signalMapper, SLOT(map()));
  connect(m_designWidg.pbOFile, SIGNAL(clicked()), signalMapper, SLOT(map()));  
  connect(signalMapper, SIGNAL(mapped(const QString &)),
         this, SLOT(browseClicked(const QString &)));

  // signals connected to the interface that this form is on
  if ( parentInterface != NULL )
  {

    // controls that copy the text from other controls
    connect(parentInterface, SIGNAL(MWDiag_updateWBV(const QString&)),
      m_WBV1, SLOT(setFileText(const QString&)));
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
  bool valid(true);
  if( m_designWidg.valInmsk->isVisible() )
  {
    valid &= false;
  }
  else
  {
    valid &= true;
  }
  
  valid &= m_WBV1->isValid();
  valid &= m_WBV2->isValid();

  if(m_designWidg.ckDoBack->isChecked() && m_monoFiles.isEmpty() )
  {
    valid = false;
  }

  return valid;
}

//this function will be replaced a function in a widget
void MWDiag::browseClicked(const QString &buttonDis)
{
  QLineEdit *editBox;
  QStringList extensions;
  bool toSave = false;
  if ( buttonDis == "InputFile")
  {
    editBox = m_designWidg.leIFile;
  }
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
  QString sampleRun = m_designWidg.ckDoBack->isChecked() ? ("r'" + m_monoFiles[0] + "'") : "";
  QString whiteBeam = "r'" + m_WBV1->getFirstFilename() + "'";
  QString whiteBeam2 = "r'" + m_WBV2->getFirstFilename() + "'";
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

  QString diagCall = 
    "diag_total_mask = diagnostics.diagnose(";
  
  if( m_designWidg.ckDoBack->isChecked() )
  {
    // Do the background check so we need all fields
    diagCall += 
      "white_run=" + whiteBeam + ","
      "sample_run=" + sampleRun + ","
      "other_white=" + whiteBeam2 + ","
      "remove_zero=" + removeZeroes + ","
      "tiny=" + lowCounts + ","
      "large=" + highCounts + ","
      "median_lo=" + lowMedian + ","
      "median_hi=" + highMedian + ","
      "signif=" + significance + ","
      "bkgd_threshold=" + acceptance + ","
      "bkgd_range=" + bkgdRange + ","
      "variation=" + variation;    
  }
  else
  {
    // No background check so don't need all of the fields
    diagCall += 
      "white_run=" + whiteBeam + ","
      "other_white=" + whiteBeam2 + ","
      "tiny=" + lowCounts + ","
      "large=" + highCounts + ","
      "median_lo=" + lowMedian + ","
      "median_hi=" + highMedian + ","
      "signif=" + significance;
  }

  // Print results argument and Closing  argument bracket
  diagCall += ", print_results=True)\n";

  QString pyCode = 
    "import diagnostics\n"
    "try:\n"
    "    " + diagCall + "\n"
    "except RuntimeError, exc:\n"
    "    print 'Exception:'\n"
    "    print str(exc)\n";
  
  return pyCode;
}

/**
 * Show the test result dialog
 */
void MWDiag::showTestResults(const QString & testSummary) const
{
  if( !m_dispDialog )
  {
    m_dispDialog = new DiagResults(this->parentWidget());
    connect(m_dispDialog, SIGNAL(runAsPythonScript(const QString&)), this, 
	    SIGNAL(runAsPythonScript(const QString&)));
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
    filter = "Files (";
    for ( int i = 0; i < exts.size(); i ++ )
    {
      filter.append("*." + exts[i] + " ");
    }
    filter.trimmed();
    filter.append(")");
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
*  @param saveSettings if the Python executes successfully and this parameter is true the settings are saved
*  @return this method catches most exceptions and this return is main way that errors are reported
*/
QString MWDiag::run(const QString &outWS, const bool saveSettings)
{
  // close any result window that is still there from a previous run, there might be nothing
  closeDialog();
  // prepare to remove any intermediate workspaces used only during the calculations
  std::vector<std::string> tempOutputWS;
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



//   try
//   {
//     // these objects read the user settings in the GUI on construction
//     whiteBeam1 firstTest(this, m_designWidg, m_WBV1->getFirstFilename(), m_instru->currentText(), outWS);
//     connect(&firstTest, SIGNAL(runAsPythonScript(const QString&)), this, SIGNAL(runAsPythonScript(const QString &)));
  
//     // the two tests below are optional, dependent on the information supplied by the user
//     boost::shared_ptr<whiteBeam2> optional1;
//     QString prob2 = possibleSecondTest(optional1, outWS);
//     boost::shared_ptr<backTest> optional2;
//     QString prob3 = possibleThirdTest(optional2, outWS);
    
  
//     //if were errors return a description of the first error
//     //stars have already been placed next to any problem input
//     if ( ! prob1.isEmpty() )
//     {
//       throw std::invalid_argument(prob1.toStdString());
//     }
//     if ( ! prob2.isEmpty() )
//     {
//       throw std::invalid_argument(prob2.toStdString());
//     }
//     if ( ! prob3.isEmpty() )
//     {
//       throw std::invalid_argument(prob3.toStdString());
//     }

//     // the input is good bring up the status window
//     raiseDialog();
//     try // run things that are dependent on the dialog box being present, it is deleted whenever the user closes it!
//     {
//       DiagResults::TestSummary sumFirst = singleWhiteBeamTest(firstTest);

//       // must have the same scope as finalTest above, these structures are used to report progress and pass results from one test to another
//       DiagResults::TestSummary sumOption1("");
//       DiagResults::TestSummary sumOption2("");
//       tempOutputWS.push_back(sumFirst.inputWS.toStdString());

//       if ( optional1 )
//       {
//         connect(optional1.get(), SIGNAL(runAsPythonScript(const QString&)), this, SIGNAL(runAsPythonScript(const QString &)));
//         sumOption1 = whiteBeamCompTest(sumFirst, optional1);
//         tempOutputWS.push_back(sumOption1.inputWS.toStdString());
//       }

//       if ( optional2 )
//       {
//         connect(optional2.get(), SIGNAL(runAsPythonScript(const QString&)), this, SIGNAL(runAsPythonScript(const QString &)));
//         sumOption2 = backGroundTest(sumFirst, sumOption1, optional2);
//       }	
//     }
//     catch ( Exception::NullPointerException & )
//     {// the diag has died, probably the user closed it
//       prob1 = "The results window was closed, calculation aborted";
//     }
//     catch (std::exception &e)
//     {
//       return QString(e.what())+"  Exception encountered running detector diagnostic tests";
//     }
//   }
//   catch (std::invalid_argument &e)
//   {
//     return "In detector test:\n" + QString(e.what());
//   }
//   // clean up tempory workspaces that were used in the calculations
//   std::vector<std::string>::const_iterator it = tempOutputWS.begin();
//   for ( ; it !=  tempOutputWS.end(); ++it )
//   {
//     Mantid::API::FrameworkManager::Instance().deleteWorkspace(*it);
//   }
//   //tell the results window it may re-enable its buttons
//   blockPython(false);

//   if (prob1.isEmpty() && saveSettings)
//   {//avoid saving user settings that caused errors but assume there are no errors by this point
//     saveDefaults();
//   }
//   // prob1 should be empty so the next line is equivalent to return ""
//   return prob1;
}

void MWDiag::blockPython(const bool block)
{//don't need to do anything if the dialog box was closed
//   if(m_dispDialog)
//   {
//     m_dispDialog->showButtons( ! block );
//   }
}

// /** Pointers the passed pointer to an object will contain all the Python script for
// *  the two white beam vanadium detector test
// *  @param whiteBeamComp will be pointed at a valid whiteBeam2 object
// *  @param outWS the name of the workspace that will remain after execution
// *  @return any errors or an empty string for no error
// */
// QString MWDiag::possibleSecondTest(boost::shared_ptr<whiteBeam2> &whiteBeamComp, const QString &WSName)
// {  
//   QString whiteBeamFile2 = m_WBV2->getFirstFilename();
//   if ( ! whiteBeamFile2.isEmpty() )
//   {// the user has supplied an input file for the second test so fill the shared_ptr with the script
//     whiteBeamComp.reset(
// 	    new whiteBeam2(this, m_designWidg, whiteBeamFile2, m_instru->currentText(), WSName));
// 	  return "";
//   }
//   else
//   {
//     whiteBeamComp = boost::shared_ptr<whiteBeam2>();
//   }
//   return "";
// }
// /** Pointers the passed pointer to an object will contain all the Python script for
// *  the detector test based on background values
// *  @param backCheck will be pointered to a valid backTest object
// *  @return any errors or an empty string for no error
// */
// QString MWDiag::possibleThirdTest(boost::shared_ptr<backTest> &backCheck, const QString &WSName)
// {
//   if (m_designWidg.ckDoBack->isChecked())
//   { //generate a Python script
//     backCheck.reset(new backTest(this, m_designWidg, m_monoFiles, m_instru->currentText(), WSName));
// 	//report any problems trying to construct it, likely to be problems with the values suggested by the user
// 	return "";
//   }
//   else
//   {
//     backCheck = boost::shared_ptr<backTest>();
//   }
//   return "";
// }
// /** Executes the Python script constructed for the first white beam vanadium test
// *  @param python this object contains a script constructed for this test
// *  @return summary data of the test results
// *  @throw invalid_argument if an exception was thrown during execution of the Python
// */
// DiagResults::TestSummary MWDiag::singleWhiteBeamTest(whiteBeam1 &python)
// {
//   DiagResults::TestSummary results("First white beam test");
//   // report to the dialog what's happening
//   results.status = "Analysing white beam vanadium 1";
//   notifyDialog(results);
  
//   // use the following code to see the script QStringList list1 = python.python().split("\n"); QMessageBox::critical(this, "", *(list1.end()-4)+'\n'+*(list1.end()-3)+'\n'+*(list1.end()-2)+'\n'+*(list1.end()-1));
//   QString error = results.pythonResults(python.run());
  
//   if ( results.status == "success" )
//   {
//     results.status = "White beam vanadium 1 complete";
//   }
//   notifyDialog(results);
  
//   if ( ! error.isEmpty() )
//   {
//     throw std::invalid_argument(error.toStdString());
//   }
//   return results;
// }

// /** Adds data from the first test to the script that was built up for the white beam comparison test
// *  and executes the comparison test
// *  @param firstTest the results from a single white beam vanadium test
// *  @param python this object contains a script constructed for this test
// *  @return summary data of the test results
// *  @throw invalid_argument if an exception was thrown during execution of the Python
// */
// DiagResults::TestSummary MWDiag::whiteBeamCompTest(const DiagResults::TestSummary firstTest, boost::shared_ptr<whiteBeam2> &python)
// {
//   DiagResults::TestSummary results("Second white beam test");
//   results.status = "Analysing white beam vanadium 2 and comparing";
//   notifyDialog(results);

//   // adds the output workspace from the first test to the current script
//   python->incPrevious(firstTest);
//   // use the following code to see the script QStringList list1 = python->python().split("\n"); QMessageBox::critical(this, "", *(list1.end()-4)+'\n'+*(list1.end()-3)+'\n'+*(list1.end()-2)+'\n'+*(list1.end()-1));
//   QString error = results.pythonResults(python->run());
  
//   if ( results.status == "success" )
//   {
//     results.status = "White beam vanadium comparison complete";
//   }
//   notifyDialog(results);

//   if ( ! error.isEmpty() )
//   {
//     throw std::invalid_argument(error.toStdString());
//   }
//   return results;
// }
// /** Adds data from the first test and second tests (the second test results can be default if there was no test to the
// *  script built for the background test and execute the script
// *  @param firstTest the results from a single white beam vanadium test
// *  @param secondTest the results from a white beam comparison test, if this is a default results summary then no second test data is used
// *  @param python this object contains a script constructed for this test
// *  @return summary data of the test results
// *  @throw invalid_argument if an exception was thrown during execution of the Python
// */
// DiagResults::TestSummary MWDiag::backGroundTest(const DiagResults::TestSummary &firstTest, const DiagResults::TestSummary &secondTest, boost::shared_ptr<backTest>& python)
// {
//   DiagResults::TestSummary results("Background test");
//   results.status = "Analysing the background regions of experimental runs";
//   notifyDialog(results);
  
//   if ( secondTest.status != "Error" )
//   {
//     python->incSecondTest(secondTest, secondTest.inputWS);
//   }
//   else
//   {
//     python->incFirstTest(firstTest);
//   }
//   // use the following code to see the script  QStringList list1 = python->python().split("\n"); QMessageBox::critical(this, "", *(list1.end()-4)+'\n'+*(list1.end()-3)+'\n'+*(list1.end()-2)+'\n'+*(list1.end()-1));
//   QString error = results.pythonResults(python->run());
  
//   if ( results.status == "success" )
//   {
//     results.status = "Tests on low flux background complete";
//   }
//   notifyDialog(results);

//   if ( ! error.isEmpty() )
//   {
//     throw std::invalid_argument(error.toStdString());
//   }
//   return results;
// }


/** Called when the user identifies the background region in a different form, it copies the values over
*  @param start the TOF value of the start of the background region
*  @param end the TOF value of the end of the background region
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
*  @param runFileNames names of the files that will be used in the background test
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

/**
 * Validate the hard mask file input
*/
void MWDiag::validateHardMaskFile()
{
  std::string filename = m_designWidg.leIFile->text().toStdString();
  if( filename.empty() )
  {
    m_designWidg.valInmsk->hide();
    return;
  }

  FileProperty *validateHardMask = new FileProperty("UnusedName", filename,FileProperty::Load);
  QString error = QString::fromStdString(validateHardMask->isValid()); 
  if( error.isEmpty() )
  {
    m_designWidg.valInmsk->hide();
  }
  else
  {
    m_designWidg.valInmsk->show();
  }
  m_designWidg.valInmsk->setToolTip(error);
}
