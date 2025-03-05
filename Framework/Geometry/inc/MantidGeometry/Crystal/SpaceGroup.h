// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
  */
class MANTID_GEOMETRY_DLL SpaceGroup : public Group {
public:
  SpaceGroup(size_t itNumber, std::string hmSymbol, const Group &group);

  size_t number() const;
  const std::string &hmSymbol() const;

  template <typename T> std::vector<T> getEquivalentPositions(const T &position) const {
    const std::vector<SymmetryOperation> &symmetryOperations = getSymmetryOperations();

    std::vector<T> equivalents;
    equivalents.reserve(symmetryOperations.size());
    std::transform(symmetryOperations.cbegin(), symmetryOperations.cend(), std::back_inserter(equivalents),
                   [&position](const SymmetryOperation &op) { return Geometry::getWrappedVector(op * position); });

    // Use fuzzy compare with the same condition as V3D::operator==().
    std::sort(equivalents.begin(), equivalents.end(), AtomPositionsLessThan());
    equivalents.erase(std::unique(equivalents.begin(), equivalents.end(), AtomPositionsEqual()), equivalents.end());

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

MANTID_GEOMETRY_DLL std::ostream &operator<<(std::ostream &stream, const SpaceGroup &self);

using SpaceGroup_sptr = std::shared_ptr<SpaceGroup>;
using SpaceGroup_const_sptr = std::shared_ptr<const SpaceGroup>;

} // namespace Geometry
} // namespace Mantid
