#ifndef MANTID_SINQ_POLDISPECTRUMCONSTANTBACKGROUND_H_
#define MANTID_SINQ_POLDISPECTRUMCONSTANTBACKGROUND_H_

#include "MantidSINQ/DllConfig.h"
#include "MantidCurveFitting/FlatBackground.h"
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
    : public CurveFitting::FlatBackground,
      public IPoldiFunction1D {
public:
  PoldiSpectrumConstantBackground();
  virtual ~PoldiSpectrumConstantBackground();

  virtual std::string name() const { return "PoldiSpectrumConstantBackground"; }

  virtual void setWorkspace(boost::shared_ptr<const API::Workspace> ws);
  size_t getTimeBinCount() const;

  virtual void poldiFunction1D(const std::vector<int> &indices,
                               const API::FunctionDomain1D &domain,
                               API::FunctionValues &values) const;

protected:
  size_t m_timeBinCount;
};

} // namespace Poldi
} // namespace Mantid

#endif /* MANTID_SINQ_POLDISPECTRUMCONSTANTBACKGROUND_H_ */
