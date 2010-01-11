#include "MantidQtMantidWidgets/MWDiagCalcs.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidKernel/FileProperty.h"
#include "MantidKernel/ConfigService.h"
#include <QDir>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

const QString whiteBeam1::tempWS = "_Diag_tempory_workspace_";
const QString whiteBeam2::tempWS = "_Diag_tempory_workspace_";

/** Read the data the user supplied to create Python code to do their calculation
* @param userSettings the form that the user filled in
*/
whiteBeam1::whiteBeam1(const Ui::MWDiag &userSettings) : m_settings(userSettings),
  pythonCalc()
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
/** Read the data the user supplied to create Python code to do their calculation
* @param userSettings the form that the user filled in
*/
whiteBeam2::whiteBeam2(const Ui::MWDiag &userSettings) : m_settings(userSettings),
  pythonCalc()
{
  QDir scriptsdir(QString::fromStdString(ConfigService::Instance().getString("pythonscripts.directory")));
  // load a template for the Python script that we will contain in a string
  QString pythonFileName =
    scriptsdir.absoluteFilePath("Excitations/diagnose/whitebeam2test.py");
  readFile(pythonFileName);
  // we make a copy of code we read from the file because we might replace some terms but later need to repeat this operation
  m_pyScript = m_templateH;
  m_pyScript.append(m_templateB);
   
  std::vector<std::string> exts;
  exts.push_back("raw"); exts.push_back("RAW");
  exts.push_back("NXS"); exts.push_back("nxs");
  FileProperty loadData("Filename", "", FileProperty::OptionalLoad, exts);
  LEChkCp("|WBVANADIUM2|", m_settings.leWBV2, &loadData);

  IAlgorithm_sptr var =
    AlgorithmManager::Instance().createUnmanaged("DetectorEfficiencyVariation");
  var->initialize();
  
  LEChkCp( "|CHANGEBETWEEN|", m_settings.leSignificance,
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
void whiteBeam2::incPrevious(DiagResults::TestSummary &firstTest)
{
  m_pyScript.replace("|INPUTMASK|", "'"+firstTest.outputWS+"'");
  m_pyScript.replace("|WBV1|", "'"+firstTest.inputWS+"'");
}
