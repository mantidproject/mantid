#include "MantidQtMantidWidgets/MWDiagCalcs.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidKernel/FileProperty.h"
#include "MantidKernel/ConfigService.h"
#include <QDir>
#include <vector>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace MantidQt::MantidWidgets;

const QString whiteBeam1::tempWS = "_Diag_temporyWS_WBV1_";

/** Read the data the user supplied to create Python code to do their calculation
* @param userSettings the form that the user filled in
*/
whiteBeam1::whiteBeam1(QWidget * const interface, const Ui::MWDiag &userSettings) :
  pythonCalc(interface), m_settings(userSettings)
{
  QDir scriptsdir(QString::fromStdString(ConfigService::Instance().getString("pythonscripts.directory")));
  // load a template for the Python script that we will contain in a string
  QString pythonFileName =
    scriptsdir.absoluteFilePath("Excitations/diagnose/whitebeam1test.py");
  readFile(pythonFileName);
  // we make a copy of code we read from the file because we might replace some terms but later need to repeat this operation
  m_pyScript = m_templateH;
  m_pyScript.append(m_templateB);
   
  std::vector<std::string> exts;
  exts.push_back("raw"); exts.push_back("RAW");
  exts.push_back("NXS"); exts.push_back("nxs");
  FileProperty loadData("Filename", "", FileProperty::Load, exts);
  LEChkCp("|WBVANADIUM1|", m_settings.leWBV1, &loadData);

  IAlgorithm_sptr out =
    AlgorithmManager::Instance().createUnmanaged("FindDetectorsOutsideLimits");
  out->initialize();
  IAlgorithm_sptr med =
    AlgorithmManager::Instance().createUnmanaged("MedianDetectorTest");
  med->initialize();

  LEChkCp("|HIGHABSOLUTE|", m_settings.leHighAbs,
    out->getProperty("HighThreshold") );
  LEChkCp("|LOWABSOLUTE|", m_settings.leLowAbs,
    out->getProperty("LowThreshold") );
  LEChkCp("|HIGHMEDIAN|",m_settings.leHighMed,
    med->getProperty("HighThreshold") );

  LEChkCp("|LOWMEDIAN|", m_settings.leLowMed,
    med->getProperty("LowThreshold") );
 
  LEChkCp("|SIGNIFICANCETEST|", m_settings.leSignificance,
    med->getProperty("SignificanceTest") );

  FileProperty hardMask("Filename", "", FileProperty::OptionalLoad);
  LEChkCp("|INPUTFILE|", m_settings.leIFile, &hardMask);
  
  m_pyScript.replace("|OUTPUTFILE|", "'"+m_settings.leOFile->text()+"'");
}

const QString whiteBeam2::tempWS = "_Diag_temporyWS_WBV2_";

/** Read the data the user supplied to create Python code to do their calculation
* @param userSettings the form that the user filled in
*/
whiteBeam2::whiteBeam2(QWidget * const interface, const Ui::MWDiag &userSettings, const QString &inFile) :
  pythonCalc(interface), m_settings(userSettings)
{
  QDir scriptsdir(QString::fromStdString(ConfigService::Instance().getString("pythonscripts.directory")));
  // load a template for the Python script that we will contain in a string
  QString pythonFileName =
    scriptsdir.absoluteFilePath("Excitations/diagnose/whitebeam2test.py");
  readFile(pythonFileName);
  // we make a copy of code we read from the file because we might replace some terms but later need to repeat this operation
  m_pyScript = m_templateH;
  m_pyScript.append(m_templateB);

  m_pyScript.replace("|WBVANADIUM2|", "'"+inFile+"'");
  
  IAlgorithm_sptr var =
    AlgorithmManager::Instance().createUnmanaged("DetectorEfficiencyVariation");
  var->initialize();
  
  LEChkCp( "|CHANGEBETWEEN|", m_settings.leVariation,
    var->getProperty("Variation") );

  // these user entries have already been validated, just do the replacement
  m_pyScript.replace("|HIGHABSOLUTE|", m_settings.leHighAbs->text());
  m_pyScript.replace("|LOWABSOLUTE|", m_settings.leLowAbs->text());
  m_pyScript.replace("|HIGHMEDIAN|", m_settings.leHighMed->text());
  m_pyScript.replace("|LOWMEDIAN|", m_settings.leLowMed->text());
  m_pyScript.replace("|SIGNIFICANCETEST|", m_settings.leSignificance->text());
  
  
  m_pyScript.replace("|OUTPUTFILE|", "'"+m_settings.leOFile->text()+"'");
}
/** Copy in the names of the input mask and workspace created in the
*  first white beam vanadium test
*  @param firstTest results created from the first test python print statments
*/
void whiteBeam2::incPrevious(const DiagResults::TestSummary &firstTest)
{
  m_pyScript.replace("|INPUTMASK|", "'"+firstTest.outputWS+"'");
  m_pyScript.replace("|WBV1|", "'"+firstTest.inputWS+"'");
}

