#include "MantidQtMantidWidgets/MWDiag.h"
#include "MantidQtMantidWidgets/MWDiagCalcs.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidAPI/FrameworkManager.h"
#include <QSignalMapper>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QSettings>
#include <QStringList>
#include <QMessageBox>

using namespace Mantid::Kernel;
using namespace MantidQt::API;
using namespace MantidQt::MantidWidgets;

// default parameters writen to in the GUI
const char defHighAbsolute[5] = "1e10";
const char defLowAbsolute[2] = "0";
const char defSignificanceTest[4] = "3.3";
const char defHighMedian[4] = "3.0";
const char defLowMedian[4] = "0.1";
const char defVariation[4] = "1.1";
const char defBackground[4] = "5.0";
bool rejectZeroBG = true;

MWDiag::MWDiag(QWidget *parent) : m_dispDialog(NULL),
  m_WBVChanged(false), m_outputWS()
{
  m_designWidg.setupUi(this);
  setDefaults();
  setupToolTips();
  connectSignals(parent);
}

void MWDiag::setDefaults()
{
  m_designWidg.ckZeroCounts->setChecked(rejectZeroBG);
  
  //defaults that depend on the instrument
  m_designWidg.leStartTime->setText("18000");
  m_designWidg.leEndTime->setText("19500");
}
  
