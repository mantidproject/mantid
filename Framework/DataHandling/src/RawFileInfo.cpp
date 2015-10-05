//----------------------------
// Includes
//----------------------------
#include "MantidDataHandling/RawFileInfo.h"
#include "MantidAPI/FileProperty.h"
#include "LoadRaw/isisraw2.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include <cstdio>

// Register the algorithm into the AlgorithmFactory
namespace Mantid {
namespace DataHandling {

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataHandling;

DECLARE_ALGORITHM(RawFileInfo)

/**
 * Return the run title from the raw data structure
 * @param isisRaw A reference to the ISISRAW data structure
 * @return A string containing the title
 */
const std::string RawFileInfo::runTitle(const ISISRAW &isisRaw) {
  return std::string(isisRaw.r_title, 80);
}

/**
 * Return the run header from the raw data structure
 * @param isisRaw A reference to the ISISRAW data structure
 * @return A string containing the header
 */
const std::string RawFileInfo::runHeader(const ISISRAW &isisRaw) {
  // coverity doesn't like assuming that the whole hdr struct
  // has each array of characters laid out consecutively in memory
  // so we shouldn't just do:
  // header(isis_raw.hdr.hd_run, 69) to pull out everything

  // Separate each section with a character. 80 chars + 6 separators
  const auto &rawHdr = isisRaw.hdr;
  char header[86] = {};
  const size_t byte = sizeof(char);
  const char fieldSep(' ');
  char *start = header;

  memcpy(start, rawHdr.inst_abrv, 3 * byte);
  start += 3;
  memset(start, fieldSep, byte); // insert separator
  start += 1;

  memcpy(start, rawHdr.hd_run, 5 * byte);
  start += 5;
  memset(start, fieldSep, byte);
  start += 1;

  memcpy(start, rawHdr.hd_user, 20 * byte);
  start += 20;
  memset(start, fieldSep, byte);
  start += 1;

  memcpy(start, rawHdr.hd_title, 24 * byte);
  start += 24;
  memset(start, fieldSep, byte);
  start += 1;

  memcpy(start, rawHdr.hd_date, 12 * byte);
  start += 12;
  memset(start, fieldSep, byte);
  start += 1;

  memcpy(start, rawHdr.hd_time, 8 * byte);
  start += 8;
  memset(start, fieldSep, byte);
  start += 1;

  memcpy(start, rawHdr.hd_dur, 8 * byte);
  // final field so no space afterward

  return std::string(header, header + 86);
}

/// Create properties
void RawFileInfo::init() {
  std::vector<std::string> exts;
  exts.push_back(".raw");
  exts.push_back(".s*");
  declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
                  "The name of the [[RAW_File | RAW]] file from which to "
                  "extract the parameters");
  declareProperty("GetRunParameters", false,
                  "If this is true, the parameters from the RPB struct are "
                  "placed into a TableWorkspace called Raw_RPB",
                  Direction::Input);
  declareProperty("RunTitle", std::string(""),
                  "The run title from the HDR struct", Direction::Output);
  declareProperty("RunHeader", std::string(""), "The run header",
                  Direction::Output);
  declareProperty("SpectraCount", -1, "The number of spectra",
                  Direction::Output);
  declareProperty("TimeChannelCount", -1, "The number of time channels",
                  Direction::Output);
  declareProperty("PeriodCount", -1, "The number of periods",
                  Direction::Output);
}

void RawFileInfo::exec() {
  const std::string filename = getPropertyValue("Filename");

  ISISRAW2 isis_raw;
  // ReadFrom File with no data
  if (isis_raw.readFromFile(filename.c_str(), false) != 0) {
    g_log.error("Unable to open file " + filename);
    throw Exception::FileError("Unable to open File:", filename);
  }

  const std::string title = RawFileInfo::runTitle(isis_raw);
  const std::string header = RawFileInfo::runHeader(isis_raw);

  g_log.debug() << "Properties retrieved from " << filename << "\n"
                << "\tRun title: " << title << "\n\tRun header: " << header
                << "\n";
  setProperty("RunTitle", title);
  setProperty("RunHeader", header);

  // Spectra count
  int num_holder = isis_raw.t_nsp1;
  setProperty("SpectraCount", num_holder);
  g_log.debug() << "\tNumber of spectra:  " << num_holder << "\n";

  // Time channel count. Note here that the raw file will say N time channels
  // which transforms into
  // (N+1) bin boundaries in Mantid
  num_holder = isis_raw.t_ntc1;
  setProperty("TimeChannelCount", num_holder);
  g_log.debug() << "\tNumber of time channels:  " << num_holder << "\n";
  // The number of periods
  num_holder = isis_raw.t_nper;
  setProperty("PeriodCount", num_holder);
  g_log.debug() << "\tNumber of periods:  " << num_holder << "\n";

  // Get the run information if we are told to
  bool get_run_info = getProperty("GetRunParameters");
  if (get_run_info) {
    declareProperty(new WorkspaceProperty<API::ITableWorkspace>(
                        "RunParameterTable", "Raw_RPB", Direction::Output),
                    "The name of the TableWorkspace in which to store the list "
                    "of run parameters");

    API::ITableWorkspace_sptr run_table =
        WorkspaceFactory::Instance().createTable("TableWorkspace");
    run_table->addColumn("int", "r_dur");      // actual run duration
    run_table->addColumn("int", "r_durunits"); // scaler for above (1=seconds)
    run_table->addColumn("int",
                         "r_dur_freq");   // testinterval for above (seconds)
    run_table->addColumn("int", "r_dmp"); // dump interval
    run_table->addColumn("int", "r_dmp_units"); // scaler for above
    run_table->addColumn("int", "r_dmp_freq");  // interval for above
    run_table->addColumn("int",
                         "r_freq"); // 2**k where source frequency = 50 / 2**k
    run_table->addColumn("double",
                         "r_gd_prtn_chrg"); // good proton charge (uA.hour)
    run_table->addColumn("double",
                         "r_tot_prtn_chrg");  // total proton charge (uA.hour)
    run_table->addColumn("int", "r_goodfrm"); // good frames
    run_table->addColumn("int", "r_rawfrm");  // raw frames
    run_table->addColumn("int", "r_dur_wanted"); // requested run duration
                                                 // (units as for "duration"
                                                 // above)
    run_table->addColumn("int", "r_dur_secs"); // actual run duration in seconds
    run_table->addColumn("int", "r_mon_sum1"); // monitor sum 1
    run_table->addColumn("int", "r_mon_sum2"); // monitor sum 2
    run_table->addColumn("int", "r_mon_sum3"); // monitor sum 3
    run_table->addColumn("str", "r_enddate");  // format DD-MMM-YYYY
    run_table->addColumn("str", "r_endtime");  // format HH-MM-SS
    run_table->addColumn("int", "r_prop");     // RB (proposal) number

    API::TableRow t = run_table->appendRow();
    t << isis_raw.rpb.r_dur << isis_raw.rpb.r_durunits
      << isis_raw.rpb.r_dur_freq << isis_raw.rpb.r_dmp
      << isis_raw.rpb.r_dmp_units << isis_raw.rpb.r_dmp_freq
      << isis_raw.rpb.r_freq << static_cast<double>(isis_raw.rpb.r_gd_prtn_chrg)
      << static_cast<double>(isis_raw.rpb.r_tot_prtn_chrg)
      << isis_raw.rpb.r_goodfrm << isis_raw.rpb.r_rawfrm
      << isis_raw.rpb.r_dur_wanted << isis_raw.rpb.r_dur_secs
      << isis_raw.rpb.r_mon_sum1 << isis_raw.rpb.r_mon_sum2
      << isis_raw.rpb.r_mon_sum3 << std::string(isis_raw.rpb.r_enddate, 11)
      << std::string(isis_raw.rpb.r_endtime, 8) << isis_raw.rpb.r_prop;

    setProperty("RunParameterTable", run_table);
  }

  // This is not going to be a slow algorithm
  progress(1.0);
}

} // namespace
}
