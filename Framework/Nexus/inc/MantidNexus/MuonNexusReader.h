// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <climits>
#include <nexus/NeXusFile.hpp>

// class MuonNexusReader - based on ISISRAW this class implements a simple
// reader for Nexus Muon data files.
class DLLExport MuonNexusReader {
  /** @class MuonNexusReader MuonNexusReader.h

  MuunNexusReader opens a Nexus file and reads certain fields expected for a
  ISIS Muon
      data file (old format). These values are stored for access via
  LoadMuonNexus.

  Required Properties:
  <UL>
  <LI> Filename - The name of and path to the input Nexus file </LI>
  </UL>

  @author Ronald Fowler, based on ISISRAW.
  @date 14/08/2008
  */
private:
  std::string nexus_instrument_name; ///< name read from nexus file
  std::string nexus_samplename;      ///< sample name read from Nexus
  int nexusLogCount;                 ///< number of NXlog sections read from file
  std::vector<bool> logType;         ///< true if i'th log is numeric
  std::vector<std::string> logNames; ///< stores name read from file
  void openFirstNXentry(NeXus::File &handle);
  bool readMuonLogData(NeXus::File &handle);             ///< method to read the fields of open NXlog section
  std::vector<std::vector<float>> logValues,             ///< array of values for i'th NXlog section
      logTimes;                                          ///< arrys of times for i'th NXlog section
  std::vector<std::vector<std::string>> logStringValues; ///< array of string values for i'th NXlog section
  std::string startTime;                                 ///< string startTime which must be read from Nexus
  /// file to base all NXlog times on
  std::time_t startTime_time_t;                            ///< startTime in time_t format
  std::time_t to_time_t(const boost::posix_time::ptime &t) ///< convert posix time to time_t
  {
    /**
    Take the input Posix time, subtract the unix epoch, and return the seconds
    as a std::time_t value.
    @param t :: time of interest as ptime
    @return :: time_t value of t
    */
    if (t == boost::posix_time::neg_infin)
      return 0;
    else if (t == boost::posix_time::pos_infin)
      return LONG_MAX;
    boost::posix_time::ptime start(boost::gregorian::date(1970, 1, 1));
    return (t - start).total_seconds();
  }

public:
  /// Default constructor
  MuonNexusReader();
  /// Destructor
  ~MuonNexusReader();

  void readFromFile(const std::string &filename); ///< read histogram data
  void readLogData(const std::string &filename);  ///< read log data
  void getTimeChannels(float *timebnds,
                       const int &nbnds) const; ///< get time bin boundaries
                                                /// return sample name
  std::string getSampleName() const { return nexus_samplename; };
  int numberOfLogs() const;                  ///< Number of NXlog sections read from file
  int getLogLength(const int i) const;       ///< Lenght of i'th log
  std::string getLogName(const int i) const; ///< Name of i'th log
  void getLogValues(const int &logNumber, const int &logSequence, std::time_t &logTime,
                    double &value); ///< get logSequence pair of logNumber log
  void getLogStringValues(const int &logNumber, const int &logSequence, std::time_t &logTime,
                          std::string &value); ///< get logSequence pair of logNumber string log
  bool logTypeNumeric(const int i) const;      ///< true if i'th log is of numeric type
  // following ISISRAW.h
  int t_nsp1; ///< number of spectra in time regime 1
  int t_ntc1; ///< number of time channels in time regime 1
  int t_nper; ///< number of periods in file (=1 at present)
  // for nexus histogram data
  float *corrected_times;                ///< temp store for corrected times
  int *counts;                           ///< temp store of histogram data
  int *detectorGroupings;                ///< detector grouping info
  int numDetectors;                      ///< detector count
  std::string getInstrumentName() const; ///< return instrument name
};
