// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/InstrumentFileFinder.h"
#include "MantidAPI/NexusFileLoader.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/BankPulseTimes.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataHandling/EventWorkspaceCollection.h"
#include "MantidDataHandling/LoadGeometry.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Events.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/NexusHDF5Descriptor.h"
#include "MantidKernel/OptionalBool.h"

#include "MantidKernel/TimeSeriesProperty.h"

#include "MantidNexus/NeXusException.hpp"
#include "MantidNexus/NeXusFile.hpp"

#include <Poco/Path.h>
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <boost/scoped_array.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <numeric>
#include <random>
#include <string>

namespace Mantid {
namespace DataHandling {

/** @class InvalidLogPeriods
 * Custom exception extending std::invalid_argument
 * Thrown when nperiods does not match period_log
 * Custom exception so we can re-propagate this error and
 * handle all other errors.
 */
class InvalidLogPeriods : public std::invalid_argument {
public:
  InvalidLogPeriods(const std::string &msg) : std::invalid_argument(msg) {}
};

bool exists(::NeXus::File &file, const std::string &name);

bool exists(const std::map<std::string, std::string> &entries, const std::string &name);

/** @class LoadEventNexus LoadEventNexus.h Nexus/LoadEventNexus.h

  Load Event Nexus files.

  Required Properties:
  <UL>
  <LI> Filename - The name of and path to the input NeXus file </LI>
  <LI> Workspace - The name of the workspace to output</LI>
  </UL>

  @date Sep 27, 2010
  */
class MANTID_DATAHANDLING_DLL LoadEventNexus : public API::NexusFileLoader {

public:
  LoadEventNexus();

  const std::string name() const override { return "LoadEventNexus"; };

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads an Event NeXus file and stores as an "
           "EventWorkspace. Optionally, you can filter out events falling "
           "outside a range of times-of-flight and/or a time interval.";
  }

  /// Version
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"LoadISISNexus", "LoadEventAndCompress"}; }

  /// Category
  const std::string category() const override { return "DataHandling\\Nexus"; }

  int confidence(Kernel::NexusHDF5Descriptor &descriptor) const override;

  template <typename T>
  static std::shared_ptr<BankPulseTimes>
  runLoadNexusLogs(const std::string &nexusfilename, T localWorkspace, Algorithm &alg, bool returnpulsetimes,
                   int &nPeriods, std::unique_ptr<const Kernel::TimeSeriesProperty<int>> &periodLog);

  template <typename T>
  static std::shared_ptr<BankPulseTimes>
  runLoadNexusLogs(const std::string &nexusfilename, T localWorkspace, Algorithm &alg, bool returnpulsetimes,
                   int &nPeriods, std::unique_ptr<const Kernel::TimeSeriesProperty<int>> &periodLog,
                   const std::vector<std::string> &allow_list, const std::vector<std::string> &block_list);

  static int checkForCorruptedPeriods(std::unique_ptr<Kernel::TimeSeriesProperty<int>> tempPeriodLog,
                                      std::unique_ptr<const Kernel::TimeSeriesProperty<int>> &periodLog,
                                      const int &nPeriods, const std::string &nexusfilename, std::string &status);

  template <typename T>
  static void loadEntryMetadata(const std::string &nexusfilename, T WS, const std::string &entry_name,
                                const Kernel::NexusHDF5Descriptor &descriptor);

  /// Load instrument from Nexus file if possible, else from IDF spacified by Nexus file
  template <typename T>
  static bool loadInstrument(const std::string &nexusfilename, T localWorkspace, const std::string &top_entry_name,
                             Algorithm *alg, const Kernel::NexusHDF5Descriptor *descriptor = nullptr);

  /// Load instrument for Nexus file
  template <typename T>
  static bool runLoadIDFFromNexus(const std::string &nexusfilename, T localWorkspace, const std::string &top_entry_name,
                                  Algorithm *alg);

  /// Load instrument from IDF file specified by Nexus file
  template <typename T>
  static bool runLoadInstrument(const std::string &nexusfilename, T localWorkspace, const std::string &top_entry_name,
                                Algorithm *alg, const Kernel::NexusHDF5Descriptor *descriptor = nullptr);

  static void loadSampleDataISIScompatibility(::NeXus::File &file, EventWorkspaceCollection &WS);

  /// method used to return instrument name for some old ISIS files where it is
  /// not written properly within the instrument
  static std::string readInstrumentFromISIS_VMSCompat(::NeXus::File &hFile);

