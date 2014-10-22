#ifndef MANTID_GEOMETRY_CRYSTALSTRUCTURE_H_
#define MANTID_GEOMETRY_CRYSTALSTRUCTURE_H_

#include "MantidKernel/System.h"
#include "MantidGeometry/Crystal/UnitCell.h"
#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidGeometry/Crystal/PointGroupFactory.h"
#include "MantidGeometry/Crystal/SpaceGroup.h"
#include "MantidGeometry/Crystal/ReflectionCondition.h"
#include "MantidGeometry/Crystal/CompositeScatterer.h"

#include <boost/make_shared.hpp>

namespace Mantid
{
namespace Geometry
{

/** CrystalStructure :

    This is a very basic class for holding the parts of a crystal structure.
    Since not all components are present in Mantid (yet), it currently
    comprises a UnitCell, a PointGroup and a ReflectionCondition-object to
    reflect lattice centering.
    
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
class DLLExport CrystalStructure
{
public:
    enum ReflectionConditionMethod {
        UseCentering,
        UseStructureFactor
    };

    CrystalStructure(const UnitCell &unitCell,
                     const PointGroup_sptr &pointGroup = PointGroupFactory::Instance().createPointGroup("-1"),
                     const ReflectionCondition_sptr &centering = boost::make_shared<ReflectionConditionPrimitive>());

    CrystalStructure(const UnitCell &unitCell,
                     const SpaceGroup_const_sptr &spaceGroup,
                     const CompositeScatterer_sptr &scatterers);

    virtual ~CrystalStructure() { }

    UnitCell cell() const;
    void setCell(const UnitCell &cell);

    SpaceGroup_const_sptr spaceGroup() const;
    void setSpaceGroup(const SpaceGroup_const_sptr &spaceGroup);

    void setPointGroup(const PointGroup_sptr &pointGroup);
    PointGroup_sptr pointGroup() const;
    PointGroup::CrystalSystem crystalSystem() const;

    void setCentering(const ReflectionCondition_sptr &centering);
    ReflectionCondition_sptr centering() const;

    CompositeScatterer_sptr getScatterers() const;
    void setScatterers(const CompositeScatterer_sptr &scatterers);

    std::vector<Kernel::V3D> getHKLs(double dMin, double dMax, ReflectionConditionMethod method = UseCentering) const;
    std::vector<Kernel::V3D> getUniqueHKLs(double dMin, double dMax, ReflectionConditionMethod method = UseCentering) const;

    std::vector<double> getDValues(const std::vector<Kernel::V3D> &hkls) const;
    std::vector<double> getFSquared(const std::vector<Kernel::V3D> &hkls) const;

protected:
    void setPointGroupFromSpaceGroup(const SpaceGroup_const_sptr &spaceGroup);
    void setReflectionConditionFromSpaceGroup(const SpaceGroup_const_sptr &spaceGroup);

    void assignSpaceGroupToScatterers(const SpaceGroup_const_sptr &spaceGroup);
    void assignUnitCellToScatterers(const UnitCell &unitCell);

    void initializeScatterers();

    bool isStateSufficientForHKLGeneration(ReflectionConditionMethod method) const;
    bool isStateSufficientForUniqueHKLGeneration(ReflectionConditionMethod method) const;

    void throwIfRangeUnacceptable(double dMin, double dMax) const;

    bool isAllowed(const Kernel::V3D &hkl, ReflectionConditionMethod method) const;

    UnitCell m_cell;
    SpaceGroup_const_sptr m_spaceGroup;
    CompositeScatterer_sptr m_scatterers;
    PointGroup_sptr m_pointGroup;
    ReflectionCondition_sptr m_centering;
    
};

typedef boost::shared_ptr<CrystalStructure> CrystalStructure_sptr;


} // namespace Geometry
} // namespace Mantid

#endif  /* MANTID_GEOMETRY_CRYSTALSTRUCTURE_H_ */
