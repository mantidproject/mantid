#include "MantidQtMantidWidgets/MWDiagCalcs.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ConfigService.h"
#include <QDir>


using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace MantidQt::MantidWidgets;

const QString whiteBeam1::tempWS = "_Diag_temporyWS_WBV1_";

/** Read the data the user supplied to create Python code to do their calculation
* @param interface a GUI object whose runAsPythonScript signal connected to MantidPlot
* @param userSettings the form that the user filled in
* @param WBVFile name of the file that contains the white beam vanadium data that will be analysised
* @param instru code for the instrument to use e.g. MAR for mari
* @param WSName the name to give the workspace that will remain after this script has completed
*/
whiteBeam1::whiteBeam1(QWidget * const interface, const Ui::MWDiag &userSettings, const QString &WBVFile, const QString &instru, const QString &WSName) :
  pythonCalc(interface)
{
  // these are the objects that will validate user settings
  IAlgorithm_sptr out =
    AlgorithmManager::Instance().createUnmanaged("FindDetectorsOutsideLimits");
  out->initialize();
  IAlgorithm_sptr med =
    AlgorithmManager::Instance().createUnmanaged("MedianDetectorTest");
  med->initialize();
  FileProperty hardMask("Filename", "", FileProperty::OptionalLoad);
  appendChk(userSettings.leIFile, &hardMask);

  // the validate the user's entries
  appendChk(userSettings.leLowAbs, out->getProperty("LowThreshold"));
  appendChk(userSettings.leHighAbs, out->getProperty("HighThreshold"));
  appendChk(userSettings.leHighMed, med->getProperty("HighThreshold"));
  appendChk(userSettings.leLowMed, med->getProperty("LowThreshold"));
  appendChk(userSettings.leSignificance, med->getProperty("SignificanceTest"));

  // load the Python function that will be run
  QDir scriptsdir(QString::fromStdString(ConfigService::Instance().getString("pythonscripts.directory")));
  QString pythonFileName =
    scriptsdir.absoluteFilePath("Inelastic/DetectorTestLib.py");
  loadFile(pythonFileName);
 
  // validate the user entries and create the Python script for this analysis
  m_pyScript.append("\nmtd.sendLogMessage(\"--First white beam test is Python command: ");
  addDiagnoseFunc(userSettings, WBVFile, instru, WSName);  
  m_pyScript.append(" --\")");

  m_pyScript.append("\nprint ");
  addDiagnoseFunc(userSettings, WBVFile, instru, WSName);
}
/** Constructs a Python diagnose() command to perform detector tests on a white beam vanadium file
* @param userSettings the form that the user filled in
* @param WBVFile name of the file that contains the white beam vanadium data that will be analysised
* @param instru code for the instrument to use e.g. MAR for mari
* @param WSName the name to give the workspace that will remain after this script has completed
*/
void whiteBeam1::addDiagnoseFunc(const Ui::MWDiag &userSettings, const QString &WBVFile, const QString &instru, const QString &WSName)
{
  //now create a script based on those entries
  m_pyScript.append("diagnose(");
  // the validation on WBVFile should have already happened before now
  m_pyScript.append("'"+instru+"'");

  m_pyScript.append(", maskFile = '"+userSettings.leIFile->text()+"'");

  // the validation on WBVFile should have already happened before now
  m_pyScript.append(", wbrf = r'"+WBVFile+"'");
  m_pyScript.append(", out_asc = r'"+userSettings.leOFile->text()+"'");
  m_pyScript.append(", tiny = '"+userSettings.leLowAbs->text()+"'");
  m_pyScript.append(", huge = '"+userSettings.leHighAbs->text()+"'");
  m_pyScript.append(", median_hi = '"+userSettings.leHighMed->text()+"'");
  m_pyScript.append(", median_lo = '"+userSettings.leLowMed->text()+"'");
  m_pyScript.append(", sv_sig = '"+userSettings.leSignificance->text()+"'");
  m_pyScript.append(", outWS = '"+WSName+"'");
  m_pyScript.append(")");
}

