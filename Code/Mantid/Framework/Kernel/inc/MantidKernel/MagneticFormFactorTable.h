#ifndef MANTID_KERNEL_MAGNETICFORMFACTORTABLE_H_
#define MANTID_KERNEL_MAGNETICFORMFACTORTABLE_H_

#include "MantidKernel/ClassMacros.h"
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/MagneticIon.h"

#include <vector>

namespace Mantid
{
  namespace PhysicalConstants
  {

    /**
      Implements a cached lookup table for a magnetic form factor. The table is
      created for a given MagneticIon.

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

      File change history is stored at: <https://github.com/mantidproject/mantid>
      Code Documentation is available at: <http://doxygen.mantidproject.org>
     */
    class MANTID_KERNEL_DLL MagneticFormFactorTable
    {
    public:
      /// Construct the table around an ion
      MagneticFormFactorTable(const size_t length, const MagneticIon & ion,
                              const uint16_t j = 0, const uint16_t l = 0);

      /// Returns an interpolated form factor for the given \f$Q^2\f$ value
      double value(const double qsqr) const;

    private:
      DISABLE_DEFAULT_CONSTRUCT(MagneticFormFactorTable);
      DISABLE_COPY_AND_ASSIGN(MagneticFormFactorTable);

      /// Setup the table with the values
      void setup(const MagneticIon & ion, const uint16_t j, const uint16_t l);

      /// Cache the size
      size_t m_length;
      /// The actual table of values
      std::vector<double> m_lookup;
      /// Point spacing
      double m_delta;
    };


  } // namespace PhysicalConstants
} // namespace Mantid

#endif  /* MANTID_KERNEL_MAGNETICFORMFACTORTABLE_H_ */
