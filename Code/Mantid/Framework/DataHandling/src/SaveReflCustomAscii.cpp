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
      declareProperty("Title", "", "Text to be written to the Title field");
      declareProperty("WriteDeltaQ", false, "If true, the error on DeltaQ will be written as the fourth column."); 
      declareProperty("Subtitle", false, "If true, subtitle added to header.");
    }

    /** virtual method to add information to the file before the data
     *  @param file :: pointer to output file stream
     */
    void SaveReflCustomAscii::extraHeaders(std::ofstream & file)
    {
      auto samp = m_ws->run();
      bool subtitle = getProperty("Subtitle");
      std::string subtitleEntry;
      std::string title = getProperty("Title");

      if (title != "") //if is toggled
      { 
        file << "#" << title << std::endl;
      }

      if(subtitle){
        try
        {
          subtitleEntry = samp.getLogData("run_title")->value();
        }
        catch (Kernel::Exception::NotFoundError &)
        {
          subtitleEntry = "";
        }
      }

      file << "#" << subtitleEntry << std::endl;
      
      const std::vector<std::string> logList = getProperty("LogList");
      ///logs
      for (auto log = logList.begin(); log != logList.end(); ++log)
      {
        file << boost::lexical_cast<std::string>(*log) << ": " << boost::lexical_cast<std::string>(samp.getLogData(*log)->value()) << std::endl;
      }
    }
    
    void SaveReflCustomAscii::data(std::ofstream & file, const std::vector<double> & XData, bool exportDeltaQ)
    {
      exportDeltaQ = getProperty("WriteDeltaQ");
      AsciiPointBase::data(file, XData, exportDeltaQ);
    }


  } // namespace DataHandling
} // namespace Mantid
