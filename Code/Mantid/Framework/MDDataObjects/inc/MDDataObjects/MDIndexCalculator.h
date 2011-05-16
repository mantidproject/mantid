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
#include "MDDataObjects/DllExport.h"
#include <vector>
namespace Mantid
{
namespace MDDataObjects
{

typedef std::vector<size_t> VecIndexes;

class EXPORT_OPT_MANTID_MDDATAOBJECTS MDWorkspaceIndexCalculator
{
private:
 /// 
  bool m_isSetup;

  /// Stores the number of dimensions == m_dimSizes.size()
  unsigned int m_nDimensions;
 
  /// Stores maximum size in each dimension.
  std::vector<unsigned int> m_dimSizes;

  /// Cached coefficients
  std::vector<int> m_coeffs;

  /// Calculate coefficients of form i + aj + ck
  std::vector<int> cacluateCoefficients() const;

  /// Checks that non-zero dimension sizes have been provided for all the required dimensions.
  bool checkValidSetUp() const;

public:

  /// Construct calculator.
  MDWorkspaceIndexCalculator(unsigned int nDimensions, int size1 = -1, int size2 = -1, int size3 = -1, int size4 = -1);

  /// Set the dimension size limit for a specified index/dimension.
  void setDimensionSize(unsigned int indexOfDimension, int size);

  /// Get the dimension size limit for a specified index/dimension.
  int getDimensionSize(unsigned int indexOfDimension) const;

  /// Checks that non-zero dimension sizes have been provided for all the required dimensions.
  bool isValid() const{return m_isSetup;}

  /// Check that the indexes requested are not out of bounds.
  void checkValidIndexesProvided(const VecIndexes& indexes) const;

  /// Calculate as single dimension index given a set of indexes relating to individual dimensions.
  size_t calculateSingleDimensionIndex(const VecIndexes& indexes) const;

  /// calculate a set of indexes relating to individual dimensions given a single dimension index.
  VecIndexes calculateDimensionIndexes(std::size_t singleDimensionIndex) const;

  /// calculate a set of indexes relating to individual dimensions given a single dimension index.
  void calculateDimensionIndexes(std::size_t singleDimensionIndex,VecIndexes&) const;

  /// In a single dimensional form, get the upper limit for a single dimensional index value.
  size_t getIndexUpperBounds() const;

  /// Get number of dimensions
  size_t getNDimensions()const{return m_nDimensions;}

  /// Assignment operator
  MDWorkspaceIndexCalculator&  operator=(const MDWorkspaceIndexCalculator&);

  /// Copy constructor
  MDWorkspaceIndexCalculator(const MDWorkspaceIndexCalculator&);

};


}
}

#endif
