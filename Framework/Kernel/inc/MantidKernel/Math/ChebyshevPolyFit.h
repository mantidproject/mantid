#ifndef MANTID_KERNEL_CHEBYSHEVPOLYFIT_H_
#define MANTID_KERNEL_CHEBYSHEVPOLYFIT_H_
/*
  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include "MantidKernel/DllConfig.h"
#include <memory>
#include <vector>

namespace Mantid {
namespace Kernel {
// Keep the implementation out of here
class ChebyshevPolyFitImpl;

/**
  Compute a weighted least-squares polynomial approximations
  to an arbitrary set of data points. Each polynomial is represented in
  Chebyshev-series form. Equivalent to the NAG routine E02ADF -
  http://www.nag.co.uk/numeric/fl/manual/pdf/E02/e02adf.pdf
*/
class MANTID_KERNEL_DLL ChebyshevPolyFit {
public:
  ChebyshevPolyFit(const size_t n);
  // Implemented in cpp so a unique_ptr member doesn't need to
  // see full ChebyshevPolyFitPrivate implementation
  ~ChebyshevPolyFit();

  std::vector<double> operator()(const std::vector<double> &xs,
                                 const std::vector<double> &ys,
                                 const std::vector<double> &wgts);

private:
  std::unique_ptr<ChebyshevPolyFitImpl> m_impl;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_CHEBYSHEVPOLYFIT_H_ */
