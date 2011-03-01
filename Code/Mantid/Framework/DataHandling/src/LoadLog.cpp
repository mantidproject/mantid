//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "LoadRaw/isisraw2.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataHandling/LoadLog.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Glob.h"
#include "MantidKernel/LogParser.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <boost/algorithm/string.hpp>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/DateTimeParser.h>
#include <Poco/DateTimeFormat.h>
#include <Poco/RegularExpression.h>

#include <fstream>  // used to get ifstream
#include <sstream>

namespace Mantid
{
namespace DataHandling
{

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadLog)

using namespace Kernel;
using API::WorkspaceProperty;
using API::MatrixWorkspace;
using API::MatrixWorkspace_sptr;
using API::FileProperty;
using DataObjects::Workspace2D;
using DataObjects::Workspace2D_sptr;

/// Empty default constructor
LoadLog::LoadLog()
{}

/// Initialisation method.
void LoadLog::init()
{
  //this->setWikiSummary("Load ISIS log file(s) into a [[workspace]].");
  //this->setOptionalMessage("Load ISIS log file(s) into a workspace.");

  // When used as a sub-algorithm the workspace name is not used - hence the "Anonymous" to satisfy the validator
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("Workspace","Anonymous",Direction::InOut),
                      "The name of the workspace to which the log data will be added");

  std::vector<std::string> exts(4, "");
  exts[0] = ".txt";
  exts[1] = ".raw";
  exts[2] = ".s*";
  exts[3] = ".add";
  declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
    "The filename (including its full or relative path) of either \n"
    "an ISIS log file, a multi-column SNS-style text file, or an ISIS raw file. \n"
    "If a raw file is specified all log files associated with\n"
    "that raw file are loaded into the specified workspace. The file extension must\n"
    "either be .raw or .s when specifying a raw file");

  declareProperty(new ArrayProperty<std::string>("Names"),
    "For SNS-style log files only: the names of each column's log, separated by commas.\n"
    "This must be one fewer than the number of columns in the file.");

  declareProperty(new ArrayProperty<std::string>("Units"),
    "For SNS-style log files only: the units of each column's log, separated by commas.\n"
    "This must be one fewer than the number of columns in the file.\n"
    "Optional: leave blank for no units in any log.");

}

  //@cond NODOC
  namespace
  {
    struct FileMatcher
    {
      FileMatcher(const std::string & expression) : m_expression(expression) {}

      bool operator()(const std::string & test) const
      {
        Poco::RegularExpression regex(m_expression, Poco::RegularExpression::RE_CASELESS);
        return regex.match(test);
      }
      
    private:
      FileMatcher();
      const std::string m_expression;
    };
  }
  //@endcond