public:
  /// The name and path of the input file
  std::string m_filename;

  /// The workspace being filled out
  std::shared_ptr<EventWorkspaceCollection> m_ws;

  /// Filter by a minimum time-of-flight
  double filter_tof_min;
  /// Filter by a maximum time-of-flight
  double filter_tof_max;
  /// Tof range is being filtered
  bool filter_tof_range;

  /// Minimum spectrum to load
  int32_t m_specMin;
  /// Maximum spectrum to load
  int32_t m_specMax;

  /// Filter by start time
  Mantid::Types::Core::DateAndTime filter_time_start;
  /// Filter by stop time
  Mantid::Types::Core::DateAndTime filter_time_stop;
  /// if wall-clock filtering was requested
  bool m_is_time_filtered{false};

  bool filter_bad_pulses{false};
  std::shared_ptr<Mantid::Kernel::TimeROI> bad_pulses_timeroi;

  /// Mutex protecting tof limits
  std::mutex m_tofMutex;

  /// Limits found to tof
  double longest_tof;
  /// Limits found to tof
  double shortest_tof;
  /// Count of all the "bad" tofs found. These are events with TOF > 2e8
  /// microsec
  size_t bad_tofs;
  /// A count of events discarded because they came from a pixel that's not in
  /// the IDF
  size_t discarded_events;

  /// Tolerance for CompressEvents; use -1 to mean don't compress.
  double compressTolerance;
  bool compressEvents;

  /// Pulse times for ALL banks, taken from proton_charge log.
  std::shared_ptr<BankPulseTimes> m_allBanksPulseTimes;

  /// name of top level NXentry to use
  std::string m_top_entry_name;
  std::unique_ptr<::NeXus::File> m_file;

private:
  /// Possible loaders types
  enum class LoaderType;

  /// Intialisation code
  void init() override;

  /// Execution code
  void execLoader() override;

  std::map<std::string, std::string> validateInputs() override;

  LoadEventNexus::LoaderType defineLoaderType(const bool haveWeights, const bool oldNeXusFileNames,
                                              const std::string &classType) const;

  DataObjects::EventWorkspace_sptr createEmptyEventWorkspace();

  void loadEvents(API::Progress *const prog, const bool monitors);
  void createSpectraMapping(const std::string &nxsfile, const bool monitorsOnly,
                            const std::vector<std::string> &bankNames = std::vector<std::string>());
  void deleteBanks(const EventWorkspaceCollection_sptr &workspace, const std::vector<std::string> &bankNames);
  void runLoadMonitors();
  /// Set the filters on TOF.
  void setTimeFilters(const bool monitors);
  template <typename T>
  T filterEventsByTime(T workspace, Mantid::Types::Core::DateAndTime &startTime,
                       Mantid::Types::Core::DateAndTime &stopTime);

  /// Load a spectra mapping from the given file
  std::unique_ptr<std::pair<std::vector<int32_t>, std::vector<int32_t>>>
  loadISISVMSSpectraMapping(const std::string &entry_name);

  template <typename T> void filterDuringPause(T workspace);

  /// Set the top entry field name
  void setTopEntryName();

  /// to open the nexus file with specific exception handling/message
  void safeOpenFile(const std::string &fname);

  /// Was the instrument loaded?
  bool m_instrument_loaded_correctly;

  /// Do we load the sample logs?
  bool loadlogs;
  /// True if the event_id is spectrum no not pixel ID
  bool event_id_is_spec;
};

//-----------------------------------------------------------------------------
//               ISIS event corrections
//-----------------------------------------------------------------------------

