//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/SaveReflCustomAscii.h"
#include "MantidDataHandling/AsciiPointBase.h"
#include "MantidKernel/ArrayProperty.h"
#include <fstream>

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(SaveReflCustomAscii)
    using namespace Kernel;
    using namespace API;
    
    /// virtual method to set the extra properties required for this algorithm
    void SaveReflCustomAscii::extraProps()
    {
      declareProperty(new ArrayProperty<std::string>("LogList"),"List of logs to write to file.");
      declareProperty("UserContact", "", "Text to be written to the User-local contact field");
      declareProperty("Title", "", "Text to be written to the Title field");
    }

    /** virtual method to add information to the file before the data
     *  @param file :: pointer to output file stream
     */
    void SaveReflCustomAscii::extraHeaders(std::ofstream & file)
    {
      auto samp = m_ws->run();
      std::string title = getProperty("Title");

      //if (title) //if is toggled
      //{ 
      //  file << "#" << title << std::endl;
      //}

      const std::vector<std::string> logList = getProperty("LogList");
      ///logs
      for (auto log = logList.begin(); log != logList.end(); ++log)
      {
        file << boost::lexical_cast<std::string>(*log) << ": " << boost::lexical_cast<std::string>(samp.getLogData(*log)->value()) << std::endl;
      }

      file << "Number of file format: 2" << std::endl;
      file << "Number of data points:" << sep() << m_xlength << std::endl;
      file << std::endl;

      file << sep() << "q" << sep() << "refl" << sep() << "refl_err" << sep() << "q_res" << std::endl;
    }
  } // namespace DataHandling
} // namespace Mantid
