#include "MantidQtCustomInterfaces/pythonCalc.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ConfigService.h"
#include "boost/lexical_cast.hpp"
#include "MantidAPI/IAlgorithm.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidAPI/AlgorithmManager.h"
#include <QTextStream>
#include <QDir>
#include <map>
#include <cmath>
#include <QMessageBox>
#include "boost/lexical_cast.hpp"
#include "Poco/StringTokenizer.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;

/** returns a read-only reference to the Python script
* @return an executable python script
*/
const QString& pythonCalc::python() const
{
  return m_pyScript;
}

const QString deltaECalc::tempWS = "_ConvertToETrans_tempory_workspace_";

/** Read the data the user supplied to create Python code to do their calculation
* @param userSettings the form that the user filled in
* @throw invalid_argument where problems with user data prevent the calculation from proceeding
*/
deltaECalc::deltaECalc(const Ui::Excitations &userSettings, deltaECalc::FileInput &runFiles) :
  m_settings(userSettings), m_templateH(), m_templateB()
{
  QDir scriptsdir(QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("pythonscripts.directory")));
  // load a template for the Python script that we will contain in a string
  QString pythonFileName =
    scriptsdir.absoluteFilePath("Excitations/DetectorEfficiency/ConvertToETrans.py");
  readFile(pythonFileName);
  
  // the run file selection has a profound effect on the script, we might once on a single file, ilterate the process over many files or sum the list of files and run just once
  const std::vector<std::string> &inputFiles = runFiles.getRunFiles();
  std::vector<std::string>::const_iterator inFile = inputFiles.begin();
  
  // ckSumSpec->isChecked() == true means sum all the files
  if (m_settings.ckSumSpecs->isChecked() || inputFiles.size() == 1)
  {// this is the easy case; the analysis is done just once on the sum of all the input spaces
	std::string fileList;
	for( ; inFile != inputFiles.end(); ++inFile)
	{// there will be a spare ',' at the end of the list but Python accepts this without error and requires it if there is only one member in the list
	  fileList += "'"+(*inFile)+"',";
	}
	QString WS = createProcessingScript(fileList,
	                              m_settings.leNameSPE->text().toStdString());
	renameWorkspace(WS);
  }
  //no summing, the analysis is done once for _each_ input file, createProcessingScript() is run many times to make one long (possibly very long) script
  else if( m_settings.leNameSPE->text().isEmpty())
  {// here the names of the output files are based on the names of the input files, these names are likely to be unique but two files can have the same name if there are in different directories. Create a set to check on duplication
	std::map<std::string, int> oldNames;
	for(Poco::StringTokenizer::Iterator end = inputFiles.end(); ; )
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
	  // remove all the workspaces that we created, except the last one
	  if (end == ++inFile)
	  {
//	    renameWorkspace(WS);
	    break;
	  }
	  m_pyScript.append("mantid.deleteWorkspace(");
	  m_pyScript.append("'"+tempWS+WS+"')\n");
	}
  }
  else
  {// we have a base name to use, distingish the multiple files using numbers
	for( int i = 0, end = inputFiles.size(); ; )
	{
	  // the coma at the end is required that so the Python interpretor will see the string as a list of strings, a list with one member
	  QString WS = createProcessingScript("'"+inputFiles[i]+"',", 
	    insertNumber(m_settings.leNameSPE->text().toStdString(), i+1));

	  // remove all the workspaces that we created, except the last one
	  if (end == ++i)
	  {
	    renameWorkspace(WS);
	    break;
	  }
	  m_pyScript.append("mantid.deleteWorkspace(");
	  m_pyScript.append("'"+tempWS+WS+"')\n");
	}
  }
}
/** reads leScale from the form to calculate the scaling factor
* @throw invalid_argument if the leScale text can't be converted to an integer
*/
QString deltaECalc::getScaling() const
{
  bool status = false;
  QString exponetial;
  int power = m_settings.leScale->text().toInt(&status);
  exponetial.setNum(pow(10.0, power));
  if ( ! status ) throw std::invalid_argument("leScale");
  return exponetial;
}
/** reads cbNormal and returns the user setting, unless it is set to monitor
* in which case it returns the text in cbMonitors
* @throw invalid_argument if normalise to monitor is set but no monitor is selected 
*/
QString deltaECalc::getNormalization() const
{
  if ( m_settings.cbNormal->currentText() != "monitor" )
  {
    return m_settings.cbNormal->currentText();
  }
  if (m_settings.cbMonitors->currentText().isEmpty()) std::invalid_argument("cbMonitors");
  return m_settings.cbMonitors->currentText();
}
/** reads leEGuess and returns the user setting
* @throw invalid_argument if the string doesn't convert to a number or the number is negative ot huge
*/
QString deltaECalc::getEGuess() const
{
  QString userVal = m_settings.leEGuess->text();

  // validate the users value using the declared property validator
  IAlgorithm_sptr alg = AlgorithmManager::Instance().createUnmanaged("GetEi");
  alg->initialize();
  //works if the return statement is moved to here
  Property *prop = alg->getProperty("EnergyEstimate");
 
  std::string error = prop->setValue(userVal.toStdString());
  if ( ! error.empty() )
  {
    throw std::invalid_argument("leEGuess:"+error);
  }
  bool success;
  double EGuess = userVal.toDouble(&success);

  if ( ! success || EGuess < 0 || EGuess > 10e6)
  {
    throw std::invalid_argument("leEGuess");
  }
  return userVal;
}
/** Insert the number before the dot of the extension
* @param filename the a full path or partial path or just the name of a file
* @param number the number to insert
*/
std::string deltaECalc::insertNumber(const std::string &filename, const int number) const
{
  // we're going to break up the file name to insert a number into it
  Poco::Path f(filename);
  // get the directory name, the full path of the file minus its name
  return f.directory(f.depth()-1) + f.getBaseName() +
	boost::lexical_cast<std::string>(number) + f.getExtension();
}
//?? is there already a function like this? where should it go?
/** Sets m_pyScript to the contents of the named file
* @param pythonFile the name of the file to read
*/
void deltaECalc::readFile(const QString &pythonFile)
{
  QFile py_script(pythonFile);
  try
  {
    if ( !py_script.open(QIODevice::ReadOnly) )
    {
      throw Mantid::Kernel::Exception::FileError(std::string("Couldn't open python file "), pythonFile.toStdString());
    }
    QTextStream stream(&py_script);
    QString line;
	while( !stream.atEnd() )
    {
	  line = stream.readLine();
	  // strip some Python comments, might speed things up when there are multiple input files and these lines are repeated many times
	  if ( line.indexOf("#") == 0 ) continue;
	  // separate out the header because we might want to create a file where the body is repeated many times but there is one header
	  if ( line.indexOf("import ") == 0 || line.indexOf(" import ") != -1 )
	  {
		m_templateH.append(line + "\n");
	  }
	  else m_templateB.append(line + "\n");
    }
    py_script.close();
  }
  catch( ... )
  {
    py_script.close();
    throw;
  }
}
  
