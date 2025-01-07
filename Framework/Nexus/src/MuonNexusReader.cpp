// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidNexus/MuonNexusReader.h"
#include "MantidKernel/Logger.h"
#include "MantidNexusCpp/NeXusException.hpp"
#include <boost/scoped_array.hpp>
#include <sstream>
#include <vector>

using std::string;

namespace { // anonymous namespace to keep things in the file
///< Type for NXdata.
const string NXDATA("NXdata");
///< Type for NXentry.
const string NXENTRY("NXentry");
///< Type for NXlog.
const string NXLOG("NXlog");
///< Special string for start time
const string START_TIME("start_time");

/// logger
Mantid::Kernel::Logger g_log("MuonNexusReader");

///< Strings for period information
const std::string PERIOD_SEQUENCES("period_sequences");
const std::string PERIOD_TYPE("period_type");
const std::string FRAMES_PERIOD_REQUESTED("frames_period_requested");
const std::string FRAMES_PERIOD_RAW("frames_period_raw");
const std::string PERIOD_OUTPUT("period_output");
const std::string TOTAL_COUNTS_PERIOD("total_counts_period");
const std::string PERIOD_LABELS("period_labels");

/// Function to convert vectors into a string separated by a delimeter delim
template <class T> std::string convertVectorToString(T values, const std::string &delim) {
  std::string result("");
  if (!values.empty()) {
    for (const auto &value : values)
      result += std::to_string(value) + delim;
    result.erase(result.length() - 1); // Remove final delim
  }
  return result;
}
} // namespace

using namespace Mantid;

/**
 * Open the first NXentry of the supplied nexus file.
 *
 * @param handle Object to work on.
 */
void MuonNexusReader::openFirstNXentry(NeXus::File &handle) {
  std::map<string, string> entries = handle.getEntries();
  const auto entry =
      std::find_if(entries.cbegin(), entries.cend(), [](const auto entry) { return entry.second == NXENTRY; });
  if (entry == entries.cend())
    throw std::runtime_error("Failed to find NXentry");
  handle.openGroup(entry->first, NXENTRY);
}

// Basic NeXus Muon file reader - simple version based on contents of test
// files.
// Read the given Nexus file into temp storage. Following the approach of
// ISISRAW
// which does not use namespace.
// This reader is only used by LoadMuonNexus - the NexusProcessed files are
// dealt with by
// NexusFileIO.cpp
//
// Expected content of Nexus file is:
//     Entry: "run" (first entry opened, whatever name is)
//       Group: "histogram_data_1" (first NXdata section read, whatever name is)
//         Data: "counts"  (2D integer array)
//         Data: "corrected time" (1D float array)
//
// @param filename ::  name of existing NeXus Muon file to read
void MuonNexusReader::readFromFile(const string &filename) {
  NeXus::File handle(filename, NXACC_READ);
  openFirstNXentry(handle);

  // find all of the NXdata in the entry
  std::vector<string> nxdataname;
  std::map<string, string> entries = handle.getEntries();
  for (auto &entry : entries) {
    if (entry.second == NXDATA) {
      nxdataname.emplace_back(entry.first);
    }
  }
  handle.openGroup(nxdataname.front(), NXDATA);

  // reused local variable
  NeXus::Info info;

  // open NXdata section
  handle.openData("counts");
  info = handle.getInfo();
  t_ntc1 = static_cast<int>(info.dims[1]);
  t_nsp1 = static_cast<int>(info.dims[0]);
  handle.getData(m_counts);
  handle.closeData();

  // Get groupings
  try {
    handle.openData("grouping");
    info = handle.getInfo();
    m_numDetectors = static_cast<int>(info.dims[0]);
    handle.getData(m_detectorGroupings);
    handle.closeData();
  } catch (...) {
    g_log.debug("Muon nexus file does not contain grouping info");
  }

  // read corrected time
  handle.openData("corrected_time");
  info = handle.getInfo();
  handle.getData(m_correctedTimes);
  handle.closeData();

  // assume only one data set in file
  t_nper = 1;
  handle.closeGroup();

  // get instrument name
  handle.openGroup("instrument", "NXinstrument");
  handle.readData("name", m_nexusInstrumentName);

  // Try to read in period information
  try {
    handle.openGroup("beam", "NXbeam");
    readPeriodInfo(handle);
    handle.closeGroup();
  } catch (...) {
    g_log.debug("Muon nexus file does not contain beam info");
  }

  handle.closeGroup(); // Close instrument group

  // Get number of switching states if available. Take this as number of periods
  // If not available set as one period.
  entries = handle.getEntries();
  t_nper = 1;
  if (std::any_of(entries.cbegin(), entries.cend(),
                  [](const auto &entry) { return entry.first == "switching_states"; })) {
    int ssPeriods;
    handle.readData("switching_states", ssPeriods);
    t_nper = abs(ssPeriods);
    // assume that number of spectra in multiperiod file should be divided by
    // periods
    t_nsp1 /= t_nper;
  }
  // file will close on leaving the function
}

