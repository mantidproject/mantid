
#ifndef MANTID_MDALGORITHMS_MAGNETICFORMFACTOR_H_
#define MANTID_MDALGORITHMS_MAGNETICFORMFACTOR_H_

#include "MantidKernel/System.h"
#include <math.h>
#include <vector>

namespace Mantid
{
    namespace MDAlgorithms
    {
        /**
        Class to provide Magnetic form factor for Tobyfit. A lookup table is used in the same way that Tobyfit does.
        This seems to be more efficient than direct computation with slightly less accuracy.

        Constructor takes the atomic number, ionisation level and table size to create.
        Provides a method to look up the form factor for a given value of q-squared
        Also provides the direct computation through the method form(qsqu).

        @date 22/02/2012

        Copyright &copy; 2007-12 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

        File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
        Code Documentation is available at: <http://doxygen.mantidproject.org>
        */
        class DLLExport MagneticFormFactor
        {
        public:
            /// Constructor
            MagneticFormFactor(const int atomicNo, const int ionisation, const int tableSize);
            virtual ~MagneticFormFactor() {}

            /// Table look up version of form - not yet implemented so calls form instead
            /// In a simple test under Linux the fortran version of formTable was about 2-4 times faster
            /// than the true expression.
            double formTable(const double qSquared) const;
            /// change the form factor to be used and recompute table
            /// @param atomicNo - scattering element
            /// @param ionisation - ionisation of element
            /// @param tableSize - number of points to use in lookup table
            void setFormFactor(const int atomicNo, const int ionisation, const int tableSize);
            /// Magnetic form for given Q Squared value (direct calculation)
            /// @param qSqu - momentum squared
            /// @return form value given by model/ element/ ionisation at qSqu
            double form(const double qSqu) const;
        protected:
            /// get model coefficients for given element ionisation
            void getCoefficients(const int atomicNo, const int ionisation, std::vector<double> & array);
        private:
            std::vector<double> m_formCoeffs;
            /// limit of table lookuo/model range
            static const double m_qSquMax; //=36.*M_PI*M_PI;
            int m_nf;
        };
    }
}
#endif /* MANTID_MDALGORITHMS_MAGNETICFORMFACTOR_H_ */
