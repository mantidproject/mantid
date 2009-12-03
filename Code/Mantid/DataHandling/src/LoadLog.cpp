//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadLog.h"
#include "MantidAPI/LogParser.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Glob.h"
#include "MantidKernel/FileProperty.h"
#include "LoadRaw/isisraw2.h"

#include "Poco/File.h"
#include "Poco/Path.h"
#include "Poco/DirectoryIterator.h"
#include "Poco/DateTimeParser.h"
#include "Poco/DateTimeFormat.h"
#include "Poco/RegularExpression.h"

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
    "The name of the workspace to which the log data will be added");

  std::vector<std::string> exts(4, "");
  exts[0] = "txt";
  exts[1] = "raw";
  exts[2] = "s*";
  exts[3] = "add";
  declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
		  "The filename (including its full or relative path) of either an ISIS log file\n"
		  "or an ISIS raw file. If a raw file is specified all log files associated with\n"
		  "that raw file are loaded into the specified workspace. The file extension must\n"
		  "either be .raw or .s when specifying a raw file");

  declareProperty("Period",1);
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

  // Get the input workspace and retrieve sample from workspace.
  // the log file(s) will be loaded into the Sample container of the workspace 

  const MatrixWorkspace_sptr localWorkspace = getProperty("Workspace");
  boost::shared_ptr<API::Sample> sample = localWorkspace->getSample();

  // If m_filename is the filename of a raw datafile then search for potential log files
  // in the directory of this raw datafile. Otherwise check if m_filename is a potential
  // log file. Add the filename of these potential log files to: potentialLogFiles.
  std::set<std::string> potentialLogFiles;
  
  // start the process or populating potential log files into the container: potentialLogFiles
  std::string l_filenamePart = Poco::Path(l_path.path()).getFileName();// get filename part only

  bool rawFile = false;// Will be true if Filename property is a name of a RAW file
  if ( isAscii(m_filename) && l_filenamePart.find("_") != std::string::npos )
  {
    // then we will assume that m_filename is an ISIS log file
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

  //If there are no log files by now, we have nothing else to do
  if( potentialLogFiles.empty() ) return;

  //Do a quick search for the icpevent file
  std::string icpevent_file_name("");
  std::set<std::string>::const_iterator icpfile = find_if(potentialLogFiles.begin(), potentialLogFiles.end(), FileMatcher(std::string(".*icpevent.*")));
  if( icpfile != potentialLogFiles.end() )
  {
    icpevent_file_name = *icpfile;
  }

  API::LogParser parser(icpevent_file_name);
  // Add mantid-created logs
  int period = getProperty("Period");
  Property* log = parser.createPeriodLog(period);
  if (log)
  {
    sample->addLogData(log);
  }
  sample->addLogData(parser.createAllPeriodsLog());
  sample->addLogData(parser.createRunningLog());

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
      // Unable to open file
      g_log.error("Unable to open file " + filename);
      throw Exception::FileError("Unable to open file:" , filename);
    }
    // figure out if second column is a number or a string
    std::string aLine;
    if( std::getline(inLogFile, aLine, '\n') )
    {
      if ( !isDateTimeString(aLine) )
      {
        g_log.warning("File" + filename + " is not a standard ISIS log file. Expected to be a two column file.");
        inLogFile.close();
        continue;
      }
      std::string dateAndTime;
      std::stringstream ins(aLine);
      ins >> dateAndTime;

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
	  sample->addLogData(log);
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

/** Takes as input a string and try to determine what type it is.
 *  @param s The input string
 *  @param s  string to be classified
 *  @return A enum kind which tells what type the string is
 */
LoadLog::kind LoadLog::classify(const std::string& s)
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
* @param strToConvert The input string
* @returns The string but with all characters in lower case
*/
std::string LoadLog::stringToLower(std::string strToConvert)
{
  std::transform(strToConvert.begin(), strToConvert.end(), strToConvert.begin(), tolower);
  return strToConvert;
}

/** Checks whether filename is a simple text file
* @param filename The filename to inspect
* @returns true if the filename has the .txt extension
*/
bool LoadLog::isAscii(const std::string& filename)
{
  FILE* file = fopen(filename.c_str(), "rb");
  char data[256];
  int n = fread(data, 1, sizeof(data), file);
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
* @param str The string to test
* @returns true if the strings format matched the expected date format
*/
bool LoadLog::isDateTimeString(const std::string& str)
{
  Poco::DateTime dt;
  int tz_diff;
  return Poco::DateTimeParser::tryParse(Poco::DateTimeFormat::ISO8601_FORMAT, str.substr(0,19), dt, tz_diff);
}

} // namespace DataHandling
} // namespace Mantid