/** Check if the file is SNS text; load it if it is, return false otherwise.
 *
 * @return true if the file was a SNS style; false otherwise.
 */
  bool LoadLog::LoadSNSText()
{

  // Get the SNS-specific parameter
  std::vector<std::string> names = getProperty("Names");
  std::vector<std::string> units = getProperty("Units");

  // Get the input workspace and retrieve run from workspace.
  // the log file(s) will be loaded into the run object of the workspace
  const MatrixWorkspace_sptr localWorkspace = getProperty("Workspace");

  // open log file
  std::ifstream inLogFile(m_filename.c_str());

  // Get the first line
  std::string aLine;
  if (!Mantid::Kernel::extractToEOL(inLogFile,aLine))
    return false;

  std::vector<double> cols;
  bool ret = SNSTextFormatColumns(aLine, cols);
  // Any error?
  if (!ret || cols.size() < 2)
    return false;

  size_t numCols = static_cast<size_t>(cols.size()-1);
  if (names.size() != numCols)
    throw std::invalid_argument("The Names parameter should have one fewer entry as the number of columns in a SNS-style text log file.");
  if ((units.size() > 0) && (units.size() != numCols))
    throw std::invalid_argument("The Units parameter should have either 0 entries or one fewer entry as the number of columns in a SNS-style text log file.");

  // Ok, create all the logs
  std::vector<TimeSeriesProperty<double>*> props;
  for(size_t i=0; i < numCols; i++)
  {
    TimeSeriesProperty<double>* p = new TimeSeriesProperty<double>(names[i]);
    if (units.size() == numCols)
      p->setUnits(units[i]);
    props.push_back(p);
  }
  // Go back to start
  inLogFile.seekg(0);
  while(Mantid::Kernel::extractToEOL(inLogFile,aLine))
  {
    if (aLine.size() == 0)
      break;

    if (SNSTextFormatColumns(aLine, cols))
    {
      if (cols.size() == numCols+1)
      {
        DateAndTime time(cols[0], 0.0);
        for(size_t i=0; i<numCols; i++)
          props[i]->addValue(time, cols[i+1]);
      }
      else
        throw std::runtime_error("Inconsistent number of columns while reading SNS-style text file.");
    }
    else
      throw std::runtime_error("Error while reading columns in SNS-style text file.");
  }
  // Now add all the full logs to the workspace
  for(size_t i=0; i < numCols; i++)
  {
    std::string name = props[i]->name();
    if (localWorkspace->mutableRun().hasProperty(name))
    {
      localWorkspace->mutableRun().removeLogData(name);
      g_log.information() << "Log data named " << name << " already existed and was overwritten.\n";
    }
    localWorkspace->mutableRun().addLogData(props[i]);
  }

  return true;
}



/** Executes the algorithm. Reading in ISIS log file(s)
 * 
 *  @throw Mantid::Kernel::Exception::FileError  Thrown if file is not recognised to be a raw datafile or log file
 *  @throw std::runtime_error Thrown with Workspace problems
 */
