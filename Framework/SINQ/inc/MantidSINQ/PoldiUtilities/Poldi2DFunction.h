#ifndef MANTID_SINQ_POLDI2DFUNCTION_H_
#define MANTID_SINQ_POLDI2DFUNCTION_H_

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IFunction1DSpectrum.h"
#include "MantidKernel/System.h"
#include "MantidSINQ/DllConfig.h"
#include "MantidSINQ/PoldiUtilities/IPoldiFunction1D.h"

namespace Mantid {
namespace Poldi {

/** Poldi2DFunction :

    Function for POLDI 2D spectrum. It implements CompositeFunction in order to
    combine functions for different peaks and IFunction1DSpectrum so that Fit
    is able to choose the correct domain creator for it.

      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 13/06/2014

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
class MANTID_SINQ_DLL Poldi2DFunction : public API::IFunction1DSpectrum,
                                        public API::CompositeFunction,
                                        public IPoldiFunction1D {
public:
  Poldi2DFunction();

  void function(const API::FunctionDomain &domain,
                API::FunctionValues &values) const override;
  void functionDeriv(const API::FunctionDomain &domain,
                     API::Jacobian &jacobian) override;

  void function1DSpectrum(const API::FunctionDomain1DSpectrum &domain,
                          API::FunctionValues &values) const override;

  void poldiFunction1D(const std::vector<int> &indices,
                       const API::FunctionDomain1D &domain,
                       API::FunctionValues &values) const override;

  void iterationFinished() override;

private:
  size_t m_iteration;
};

using Poldi2DFunction_sptr = boost::shared_ptr<Poldi2DFunction>;

} // namespace Poldi
} // namespace Mantid

#endif /* MANTID_SINQ_POLDI2DFUNCTION_H_ */
