// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveReflThreeColumnAscii.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidDataHandling/AsciiPointBase.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveReflThreeColumnAscii)
using namespace Kernel;
using namespace API;

/// virtual method to set the extra properties required for this algorithm
void SaveReflThreeColumnAscii::extraProps() {
  declareProperty("Title", "", "Text to be written to the Title field");
  declareProperty(std::make_unique<ArrayProperty<std::string>>("LogList"),
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
 *  @param exportDeltaQ :: bool on whether deltaQ column to be printed
 * (permanantly false in this case)
 */
void SaveReflThreeColumnAscii::data(std::ofstream &file, bool exportDeltaQ) {
  exportDeltaQ = false;
  AsciiPointBase::data(file, exportDeltaQ);
}

} // namespace DataHandling
} // namespace Mantid
