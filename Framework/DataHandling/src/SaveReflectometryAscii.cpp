// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveReflectometryAscii.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidKernel/make_unique.h"

#include <Poco/File.h>
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>
#include <cmath>
#include <iomanip>
#include <limits>
#include <map>
#include <stdexcept>

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace API;

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveReflectometryAscii)

/// Initialise the algorithm
void SaveReflectometryAscii::init() {
  declareProperty(
      make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "",
                                                      Direction::Input),
      "The name of the workspace containing the data you want to save.");
  declareProperty(make_unique<FileProperty>("Filename", "", FileProperty::Save),
                  "The output filename");
  std::vector<std::string> extension = {".mft", ".txt", ".dat", "custom"};
  declareProperty("FileExtension", ".mft",
                  boost::make_shared<StringListValidator>(extension),
                  "Choose the file extension according to the file format.");
  auto mft =
      make_unique<VisibleWhenProperty>("FileExtension", IS_EQUAL_TO, "mft");
  auto cus =
      make_unique<VisibleWhenProperty>("FileExtension", IS_EQUAL_TO, "custom");
  declareProperty(make_unique<ArrayProperty<std::string>>("LogList"),
                  "List of logs to write to file.");
  setPropertySettings("LogList", make_unique<VisibleWhenProperty>(
                                     std::move(mft), std::move(cus), OR));
  declareProperty("WriteHeader", false, "Whether to write header lines.");
  setPropertySettings(
      "WriteHeader",
      make_unique<VisibleWhenProperty>("FileExtension", IS_EQUAL_TO, "custom"));
  std::vector<std::string> separator = {"comma", "space", "tab"};
  declareProperty(
      "WriteResolution", true,
      "Whether to compute resolution values and write them as fourth "
      "data column.");
  setPropertySettings(
      "WriteResolution",
      make_unique<VisibleWhenProperty>("FileExtension", IS_EQUAL_TO, "custom"));
  declareProperty("Separator", "tab",
                  boost::make_shared<StringListValidator>(separator),
                  "The separator used for splitting data columns.");
  setPropertySettings("Separator", make_unique<VisibleWhenProperty>(
                                       "FileExtension", IS_EQUAL_TO, "custom"));
}

/// Input validation for single MatrixWorkspace
std::map<std::string, std::string> SaveReflectometryAscii::validateInputs() {
  std::map<std::string, std::string> issues;
  m_filename = getPropertyValue("Filename");
  if (m_filename.empty())
    issues["InputWorkspace"] = "Provide a file name";
  m_ext = getPropertyValue("FileExtension");
  if (m_ext != "custom")
    m_filename.append(m_ext);
  m_ws = getProperty("InputWorkspace");
  if (!m_ws) {
    WorkspaceGroup_const_sptr group =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
            getPropertyValue("InputWorkspace"));
    if (!group)
      issues["InputWorkspace"] = "Must be a MatrixWorkspace";
  } else {
    try {
      m_ws->y(0).size();
    } catch (std::range_error &) {
      issues["InputWorkspace"] = "Workspace does not contain data";
    }
  }
  return issues;
}

/// Write data to file
void SaveReflectometryAscii::data() {
  m_file << std::scientific;
  m_file << std::setprecision(std::numeric_limits<double>::digits10);
  const auto points = m_ws->points(0);
  const auto &yData = m_ws->y(0);
  const auto &eData = m_ws->e(0);
  for (size_t i = 0; i < m_ws->y(0).size(); ++i) {
    outputval(points[i]);
    outputval(yData[i]);
    outputval(eData[i]);
    if ((m_ext == "custom" && getProperty("WriteResolution")) ||
        (m_ext == ".mft") || (m_ext == ".txt")) {
      if (m_ws->hasDx(0))
        outputval(m_ws->dx(0)[i]);
      else {
        if (m_ext != ".mft")
          outputval(points[i] * ((points[1] + points[0]) / points[1]));
      }
    }
    m_file << '\n';
  }
}

/// Determine separator
void SaveReflectometryAscii::separator() {
  if (m_ext == "custom") {
    const std::string sepOption = getProperty("Separator");
    if (sepOption == "comma")
      m_sep = ',';
    if (sepOption == "space")
      m_sep = ' ';
  }
}

/** Write string value
 * @param write :: if true, write string
 * @param s :: string
 */
bool SaveReflectometryAscii::writeString(bool write, std::string s) {
  if (write) {
    if (m_ext == "custom" || m_ext == ".txt")
      m_file << m_sep << s;
    else {
      m_file << std::setw(28);
      m_file << s;
    }
  }
  return write;
}

/** Write formatted line of data
 *  @param val :: the double value to be written
 */
void SaveReflectometryAscii::outputval(double val) {
  bool inf = writeString(std::isinf(val), "inf");
  bool nan = writeString(std::isnan(val), "nan");
  if (!inf && !nan) {
    if (m_ext == "custom" || m_ext == ".txt")
      m_file << m_sep << val;
    else {
      m_file << std::setw(28);
      m_file << val;
    }
  }
}

/** Write formatted line of data
 *  @param val :: a string value to be written
 */
void SaveReflectometryAscii::outputval(std::string val) {
  m_file << std::setw(28) << val;
}

