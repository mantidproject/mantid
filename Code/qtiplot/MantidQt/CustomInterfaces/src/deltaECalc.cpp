#include "MantidQtCustomInterfaces/deltaECalc.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/FileProperty.h"
#include <QDir>
#include "boost/lexical_cast.hpp"
#include <cmath>

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::Kernel;
using namespace Mantid::API;

const QString deltaECalc::tempWS = "mono_sample_temporyWS";

/** Read the data the user supplied to create Python code to do their calculation
* @param userSettings the form that the user filled in
* @throw invalid_argument where problems with user data prevent the calculation from proceeding
*/
deltaECalc::deltaECalc(QWidget * const interface, const Ui::ConvertToEnergy &userSettings, 
                       const bool removalBg, const double TOFWinSt, const double TOFWinEnd) :
  pythonCalc(interface), m_sets(userSettings), m_bgRemove(removalBg), m_TOFWinSt(TOFWinSt), m_TOFWinEnd(TOFWinEnd), m_diagnosedWS("")
{
}

/** Adds user values from the GUI into the Python script
* @param inputFiles a coma separated list of data file names
* @param whiteB The filename of the white beam run
*/
void deltaECalc::createProcessingScript(const std::vector<std::string> &runFiles, const QString &whiteBeam,
					const std::vector<std::string> &absRunFiles, const QString &absWhiteBeam,
					const QString & saveName)
{ 
  QString pyCode = "import DirectEnergyConversion as direct\n";
  pyCode += QString("mono_sample = direct.DirectEnergyConversion('%1')\n").arg(m_sets.loadRun_cbInst->currentText());

  addAnalysisOptions(pyCode);
  addMaskingCommands(pyCode);
 
  pyCode += QString("mono_sample.convert_to_energy(%1, '%2', %3, %4, '%5', %6, '%7')");
  
  QString runFilesList = vectorToPyList(runFiles);
  
  pyCode = pyCode.arg(runFilesList, whiteBeam, m_sets.leEGuess->text());

  if( absRunFiles.empty() )
  {
    pyCode = pyCode.arg("None","None");
  }
  else
  {
    runFilesList = vectorToPyList(absRunFiles);
    pyCode = pyCode.arg(runFilesList, absWhiteBeam);
  }
  pyCode = pyCode.arg(m_sets.leVanEi->text(), saveName);
  std::cerr << pyCode.toStdString() << "\n";

  m_pyScript = pyCode;
}

/**
 * Add the analysis options from the form to the script
 * @param pyCode The string containing the script to update
 */
  void deltaECalc::addAnalysisOptions(QString & pyCode)
{
    //Analysis options
  QString inputValue = m_sets.cbNormal->currentText();  ;
  pyCode += QString("mono_sample.normalise_method = '%1'\n").arg(inputValue);

  pyCode += QString("mono_sample.background = %1\n");
  if( this->m_bgRemove )
  {
    pyCode = pyCode.arg("True");
    pyCode += QString("mono_sample.background_range = [%1, %2]\n").arg(this->m_TOFWinSt).arg(this->m_TOFWinEnd);
  }
  else
  {
    pyCode = pyCode.arg("False");
  }

  //Convert to energy
  pyCode += QString("mono_sample.fix_ei = %1\n");
  if( m_sets.ckFixEi->isChecked() )
  {
    pyCode = pyCode.arg("True");
  }
  else
  {
    pyCode = pyCode.arg("False");
  }
  pyCode += QString("mono_sample.energy_bins = '%1,%2,%3'\n").arg(m_sets.leELow->text(), m_sets.leEWidth->text(), m_sets.leEHigh->text());
  pyCode += QString("mono_sample.map_file = '%1'\n").arg(m_sets.map_fileInput_leName->text());
  if( m_sets.ckRunAbsol->isChecked() )
  {
    pyCode += QString("mono_sample.abs_map_file = '%1'\n").arg(m_sets.leVanMap->text());
  }
}


void deltaECalc::addMaskingCommands(QString & analysisScript)
{
  if( m_diagnosedWS.isEmpty() )
  {
    return;
  }

  QString tmpWS = QString("tmp_") + m_diagnosedWS;

  analysisScript += "fdol_alg = FindDetectorsOutsideLimits(InputWorkspace='%1',OutputWorkspace='%2',HighThreshold=10,LowThreshold=-1,OutputFile='')\n";
  analysisScript += "mono_sample.spectra_masks = fdol_alg.getPropertyValue('BadSpectraNums')\n";
  analysisScript += "mtd.deleteWorkspace('%2')\n";

  analysisScript = analysisScript.arg(m_diagnosedWS).arg(tmpWS);
}

QString deltaECalc::vectorToPyList(const std::vector<std::string> & names) const
{
  QString asString = "[";
  const size_t nFiles = names.size(); 
  for( size_t i = 0; i < nFiles; )
  {
    asString += "'" + QString::fromStdString(names[i]) + "'";
    if( ++i != nFiles )
    {
      asString += ",";
    }
  }
  asString += "]";
  return asString;
}


/** Use the detector masking present in the workspace whose name was passed in
*  the input workspace/s
*  @param maskWS name of the workspace whose detector masking will be copied
*/
void deltaECalc::setDiagnosedWorkspaceName(const QString &maskWS)
{
  m_diagnosedWS = maskWS;
}
/** Insert the number before the dot of the extension
* @param filename the a full path or partial path or just the name of a file
* @param number the number to insert
* @return the filename with the number inserted
*/
std::string deltaECalc::insertNumber(const std::string &filename, const int number) const
{
  // we're going to break up the file name to insert a number into it
  Poco::Path f(filename);
  // check if the path is given in the filename
  if ( f.depth() > 0 )
  {// get the directory name, the full path of the file minus its name and add it back to the result so that we don't lose the path
    return f.directory(f.depth()-1)+"/"+f.getBaseName()+"_"+
	  boost::lexical_cast<std::string>(number)+"."+f.getExtension();
  }
  return f.getBaseName()+"_"+boost::lexical_cast<std::string>(number)+
    "."+f.getExtension();
}
/** Replaces the marker word in the given text with the text in the QLineEdit
*  checking if the value will fail validation, storing any error in m_fails
* @param text the string in which the string will be replaced
* @param pythonMark the word to search for and replace
* @param userVal points to the QLineEdit containing the user value
* @param check the property that will be used to for validity checking
*/
void deltaECalc::LEChkCpIn(QString &text, QString pythonMark, QLineEdit * userVal, Property * const check)
{
  const QString setting = userVal->text();
  std::string error = replaceInErrsFind(text, pythonMark, setting, check);
  if ( ! error.empty() )
  {
    m_fails[userVal] = error;
  }
}
/** Replaces the marker word in the given text with the a string
*  checking if that string will pass validation
* @param text the string in which the string will be replaced
* @param pythonMark the word to search for and replace
* @param setting the string to check the validity of and insert
* @param check the property used to for validity checking
* @return a description of any validity failure or "" if the setting is valid
*/
std::string deltaECalc::replaceInErrsFind(QString &text, QString pythonMark, const QString &setting, Property * const check) const
{
  text.replace(pythonMark, "'"+setting+"'");
  return check->setValue(setting.toStdString());
}
/** appends Python commands that will remove the tempWS string from the
* workspace name and save the file
* @param name the finbal name of the workspace (orginal name was tempWS+name)
*/
void deltaECalc::saveWorkspace(const QString &name)
{
  m_pyScript.append("RenameWorkspace('"+tempWS+name+"', '"+name+"')\n");
  m_pyScript.append("SaveSPE('"+name+"', '"+name+"')\n");
}
