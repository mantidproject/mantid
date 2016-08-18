#include "MantidAPI/Progress.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/Events.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Timer.h"

#include <boost/lexical_cast.hpp>
#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>

using namespace Mantid;

//==============================================================================================
// Class ProcessBankData
//==============================================================================================
/** This task does the disk IO from loading the NXS file,
* It is designed to be called by methods in class LoadBankFromDiskTask */
class ProcessBankData { // : public Mantid::Kernel::Task {
public:
  //----------------------------------------------------------------------------------------------
  /** Constructor
  *
  * @param alg :: LoadEventNexus
  * @param entry_name :: name of the bank
  * @param prog :: Progress reporter
  * @param event_id :: array with event IDs
  * @param event_time_of_flight :: array with event TOFS
  * @param numEvents :: how many events in the arrays
  * @param startAt :: index of the first event from event_index
  * @param event_index :: vector of event index (length of # of pulses)
  * @param thisBankPulseTimes :: ptr to the pulse times for this particular
  *bank.
  * @param have_weight :: flag for handling simulated files
  * @param event_weight :: array with weights for events
  * @param min_event_id ;: minimum detector ID to load
  * @param max_event_id :: maximum detector ID to load
  * @return
  */
  ProcessBankData(Mantid::DataHandling::LoadEventNexus *alg,
                  std::string entry_name, Mantid::API::Progress *prog,
                  boost::shared_array<uint32_t> event_id,
                  boost::shared_array<float> event_time_of_flight,
                  size_t numEvents, size_t startAt,
                  boost::shared_ptr<std::vector<uint64_t>> event_index,
                  boost::shared_ptr<Mantid::DataHandling::BankPulseTimes>
                      thisBankPulseTimes,
                  bool have_weight, boost::shared_array<float> event_weight,
                  detid_t min_event_id, detid_t max_event_id);

  void compressEvents(bool compress, bool pulsetimesincreasing,
                      const std::vector<bool> &usedDetIds);

  // bool findPulseTime(int &pulse_i, int numPulses);

  void run();

  // Pre-counting events per pixel ID
  void precountEvents();

  void processEvents(int numPulses, int pulse_i, bool compress,
                     bool &pulsetimesincreasing, double &my_shortest_tof,
                     double &my_longest_tof, size_t &badTofs,
                     size_t &my_discarded_events,
                     std::vector<bool> &usedDetIds);

private:
  /// Algorithm being run
  Mantid::DataHandling::LoadEventNexus *alg;
  /// NXS path to bank
  std::string entry_name;
  /// Vector where (index = pixel ID+pixelID_to_wi_offset), value = workspace
  /// index)
  const std::vector<size_t> &pixelID_to_wi_vector;
  /// Offset in the pixelID_to_wi_vector to use.
  detid_t pixelID_to_wi_offset;
  /// Progress reporting
  Mantid::API::Progress *prog;
  /// event pixel ID array
  boost::shared_array<uint32_t> event_id;
  /// event TOF array
  boost::shared_array<float> event_time_of_flight;
  /// # of events in arrays
  size_t numEvents;
  /// index of the first event from event_index
  size_t startAt;
  /// vector of event index (length of # of pulses)
  boost::shared_ptr<std::vector<uint64_t>> event_index;
  /// Pulse times for this bank
  boost::shared_ptr<Mantid::DataHandling::BankPulseTimes> thisBankPulseTimes;
  /// Flag for simulated data
  bool have_weight;
  /// event weights array
  boost::shared_array<float> event_weight;
  /// Minimum pixel id
  detid_t m_min_id;
  /// Maximum pixel id
  detid_t m_max_id;
  /// timer for performance
  Mantid::Kernel::Timer m_timer;
}; // END-DEF-CLASS ProcessBankData

/** This task does the disk IO from loading the NXS file,
* and so will be on a disk IO mutex */
class LoadBankFromDiskTask { // : public Mantid::Kernel::Task {

public:
  LoadBankFromDiskTask(Mantid::DataHandling::LoadEventNexus *input_alg,
                       const std::string &entry_name,
                       const std::string &entry_type,
                       const bool oldNeXusFileNames,
                       Mantid::API::Progress *prog,
                       const std::vector<int> &framePeriodNumbers,
                       Mantid::Kernel::Logger &logger);

  void loadPulseTimes(::NeXus::File &file);

  void loadEventIndex(::NeXus::File &file, std::vector<uint64_t> &event_index);

  void prepareEventId(::NeXus::File &file, size_t &start_event,
                      size_t &stop_event, std::vector<uint64_t> &event_index);

  void loadEventId(::NeXus::File &file);

  void loadTof(::NeXus::File &file);

  void loadEventWeights(::NeXus::File &file);

  void run();

  void readFile(std::vector<uint64_t> *index_ptr);

  bool checkSpectra();

  int64_t recalculateDataSize(const int64_t &size);

private:
  /// Algorithm being run
  // LoadEventNexus *alg;
  Mantid::DataHandling::LoadEventNexus *alg;
  /// NXS path to bank
  std::string entry_name;
  /// NXS type
  std::string entry_type;
  /// Progress reporting
  Mantid::API::Progress *prog;
  /// ThreadScheduler running this task
  // Mantid::Kernel::ThreadScheduler *scheduler;
  /// Object with the pulse times for this bank
  boost::shared_ptr<Mantid::DataHandling::BankPulseTimes> thisBankPulseTimes;
  /// Did we get an error in loading
  bool m_loadError;
  /// Old names in the file?
  bool m_oldNexusFileNames;
  /// Index to load start at in the file
  std::vector<int> m_loadStart;
  /// How much to load in the file
  std::vector<int> m_loadSize;
  /// Event pixel ID data
  uint32_t *m_event_id;
  /// Minimum pixel ID in this data
  uint32_t m_min_id;
  /// Maximum pixel ID in this data
  uint32_t m_max_id;
  /// TOF data
  float *m_event_time_of_flight;
  /// Flag for simulated data
  bool m_have_weight;
  /// Event weights
  float *m_event_weight;
  /// Frame period numbers
  const std::vector<int> m_framePeriodNumbers;

  /// TODO-FIXME: NEW CLASS VARIABLES! NOT INITIALIZED IN CONSTRUCTOR YET!

  Mantid::Kernel::Logger &alg_Logger;
}; // END-DEF-CLASS LoadBankFromDiskTask