const QString whiteBeam2::tempWS = "_Diag_temporyWS_WBV2_";
/** Read the data the user supplied to create Python code to do their calculation
* @param userSettings the form that the user filled in
*/
whiteBeam2::whiteBeam2(QWidget * const interface, const Ui::MWDiag &userSettings, const QString &inFile, const QString &instru, const QString &WSName) :
  pythonCalc(interface), m_settings(userSettings)
{  // load the Python function that will be run
  QDir scriptsdir(QString::fromStdString(ConfigService::Instance().getString("pythonscripts.directory")));
  QString pythonFileName =
    scriptsdir.absoluteFilePath("Inelastic/DetectorTestLib.py");
  loadFile(pythonFileName);
  
  // validate the user entries and create the Python script for this analysis
  m_pyScript.append("\nmtd.sendLogMessage(\"--Second white beam test is Python command: ");
  addDiagnoseFunc(userSettings, inFile, instru, WSName); 
  m_pyScript.append(", prevList = ... \")");

  m_pyScript.append("\nprint ");
  addDiagnoseFunc(userSettings, inFile, instru, WSName);
}
/** Constructs a Python diagnose() command to perform detector tests on a second white beam vanadium file
* @param userSettings the form that the user filled in
* @param WBVFile name of the file that contains the white beam vanadium data that will be analysised
* @param instru code for the instrument to use e.g. MAR for mari
* @param WSName the name to give the workspace that will remain after this script has completed
*/
void whiteBeam2::addDiagnoseFunc(const Ui::MWDiag &userSettings, const QString &inFile, const QString &instru, const QString &WSName)
{
  m_pyScript.append("diagnose(");

  //the following user settings should already have been validated by the previous function
  m_pyScript.append("'"+instru+"'");
  m_pyScript.append(", wbrf2 = r'"+inFile+"'");
  m_pyScript.append(", out_asc = r'"+userSettings.leOFile->text()+"'");
  m_pyScript.append(", tiny = '"+userSettings.leLowAbs->text()+"'");
  m_pyScript.append(", huge = '"+userSettings.leHighAbs->text()+"'");
  m_pyScript.append(", median_hi = '"+userSettings.leHighMed->text()+"'");
  m_pyScript.append(", median_lo = '"+userSettings.leLowMed->text()+"'");
  m_pyScript.append(", sv_sig = '"+userSettings.leSignificance->text()+"'");
  m_pyScript.append(", outWS = '"+WSName+"'");
}
/** Copy in the names of the input mask and workspace created in the
*  first white beam vanadium test
*  @param firstTest results created from the first test python print statments
*/
void whiteBeam2::incPrevious(const DiagResults::TestSummary &firstTest)
{
  m_pyScript.append(", prevList = '"+firstTest.listBad+"'");
  m_pyScript.append(", previousWS = '"+firstTest.inputWS+"'");
  m_pyScript.append(")");
}

const QString backTest::tempWS = "_Diag_temporyWS_back_";

/** Read the data the user supplied to create Python code to do their calculation
* @param userSettings the form that the user filled in
*/
backTest::backTest(QWidget * const interface, const Ui::MWDiag &userSettings, const QStringList & runs, const QString &instru, const QString &WSName) :
  pythonCalc(interface), m_settings(userSettings)
{
  if (runs.count() == 0)
  {
    throw std::invalid_argument("No input files have been specified, uncheck \"Check run backgrounds\" to continue");
  }

  IAlgorithm_sptr inte =
    AlgorithmManager::Instance().createUnmanaged("Integration");
  inte->initialize();
  appendChk(m_settings.leStartTime, inte->getProperty("RangeLower"));
  appendChk(m_settings.leEndTime, inte->getProperty("RangeUpper") );
  
  IAlgorithm_sptr med =
  AlgorithmManager::Instance().createUnmanaged("MedianDetectorTest");
  med->initialize();
  appendChk(userSettings.leAcceptance, med->getProperty("HighThreshold"));

  // load the Python function that will be run
  QDir scriptsdir(QString::fromStdString(ConfigService::Instance().getString("pythonscripts.directory")));
  QString pythonFileName =
    scriptsdir.absoluteFilePath("Inelastic/DetectorTestLib.py");
  loadFile(pythonFileName);
 
  // validate the user entries and create the Python script for this analysis
  m_pyScript.append("\nmtd.sendLogMessage(\"--Background test is Python command: ");
  addDiagnoseFunc(userSettings, runs, instru, WSName); 
  m_pyScript.append(", prevList = ... \")");

  m_pyScript.append("\nprint ");
  addDiagnoseFunc(userSettings, runs, instru, WSName);
}
void backTest::addDiagnoseFunc(const Ui::MWDiag &userSettings, const QStringList &runs, const QString &instru, const QString &WSName)
{  m_pyScript.append("diagnose(");

  //the following user settings should already have been validated by the previous function
  m_pyScript.append("'"+instru+"'");  

  m_pyScript.append(", runs = r'" + runs.join(",")+"'");
  m_pyScript.append(", zero = ");
  if ( userSettings.ckZeroCounts->isChecked() )
  {
    m_pyScript.append("True");
  }
  else
  {
    m_pyScript.append("False");
  }
  m_pyScript.append(", out_asc = r'"+userSettings.leOFile->text()+"'");
  m_pyScript.append(", bmin = '"+m_settings.leStartTime->text()+"'");
  m_pyScript.append(", bmax = '"+m_settings.leEndTime->text()+"'");
  m_pyScript.append(", tiny = '"+userSettings.leLowAbs->text()+"'");
  m_pyScript.append(", huge = '"+userSettings.leHighAbs->text()+"'");
  m_pyScript.append(", median_hi = '"+userSettings.leHighMed->text()+"'");
  m_pyScript.append(", median_lo = '"+userSettings.leLowMed->text()+"'");
  m_pyScript.append(", sv_sig = '"+userSettings.leSignificance->text()+"'");
  m_pyScript.append(", bmedians = '"+userSettings.leAcceptance->text()+"'");
  m_pyScript.append(", outWS = '"+WSName+"'");
}
void backTest::incFirstTest(const DiagResults::TestSummary &results)
{
  m_pyScript.append(", prevList = '"+results.listBad+"'");
  m_pyScript.append(")");
}
void backTest::incSecondTest(const DiagResults::TestSummary &results, const QString &WS)
{
  m_pyScript.append(", prevList = '"+results.listBad+"'");
  m_pyScript.append(", previousWS = '"+WS+"'");
  m_pyScript.append(")");
}
