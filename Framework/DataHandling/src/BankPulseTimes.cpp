// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/BankPulseTimes.h"

#include <nexus/NeXusFile.hpp>
#include <numeric>

using namespace Mantid::Kernel;

//===============================================================================================
// BankPulseTimes
//===============================================================================================

/// The first period
const unsigned int BankPulseTimes::FirstPeriod = 1;

namespace {
// these values are simliar to those in EventList::EventSortType
enum PulseSorting { UNKNOWN, UNSORTED, PULSETIME_SORT };
} // namespace

//----------------------------------------------------------------------------------------------
/** Constructor. Loads the pulse times from the bank entry of the file
 *
 * @param file :: nexus file open in the right bank entry
 * @param pNumbers :: Period numbers to index into. Index via frame/pulse
 */
BankPulseTimes::BankPulseTimes(::NeXus::File &file, const std::vector<int> &pNumbers)
    : periodNumbers(pNumbers), m_sorting_info(PulseSorting::UNKNOWN) {
  file.openData("event_time_zero");
  // Read the offset (time zero)
  // If the offset is not present, use Unix epoch
  if (!file.hasAttr("offset"))
    startTime = "1970-01-01T00:00:00Z";
  else
    file.getAttr("offset", startTime);
  Mantid::Types::Core::DateAndTime start(startTime);

  // number of pulse times
  const auto dataInfo = file.getInfo();
  const int64_t numValues =
      std::accumulate(dataInfo.dims.cbegin(), dataInfo.dims.cend(), int64_t{1}, std::multiplies<>());
  if (numValues == 0)
    throw std::runtime_error("event_time_zero field has no data!");

  const auto heldTimeZeroType = dataInfo.type;

  // Nexus only requires event_time_zero to be a NXNumber, we support two
  // possilites
  if (heldTimeZeroType == ::NeXus::FLOAT64) {
    this->readData<double>(file, numValues, start);
  } else if (heldTimeZeroType == ::NeXus::UINT64) {
    this->readData<uint64_t>(file, numValues, start);
  } else {
    throw std::invalid_argument("Unsupported type for event_time_zero");
  }
  file.closeData();

  // Ensure that we always have a consistency between nPulses and
  // periodNumbers containers
  if (pulseTimes.size() != pNumbers.size()) {
    periodNumbers = std::vector<int>(pulseTimes.size(), FirstPeriod);
  }
}

template <typename ValueType>
void BankPulseTimes::readData(::NeXus::File &file, int64_t numValues, Mantid::Types::Core::DateAndTime &start) {
  std::vector<int64_t> indexStart{0};
  std::vector<int64_t> indexStep{std::min(numValues, static_cast<int64_t>(12 * 3600 * 60))}; // 12 hour at 60Hz

  // getSlab needs the data allocated already
  std::vector<ValueType> rawData(indexStep[0]);

  // loop over chunks of data and transform each chunk
  while ((indexStep[0] > 0) && (indexStart[0] < numValues)) {
    file.getSlab(rawData.data(), indexStart, indexStep);

    // Now create the pulseTimes
    std::transform(rawData.cbegin(), rawData.cend(), std::back_inserter(pulseTimes),
                   [start](ValueType incremental_time) { return start + incremental_time; });

    // increment the slab to get
    indexStart[0] += indexStep[0];
    indexStep[0] = std::min(indexStep[0], numValues - indexStart[0]);
    // resize the vector
    rawData.resize(indexStep[0]);
  }
}

bool BankPulseTimes::arePulseTimesIncreasing() const {
  if (m_sorting_info == PulseSorting::UNKNOWN) {
    // only allow one thread to check sorting at a time
    // others can get the value without interference
    std::scoped_lock<std::mutex> _lock(m_sortingMutex);
    if (std::is_sorted(pulseTimes.cbegin(), pulseTimes.cend()))
      m_sorting_info = PulseSorting::PULSETIME_SORT;
    else
      m_sorting_info = PulseSorting::UNSORTED;
  }
  return m_sorting_info == PulseSorting::PULSETIME_SORT;
}

//----------------------------------------------------------------------------------------------
/** Constructor. Build from a vector of date and times.
 *  Handles a zero-sized vector
 *  @param times
 */
BankPulseTimes::BankPulseTimes(const std::vector<Mantid::Types::Core::DateAndTime> &times) : pulseTimes(times) {
  if (times.empty())
    periodNumbers = std::vector<int>();
  else
    periodNumbers = std::vector<int>(pulseTimes.size(), FirstPeriod); // TODO we are fixing this at 1 period for all
}

//----------------------------------------------------------------------------------------------
/** Comparison. Is this bank's pulse times array the same as another one.
 *
 * @param otherNumPulse :: number of pulses in the OTHER bank event_time_zero.
 * @param otherStartTime :: "offset" attribute of the OTHER bank
 * event_time_zero.
 * @return true if the pulse times are the same and so don't need to be
 * reloaded.
 */
bool BankPulseTimes::equals(size_t otherNumPulse, const std::string &otherStartTime) {
  return ((this->startTime == otherStartTime) && (this->pulseTimes.size() == otherNumPulse));
}