void LoadLog::exec()
{
  // Retrieve the filename from the properties and perform some initial checks on the filename
  m_filename = getPropertyValue("Filename");

  // File property checks whether the given path exists, just check that is actually a file 
  Poco::File l_path( m_filename );
  if ( l_path.isDirectory() )
  {
    g_log.error("In LoadLog: " + m_filename + " must be a filename not a directory.");
    throw Exception::FileError("Filename is a directory:" , m_filename);
  }

  // Get the input workspace and retrieve run from workspace.
  // the log file(s) will be loaded into the run object of the workspace 
  const MatrixWorkspace_sptr localWorkspace = getProperty("Workspace");

  // If m_filename is the filename of a raw datafile then search for potential log files
  // in the directory of this raw datafile. Otherwise check if m_filename is a potential
  // log file. Add the filename of these potential log files to: potentialLogFiles.
  std::set<std::string> potentialLogFiles;
  
  // start the process or populating potential log files into the container: potentialLogFiles
  std::string l_filenamePart = Poco::Path(l_path.path()).getFileName();// get filename part only
  bool rawFile = false;// Will be true if Filename property is a name of a RAW file

  if ( isAscii(m_filename) )
  {
    // Is it a SNS style file? If so, we load it and abort.
    if (LoadSNSText())
      return;
    // Otherwise we continue.
  }

  if ( isAscii(m_filename) && l_filenamePart.find("_") != std::string::npos )
  {
    // then we will assume that m_filename is an ISIS/SNS log file
    potentialLogFiles.insert(m_filename);
  }
  else
  {
    // then we will assume that m_filename is an ISIS raw file. The file validator will have warned the user if the
    // extension is not one of the suggested ones
    rawFile = true;
    // strip out the raw data file identifier
    std::string l_rawID("");
    size_t idx = l_filenamePart.rfind('.');
    if( idx != std::string::npos )
    {
      l_rawID = l_filenamePart.substr(0, l_filenamePart.rfind('.'));
    }
    else
    {
      l_rawID = l_filenamePart;
    }
    /// check for alternate data stream exists for raw file
    /// if exists open the stream and read  log files name  from ADS
    if(adsExists())
    {
      potentialLogFiles = getLogfilenamesfromADS();
    }
    else
    {
      // look for log files in the directory of the raw datafile
      std::string pattern(l_rawID + "_*.txt");
      Poco::Path dir(m_filename);
      dir.makeParent();
      try
      {
        Kernel::Glob::glob(Poco::Path(dir).resolve(pattern),potentialLogFiles);
      }
      catch(std::exception &)
      {
      }

      if( potentialLogFiles.empty() )
      {
        Poco::RegularExpression regex(l_rawID + "_.*\\.txt", Poco::RegularExpression::RE_CASELESS );
        Poco::DirectoryIterator end_iter;
        for ( Poco::DirectoryIterator dir_itr(Poco::Path(m_filename).parent()); dir_itr != end_iter; ++dir_itr )
        {
          if ( !Poco::File(dir_itr->path() ).isFile() ) continue;

          l_filenamePart = Poco::Path(dir_itr->path()).getFileName();

          if ( regex.match(l_filenamePart) )
          {
            potentialLogFiles.insert( dir_itr->path() );
          }
        }

      }
    }

    //.if a .log file exists in the raw file directory
    std::string threecolumnLogfile = getThreeColumnName();
    if( !threecolumnLogfile.empty() )
    {
      std::set<std::string> blockFileNameList=createthreecolumnFileLogProperty(threecolumnLogfile,localWorkspace->mutableRun());
      //remove the file name from potential logfiles list if it's there in the .log file.
      std::set<std::string>::const_iterator itr;
      for(itr=blockFileNameList.begin();itr!=blockFileNameList.end();++itr)
      {
        std::set<std::string>::iterator litr= find(potentialLogFiles.begin(),potentialLogFiles.end(),*itr);
        if(litr!=potentialLogFiles.end())
        {
          potentialLogFiles.erase(litr);
        }
      }
    }
  }
 
  //If there are no log files by now, we have nothing else to do
  if( potentialLogFiles.empty() ) return;

  //Do a quick search for the icpevent file
  std::string icpevent_file_name("");
  std::set<std::string>::const_iterator icpfile = find_if(potentialLogFiles.begin(), potentialLogFiles.end(), FileMatcher(std::string(".*icpevent.*")));
  if( icpfile != potentialLogFiles.end() )
  {
    icpevent_file_name = *icpfile;
  }

  Kernel::LogParser parser(icpevent_file_name);
  // Add mantid-created logs
  
  m_periods=parser.getPeriodsProperty();
  localWorkspace->mutableRun().addLogData(parser.createAllPeriodsLog());
  localWorkspace->mutableRun().addLogData(parser.createRunningLog());

  // Extract the common part of log file names (the workspace name)
  std::string ws_name = Poco::Path(m_filename).getFileName();
  ws_name.erase(ws_name.find_last_of('.'));
  ws_name += '_';
  size_t n_common_chars = ws_name.size();

  // Attempt to load the content of each potential log file into the Sample object
  std::set<std::string>::const_iterator logs_end = potentialLogFiles.end();
  for(std::set<std::string>::const_iterator logs_itr = potentialLogFiles.begin(); logs_itr != logs_end; ++logs_itr)
  {
    std::string filename = *logs_itr;
    // open log file
    std::ifstream inLogFile(filename.c_str());

    if (!inLogFile)
    {
      // Unable to open file...
      g_log.warning("Unable to open file " + filename);
      // ...go on to the next one
      continue;
    }
    // figure out if second column is a number or a string
    std::string aLine;
    if( Mantid::Kernel::extractToEOL(inLogFile,aLine) )
    {

      if ( !isDateTimeString(aLine) )
      {
        g_log.warning("File" + filename + " is not a standard ISIS log file. Expected to be a two column file.");
        inLogFile.close();
        continue;
      }
      std::string DateAndTime;
      std::stringstream ins(aLine);
      ins >> DateAndTime;

      // read in what follows the date-time string in the log file and figure out
      // what type it is
      std::string whatType;
      ins >> whatType;
      kind l_kind = classify(whatType);
      if ( LoadLog::string != l_kind && LoadLog::number != l_kind )
      {
        g_log.warning("ISIS log file contains unrecognised second column entries: " + filename);
        inLogFile.close();
        continue;
      }
      
      try
      {
        // Make the property name by removing the workspce name and file extension from the log filename
        std::string log_name = Poco::Path(Poco::Path(filename).getFileName()).getBaseName();

        if (rawFile)
        {
          log_name.erase(0, n_common_chars);
        }

        Property* log = parser.createLogProperty(*logs_itr,stringToLower(log_name));
        if (log)
        {
          localWorkspace->mutableRun().addLogData(log);
        }
      }
      catch(std::exception&)
      {
        continue;
      }

    } 
    inLogFile.close();
  } // end for


  // operation was a success and ended normally
  return;
}

