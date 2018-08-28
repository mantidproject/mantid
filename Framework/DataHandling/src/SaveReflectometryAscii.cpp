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
  std::vector<std::string> extension = {".mft", ".txt", ".dat", ""};
  declareProperty("FileExtension", ".mft",
                  boost::make_shared<StringListValidator>(extension),
                  "Choose the file extension according to the file format");
  declareProperty(make_unique<ArrayProperty<std::string>>("LogList"),
                  "List of logs to write to file.");
  declareProperty("WriteHeader", false, "Whether to write header lines.");
  setPropertySettings("WriteHeader", Kernel::make_unique<VisibleWhenProperty>(
                                         "FileExtension", IS_EQUAL_TO, ""));
  std::vector<std::string> separator = {"comma", "space", "tab"};
  declareProperty("Separator", "tab",
                  boost::make_shared<StringListValidator>(separator),
                  "The separator used for splitting data columns");
  setPropertySettings("Separator", Kernel::make_unique<VisibleWhenProperty>(
                                       "FileExtension", IS_EQUAL_TO, ""));
}

/// Input validation for single MatrixWorkspace
std::map<std::string, std::string> SaveReflectometryAscii::validateInputs() {
  std::map<std::string, std::string> issues;
  try {
    m_ws = getProperty("InputWorkspace");
    if (!m_ws)
      issues["InputWorkspace"] = "InputWorkspace must be a MatrixWorkspace";
    try {
      m_ws->y(0).size();
    } catch (std::range_error &) {
      issues["InputWorkspace"] = "InputWorkspace does not contain data";
    }
    m_filename = getPropertyValue("Filename");
    if (m_filename.empty())
      issues["InputWorkspace"] = "Provide a file name";
    m_filename.append(getPropertyValue("FileExtension"));
    return issues;
  } catch (std::runtime_error &) {
    return issues;
  }
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
    if (m_ws->hasDx(0))
      outputval(m_ws->dx(0)[i]);
    m_file << '\n';
  }
}

/** Write formatted line of data
 *  @param val :: the double value to be written
 */
void SaveReflectometryAscii::outputval(double val) {
  m_file << std::setw(28);
  if (!std::isnan(val) && !std::isinf(val))
    m_file << val;
  else if (std::isinf(val))
    m_file << "inf";
  else
    m_file << "nan";
}

/** Write formatted line of data
 *  @param val :: a string value to be written
 */
void SaveReflectometryAscii::outputval(std::string val) {
  m_file << std::setw(28) << val;
}

/// Retrieve sample log information
std::string SaveReflectometryAscii::sampleInfo(const std::string &logName) {
  auto run = m_ws->run();
  try {
    return boost::lexical_cast<std::string>(run.getLogData(logName)->value());
  } catch (Exception::NotFoundError &) {
  }
  return "Not defined";
}

/// Write one header line
void SaveReflectometryAscii::writeInfo(const std::string logName,
                                       const std::string logValue) {
  if (!logValue.empty())
    m_file << logName << " : " << sampleInfo(logValue) << '\n';
  else
    m_file << logName << " : " << sampleInfo(logName) << '\n';
}

/// Write header lines
void SaveReflectometryAscii::header() {
  m_file << std::setfill(' ');
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
  const std::string ext = getProperty("FileExtension");
  if (getProperty("WriteHeader") || !ext.compare(".mft"))
    header();
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
        m_ws = boost::dynamic_pointer_cast<MatrixWorkspace>(i);
        if (!m_ws)
          throw(std::runtime_error(
              "WorkspaceGroup must contain MatrixWorkspaces"));
        try {
          m_ws->y(0).size();
        } catch (std::range_error &) {
          throw(std::runtime_error("InputWorkspace does not contain data"));
        }
        m_group.emplace_back(m_ws);
        m_wsName.emplace_back(i->getName()); // since we lost names
      }
    }
    if (group->isEmpty())
      g_log.warning("WorkspaceGroup does not contain MatrixWorkspaces");
    const std::string filename = getPropertyValue("Filename");
    if (filename.empty())
      throw(std::runtime_error("Provide a file name"));
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
      ending = getPropertyValue("FileExtension");
    m_filename = filename.substr(0, filename.find(".")) + m_wsName[i] + ending;
    this->exec();
  }
  return true;
}

} // namespace DataHandling
} // namespace Mantid