QString deltaECalc::createProcessingScript(const std::string &inFiles, const std::string &oName)
{	
  // we make a copy of code we read from the file because we might replace some terms and we might need to repeat this operation
  QString newScript = m_templateB;
  
  // here we are placing code directly into specified parts of the Python that will get run
  newScript.replace("|GUI_SET_RAWFILE_LIST|", QString::fromStdString(inFiles));
  
  if (m_settings.ckFixEi->isChecked())
  {// the function returns a string that is placed in the python source code where the python intepretor reads it as a number
    newScript.replace("|GUI_SET_E|", getEGuess());
  }
  else
  {
    newScript.replace("|GUI_SET_E|", "'Run GetEi'");
  }
  //this e guess (approximate Ei value) doesn't always get used, it depends on the tests above. However, Pyhton requires that the '|' in the code is replaced by something
  newScript.replace("|GUI_SET_E_GUESS|", getEGuess());
  
  //??STEVES?? validate input data here
  QString rebinStr = m_settings.leELow->text()+","+m_settings.leEWidth->text()+","+m_settings.leEHigh->text();
  newScript.replace("|GUI_SET_BIN_BOUNDS|", "'"+rebinStr+"'");

  newScript.replace("|GUI_SET_NORM|", getNormalization());

  newScript.replace("|GUI_SET_WBV|", "'"+m_settings.leWBV0->text()+"'");
  if ( ! m_settings.leWBV0->text().isEmpty() )
  {
    newScript.replace("|GUI_SET_WBVLow|", m_settings.leWBV0Low->text());
    newScript.replace("|GUI_SET_WBVHigh|", m_settings.leWBV0High->text());
  }
  else
  {// inserdt dummy values that Python will accept 
    newScript.replace("|GUI_SET_WBVLow|", "-1");
    newScript.replace("|GUI_SET_WBVHigh|", "-1");
  }

  newScript.replace("|GUI_SET_MAP_FILE|", "'"+m_settings.map_fileInput_leName->text()+"'");

  // we are forcing the name to have contain .spe, but it doesn't have be right at the end of the file, if the user sees fit to have a different extension
  QString WSName = QString::fromStdString(oName);
  if ( ! WSName.contains(".spe") )
  {
	WSName += ".spe";
  }

  newScript.replace("|GUI_SET_OUTWS|", "'"+tempWS+WSName+"'");
  newScript.replace("|GUI_SET_OUTPUT|","'"+WSName+"'");
  
  newScript.replace("|GUI_SET_SCALING|", getScaling());

  if (m_pyScript.isEmpty())
  {
    m_pyScript.append(m_templateH);
  }
  m_pyScript.append(newScript);
  return WSName;
}
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