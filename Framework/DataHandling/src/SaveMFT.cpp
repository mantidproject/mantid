#include "MantidDataHandling/SaveMFT.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"

#include <fstream>

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveMFT)
using namespace Kernel;
// using namespace API;

/// virtual method to set the extra properties required for this algorithm
void SaveMFT::extraProps() {
  declareProperty(make_unique<ArrayProperty<std::string>>("LogList"),
                  "List of logs to write to file.");
  declareProperty("UserContact", "",
                  "Text to be written to the User-local contact field");
  declareProperty("Title", "", "Text to be written to the Title field");
}

/** virtual method to add information to the file before the data
 *  @param file :: pointer to output file stream
 */
void SaveMFT::extraHeaders(std::ofstream &file) {
  API::MatrixWorkspace_const_sptr m_ws = getProperty("InputWorkspace");
  if (!m_ws)
    throw std::runtime_error("Cannot treat InputWorkspace");
  auto samp = m_ws->run();
  std::string instrument{""};
  std::string user = getProperty("UserContact");
  std::string title = getProperty("Title");
  std::string subtitle{""};
  std::string startDT{""};
  std::string endDT{""};
  auto tempInst = m_ws->getInstrument();
  if (tempInst)
    instrument = tempInst->getName();
  try {
    subtitle = samp.getLogData("run_title")->value();
    startDT = samp.getLogData("run_start")->value();
    endDT = samp.getLogData("run_end")->value();
  } catch (Kernel::Exception::NotFoundError &) {
  }
  file << "MFT\n";
  file << "Instrument: " << instrument << '\n';
  file << "User-local contact: " << user << '\n';
  file << "Title: " << title << '\n';
  file << "Subtitle: " << subtitle << '\n';
  file << "Start date + time: " << startDT << '\n';
  file << "End date + time: " << endDT << '\n';
  const std::vector<std::string> logList = getProperty("LogList");
  for (const auto &log : logList) {
    file << boost::lexical_cast<std::string>(log) << ": "
         << boost::lexical_cast<std::string>(samp.getLogData(log)->value())
         << '\n';
  }
  file << "Number of file format: 2\n";
  file << "Number of data points: " << m_length << '\n';
  file << '\n';
  file << m_sep << "q" << m_sep << "refl" << m_sep << "refl_err" << m_sep
       << "q_res\n";
}
} // namespace DataHandling
} // namespace Mantid
