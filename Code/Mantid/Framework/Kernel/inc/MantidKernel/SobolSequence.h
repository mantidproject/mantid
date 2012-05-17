#ifndef MANTID_KERNEL_SOBOLSEQUENCE_H_
#define MANTID_KERNEL_SOBOLSEQUENCE_H_
/**
  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidKernel/QuasiRandomNumberSequence.h"
#include "MantidKernel/ClassMacros.h"
#include <gsl/gsl_qrng.h>

//------------------------------------------------------------------------------
// Forward declarations
//------------------------------------------------------------------------------

namespace Mantid
{
  namespace Kernel
  {
    /**
     *
     * Defines a generator that produces quasi-random numbers according
     * to a Sobol sequence http://en.wikipedia.org/wiki/Sobol_sequence
     */
    class MANTID_KERNEL_DLL SobolSequence : public QuasiRandomNumberSequence
    {
    public:
      /// Constructor taking the number of dimensions
      explicit SobolSequence(const unsigned int ndims);
      /// Destructor
      ~SobolSequence();
      /// Returns the next value in the sequence
      std::vector<double> nextPoint();
      /// Reset the sequence
      void restart();

    private:
      DISABLE_DEFAULT_CONSTRUCT(SobolSequence);
      DISABLE_COPY_AND_ASSIGN(SobolSequence);

      /// Set the number of dimensions
      void setNumberOfDimensions(const unsigned int ndims);
      /// Frees resources allocated by current generator
      void deleteCurrentGenerator();

      /// GSL quasi-random number state
      gsl_qrng *m_gslGenerator;
      /// Number of dimensions of current generator
      unsigned int m_numDims;
      /// A n-dimensional vector holding the current coordinate points
      std::vector<double> m_currentPoint;
    };
  }
}

#endif /* MANTID_KERNEL_SOBOLSEQUENCE_H_ */
