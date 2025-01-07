// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../src/LoadRaw/isisraw2.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidNexusCpp/NeXusFile.hpp"

#include <climits>
#include <memory>

namespace Mantid {
namespace DataHandling {
/**
The SaveISISNexus algorithm will convert a RAW file to a NeXus file.

Required Properties:
<UL>
<LI> InputFilename - The name of and path to the input RAW file </LI>
<LI> OutputFilename - The name of and path to the input NeXus file </LI>
</UL>

@author Roman Tolchenov, Tessella plc
@date 03/03/2011
*/
class MANTID_DATAHANDLING_DLL SaveISISNexus final : public API::Algorithm {
public:
  /// Default constructor
  SaveISISNexus();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "SaveISISNexus"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "The SaveISISNexus algorithm will convert a RAW file to a NeXus "
           "file.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"SaveNexusProcessed", "SaveNexus", "LoadNexus"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Nexus"; }

private:
  /// Overwrites Algorithm method.
  void init() override;

  /// Overwrites Algorithm method
  void exec() override;

  std::unique_ptr<ISISRAW2> m_isisRaw;
  NXhandle handle;
  FILE *rawFile;
  std::vector<int> monitorData;
  /// <spectrum_index,monitor_index>. spectrum index is an index in any detector
  /// related array, not spectrum number
  std::map<int, int> monitor_index;
  int nper; ///< number of periods
  int nsp;  ///< number of spectra
  int ntc;  ///< number of time channels
  int nmon; ///< number of monitors
  int ndet; ///< number of detectors
  std::string start_time_str;
  std::vector<std::string> log_notes;

  NXlink counts_link;
  NXlink period_index_link;
  NXlink spectrum_index_link;
  NXlink time_of_flight_link;
  NXlink time_of_flight_raw_link;
  int *getMonitorData(int period, int imon);

  void saveInt(const char *name, const void *data, const int size = 1);
  void saveChar(const char *name, const void *data, const int size);
  void saveFloat(const char *name, const void *data, const int size);
  void saveIntOpen(const char *name, const void *data, const int size = 1);
  void saveCharOpen(const char *name, const void *data, const int size);
  void saveFloatOpen(const char *name, const void *data, const int size);
  int saveStringVectorOpen(const char *name, const std::vector<std::string> &str_vec, int max_str_size = -1);
  void saveString(const char *name, const std::string &str);
  void saveStringOpen(const char *name, const std::string &str);
  inline void close() { NXclosedata(handle); }       ///< close an open dataset.
  inline void closegroup() { NXclosegroup(handle); } ///< close an open group.
  void putAttr(const char *name, const std::string &value);
  void putAttr(const char *name, const char *value, const int size);
  void putAttr(const char *name, int value, int size = 1);
  void toISO8601(std::string &str);

  template <typename T> friend class getWithoutMonitors;

  /// Write vms_compat
  void write_isis_vms_compat();
  /// Write monitors
  void write_monitors();
  /// Write single monitor
  void monitor_i(int i);
  /// Write instrument
  void instrument();
  /// Write instrument/detector_1
  void detector_1();
  /// Write instrument/moderator
  void moderator();
  /// Write instrument/dae
  void dae();
  /// Write instrument/source
  void source();
  /// Create a link to some of detector_1's data
  void make_detector_1_link();
  /// Write user
  void user();
  /// Write sample
  void sample();
  /// Write runlog
  void runlog();
  /// write one run log
  void write_runlog(const char *name, const void *times, const void *data, const int type, const int size,
                    const std::string &units);
  /// write NXlog
  void write_logOpen(const char *name, const void *times, const void *data, const int type, const int size,
                     const std::string &units);
  /// Write selog
  void selog();
  /// Write notes from LOG_STRUCT
  void logNotes();
  /// Write run cycle
  void run_cycle();
  void write_rpb();
  void write_spb();
  void write_vpb();

  /// The name and path of the input file
  std::string inputFilename;
};

} // namespace DataHandling
} // namespace Mantid
