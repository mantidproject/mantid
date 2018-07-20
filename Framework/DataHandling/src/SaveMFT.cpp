#include "MantidDataHandling/SaveMFT.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ListValidator.h"

#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>
#include <cmath>
#include <iomanip>
#include <limits>
#include <map>
#include <stdexcept>
#include <vector>
#include <Poco/File.h>

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace API;

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveMFT)

/// Initialise the algorithm
void SaveMFT::init() {
  declareProperty(
      make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "",
                                                      Direction::Input),
      "The name of the workspace containing the data you want to save.");
  declareProperty(
      make_unique<FileProperty>("Filename", "", FileProperty::Save, ".mft"),
      "The output filename.");
  declareProperty(make_unique<ArrayProperty<std::string>>("LogList"),
                  "List of logs to write to file.");
  std::vector<std::string> header;
  header.push_back("Write header lines");
  header.push_back("Do not write header lines");
  declareProperty("Header", "Write header lines",
                  boost::make_shared<StringListValidator>(header),
                  "Wether to write header lines.");
}

/// Write data to file
void SaveMFT::data() {
  m_file << std::scientific;
  m_file << std::setprecision(std::numeric_limits<double>::digits10);
  const auto points = m_ws->points(0);
  const auto &yData = m_ws->y(0);
  const auto &eData = m_ws->e(0);
  for (size_t i = 0; i < m_length; ++i) {
    outputval(points[i]);
    outputval(yData[i]);
    outputval(eData[i]);
    if (m_ws->hasDx(0))
      outputval(m_ws->dx(0)[i]);
    m_file << '\n';
  }
}

/** Write formatted line of data
 *  @param val :: the double value to be written
 */
void SaveMFT::outputval(double val) {
  bool nancheck = std::isnan(val);
  bool infcheck = std::isinf(val);
  m_file << '\t';
  if (!nancheck && !infcheck)
    m_file << val;
  else if (infcheck)
    m_file << "inf";
  else
    m_file << "nan";
}

/// Retrieve sample log information
std::string SaveMFT::sampleInfo(const std::string &logName) {
  auto run = m_ws->run();
  try {
    return boost::lexical_cast<std::string>(run.getLogData(logName)->value());
  } catch (Exception::NotFoundError) {
  }
  return "Not defined";
}

/// Write one header line
void SaveMFT::writeInfo(const std::string logName, const std::string logValue) {
  if (!logValue.empty())
    m_file << logName << " : " << sampleInfo(logValue) << '\n';
  else
    m_file << logName << " : " << sampleInfo(logName) << '\n';
}

/// Write header lines
void SaveMFT::header() {
  m_file << "MFT\n";
  std::map<std::string, std::string> logs;
  logs["Instrument"] = "instrument.name";
  logs["User-local contact"] = "user.namelocalcontact";
  logs["Title"] = "title";
  logs["Subtitle"] = "";
  logs["Start date + time"] = "start_time";
  logs["End date + time"] = "end_time";
  logs["Theta 1 + dir + ref numbers"] = "";
  logs["Theta 2 + dir + ref numbers"] = "";
  logs["Theta 3 + dir + ref numbers"] = "";
  writeInfo("Instrument", "instrument.name");
  writeInfo("User-local contact", "user.namelocalcontact");
  writeInfo("Title", "title");
  writeInfo("Subtitle", "");
  writeInfo("Start date + time", "start_time");
  writeInfo("End date + time", "end_time");
  writeInfo("Theta 1 + dir + ref numbers", "");
  writeInfo("Theta 2 + dir + ref numbers", "");
  writeInfo("Theta 3 + dir + ref numbers", "");
  const std::vector<std::string> logList = getProperty("LogList");
  int nlogs = 0;
  for (const auto &log : logList) {
    if (logs.find(log) == logs.end()) {
      writeInfo(log);
      ++nlogs;
    }
  }
  for (auto i = nlogs + 1; i < 10; ++i)
    writeInfo("Parameter ");
  m_file << "Number of file format : " << "40\n";
  m_file << "Number of data points : " << m_length << '\n';
  m_file << '\n';
  m_file << std::setfill(' ') << std::setw(29) << "q" << std::setfill(' ')
         << std::setw(24) << "refl" << std::setfill(' ') << std::setw(24)
         << "refl_err";
  if (m_ws->hasDx(0))
    m_file << std::setw(25) << "q_res (FWHM)\n";
  else
    m_file << "\n";
}

/// Execute the algorithm
void SaveMFT::exec() {
  const std::string filename = getProperty("Filename");
  g_log.information("Filename: " + filename);
  if (Poco::File(filename).exists()) {
    g_log.warning("File already exists and will be overwritten.");
    try {
      Poco::File(filename).remove();
    } catch (...) { // maybe we do not have the permission to delete the file
      g_log.error("Error deleting file " + filename);
      throw Exception::FileError("Unable to delete existing m_file: ",
                                 filename);
    }
  }
  m_file.open(filename);
  if (!m_file.is_open()) {
    g_log.error("Unable to create file: " + filename);
    throw Exception::FileError("Unable to create file: ", filename);
  }
  m_ws = getProperty("InputWorkspace");
  if (!m_ws)
    throw std::runtime_error("Cannot treat InputWorkspace");
  try {
    m_length = m_ws->y(0).size();
  } catch (std::range_error) {
    g_log.warning("InputWorkspace does not contain data");
  }
  if (getPointerToProperty("Header")->isDefault())
    header();
  data();
}

} // namespace DataHandling
} // namespace Mantid
