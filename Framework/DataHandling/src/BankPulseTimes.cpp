// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/BankPulseTimes.h"
#include "MantidNexusCpp/NeXusFile.hpp"
#include <numeric>

using namespace Mantid::Kernel;

//===============================================================================================
// BankPulseTimes
//===============================================================================================

namespace {
// these values are simliar to those in EventList::EventSortType
enum PulseSorting { UNKNOWN, UNSORTED, PULSETIME_SORT };

} // namespace

namespace Mantid::DataHandling {

const std::string BankPulseTimes::DEFAULT_START_TIME("1970-01-01T00:00:00Z");

const int BankPulseTimes::FIRST_PERIOD(1);

//----------------------------------------------------------------------------------------------
/** Constructor. Build from a vector of date and times.
 *  Handles a zero-sized vector
 *  @param times
 */
BankPulseTimes::BankPulseTimes(const std::vector<Mantid::Types::Core::DateAndTime> &times)
    : startTime(DEFAULT_START_TIME), pulseTimes(times), have_period_info(false), m_sorting_info(PulseSorting::UNKNOWN) {
  this->updateStartTime();
  this->finalizePeriodNumbers();
}

BankPulseTimes::BankPulseTimes(const std::vector<Mantid::Types::Core::DateAndTime> &times,
                               const std::vector<int> &periodNumbers)
    : startTime(DEFAULT_START_TIME), periodNumbers(periodNumbers), pulseTimes(times), have_period_info(true),
      m_sorting_info(PulseSorting::UNKNOWN) {
  this->updateStartTime();
  this->finalizePeriodNumbers();
}

/** Constructor. Loads the pulse times from the bank entry of the file
 *
 * @param file :: nexus file open in the right bank entry
 * @param periodNumbers :: Period numbers to index into. Index via frame/pulse
 */
BankPulseTimes::BankPulseTimes(::NeXus::File &file, const std::vector<int> &periodNumbers)
    : startTime(DEFAULT_START_TIME), periodNumbers(periodNumbers), have_period_info(true),
      m_sorting_info(PulseSorting::UNKNOWN) {

  // Some old data use "pulse_time" instead of "event_time_zero" as entry
  try {
    file.openData("event_time_zero");
  } catch (const std::exception &) {
    file.openData("pulse_time");
  }
  // Read the offset (time zero)

  // Use the offset if it is present
  if (file.hasAttr("offset"))
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
  if (heldTimeZeroType == NXnumtype::FLOAT64) {
    this->readData<double>(file, numValues, start);
  } else if (heldTimeZeroType == NXnumtype::UINT64) {
    this->readData<uint64_t>(file, numValues, start);
  } else {
    throw std::invalid_argument("Unsupported type for event_time_zero");
  }
  file.closeData();

  this->finalizePeriodNumbers();
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

void BankPulseTimes::updateStartTime() {
  if (!pulseTimes.empty()) {
    const auto minimum = std::min_element(pulseTimes.cbegin(), pulseTimes.cend());
    startTime = minimum->toISO8601String();
  }
  // otherwise the existing startTime stays
}

void BankPulseTimes::finalizePeriodNumbers() {
  if (pulseTimes.empty()) {
    // set periods to empty vector
    periodNumbers = std::vector<int>();
    have_period_info = true;
  } else if (pulseTimes.size() != periodNumbers.size()) {
    // something went wrong, everything is first period
    have_period_info = false;
  }
}

//----------------------------------------------------------------------------------------------

size_t BankPulseTimes::numberOfPulses() const { return pulseTimes.size(); }

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

int BankPulseTimes::periodNumber(const size_t index) const {
  if (have_period_info)
    return this->periodNumbers[index];
  else
    return FIRST_PERIOD;
}

const Mantid::Types::Core::DateAndTime &BankPulseTimes::pulseTime(const size_t index) const {
  return this->pulseTimes[index];
}

//----------------------------------------------------------------------------------------------

namespace {
std::size_t getFirstIncludedIndex(const std::vector<Mantid::Types::Core::DateAndTime> &pulseTimes,
                                  const std::size_t startIndex, const Mantid::Types::Core::DateAndTime &start,
                                  const Mantid::Types::Core::DateAndTime &stop) {
  const auto NUM_PULSES{pulseTimes.size()};
  if (startIndex >= NUM_PULSES)
    return NUM_PULSES;

  for (size_t i = startIndex; i < NUM_PULSES; ++i) {
    const auto pulseTime = pulseTimes[i];
    if (pulseTime >= start && pulseTime < stop)
      return i;
  }
  return NUM_PULSES; // default is the number of pulses
}

std::size_t getFirstExcludedIndex(const std::vector<Mantid::Types::Core::DateAndTime> &pulseTimes,
                                  const std::size_t startIndex, const Mantid::Types::Core::DateAndTime &start,
                                  const Mantid::Types::Core::DateAndTime &stop) {
  const auto NUM_PULSES{pulseTimes.size()};
  if (startIndex >= NUM_PULSES)
    return NUM_PULSES;

  for (size_t i = startIndex; i < NUM_PULSES; ++i) {
    const auto pulseTime = pulseTimes[i];
    if (pulseTime < start || pulseTime > stop)
      return i;
  }
  return NUM_PULSES; // default is the number of pulses
}
} // namespace

// use this to get timeof interest into pulseindexter
std::vector<size_t> BankPulseTimes::getPulseIndices(const Mantid::Types::Core::DateAndTime &start,
                                                    const Mantid::Types::Core::DateAndTime &stop) const {
  std::vector<size_t> roi;
  if (this->arePulseTimesIncreasing()) {
    // sorted pulse times don't have to go through the whole vector, just look at the ends
    const bool includeStart = start <= this->pulseTimes.front();
    const bool includeStop = stop >= this->pulseTimes.back();
    if (!(includeStart && includeStop)) {
      // get start index
      if (includeStart) {
        roi.push_back(0);
      } else {
        // do a linear search with the assumption that the index will be near the beginning
        roi.push_back(getFirstIncludedIndex(this->pulseTimes, 0, start, stop));
      }
      // get stop index
      if (includeStop) {
        roi.push_back(this->pulseTimes.size());
      } else {
        const auto start_index = roi.front();
        // do a linear search with the assumption that the index will be near the beginning
        for (size_t index = this->pulseTimes.size() - 1; index > start_index; --index) {
          if (this->pulseTime(index) <= stop) {
            roi.push_back(index + 1); // include this pulse
            break;
          }
        }
      }
    }
  } else {
    // loop through the entire vector of pulse times
    const auto [min_ele, max_ele] = std::minmax_element(this->pulseTimes.cbegin(), this->pulseTimes.cend());
    const bool includeStart = (start <= *min_ele);
    const bool includeStop = (stop >= *max_ele);

    // only put together range if one is needed
    if (!(includeStart && includeStop)) {
      const auto NUM_PULSES = this->pulseTimes.size();
      std::size_t firstInclude = getFirstIncludedIndex(this->pulseTimes, 0, start, stop);
      while (firstInclude < NUM_PULSES) {
        auto firstExclude = getFirstExcludedIndex(this->pulseTimes, firstInclude + 1, start, stop);
        if (firstInclude != firstExclude) {
          roi.push_back(firstInclude);
          roi.push_back(firstExclude);
          firstInclude = getFirstIncludedIndex(this->pulseTimes, firstExclude + 1, start, stop);
        }
      }
    }
  }

  if ((!roi.empty()) && (roi.size() % 2 != 0)) {
    std::stringstream msg;
    msg << "Invalid state for ROI. Has odd number of values: " << roi.size();
    throw std::runtime_error(msg.str());
  }

  return roi;
}

std::vector<size_t> BankPulseTimes::getPulseIndices(const std::vector<Mantid::Kernel::TimeInterval> &splitters) const {
  std::vector<size_t> roi;
  size_t start_index{0};
  for (const auto &splitter : splitters) {
    // do a linear search using the previous index as the starting point
    roi.push_back(start_index =
                      getFirstIncludedIndex(this->pulseTimes, start_index, splitter.start(), splitter.stop()));
    // we need the one before the first excluded index so do -1
    roi.push_back(start_index =
                      getFirstExcludedIndex(this->pulseTimes, start_index, splitter.start(), splitter.stop()) - 1);
  }
  return roi;
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

} // namespace Mantid::DataHandling
