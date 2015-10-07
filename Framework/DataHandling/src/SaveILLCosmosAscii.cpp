//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/SaveILLCosmosAscii.h"
#include "MantidDataHandling/AsciiPointBase.h"
#include "MantidKernel/ArrayProperty.h"
#include <fstream>

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveILLCosmosAscii)
using namespace Kernel;
using namespace API;

/// virtual method to set the extra properties required for this algorithm
void SaveILLCosmosAscii::extraProps() {
  declareProperty(new ArrayProperty<std::string>("LogList"),
                  "List of logs to write to file.");
  declareProperty("UserContact", "",
                  "Text to be written to the User-local contact field");
  declareProperty("Title", "", "Text to be written to the Title field");
}

/** virtual method to add information to the file before the data
 *  @param file :: pointer to output file stream
 */
void SaveILLCosmosAscii::extraHeaders(std::ofstream &file) {
  auto samp = m_ws->run();
  std::string instrument;
  std::string user = getProperty("UserContact");
  std::string title = getProperty("Title");
  std::string subtitle;
  std::string startDT;
  std::string endDT;
  auto tempInst = m_ws->getInstrument();
  if (tempInst) {
    instrument = tempInst->getName();
  }

  try {
    subtitle = samp.getLogData("run_title")->value();
  } catch (Kernel::Exception::NotFoundError &) {
    subtitle = "";
  }

  try {
    startDT = samp.getLogData("run_start")->value();
  } catch (Kernel::Exception::NotFoundError &) {
    startDT = "";
  }

  try {
    endDT = samp.getLogData("run_end")->value();
  } catch (Kernel::Exception::NotFoundError &) {
    endDT = "";
  }

  file << "MFT" << std::endl;
  file << "Instrument: " << instrument << std::endl;
  file << "User-local contact: " << user << std::endl; // add optional property
  file << "Title: " << title << std::endl;
  file << "Subtitle: " << subtitle << std::endl;
  file << "Start date + time: " << startDT << std::endl;
  file << "End date + time: " << endDT << std::endl;

  const std::vector<std::string> logList = getProperty("LogList");
  /// logs
  for (auto log = logList.begin(); log != logList.end(); ++log) {
    file << boost::lexical_cast<std::string>(*log) << ": "
         << boost::lexical_cast<std::string>(samp.getLogData(*log)->value())
         << std::endl;
  }

  file << "Number of file format: 2" << std::endl;
  file << "Number of data points:" << sep() << m_xlength << std::endl;
  file << std::endl;

  file << sep() << "q" << sep() << "refl" << sep() << "refl_err" << sep()
       << "q_res" << std::endl;
}
} // namespace DataHandling
} // namespace Mantid
