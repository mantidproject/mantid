#ifndef MANTID_SINQ_MILLERINDICES_H
#define MANTID_SINQ_MILLERINDICES_H

#include "MantidSINQ/DllConfig.h"
#include "MantidKernel/V3D.h"
#include <vector>

namespace Mantid {
namespace Poldi {

/** MillerIndices :
 *
  Small helper class which holds Miller indices for use with other
  POLDI routines.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 14/03/2014

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

class MANTID_SINQ_DLL MillerIndices {
public:
  MillerIndices(int h = 0, int k = 0, int l = 0);
  MillerIndices(std::vector<int> hkl);
  MillerIndices(const Kernel::V3D &hkl);
  ~MillerIndices() {}

  int h() const;
  int k() const;
  int l() const;

  int operator[](int index);
  bool operator==(const MillerIndices &other) const;
  bool operator!=(const MillerIndices &other) const;

  const std::vector<int> &asVector() const;
  const Kernel::V3D &asV3D() const;

private:
  void populateVector();

  int m_h;
  int m_k;
  int m_l;

  std::vector<int> m_asVector;
  Kernel::V3D m_asV3D;
};
}
}

#endif // MANTID_SINQ_MILLERINDICES_H
