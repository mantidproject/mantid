#ifndef MANTID_KERNEL_BOSEEINSTEINDISTRIBUTION_H_
#define MANTID_KERNEL_BOSEEINSTEINDISTRIBUTION_H_
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
#include "MantidKernel/DllConfig.h"

namespace Mantid
{
  namespace Kernel
  {
    namespace Math
    {
      /**
       * Defines a static class for computing the coefficient from a
       * Bose-Einstein distribution for a given energy in meV & temperature in Kelvin
       */
      class MANTID_KERNEL_DLL BoseEinsteinDistribution
      {
      public:
        /// Calculate the expected number of particles in an energy state at a given temperature
        /// for a degenerate distribution with zero chemical potential
        static double n(const double energy, const double temperature);
        /// Calculate the \f$(n+1)\epsilon\f$ for a degenerate distribution with zero chemical potential
        /// where n is the Bose-Einstein distribution
        static double np1Eps(const double energy, const double temperature);
      };

    }
  }
}

#endif /* MANTID_KERNEL_BOSEEINSTEINDISTRIBUTION_H_ */