/**
 * Load the time of flight data. file must have open the group containing
 * "time_of_flight" data set. This will add a offset to all of the
 * time-of-flight values or a random number to each time-of-flight. It
 * should only ever be called on event files that have a "detector_1_events"
 * group inside the "NXentry". It is an old ISIS requirement that is rarely
 * used now.
 *
 * Due to hardware issues with retro-fitting event mode to old electronics,
 * ISIS event mode is really a very fine histogram with between 1 and 2
 * microseconds bins.
 *
 * If we just took "middle of bin" as the true event time here then WISH
 * observed strange ripples when they added spectra. The solution was to
 * randomise the probability of an event within the bin.
 *
 * This randomisation is now performed in the control program which also writes
 * the "event_time_offset_shift" dataset (with a single value of "random") when
 * it has been performed. If this dataset is present in an event file then no
 * randomisation is performed in LoadEventNexus.
 *
 * This code should remain for loading older ISIS event datasets.
 *
 * @param file :: The nexus file to read from.
 * @param localWorkspace :: The event workspace collection to write to.
 * @param binsName :: bins name
 * @param start_wi :: First workspace index to process
 * @param end_wi :: Last workspace index to process
 */
template <typename T>
void makeTimeOfFlightDataFuzzy(::NeXus::File &file, T localWorkspace, const std::string &binsName, size_t start_wi = 0,
                               size_t end_wi = 0) {
  const std::string EVENT_TIME_SHIFT_TAG("event_time_offset_shift");
  // first check if the data is already randomized
  const auto entries = file.getEntries();
  if (entries.find(EVENT_TIME_SHIFT_TAG) != entries.end()) {
    std::string event_shift_type;
    file.readData(EVENT_TIME_SHIFT_TAG, event_shift_type);
    if (event_shift_type == "random") {
      return;
    }
  }

  // if the data is not randomized randomize it uniformly within each bin
  file.openData(binsName);
  // time of flights of events
  std::vector<float> tofsFile;
  file.getData(tofsFile);
  file.closeData();

  // todo: try to find if tof can be reduced to just 3 numbers: start, end and
  // dt
  if (end_wi <= start_wi) {
    end_wi = localWorkspace->getNumberHistograms();
  }

  // random number generator
  std::mt19937 rng;

  // loop over spectra
  for (size_t wi = start_wi; wi < end_wi; ++wi) {
    DataObjects::EventList &event_list = dynamic_cast<DataObjects::EventList &>(localWorkspace->getSpectrum(wi));
    if (event_list.empty())
      continue;
    // sort the events
    event_list.sortTof();
    auto tofsEventList = event_list.getTofs();

    size_t n = tofsFile.size();
    // iterate over the events and time bins
    auto ev = tofsEventList.begin();
    auto ev_end = tofsEventList.end();
    for (size_t i = 1; i < n; ++i) {
      double right = double(tofsFile[i]);
      // find the right boundary for the current event
      if ((ev != ev_end) && (right < *ev)) {
        continue;
      }
      // count events which have the same right boundary
      size_t m = 0;
      while ((ev != ev_end) && (*ev < right)) {
        ++ev;
        ++m; // count events in the i-th bin
      }

      if (m > 0) { // m events in this bin
        double left = double(tofsFile[i - 1]);
        // spread the events uniformly inside the bin
        std::uniform_real_distribution<double> flat(left, right);
        std::vector<double> random_numbers(m);
        std::generate(random_numbers.begin(), random_numbers.end(), [&flat, &rng]() { return flat(rng); });
        std::sort(random_numbers.begin(), random_numbers.end());
        auto it = random_numbers.begin();
        for (auto ev1 = ev - m; ev1 != ev; ++ev1, ++it) {
          *ev1 = *it;
        }
      }

    } // for i
    event_list.setTofs(tofsEventList);

    event_list.sortTof();
  } // for wi
}

/**
 * ISIS specific method for dealing with wide events. Check if time_of_flight
 * can be found in the file and load it.
 *
 * THIS ONLY APPLIES TO ISIS FILES WITH "detector_1_events" IN THE "NXentry."
 *
 * @param file :: The nexus file to read from.
 * @param localWorkspace :: The event workspace collection which events will be
 *modified.
 * @param entry_name :: An NXentry tag in the file
 * @param classType :: The type of the events: either detector or monitor
 * @param descriptor :: input descriptor carrying metadata information
 */
