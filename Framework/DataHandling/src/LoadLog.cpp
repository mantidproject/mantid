// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadLog.h"
#include "LoadRaw/isisraw2.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Glob.h"
#include "MantidKernel/LogParser.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include "MantidTypes/Core/DateAndTimeHelpers.h"

#include <Poco/DateTimeFormat.h>
#include <Poco/DateTimeParser.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <boost/algorithm/string.hpp>
#include <fstream> // used to get ifstream
#include <regex>
#include <sstream>
#include <utility>

using Mantid::Types::Core::DateAndTime;

namespace Mantid::DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadLog)

using namespace Kernel;
using API::FileProperty;
using API::MatrixWorkspace;
using API::MatrixWorkspace_sptr;
using API::WorkspaceProperty;
using DataObjects::Workspace2D_sptr;
using Types::Core::DateAndTime;

namespace {

template <class MapClass, class LoggerType>
void addLogDataToRun(Mantid::API::Run &run, MapClass &aMap, LoggerType &logger) {
  for (auto &itr : aMap) {
    try {
      run.addLogData(itr.second.release());
    } catch (std::invalid_argument &e) {
      logger.warning() << e.what() << '\n';
    } catch (Exception::ExistsError &e) {
      logger.warning() << e.what() << '\n';
    }
  }
}

} // namespace

/// Empty default constructor
LoadLog::LoadLog() = default;

/// Initialisation method.
void LoadLog::init() {
  // When used as a Child Algorithm the workspace name is not used - hence the
  // "Anonymous" to satisfy the validator
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("Workspace", "Anonymous", Direction::InOut),
                  "The name of the workspace to which the log data will be added.");

  const std::vector<std::string> exts{".txt", ".log"};
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, exts),
                  "The filename (including its full or relative path) of a SNS "
                  "text log file (not cvinfo), "
                  "an ISIS log file, or an ISIS raw file. "
                  "If a raw file is specified all log files associated with "
                  "that raw file are loaded into the specified workspace. The "
                  "file extension must "
                  "either be .raw or .s when specifying a raw file");

  declareProperty(std::make_unique<ArrayProperty<std::string>>("Names"),
                  "For SNS-style log files only: the names of each column's log, separated "
                  "by commas. "
                  "This must be one fewer than the number of columns in the file.");

  declareProperty(std::make_unique<ArrayProperty<std::string>>("Units"),
                  "For SNS-style log files only: the units of each column's log, separated "
                  "by commas. "
                  "This must be one fewer than the number of columns in the file. "
                  "Optional: leave blank for no units in any log.");

  declareProperty("NumberOfColumns", Mantid::EMPTY_INT(),
                  "Number of columns in the file. If not set Mantid will "
                  "attempt to guess.");
}

/**
 * Executes the algorithm. Reading in ISIS log file(s)
 * @throw Mantid::Kernel::Exception::FileError  Thrown if file is not recognised
 * to be a raw datafile or log file
 * @throw std::runtime_error Thrown with Workspace problems
 */
void LoadLog::exec() {
  // Retrieve the filename from the properties and perform some initial checks
  // on the filename
  m_filename = getPropertyValue("Filename");
  // Get the log file names if provided.
  std::vector<std::string> names = getProperty("Names");
  // Open file, in order to pass it once to all functions that will load it.
  std::ifstream logFileStream(m_filename.c_str());

  // File property checks whether the given path exists, just check that is
  // actually a file
  Poco::File l_path(m_filename);
  if (l_path.isDirectory()) {
    throw Exception::FileError("Filename is a directory:", m_filename);
  }

  // Get the input workspace and retrieve run from workspace.
  // the log file(s) will be loaded into the run object of the workspace
  const MatrixWorkspace_sptr localWorkspace = getProperty("Workspace");

  if (isAscii(m_filename)) {
    // Is it a SNS style file? If so, we load it and abort.
    if (LoadSNSText()) {
      return;
    } // Otherwise we continue.
  }

  // If there's more than one log name provided, then it's an invalid ISIS file.
  if (names.size() > 1) {
    throw std::invalid_argument("More than one log name provided. Invalid ISIS log file.");
  }

  // If it's an old log file (pre-2007), then it is not currently supported.
  if (isOldDateTimeFormat(logFileStream)) {
    throw std::invalid_argument("File " + m_filename + " cannot be read because it has an old unsupported format.");
  }

  int colNum = static_cast<int>(getProperty("NumberOfColumns"));

  if (colNum == Mantid::EMPTY_INT()) {
    colNum = countNumberColumns(logFileStream, m_filename);
  }

  switch (colNum) {
  case 2:
    loadTwoColumnLogFile(logFileStream, extractLogName(names), localWorkspace->mutableRun());
    break;
  case 3:
    loadThreeColumnLogFile(logFileStream, m_filename, localWorkspace->mutableRun());
    break;
  default:
    throw std::invalid_argument("The log file provided is invalid as it has "
                                "less than 2 or more than three columns.");
    break;
  }
}