/** Return the name of the three column log file if we have one.
 * @returns A string containing the full log file path to a three column log file if one exists. An empty string otherwise.
*/
std::string LoadLog::getThreeColumnName() const
{  
  std::string rawID;
  std::string::size_type dot = m_filename.rfind(".");
  if( dot != std::string::npos)
  {
    rawID = m_filename.substr(0, dot);
  }
  // append .log to get the .log file name
  std::string logfileName=rawID+".log";	
  int count=0;
  if (Poco::File(logfileName).exists())
  {
    //validate the file
    std::ifstream inLogFile(logfileName.c_str());
    if (!inLogFile)
    { 
      throw Exception::FileError("Unable to open file:" ,logfileName );
    }

    //check if first 19 characters of a string is data-time string according to yyyy-mm-ddThh:mm:ss
    std::string aLine;
    kind l_kind(LoadLog::empty);
    while(Mantid::Kernel::extractToEOL(inLogFile,aLine))
    {			 
      if ( !isDateTimeString(aLine) )
      { g_log.warning("File" + logfileName + " is not a standard ISIS log file. Expected to be a file starting with DateTime String format.");
      inLogFile.close();
      return "";
      }

      std::stringstream ins(aLine);
      std::string firstcolumn;
      ins >> firstcolumn;
      // read in what follows the date-time string in the log file and figure out
      // what type it is
      std::string secondcolumn;
      ins >> secondcolumn;
      l_kind = classify(secondcolumn);
      if ( LoadLog::string != l_kind )
      {
        g_log.warning("ISIS log file contains unrecognised second column entries: " + logfileName);
        inLogFile.close();
        return "";
      }

      std::string thirdcolumn;
      ins>>thirdcolumn;
      l_kind = classify(thirdcolumn);
      if ( LoadLog::string != l_kind && LoadLog::number!=l_kind)
      {
        g_log.warning("ISIS log file contains unrecognised third column entries: " + logfileName);
        inLogFile.close();
        return "";
      }
      ++count;
      if(count==2) ///reading first two lines from file for validation purpose.
        break;
    }
    return logfileName;
  }
  else return "";
}

/* this method looks for ADS with name checksum exists
 * @return True if ADS stream checksum exists
 */
bool LoadLog::adsExists()
{
  std::string adsname(m_filename+":checksum");
  std::ifstream adstream(adsname.c_str());
  if(!adstream)
  {return false;
  }
  return true;
}

/* this method reads  the checksum ADS associated with the
 * raw file and returns the filensmes of the log files
 * @return list of logfile names.
 */
std::set<std::string> LoadLog::getLogfilenamesfromADS()
{	
  std::string adsname(m_filename+":checksum");
  std::ifstream adstream(adsname.c_str());
  if(!adstream)
    return std::set<std::string>();
  std::string str;
  std::string path;
  std::string logFile;
  std::set<std::string>logfilesList;
  Poco::Path logpath(m_filename);
  std::string p=logpath.home();
  size_t pos =m_filename.find_last_of("/");
  if(pos==std::string::npos)
  {
    pos =m_filename.find_last_of("\\");
  }
  if(pos!=std::string::npos)
    path=m_filename.substr(0,pos);
  while(Mantid::Kernel::extractToEOL(adstream,str))
  {
    std::string fileName;
    pos = str.find("*");
    if(pos==std::string::npos)
      continue;
    fileName=str.substr(pos+1,str.length()-pos);
    pos= fileName.find("txt");
    if(pos==std::string::npos)
      continue;
    logFile=path+"/"+fileName;
    if(logFile.empty())
      continue;
    logfilesList.insert(logFile);
  }
  return logfilesList;
}

