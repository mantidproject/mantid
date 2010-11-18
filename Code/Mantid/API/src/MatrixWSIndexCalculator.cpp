#include "MantidAPI/MatrixWSIndexCalculator.h"

namespace Mantid
{
  namespace API
  {
    MatrixWSIndexCalculator::MatrixWSIndexCalculator(long blockSize) : m_blockSize(blockSize)
    {
    }

    long MatrixWSIndexCalculator::getHistogramIndex(long index)
    {
      return index / m_blockSize;
    }

    long MatrixWSIndexCalculator::getBinIndex(long index, long histogram)
    {
      return index  - (histogram * m_blockSize);
    }
  }
}