/**
 * Load an ISIS log file into the local workspace.
 * @param logFileStream :: The stream of the log file (data).
 * @param logFileName :: The name of the log file to load.
 * @param run :: The run information object
 */
void LoadLog::loadTwoColumnLogFile(std::ifstream &logFileStream, std::string logFileName, API::Run &run) {
  if (!logFileStream) {
    throw std::invalid_argument("Unable to open file " + m_filename);
  }

  // figure out if second column is a number or a string
  std::string aLine;
  if (Mantid::Kernel::Strings::extractToEOL(logFileStream, aLine)) {
    if (!isDateTimeString(aLine)) {
      throw std::invalid_argument("File " + m_filename +
                                  " is not a standard ISIS log file. Expected "
                                  "to be a two column file.");
    }

    std::string DateAndTime;
    std::stringstream ins(aLine);
    ins >> DateAndTime;

    // read in what follows the date-time string in the log file and figure out
    // what type it is
    std::string whatType;
    ins >> whatType;
    kind l_kind = classify(whatType);

    if (LoadLog::string != l_kind && LoadLog::number != l_kind) {
      throw std::invalid_argument("ISIS log file contains unrecognised second column entries: " + m_filename);
    }

    try {
      Property *log = LogParser::createLogProperty(m_filename, stringToLower(std::move(logFileName)));
      if (log) {
        run.addLogData(log);
      }
    } catch (std::exception &) {
    }
  }
}

/**
 * reads the .log stream and creates timeseries property and sets that to the
 * run object
 * @param logFileStream :: The stream of the log file (data).
 * @param logFileName :: The name of the log file to load.
 * @param run :: The run information object
 */
void LoadLog::loadThreeColumnLogFile(std::ifstream &logFileStream, const std::string &logFileName, API::Run &run) {
  std::string str;
  std::string propname;
  std::map<std::string, std::unique_ptr<Kernel::TimeSeriesProperty<double>>> dMap;
  std::map<std::string, std::unique_ptr<Kernel::TimeSeriesProperty<std::string>>> sMap;
  kind l_kind(LoadLog::empty);
  bool isNumeric(false);

  if (!logFileStream) {
    throw std::invalid_argument("Unable to open file " + m_filename);
  }

  while (Mantid::Kernel::Strings::extractToEOL(logFileStream, str)) {
    if (!isDateTimeString(str) && !str.empty()) {
      throw std::invalid_argument("File " + logFileName +
                                  " is not a standard ISIS log file. Expected "
                                  "to be a file starting with DateTime String "
                                  "format.");
    }

    if (!Kernel::TimeSeriesProperty<double>::isTimeString(str) || (str.empty() || str[0] == '#')) {
      // if the line doesn't start with a time read the next line
      continue;
    }

    std::stringstream line(str);
    std::string timecolumn;
    line >> timecolumn;

    std::string blockcolumn;
    line >> blockcolumn;
    l_kind = classify(blockcolumn);

    if (LoadLog::empty == l_kind) {
      g_log.warning() << "Failed to parse line in log file: " << timecolumn << "\t" << blockcolumn;
      continue;
    }

    if (LoadLog::string != l_kind) {
      throw std::invalid_argument("ISIS log file contains unrecognised second column entries: " + logFileName);
    }

    std::string valuecolumn;
    line >> valuecolumn;
    l_kind = classify(valuecolumn);

    if (LoadLog::string != l_kind && LoadLog::number != l_kind) {
      continue; // no value defined, just skip this entry
    }

    // column two in .log file is called block column
    propname = stringToLower(blockcolumn);
    // check if the data is numeric
    std::istringstream istr(valuecolumn);
    double dvalue;
    istr >> dvalue;
    isNumeric = !istr.fail();

    if (isNumeric) {
      auto ditr = dMap.find(propname);
      if (ditr != dMap.end()) {
        auto prop = ditr->second.get();
        if (prop)
          prop->addValue(timecolumn, dvalue);
      } else {
        auto logd = std::make_unique<Kernel::TimeSeriesProperty<double>>(propname);
        logd->addValue(timecolumn, dvalue);
        dMap.emplace(propname, std::move(logd));
      }
    } else {
      auto sitr = sMap.find(propname);
      if (sitr != sMap.end()) {
        auto prop = sitr->second.get();
        if (prop)
          prop->addValue(timecolumn, valuecolumn);
      } else {
        auto logs = std::make_unique<Kernel::TimeSeriesProperty<std::string>>(propname);
        logs->addValue(timecolumn, valuecolumn);
        sMap.emplace(propname, std::move(logs));
      }
    }
  }
  addLogDataToRun(run, dMap, g_log);
  addLogDataToRun(run, sMap, g_log);
}