/** This method reads the.log file and creates timeseries property and sets that to the run object
 * @param logfile :: three column log(.log) file name.
 * @param run :: The run information object
 * @returns list of logfiles which exists as blockname in the .log file
 */
std::set<std::string> LoadLog::createthreecolumnFileLogProperty(const std::string& logfile,API::Run& run)
{    
  std::set<std::string> blockFileNameList;
  std::string sdata,str;
  std::string propname;
  Mantid::Kernel::TimeSeriesProperty<double>* logd=0;
  Mantid::Kernel::TimeSeriesProperty<std::string>* logs=0;
  std::map<std::string,Kernel::TimeSeriesProperty<double>*> dMap;
  std::map<std::string,Kernel::TimeSeriesProperty<std::string>*> sMap;
  typedef std::pair<std::string,Kernel::TimeSeriesProperty<double>* > dpair;
  typedef std::pair<std::string,Kernel::TimeSeriesProperty<std::string>* > spair;

  std::string path = m_filename;
  std::string::size_type pos=m_filename.rfind(".");
  if( pos != std::string::npos )
  {
    path = path.substr(0, pos);
  }
  bool isNumeric(false);

  std::ifstream file(logfile.c_str());
  if (!file)
  {	
    g_log.warning()<<"Cannot open log file "<<logfile<<"\n";
    return std::set<std::string>();
  }
  while(Mantid::Kernel::extractToEOL(file,str))
  {
    if (!Kernel::TimeSeriesProperty<double>::isTimeString(str) || (str[0]=='#')) 
    {    //if the line doesn't start with a time read the next line
      continue;
    }
    std::stringstream line(str);
    std::string timecolumn;
    line>>timecolumn;
    std::string blockcolumn;
    line>>blockcolumn;
    std::string valuecolumn;
    line>>valuecolumn;
    sdata=valuecolumn;

    /////column two in .log file is called block column
    /////if any .txt file with rawfilename_blockcolumn.txt exists
    ///// donot load that txt  files
    ///// blockFileNameList conatins the file names to be removed from potentiallogfiles list.
    propname=stringToLower(blockcolumn);
    //check if the data is numeric
    std::istringstream istr(valuecolumn);
    double dvalue;
    istr >> dvalue;
    isNumeric = !istr.fail();
    if (isNumeric)
    {				
      std::map<std::string,Kernel::TimeSeriesProperty<double>*>::iterator ditr=dMap.find(propname);
      if(ditr!=dMap.end())
      {	
        Kernel::TimeSeriesProperty<double>* p=ditr->second;
        if (p) p->addValue(timecolumn,dvalue);
      }
      else
      {	

        logd = new Kernel::TimeSeriesProperty<double>(propname);
        logd->addValue(timecolumn,dvalue);
        dMap.insert(dpair(propname,logd));
        std::string blockcolumnFileName=path+"_"+blockcolumn+".txt";
        if(blockcolumnFileExists(blockcolumnFileName))
        {
          blockFileNameList.insert(blockcolumnFileName);
        }
      }
    }
    else
    {		
      std::map<std::string,Kernel::TimeSeriesProperty<std::string>*>::iterator sitr=sMap.find(propname);
      if(sitr!=sMap.end())
      {	
        Kernel::TimeSeriesProperty<std::string>* prop=sitr->second;
        if (prop) prop->addValue(timecolumn,valuecolumn);
      }
      else
      {	
        logs = new Kernel::TimeSeriesProperty<std::string>(propname);
        logs->addValue(timecolumn,valuecolumn);
        sMap.insert(spair(propname,logs));
      }
      std::string blockcolumnFileName=path+"_"+blockcolumn+".txt";
      if(blockcolumnFileExists(blockcolumnFileName))
      {
        blockFileNameList.insert(blockcolumnFileName);
      }
    }
  }
  try
  {
    std::map<std::string,Kernel::TimeSeriesProperty<double>*>::const_iterator itr=dMap.begin();
    for(;itr!=dMap.end();++itr)
    {
      run.addLogData(itr->second);
    }	
    std::map<std::string,Kernel::TimeSeriesProperty<std::string>*>::const_iterator sitr=sMap.begin();
    for(;sitr!=sMap.end();++sitr)
    {
      run.addLogData(sitr->second);
    }
  }
  catch(std::invalid_argument &e)
  {
    g_log.warning()<<e.what();
  }
  catch(Exception::ExistsError&e)
  {
    g_log.warning()<<e.what();
  }

  return blockFileNameList;

}

