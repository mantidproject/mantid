#include "MantidDataHandling/BankPulseTimes.h"

using namespace Mantid::Kernel;
//===============================================================================================
// BankPulseTimes
//===============================================================================================

/// The first period
const unsigned int BankPulseTimes::FirstPeriod = 1;

//----------------------------------------------------------------------------------------------
/** Constructor. Loads the pulse times from the bank entry of the file
 *
 * @param file :: nexus file open in the right bank entry
 * @param pNumbers :: Period numbers to index into. Index via frame/pulse
 */
BankPulseTimes::BankPulseTimes(::NeXus::File &file,
                               const std::vector<int> &pNumbers)
    : periodNumbers(pNumbers) {
  file.openData("event_time_zero");
  // Read the offset (time zero)
  file.getAttr("offset", startTime);
  Mantid::Types::Core::DateAndTime start(startTime);
  // Load the seconds offsets
  std::vector<double> seconds;
  file.getData(seconds);
  file.closeData();
  // Now create the pulseTimes
  numPulses = seconds.size();
  if (numPulses == 0)
    throw std::runtime_error("event_time_zero field has no data!");

  // Ensure that we always have a consistency between nPulses and periodNumbers
  // containers
  if (numPulses != pNumbers.size()) {
    periodNumbers = std::vector<int>(numPulses, FirstPeriod);
    ;
  }

  pulseTimes = new Mantid::Types::Core::DateAndTime[numPulses];
  for (size_t i = 0; i < numPulses; i++)
    pulseTimes[i] = start + seconds[i];
}

//----------------------------------------------------------------------------------------------
/** Constructor. Build from a vector of date and times.
 *  Handles a zero-sized vector
 *  @param times
 */
BankPulseTimes::BankPulseTimes(
    const std::vector<Mantid::Types::Core::DateAndTime> &times) {
  numPulses = times.size();
  pulseTimes = nullptr;
  if (numPulses == 0)
    return;
  pulseTimes = new Mantid::Types::Core::DateAndTime[numPulses];
  periodNumbers = std::vector<int>(
      numPulses, FirstPeriod); // TODO we are fixing this at 1 period for all
  for (size_t i = 0; i < numPulses; i++)
    pulseTimes[i] = times[i];
}

//----------------------------------------------------------------------------------------------
/** Destructor */
BankPulseTimes::~BankPulseTimes() { delete[] this->pulseTimes; }

//----------------------------------------------------------------------------------------------
/** Comparison. Is this bank's pulse times array the same as another one.
 *
 * @param otherNumPulse :: number of pulses in the OTHER bank event_time_zero.
 * @param otherStartTime :: "offset" attribute of the OTHER bank
 * event_time_zero.
 * @return true if the pulse times are the same and so don't need to be
 * reloaded.
 */
bool BankPulseTimes::equals(size_t otherNumPulse, std::string otherStartTime) {
  return ((this->startTime == otherStartTime) &&
          (this->numPulses == otherNumPulse));
}
