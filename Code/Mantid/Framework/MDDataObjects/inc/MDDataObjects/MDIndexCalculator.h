#ifndef MANTID_MDDATAOBJECTS_MDINDEXCALCULATOR_H_
#define MANTID_MDDATAOBJECTS_MDINDEXCALCULATOR_H_

/** Handles calculations involving the translation of single dimensional indexes (used for efficient, dimensional agnostic storage) and conceptual
 *  multidimensional indexes. Peforms inverse calculations. Templated usage for specific dimensinal schenario.

    @author Owen Arnold, RAL ISIS
    @date 25/02/2011

    Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include <vector>
namespace Mantid
{
namespace MDDataObjects
{

typedef std::vector<size_t> VecIndexes;

template<int nDimensions>
class MDWorkspaceIndexCalculator
{
private:

  /// Stores maximum size in each dimension.
  std::vector<unsigned int> m_dimSizes;

  /// Calculate coefficients of form i + aj + ck
  std::vector<int> cacluateCoefficients() const;

public:

  /// Construct calculator.
  MDWorkspaceIndexCalculator();

  /// Set the dimension size limit for a specified index/dimension.
  void setDimensionSize(unsigned int indexOfDimension, int size);

  /// Get the dimension size limit for a specified index/dimension.
  int getDimensionSize(int indexOfDimension) const;

  /// Checks that non-zero dimension sizes have been provided for all the required dimensions.
  void checkValidSetUp() const;

  /// Check that the indexes requested are not out of bounds.
  void checkValidIndexesProvided(const VecIndexes& indexes) const;

  /// Calculate as single dimension index given a set of indexes relating to individual dimensions.
  size_t calculateSingleDimensionIndex(VecIndexes indexes) const;

  /// calculate a set of indexes relating to individual dimensions given a single dimension index.
  VecIndexes calculateDimensionIndexes(std::size_t singleDimensionIndex) const;

  /// In a single dimensional form, get the upper limit for a single dimensional index value.
  size_t getIndexUpperBounds() const;
};

template<int nDimensions>
std::vector<int> MDWorkspaceIndexCalculator<nDimensions>::cacluateCoefficients() const
{
  std::vector<int> coeffs(nDimensions);
  coeffs[0] = 1;
  for (unsigned int i = 1; i < nDimensions; i++)
  {
    coeffs[i] = coeffs[i - 1] * m_dimSizes[i - 1];
  }
  return coeffs;
}

template<int nDimensions>
MDWorkspaceIndexCalculator<nDimensions>::MDWorkspaceIndexCalculator() :
  m_dimSizes(nDimensions, 0)
{
}

template<int nDimensions>
void MDWorkspaceIndexCalculator<nDimensions>::setDimensionSize(unsigned int indexOfDimension, int size)
{
  if (indexOfDimension >= nDimensions)
  {
    throw std::runtime_error("indexOfDimension is out of bounds");
  }
  m_dimSizes[indexOfDimension] = size;
}

template<int nDimensions>
int MDWorkspaceIndexCalculator<nDimensions>::getDimensionSize(int indexOfDimension) const
{
  if (indexOfDimension >= nDimensions)
  {
    throw std::runtime_error("indexOfDimension is out of bounds");
  }
  return m_dimSizes[indexOfDimension];
}

template<int nDimensions>
void MDWorkspaceIndexCalculator<nDimensions>::checkValidSetUp() const
{
  for (int i = 0; i < nDimensions; i++)
  {
    if (m_dimSizes[i] == 0)
    {
      throw std::runtime_error("MDWorkspaceIndexCalculator:: Not all dimensions have sizes set.");
    }
  }
}

template<int nDimensions>
void MDWorkspaceIndexCalculator<nDimensions>::checkValidIndexesProvided(const VecIndexes& indexes) const
{
  checkValidSetUp();
  if (indexes.size() != nDimensions)
  {
    throw std::range_error("Incorrect number of indexes provided");
  }
  for (unsigned int i = 0; i < nDimensions; i++)
  {
    if (indexes[i] >= m_dimSizes[i])
    {
      throw std::range_error("index provided is out of bounds wrt the dimension on which it is to act.");
    }
  }
}

template<int nDimensions>
size_t MDWorkspaceIndexCalculator<nDimensions>::calculateSingleDimensionIndex(VecIndexes indexes) const
{
  checkValidIndexesProvided(indexes);
  std::vector<int> coeffs = cacluateCoefficients();
  size_t singleDimensionalIndex = 0;
  // = i + ni*j + ni*nj*k + ....+
  for (int i = 0; i < nDimensions; i++)
  {
    singleDimensionalIndex += coeffs[i] * indexes[i];
  }
  return singleDimensionalIndex;
}

template<int nDimensions>
VecIndexes MDWorkspaceIndexCalculator<nDimensions>::calculateDimensionIndexes(
    std::size_t singleDimensionIndex) const
{
  std::vector<size_t> result(nDimensions);

  //Calculate coefficients
  std::vector<int> coeffs = cacluateCoefficients();

  size_t sum = 0;
  for (int i = nDimensions - 1; i >= 0; i--)
  {
    // single dimension index - all those already accounted for by higher dimensions
    size_t truncatedDimensionIndex = (singleDimensionIndex - sum);
    result[i] = truncatedDimensionIndex / coeffs[i]; //Integer division
    sum += result[i] * coeffs[i];
  }
  return result;
}

template<int nDimensions>
size_t MDWorkspaceIndexCalculator<nDimensions>::getIndexUpperBounds() const
{
  checkValidSetUp();
  size_t sum = m_dimSizes[0];
  for (int i = 1; i < nDimensions; i++)
  {
    sum *= m_dimSizes[i];
  }
  return sum - 1;
}

}
}

#endif
