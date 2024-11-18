// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/DataBlockComposite.h"
#include "MantidDataHandling/DataBlockGenerator.h"

#include <algorithm>
#include <cassert>
#include <numeric>

namespace {

using Mantid::specnum_t;
const specnum_t INVALIDINTERVALVALUE = std::numeric_limits<specnum_t>::min();
using Mantid::DataHandling::SpectrumPair;
/**
 * Gets all removal intervals which have an overlap with the original interval.
 * This can be either
 * 1. partial overlap: check if start or stop of remove interval
 * is contained in original interval.
 * 2. remove interval is contained in original interval: same check as above
 * 3. remove interval contains original interval: check start value of original
 * in range
 */
std::vector<SpectrumPair>
getRemovalIntervalsRelevantForTheCurrentOriginalInterval(const SpectrumPair &original,
                                                         const std::vector<SpectrumPair> &removeIntervals) {

  auto hasOverlap = [](const SpectrumPair &original, const SpectrumPair &toRemove) {
    return ((original.first <= toRemove.first) && (toRemove.first <= original.second)) ||
           ((original.first <= toRemove.second) && (toRemove.second <= original.second)) ||
           ((toRemove.first <= original.first) && (original.first <= toRemove.second));
  };

  std::vector<SpectrumPair> overlaps;
  for (auto &removeInterval : removeIntervals) {
    if (hasOverlap(original, removeInterval)) {
      overlaps.emplace_back(removeInterval);
    }
  }
  return overlaps;
}

/**
* Handles the scenario:
*   original :        |------....
*   toRemove:     |------|
*   result:
         cut             |---....
         return: NONE
*/
void handleLeftHandSideOverlap(SpectrumPair &original, const SpectrumPair &toRemove) {
  original.first = toRemove.second + 1;
}

/**
* Handles the scenario:
*   original :  ...------|
*   toRemove:        |------|
*   result:
        cut     we are at the end, set interval to invalid
     return     ...-|
*/
SpectrumPair handleRightHandSideOverlap(SpectrumPair &original, const SpectrumPair &toRemove) {
  auto newInterval = std::make_pair(original.first, toRemove.first - 1);
  original.first = INVALIDINTERVALVALUE;
  original.second = INVALIDINTERVALVALUE;
  return newInterval;
}

/**
* Handles the scenario:
*   original :  ...------------....
*   toRemove:        |------|
*   result:
       cut                  |---...
    return      ...--|
*/
SpectrumPair handleFullyContained(SpectrumPair &original, const SpectrumPair &toRemove) {
  // It is important to first creat the new pair and then perform the cut
  auto newPair = std::make_pair(original.first, toRemove.first - 1);
  original.first = toRemove.second + 1;
  return newPair;
}

std::vector<SpectrumPair> getSlicedIntervals(SpectrumPair original, const std::vector<SpectrumPair> &removeIntervals) {
  // If there is nothing to remove return the original
  if (removeIntervals.empty()) {
    return std::vector<SpectrumPair>{original};
  }

  // There are several overlap scenarios.
  // 1. Full overlap
  //    original :    |-------|      and |------|
  //    toRemove: ...------------... and |------|
  // 2. Left hand side overlap
  //    original :     |------...  and |-----....
  //    toRemove:   |------|       and |---|
  // 3. Right hand side overlap
  //    original :  ...-------|    and ...-----|
  //    toRemove:          |-----| and     |---|
  // 4. Fully contained
  //    original :  ...-------...
  //    toRemove:       |---|

  auto isFullOverlap = [](const SpectrumPair &original, const SpectrumPair &toRemove) {
    return (toRemove.first <= original.first) && (original.first <= toRemove.second) &&
           (toRemove.first <= original.second) && (original.second <= toRemove.second);
  };

  auto isLeftHandSideOverlap = [](const SpectrumPair &original, const SpectrumPair &toRemove) {
    return (toRemove.first <= original.first) && (original.first <= toRemove.second) &&
           (toRemove.second < original.second);
  };

  auto isRightHandSideOverlap = [](const SpectrumPair &original, const SpectrumPair &toRemove) {
    return (original.first < toRemove.first) && (toRemove.first <= original.second) &&
           (original.second <= toRemove.second);
  };

  auto isFullyContained = [](const SpectrumPair &original, const SpectrumPair &toRemove) {
    return (original.first < toRemove.first) && (toRemove.first < original.second) &&
           (original.first < toRemove.second) && (toRemove.second < original.second);
  };

  // Use that removeIntervals has oredred, non-overlapping intervals
  // Subtract all the removeIntervals
  std::vector<SpectrumPair> newIntervals;
  for (auto &removeInterval : removeIntervals) {

    if (isFullOverlap(original, removeInterval)) {
      // In this case we should remove everything. At this point
      // newIntervals
      // should still be empty, since the remove intervals should not be
      // overlapping
      assert(newIntervals.empty() && "DataBlockComposite: The "
                                     "newIntervals container should be "
                                     "empty");
      // Set the remainder of the original to invalid, such that we don't
      // pick
      // it up at the very end
      original.first = INVALIDINTERVALVALUE;
      original.second = INVALIDINTERVALVALUE;
      break;
    } else if (isRightHandSideOverlap(original, removeInterval)) {
      auto newInterval = handleRightHandSideOverlap(original, removeInterval);
      newIntervals.emplace_back(newInterval);
    } else if (isLeftHandSideOverlap(original, removeInterval)) {
      handleLeftHandSideOverlap(original, removeInterval);
    } else if (isFullyContained(original, removeInterval)) {
      auto newInterval = handleFullyContained(original, removeInterval);
      newIntervals.emplace_back(newInterval);
    } else {
      throw std::runtime_error("DataBlockComposite: The intervals don't seem to overlap.");
    }
  }

  // There might be some remainder in the original interval, e.g if there
  // wasn't
  // a full overlap removal
  // or no righ-hand-side overlap of a removal interval
  if ((original.first != INVALIDINTERVALVALUE) && (original.second != INVALIDINTERVALVALUE)) {
    newIntervals.emplace_back(original);
  }

  return newIntervals;
}

/**
 * Sorts a data block collection.
 */
template <typename T> void sortDataBlocks(T &dataBlcokCollection) {
  // Sort the intervals. We sort them by minimum value
  using namespace Mantid::DataHandling;
  auto comparison = [](const DataBlock &el1, const DataBlock &el2) {
    return el1.getMinSpectrumID() < el2.getMinSpectrumID();
  };
  std::sort(std::begin(dataBlcokCollection), std::end(dataBlcokCollection), comparison);
}

std::vector<SpectrumPair> spectrumIDIntervals(const std::vector<Mantid::DataHandling::DataBlock> &blocks) {
  std::vector<SpectrumPair> intervals;
  intervals.reserve(blocks.size());

  std::transform(blocks.begin(), blocks.end(), std::back_inserter(intervals),
                 [](const auto &block) { return std::make_pair(block.getMinSpectrumID(), block.getMaxSpectrumID()); });
  return intervals;
}
} // namespace

