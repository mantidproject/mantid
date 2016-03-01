#include "MantidDataHandling/DataBlock.h"
#include "MantidDataHandling/DataBlockGenerator.h"

namespace Mantid {
namespace DataHandling {

// -------------------------------------------------------------
// DataBlock Generator
// -------------------------------------------------------------
DataBlockGenerator::DataBlockGenerator(DataBlock *datablock, int64_t startIndex)
    : m_currentIndex(startIndex) {}

void DataBlockGenerator::operator++() {
  m_currentIndex = m_dataBlock->getNextSpectrumID(m_currentIndex);
}

bool DataBlockGenerator::isDone() { return m_currentIndex != DataBlock::end; }

int64_t DataBlockGenerator::getCurrent() { return m_currentIndex; }
}
}
