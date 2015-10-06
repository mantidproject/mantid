#ifndef MANTID_API_IFUNCTION1DSPECTRUM_H_
#define MANTID_API_IFUNCTION1DSPECTRUM_H_

#include "MantidAPI/DllConfig.h"

#include "MantidAPI/IFunction.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidKernel/Logger.h"

namespace Mantid {
namespace API {

/** IFunction1DSpectrum :

    IFunction1DSpectrum is very similar to IFunction1D, it just builds on
    FunctionDomain1DSpectrum, which is more specialized than FunctionDomain1D.

      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 03/06/2014

    Copyright © 2014 PSI-MSS

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
class DLLExport IFunction1DSpectrum : public virtual IFunction {
public:
  virtual ~IFunction1DSpectrum() {}

  virtual void function(const FunctionDomain &domain,
                        FunctionValues &values) const;
  virtual void functionDeriv(const FunctionDomain &domain, Jacobian &jacobian);

  /// Provide a concrete function in an implementation that operates on a
  /// FunctionDomain1DSpectrum.
  virtual void function1DSpectrum(const FunctionDomain1DSpectrum &domain,
                                  FunctionValues &values) const = 0;

  /// Derivatives of the function. The base implementation calculates numerical
  /// derivatives.
  virtual void functionDeriv1DSpectrum(const FunctionDomain1DSpectrum &domain,
                                       Jacobian &jacobian);

protected:
  static Kernel::Logger g_log;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_IFUNCTION1DSPECTRUM_H_ */
