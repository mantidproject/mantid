#include "MDDataObjects/MDIndexCalculator.h"


namespace Mantid
{
namespace MDDataObjects
{

/**
  * Constructor.
  * @param nDimensions :: Number of dimensions.
  * @param size1 :: Size of the first dimension. Passing -1 (default) means the dimension is undefined.
  * @param size2 :: Size of the second dimension. Passing -1 (default) means the dimension is undefined.
  * @param size3 :: Size of the third dimension. Passing -1 (default) means the dimension is undefined.
  * @param size4 :: Size of the fourth dimension. Passing -1 (default) means the dimension is undefined.
  */
MDWorkspaceIndexCalculator::MDWorkspaceIndexCalculator(unsigned int nDimensions, int size1, int size2, int size3, int size4) :
  m_nDimensions(nDimensions),m_dimSizes(nDimensions, 0),m_isSetup(false)
{
  if (size1 <= 0) return;
  setDimensionSize(0,size1);
  if (size2 <= 0 || nDimensions < 2) return;
  setDimensionSize(1,size2);
  if (size3 <= 0 || nDimensions < 3) return;
  setDimensionSize(2,size3);
  if (size4 <= 0 || nDimensions < 4) return;
  setDimensionSize(3,size4);
}

std::vector<int> MDWorkspaceIndexCalculator::cacluateCoefficients() const
{
  std::vector<int> coeffs(m_nDimensions);
  coeffs[0] = 1;
  for (unsigned int i = 1; i < m_nDimensions; i++)
  {
    coeffs[i] = coeffs[i - 1] * m_dimSizes[i - 1];
  }
  return coeffs;
}

void MDWorkspaceIndexCalculator::setDimensionSize(unsigned int indexOfDimension, int size)
{
  if (indexOfDimension >= m_nDimensions)
  {
    throw std::runtime_error("indexOfDimension is out of bounds");
  }
  m_dimSizes[indexOfDimension] = size;
  if (checkValidSetUp())
  {
    m_coeffs = cacluateCoefficients();
    m_isSetup = true;
  }
}

int MDWorkspaceIndexCalculator::getDimensionSize(unsigned int indexOfDimension) const
{
  if (indexOfDimension >= m_nDimensions)
  {
    throw std::runtime_error("indexOfDimension is out of bounds");
  }
  return m_dimSizes[indexOfDimension];
}


bool MDWorkspaceIndexCalculator::checkValidSetUp() const
{
  for (unsigned int i = 0; i < m_nDimensions; i++)
  {
    if (m_dimSizes[i] == 0)
    {
      return false;
    }
  }
  return true;
}


void MDWorkspaceIndexCalculator::checkValidIndexesProvided(const VecIndexes& indexes) const
{
  if (!m_isSetup)
  {
    throw std::runtime_error("MDWorkspaceIndexCalculator:: Not all dimensions have sizes set.");
  }
  if (indexes.size() != m_nDimensions)
  {
    throw std::range_error("Incorrect number of indexes provided");
  }
  for (unsigned int i = 0; i < m_nDimensions; i++)
  {
    if (indexes[i] >= m_dimSizes[i])
    {
      throw std::range_error("index provided is out of bounds wrt the dimension on which it is to act.");
    }
  }
}


size_t MDWorkspaceIndexCalculator::calculateSingleDimensionIndex(const VecIndexes& indexes) const
{
  checkValidIndexesProvided(indexes);
  size_t singleDimensionalIndex = 0;
  // = i + ni*j + ni*nj*k + ....+
  for (unsigned int i = 0; i < m_nDimensions; i++)
  {
    singleDimensionalIndex += m_coeffs[i] * indexes[i];
  }
  return singleDimensionalIndex;
}


VecIndexes MDWorkspaceIndexCalculator::calculateDimensionIndexes(
    std::size_t singleDimensionIndex) const
{
  std::vector<size_t> result(m_nDimensions);
  calculateDimensionIndexes(singleDimensionIndex,result);
  return result;
}

void MDWorkspaceIndexCalculator::calculateDimensionIndexes(
    std::size_t singleDimensionIndex,VecIndexes& indexes) const
{
  if (indexes.size() != m_nDimensions)
  {
    indexes.resize(m_nDimensions);
  }

  size_t sum = 0;
  for (int i = m_nDimensions - 1; i >= 0; i--)
  {
    // single dimension index - all those already accounted for by higher dimensions
    size_t truncatedDimensionIndex = (singleDimensionIndex - sum);
    indexes[i] = truncatedDimensionIndex / m_coeffs[i]; //Integer division
    sum += indexes[i] * m_coeffs[i];
  }
}


size_t MDWorkspaceIndexCalculator::getIndexUpperBounds() const
{
  if (!m_isSetup)
  {
    throw std::runtime_error("MDWorkspaceIndexCalculator:: Not all dimensions have sizes set.");
  }
  size_t sum = m_dimSizes[0];
  for (unsigned int i = 1; i < m_nDimensions; i++)
  {
    sum *= m_dimSizes[i];
  }
  return sum - 1;
}


} // MDDataObjects
} // Mantid
