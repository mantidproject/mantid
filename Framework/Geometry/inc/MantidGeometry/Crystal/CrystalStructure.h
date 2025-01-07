// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Crystal/CompositeBraggScatterer.h"
#include "MantidGeometry/Crystal/HKLFilter.h"
#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidGeometry/Crystal/PointGroupFactory.h"
#include "MantidGeometry/Crystal/ReflectionCondition.h"
#include "MantidGeometry/Crystal/SpaceGroup.h"
#include "MantidGeometry/Crystal/UnitCell.h"
#include "MantidGeometry/DllConfig.h"

#include <memory>

namespace Mantid {
namespace Geometry {

// Forward declaration
class StructureFactorCalculator;

/**
    @class CrystalStructure

    Three components are required to describe a crystal structure:

        1. Unit cell metric
        2. Space group
        3. Scatterers in the asymmetric unit

    Representations for all of these components exist in the
    MantidGeometry-library separately and this class combines
    them in order to provide some useful calculations that are
    common when working with crystal structures.

    Besides construction from actual objects, a CrystalStructure object can be
    constructed from three strings that contain the corresponding information.

    The unit cell string must consist of 3 or 6 floating point numbers, which
    are 3 lenghts in Angström and 3 angles in degree. If the angles are not
    supplied, they are assumed to be 90 degrees.

    The space group string must be a valid space group that is registered into
    the factory. Lastly, specification of the atoms in the asymmetric unit is
    required. The format is as follows:

        Element x y z Occupancy U_iso;
        Element x y z Occupancy U_iso;
        ...

    Element has to be a valid NeutronAtom, x, y and z are fractional coordinates
    between 0 and 1 (other coordinates will be transformed to that range). It is
    allowed (and encouraged) to use proper fractions in the coordinates, for
    example:

        Mg 1/3 2/3 1/4 1.0 0.05;

    Occupancy must be given as values between 0 and 1 and U_iso is the isotropic
    thermal displacement parameter, given in Angrström^2.

      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 05/08/2014
  */
class MANTID_GEOMETRY_DLL CrystalStructure {
public:
  CrystalStructure(const UnitCell &unitCell, const SpaceGroup_const_sptr &spaceGroup,
                   const CompositeBraggScatterer_sptr &scatterers);

  CrystalStructure(const std::string &unitCellString, const std::string &spaceGroupString,
                   const std::string &scattererString);

  UnitCell cell() const;
  void setCell(const UnitCell &cell);

  SpaceGroup_const_sptr spaceGroup() const;
  void setSpaceGroup(const SpaceGroup_const_sptr &spaceGroup);

  ReflectionCondition_sptr centering() const { return m_centering; }

  CompositeBraggScatterer_sptr getScatterers() const;
  void setScatterers(const CompositeBraggScatterer_sptr &scatterers);
  void addScatterers(const CompositeBraggScatterer_sptr &scatterers);

protected:
  void assignUnitCellToScatterers(const UnitCell &unitCell);

  void setReflectionConditionFromSpaceGroup(const SpaceGroup_const_sptr &spaceGroup);

  void initializeScatterers();

  UnitCell m_cell;
  SpaceGroup_const_sptr m_spaceGroup;
  ReflectionCondition_sptr m_centering;
  CompositeBraggScatterer_sptr m_scatterers;
};

using CrystalStructure_sptr = std::shared_ptr<CrystalStructure>;

} // namespace Geometry
} // namespace Mantid
