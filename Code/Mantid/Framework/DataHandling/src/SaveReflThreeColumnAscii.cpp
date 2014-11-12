//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/SaveReflThreeColumnAscii.h"
#include "MantidDataHandling/AsciiPointBase.h"
#include "MantidKernel/ArrayProperty.h"
#include <fstream>

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(SaveReflThreeColumnAscii)
    using namespace Kernel;
    using namespace API;
    
    /// virtual method to set the extra properties required for this algorithm
   void SaveReflThreeColumnAscii::extraProps()
    {
      declareProperty(new ArrayProperty<std::string>("LogList"),"List of logs to write to file.");
      declareProperty("Title", "", "Text to be written to the Title field");
    }

    /** virtual method to add information to the file before the data
     *  @param file :: pointer to output file stream
     */
    void SaveReflThreeColumnAscii::extraHeaders(std::ofstream & file)
    {
      auto samp = m_ws->run();
      std::string subtitle;
      std::string title = getProperty("Title");
      try
      {
        subtitle = samp.getLogData("run_title")->value();
      }
      catch (Kernel::Exception::NotFoundError &)
      {
        subtitle = "";
      }
      ////git change!!!!!!!!
      if (title != "") //if is toggled
      { 
        file << "#" << title << std::endl;
      }

      const std::vector<std::string> logList = getProperty("LogList");
      ///logs
      for (auto log = logList.begin(); log != logList.end(); ++log)
      {
        file << boost::lexical_cast<std::string>(*log) << ": " << boost::lexical_cast<std::string>(samp.getLogData(*log)->value()) << std::endl;
      }
    }
    
    void SaveReflThreeColumnAscii::data(std::ofstream & file, const std::vector<double> & XData)
    {
      AsciiPointBase::data(file, XData, false);
    }


  } // namespace DataHandling
} // namespace Mantid
