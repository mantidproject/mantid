#ifndef MANTID_GEOMETRY_PRODUCTGROUP_H_
#define MANTID_GEOMETRY_PRODUCTGROUP_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Crystal/Group.h"

namespace Mantid
{
namespace Geometry
{

/** ProductGroup :

    ProductGroup objects can be constructed using a string which contains
    a series of symmetry operations S_i, separated by semicolons:

      x,y,z; -x,-y,-z; x,-y,z

    For each symmetry operation, a CyclicGroup G_i object is created. These groups
    are then multiplied to form a ProductGroup:

      P = G_1 * G_2 * G_3 * ... * G_n

    This way, groups with many symmetry operations can be generated from a small
    set of generators. This is for example described in [1].

    [1] Shmueli, U. Acta Crystallogr. A 40, 559–567 (1984).
        http://dx.doi.org/10.1107/S0108767384001161

      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 08/10/2014

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
class MANTID_GEOMETRY_DLL ProductGroup : public Group
{
public:
    ProductGroup(const std::string &generators);
    ProductGroup(const std::vector<Group_const_sptr> &factorGroups);
    virtual ~ProductGroup() { }

protected:
    Group_const_sptr getGeneratedGroup(const std::string &generators) const;
    std::vector<Group_const_sptr> getFactorGroups(const std::vector<SymmetryOperation> &symmetryOperations) const;
    Group_const_sptr getProductGroup(const std::vector<Group_const_sptr> &factorGroups) const;
};


} // namespace Geometry
} // namespace Mantid

#endif  /* MANTID_GEOMETRY_PRODUCTGROUP_H_ */
