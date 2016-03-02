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
    if (temp < max) {
      max = temp;
    }
  }
  return max;
}

void DataBlockComposite::setMaxSpectrumID(int64_t) {
  // DO NOTHING
}

std::unique_ptr<DataBlockGenerator> DataBlockComposite::getGenerator() {
  std::vector<std::pair<int64_t, int64_t>> intervals;
  for (const auto &dataBlock : m_dataBlocks) {
    intervals.push_back(
        std::make_pair(dataBlock.getMinSpectrumID(), dataBlock.getMaxSpectrumID()));
  }
  return Mantid::Kernel::make_unique<DataBlockGenerator>(intervals);
}

int64_t DataBlockComposite::getNextSpectrumID(int64_t spectrumID) const {
  auto isInRange = [spectrumID](std::vector<DataBlock>::const_iterator block) {
    return (spectrumID >= block->getMinSpectrumID()) &&
           (spectrumID <= block->getMaxSpectrumID());
  };

  auto isBetween = [&spectrumID](
      std::vector<DataBlock>::const_iterator block,
      std::vector<DataBlock>::const_iterator blockPlus1) { return true; };

  for (auto it = m_dataBlocks.cbegin(); it != m_dataBlocks.cend(); ++it) {
  }

  return 12;
}

void DataBlockComposite::addDataBlock(DataBlock dataBlock) {
  // Set the number of periods, number of spectra and number of channel
  m_numberOfPeriods = dataBlock.getNumberOfPeriods();
  m_numberOfChannels = dataBlock.getNumberOfChannels();
  m_numberOfSpectra = dataBlock.getNumberOfSpectra();

  // Insert the data block
  m_dataBlocks.push_back(dataBlock);

  // We need to sort the data items. TODO: make this more efficient.
  auto comparison = [](const DataBlock &first, const DataBlock &second) {
    return first.getMinSpectrumID() < second.getMinSpectrumID();
  };
  std::sort(m_dataBlocks.begin(), m_dataBlocks.end(), comparison);
}
}
}
