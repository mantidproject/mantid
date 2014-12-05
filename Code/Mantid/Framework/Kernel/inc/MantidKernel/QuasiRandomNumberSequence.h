#ifndef MANTID_KERNEL_QUASIRANDOMNUMBERSEQUENCE_H_
#define MANTID_KERNEL_QUASIRANDOMNUMBERSEQUENCE_H_
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
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidKernel/NDRandomNumberGenerator.h"

namespace Mantid
{
  namespace Kernel
  {
    /**
     *
     * Defines an interface to a quasi-random number sequence. A quasi-random sequence
     * progressively covers a d-dimensional space with a set of points that are
     * uniformly distributed.
     */
    class MANTID_KERNEL_DLL QuasiRandomNumberSequence : public NDRandomNumberGenerator
    {
    public:
      QuasiRandomNumberSequence(const unsigned int ndims)
        : NDRandomNumberGenerator(ndims)
      {}

    };
  }
}

#endif /* MANTID_KERNEL_QUASIRANDOMNUMBERSEQUENCE_H_ */
