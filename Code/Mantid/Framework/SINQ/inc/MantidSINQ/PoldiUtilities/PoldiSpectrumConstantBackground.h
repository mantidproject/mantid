#ifndef MANTID_SINQ_POLDISPECTRUMCONSTANTBACKGROUND_H_
#define MANTID_SINQ_POLDISPECTRUMCONSTANTBACKGROUND_H_

#include "MantidSINQ/DllConfig.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidSINQ/PoldiUtilities/IPoldiFunction1D.h"

namespace Mantid {
namespace Poldi {

/** PoldiSpectrumConstantBackground

    The function inherits from FlatBackground and also implements the
    IPoldiFunction1D interface.

        @author Michael Wedel, Paul Scherrer Institut - SINQ
        @date 07/01/2015

    Copyright © 2015 PSI-MSS

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
class MANTID_SINQ_DLL PoldiSpectrumConstantBackground
    : public API::ParamFunction,
      public API::IFunction1D,
      public IPoldiFunction1D {
public:
  PoldiSpectrumConstantBackground();
  virtual ~PoldiSpectrumConstantBackground();

  virtual std::string name() const { return "PoldiSpectrumConstantBackground"; }

  void function1D(double *out, const double *xValues, const size_t nData) const;
  void functionDeriv1D(API::Jacobian *out, const double *xValues,
                       const size_t nData);

  virtual void setWorkspace(boost::shared_ptr<const API::Workspace> ws);
  size_t getTimeBinCount() const;

  virtual void poldiFunction1D(const std::vector<int> &indices,
                               const API::FunctionDomain1D &domain,
                               API::FunctionValues &values) const;

  void setParameter(const std::string &name, const double &value, bool explicitlySet = true);
  void setParameter(size_t i, const double &value, bool explicitlySet = true);

  double getParameter(const std::string &name) const;
  double getParameter(size_t i) const;

protected:
  void init();

  size_t m_timeBinCount;
  API::IFunction1D_sptr m_flatBackground;
};

} // namespace Poldi
} // namespace Mantid

#endif /* MANTID_SINQ_POLDISPECTRUMCONSTANTBACKGROUND_H_ */