template <typename T>
void adjustTimeOfFlightISISLegacy(::NeXus::File &file, T localWorkspace, const std::string &entry_name,
                                  const std::string &classType,
                                  const Kernel::NexusHDF5Descriptor *descriptor = nullptr) {
  bool done = false;
  // Go to the root, and then top entry
  file.openPath("/");
  file.openGroup(entry_name, "NXentry");

  // NexusHDF5Descriptor
  if (descriptor != nullptr) {
    // not an ISIS file
    if (!descriptor->isEntry("/" + entry_name + "/detector_1_events")) {
      return;
    }
  }

  using string_map_t = std::map<std::string, std::string>;
  string_map_t entries = file.getEntries();

  if (entries.find("detector_1_events") == entries.end()) { // not an ISIS file
    return;
  }

  // try if monitors have their own bins
  if (classType == "NXmonitor") {
    std::vector<std::string> bankNames;
    for (string_map_t::const_iterator it = entries.begin(); it != entries.end(); ++it) {
      const std::string entry_class(it->second);
      if (entry_class == classType) {
        const std::string entryName(it->first);
        bankNames.emplace_back(entryName);
      }
    }
    for (size_t i = 0; i < bankNames.size(); ++i) {
      const std::string &mon = bankNames[i];
      file.openGroup(mon, classType);
      entries = file.getEntries();
      if (entries.find("event_time_bins") == entries.end()) {
        // bins = entries.find("time_of_flight"); // I think time_of_flight
        // doesn't work here
        // if (bins == entries.end())
        //{
        done = false;
        file.closeGroup();
        break; // done == false => use bins from the detectors
               //}
      }
      done = true;
      makeTimeOfFlightDataFuzzy(file, localWorkspace, "event_time_bins", i, i + 1);
      file.closeGroup();
    }
  }

  if (!done) {
    // first check detector_1_events
    file.openGroup("detector_1_events", "NXevent_data");
    entries = file.getEntries();
    for (string_map_t::const_iterator it = entries.begin(); it != entries.end(); ++it) {
      if (it->first == "time_of_flight" || it->first == "event_time_bins") {
        makeTimeOfFlightDataFuzzy(file, localWorkspace, it->first);
        done = true;
      }
    }
    file.closeGroup(); // detector_1_events

    if (!done) { // if time_of_flight was not found try
                 // instrument/dae/time_channels_#
      file.openGroup("instrument", "NXinstrument");
      file.openGroup("dae", "IXdae");
      entries = file.getEntries();
      size_t time_channels_number = 0;
      for (string_map_t::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        // check if there are groups with names "time_channels_#" and select the
        // one with the highest number
        if (it->first.size() > 14 && it->first.substr(0, 14) == "time_channels_") {
          size_t n = boost::lexical_cast<size_t>(it->first.substr(14));
          if (n > time_channels_number) {
            time_channels_number = n;
          }
        }
      }
      if (time_channels_number > 0) // the numbers start with 1
      {
        file.openGroup("time_channels_" + std::to_string(time_channels_number), "IXtime_channels");
        entries = file.getEntries();
        for (string_map_t::const_iterator it = entries.begin(); it != entries.end(); ++it) {
          if (it->first == "time_of_flight" || it->first == "event_time_bins") {
            makeTimeOfFlightDataFuzzy(file, localWorkspace, it->first);
          }
        }
        file.closeGroup();
      }
      file.closeGroup(); // dae
      file.closeGroup(); // instrument
    }
  }

  // close top entry (or entry given in entry_name)
  file.closeGroup();
}

//-----------------------------------------------------------------------------
/** Load the instrument definition file specified by info in the NXS file.
 *
 *  @param nexusfilename :: Used to pick the instrument.
 *  @param localWorkspace :: Templated workspace in which to put the instrument
 *geometry
 *  @param top_entry_name :: entry name at the top of the NXS file
 *  @param alg :: Handle of the algorithm
 *  @param descriptor :: The Nexus HDF5 file descriptor
 *  @return true if successful
 */
