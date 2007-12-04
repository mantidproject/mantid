//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadLog.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <fstream>  // used to get ifstream
#include <sstream>

namespace Mantid
{
namespace DataHandling
{

  // Register the algorithm into the algorithm factory
  DECLARE_ALGORITHM(LoadLog)

  using namespace Kernel;

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
    try {
      m_filename = getPropertyValue("Filename");
    } catch (Exception::NotFoundError e) {
      g_log.error("Filename property has not been set.");
      return StatusCode::FAILURE;      
    }
    

    // open log file
    
    std::ifstream inLogFile(m_filename.c_str());

    if (!inLogFile)
    {
      // Unable to open file
      g_log.error("Unable to open file " + m_filename);
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
        g_log.error("File" + m_filename + " is not a ISIS log file.");
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
        g_log.error("File" + m_filename + " is not a ISIS log file. Can't recognise TYPE");
        return StatusCode::FAILURE;
      }

      break;
    }

    // reset random access to beginning

    inLogFile.seekg(0, std::ios::beg);


    // this a temporary solution

    TimeSeriesProperty<std::string> timeSeriesString(m_filename);
    TimeSeriesProperty<double> timeSeriesDouble(m_filename);


    // read in the log file

    while ( std::getline(inLogFile, aLine, '\n') ) 
    {

      if ( aLine.size() < 19 )
      {
        // A date-time string in a log file is 19 characters
        // hence if the first line is not that length sometime
        // is not right!
        g_log.error("File" + m_filename + " is not a ISIS log file.");
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

        timeSeriesDouble.addValue(dateAndTime, readNumber);
      }
      else
      {
        timeSeriesString.addValue(dateAndTime, aLine.erase(0,19));
      }

    }


    // for debugging
    //timeSeriesDouble.printMapToScreen();
    //timeSeriesString.printMapToScreen();


    inLogFile.close();
 
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


} // namespace DataHandling
} // namespace Mantid
