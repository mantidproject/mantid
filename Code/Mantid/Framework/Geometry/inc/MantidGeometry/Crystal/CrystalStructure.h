#ifndef MANTID_GEOMETRY_CRYSTALSTRUCTURE_H_
#define MANTID_GEOMETRY_CRYSTALSTRUCTURE_H_

#include "MantidKernel/System.h"
#include "MantidGeometry/Crystal/UnitCell.h"
#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidGeometry/Crystal/PointGroupFactory.h"
#include "MantidGeometry/Crystal/SpaceGroup.h"
#include "MantidGeometry/Crystal/ReflectionCondition.h"
#include "MantidGeometry/Crystal/CompositeBraggScatterer.h"

#include <boost/make_shared.hpp>

namespace Mantid {
namespace Geometry {

/** CrystalStructure :

    Three components are required to describe a crystal structure:

        1. Unit cell metric
        2. Space group
        3. Scatterers in the asymmetric unit

    Representations for all of these components exist in the
    MantidGeometry-library separately and this class combines
    them in order to provide some useful calculations that are
    common when working with crystal structures.

    A common task is to calculate lattice plane spacings for a
    given list of HKL-values. For this calculation, only the unit
    cell has to be known:

        CrystalStructure structure(someUnitCell);

        std::vector<double> dValues = structure.getDValues(hklList);

    Where did 'hklList' come from? For this, CrystalStructure offers
    a method as well. Provided a unit cell, it's possible to get all
    HKLs within a given d range:

        CrystalStructure structure(someUnitCell);
        std::vector<V3D> hklList = structure.getHKLs(0.5, 10.0);

    If the lattice is centered, not all combinations of h, k and l are
    allowed. The first method to provide a centering is to use
    one of the ReflectionCondition-classes directly:

        ReflectionCondition_sptr fCentering =
   boost::make_shared<ReflectionConditionAllFaceCentred>();
        structure.setCentering(fCentering);

    Now, only reflections that fulfill the centering condition are
    returned by getHKLs. For powder diffraction it's not very
    useful to get all reflections, because symmetrically equivalent
    reflections can not be distinguished. Which reflections are
    equivalent is determined by the point group. Again, one possibility
    is to directly assign a point group:

        PointGroup_sptr pointGroup =
   PointGroupFactory::Instance().createPointGroup("m-3m");
        structure.setPointGroup(pointGroup);

        std::vector<V3D> uniqueHKLs = structure.getUniqueHKLs(0.5, 10.0);

    The list uniqueHKLs contains only symmetrically independent
    reflections. Both point group and centering may be provided
    to the constructor of CrystalStructure as well:

        CrystalStructure structure(someUnitCell, pointGroup, centering);

    According to the list given above, this is not a complete description
    of a crystal structure - indeed, the information only describes
    the crystal lattice. As demonstrated by the examples, this is already
    enough information for knowing which reflections are present in
    a given d-range for many cases.

    An example where more information is required can be found in a very
    common crystal structure, the structure of Silicon. Silicon crystallizes
    in the space group Fd-3m (No. 227) and the asymmetric unit consists of
    one Si-atom at the position (1/8, 1/8, 1/8) (for origin choice 2, with the
    inversion at the origin). Looking up this space group in the International
    Tables for Crystallography A reveals that placing a scatterer at this
   position
    introduces a new reflection condition for general reflections hkl:

           h = 2n + 1 (reflections with odd h are allowed)
        or h + k + l = 4n

    This means that for example the reflection family {2 2 2} is not allowed,
    even though the F-centering would allow it. In other words, guessing
   existing
    reflections of silicon only using lattice information does not lead to the
    correct result. Besides that, there are also glide planes in that space
   group,
    which come with additional reflection conditions as well.

    One way to obtain the correct result for this case is to calculate
    structure factors for each HKL and check whether |F|^2 is non-zero. Of
   course,
    to perform this calculation, all three items mentioned in the list at
    the beginning must be present. CrystalStructure offers an additional
    constructor for this purpose:

        CrystalStructure silicon(unitcell, spaceGroup, scatterers);

    After constructing the object like this, it's no longer possible to set
    point group and centering, because this is determined by the space group.
    Now, a different method for checking allowed reflections is available:

        std::vector<V3D> uniqueHKLs = silicon.getUniqueHKLs(0.5, 10.0,
   CrystalStructure::UseStructureFactors);

    By supplying the extra argument, |F|^2 is calculated for each reflection
    and if it's greater than 1e-9, it's considered to be allowed.

      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 05/08/2014

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
class DLLExport CrystalStructure {
public:
  enum ReflectionConditionMethod { UseCentering, UseStructureFactor };

  CrystalStructure(
      const UnitCell &unitCell,
      const PointGroup_sptr &pointGroup =
          PointGroupFactory::Instance().createPointGroup("-1"),
      const ReflectionCondition_sptr &centering =
          ReflectionCondition_sptr(new ReflectionConditionPrimitive));

  CrystalStructure(const UnitCell &unitCell,
                   const SpaceGroup_const_sptr &spaceGroup,
                   const CompositeBraggScatterer_sptr &scatterers);

  virtual ~CrystalStructure() {}

  UnitCell cell() const;
  void setCell(const UnitCell &cell);

  SpaceGroup_const_sptr spaceGroup() const;
  void setSpaceGroup(const SpaceGroup_const_sptr &spaceGroup);

  void setPointGroup(const PointGroup_sptr &pointGroup);
  PointGroup_sptr pointGroup() const;
  PointGroup::CrystalSystem crystalSystem() const;

  void setCentering(const ReflectionCondition_sptr &centering);
  ReflectionCondition_sptr centering() const;

  CompositeBraggScatterer_sptr getScatterers() const;
  void setScatterers(const CompositeBraggScatterer_sptr &scatterers);
  void addScatterers(const CompositeBraggScatterer_sptr &scatterers);

  std::vector<Kernel::V3D>
  getHKLs(double dMin, double dMax,
          ReflectionConditionMethod method = UseCentering) const;
  std::vector<Kernel::V3D>
  getUniqueHKLs(double dMin, double dMax,
                ReflectionConditionMethod method = UseCentering) const;

  std::vector<double> getDValues(const std::vector<Kernel::V3D> &hkls) const;
  std::vector<double> getFSquared(const std::vector<Kernel::V3D> &hkls) const;

protected:
  void setPointGroupFromSpaceGroup(const SpaceGroup_const_sptr &spaceGroup);
  void
  setReflectionConditionFromSpaceGroup(const SpaceGroup_const_sptr &spaceGroup);

  void assignSpaceGroupToScatterers(const SpaceGroup_const_sptr &spaceGroup);
  void assignUnitCellToScatterers(const UnitCell &unitCell);

  void initializeScatterers();

  bool
  isStateSufficientForHKLGeneration(ReflectionConditionMethod method) const;
  bool isStateSufficientForUniqueHKLGeneration(
      ReflectionConditionMethod method) const;

  void throwIfRangeUnacceptable(double dMin, double dMax) const;

  bool isAllowed(const Kernel::V3D &hkl,
                 ReflectionConditionMethod method) const;

  double getDValue(const Kernel::V3D &hkl) const;
  double getFSquared(const Kernel::V3D &hkl) const;

  UnitCell m_cell;
  SpaceGroup_const_sptr m_spaceGroup;
  CompositeBraggScatterer_sptr m_scatterers;
  PointGroup_sptr m_pointGroup;
  ReflectionCondition_sptr m_centering;
};

typedef boost::shared_ptr<CrystalStructure> CrystalStructure_sptr;

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_CRYSTALSTRUCTURE_H_ */
