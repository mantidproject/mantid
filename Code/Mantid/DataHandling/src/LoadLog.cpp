//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadLog.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidDataObjects/Workspace2D.h"

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include <fstream>  // used to get ifstream
#include <sstream>
//#include <ctype.h>

namespace Mantid
{
namespace DataHandling
{

  // Register the algorithm into the algorithm factory
  DECLARE_ALGORITHM(LoadLog)

  using namespace Kernel;
	using DataObjects::Workspace2D;
  
  namespace fs = boost::filesystem;  // to help clarify which bits are boost in code below

  Logger& LoadLog::g_log = Logger::get("LoadLog");

  /// Empty default constructor
  LoadLog::LoadLog() { }

  /** Initialisation method.
   * 
   *  @return A StatusCode object indicating whether the operation was successful
   */
  StatusCode LoadLog::init()
  {
    declareProperty("Filename",".");
    
    return StatusCode::SUCCESS;
  }
  
  /** Executes the algorithm. Reading in the ISIS log file, for now, directly into 
   *  a TimeSeriesProperties object
   * 
   *  @return A StatusCode object indicating whether the operation was successful
   */
  StatusCode LoadLog::exec()
  {
    // Retrieve the filename from the properties

    try 
    {
      m_filename = getPropertyValue("Filename");
    } 
    catch (Exception::NotFoundError e) 
    {
      g_log.error("Filename property has not been set.");
      return StatusCode::FAILURE;      
    }


	  // Retrieve the ws names from the properties

	  std::string inputWorkspaceName;
	  std::string outputWorkspaceName;
	  try
	  {
		  inputWorkspaceName = getPropertyValue("InputWorkspace");
	  }
	  catch (Kernel::Exception::NotFoundError& ex)
	  {
	    g_log.debug("InputWorkspace has not been set.");
	  }

	  try
	  {
		  outputWorkspaceName = getPropertyValue("OutputWorkspace");
	  }
	  catch (Kernel::Exception::NotFoundError& ex)
	  {
	    g_log.error("OutputWorkspace has not been set.");
	    return StatusCode::FAILURE;
	  }


	  // Create the 2D workspace for the output
	  // Get a pointer to the workspace factory (later will be shared)

	  if (inputWorkspaceName != outputWorkspaceName)
	  {
		  API::WorkspaceFactory *factory = API::WorkspaceFactory::Instance();
		  m_outputWorkspace = factory->create("Workspace2D");
	  }
	  else
	  {
		  m_outputWorkspace = m_inputWorkspace;
	  }

	  Workspace2D *localWorkspace = dynamic_cast<Workspace2D*>(m_outputWorkspace);

    API::Sample& sample = localWorkspace->getSample();


    // figure out if m_filename is the filename of a raw datafile or the filename
    // of a log file. 

    fs::path l_path( m_filename );

    if ( !fs::exists( l_path ) )
    {
      g_log.error("In LoadLog: " + m_filename + " does not exist."); 
	    return StatusCode::FAILURE;
    }

    if ( fs::is_directory( l_path ) )
    {
      g_log.error("In LoadLog: " + m_filename + " must be a filename not a directory."); 
	    return StatusCode::FAILURE;
    }
    

    std::vector<std::string> potentialLogFiles;

    // strip out filename part

    std::string l_filenamePart = l_path.leaf();

    if ( isLogFile(l_filenamePart) )
    {
      // then we will assume that m_filename is an ISIS log file

      potentialLogFiles.push_back(m_filename);
    }
    else if ( stringToLower(l_filenamePart).find(".raw") != std::string::npos && l_filenamePart.size() >= 12 )
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

 
    for (unsigned int i = 0; i < potentialLogFiles.size(); i++)
    {

      // open log file
    
      std::ifstream inLogFile(potentialLogFiles[i].c_str());

      if (!inLogFile)
      {
        // Unable to open file
        g_log.error("Unable to open file " + potentialLogFiles[i]);
        return StatusCode::FAILURE;
      }
  

      // figure out if second column is a number or a string

      std::string aLine;
      std::string dateAndTime;

      kind l_kind;



      while ( std::getline(inLogFile, aLine, '\n') ) 
      {

        if ( aLine.size() < 19 )
        {
          // A date-time string in a log file is 19 characters
          // hence if the first line is not that length sometime
          // is not right!
          g_log.error("File" + potentialLogFiles[i] + " is not a ISIS log file.");
          return StatusCode::FAILURE;
        }

        std::stringstream ins(aLine);
      
        ins >> dateAndTime;


        // read in what follows the date-time string and figure out
        // what type it is

        std::string whatType;

        ins >> whatType;

        l_kind = classify(whatType);

        if ( LoadLog::string != l_kind && LoadLog::number != l_kind )
        {
          g_log.error("File" + potentialLogFiles[i] + " is not a ISIS log file. Can't recognise TYPE");
          return StatusCode::FAILURE;
        }

        break;
      } // end while


      // reset random access to beginning

      inLogFile.seekg(0, std::ios::beg);

/*
      Property *l_Property;

      if ( l_kind == LoadLog::number )
        l_Property = new TimeSeriesProperty<std::string>(m_filename);
      else
        l_Property = new TimeSeriesProperty<double>(m_filename);
*/
//      TimeSeriesProperty<std::string> *l_PropertyString = dynamic_cast<TimeSeriesProperty<std::string>*>(l_Property);
//      TimeSeriesProperty<double> *l_PropertyDouble = dynamic_cast<TimeSeriesProperty<double>*>(l_PropertyDouble);


      
      TimeSeriesProperty<std::string> *l_PropertyString = new TimeSeriesProperty<std::string>(m_filename);
      TimeSeriesProperty<double> *l_PropertyDouble = new TimeSeriesProperty<double>(m_filename);


      // read in the log file

      while ( std::getline(inLogFile, aLine, '\n') ) 
      {

        if ( aLine.size() < 19 )
        {
          // A date-time string in a log file is 19 characters
          // hence if the first line is not that length sometime
          // is not right!
          g_log.error("File" + potentialLogFiles[i] + " is not a ISIS log file.");
          return StatusCode::FAILURE;
        }

        std::istringstream ins(aLine);
      
        ins >> dateAndTime;

        // strim down the date-time string to format YYYYMMDDTHHmmss

        dateAndTime.erase(16,1);
        dateAndTime.erase(13,1);
        dateAndTime.erase(7,1);
        dateAndTime.erase(4,1);

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


      if ( l_kind == LoadLog::number )
      {
        l_PropertyDouble->printMapToScreen();
        // Property *l_PropertyDouble2 = dynamic_cast<Property*>(l_PropertyDouble);
        //sample.addLogData(l_PropertyDouble);
      }
      else
      {
        l_PropertyString->printMapToScreen();
        //sample.addLogData(l_PropertyString);
      }
//      sample.addLogData(l_Property);


      inLogFile.close();
    } // end for
 
    return StatusCode::SUCCESS;
  }


  /** Finalisation method. Does nothing at present.
   *
   *  @return A StatusCode object indicating whether the operation was successful
   */
	StatusCode LoadLog::final()
  {
    return StatusCode::SUCCESS;
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


} // namespace DataHandling
} // namespace Mantid
