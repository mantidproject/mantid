#include "MantidAPI/MatrixWSIndexCalculator.h"

namespace Mantid
{
  namespace API
  {
    MatrixWSIndexCalculator::MatrixWSIndexCalculator() : m_blockSize(-1)
    {
    }

    MatrixWSIndexCalculator::MatrixWSIndexCalculator(int blockSize) : m_blockSize(blockSize)
    {
    }

    HistogramIndex MatrixWSIndexCalculator::getHistogramIndex(Index oneDimIndex) const
    {
      return oneDimIndex / m_blockSize;
    }

    BinIndex MatrixWSIndexCalculator::getBinIndex(Index oneDimIndex, HistogramIndex histogramDimIndex) const
    {
      return oneDimIndex - (histogramDimIndex * m_blockSize);
    }

    Index MatrixWSIndexCalculator::getOneDimIndex(HistogramIndex histogramIndex, BinIndex binIndex) const
    {
       return binIndex + (histogramIndex * m_blockSize);
    }

    MatrixWSIndexCalculator& MatrixWSIndexCalculator::operator=(const MatrixWSIndexCalculator& other)
    {
      if (this != &other) // protect against invalid self-assignment
      {
        this->m_blockSize = other.m_blockSize;
      }
      return *this;
    }

    MatrixWSIndexCalculator::MatrixWSIndexCalculator(const MatrixWSIndexCalculator& other)
    {
      m_blockSize =other.m_blockSize;
    }
  }
}
