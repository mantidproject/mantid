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
deltaECalc::deltaECalc(QWidget * const interface, const Ui::Homer &userSettings, const std::vector<std::string>  &input, const bool removalBg, const double TOFWinSt, const double TOFWinEnd, const QString WBV) :
  pythonCalc(interface), m_sets(userSettings), m_bgRemove(removalBg), m_TOFWinSt(TOFWinSt), m_TOFWinEnd(TOFWinEnd)
{
  QDir scriptsdir(QString::fromStdString(ConfigService::Instance().getString("pythonscripts.directory")));
  // load a template for the Python script that we will contain in a string
  QString pythonFileName =
    scriptsdir.absoluteFilePath("Excitations/DetectorTestLib.py");
  appendFile(pythonFileName);
  pythonFileName =
    scriptsdir.absoluteFilePath("Excitations/ConversionLib.py");
  appendFile(pythonFileName);
  
  if (WBV.isEmpty())
  {
    throw std::invalid_argument("A file must be selected for the ");
  }
  // the run file selection has a profound effect on the script, we might once on a single file, ilterate the process over many files or sum the list of files and run just once
  std::vector<std::string>::const_iterator inFile = input.begin();
  // ckSumSpec->isChecked() == true means sum all the files
  if (m_sets.ckSumSpecs->isChecked() || input.size() < 2)
  {// this is the easy case; the analysis is done just once on the sum of all the input spaces
	  QString WS = createProcessingScript(vectorToCommaSep(input),
	                              m_sets.leNameSPE->text().toStdString(), WBV);
    saveWorkspace(WS);
  }
  //no summing, the analysis is done once for _each_ input file, createProcessingScript() is run many times to make one long (possibly very long) script
  else if( m_sets.leNameSPE->text().isEmpty())
  {// here the names of the output files are based on the names of the input files, these names are likely to be unique but two files can have the same name if there are in different directories. Create a map to check on duplication
    std::map<std::string, int> oldNames;
    for(std::vector<std::string>::const_iterator end = input.end(); ; )
    {
      std::string saveName = SPEFileName(*inFile).toStdString();
      if ( oldNames.find(saveName) == oldNames.end() )
      {// this is the first time this name has been used, record the name and say that it hasn't been used before
	      oldNames[saveName] = 1;
      }
      else
      {// duplicate names!
        saveName = insertNumber(saveName, oldNames[saveName]++);
      }
      QString WS = createProcessingScript(*inFile, saveName, WBV);
	    // the last workspace will retained for the user to examine
	    if (end == ++inFile)
      {// rename it to something they'll recognise
        saveWorkspace(WS);
	      break;
      }
      // all the other workspaces will be deleted
      m_pyScript.append("mantid.deleteWorkspace(");
      m_pyScript.append("'"+tempWS+WS+"')\n");
	  }
  }
  else
  {// we have a base name to use, distingish the multiple output files using numbers
    for( int i = 0, end = input.size(); ; )
	  {
	    // the coma at the end is required that so the Python interpretor will see the string as a list of strings, a list with one member
	    QString WS = createProcessingScript(input[i], 
	      insertNumber(m_sets.leNameSPE->text().toStdString(), i), WBV);

	    // the last workspace will retained for the user to examine
	    if (end == ++i)
	    {// rename it to something they'll recognise
	      saveWorkspace(WS);
	      break;
	    }
	    m_pyScript.append("mantid.deleteWorkspace(");
	    m_pyScript.append("'"+tempWS+WS+"')\n");
	  }
  }
}
/** Adds user values from the GUI into the Python script
*  @param inFiles a coma separated list of data file names
*  @param oName the name of the output workspace
*  @return the name that will be given to the output workspace
*/
QString deltaECalc::createProcessingScript(const std::string &inFiles, const std::string &oName, const QString &whiteB)
{	
  QDir scriptsdir(QString::fromStdString(ConfigService::Instance().getString("pythonscripts.directory")));
  QString pythonFileName =
    scriptsdir.absoluteFilePath("Excitations/GUI_Interface.py");
  appendFile(pythonFileName);  // we make a copy of code we read from the file because we might replace some terms and we might need to repeat this operation

  QString err;

  // here we are placing code directly into specified parts of the Python that will get run
  m_pyScript.replace("|GUI_SET_RAWFILE_LIST|", QString::fromStdString(inFiles));
  // these functions replace whole blocks of code
  createGetEIStatmens(m_pyScript);

  createRemoveBgStatmens(m_pyScript);

  createRebinStatmens(m_pyScript);

  createNormalizationStatmens(m_pyScript, whiteB);

  // use a FileProperty to check that the file exists
  std::vector<std::string> exts;
  exts.push_back("map"); exts.push_back("MAP");
  FileProperty loadData("Filename", "", FileProperty::OptionalLoad, exts);
  LEChkCpIn(m_pyScript,"|GUI_SET_MAP_FILE|",m_sets.map_fileInput_leName,&loadData);
  
  QString WSName = QString::fromStdString(oName);
  createOutputStatmens(WSName, m_pyScript);
  
  m_pyScript.replace("|TEMPWS|", tempWS);

  return WSName;
}
/** Completes the Python statements that implement calculating the initial energy
*  using user values from the form
*  @param newScr the Python script being developed from the template
*/
void deltaECalc::createGetEIStatmens(QString &newScr)
{
  // create this algorithm for doing the validation, actually, depending on the user settings, getEi might not get run but the validation is good anyway
  IAlgorithm_sptr gEi = AlgorithmManager::Instance().createUnmanaged("GetEi");
  gEi->initialize();
  // run getEi, unless the user checked the box 
  if (m_sets.ckFixEi->isChecked())
  {
    newScr.replace("|GUI_SET_E|", "'fixei'");
  }
  else
  {
    newScr.replace("|GUI_SET_E|", "'true'");
  }
  //this e guess (approximate Ei value) doesn't always get used, it depends on the tests above. However, Pyhton requires that the '|' in the code is replaced by something
  LEChkCpIn(newScr, "|GUI_SET_E_GUESS|", m_sets.leEGuess,
    gEi->getProperty("EnergyEstimate"));
}
/** Completes the Python statements that implement normalisation
*  @param newScr the Python script being developed from the template
*/
void deltaECalc::createNormalizationStatmens(QString &newScr, const QString &norm)
{  
  newScr.replace("|GUI_SET_NORM|", "'"+getNormalization()+"'");

  newScr.replace("|GUI_SET_WBV|", "'"+norm+"'");
  
  newScr.replace("|GUI_WB_LOW_E|", "'"+m_sets.leWBV0Low->text()+"'");
  newScr.replace("|GUI_WB_HIGH_E|", "'"+m_sets.leWBV0High->text()+"'");

  QString rebinStr = m_sets.leWBV0Low->text()+","+QString::number(
    2*(m_sets.leWBV0High->text().toDouble()-m_sets.leWBV0Low->text().toDouble()))
	+","+m_sets.leWBV0High->text();
  
  bool isNumber;
  // check that we've got a usable value, an empty box is OK it means that the default will be used
  if ( ! m_sets.leWBV0Low->text().isEmpty() )
  {
    m_sets.leWBV0Low->text().toDouble(&isNumber);
	if ( ! isNumber )
	{
	  m_fails[m_sets.leWBV0Low] = "Must be a number";
	}
  }
  
  // using the algorithm's validator is a more stringent but less specific test, we don't get which number is out
  IAlgorithm_sptr rebin = AlgorithmManager::Instance().createUnmanaged("Rebin");
  rebin->initialize();
  Property * const params = rebin->getProperty("Params");
  // this is the rebin string that is created python

  std::string error = params->setValue(rebinStr.toStdString());
  if ( ! error.empty() )
  {// the combination of the two numbers is wrong but which needs changing? Place the star at the end and allow the user to work it out
    m_fails[m_sets.leWBV0High] = error;
  }
}
/** Tests the user definitions for the TOF start and ends of the background regions
*  against their FlatBackground validator and adds the user values to the Python script
*  @param newScr the Python script being developed from the template
*/
void deltaECalc::createRemoveBgStatmens(QString &newScr)
{
  newScr.replace("|RM_BG|", m_bgRemove ? "'yes'" : "'no'");
  
  IAlgorithm_sptr flatB
    = AlgorithmManager::Instance().createUnmanaged("FlatBackground");
  flatB->initialize();
  
  std::string error = replaceInErrsFind(newScr, "|TOF_LOW|",
    QString::number(m_TOFWinSt), flatB->getProperty("StartX"));
  if ( ! error.empty() && m_bgRemove )
  {
    m_fails[m_sets.pbBack] = error;
  }

  error = replaceInErrsFind(newScr, "|TOF_HIGH|",
    QString::number(m_TOFWinEnd), flatB->getProperty("EndX"));
  if ( ! error.empty() && m_bgRemove )
  {
    m_fails[m_sets.pbBack] = error;
  }
}
/** Tests the user's rebin parameters against its Rebin validator and
*  adds the user values to the Python script
*  @param newScr the Python script being developed from the template
*/
void deltaECalc::createRebinStatmens(QString &newScr)
{
  QString rebinStr = m_sets.leELow->text()
    +","+m_sets.leEWidth->text()+","+m_sets.leEHigh->text();
	
  newScr.replace("|GUI_SET_BIN_BOUNDS|", "'"+rebinStr+"'");
  // validate the user's values using the rebin algorithm validator, unless they left the boxes empty in which case rebin wont be run and we do nothing
  IAlgorithm_sptr rebin = AlgorithmManager::Instance().createUnmanaged("Rebin");
  rebin->initialize();
  Property * const params = rebin->getProperty("Params");
  std::string error = params->setValue(rebinStr.toStdString());
  if ( ! error.empty() )
  {
    m_fails[m_sets.leEHigh] = error;
  }
}
/** Write the save SPE file statements to the Python script
*  @param WSName name of the output workspace
*  @param newScr the Python script being developed from the template
*/
void deltaECalc::createOutputStatmens(QString &WSName, QString &newScr)
{// we are forcing the name to have contain .spe, but it doesn't have be right at the end of the file, if the user sees fit to have a different extension
  if ( ! WSName.contains(".spe") )
  {
	WSName += ".spe";
  }  
  newScr.replace("|GUI_SET_OUTPUT|","'"+WSName+"'");
  // if a full path was given check it exists, is what the test below amounts to
  IAlgorithm_sptr saveSPE = AlgorithmManager::Instance().createUnmanaged("SaveSPE");
  saveSPE->initialize();
  Property * const Filename = saveSPE->getProperty("Filename");
  std::string error = Filename->setValue(WSName.toStdString());
  if ( ! error.empty() )
  {
    m_fails[m_sets.leNameSPE] = error;
  }

  // the name of the workspace which may remain in Mantid after this interface has finished will be the same as the saved file
  newScr.replace("|GUI_SET_OUTWS|", tempWS+WSName);
}
/** reads cbNormal and returns the user setting, unless it is set to monitor
* in which case it returns the text in cbMonitors
* @throw NotFoundError if normalise to monitor is set but no monitor is selected 
*/
QString deltaECalc::getNormalization() const
{
  if ( m_sets.cbNormal->currentText() != "monitor" )
  {
    return m_sets.cbNormal->currentText();
  }// if we get passed this point it must be one of the monitor normalisations
  if (m_sets.cbMonitors->currentText().isEmpty()) Exception::NotFoundError("Normalise to monitor has been selected but which monitor to use hasn't been specified", "monitors");
  return "monitor-"+m_sets.cbMonitors->currentText();
}
/** Use the detector masking present in the workspace whose name was passed in
*  the input workspace/s
*  @param maskWS name of the workspace whose detector masking will be copied
*/
void deltaECalc::maskDetects(const QString &maskWS)
{
  m_pyScript.replace("''#+|MASK_WORKSPACE|", "'"+maskWS+"'");
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
