#include "MantidQtCustomInterfaces/deltaECalc.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/FileProperty.h"
#include <QDir>
#include "boost/lexical_cast.hpp"
#include "Poco/StringTokenizer.h"
#include <cmath>

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::Kernel;
using namespace Mantid::API;

const QString deltaECalc::tempWS = "_Conv_ETrans_tempory_WS_";

/** Read the data the user supplied to create Python code to do their calculation
* @param userSettings the form that the user filled in
* @throw invalid_argument where problems with user data prevent the calculation from proceeding
*/
deltaECalc::deltaECalc(QWidget * const interface, const Ui::Excitations &userSettings, deltaECalc::FileInput &runFiles, const bool removalBg, const double TOFWinSt, const double TOFWinEnd) :
  pythonCalc(interface), m_sets(userSettings), m_bgRemove(removalBg), m_TOFWinSt(TOFWinSt), m_TOFWinEnd(TOFWinEnd)
{
  QDir scriptsdir(QString::fromStdString(ConfigService::Instance().getString("pythonscripts.directory")));
  // load a template for the Python script that we will contain in a string
  QString pythonFileName =
    scriptsdir.absoluteFilePath("Excitations/DeltaE/ConvertToETrans.py");
  readFile(pythonFileName);
  
  // the run file selection has a profound effect on the script, we might once on a single file, ilterate the process over many files or sum the list of files and run just once
  const std::vector<std::string> &inputFiles = runFiles.getRunFiles();
  std::vector<std::string>::const_iterator inFile = inputFiles.begin();
  if ( inputFiles.size() < 1 )
  {
    m_fails[m_sets.loadRun_lenumber] = "There must be at least one input file";
  }

  // ckSumSpec->isChecked() == true means sum all the files
  if (m_sets.ckSumSpecs->isChecked() || inputFiles.size() < 2)
  {// this is the easy case; the analysis is done just once on the sum of all the input spaces
	QString WS = createProcessingScript(runFiles.getRunString().toStdString(),
	                              m_sets.leNameSPE->text().toStdString());
	renameWorkspace(WS);
  }
  //no summing, the analysis is done once for _each_ input file, createProcessingScript() is run many times to make one long (possibly very long) script
  else if( m_sets.leNameSPE->text().isEmpty())
  {// here the names of the output files are based on the names of the input files, these names are likely to be unique but two files can have the same name if there are in different directories. Create a map to check on duplication
	std::map<std::string, int> oldNames;
	for(std::vector<std::string>::const_iterator end = inputFiles.end(); ; )
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
      QString WS = createProcessingScript("'"+(*inFile)+"',", saveName);
	  // the last workspace will retained for the user to examine
	  if (end == ++inFile)
	  {// rename it to something they'll recognise
	    renameWorkspace(WS);
	    break;
	  }
	  // all the other workspaces will be deleted
	  m_pyScript.append("mantid.deleteWorkspace(");
	  m_pyScript.append("'"+tempWS+WS+"')\n");
	}
  }
  else
  {// we have a base name to use, distingish the multiple output files using numbers
	for( int i = 0, end = inputFiles.size(); ; )
	{
	  // the coma at the end is required that so the Python interpretor will see the string as a list of strings, a list with one member
	  QString WS = createProcessingScript("'"+inputFiles[i]+"',", 
	    insertNumber(m_sets.leNameSPE->text().toStdString(), i));

	  // the last workspace will retained for the user to examine
	  if (end == ++i)
	  {// rename it to something they'll recognise
	    renameWorkspace(WS);
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
QString deltaECalc::createProcessingScript(const std::string &inFiles, const std::string &oName)
{	
  // we make a copy of code we read from the file because we might replace some terms and we might need to repeat this operation
  QString newScr = m_templateB;
  // validate the user's values using the validator from the algorithm that will be run
  IAlgorithm_sptr group = AlgorithmManager::Instance().createUnmanaged("GroupDetectors");
  group->initialize();
  QString err;

  // here we are placing code directly into specified parts of the Python that will get run
  newScr.replace("|GUI_SET_RAWFILE_LIST|", QString::fromStdString(inFiles));
  // these functions replace whole blocks of code
  createGetEIStatmens(newScr);

  createRemoveBgStatmens(newScr);

  createRebinStatmens(newScr);

  createNormalizationStatmens(newScr);

  LEChkCpIn(newScr, "|GUI_SET_MAP_FILE|", m_sets.map_fileInput_leName,
    group->getProperty("MapFile"));
  
  QString WSName = QString::fromStdString(oName);
  createOutputStatmens(WSName, newScr);
  
  newScr.replace("|GUI_SET_SCALING|", getScaling());

  if (m_pyScript.isEmpty())
  {// put the import statement at the top of the script, only do this once
    m_pyScript.append(m_templateH);
  }
  m_pyScript.append(newScr);

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
  {// the function returns a string that is placed in the python source code where the python intepretor reads it as a number
    LEChkCpIn(newScr, "|GUI_SET_E|", m_sets.leEGuess,
	  gEi->getProperty("EnergyEstimate"));
  }
  else
  {
    newScr.replace("|GUI_SET_E|", "'Run GetEi'");
  }
  //this e guess (approximate Ei value) doesn't always get used, it depends on the tests above. However, Pyhton requires that the '|' in the code is replaced by something
  LEChkCpIn(newScr, "|GUI_SET_E_GUESS|", m_sets.leEGuess,
    gEi->getProperty("EnergyEstimate"));
}
/** Completes the Python statements that implement normalisation
*  @param newScr the Python script being developed from the template
*/
void deltaECalc::createNormalizationStatmens(QString &newScr)
{  
  newScr.replace("|GUI_SET_NORM|", "'"+getNormalization()+"'");

  if ( m_sets.leWBV0->text().isEmpty() )
  {// the whitebeam normalisation isn't going to happen so these values aren't going to be used insert dummy values that Python will accept
    newScr.replace("|GUI_SET_WBV|", "''");
    newScr.replace("|GUI_SET_WBVLOW|", "''");
    newScr.replace("|GUI_SET_WBVHIGH|", "''");
	return;
  }
  
  // we are going to do a normalisation, check the values
  newScr.replace("|GUI_SET_WBVLOW|", m_sets.leWBV0Low->text());
  newScr.replace("|GUI_SET_WBVHIGH|", m_sets.leWBV0High->text());

  std::vector<std::string> exts;
  exts.push_back("raw"); exts.push_back("RAW");
  exts.push_back("NXS"); exts.push_back("nxs");
  FileProperty loadData("Filename", "", FileProperty::Load, exts);
  LEChkCpIn(newScr, "|GUI_SET_WBV|", m_sets.leWBV0, &loadData);
  
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
  if ( ! m_sets.leWBV0High->text().isEmpty() )
  {
	m_sets.leWBV0High->text().toDouble(&isNumber);
	if ( ! isNumber )
	{
	  m_fails[m_sets.leWBV0High] = "Must be a number";
	}
  }
  // using the algorithm's validator is a more stringent but less specific test, we don't get which number is out
  IAlgorithm_sptr rebin = AlgorithmManager::Instance().createUnmanaged("Rebin");
  rebin->initialize();
  Property * const params = rebin->getProperty("Params");
  // this is the rebin string that is created python
  QString rebinStr = m_sets.leWBV0Low->text()+","+
    QString::number(m_sets.leWBV0High->text().toDouble()-m_sets.leWBV0Low->text().toDouble())
	+","+m_sets.leWBV0High->text();
	
  std::string error = params->setValue(rebinStr.toStdString());
  if ( ! error.empty() )
  {// the combination of the two number is wrong but which needs changing? Place the star at the end and allow the user to work it out
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
  newScr.replace("|GUI_SET_OUTWS|", "'"+tempWS+WSName+"'");
}
/** reads leScale from the form to calculate the scaling factor
* @throw NotFoundError if the leScale text can't be converted to an integer
*/
QString deltaECalc::getScaling() const
{
  bool status = false;
  QString exponetial;
  int power = m_sets.leScale->text().toInt(&status);
  if ( ! status ) throw Exception::NotFoundError("Can't scale by a none integer power of ten", "integer");
  exponetial.setNum(std::pow(10.0, power));
  return exponetial;
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
* workspace name that will create a workspace with the name that was passed
* @param name the name that the workspace will be renamed to (orginal name was tempWS+name)
*/
void deltaECalc::renameWorkspace(const QString &name)
{
  m_pyScript.append("RenameWorkspace(");
  m_pyScript.append("'"+tempWS+name+"', '"+name+"')\n");
}

deltaECalc::FileInput::FileInput(QLineEdit &num, QComboBox &instr)
  : m_lineEdit(num), m_box(instr), m_files()
{}
/** Convert integers into filenames, leaves all non-integers values untouched
*/
const std::vector<std::string>& deltaECalc::FileInput::getRunFiles()
{
  readComasAndHyphens(m_lineEdit.text().toStdString(), m_files);
  std::vector<std::string>::iterator i = m_files.begin(), end = m_files.end();
  for ( ; i != end; ++i )
  {
    try
	{
	  const unsigned int runNumber = boost::lexical_cast<unsigned int>(*i);
	  *i = m_box.currentText().toStdString() + //instrument code
	    boost::lexical_cast<std::string>(runNumber) +
		".raw";                                      //only raw files are supported at the moment
	}
	catch ( boost::bad_lexical_cast )
	{// the entry doesn't read as a run number
	}// the alternative is that they entered a filename and we leave that as it is
  }
  return m_files;
}
/** Safer than using getRunFiles() in the situation were there are no files
*  @return the name of the first input file, or an empty string if no input files have been defined yet
*/
std::string deltaECalc::FileInput::getFile1()
{
  return m_files.begin() != m_files.end() ? *m_files.begin() : "";
}

//??STEVES? move this function into the file widget
void deltaECalc::FileInput::readComasAndHyphens(const std::string &in, std::vector<std::string> &out)
{
  if ( in.empty() )
  {// it is not an error to have an empty line but it would cause problems with an error check a the end of this function
    return;
  }
  
  Poco::StringTokenizer ranges(in, "-");
  Poco::StringTokenizer::Iterator aList = ranges.begin();

  out.clear();
  do
  {
    Poco::StringTokenizer preHyp(*aList, ",", Poco::StringTokenizer::TOK_TRIM);
    Poco::StringTokenizer::Iterator readPos = preHyp.begin();
    if ( readPos == preHyp.end() )
    {// can only mean that there was only an empty string or white space the '-'
      throw std::invalid_argument("'-' found at the start of a list, can't interpret range specification");
    }
	out.reserve(out.size()+preHyp.count());
    for ( ; readPos != preHyp.end(); ++readPos )
    {
      out.push_back(*readPos);
    }
    if (aList == ranges.end()-1)
    {// there is no more input
      break;
    }

    Poco::StringTokenizer postHy(*(aList+1),",",Poco::StringTokenizer::TOK_TRIM);
    readPos = postHy.begin();
    if ( readPos == postHy.end() )
    {
      throw std::invalid_argument("A '-' follows straight after another '-', can't interpret range specification");
    }
	
    try
    {//we've got the point in the string where there was a hyphen, first get the stuff that is after it
      // the tokenizer will always return at least on string
      const int rangeEnd = boost::lexical_cast<int>(*readPos);
      // the last string before the hyphen should be an int, here we get the it
      const int rangeStart = boost::lexical_cast<int>(out.back()); 
      // counting down isn't supported
      if ( rangeStart > rangeEnd )
      {
        throw std::invalid_argument("A range where the first integer is larger than the second is not allowed");
      }
      // expand the range
      for ( int j = rangeStart+1; j < rangeEnd; j++ )
      {
        out.push_back(boost::lexical_cast<std::string>(j));
      }
	}
	catch (boost::bad_lexical_cast)
	{// the hyphen wasn't between two numbers, don't intepret it as a range instaed reconstruct the hyphenated string and add it to the list
	  out.back() += "-" + *readPos;
	}
  }
  while( ++aList != ranges.end() );

  if ( *(in.end()-1) == '-' )
  {
    throw std::invalid_argument("'-' found at the end of a list, can't interpret range specification");
  }
}
QString deltaECalc::FileInput::getRunString()
{
  const std::vector<std::string> &inputFiles = getRunFiles();
  std::vector<std::string>::const_iterator inFile = inputFiles.begin();
  std::string fileList;
  for( ; inFile != inputFiles.end(); ++inFile)
  {// there will be a spare ',' at the end of the list but Python accepts this without error and requires it if there is only one member in the list
	fileList += "'"+(*inFile)+"',";
  }
  return QString::fromStdString(fileList);
}
