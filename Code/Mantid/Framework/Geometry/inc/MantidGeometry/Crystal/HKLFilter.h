#ifndef MANTID_GEOMETRY_HKLFILTER_H_
#define MANTID_GEOMETRY_HKLFILTER_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/V3D.h"

#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace Geometry {

/** HKLFilter


      @author Michael Wedel, ESS
      @date 06/09/2015

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
class MANTID_GEOMETRY_DLL HKLFilter {
public:
  HKLFilter() {}
  virtual ~HKLFilter() {}

  virtual std::string getName() const { return "Base"; }

  bool operator()(const Kernel::V3D &hkl) const { return isAllowed(hkl); }

  virtual bool isAllowed(const Kernel::V3D &hkl) const {
    UNUSED_ARG(hkl);
    return true;
  }
};

typedef boost::shared_ptr<const HKLFilter> HKLFilter_const_sptr;

class MANTID_GEOMETRY_DLL HKLFilterBinaryLogicOperation : public HKLFilter {
public:
  HKLFilterBinaryLogicOperation(const HKLFilter_const_sptr &lhs, const HKLFilter_const_sptr &rhs)
      : HKLFilter(), m_lhs(lhs), m_rhs(rhs) {}
  virtual ~HKLFilterBinaryLogicOperation() {}

  HKLFilter_const_sptr getLHS() const { return m_lhs; }
  HKLFilter_const_sptr getRHS() const { return m_rhs; }

protected:
  HKLFilter_const_sptr m_lhs;
  HKLFilter_const_sptr m_rhs;
};

class MANTID_GEOMETRY_DLL HKLFilterAnd : public HKLFilterBinaryLogicOperation {
public:
  HKLFilterAnd(const HKLFilter_const_sptr &lhs, const HKLFilter_const_sptr &rhs)
      : HKLFilterBinaryLogicOperation(lhs, rhs) {}

  std::string getName() const { return "AND"; }

  bool isAllowed(const Kernel::V3D &hkl) const {
    return m_lhs->isAllowed(hkl) && m_rhs->isAllowed(hkl);
  }
};

MANTID_GEOMETRY_DLL HKLFilter_const_sptr
operator&(const HKLFilter_const_sptr &lhs, const HKLFilter_const_sptr &rhs);

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_HKLFILTER_H_ */