/// Retrieve sample log value
std::string SaveReflectometryAscii::sampleLogValue(const std::string &logName) {
  auto run = m_ws->run();
  try {
    return boost::lexical_cast<std::string>(run.getLogData(logName)->value());
  } catch (Exception::NotFoundError &) {
    return "Not defined";
  }
}

/// Retrieve sample log unit
std::string SaveReflectometryAscii::sampleLogUnit(const std::string &logName) {
  auto run = m_ws->run();
  try {
    return " " +
           boost::lexical_cast<std::string>(run.getLogData(logName)->units());
  } catch (Exception::NotFoundError &) {
    return "";
  }
}

/** Write one header line
 *  @param logName :: the name of a SampleLog entry to get its value from
 *  @param logNameFixed :: the name of the SampleLog entry defined by the header
 */
void SaveReflectometryAscii::writeInfo(const std::string logName,
                                       const std::string logNameFixed) {
  const std::string logValue = sampleLogValue(logName);
  const std::string logUnit = sampleLogUnit(logName);
  if (!logNameFixed.empty()) {
    // The logName corresponds to an existing header line of given name
    m_file << logNameFixed;
  } else {
    // The user provided a log name which is not part of the defined header
    m_file << logName;
  }
  m_file << " : " << logValue << logUnit << '\n';
}

/// Write header lines
void SaveReflectometryAscii::header() {
  m_file << std::setfill(' ');
  m_file << "MFT\n";
  std::vector<std::string> logs{"instrument.name", "user.namelocalcontact",
                                "title", "start_time", "end_time"};
  writeInfo("instrument.name", "Instrument");
  writeInfo("user.namelocalcontact", "User-local contact");
  writeInfo("title", "Title");
  writeInfo("", "Subtitle");
  writeInfo("start_time", "Start date + time");
  writeInfo("end_time", "End date + time");
  writeInfo("", "Theta 1 + dir + ref numbers");
  writeInfo("", "Theta 2 + dir + ref numbers");
  writeInfo("", "Theta 3 + dir + ref numbers");
  const std::vector<std::string> logList = getProperty("LogList");
  int nLogs = 0;
  for (const auto &log : logList) {
    if (find(logs.cbegin(), logs.cend(), log) ==
        logs.end()) { // do not repeat a log
      writeInfo(log);
      ++nLogs;
    }
  }
  // Write "Parameter : Not defined" 9 times minus the number of new user logs
  for (auto i = nLogs; i < 9; ++i)
    writeInfo("", "Parameter ");
  m_file << "Number of file format : "
         << "40\n";
  m_file << "Number of data points : " << m_ws->y(0).size() << '\n';
  m_file << '\n';
  outputval("q");
  outputval("refl");
  outputval("refl_err");
  if (m_ws->hasDx(0))
    outputval("q_res (FWHM)");
  m_file << "\n";
}

/// Check file
void SaveReflectometryAscii::checkFile(const std::string filename) {
  if (Poco::File(filename).exists()) {
    g_log.warning("File already exists and will be overwritten");
    try {
      Poco::File(filename).remove();
    } catch (...) { // maybe we do not have the permission to delete the file
      g_log.error("Error deleting file " + filename);
    }
  }
  m_file.open(filename);
  if (!m_file.is_open()) {
    g_log.error("Unable to create file: " + filename);
  }
  g_log.information("Filename: " + filename);
}

/// Execute the algorithm
void SaveReflectometryAscii::exec() {
  checkFile(m_filename);
  separator();
  if ((getProperty("WriteHeader") && m_ext == "custom") || m_ext == ".mft")
    header();
  else if (m_ext == ".dat")
    m_file << m_ws->y(0).size() << "\n";
  data();
  m_file.close();
}

/// Check if input workspace is a group
bool SaveReflectometryAscii::checkGroups() {
  try {
    WorkspaceGroup_const_sptr group =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
            getPropertyValue("InputWorkspace"));
    if (!group)
      return false;
    for (auto i : group->getAllItems()) {
      if (i->getName().empty())
        g_log.warning("InputWorkspace must have a name, skip");
      else {
        const auto ws = boost::dynamic_pointer_cast<MatrixWorkspace>(i);
        if (!ws)
          g_log.warning("WorkspaceGroup must contain MatrixWorkspaces, skip");
        else {
          try {
            ws->y(0).size();
          } catch (std::range_error &) {
            throw(std::runtime_error("InputWorkspace does not contain data"));
          }
          m_group.emplace_back(ws);
          m_wsName.emplace_back(i->getName()); // since we lost names
        }
      }
    }
    if (group->isEmpty())
      g_log.warning("WorkspaceGroup does not contain MatrixWorkspaces");
    const std::string filename = getPropertyValue("Filename");
    if (filename.empty())
      throw(std::runtime_error("Provide a file name"));
    m_ext = getPropertyValue("FileExtension");
    return true;
  } catch (...) {
    return false;
  }
}

/// Execution of group workspaces
bool SaveReflectometryAscii::processGroups() {
  const std::string filename = getPropertyValue("Filename");
  for (auto i = 0u; i < m_group.size(); ++i) {
    m_ws = m_group[i]->clone();
    std::string ending{""};
    try {
      ending = filename.substr(filename.find("."));
    } catch (...) {
    }
    if (ending.empty())
      ending = m_ext;
    m_filename = filename.substr(0, filename.find(".")) + m_wsName[i] + ending;
    this->exec();
  }
  return true;
}

} // namespace DataHandling
} // namespace Mantid
