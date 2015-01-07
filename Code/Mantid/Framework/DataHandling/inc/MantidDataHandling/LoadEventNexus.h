#ifndef MANTID_DATAHANDLING_LOADEVENTNEXUS_H_
#define MANTID_DATAHANDLING_LOADEVENTNEXUS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFileLoader.h"
#include "MantidDataObjects/EventWorkspace.h"
#include <nexus/NeXusFile.hpp>
#include <nexus/NeXusException.hpp>
#include "MantidDataObjects/Events.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid {

namespace DataHandling {

/** This class defines the pulse times for a specific bank.
 * Since some instruments (ARCS, VULCAN) have multiple preprocessors,
 * this means that some banks have different lists of pulse times.
 */
class BankPulseTimes {
public:
  /// Constructor with NeXus::File
  BankPulseTimes(::NeXus::File &file);

  /// Constructor with vector of DateAndTime
  BankPulseTimes(const std::vector<Kernel::DateAndTime> &times);

  /// Destructor
  ~BankPulseTimes();

  /// Equals
  bool equals(size_t otherNumPulse, std::string otherStartTime);

  /// String describing the start time
  std::string startTime;

  /// Size of the array of pulse times
  size_t numPulses;

  /// Array of the pulse times
  Kernel::DateAndTime *pulseTimes;
};

/** @class LoadEventNexus LoadEventNexus.h Nexus/LoadEventNexus.h

  Load Event Nexus files.

  Required Properties:
  <UL>
  <LI> Filename - The name of and path to the input NeXus file </LI>
  <LI> Workspace - The name of the workspace to output</LI>
  </UL>

  @date Sep 27, 2010

  Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  */
class DLLExport LoadEventNexus
    : public API::IFileLoader<Kernel::NexusDescriptor> {

public:
  LoadEventNexus();
  virtual ~LoadEventNexus();

  virtual const std::string name() const { return "LoadEventNexus"; };

  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Loads Event NeXus files (produced by the SNS) and stores it in an "
           "EventWorkspace. Optionally, you can filter out events falling "
           "outside a range of times-of-flight and/or a time interval.";
  }

  /// Version
  virtual int version() const { return 1; };

  /// Category
  virtual const std::string category() const { return "DataHandling\\Nexus"; }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const;

  /** Sets whether the pixel counts will be pre-counted.
   * @param value :: true if you want to precount. */
  void setPrecount(bool value) { precount = value; }

  static boost::shared_ptr<BankPulseTimes>
  runLoadNexusLogs(const std::string &nexusfilename,
                   API::MatrixWorkspace_sptr localWorkspace, Algorithm &alg,
                   bool returnpulsetimes);

  static void loadEntryMetadata(const std::string &nexusfilename,
                                Mantid::API::MatrixWorkspace_sptr WS,
                                const std::string &entry_name);
  /// Load instrument from Nexus file if possible, else from IDF spacified by
  /// Nexus file
  static bool loadInstrument(const std::string &nexusfilename,
                             API::MatrixWorkspace_sptr localWorkspace,
                             const std::string &top_entry_name, Algorithm *alg);

  /// Load instrument for Nexus file
  static bool runLoadIDFFromNexus(const std::string &nexusfilename,
                                  API::MatrixWorkspace_sptr localWorkspace,
                                  const std::string &top_entry_name,
                                  Algorithm *alg);

  /// Load instrument from IDF file specified by Nexus file
  static bool runLoadInstrument(const std::string &nexusfilename,
                                API::MatrixWorkspace_sptr localWorkspace,
                                const std::string &top_entry_name,
                                Algorithm *alg);

  static void
  loadSampleDataISIScompatibility(::NeXus::File &file,
                                  Mantid::API::MatrixWorkspace_sptr WS);

  /// method used to return instrument name for some old ISIS files where it is
  /// not written properly within the instrument
  static std::string readInstrumentFromISIS_VMSCompat(::NeXus::File &hFile);

public:
  /// The name and path of the input file
  std::string m_filename;

  /// The workspace being filled out
  DataObjects::EventWorkspace_sptr WS;

  /// Filter by a minimum time-of-flight
  double filter_tof_min;
  /// Filter by a maximum time-of-flight
  double filter_tof_max;

  /// Spectra list to load
  std::vector<int32_t> m_specList;
  /// Minimum spectrum to load
  int32_t m_specMin;
  /// Maximum spectrum to load
  int32_t m_specMax;