/**
 * Try to read in vairous peices of period information.
 * Period info includes:
 * - Period sequences
 * - Period Names
 * - Period type
 * - Frames periods requested
 * - Raw frames
 * - Tag
 * - Total counts per period
 */
void MuonNexusReader::readPeriodInfo(NeXus::File &handle) {
  auto parsePeriod = [&handle](const std::string &logName, auto &destAttr) {
    try {
      std::remove_reference_t<decltype(destAttr)> tempAttr;
      handle.readData(logName, tempAttr);
      destAttr = tempAttr;
    } catch (...) {
      g_log.debug("Muon nexus file does not contain " + logName);
    }
  };

  auto parseIntVector = [&handle](const std::string &logName, auto &destAttr) {
    try {
      std::vector<int> tempIntVector;
      handle.readData(logName, tempIntVector);
      destAttr = convertVectorToString(tempIntVector, ";");
    } catch (...) {
      g_log.debug("Muon nexus file does not contain " + logName);
    }
  };

  auto parseFloatVector = [&handle](const std::string &logName, auto &destAttr) {
    try {
      std::vector<float> tempFloatVector;
      handle.readData(logName, tempFloatVector);
      destAttr = convertVectorToString(tempFloatVector, ";");
    } catch (...) {
      g_log.debug("Muon nexus file does not contain " + logName);
    }
  };

  parsePeriod(PERIOD_LABELS, m_periodNames);
  parsePeriod(PERIOD_SEQUENCES, m_numPeriodSequences);
  parseFloatVector(TOTAL_COUNTS_PERIOD, m_periodsCounts);
  parseIntVector(PERIOD_TYPE, m_periodTypes);
  parseIntVector(FRAMES_PERIOD_REQUESTED, m_framesPeriodsRequested);
  parseIntVector(FRAMES_PERIOD_RAW, m_framesPeriodsRaw);
  parseIntVector(PERIOD_OUTPUT, m_periodsOutput);
}

// Get time boundary data as in ISISRAW. Simpler here as NeXus stores real times
// Not clear if corrected_time is what is wanted. Assume that values are bin
// centre
// times and that bin boundary values are wanted, as ISISRAW.
// @param  timebnds  float pointer for time values to be stored
// @param  ndnbs     int count of expected points
void MuonNexusReader::getTimeChannels(float *timebnds, const int &nbnds) const {
  // assume constant time bin width given by difference of first two values
  float binHalfWidth = (m_correctedTimes[1] - m_correctedTimes[0]) / float(2.0);
  for (int i = 0; i < nbnds - 1; i++)
    timebnds[i] = m_correctedTimes[i] - binHalfWidth;
  timebnds[nbnds - 1] = timebnds[nbnds - 2] + float(2.0) * binHalfWidth;
}

std::string const &MuonNexusReader::getInstrumentName() const { return m_nexusInstrumentName; }

// NeXus Muon file reader for NXlog data.
// Read the given Nexus file into temp storage.
//
// Expected content of Nexus file is:
//     NXentry: "run" (or any name, ignored at present)
//            Zero or more NXlog entries which are of the form: <time>,<value>
//            <time> is 32bit float time wrt start_time and <value> either 32bit
//            float
//            or string.
//
// @param filename ::  name of existing NeXus Muon file to read
void MuonNexusReader::readLogData(const string &filename) {
  // reset the count of logs
  m_nexusLogCount = 0;

  NeXus::File handle(filename, NXACC_READ);
  openFirstNXentry(handle);

  // read nexus fields at this level looking for NXlog and loading these into
  // memory
  // Also get the start_time string needed to change these times into ISO times
  std::map<string, string> entries = handle.getEntries();
  for (const auto &entrie : entries) {
    string nxname = entrie.first;
    string nxclass = entrie.second;

    if (nxclass == NXLOG) {
      handle.openGroup(nxname, nxclass);

      if (readMuonLogData(handle)) {
        m_nexusLogCount++;
      }

      handle.closeGroup();
    }
    if (nxclass == "NXSample" || nxclass == "NXsample") // NXSample should be NXsample
    {
      handle.openGroup(nxname, nxclass);
      handle.readData("name", m_nexusSampleName);
      handle.closeGroup();
    }
    if (nxname == START_TIME) {
      handle.readData(START_TIME, m_startTime);
      if ((m_startTime.find('T')) != string::npos)
        m_startTime.replace(m_startTime.find('T'), 1, " ");
      boost::posix_time::ptime pt = boost::posix_time::time_from_string(m_startTime);
      m_startTime_time_t = to_time_t(pt);
    }
  }

  // file will close on leaving the function
}