/**
 * Check if log file property name has been set. If not set, return the
 * workspace + log file name (e.g. HRP37129_ICPevent). Otherwise return first
 * log name.
 * @param logName :: The vector containing log file names.
 * @return The name of the log file.
 */
std::string LoadLog::extractLogName(const std::vector<std::string> &logName) {
  if (logName.empty()) {
    return (Poco::Path(Poco::Path(m_filename).getFileName()).getBaseName());
  } else {
    return (logName.front());
  }
}

/**
 * Check if the file is SNS text; load it if it is, return false otherwise.
 * @return true if the file was a SNS style; false otherwise.
 */
bool LoadLog::LoadSNSText() {

  // Get the SNS-specific parameter
  std::vector<std::string> names = getProperty("Names");
  std::vector<std::string> units = getProperty("Units");

  // Get the input workspace and retrieve run from workspace.
  // the log file(s) will be loaded into the run object of the workspace
  const MatrixWorkspace_sptr localWorkspace = getProperty("Workspace");

  // open log file
  std::ifstream inLogFile(m_filename.c_str());

  // Get the first line
  std::string aLine;
  if (!Mantid::Kernel::Strings::extractToEOL(inLogFile, aLine))
    return false;

  std::vector<double> cols;
  bool ret = SNSTextFormatColumns(aLine, cols);
  // Any error?
  if (!ret || cols.size() < 2)
    return false;

  auto numCols = static_cast<size_t>(cols.size() - 1);
  if (names.size() != numCols)
    throw std::invalid_argument("The Names parameter should have one fewer "
                                "entry as the number of columns in a SNS-style "
                                "text log file.");
  if ((!units.empty()) && (units.size() != numCols))
    throw std::invalid_argument("The Units parameter should have either 0 "
                                "entries or one fewer entry as the number of "
                                "columns in a SNS-style text log file.");

  // Ok, create all the logs
  std::vector<TimeSeriesProperty<double> *> props;
  for (size_t i = 0; i < numCols; i++) {
    auto p = new TimeSeriesProperty<double>(names[i]);
    if (units.size() == numCols)
      p->setUnits(units[i]);
    props.emplace_back(p);
  }
  // Go back to start
  inLogFile.seekg(0);
  while (Mantid::Kernel::Strings::extractToEOL(inLogFile, aLine)) {
    if (aLine.empty())
      break;

    if (SNSTextFormatColumns(aLine, cols)) {
      if (cols.size() == numCols + 1) {
        DateAndTime time(cols[0], 0.0);
        for (size_t i = 0; i < numCols; i++)
          props[i]->addValue(time, cols[i + 1]);
      } else
        throw std::runtime_error("Inconsistent number of columns while reading "
                                 "SNS-style text file.");
    } else
      throw std::runtime_error("Error while reading columns in SNS-style text file.");
  }
  // Now add all the full logs to the workspace
  for (size_t i = 0; i < numCols; i++) {
    std::string propName = props[i]->name();
    if (localWorkspace->mutableRun().hasProperty(propName)) {
      localWorkspace->mutableRun().removeLogData(propName);
      g_log.information() << "Log data named " << propName << " already existed and was overwritten.\n";
    }
    localWorkspace->mutableRun().addLogData(props[i]);
  }

  return true;
}

/**
 * Takes as input a string and try to determine what type it is.
 * @param s :: The input string to be classified
 * @return A enum kind which tells what type the string is
 */
