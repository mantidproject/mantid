// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveReflCustomAscii.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidDataHandling/AsciiPointBase.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveReflCustomAscii)
using namespace Kernel;
using namespace API;

/// virtual method to set the extra properties required for this algorithm
void SaveReflCustomAscii::extraProps() {
  declareProperty(std::make_unique<ArrayProperty<std::string>>("LogList"),
                  "List of logs to write to file.");
  declareProperty("Title", "", "Text to be written to the Title field");
  declareProperty(
      "WriteDeltaQ", false,
      "If true, the error on DeltaQ will be written as the fourth column.");
  declareProperty("Subtitle", false, "If true, subtitle added to header.");
  appendSeparatorProperty();
}

/** virtual method to add information to the file before the data
 *  @param file :: pointer to output file stream
 */
void SaveReflCustomAscii::extraHeaders(std::ofstream &file) {
  auto samp = m_ws->run();
  bool subtitle = getProperty("Subtitle");
  std::string subtitleEntry;
  std::string title = getProperty("Title");
  if (!title.empty()) // if is toggled
    file << "#" << title << '\n';
  if (subtitle) {
    try {
      subtitleEntry = samp.getLogData("run_title")->value();
    } catch (Kernel::Exception::NotFoundError &) {
      subtitleEntry = "";
    }
  }
  file << "#" << subtitleEntry << '\n';
  const std::vector<std::string> logList = getProperty("LogList");
  /// logs
  for (const auto &log : logList) {
    file << boost::lexical_cast<std::string>(log) << ": "
         << boost::lexical_cast<std::string>(samp.getLogData(log)->value())
         << '\n';
  }
}

void SaveReflCustomAscii::data(std::ofstream &file, bool exportDeltaQ) {
  exportDeltaQ = getProperty("WriteDeltaQ");
  AsciiPointBase::data(file, exportDeltaQ);
}

} // namespace DataHandling
} // namespace Mantid