void MWDiag::setupToolTips()
{  
  // deal with each input control in turn doing,
  //   go through each control and add (??previous?? or) default values
  //   add tool tips
  //   store any relation to algorthim properties as this will be used for validation
  QString iFileToolTip = "A file containing a list of spectra numbers which we aleady know should be masked";
  m_designWidg.lbIFile->setToolTip(iFileToolTip);
  m_designWidg.leIFile->setToolTip(iFileToolTip);
  m_designWidg.pbIFile->setToolTip(iFileToolTip);
  
  QString oFileToolTip =
    "The name of a file to write the spectra numbers of those that fail a test";
  m_designWidg.lbOFile->setToolTip(oFileToolTip);
  m_designWidg.leOFile->setToolTip(oFileToolTip);
  m_designWidg.pbOFile->setToolTip(oFileToolTip);
  
  m_designWidg.leSignificance->setText(defSignificanceTest);
  QString significanceToolTip =
    "Spectra with integrated counts within this number of standard deviations from\n"
    "the median will not be labelled bad (sets property SignificanceTest when\n"
    "MedianDetectorTest is run)";
  m_designWidg.leSignificance->setToolTip(significanceToolTip);
  m_designWidg.lbError->setToolTip(significanceToolTip);
//-------------------------------------------------------------------------------------------------
  QString WBV1ToolTip =
    "The name of a white beam vanadium run from the instrument of interest";
  m_designWidg.lbWBV1->setToolTip(WBV1ToolTip);
  m_designWidg.leWBV1->setToolTip(WBV1ToolTip);
  m_designWidg.pbWBV1->setToolTip(WBV1ToolTip);

  m_designWidg.leHighAbs->setText(defHighAbsolute);
  QString highAbsSetTool =
    "Reject any spectrum that contains more than this number of counts in total\n"
    "(sets property HighThreshold when FindDetectorsOutsideLimits is run)";
  m_designWidg.leHighAbs->setToolTip(highAbsSetTool);
  m_designWidg.lbHighAbs->setToolTip(highAbsSetTool);
  
  m_designWidg.leLowAbs->setText(defLowAbsolute);
  QString lowAbsSetTool =
    "Reject any spectrum that contains less than this number of counts in total\n"
    "(sets property LowThreshold when FindDetectorsOutsideLimits is run)";
  m_designWidg.leLowAbs->setToolTip(lowAbsSetTool);
  m_designWidg.lbLowAbs->setToolTip(lowAbsSetTool);

  m_designWidg.leHighMed->setText(defHighMedian);
  QString highMedToolTip =
    "Reject any spectrum whose total number of counts is more than this number of\n"
    "times the median total for spectra (sets property HighThreshold when\n"
    "MedianDetectorTest is run)";
  m_designWidg.leHighMed->setToolTip(highMedToolTip);
  m_designWidg.lbHighMed->setToolTip(highMedToolTip);

  m_designWidg.leLowMed->setText(defLowMedian);
  QString lowMedToolTip =
    "Reject any spectrum whose total number of counts is less than this number of\n"
    "times the median total for spectra (sets property LowThreshold when\n"
    "MedianDetectorTest is run)";
  m_designWidg.leLowMed->setToolTip(lowMedToolTip);
  m_designWidg.lbLowMed->setToolTip(lowMedToolTip);
//-------------------------------------------------------------------------------------------------
  QString WBV2ToolTip =
    "The name of a white beam vanadium run from the same instrument as the first";
  m_designWidg.lbWBV2->setToolTip(WBV2ToolTip);
  m_designWidg.leWBV2->setToolTip(WBV2ToolTip);
  m_designWidg.pbWBV2->setToolTip(WBV2ToolTip);

  m_designWidg.leVariation->setText(defVariation);
  QString variationToolTip = 
    "When comparing equilivient spectra in the two white beam vanadiums reject any\n"
    "whose the total number of counts varies by more than this multiple of the\n"
    "medain variation (sets property Variation when DetectorEfficiencyVariation is\n"
    "is run)";
  m_designWidg.leVariation->setToolTip(variationToolTip);
  m_designWidg.lbVariation->setToolTip(variationToolTip);

  m_designWidg.leAcceptance->setText(defBackground);
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
  signalMapper->setMapping(m_designWidg.pbWBV1, QString("WBVanadium1"));
  signalMapper->setMapping(m_designWidg.pbWBV2, QString("WBVanadium2"));
  connect(m_designWidg.pbIFile, SIGNAL(clicked()), signalMapper, SLOT(map()));
  connect(m_designWidg.pbOFile, SIGNAL(clicked()), signalMapper, SLOT(map()));
  connect(m_designWidg.pbWBV1, SIGNAL(clicked()), signalMapper, SLOT(map()));
  connect(m_designWidg.pbWBV2, SIGNAL(clicked()), signalMapper, SLOT(map()));
  
  connect(signalMapper, SIGNAL(mapped(const QString &)),
         this, SLOT(browseClicked(const QString &)));

  // deal with controls that copy the text from other controls
  if ( parentInterface != NULL )
  {
    connect(m_designWidg.leWBV1, SIGNAL(editingFinished()), this, SLOT(WBVUpd()));
    connect(parentInterface, SIGNAL(MWDiag_updateWBV(const QString&)), this, SLOT(updateWBV(const QString&)));
  }
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
  if ( buttonDis == "WBVanadium1")
  {
    editBox = m_designWidg.leWBV1;
	extensions << "raw" << "RAW" << "NXS" << "nxs";
  }
  if ( buttonDis == "WBVanadium2")
  {
    editBox = m_designWidg.leWBV2;
	extensions << "raw" << "RAW" << "NXS" << "nxs";
  }
  
  if( ! editBox->text().isEmpty() )
  {
    QString dir = QFileInfo(editBox->text()).absoluteDir().path();
    //STEVES use QSettings to store the last entry instead of the line below
//  MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(dir);
  }  

  QString filepath = openFileDialog(toSave, extensions);
  if( filepath.isEmpty() ) return;
  editBox->setText(filepath);
  
  if ( buttonDis == "WBVanadium1")
  {
    WBVUpd();
  }
}
/** create, setup and show a dialog box that reports the number of bad
* detectors.
* @param summaryInfo the results from running each the test
*/
void MWDiag::raiseDialog()
{// uses new to create the form but the form should have setAttribute(Qt::WA_DeleteOnClose) and so the memory will be freed
  m_dispDialog = new DiagResults(this);
  connect(m_dispDialog, SIGNAL(runAsPythonScript(const QString&)),
                  this, SIGNAL(runAsPythonScr(const QString&)));
  m_dispDialog->show();
}
// ??STEVES?? get rid of this function?
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
    filename = QFileDialog::getSaveFileName(this, "Save file", AlgorithmInputHistory::Instance().getPreviousDirectory(), filter);
  }
  else
  {
    filename = QFileDialog::getOpenFileName(this, "Open file", AlgorithmInputHistory::Instance().getPreviousDirectory(), filter);
  }

  if( !filename.isEmpty() ) 
  {
    AlgorithmInputHistory::Instance().setPreviousDirectory(QFileInfo(filename).absoluteDir().path());
  }
  return filename;
}
bool MWDiag::run()
{
  // these objects read the user settings in the GUI on construction
  whiteBeam1 firstTest(m_designWidg);
  whiteBeam2 optional1(m_designWidg);
  // handle requests to run Python scripts by passing them up to the parent interface
  connect(&firstTest, SIGNAL(runAsPythonScript(const QString&)),
                this, SIGNAL(runAsPythonScr(const QString&)));
  connect(&optional1, SIGNAL(runAsPythonScript(const QString&)),
                this, SIGNAL(runAsPythonScr(const QString&)));

    QMessageBox::critical(this, this->windowTitle(), firstTest.python());
//  background optional2(m_designWidg);connect it too
  // these structures are used to report progress and pass results from one test to another, at the moment there is no progress
  DiagResults::TestSummary sumFirst("First white beam test");
  DiagResults::TestSummary sumOption1("Second white beam test");
	  
  bool noErrorFound = true;
  if ( firstTest.invalid().size() > 0 )
  {
    noErrorFound = false;
    //??STEVES?? display validator labels
  }
  
  // the two tests below are optional dependent on the information supplied by the user
  bool doOpt1 = ! m_designWidg.leWBV2->text().isEmpty();
  if ( doOpt1 && optional1.invalid().size() > 0 )
  {
    noErrorFound = false;
    //??STEVES?? display validator labels
  }
  
//  if ( optional2.invalid().size() > 0 )
//  {
//    noErrorFound = false;
//    //?? STEVES ?? display validator labels
//	return false;
//  }
  
  if ( ! noErrorFound )
  {
    //??STEVES?? remove this as the validator labels will do the job  
    if ( firstTest.invalid().size() > 0 )
	{
	  QString error = "Problem " + QString::fromStdString(firstTest.invalid().begin()->second);
	  QMessageBox::information(this, "", error);
	}
	else
	{//??STEVES optional1.invalid().size()
	QString error = "Problem " + QString::fromStdString(optional1.invalid().begin()->second);
	  QMessageBox::information(this, "", error);
	  }
    return false;
  }

  // the input is good bring up the status window
  raiseDialog();

  // report to the dialog what's happening
  sumFirst.status = "Analysing white beam vanadium 1";

  if ( ! m_dispDialog ) return false;
  m_dispDialog->notifyDialog(sumFirst);

  QString errors = sumFirst.pythonResults(firstTest.run());
  if ( ! errors.isEmpty() || ! m_dispDialog )
  {
    QMessageBox::critical(this, this->windowTitle(), errors);
	return false;
  }
  m_dispDialog->notifyDialog(sumFirst);
  // value of the output workspace maybe overwriten below if further tests are done
  m_outputWS = sumFirst.outputWS;
  
  if (doOpt1)
  {
    
    sumOption1.status = "Analysing white beam vanadium 2 and comparing";
    if ( ! m_dispDialog ) return false;
    m_dispDialog->notifyDialog(sumOption1);

    // adds the output workspace from the first test to the current script
	optional1.incPrevious(sumFirst);

	errors = sumOption1.pythonResults(optional1.run());
    if ( ! errors.isEmpty() || ! m_dispDialog )
	{
      QMessageBox::critical(this, this->windowTitle(), errors);
	  return false;
    }
    m_dispDialog->notifyDialog(sumOption1);
    m_outputWS = sumOption1.outputWS;
  }
//  runPythonCode(optional2.python());

  // clean up tempory workspaces that were used in the calculations
  Mantid::API::FrameworkManager::Instance().deleteWorkspace(sumFirst.inputWS.toStdString());
  if (doOpt1)
  {
    Mantid::API::FrameworkManager::Instance().deleteWorkspace(sumOption1.inputWS.toStdString());
  }
  // return the success flag
  return true;
}
QString MWDiag::getOutputWS() const
{
  return m_outputWS;
}
void MWDiag::updateWBV(const QString &WBVSuggestion)
{// if the user added their own value don't change it
  if ( ! m_WBVChanged ) 
  {
    m_designWidg.leWBV1->setText(WBVSuggestion);
  }
}
void MWDiag::WBVUpd()
{// if the user had already altered the contents of the box it has been noted that the save name is under user control so do nothing
  if (m_WBVChanged) return;
  m_WBVChanged = m_designWidg.leWBV1->text() != "C:/mantid/Test/Data//MAR11060.RAW";
}