#include "MantidAPI/MatrixWSIndexCalculator.h"

namespace Mantid
{
  namespace API
  {
    MatrixWSIndexCalculator::MatrixWSIndexCalculator(int blockSize) : m_blockSize(blockSize)
    {
    }

    int MatrixWSIndexCalculator::getHistogramIndex(int index)
    {
      return index / m_blockSize;
    }

    int MatrixWSIndexCalculator::getBinIndex(int index, int histogram)
    {
      return index  - (histogram * m_blockSize);
    }
  }
}