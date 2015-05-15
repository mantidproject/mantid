#ifndef MANTID_API_LATTICEDOMAIN_H_
#define MANTID_API_LATTICEDOMAIN_H_

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/FunctionDomain.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace API {

/** LatticeDomain

  This domain stores V3D-objects as HKLs instead of double-values. It can be
  used to refine lattice parameters from HKL/d-pairs.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 15/04/2015

  Copyright © 2015 PSI-NXMM

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
class MANTID_API_DLL LatticeDomain : public FunctionDomain {
public:
  LatticeDomain(const std::vector<Kernel::V3D> &hkls);
  virtual ~LatticeDomain() {}

  size_t size() const;

  const Kernel::V3D &operator[](size_t i) const;

protected:
  std::vector<Kernel::V3D> m_hkls;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_LATTICEDOMAIN_H_ */
