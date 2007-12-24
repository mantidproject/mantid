//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadLog.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/Workspace2D.h"

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

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
using API::Workspace;
using API::Workspace_sptr;
using DataObjects::Workspace2D;
using DataObjects::Workspace2D_sptr;

namespace fs = boost::filesystem; // to help clarify which bits are boost in code below

Logger& LoadLog::g_log = Logger::get("LoadLog");

/// Empty default constructor
LoadLog::LoadLog()
{}

/// Initialisation method.
void LoadLog::init()
{
  declareProperty(new WorkspaceProperty<Workspace>("Workspace","",Direction::InOut));
  declareProperty("Filename","");
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

  fs::path l_path( m_filename );

  if ( !fs::exists( l_path ) )
  {
    g_log.error("In LoadLog: " + m_filename + " does not exist.");
    throw Exception::FileError("File does not exist:" , m_filename);
  }

  if ( fs::is_directory( l_path ) )
  {
    g_log.error("In LoadLog: " + m_filename + " must be a filename not a directory.");
    throw Exception::FileError("Filename is a directory:" , m_filename);
  }


  // Get the input workspace and retrieve sample from workspace.
  // the log file(s) will be loaded into the Sample container of the workspace 

  Workspace_sptr localWorkspace = getProperty("Workspace");
  API::Sample& sample = localWorkspace->getSample();


  // If m_filename is the filename of a raw datafile then search for potential log files
  // in the directory of this raw datafile. Otherwise check if m_filename is a potential
  // log file. Add the filename of these potential log files to: potentialLogFiles.

  std::vector<std::string> potentialLogFiles;

  
  // start the process or populating potential log files into the container: potentialLogFiles

  std::string l_filenamePart = l_path.leaf();  // get filename part only

  if ( isLogFile(l_filenamePart) )
  {
    // then we will assume that m_filename is an ISIS log file

    potentialLogFiles.push_back(m_filename);
  }
  else if ( ( stringToLower(l_filenamePart).find(".raw") != std::string::npos ||
              stringToLower(l_filenamePart).find(".s") != std::string::npos ) && l_filenamePart.size() >= 12 )
  {
    // then we will assume that m_filename is an ISIS raw file

    // strip out the raw data file identifier

    size_t l_pos = l_filenamePart.find(".");

    std::string l_rawID = stringToLower(l_filenamePart.substr(0,l_pos));

    // look for log files in the directory of the raw datafile

    l_path = l_path.remove_leaf();

    fs::directory_iterator end_iter;
    for ( fs::directory_iterator dir_itr( l_path ); dir_itr != end_iter; ++dir_itr )
    {
      if ( fs::is_regular( dir_itr->status() ) )
      {
        l_filenamePart = dir_itr->path().leaf();
        if ( isLogFile(l_filenamePart) )
        if ( stringToLower(l_filenamePart).find(l_rawID) != std::string::npos )
        potentialLogFiles.push_back( dir_itr->path().string() );
      }
    }
  }
  else
  {
    g_log.error("In LoadLog: " + m_filename + " found to be neither a raw datafile nor a log file.");
    throw Exception::FileError("Filename found to be neither a raw datafile nor a log file." , m_filename);    
  }


  // Attempt to load the content of each potential log file into the Sample object

  for (unsigned int i = 0; i < potentialLogFiles.size(); i++)
  {
    // open log file

    std::ifstream inLogFile(potentialLogFiles[i].c_str());

    if (!inLogFile)
    {
      // Unable to open file
      g_log.error("Unable to open file " + potentialLogFiles[i]);
      throw Exception::FileError("Unable to open file:" , potentialLogFiles[i]);
    }


    // figure out if second column is a number or a string

    std::string aLine;
    std::string dateAndTime;

    kind l_kind;

    if( std::getline(inLogFile, aLine, '\n') )
    {
      if ( !isDateTimeString(aLine) )
      {
        g_log.warning("File" + potentialLogFiles[i] + " is not a standard ISIS log file. Expected to be a two column file.");
        inLogFile.close();
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
        g_log.error("File" + potentialLogFiles[i] + " is not a ISIS log file. Can't recognise TYPE");
        throw Exception::FileError("ISIS log file contains unrecognised second column entries:", potentialLogFiles[i]);
      }
    } 


    // reset random access to beginning

    inLogFile.seekg(0, std::ios::beg);


    // Read log file into Property which is then stored in Sample object

    TimeSeriesProperty<std::string> *l_PropertyString = new TimeSeriesProperty<std::string>(potentialLogFiles[i]);
    TimeSeriesProperty<double> *l_PropertyDouble = new TimeSeriesProperty<double>(potentialLogFiles[i]);

    // read in the log file

    bool l_JumpToNextLogFile = false;

    while ( std::getline(inLogFile, aLine, '\n') )
    {
      if ( !isDateTimeString(aLine) )
      {
        g_log.warning("File" + potentialLogFiles[i] + " is not a standard ISIS log file. Expected to be two a column file.");
        l_JumpToNextLogFile = true;
        inLogFile.close();
        break; // break out of while look
      }

      std::istringstream ins(aLine);

      ins >> dateAndTime;

      // strim down the date-time string to format YYYYMMDDTHHmmss

      dateAndTime.erase(16,1);
      dateAndTime.erase(13,1);
      dateAndTime.erase(7,1);
      dateAndTime.erase(4,1);

      // Store log file line in Property

      if ( l_kind == LoadLog::number )
      {
        double readNumber;

        ins >> readNumber;

        l_PropertyDouble->addValue(dateAndTime, readNumber);
      }
      else
      {
        l_PropertyString->addValue(dateAndTime, aLine.erase(0,19));
      }

    } // end while

    if ( l_JumpToNextLogFile )
    {
      continue; // jump to next log file
    }

    // store Property in Sample object

    if ( l_kind == LoadLog::number )
    {
      sample.addLogData(l_PropertyDouble);
    }
    else
    {
      sample.addLogData(l_PropertyString);
    }

    inLogFile.close();
  } // end for


  // operation was a success and ended normally
  return;
}

/** Finalisation method. Does nothing at present.
 *   
 */
void LoadLog::final()
{
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

/// change each element of the string to lower case
std::string LoadLog::stringToLower(std::string strToConvert)
{
  for(unsigned int i=0;i<strToConvert.length();i++)
  {
    strToConvert[i] = tolower(strToConvert[i]);
  }
  return strToConvert; //return the converted string
}

/// looks whether filename has the .txt extension and contain a '_'
bool LoadLog::isLogFile(const std::string& filenamePart)
{
  if ( stringToLower(filenamePart).find(".txt") != std::string::npos && filenamePart.find("_") != std::string::npos )
  return true;
  else
  return false;
}

/// check if first 19 characters of a string is data-time string according to yyyy-mm-ddThh:mm:ss
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