LoadLog::kind LoadLog::classify(const std::string &s) const {
  if (s.empty()) {
    return LoadLog::empty;
  }

  using std::string;
  const string lower("abcdefghijklmnopqrstuvwxyz");
  const string upper("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  const string letters = lower + upper + '_';

  if (letters.find_first_of(s) != string::npos) {
    return LoadLog::string;
  }

  const auto isNumber = [](const std::string &str) {
    // try and get stold to parse a number out of the string
    // if this throws then we don't have a number
    try {
      // cppcheck-suppress ignoredReturnValue
      std::stold(str);
      return true;
    } catch (const std::invalid_argument &) {
      return false;
    } catch (const std::out_of_range &) {
      return false;
    }
  };

  return (isNumber(s)) ? LoadLog::number : LoadLog::empty;
}

/**
 * Change each element of the string to lower case
 * @param strToConvert :: The input string
 * @returns The string but with all characters in lower case
 */
std::string LoadLog::stringToLower(std::string strToConvert) {
  std::transform(strToConvert.begin(), strToConvert.end(), strToConvert.begin(), tolower);
  return strToConvert;
}

/**
 * Checks whether filename is a simple text file
 * @param filename :: The filename to inspect
 * @returns true if the filename has the .txt extension
 */
bool LoadLog::isAscii(const std::string &filename) {
  FILE *file = fopen(filename.c_str(), "rb");
  char data[256];
  size_t n = fread(data, 1, sizeof(data), file);
  fclose(file);
  char *pend = &data[n];
  /*
   * Call it a binary file if we find a non-ascii character in the
   * first 256 bytes of the file.
   */
  for (char *p = data; p < pend; ++p) {
    auto ch = static_cast<unsigned long>(*p);
    if (!(ch <= 0x7F)) {
      return false;
    }
  }
  return true;
}

/**
 * Check if the string conforms to the ISO8601 standard.
 * @param str :: The string to test
 * @returns true if the strings format matched the expected date format
 */
bool LoadLog::isDateTimeString(const std::string &str) const {
  return Types::Core::DateAndTimeHelpers::stringIsISO8601(str.substr(0, 19));
}

/**
 * Check whether the string is consistent with the old log file
 * date-time format, for example:
 * Fri 31-JAN-2003 11:28:15
 * Wed  9-FEB-2005 09:47:01
 * @param logFileStream :: The file to test
 * @return true if the format matches the old log file format.
 */
bool LoadLog::isOldDateTimeFormat(std::ifstream &logFileStream) const {
  // extract first line of file
  std::string firstLine;
  Mantid::Kernel::Strings::extractToEOL(logFileStream, firstLine);
  // reset file back to the beginning
  logFileStream.seekg(0);

  std::regex oldDateFormat(R"([A-Z][a-z]{2} [ 1-3]\d-[A-Z]{3}-\d{4} \d{2}:\d{2}:\d{2})");

  return std::regex_match(firstLine.substr(0, 24), oldDateFormat);
}

/**
 * Read a line of a SNS-style text file.
 * @param input :: The string to test
 * @param out :: a vector that will be filled with the double values.
 * @return false if the format is NOT SNS style or a conversion failed.
 */
bool LoadLog::SNSTextFormatColumns(const std::string &input, std::vector<double> &out) const {
  std::vector<std::string> strs;
  out.clear();
  boost::split(strs, input, boost::is_any_of("\t "));
  double val;
  // Every column must evaluate to a double
  for (auto &str : strs) {
    if (!Strings::convert<double>(str, val))
      return false;
    else
      out.emplace_back(val);
  }
  // Nothing failed = it is that format.
  return true;
}

/**
 * Count the number of columns in the first line of the text file
 * @param logFileStream :: stream to the file
 * @param logFileName :: name for the log file
 */
int LoadLog::countNumberColumns(std::ifstream &logFileStream, const std::string &logFileName) {
  if (!logFileStream) {
    throw std::invalid_argument("Unable to open file " + m_filename);
  }

  std::string str;
  kind l_kind(LoadLog::empty);

  // extract first line of file
  Mantid::Kernel::Strings::extractToEOL(logFileStream, str);

  if (!isDateTimeString(str)) {
    throw std::invalid_argument("File " + logFileName +
                                " is not a standard ISIS log file. Expected to "
                                "be a file starting with DateTime String "
                                "format.");
  }

  std::stringstream line(str);
  std::string timecolumn;
  line >> timecolumn;

  std::string blockcolumn;
  line >> blockcolumn;
  l_kind = classify(blockcolumn);

  if (LoadLog::string != l_kind && LoadLog::number != l_kind) {
    throw std::invalid_argument("ISIS log file contains unrecognised second column entries: " + logFileName);
  }

  std::string valuecolumn;
  line >> valuecolumn;
  l_kind = classify(valuecolumn);

  // reset file back to the beginning
  logFileStream.seekg(0);

  if (LoadLog::string != l_kind && LoadLog::number != l_kind) {
    return 2; // looks like a two column file
  } else {
    return 3; // looks like a three column file
  }
}

} // namespace Mantid::DataHandling