bool MuonNexusReader::readMuonLogData(NeXus::File &handle) {
  const string NAME("name");
  const string VALUES("values");
  const string TIME("time");

  // read name of Log data
  string dataName;
  handle.readData(NAME, dataName);

  // read data values
  try {
    handle.openData(VALUES);
  } catch (NeXus::Exception &) {
    g_log.warning() << "No " << VALUES << " set in " << handle.getPath() << "\n";
    return false;
  }

  std::vector<float> values;
  std::vector<std::string> stringValues;
  bool isNumeric(false);
  std::string units = "";

  NeXus::Info info = handle.getInfo();
  if (info.type == NX_FLOAT32 && info.dims.size() == 1) {
    isNumeric = true;
    boost::scoped_array<float> dataVals(new float[info.dims[0]]);
    handle.getAttr("units", units);
    handle.getData(dataVals.get());
    values.assign(dataVals.get(), dataVals.get() + info.dims[0]);
    stringValues.resize(info.dims[0]); // Leave empty
  } else if (info.type == NX_CHAR && info.dims.size() == 2) {
    boost::scoped_array<char> dataVals(new char[info.dims[0] * info.dims[1] + 1]);
    handle.getAttr("units", units);
    handle.getData(dataVals.get());
    dataVals[info.dims[0] * info.dims[1]] = 0;
    for (int i = 0; i < info.dims[0]; ++i) {
      std::string str(&dataVals[i * info.dims[1]], &dataVals[(i + 1) * info.dims[1]]);
      stringValues.emplace_back(str);
    }
    values.resize(info.dims[0]); // Leave empty
  } else {
    // Leave both empty
    values.resize(info.dims[0]);
    stringValues.resize(info.dims[0]);
  }
  handle.closeData();

  // read time values
  try {
    handle.openData(TIME);
  } catch (NeXus::Exception &) {
    g_log.warning() << "No " << TIME << " set in " << handle.getPath() << "\n";
    return false;
  }

  info = handle.getInfo();
  boost::scoped_array<float> timeVals(new float[info.dims[0]]);
  if (info.type == NX_FLOAT32 && info.dims.size() == 1) {
    handle.getData(timeVals.get());
  } else {
    throw std::runtime_error("Error in MuonNexusReader: expected float array for log times");
  }
  handle.closeData();

  // Add loaded values to vectors

  m_logNames.emplace_back(dataName);

  std::vector<float> tmp(timeVals.get(), timeVals.get() + info.dims[0]);
  m_logTimes.emplace_back(tmp);
  m_logUnits.emplace_back(units);
  m_logType.emplace_back(isNumeric);
  m_logValues.emplace_back(values);
  m_logStringValues.emplace_back(stringValues);

  return true;
}

void MuonNexusReader::getLogValues(const int &logNumber, const int &logSequence, std::time_t &logTime, double &value) {
  // for the given log find the logTime and value at given sequence in log
  double time = m_logTimes[logNumber][logSequence];
  // boost::posix_time::ptime pt=boost::posix_time::time_from_string(startTime);
  // std::time_t atime=to_time_t(pt);
  // atime+=time;
  logTime = static_cast<std::time_t>(time) + m_startTime_time_t;
  // DateAndTime="2008-08-12T09:00:01"; //test
  value = m_logValues[logNumber][logSequence];
}

void MuonNexusReader::getLogStringValues(const int &logNumber, const int &logSequence, std::time_t &logTime,
                                         string &value) {
  // for the given log find the logTime and value at given sequence in log
  const double time = m_logTimes[logNumber][logSequence];
  logTime = static_cast<std::time_t>(time) + m_startTime_time_t;
  const std::vector<string> &strings = m_logStringValues[logNumber];
  if (logSequence < int(strings.size())) {
    value = strings[logSequence];
  } else {
    value = "";
  }
}

int MuonNexusReader::numberOfLogs() const { return (m_nexusLogCount); }

int MuonNexusReader::getLogLength(const int i) const { return (static_cast<int>(m_logTimes[i].size())); }

std::string MuonNexusReader::logUnits(const int i) const { return (m_logUnits[i]); }

bool MuonNexusReader::logTypeNumeric(const int i) const { return (m_logType[i]); }

/** return log name of i'th NXlog section
 * @param i :: the number of the NXlog section find name of.
 * @return the log name at the given index
 */
string MuonNexusReader::getLogName(const int i) const { return (m_logNames[i]); }