namespace Mantid::DataHandling {

specnum_t DataBlockComposite::getMinSpectrumID() const {
  specnum_t min = std::numeric_limits<specnum_t>::max();
  for (const auto &child : m_dataBlocks) {
    auto temp = child.getMinSpectrumID();
    if (temp < min) {
      min = temp;
    }
  }
  return min;
}

void DataBlockComposite::setMinSpectrumID(specnum_t /*minSpecID*/) {
  // DO NOTHING
}

specnum_t DataBlockComposite::getMaxSpectrumID() const {
  specnum_t max = std::numeric_limits<specnum_t>::min();
  for (const auto &child : m_dataBlocks) {
    auto temp = child.getMaxSpectrumID();
    if (temp > max) {
      max = temp;
    }
  }
  return max;
}

void DataBlockComposite::setMaxSpectrumID(specnum_t /*minSpecID*/) {
  // DO NOTHING
}

std::unique_ptr<DataBlockGenerator> DataBlockComposite::getGenerator() const {
  const auto intervals = spectrumIDIntervals(m_dataBlocks);
  return std::make_unique<DataBlockGenerator>(intervals);
}

void DataBlockComposite::addDataBlock(const DataBlock &dataBlock) {
  // Set the number of periods, number of spectra and number of channel
  m_numberOfPeriods = dataBlock.getNumberOfPeriods();
  m_numberOfChannels = dataBlock.getNumberOfChannels();
  m_numberOfSpectra = dataBlock.getNumberOfSpectra();

  // Insert the data block
  m_dataBlocks.emplace_back(dataBlock);
}

size_t DataBlockComposite::getNumberOfSpectra() const {
  return std::accumulate(m_dataBlocks.cbegin(), m_dataBlocks.cend(), static_cast<size_t>(0),
                         [](size_t sum, const auto &element) { return sum + element.getNumberOfSpectra(); });
}

size_t DataBlockComposite::getNumberOfChannels() const {
  return m_dataBlocks.empty() ? 0 : m_dataBlocks[0].getNumberOfChannels();
}

int DataBlockComposite::getNumberOfPeriods() const {
  return m_dataBlocks.empty() ? 0 : m_dataBlocks[0].getNumberOfPeriods();
}

DataBlockComposite DataBlockComposite::operator+(const DataBlockComposite &other) {
  DataBlockComposite output;
  output.m_dataBlocks.insert(std::end(output.m_dataBlocks), std::begin(m_dataBlocks), std::end(m_dataBlocks));
  output.m_dataBlocks.insert(std::end(output.m_dataBlocks), std::begin(other.m_dataBlocks),
                             std::end(other.m_dataBlocks));
  return output;
}

std::vector<DataBlock> DataBlockComposite::getDataBlocks() {
  // Sort the intervals. We sort them by minimum value
  sortDataBlocks(m_dataBlocks);
  return m_dataBlocks;
}

void DataBlockComposite::truncate(specnum_t specMin, specnum_t specMax) {
  sortDataBlocks(m_dataBlocks);
  // Find the first data block which is not completely cut off by specMin
  // original: |-----|      |--------|   |------|
  // spec_min:         | or | or | or|
  // result:                 this one
  auto isNotCompletelyCutOffFromMin = [&specMin](const DataBlock &block) {
    return (specMin <= block.getMinSpectrumID()) || (specMin <= block.getMaxSpectrumID());
  };

  // Find the last data block which is not completely cut off by specMax
  // original: |-----|      |--------|         |------|
  // spec_min:              | or | or| or  |
  // result:                 this one
  auto isNotCompletelyCutOffFromMax = [&specMax](const DataBlock &block) {
    return (block.getMinSpectrumID() <= specMax) || (block.getMaxSpectrumID() <= specMax);
  };

  auto firstDataBlock = std::find_if(std::begin(m_dataBlocks), std::end(m_dataBlocks), isNotCompletelyCutOffFromMin);

  // Note that we have to start from the back.
  auto lastDataBlockReverseIterator =
      std::find_if(m_dataBlocks.rbegin(), m_dataBlocks.rend(), isNotCompletelyCutOffFromMax);

  // Check the case where the actuall don't have any spectrum numbers in the
  // truncation interval
  // e.g    |-----|   |------|   |----|    or |-----|   |------|   |----|
  //     | |                                         | |
  auto isFirstDataBlockAtEnd = firstDataBlock == m_dataBlocks.end();
  auto isLastDataBlockReverseIteratorAtREnd = lastDataBlockReverseIterator == m_dataBlocks.rend();

  if (isFirstDataBlockAtEnd || isLastDataBlockReverseIteratorAtREnd) {
    std::vector<DataBlock> newDataBlocks;
    m_dataBlocks.swap(newDataBlocks);
    return;
  }

  // Check if we have an empty interval in the truncation
  auto isEmptyInterval = firstDataBlock->getMinSpectrumID() > lastDataBlockReverseIterator->getMaxSpectrumID();

  if (isEmptyInterval) {
    std::vector<DataBlock> newDataBlocks;
    m_dataBlocks.swap(newDataBlocks);
    return;
  }

  auto lastDataBlock = std::find(std::begin(m_dataBlocks), std::end(m_dataBlocks), *lastDataBlockReverseIterator);

  // Create datablocks
  // Increment since we want to include the last data block
  ++lastDataBlock;
  std::vector<DataBlock> newDataBlocks(firstDataBlock, lastDataBlock);

  // Adjust the spec_min and spec_max value. Only change the block if
  // the it cuts the block.
  if (newDataBlocks[0].getMinSpectrumID() < specMin) {
    auto numberOfSpectra = newDataBlocks[0].getMaxSpectrumID() - specMin + 1;
    DataBlock block(newDataBlocks[0].getNumberOfPeriods(), numberOfSpectra, newDataBlocks[0].getNumberOfChannels());
    block.setMinSpectrumID(specMin);
    block.setMaxSpectrumID(newDataBlocks[0].getMaxSpectrumID());
    newDataBlocks[0] = block;
  }

  auto lastIndex = newDataBlocks.size() - 1;
  if (specMax < newDataBlocks[lastIndex].getMaxSpectrumID()) {
    auto numberOfSpectra = specMax - newDataBlocks[lastIndex].getMaxSpectrumID() + 1;
    DataBlock block(newDataBlocks[lastIndex].getNumberOfPeriods(), numberOfSpectra,
                    newDataBlocks[lastIndex].getNumberOfChannels());
    block.setMinSpectrumID(newDataBlocks[lastIndex].getMinSpectrumID());
    block.setMaxSpectrumID(specMax);
    newDataBlocks[lastIndex] = block;
  }

  m_dataBlocks.swap(newDataBlocks);
}

bool DataBlockComposite::operator==(const DataBlockComposite &other) const {
  if (other.m_dataBlocks.size() != m_dataBlocks.size()) {
    return false;
  }

  // Create a copy of the intervals, since the comparison operator should not
  // have
  // side effects!!!!! We need the vector sorted to compare though
  auto otherDataBlocks = other.m_dataBlocks;
  auto thisDataBlocks = m_dataBlocks;
  sortDataBlocks(otherDataBlocks);
  sortDataBlocks(thisDataBlocks);

  auto isEqual = true;
  auto itOther = otherDataBlocks.cbegin();
  auto itThis = thisDataBlocks.cbegin();
  for (; itOther != otherDataBlocks.cend(); ++itOther, ++itThis) {
    isEqual = isEqual && *itOther == *itThis;
  }
  return isEqual;
}

/**
 * Removes the input data blocks from the current list of data blocks.
 * @param toRemove: data block composite to remove
 *
 * original: |-----|  |-------|     |----|
 * toRemove:     |------|       |--|
 * result:   |---|      |------|    |----|
 */
void DataBlockComposite::removeSpectra(DataBlockComposite &toRemove) {
  // Get intervals for current data blocks
  const auto originalIntervals = spectrumIDIntervals(m_dataBlocks);

  // Get intervals for the data blocks which should be removed
  const auto removeBlocks = toRemove.getDataBlocks();
  const auto toRemoveIntervals = spectrumIDIntervals(removeBlocks);

  // Now create the new intervals which don't include the removeInterval
  // values
  std::vector<SpectrumPair> newIntervals;
  for (const auto &originalInterval : originalIntervals) {
    // Find all relevant remove intervals. In principal this could
    // be made more efficient.
    auto currentRemovalIntervals =
        getRemovalIntervalsRelevantForTheCurrentOriginalInterval(originalInterval, toRemoveIntervals);
    auto slicedIntervals = getSlicedIntervals(originalInterval, currentRemovalIntervals);
    newIntervals.insert(std::end(newIntervals), std::begin(slicedIntervals), std::end(slicedIntervals));
  }

  // Create a new set of data blocks
  auto numberOfPeriods = m_dataBlocks[0].getNumberOfPeriods();
  auto numberOfChannels = m_dataBlocks[0].getNumberOfChannels();

  m_dataBlocks.clear();
  for (const auto &newInterval : newIntervals) {
    DataBlock dataBlock(numberOfPeriods, newInterval.second - newInterval.first + 1, numberOfChannels);
    dataBlock.setMinSpectrumID(newInterval.first);
    dataBlock.setMaxSpectrumID(newInterval.second);
    m_dataBlocks.emplace_back(dataBlock);
  }
}

/**
 * Provides a container with all spectrum numbers
 * @returns a container with all sepctrum numbers
 */
std::vector<specnum_t> DataBlockComposite::getAllSpectrumNumbers() {
  auto generator = getGenerator();
  std::vector<specnum_t> allSpectra;

  for (; !generator->isDone(); generator->next()) {
    allSpectra.emplace_back(generator->getValue());
  }

  return allSpectra;
}

bool DataBlockComposite::isEmpty() { return m_dataBlocks.empty(); }
} // namespace Mantid::DataHandling
