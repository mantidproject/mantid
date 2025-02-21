// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/DataBlock.h"
#include "MantidDataHandling/DataBlockGenerator.h"

#include <vector>

namespace Mantid::DataHandling {

// Defines a negative value which is not something which spectra
// indices can have.

DataBlock::DataBlock()
    : m_numberOfPeriods(0), m_numberOfSpectra(0), m_numberOfChannels(0),
      m_minSpectraID(std::numeric_limits<specnum_t>::max()), m_maxSpectraID(0) {}

DataBlock::DataBlock(const Mantid::NeXus::NXInt &data)
    : m_numberOfPeriods(static_cast<int>(data.dim0())), m_numberOfSpectra(data.dim1()), m_numberOfChannels(data.dim2()),
      m_minSpectraID(std::numeric_limits<specnum_t>::max()), m_maxSpectraID(0) {}

DataBlock::DataBlock(size_t numberOfPeriods, size_t numberOfSpectra, size_t numberOfChannels)
    : m_numberOfPeriods(numberOfPeriods), m_numberOfSpectra(numberOfSpectra), m_numberOfChannels(numberOfChannels),
      m_minSpectraID(std::numeric_limits<specnum_t>::max()), m_maxSpectraID(0) {}

specnum_t DataBlock::getMinSpectrumID() const { return m_minSpectraID; }

void DataBlock::setMinSpectrumID(specnum_t minSpecID) { m_minSpectraID = minSpecID; }

specnum_t DataBlock::getMaxSpectrumID() const { return m_maxSpectraID; }

void DataBlock::setMaxSpectrumID(specnum_t maxSpecID) { m_maxSpectraID = maxSpecID; }

size_t DataBlock::getNumberOfSpectra() const { return m_numberOfSpectra; }

size_t DataBlock::getNumberOfPeriods() const { return m_numberOfPeriods; }

size_t DataBlock::getNumberOfChannels() const { return m_numberOfChannels; }

std::unique_ptr<DataBlockGenerator> DataBlock::getGenerator() const {
  std::vector<SpectrumPair> interval{std::make_pair(m_minSpectraID, m_maxSpectraID)};
  return std::make_unique<DataBlockGenerator>(interval);
}

bool DataBlock::operator==(const DataBlock &other) const {
  return (m_numberOfPeriods == other.m_numberOfPeriods) && (m_numberOfChannels == other.m_numberOfChannels) &&
         (m_numberOfSpectra == other.m_numberOfSpectra) && (m_minSpectraID == other.m_minSpectraID) &&
         (m_maxSpectraID == other.m_maxSpectraID);
}

} // namespace Mantid::DataHandling
