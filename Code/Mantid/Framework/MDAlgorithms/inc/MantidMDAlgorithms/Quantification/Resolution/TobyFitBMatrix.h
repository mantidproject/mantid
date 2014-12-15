#ifndef MANTID_MDALGORITHMS_RESOLUTIONCOEFFICIENTS_H_
#define MANTID_MDALGORITHMS_RESOLUTIONCOEFFICIENTS_H_
/**
  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

  File change history is stored at: <https://github.com/mantidproject/mantid>.
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#include "MantidKernel/Matrix.h"

namespace Mantid
{
  namespace MDAlgorithms
  {
    //-------------------------------------------------------------------------
    // Forward declarations
    //-------------------------------------------------------------------------
    class CachedExperimentInfo;
    struct QOmegaPoint;

    /**
     * Defines the linear transformation from vector of
     * independent integration functions to random
     * integration variables. Defined in Toby G Perring's thesis
     * pg 112, equation A.48. It is intimately linked to the
     * TobyFitYVector as their values need to be in sync
     */
    class DLLExport TobyFitBMatrix : public Kernel::DblMatrix
    {
    public:
      /// Default constructor sets the size of the matrix
      TobyFitBMatrix();

      /// Calculate the values for this observation & QDeltaE point
      void recalculate(const CachedExperimentInfo & observation,
                       const QOmegaPoint & qOmega);

    };

  }
}

#endif /* MANTID_MDALGORITHMS_RESOLUTIONCOEFFICIENTS_H_ */
