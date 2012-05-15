#ifndef MANTID_KERNEL_NDPSEUDORANDOMNUMBERGENERATOR_H_
#define MANTID_KERNEL_NDPSEUDORANDOMNUMBERGENERATOR_H_
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
#include "MantidKernel/PseudoRandomNumberGenerator.h"
#include "MantidKernel/ClassMacros.h"
#include <boost/shared_ptr.hpp>

namespace Mantid
{
  namespace Kernel
  {
    /**
     *
     * Defines an ND pseudo-random number generator. This uses a single
     * 1D pseudo-random number generator to produce ND random values
     */
    class MANTID_KERNEL_DLL NDPseudoRandomNumberGenerator : public NDRandomNumberGenerator
    {
      /// The type for generating a single random number
      typedef boost::shared_ptr<PseudoRandomNumberGenerator> SingleValueGenerator;
    public:
      /// Constructor
      NDPseudoRandomNumberGenerator(const unsigned int ndims, SingleValueGenerator singleValueGen);
      /// Set the random number seed
      void setSeed(const size_t seedValue);
      /// Returns the ND point
      std::vector<double> nextPoint();
      /// Resets the generator
      void restart();

    private:
      DISABLE_DEFAULT_CONSTRUCT(NDPseudoRandomNumberGenerator);
      DISABLE_COPY_AND_ASSIGN(NDPseudoRandomNumberGenerator);

      /// The number of dimensions
      const unsigned int m_ndims;
      /// The single value generator
      SingleValueGenerator m_singleValueGen;
    };
  }
}

#endif /* MANTID_KERNEL_NDPSEUDORANDOMNUMBERGENERATOR_H_ */
