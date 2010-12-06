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
   
    class DLLExport MatrixWSIndexCalculator
    {
    private:

      int m_blockSize;

    public:

      MatrixWSIndexCalculator(int blockSize);

      int getHistogramIndex(int index);

      int getBinIndex(int index, int histogram);
    };

  }
}

#endif
