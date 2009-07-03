//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadLog.h"
#include "MantidAPI/LogParser.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidDataObjects/Workspace2D.h"

#include "Poco/File.h"
#include "Poco/Path.h"
#include "Poco/DirectoryIterator.h"

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
using DataObjects::Workspace2D;
using DataObjects::Workspace2D_sptr;

/// Empty default constructor
LoadLog::LoadLog()
{}

/// Initialisation method.
void LoadLog::init()
{
  // When used as a sub-algorithm the workspace name is not used - hence the "Anonymous" to satisfy the validator
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("Workspace","Anonymous",Direction::InOut),
    "The filename (including its full or relative path) of either an ISIS log file\n"
    "or an ISIS raw file. If a raw file is specified all log files associated with\n"
    "that raw file are loaded into the specified workspace. The file extension must\n"
    "either be .raw or .s when specifying a raw file" );
  declareProperty("Filename", "",
    "The name of the workspace to which the log data will be added");
  declareProperty("Period",1);
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

  Poco::File l_path( m_filename );

  if ( !l_path.exists() )
  {
    g_log.error("In LoadLog: " + m_filename + " does not exist.");
    throw Exception::FileError("File does not exist:" , m_filename);
  }

  if ( l_path.isDirectory() )
  {
    g_log.error("In LoadLog: " + m_filename + " must be a filename not a directory.");
    throw Exception::FileError("Filename is a directory:" , m_filename);
  }

  // Get the input workspace and retrieve sample from workspace.
  // the log file(s) will be loaded into the Sample container of the workspace 

  const MatrixWorkspace_sptr localWorkspace = getProperty("Workspace");
  boost::shared_ptr<API::Sample> sample = localWorkspace->getSample();

  int Period = getProperty("Period");

  // If m_filename is the filename of a raw datafile then search for potential log files
  // in the directory of this raw datafile. Otherwise check if m_filename is a potential
  // log file. Add the filename of these potential log files to: potentialLogFiles.

  std::vector<std::string> potentialLogFiles;

  
  // start the process or populating potential log files into the container: potentialLogFiles

  std::string l_filenamePart = Poco::Path(l_path.path()).getFileName();// get filename part only

  bool rawFile = false;// Will be true if Filename property is a name of a RAW file
  if ( isLogFile(l_filenamePart) )
  {
    // then we will assume that m_filename is an ISIS log file
    potentialLogFiles.push_back(m_filename);
  }
  else if ( ( stringToLower(l_filenamePart).find(".raw") != std::string::npos ||
              stringToLower(l_filenamePart).find(".s") != std::string::npos ) && l_filenamePart.size() >= 10 )
  {
    // then we will assume that m_filename is an ISIS raw file
      rawFile = true;

    // strip out the raw data file identifier

    size_t l_pos = l_filenamePart.find(".");

    std::string l_rawID = stringToLower(l_filenamePart.substr(0,l_pos));

    // look for log files in the directory of the raw datafile
    Poco::DirectoryIterator end_iter;
    for ( Poco::DirectoryIterator dir_itr( Poco::Path(l_path.path()).makeAbsolute().parent() ); dir_itr != end_iter; ++dir_itr )
    {
      if ( !Poco::File(dir_itr->path() ).isFile() ) continue;

      l_filenamePart = Poco::Path(dir_itr->path()).getFileName();
      if ( !isLogFile(l_filenamePart) ) continue;
	
      if ( stringToLower(l_filenamePart).find(l_rawID) != std::string::npos )
      {
	  potentialLogFiles.push_back( dir_itr->path() );
      }

    }
  }
  else
  {
    g_log.error("In LoadLog: " + m_filename + " found to be neither a raw datafile nor a log file.");
    throw Exception::FileError("Filename found to be neither a raw datafile nor a log file." , m_filename);    
  }


  // Attempt to load the content of each potential log file into the Sample object

  std::vector<std::string>::iterator file = potentialLogFiles.begin();
  for (; file != potentialLogFiles.end(); file++)
  {
    // open log file

    std::ifstream inLogFile(file->c_str());

    if (!inLogFile)
    {
      // Unable to open file
      g_log.error("Unable to open file " + (*file));
      throw Exception::FileError("Unable to open file:" , (*file));
    }


    // figure out if second column is a number or a string

    std::string aLine;
    std::string dateAndTime;

    kind l_kind(LoadLog::empty);

    if( std::getline(inLogFile, aLine, '\n') )
    {
      if ( !isDateTimeString(aLine) )
      {
        g_log.warning("File" + (*file) + " is not a standard ISIS log file. Expected to be a two column file.");
        inLogFile.close();
        file = potentialLogFiles.erase(file);
        if (file == potentialLogFiles.end()) break;
        continue;
      }

      std::stringstream ins(aLine);

      ins >> dateAndTime;

      // read in what follows the date-time string in the log file and figure out
      // what type it is

      std::string whatType;

      ins >> whatType;

      l_kind = classify(whatType);

      if ( LoadLog::string != l_kind && LoadLog::number != l_kind )
      {
        g_log.warning("ISIS log file contains unrecognised second column entries: " + (*file));
        inLogFile.close();
        file = potentialLogFiles.erase(file);
        if (file == potentialLogFiles.end()) break;
        continue;
      }
    } 

    inLogFile.close();
  } // end for

  // Extract the common part of log file names (the workspace name)
  std::string ws_name = Poco::Path(m_filename).getFileName();
  ws_name.erase(ws_name.find_last_of('.'));
  ws_name += '_';


  // Find the icpevent filename
  std::string icpevent_file_name;
  for(size_t i=0;i<potentialLogFiles.size();i++)
  {
      if (stringToLower(potentialLogFiles[i]).find("icpevent") != std::string::npos)
      {
          icpevent_file_name = potentialLogFiles[i];
          break;
      }
  }

  API::LogParser parser(icpevent_file_name);

  // Add mantid-created logs
  Property* log = parser.createPeriodLog(Period);
  if (log)
  {
      sample->addLogData(log);
  }
  sample->addLogData(parser.createAllPeriodsLog());
  sample->addLogData(parser.createRunningLog());

  // Add log data from the files
  for(size_t i=0;i<potentialLogFiles.size();i++)
  {
      try
      {
          // Make the property name by removing the workspce name and file extension from the log filename
          std::string log_name = Poco::Path(potentialLogFiles[i]).getFileName();

          if (rawFile)
              log_name.erase(0,ws_name.size());

          size_t j = log_name.find_last_of('.');
          if (j != std::string::npos)
              log_name.erase(j);

          Property* log = parser.createLogProperty(potentialLogFiles[i],stringToLower(log_name));
          if (log)
              sample->addLogData(log);
      }
      catch(...)
      {
          continue;
      }
  }

  // operation was a success and ended normally
  return;
}