template <typename T>
bool LoadEventNexus::runLoadInstrument(const std::string &nexusfilename, T localWorkspace,
                                       const std::string &top_entry_name, Algorithm *alg,
                                       const Kernel::NexusHDF5Descriptor *descriptor) {
  std::string instrument;
  std::string instFilename;

  const bool isNexus = (descriptor == nullptr) ? LoadGeometry::isNexus(nexusfilename)
                                               : LoadGeometry::isNexus(descriptor->getAllEntries());

  // Check if the geometry can be loaded directly from the Nexus file
  if (isNexus) {
    instFilename = nexusfilename;
  } else {
    // Get the instrument name
    ::NeXus::File nxfile(nexusfilename);
    // Start with the base entry
    nxfile.openGroup(top_entry_name, "NXentry");
    // Open the instrument
    nxfile.openGroup("instrument", "NXinstrument");
    try {
      nxfile.openData("name");
      instrument = nxfile.getStrData();
      alg->getLogger().debug() << "Instrument name read from NeXus file is " << instrument << '\n';
    } catch (::NeXus::Exception &) {
      // Try to fall back to isis compatibility options
      nxfile.closeGroup();
      instrument = readInstrumentFromISIS_VMSCompat(nxfile);
      if (instrument.empty()) {
        // Get the instrument name from the file instead
        size_t n = nexusfilename.rfind('/');
        if (n != std::string::npos) {
          std::string temp = nexusfilename.substr(n + 1, nexusfilename.size() - n - 1);
          n = temp.find('_');
          if (n != std::string::npos && n > 0) {
            instrument = temp.substr(0, n);
          }
        }
      }
    }
    if (instrument == "POWGEN3") // hack for powgen b/c of bad long name
      instrument = "POWGEN";
    if (instrument == "NOM") // hack for nomad
      instrument = "NOMAD";

    if (instrument.empty())
      throw std::runtime_error("Could not find the instrument name in the NXS "
                               "file or using the filename. Cannot load "
                               "instrument!");

    // Now let's close the file as we don't need it anymore to load the
    // instrument.
    nxfile.close();
  }

  if (instFilename.empty()) {
    try {
      instFilename =
          API::InstrumentFileFinder::getInstrumentFilename(instrument, localWorkspace->getWorkspaceStartDate());
    } catch (Kernel::Exception::NotFoundError &) {
      if (instFilename.empty()) {
        Poco::Path directory(Kernel::ConfigService::Instance().getInstrumentDirectory());
        Poco::Path file(instrument + "_Definition.xml");
        Poco::Path fullPath(directory, file);
        instFilename = fullPath.toString();
      }
    }
  }

  // do the actual work
  auto loadInst = alg->createChildAlgorithm("LoadInstrument");

  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  bool executionSuccessful(true);
  try {
    loadInst->setPropertyValue("Filename", instFilename);
    loadInst->setPropertyValue("InstrumentName", instrument);
    loadInst->setProperty<Mantid::API::MatrixWorkspace_sptr>("Workspace", localWorkspace);
    loadInst->setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(false));
    loadInst->execute();

    // Populate the instrument parameters in this workspace - this works around
    // a bug
    localWorkspace->populateInstrumentParameters();
  } catch (std::invalid_argument &e) {
    alg->getLogger().information() << "Invalid argument to LoadInstrument Child Algorithm : " << e.what() << '\n';
    executionSuccessful = false;
  } catch (std::runtime_error &e) {
    alg->getLogger().information("Unable to successfully run LoadInstrument Child Algorithm");
    alg->getLogger().information(e.what());
    executionSuccessful = false;
  }

  // If loading instrument definition file fails
  if (!executionSuccessful) {
    alg->getLogger().error() << "Error loading Instrument definition file\n";
    return false;
  }

  // Ticket #2049: Cleanup all loadinstrument members to a single instance
  // If requested update the instrument to positions in the data file
  const auto &pmap = localWorkspace->constInstrumentParameters();
  if (!pmap.contains(localWorkspace->getInstrument()->getComponentID(), "det-pos-source"))
    return executionSuccessful;

  std::shared_ptr<Geometry::Parameter> updateDets =
      pmap.get(localWorkspace->getInstrument()->getComponentID(), "det-pos-source");
  std::string value = updateDets->value<std::string>();
  if (value.substr(0, 8) == "datafile") {
    auto updateInst = alg->createChildAlgorithm("UpdateInstrumentFromFile");
    updateInst->setProperty<Mantid::API::MatrixWorkspace_sptr>("Workspace", localWorkspace);
    updateInst->setPropertyValue("Filename", nexusfilename);
    if (value == "datafile-ignore-phi") {
      updateInst->setProperty("IgnorePhi", true);
      alg->getLogger().information("Detector positions in IDF updated with "
                                   "positions in the data file except for the "
                                   "phi values");
    } else {
      alg->getLogger().information("Detector positions in IDF updated with positions in the data file");
    }
    // We want this to throw if it fails to warn the user that the information
    // is not correct.
    updateInst->execute();
  }

  return executionSuccessful;
}

