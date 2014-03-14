/*WIKI* 
SaveILLCosmosAscii is an export-only Ascii-based save format with no associated loader. It is based on a python script by Maximilian Skoda, written for the ISIS Reflectometry GUI
==== Limitations ====
While Files saved with SaveILLCosmosAscii can be loaded back into mantid using LoadAscii, the resulting workspaces won't be usful as the data written by SaveILLCosmosAscii is not in the normal X,Y,E,DX format.
*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/SaveILLCosmosAscii.h"
#include "MantidDataHandling/AsciiPointBase.h"
#include "MantidKernel/ArrayProperty.h"
#include <fstream>

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(SaveILLCosmosAscii)
    using namespace Kernel;
    using namespace API;

    // Initialise the logger
    Logger& SaveILLCosmosAscii::g_log = Logger::get("SaveILLCosmosAscii");

    /// Sets documentation strings for this algorithm
    void SaveILLCosmosAscii::initDocs()
    {
      this->setWikiSummary("Saves a 2D [[workspace]] to a tab separated ascii file with headers for extra information. ");
      this->setOptionalMessage("Saves a 2D workspace to a ascii file.");
    }
    
    void SaveILLCosmosAscii::extraProps()
    {
      declareProperty(new ArrayProperty<std::string>("LogList"),"List of logs to write to file.");
      declareProperty("UserContact", "", "Text to be written to the User-local contact field");
      declareProperty("Title", "", "Text to be written to the Title field");
    }

    void SaveILLCosmosAscii::extraHeaders(std::ofstream & file)
    {
      auto samp = m_ws->run();
      std::string instrument;
      std::string user;
      std::string title;
      std::string subtitle;
      std::string startDT;
      std::string endDT;
      try
      {instrument = m_ws->getInstrument()->getName();}
      catch (...)
      {instrument = "";}

      try
      {user = getProperty("UserContact");}
      catch (...)
      {user = "";}

      try
      {title = getProperty("Title");}
      catch (...)
      {title = "";}

      try
      {subtitle = samp.getLogData("run_title")->value();}
      catch (...)
      {subtitle = "";}

      try
      {startDT = samp.getLogData("run_start")->value();}
      catch (...)
      {startDT = "";}

      try
      {endDT = samp.getLogData("run_end")->value();}
      catch (...)
      {endDT = "";}

      file << "MFT" << std::endl;
      file << "Instrument: "<< instrument << std::endl;
      file << "User-local contact: " << user << std::endl; //add optional property
      file << "Title: " << title << std::endl;
      file << "Subtitle: " << subtitle << std::endl;
      file << "Start date + time: " << startDT << std::endl;
      file << "End date + time: " << endDT << std::endl;

      const std::vector<std::string> logList = getProperty("LogList");
      ///logs
      for (auto log = logList.begin(); log != logList.end(); ++log)
      {
        file << boost::lexical_cast<std::string>(*log) << ": " << boost::lexical_cast<std::string>(samp.getLogData(*log)->value()) << std::endl;
      }

      file << "Number of file format: 2" << std::endl;
      file << "Number of data points:" << m_sep << m_xlength << std::endl;
      file << std::endl;

      file << m_sep << "q" << m_sep << "refl" << m_sep << "refl_err" << m_sep << "q_res" << std::endl;
    }
  } // namespace DataHandling
} // namespace Mantid
