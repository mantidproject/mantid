// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveILLCosmosAscii.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidDataHandling/AsciiPointBase.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveILLCosmosAscii)
using namespace Kernel;
using namespace API;

/// virtual method to set the extra properties required for this algorithm
void SaveILLCosmosAscii::extraProps() {
  declareProperty(std::make_unique<ArrayProperty<std::string>>("LogList"),
                  "List of logs to write to file.");
  declareProperty("UserContact", "",
                  "Text to be written to the User-local contact field");
  declareProperty("Title", "", "Text to be written to the Title field");
  appendSeparatorProperty();
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
  if (tempInst)
    instrument = tempInst->getName();

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

  file << "MFT\n";
  file << "Instrument: " << instrument << '\n';
  file << "User-local contact: " << user << '\n'; // add optional property
  file << "Title: " << title << '\n';
  file << "Subtitle: " << subtitle << '\n';
  file << "Start date + time: " << startDT << '\n';
  file << "End date + time: " << endDT << '\n';

  const std::vector<std::string> logList = getProperty("LogList");
  /// logs
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