/** Takes as input a string and try to determine what type it is.
 *  @param s The input string
 *  @param s  string to be classified
 *  @return A enum kind which tells what type the string is
 */
LoadLog::kind LoadLog::classify(const std::string& s)
{
  using std::string;
  const string lower("abcdefghijklmnopqrstuvwxyz");
  const string upper("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  const string letters = lower + upper + '_';

  if (s.empty())
  return LoadLog::empty;

  if (letters.find_first_of(s) != string::npos)
  return LoadLog::string;
  else
  return LoadLog::number;
}

/** change each element of the string to lower case
* @param strToConvert The input string
* @returns The string but with all characters in lower case
*/
std::string LoadLog::stringToLower(std::string strToConvert)
{
  for(unsigned int i=0;i<strToConvert.length();i++)
  {
    strToConvert[i] = tolower(strToConvert[i]);
  }
  return strToConvert; //return the converted string
}

/** looks whether filename has the .txt extension and contain a '_'
* @param filenamePart The filename to inspect
* @returns true if the filename has the .txt extension and contain a '_'
*/
bool LoadLog::isLogFile(const std::string& filenamePart)
{
  if ( stringToLower(filenamePart).find(".txt") != std::string::npos && filenamePart.find("_") != std::string::npos )
  return true;
  else
  return false;
}

/** check if first 19 characters of a string is data-time string according to yyyy-mm-ddThh:mm:ss
* @param str The string to test
* @returns true if the strings format matched the expected date format
*/
bool LoadLog::isDateTimeString(const std::string& str)
{
  if ( str.size() >= 19 )
  if ( str.compare(4,1,"-") == 0 && str.compare(7,1,"-") == 0 && str.compare(13,1,":") == 0
      && str.compare(16,1,":") == 0 && str.compare(10,1,"T") == 0 )
  return true;

  return false;
}

} // namespace DataHandling
} // namespace Mantid
