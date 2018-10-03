#ifndef MANTID_SINQ_POLDISPECTRUMLINEARBACKGROUND_H_
#define MANTID_SINQ_POLDISPECTRUMLINEARBACKGROUND_H_

#include "MantidAPI/IFunction1DSpectrum.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidSINQ/DllConfig.h"
#include "MantidSINQ/PoldiUtilities/IPoldiFunction1D.h"

namespace Mantid {
namespace Poldi {

/** PoldiSpectrumLinearBackground :

    A function that is defined like this:

        f(x) = A1 * wi

    where wi is the workspace index and A1 is the only parameter
    of the function. Since it's derived from IFunction1DSpectrum,
    it works only on the proper domain.

        @author Michael Wedel, Paul Scherrer Institut - SINQ
        @date 06/08/2014

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
class MANTID_SINQ_DLL PoldiSpectrumLinearBackground
    : public API::ParamFunction,
      public API::IFunction1DSpectrum,
      public IPoldiFunction1D {
public:
  PoldiSpectrumLinearBackground();

  std::string name() const override { return "PoldiSpectrumLinearBackground"; }

  void setWorkspace(boost::shared_ptr<const API::Workspace> ws) override;
  size_t getTimeBinCount() const;

  void function1DSpectrum(const API::FunctionDomain1DSpectrum &domain,
                          API::FunctionValues &values) const override;
  void functionDeriv1DSpectrum(const API::FunctionDomain1DSpectrum &domain,
                               API::Jacobian &jacobian) override;

  void poldiFunction1D(const std::vector<int> &indices,
                       const API::FunctionDomain1D &domain,
                       API::FunctionValues &values) const override;

protected:
  void init() override;

  size_t m_timeBinCount;
};

} // namespace Poldi
} // namespace Mantid

#endif /* MANTID_SINQ_POLDISPECTRUMLINEARBACKGROUND_H_ */
