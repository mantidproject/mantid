#include "MantidDataHandling/DataBlockComposite.h"
#include "MantidDataHandling/DataBlockGenerator.h"
#include <algorithm>

namespace Mantid {
namespace DataHandling {

int64_t DataBlockComposite::getMinSpectrumID() const {
  int64_t min = std::numeric_limits<int64_t>::max();
  for (const auto &child : m_dataBlocks) {
    auto temp = child.getMinSpectrumID();
    if (temp < min) {
      min = temp;
    }
  }
  return min;
}

void DataBlockComposite::setMinSpectrumID(int64_t) {
  // DO NOTHING
}

int64_t DataBlockComposite::getMaxSpectrumID() const {
  int64_t max = std::numeric_limits<int64_t>::min();
  for (const auto &child : m_dataBlocks) {
    auto temp = child.getMaxSpectrumID();
    if (temp > max) {
      max = temp;
    }
  }
  return max;
}

void DataBlockComposite::setMaxSpectrumID(int64_t) {
  // DO NOTHING
}

std::unique_ptr<DataBlockGenerator> DataBlockComposite::getGenerator() const {
  std::vector<std::pair<int64_t, int64_t>> intervals;
  for (const auto &dataBlock : m_dataBlocks) {
    intervals.push_back(std::make_pair(dataBlock.getMinSpectrumID(),
                                       dataBlock.getMaxSpectrumID()));
  }
  return Mantid::Kernel::make_unique<DataBlockGenerator>(intervals);
}

void DataBlockComposite::addDataBlock(DataBlock dataBlock) {
  // Set the number of periods, number of spectra and number of channel
  m_numberOfPeriods = dataBlock.getNumberOfPeriods();
  m_numberOfChannels = dataBlock.getNumberOfChannels();
  m_numberOfSpectra = dataBlock.getNumberOfSpectra();

  // Insert the data block
  m_dataBlocks.push_back(dataBlock);
}

size_t DataBlockComposite::getNumberOfSpectra() const {
  size_t total = 0;
  for (const auto &element : m_dataBlocks) {
    total += element.getNumberOfSpectra();
  }
  return total;
}

DataBlockComposite DataBlockComposite::
operator+(const DataBlockComposite &other) {
  DataBlockComposite output;
  output.m_dataBlocks.insert(std::end(output.m_dataBlocks),
                             std::begin(m_dataBlocks), std::end(m_dataBlocks));
  output.m_dataBlocks.insert(std::end(output.m_dataBlocks),
                             std::begin(other.m_dataBlocks),
                             std::end(other.m_dataBlocks));
  return output;
}


std::vector<DataBlock> DataBlockComposite::getIntervals() {
  // Sort the intervals. We sort them by minimum value
  auto comparison = [](const DataBlock &el1,
    const DataBlock &el2) {
    return el1.getMinSpectrumID() < el2.getMinSpectrumID();
  };
  std::sort(m_dataBlocks.begin(), m_dataBlocks.end(), comparison);
  return m_dataBlocks;
}


}
}