  /// Filter by start time
  Kernel::DateAndTime filter_time_start;
  /// Filter by stop time
  Kernel::DateAndTime filter_time_stop;
  /// chunk number
  int chunk;
  /// number of chunks
  int totalChunks;
  /// for multiple chunks per bank
  int firstChunkForBank;
  /// number of chunks per bank
  size_t eventsPerChunk;

  /// Was the instrument loaded?
  bool instrument_loaded_correctly;

  /// Mutex protecting tof limits
  Poco::FastMutex m_tofMutex;

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

  /// Do we pre-count the # of events in each pixel ID?
  bool precount;

  /// Tolerance for CompressEvents; use -1 to mean don't compress.
  double compressTolerance;

  /// Do we load the sample logs?
  bool loadlogs;
  /// have the logs been loaded?
  bool logs_loaded_correctly;

  /// Pointer to the vector of events
  typedef std::vector<Mantid::DataObjects::TofEvent> *EventVector_pt;

  /// Vector where index = event_id; value = ptr to std::vector<TofEvent> in the
  /// event list.
  std::vector<EventVector_pt> eventVectors;

  /// Mutex to protect eventVectors from each task
  Poco::Mutex m_eventVectorMutex;

  /// Maximum (inclusive) event ID possible for this instrument
  int32_t eventid_max;

  /// Vector where (index = pixel ID+pixelID_to_wi_offset), value = workspace
  /// index)
  std::vector<size_t> pixelID_to_wi_vector;

  /// Offset in the pixelID_to_wi_vector to use.
  detid_t pixelID_to_wi_offset;

  /// True if the event_id is spectrum no not pixel ID
  bool event_id_is_spec;

  /// One entry of pulse times for each preprocessor
  std::vector<boost::shared_ptr<BankPulseTimes>> m_bankPulseTimes;

  /// Pulse times for ALL banks, taken from proton_charge log.
  boost::shared_ptr<BankPulseTimes> m_allBanksPulseTimes;

  /// Flag for dealing with a simulated file
  bool m_haveWeights;

  /// Pointer to the vector of weighted events
  typedef std::vector<Mantid::DataObjects::WeightedEvent> *
      WeightedEventVector_pt;

  /// Vector where index = event_id; value = ptr to std::vector<WeightedEvent>
  /// in the event list.
  std::vector<WeightedEventVector_pt> weightedEventVectors;

  DataObjects::EventWorkspace_sptr createEmptyEventWorkspace();

  /// Map detector IDs to event lists.
  template <class T> void makeMapToEventLists(std::vector<T> &vectors);

  void loadEvents(API::Progress *const prog, const bool monitors);
  void createSpectraMapping(
      const std::string &nxsfile, const bool monitorsOnly,
      const std::vector<std::string> &bankNames = std::vector<std::string>());
  void deleteBanks(API::MatrixWorkspace_sptr workspace,
                   std::vector<std::string> bankNames);
  bool hasEventMonitors();
  void runLoadMonitorsAsEvents(API::Progress *const prog);
  void runLoadMonitors();
  /// Set the filters on TOF.
  void setTimeFilters(const bool monitors);

  /// Load a spectra mapping from the given file
  bool loadSpectraMapping(const std::string &filename, const bool monitorsOnly,
                          const std::string &entry_name);

private:
  void init();
  void exec();

  /// ISIS specific methods for dealing with wide events
  static void loadTimeOfFlight(const std::string &nexusfilename,
                               DataObjects::EventWorkspace_sptr WS,
                               const std::string &entry_name,
                               const std::string &classType);

  static void loadTimeOfFlightData(::NeXus::File &file,
                                   DataObjects::EventWorkspace_sptr WS,
                                   const std::string &binsName,
                                   size_t start_wi = 0, size_t end_wi = 0);

  void filterDuringPause(API::MatrixWorkspace_sptr workspace);

  // Validate the optional spectra input properties and initialize m_specList
  void createSpectraList(int32_t min, int32_t max);

public:
  /// name of top level NXentry to use
  std::string m_top_entry_name;
  /// Set the top entry field name
  void setTopEntryName();
  /// whether or not to launch multiple ProcessBankData jobs per bank
  bool splitProcessing;
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADEVENTNEXUS_H_*/
