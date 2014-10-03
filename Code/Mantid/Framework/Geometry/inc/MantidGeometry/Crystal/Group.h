#ifndef MANTID_GEOMETRY_GROUP_H_
#define MANTID_GEOMETRY_GROUP_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Crystal/SymmetryOperation.h"

#include <vector>
#include <set>

#include <boost/shared_ptr.hpp>

namespace Mantid
{
namespace Geometry
{

/** Group :

    A class representing a group of symmetry operations.

      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 03/10/2014

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
class MANTID_GEOMETRY_DLL Group
{
public:
    Group();
    Group(const std::vector<SymmetryOperation> &symmetryOperations);
    Group(const Group &other);
    Group &operator =(const Group &other);

    virtual ~Group() { }

    size_t order() const;
    std::vector<SymmetryOperation> getSymmetryOperations() const;

    Group operator *(const Group &other) const;

    std::vector<Kernel::V3D> operator *(const Kernel::V3D &vector) const;

    bool operator ==(const Group &other) const;
    bool operator !=(const Group &other) const;

    static int m_numOps;

protected:
    void setSymmetryOperations(const std::vector<SymmetryOperation> &symmetryOperations);

    std::vector<SymmetryOperation> m_allOperations;
    std::set<SymmetryOperation> m_operationSet;
};

typedef boost::shared_ptr<Group> Group_sptr;
typedef boost::shared_ptr<const Group> Group_const_sptr;

MANTID_GEOMETRY_DLL Group_const_sptr operator *(const Group_const_sptr &lhs, const Group_const_sptr &rhs);
MANTID_GEOMETRY_DLL std::vector<Kernel::V3D> operator *(const Group_const_sptr &lhs, const Kernel::V3D &rhs);
MANTID_GEOMETRY_DLL bool operator ==(const Group_const_sptr &lhs, const Group_const_sptr &rhs);
MANTID_GEOMETRY_DLL bool operator !=(const Group_const_sptr &lhs, const Group_const_sptr &rhs);


} // namespace Geometry
} // namespace Mantid

#endif  /* MANTID_GEOMETRY_GROUP_H_ */
