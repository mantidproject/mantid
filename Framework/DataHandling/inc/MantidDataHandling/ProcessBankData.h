#ifndef MANTID_DATAHANDLING_PROCESSBANKDATA_H
#define MANTID_DATAHANDLING_PROCESSBANKDATA_H

#include "MantidDataHandling/BankPulseTimes.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/Task.h"
#include "MantidKernel/Timer.h"

#include <boost/shared_array.hpp>

namespace Mantid {
namespace API {
class Progress;
}
namespace DataHandling {
class DefaultEventLoader;

/** This task does the disk IO from loading the NXS file,
 * and so will be on a disk IO mutex */
class ProcessBankData : public Mantid::Kernel::Task {
public:
  /** Constructor
  *
  * @param loader :: DefaultEventLoader
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
  */ // API::IFileLoader<Kernel::NexusDescriptor>
  ProcessBankData(DefaultEventLoader &loader, std::string entry_name,
                  API::Progress *prog, boost::shared_array<uint32_t> event_id,
                  boost::shared_array<float> event_time_of_flight,
                  size_t numEvents, size_t startAt,
                  boost::shared_ptr<std::vector<uint64_t>> event_index,
                  boost::shared_ptr<BankPulseTimes> thisBankPulseTimes,
                  bool have_weight, boost::shared_array<float> event_weight,
                  detid_t min_event_id, detid_t max_event_id);

  void run() override;

private:
  size_t getWorkspaceIndexFromPixelID(const detid_t pixID);

  /// Algorithm being run
  DefaultEventLoader &m_loader;
  /// NXS path to bank
  std::string entry_name;
  /// Vector where (index = pixel ID+pixelID_to_wi_offset), value = workspace
  /// index)
  const std::vector<size_t> &pixelID_to_wi_vector;
  /// Offset in the pixelID_to_wi_vector to use.
  detid_t pixelID_to_wi_offset;
  /// Progress reporting
  API::Progress *prog;
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
  boost::shared_ptr<BankPulseTimes> thisBankPulseTimes;
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
}; // ENDDEF-CLASS ProcessBankData
} // namespace DataHandling
} // namespace Mantid

#endif
