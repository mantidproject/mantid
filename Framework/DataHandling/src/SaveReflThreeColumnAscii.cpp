#include "MantidDataHandling/SaveReflThreeColumnAscii.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidDataHandling/AsciiPointBase.h"
#include "MantidKernel/ArrayProperty.h"

#include <fstream>

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveReflThreeColumnAscii)
using namespace Kernel;
using namespace API;

/// virtual method to set the extra properties required for this algorithm
void SaveReflThreeColumnAscii::extraProps() {
  declareProperty("Title", "", "Text to be written to the Title field");
  declareProperty(make_unique<ArrayProperty<std::string>>("LogList"),
                  "List of logs to write to file.");
  appendSeparatorProperty();
}

/** virtual method to add information to the file before the data
 *  @param file :: pointer to output file stream
 */
void SaveReflThreeColumnAscii::extraHeaders(std::ofstream &file) {
  auto samp = m_ws->run();
  std::string title = getProperty("Title");

  if (!title.empty()) // if is toggled
  {
    file << "#" << title << '\n';
  }

  const std::vector<std::string> logList = getProperty("LogList");
  /// logs
  for (const auto &log : logList) {
    file << boost::lexical_cast<std::string>(log) << ": "
         << boost::lexical_cast<std::string>(samp.getLogData(log)->value())
         << '\n';
  }
}

/** virtual method to add information to the file before the data
 *  @param file :: pointer to output file stream
 *  @param XData :: pointer to a std::vector<double> containing the point data
 * to be printed
 *  @param exportDeltaQ :: bool on whether deltaQ column to be printed
 * (permanantly false in this case)
 */
void SaveReflThreeColumnAscii::data(std::ofstream &file,
                                    const std::vector<double> &XData,
                                    bool exportDeltaQ) {
  exportDeltaQ = false;
  AsciiPointBase::data(file, XData, exportDeltaQ);
}

} // namespace DataHandling
} // namespace Mantid
