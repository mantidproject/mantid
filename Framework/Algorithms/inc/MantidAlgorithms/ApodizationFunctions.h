#ifndef MANTID_ALGORITHM_APODIZATIONFUNCTIONS_H_
#define MANTID_ALGORITHM_APODIZATIONFUNCTIONS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

/*
Contains the functions for calculating apodization,
which can be applied to data prior to a FFT.

@author Anthony Lim
@date 10/08/2017

Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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

namespace Mantid {
namespace Algorithms {
namespace ApodizationFunctions {

double lorentz(double time, double decayConstant);
double gaussian(const double time, const double decayConstant);
double none(const double, const double);
} // namespace ApodizationFunctions
} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_APODIZATIONFUNCTIONS_H_*/
