#ifndef MANTID_GEOMETRY_SPACEGROUP_H_
#define MANTID_GEOMETRY_SPACEGROUP_H_

#include "MantidGeometry/Crystal/Group.h"
#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidGeometry/Crystal/SymmetryOperation.h"
#include "MantidGeometry/Crystal/UnitCell.h"
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/V3D.h"

#include <set>

namespace Mantid {
namespace Geometry {

/**
    @class SpaceGroup

    A class for representing space groups, inheriting from Group.

    SpaceGroup-objects represent a space group, which is a set
    of symmetry operations. Along with storing the operations themselves,
    which is realized through inheriting from Group, SpaceGroup also
    stores a number (space group number according to the International
    Tables for Crystallography A) and a Hermann-Mauguin-symbol.

    SpaceGroup may for example be used to generate all equivalent positions
    within the unit cell:

        SpaceGroup_const_sptr group;

        V3D position(0.13, 0.54, 0.38);
        std::vector<V3D> equivalents = group->getEquivalentPositions(position);

    The class should not be instantiated directly, see SpaceGroupFactoryImpl
    instead.

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
class MANTID_GEOMETRY_DLL SpaceGroup : public Group {
public:
  SpaceGroup(size_t itNumber, const std::string &hmSymbol, const Group &group);

  size_t number() const;
  std::string hmSymbol() const;

  template <typename T>
  std::vector<T> getEquivalentPositions(const T &position) const {
    const std::vector<SymmetryOperation> &symmetryOperations =
        getSymmetryOperations();

    std::vector<T> equivalents;
    equivalents.reserve(symmetryOperations.size());
    std::transform(symmetryOperations.cbegin(), symmetryOperations.cend(),
                   std::back_inserter(equivalents),
                   [&position](const SymmetryOperation &op) {
                     return Geometry::getWrappedVector(op * position);
                   });

    // Use fuzzy compare with the same condition as V3D::operator==().
    std::sort(equivalents.begin(), equivalents.end(), AtomPositionsLessThan());
    equivalents.erase(std::unique(equivalents.begin(), equivalents.end(),
                                  AtomPositionsEqual()),
                      equivalents.end());

    return equivalents;
  }

  bool isAllowedReflection(const Kernel::V3D &hkl) const;
  bool isAllowedUnitCell(const UnitCell &cell) const;

  PointGroup_sptr getPointGroup() const;
  Group_const_sptr getSiteSymmetryGroup(const Kernel::V3D &position) const;

protected:
  size_t m_number;
  std::string m_hmSymbol;
};

MANTID_GEOMETRY_DLL std::ostream &operator<<(std::ostream &stream,
                                             const SpaceGroup &self);

using SpaceGroup_sptr = boost::shared_ptr<SpaceGroup>;
using SpaceGroup_const_sptr = boost::shared_ptr<const SpaceGroup>;

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_SPACEGROUP_H_ */