const QString backTest::tempWS = "_Diag_temporyWS_back_";

/** Read the data the user supplied to create Python code to do their calculation
* @param userSettings the form that the user filled in
*/
backTest::backTest(QWidget * const interface, const Ui::MWDiag &userSettings, const std::vector<std::string> &runs) :
  pythonCalc(interface), m_settings(userSettings)
{
  QDir scriptsdir(QString::fromStdString(ConfigService::Instance().getString("pythonscripts.directory")));
  // load a template for the Python script that we will contain in a string
  QString pythonFileName =
    scriptsdir.absoluteFilePath("Excitations/diagnose/backgroundtest.py");
  readFile(pythonFileName);
  // we make a copy of code we read from the file because we might replace some terms but later need to repeat this operation
  m_pyScript = m_templateH;
  m_pyScript.append(m_templateB);
  {// I'm limitting the scope of these algroithms pointers so that I don't mix them up while coding
  IAlgorithm_sptr med =
    AlgorithmManager::Instance().createUnmanaged("MedianDetectorTest");
  med->initialize();
  
  LEChkCp( "|ERRORBARS|", m_settings.leSignificance,
    med->getProperty("SignificanceTest") );
  }
  
  if (runs.size() == 0)
  {
    throw std::invalid_argument("No input files have been specified, uncheck \"Check run backgrounds\" to continue");
  }
  m_pyScript.replace("|EXPFILES|",
    QString::fromStdString(vectorToTupple(runs)));
  
  {// I'm limitting the scope of these algroithms pointers so that I don't mix them up while coding
  IAlgorithm_sptr inte =
    AlgorithmManager::Instance().createUnmanaged("Integration");
  inte->initialize();
  LEChkCp( "|TOF_WIN_LOW|", m_settings.leStartTime,
    inte->getProperty("RangeLower") );
  LEChkCp( "|TOF_WIN_HIGH|", m_settings.leEndTime,
    inte->getProperty("RangeUpper") );
  }
  QString rmZeros = m_settings.ckZeroCounts->isChecked() ? "true" : "false";
  m_pyScript.replace("|REMOVEZEROS|", "'"+rmZeros+"'");
  m_pyScript.replace("|BACKGROUNDACCEPT|", m_settings.leAcceptance->text());
  m_pyScript.replace("|OUTPUTFILE|", "'"+m_settings.leOFile->text()+"'");
}
/** Copy in the names of the input mask and workspace created in the
*  first white beam vanadium test
*  @param results1 summary of results created from the first test python print statments
*/
void backTest::incFirstTest(const DiagResults::TestSummary &results1)
{
  m_pyScript.replace("|MASK1|", "'"+results1.outputWS+"'");
  m_pyScript.replace("|WBV1|", "'"+results1.inputWS+"'");
}
/** Copy in the names of the input mask and workspace created in the
*  second white beam vanadium test
*  @param results2 summary of results created from the first test python print statments
*/
void backTest::incSecondTest(const DiagResults::TestSummary &results2)
{
  m_pyScript.replace("|MASK2|", "'"+results2.outputWS+"'");
  m_pyScript.replace("|WBV2|", "'"+results2.inputWS+"'");
}
/** Update the script with empty quotes for the results from the second test:
*  the second vanadium workspace and the workspace with masked detectors
*/
void backTest::noSecondTest()
{
  m_pyScript.replace("|MASK2|", "''");
  m_pyScript.replace("|WBV2|", "''");
}