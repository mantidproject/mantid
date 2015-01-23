#include <vector>
#include <sstream>
#include "MantidKernel/System.h"
#include "MantidNexus/MuonNexusReader.h"
#include <boost/scoped_array.hpp>
#include <nexus/NeXusException.hpp>

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
}

using namespace Mantid;

/// Default constructor
MuonNexusReader::MuonNexusReader()
    : nexus_instrument_name(), corrected_times(NULL), counts(NULL),
      detectorGroupings(NULL) {}

/// Destructor deletes temp storage
MuonNexusReader::~MuonNexusReader() {
  delete[] corrected_times;
  delete[] counts;
  delete[] detectorGroupings;
}
/**
 * Open the first NXentry of the supplied nexus file.
 *
 * @param handle Object to work on.
 */
void MuonNexusReader::openFirstNXentry(NeXus::File &handle) {
  std::map<string, string> entries = handle.getEntries();
  bool found = false;
  for (auto it = entries.begin(); it != entries.end(); ++it) {
    if (it->second == NXENTRY) {
      handle.openGroup(it->first, NXENTRY);
      found = true;
      break;
    }
  }
  if (!found)
    throw std::runtime_error("Failed to find NXentry");
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
  for (auto it = entries.begin(); it != entries.end(); ++it) {
    if (it->second == NXDATA) {
      nxdataname.push_back(it->first);
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
  counts = new int[t_ntc1 * t_nsp1];
  handle.getData(counts);
  handle.closeData();

  // Get groupings
  try {
    handle.openData("grouping");
    info = handle.getInfo();
    numDetectors = static_cast<int>(info.dims[0]);
    detectorGroupings = new int[numDetectors];
    handle.getData(detectorGroupings);
    handle.closeData();
  } catch (...) {
    g_log.debug("Muon nexus file does not contain grouping info");
  }

  // read corrected time
  handle.openData("corrected_time");
  info = handle.getInfo();
  corrected_times = new float[info.dims[0]];
  handle.getData(corrected_times);
  handle.closeData();

  // assume only one data set in file
  t_nper = 1;
  handle.closeGroup();

  // get instrument name
  handle.openGroup("instrument", "NXinstrument");
  handle.readData("name", nexus_instrument_name);
  handle.closeGroup();

  // Get number of switching states if available. Take this as number of periods
  // If not available set as one period.
  entries = handle.getEntries();
  t_nper = 1;
  for (auto it = entries.begin(); it != entries.end(); ++it) {
    if (it->first == "switching_states") {
      int ssPeriods;
      handle.readData("switching_states", ssPeriods);
      t_nper = abs(ssPeriods);
      t_nsp1 /= t_nper; // assume that number of spectra in multiperiod file
                        // should be divided by periods
      break;
    }
  }

  // file will close on leaving the function
}

// Get time boundary data as in ISISRAW. Simpler here as NeXus stores real times
// Not clear if corrected_time is what is wanted. Assume that values are bin
// centre
// times and that bin boundary values are wanted, as ISISRAW.
// @param  timebnds  float pointer for time values to be stored
// @param  ndnbs     int count of expected points
void MuonNexusReader::getTimeChannels(float *timebnds, const int &nbnds) const {
  // assume constant time bin width given by difference of first two values
  float binHalfWidth = (corrected_times[1] - corrected_times[0]) / float(2.0);
  for (int i = 0; i < nbnds - 1; i++)
    timebnds[i] = corrected_times[i] - binHalfWidth;
  timebnds[nbnds - 1] = timebnds[nbnds - 2] + float(2.0) * binHalfWidth;
}

string MuonNexusReader::getInstrumentName() const {
  return (nexus_instrument_name);
}

// NeXus Muon file reader for NXlog data.
// Read the given Nexus file into temp storage.
//
// Expected content of Nexus file is:
//     NXentry: "run" (or any name, ignored at present)
//            Zero or more NXlog entries which are of the form: <time>,<value>
//            <time> is 32bit float time wrt start_time and <value> either 32bit
//            float
//            or sting.
//
// @param filename ::  name of existing NeXus Muon file to read
void MuonNexusReader::readLogData(const string &filename) {
  // reset the count of logs
  nexusLogCount = 0;
  int nexusSampleCount = 0; // debug

  NeXus::File handle(filename, NXACC_READ);
  openFirstNXentry(handle);

  // read nexus fields at this level looking for NXlog and loading these into
  // memory
  // Also get the start_time string needed to change these times into ISO times
  std::map<string, string> entries = handle.getEntries();
  for (auto it = entries.begin(); it != entries.end(); ++it) {
    string nxname = it->first;
    string nxclass = it->second;

    if (nxclass == NXLOG) {
      handle.openGroup(nxname, nxclass);

      if (readMuonLogData(handle)) {
        nexusLogCount++;
      }

      handle.closeGroup();
    }
    if (nxclass == "NXSample" ||
        nxclass == "NXsample") // NXSample should be NXsample
    {
      handle.openGroup(nxname, nxclass);
      handle.readData("name", nexus_samplename);
      handle.closeGroup();
      nexusSampleCount++; // debug
    }
    if (nxname == START_TIME) {
      handle.readData(START_TIME, startTime);
      if ((startTime.find('T')) != string::npos)
        startTime.replace(startTime.find('T'), 1, " ");
      boost::posix_time::ptime pt =
          boost::posix_time::time_from_string(startTime);
      startTime_time_t = to_time_t(pt);
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
    g_log.warning() << "No " << VALUES << " set in " << handle.getPath()
                    << "\n";
    return false;
  }

  std::vector<float> values;
  std::vector<std::string> stringValues;
  bool isNumeric(false);

  NeXus::Info info = handle.getInfo();
  if (info.type == NX_FLOAT32 && info.dims.size() == 1) {
    isNumeric = true;
    boost::scoped_array<float> dataVals(new float[info.dims[0]]);
    handle.getData(dataVals.get());
    values.assign(dataVals.get(), dataVals.get() + info.dims[0]);
    stringValues.resize(info.dims[0]); // Leave empty
  } else if (info.type == NX_CHAR && info.dims.size() == 2) {
    boost::scoped_array<char> dataVals(
        new char[info.dims[0] * info.dims[1] + 1]);
    handle.getData(dataVals.get());
    dataVals[info.dims[0] * info.dims[1]] = 0;
    for (int i = 0; i < info.dims[0]; ++i) {
      std::string str(&dataVals[i * info.dims[1]],
                      &dataVals[(i + 1) * info.dims[1]]);
      stringValues.push_back(str);
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
    throw std::runtime_error(
        "Error in MuonNexusReader: expected float array for log times");
  }
  handle.closeData();

  // Add loaded values to vectors

  logNames.push_back(dataName);

  std::vector<float> tmp(timeVals.get(), timeVals.get() + info.dims[0]);
  logTimes.push_back(tmp);

  logType.push_back(isNumeric);
  logValues.push_back(values);
  logStringValues.push_back(stringValues);

  return true;
}

void MuonNexusReader::getLogValues(const int &logNumber, const int &logSequence,
                                   std::time_t &logTime, double &value) {
  // for the given log find the logTime and value at given sequence in log
  double time = logTimes[logNumber][logSequence];
  // boost::posix_time::ptime pt=boost::posix_time::time_from_string(startTime);
  // std::time_t atime=to_time_t(pt);
  // atime+=time;
  logTime = static_cast<std::time_t>(time) + startTime_time_t;
  // DateAndTime="2008-08-12T09:00:01"; //test
  value = logValues[logNumber][logSequence];
}

void MuonNexusReader::getLogStringValues(const int &logNumber,
                                         const int &logSequence,
                                         std::time_t &logTime, string &value) {
  // for the given log find the logTime and value at given sequence in log
  double time = logTimes[logNumber][logSequence];
  logTime = static_cast<std::time_t>(time) + startTime_time_t;
  std::vector<string> &strings = logStringValues[logNumber];
  if (logSequence < int(strings.size())) {
    value = strings[logSequence];
  } else {
    value = "";
  }
}

int MuonNexusReader::numberOfLogs() const { return (nexusLogCount); }

int MuonNexusReader::getLogLength(const int i) const {
  return (static_cast<int>(logTimes[i].size()));
}

bool MuonNexusReader::logTypeNumeric(const int i) const { return (logType[i]); }

/** return log name of i'th NXlog section
 * @param i :: the number of the NXlog section find name of.
 * @return the log name at the given index
 */
string MuonNexusReader::getLogName(const int i) const { return (logNames[i]); }
