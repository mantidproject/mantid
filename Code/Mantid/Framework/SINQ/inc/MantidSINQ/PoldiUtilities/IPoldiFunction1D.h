#ifndef MANTID_SINQ_IPOLDIFUNCTION1D_H_
#define MANTID_SINQ_IPOLDIFUNCTION1D_H_

#include "MantidSINQ/DllConfig.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"

namespace Mantid {
namespace Poldi {

/** IPoldiFunction1D :

    This is an auxilliary interface that has to be implemented by
    functions that are supposed to be used for POLDI fits.

    This way the calculation of a theoretical diffractogram can be
    performed by the corresponding functions, which may behave differently
    depending on their nature. For examples see the following classes:

        PoldiSpectrumConstantBackground
        PoldiSpectrumDomainFunction
        Poldi2DFunction

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
class MANTID_SINQ_DLL IPoldiFunction1D {
public:
  IPoldiFunction1D();
  virtual ~IPoldiFunction1D() {}

  virtual void poldiFunction1D(const std::vector<int> &indices,
                               const API::FunctionDomain1D &domain,
                               API::FunctionValues &values) const = 0;
};

} // namespace Poldi
} // namespace Mantid

#endif /* MANTID_SINQ_IPOLDIFUNCTION1D_H_ */
