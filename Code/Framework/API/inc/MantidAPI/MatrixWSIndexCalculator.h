#ifndef _MATRIX_WS_INDEX_CALCULATOR_H_
#define _MATRIX_WS_INDEX_CALCULATOR_H_

#include "MantidKernel/System.h"

namespace Mantid
{
  namespace API
  {
    /** \class MantidWSIndexCalculator

    Utitlity type to decompose indexes in one dimension to dual indexes used by MatrixWorkspaces.

    \author Owen Arnold Tessella
    \date 18/11/2010

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
   
    /// Denotes index in 1D array.
    typedef int Index;
    /// Denotes index of a particular bin in a histogram.
    typedef int BinIndex;
    /// Denotes index of a particular histogram in the Matrix Workspace.
    typedef int HistogramIndex;

    class DLLExport MatrixWSIndexCalculator
    {
    private:

      int m_blockSize;

    public:

      //Default constructor gives blocksize as 1.
      MatrixWSIndexCalculator();
   
      /// Constructor requires block size for calculation.
      MatrixWSIndexCalculator(int blockSize);

      /// Determine which histogram an index in a 1D array relates to given the known blockSize
      HistogramIndex getHistogramIndex(Index index) const;

      /// Get the bin index.
      BinIndex getBinIndex(Index index, HistogramIndex histogram) const;

      /// Get the one dimensional index given a histogram and bin index.
      Index getOneDimIndex(HistogramIndex histogram, BinIndex binIndex) const;

      ////Assignment operator
      MatrixWSIndexCalculator& operator=(const MatrixWSIndexCalculator& other);

      ////Copy constructor
      MatrixWSIndexCalculator(const MatrixWSIndexCalculator& other);

    };

  }
}

#endif