//-----------------------------------------------------------------------------
/** Load the run number and other meta data from the given bank */
template <typename T>
void LoadEventNexus::loadEntryMetadata(const std::string &nexusfilename, T WS, const std::string &entry_name,
                                       const Kernel::NexusHDF5Descriptor &descriptor) {
  // Open the file
  ::NeXus::File file(nexusfilename);
  file.openGroup(entry_name, "NXentry");

  // get the title
  if (descriptor.isEntry("/" + entry_name + "/title", "SDS")) {
    file.openData("title");
    if (file.getInfo().type == NXnumtype::CHAR) {
      std::string title = file.getStrData();
      if (!title.empty())
        WS->setTitle(title);
    }
    file.closeData();
  }

  // get the notes
  if (descriptor.isEntry("/" + entry_name + "/notes", "SDS")) {
    file.openData("notes");
    if (file.getInfo().type == NXnumtype::CHAR) {
      std::string notes = file.getStrData();
      if (!notes.empty())
        WS->mutableRun().addProperty("file_notes", notes, true);
    }
    file.closeData();
  }

  // Get the run number
  if (descriptor.isEntry("/" + entry_name + "/run_number", "SDS")) {
    file.openData("run_number");
    std::string run;
    if (file.getInfo().type == NXnumtype::CHAR) {
      run = file.getStrData();
    } else if (file.isDataInt()) {
      // inside ISIS the run_number type is int32
      std::vector<int> value;
      file.getData(value);
      if (!value.empty())
        run = std::to_string(value[0]);
    }
    if (!run.empty()) {
      WS->mutableRun().addProperty("run_number", run, true);
    }
    file.closeData();
  }

  // get the experiment identifier
  if (descriptor.isEntry("/" + entry_name + "/experiment_identifier", "SDS")) {
    file.openData("experiment_identifier");
    std::string expId;
    if (file.getInfo().type == NXnumtype::CHAR) {
      expId = file.getStrData();
    }
    if (!expId.empty()) {
      WS->mutableRun().addProperty("experiment_identifier", expId, true);
    }
    file.closeData();
  }

  // get the sample name - nested try/catch to leave the handle in an
  // appropriate state
  if (descriptor.isEntry("/" + entry_name + "/sample", "NXsample")) {
    file.openGroup("sample", "NXsample");
    try {
      if (descriptor.isEntry("/" + entry_name + "/sample/name", "SDS")) {
        file.openData("name");
        const auto info = file.getInfo();
        std::string sampleName;
        if (info.type == NXnumtype::CHAR) {
          if (info.dims.size() == 1) {
            sampleName = file.getStrData();
          } else { // something special for 2-d array
            const int64_t total_length = std::accumulate(info.dims.begin(), info.dims.end(), static_cast<int64_t>(1),
                                                         std::multiplies<int64_t>());
            boost::scoped_array<char> val_array(new char[total_length]);
            file.getData(val_array.get());
            sampleName = std::string(val_array.get(), total_length);
          }
        }
        file.closeData();
        if (!sampleName.empty()) {
          WS->mutableSample().setName(sampleName);
        }
      }
    } catch (::NeXus::Exception &) {
      // let it drop on floor if an exception occurs while reading sample
    }
    file.closeGroup();
  }

  // get the duration
  if (descriptor.isEntry("/" + entry_name + "/duration", "SDS")) {
    file.openData("duration");
    std::vector<double> duration;
    file.getDataCoerce(duration);
    if (duration.size() == 1) {
      // get the units
      std::vector<::NeXus::AttrInfo> infos = file.getAttrInfos();
      std::string units;
      for (auto it = infos.begin(); it != infos.end(); ++it) {
        // cppcheck-suppress useStlAlgorithm
        if (it->name == "units") {
          units = file.getStrAttr(*it);
          break;
        }
      }

      // set the property
      WS->mutableRun().addProperty("duration", duration[0], units, true);
    }
    file.closeData();
  }

  // close the file
  file.close();
}

