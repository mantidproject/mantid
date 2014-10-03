#ifndef MANTID_GEOMETRY_CYCLICGROUP_H_
#define MANTID_GEOMETRY_CYCLICGROUP_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Crystal/Group.h"

#include <boost/shared_ptr.hpp>
#include <set>

namespace Mantid
{
namespace Geometry
{

/** CyclicGroup :

    A class that represents a cyclic group of symmetry operations.
    It just constructs a Group by multiplying itself "order" times.

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
class MANTID_GEOMETRY_DLL CyclicGroup : public Group
{
public:
    CyclicGroup(const SymmetryOperation &symmetryOperation);
    virtual ~CyclicGroup() { }

    static Group_const_sptr create(const std::string &symmetryOperation);

protected:
    std::vector<SymmetryOperation> generateAllOperations(const SymmetryOperation &operation) const;

};

typedef boost::shared_ptr<CyclicGroup> CyclicGroup_sptr;
typedef boost::shared_ptr<const CyclicGroup> CyclicGroup_const_sptr;


} // namespace Geometry
} // namespace Mantid

#endif  /* MANTID_GEOMETRY_CYCLICGROUP_H_ */
