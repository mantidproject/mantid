#include "MantidDataHandling/DataBlock.h"
#include "MantidDataHandling/DataBlockGenerator.h"
#include "MantidKernel/make_unique.h"
#include <vector>

namespace Mantid {
namespace DataHandling {

// Defines a negative value which is not something which spectra
// indices can have.
const int64_t DataBlock::end = std::numeric_limits<int64_t>::min();

DataBlock::DataBlock()
  : m_numberOfPeriods(0), m_numberOfChannels(0), m_numberOfSpectra(0),
  m_minSpectraID(std::numeric_limits<int64_t>::max()), m_maxSpectraID(0) {
}

DataBlock::DataBlock(const Mantid::NeXus::NXInt &data)
  : m_numberOfPeriods(data.dim0()), m_numberOfChannels(data.dim2()),
    m_numberOfSpectra(data.dim1()),
  m_minSpectraID(std::numeric_limits<int64_t>::max()), m_maxSpectraID(0) {}

DataBlock::~DataBlock() {}

int64_t DataBlock::getMinSpectrumID() const { return m_minSpectraID; }

void DataBlock::setMinSpectrumID(int64_t minSpecID) {
  m_minSpectraID = minSpecID;
}

int64_t DataBlock::getMaxSpectrumID() const { return m_maxSpectraID; }

void DataBlock::setMaxSpectrumID(int64_t maxSpecID) {
  m_maxSpectraID = maxSpecID;
}

size_t DataBlock::getNumberOfSpectra() const { return m_numberOfSpectra; }

int DataBlock::getNumberOfPeriods() const { return m_numberOfPeriods; }

size_t DataBlock::getNumberOfChannels() const { return m_numberOfChannels; }

std::unique_ptr<DataBlockGenerator> DataBlock::getGenerator() {
  std::vector<std::pair<int64_t, int64_t>> interval{ std::make_pair(m_minSpectraID, m_maxSpectraID) };
  return Mantid::Kernel::make_unique<DataBlockGenerator>(interval);
}

} // namespace DataHandling
} // namespace Mantid