//-----------------------------------------------------------------------------
/** Load the instrument from the nexus file if property LoadNexusInstrumentXML
 *  is set to true. If instrument XML not found from the IDF file
 *  (specified by the info in the Nexus file) load the IDF.
 *
 *  @param nexusfilename :: The Nexus file name
 *  @param localWorkspace :: templated workspace in which to put the
 *instrument geometry
 *  @param top_entry_name :: entry name at the top of the Nexus file
 *  @param alg :: Handle of the algorithm
 *  @param descriptor :: The Nexus HDF5 file descriptor
 *  @return true if successful
 */
template <typename T>
bool LoadEventNexus::loadInstrument(const std::string &nexusfilename, T localWorkspace,
                                    const std::string &top_entry_name, Algorithm *alg,
                                    const Kernel::NexusHDF5Descriptor *descriptor) {

  bool loadNexusInstrumentXML = true;
  if (alg->existsProperty("LoadNexusInstrumentXML"))
    loadNexusInstrumentXML = alg->getProperty("LoadNexusInstrumentXML");

  bool foundInstrument = false;
  if (loadNexusInstrumentXML)
    foundInstrument = runLoadIDFFromNexus<T>(nexusfilename, localWorkspace, top_entry_name, alg);
  if (!foundInstrument)
    foundInstrument = runLoadInstrument<T>(nexusfilename, localWorkspace, top_entry_name, alg, descriptor);
  return foundInstrument;
}

//-----------------------------------------------------------------------------
/** Load the instrument from the nexus file
 *
 *  @param nexusfilename :: The name of the nexus file being loaded
 *  @param localWorkspace :: templated workspace in which to put the
 *instrument geometry
 *  @param top_entry_name :: entry name at the top of the Nexus file
 *  @param alg :: Handle of the algorithm
 *  @return true if successful
 */
template <typename T>
bool LoadEventNexus::runLoadIDFFromNexus(const std::string &nexusfilename, T localWorkspace,
                                         const std::string &top_entry_name, Algorithm *alg) {
  // Test if IDF exists in file, move on quickly if not
  try {
    ::NeXus::File nxsfile(nexusfilename);
    nxsfile.openPath(top_entry_name + "/instrument/instrument_xml");
  } catch (::NeXus::Exception &) {
    alg->getLogger().information("No instrument XML definition found in " + nexusfilename + " at " + top_entry_name +
                                 "/instrument");
    return false;
  }

  auto loadInst = alg->createChildAlgorithm("LoadIDFFromNexus");

  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  try {
    loadInst->setPropertyValue("Filename", nexusfilename);
    loadInst->setProperty<Mantid::API::MatrixWorkspace_sptr>("Workspace", localWorkspace);
    loadInst->setPropertyValue("InstrumentParentPath", top_entry_name);
    loadInst->execute();
  } catch (std::invalid_argument &) {
    alg->getLogger().error("Invalid argument to LoadIDFFromNexus Child Algorithm ");
  } catch (std::runtime_error &) {
    alg->getLogger().debug("No instrument definition found by LoadIDFFromNexus in " + nexusfilename + " at " +
                           top_entry_name + "/instrument");
  }

  if (!loadInst->isExecuted())
    alg->getLogger().information("No IDF loaded from Nexus file.");
  return loadInst->isExecuted();
}
} // namespace DataHandling
} // namespace Mantid