/** this method looks for file with second column(block column) name exists in the raw file directory
 * @param fileName :: -name of the file
 * @return True if the file exists
 */
bool LoadLog::blockcolumnFileExists(const std::string& fileName)
{
  if (Poco::File(fileName).exists()) return true;
  else return false;
}

/** Takes as input a string and try to determine what type it is.
 *  @param s :: The input string
 *  @param s ::  string to be classified
 *  @return A enum kind which tells what type the string is
 */
LoadLog::kind LoadLog::classify(const std::string& s) const
{
  if( s.empty() )
  {
    return LoadLog::empty;
  }

  using std::string;
  const string lower("abcdefghijklmnopqrstuvwxyz");
  const string upper("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  const string letters = lower + upper + '_';

  if (letters.find_first_of(s) != string::npos)
  {
    return LoadLog::string;
  }
  else
  {
    return LoadLog::number;
  }
}

/** change each element of the string to lower case
 * @param strToConvert :: The input string
 * @returns The string but with all characters in lower case
 */
std::string LoadLog::stringToLower(std::string strToConvert)
{
  std::transform(strToConvert.begin(), strToConvert.end(), strToConvert.begin(), tolower);
  return strToConvert;
}

/** Checks whether filename is a simple text file
 * @param filename :: The filename to inspect
 * @returns true if the filename has the .txt extension
 */
bool LoadLog::isAscii(const std::string& filename)
{
  FILE* file = fopen(filename.c_str(), "rb");
  char data[256];
  int n = fread(data, 1, sizeof(data), file);
  fclose(file);
  char *pend = &data[n];
  /*
   * Call it a binary file if we find a non-ascii character in the 
   * first 256 bytes of the file.
   */
  for( char *p = data;  p < pend; ++p )
  {
    unsigned long ch = (unsigned long)*p;
    if( !(ch <= 0x7F) )
    {
      return false;
    }
    
  }
  return true;
}

/** check if first 19 characters of a string is date-time string according to yyyy-mm-ddThh:mm:ss
 * @param str :: The string to test
 * @returns true if the strings format matched the expected date format
 */
bool LoadLog::isDateTimeString(const std::string& str) const
{
  return DateAndTime::string_isISO8601(str.substr(0,19));
}


/** Read a line of a SNS-style text file.
 *
 * @param str :: The string to test
 * @param out :: a vector that will be filled with the double values.
 * @return false if the format is NOT SNS style or a conversion failed.
 */
bool LoadLog::SNSTextFormatColumns(const std::string& str, std::vector<double> & out) const
{
  std::vector<std::string> strs;
  out.clear();
  boost::split(strs, str, boost::is_any_of("\t "));
  double val;
  // Every column must evaluate to a double
  for (size_t i=0; i<strs.size(); i++)
  {
    if (!Strings::convert<double>(strs[i],val))
      return false;
    else
      out.push_back(val);
  }
  // Nothing failed = it is that format.
  return true;
}

} // namespace DataHandling
} // namespace Mantid
