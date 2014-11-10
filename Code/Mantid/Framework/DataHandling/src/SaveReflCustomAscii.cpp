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
      declareProperty("WriteXError", false, "If true, the error on X will be written as the fourth column.");
    }

    /** virtual method to add information to the file before the data
     *  @param file :: pointer to output file stream
     */
    void SaveReflCustomAscii::extraHeaders(std::ofstream & file)
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
    /*void SaveReflCustomAscii::data(std::ofstream & file, const std::vector<double> & XData)
    {
      const std::vector<double> & yData = m_ws->readY(0);
      const std::vector<double> & eData = m_ws->readE(0);
      {if (1){
      for (size_t i = 0; i < m_xlength; ++i)
      {
        double dq = XData[i]*m_qres;
        outputval(XData[i], file, leadingSep());
        outputval(yData[i], file);
        outputval(eData[i], file);
        outputval(dq, file);
        file << std::endl;
      }}
      else
      {
        for (size_t i = 0; i < m_xlength; ++i)
      {
        outputval(XData[i], file, leadingSep());
        outputval(yData[i], file);
        outputval(eData[i], file);
        file << std::endl;
      }
      }};*/
  } // namespace DataHandling
} // namespace Mantid
