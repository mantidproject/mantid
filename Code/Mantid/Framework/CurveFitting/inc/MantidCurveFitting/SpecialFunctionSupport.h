#ifndef SPECIALFUNCTIONSUPPORT_H_
#define SPECIALFUNCTIONSUPPORT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include <vector>
#include <functional>
#include <cmath>
#include <complex>

namespace Mantid {
namespace CurveFitting {
/*
    Special functions used as part of peak shape functions that are not
    included in the Gnu Scientific Lib or other C/C++ open source code projects

    @author Anders J Markvardsen, Rutherford Appleton Laboratory
    @date 30/09/2009

    Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
namespace SpecialFunctionSupport {
/// Compute exp(z)*E1(z) where z is complex and E1(z) is the Exponential
/// Integral
std::complex<double> DLLExport
    exponentialIntegral(const std::complex<double> &z);

} // namespace SpecialFunctionSupport
} // namespace CurveFitting
} // namespace Mantid

#endif /*SPECIALFUNCTIONSUPPORT_H_*/
